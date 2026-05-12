// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/runtime_upload.hpp"

#include "mirakana/assets/asset_source_format.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mirakana::runtime_rhi {
namespace {

[[nodiscard]] RuntimeTextureUploadResult texture_upload_failure(std::string diagnostic) {
    RuntimeTextureUploadResult result;
    result.diagnostic = std::move(diagnostic);
    return result;
}

[[nodiscard]] RuntimeMeshUploadResult mesh_upload_failure(std::string diagnostic) {
    RuntimeMeshUploadResult result;
    result.diagnostic = std::move(diagnostic);
    return result;
}

[[nodiscard]] RuntimeSkinnedMeshUploadResult skinned_upload_failure(std::string diagnostic) {
    RuntimeSkinnedMeshUploadResult result;
    result.diagnostic = std::move(diagnostic);
    return result;
}

[[nodiscard]] RuntimeMorphMeshUploadResult morph_upload_failure(std::string diagnostic) {
    RuntimeMorphMeshUploadResult result;
    result.diagnostic = std::move(diagnostic);
    return result;
}

[[nodiscard]] RuntimeMorphMeshComputeBinding morph_compute_binding_failure(std::string diagnostic) {
    RuntimeMorphMeshComputeBinding result;
    result.diagnostic = std::move(diagnostic);
    return result;
}

[[nodiscard]] RuntimeMorphMeshComputeOutputSlot
runtime_morph_compute_output_slot(const RuntimeMorphMeshComputeBinding& binding,
                                  std::uint32_t output_slot_index) noexcept {
    if (!binding.output_slots.empty()) {
        if (output_slot_index >= binding.output_slots.size()) {
            return RuntimeMorphMeshComputeOutputSlot{};
        }
        return binding.output_slots[output_slot_index];
    }

    if (output_slot_index != 0 || binding.output_position_buffer.value == 0) {
        return RuntimeMorphMeshComputeOutputSlot{};
    }

    return RuntimeMorphMeshComputeOutputSlot{
        .output_position_buffer = binding.output_position_buffer,
        .output_normal_buffer = binding.output_normal_buffer,
        .output_tangent_buffer = binding.output_tangent_buffer,
        .output_position_buffer_desc = binding.output_position_buffer_desc,
        .output_normal_buffer_desc = binding.output_normal_buffer_desc,
        .output_tangent_buffer_desc = binding.output_tangent_buffer_desc,
        .output_position_bytes = binding.output_position_bytes,
        .output_normal_bytes = binding.output_normal_bytes,
        .output_tangent_bytes = binding.output_tangent_bytes,
    };
}

[[nodiscard]] RuntimeMeshVertexLayoutDesc mesh_layout_failure(std::string diagnostic) {
    RuntimeMeshVertexLayoutDesc result;
    result.diagnostic = std::move(diagnostic);
    return result;
}

[[nodiscard]] RuntimeMaterialGpuBinding material_binding_failure(std::string diagnostic) {
    RuntimeMaterialGpuBinding result;
    result.diagnostic = std::move(diagnostic);
    return result;
}

[[nodiscard]] RuntimeMaterialDescriptorSetLayoutDescResult material_layout_desc_failure(std::string diagnostic) {
    return RuntimeMaterialDescriptorSetLayoutDescResult{.desc = {}, .diagnostic = std::move(diagnostic)};
}

struct TextureUploadStaging {
    std::vector<std::uint8_t> bytes;
    rhi::BufferTextureCopyRegion region;
};

[[nodiscard]] std::uint64_t align_up(std::uint64_t value, std::uint64_t alignment) {
    if (alignment == 0) {
        throw std::invalid_argument("runtime texture upload alignment must be non-zero");
    }
    const auto remainder = value % alignment;
    if (remainder == 0) {
        return value;
    }
    const auto padding = alignment - remainder;
    if (padding > (std::numeric_limits<std::uint64_t>::max)() - value) {
        throw std::overflow_error("runtime texture upload row pitch overflowed");
    }
    return value + padding;
}

[[nodiscard]] TextureUploadStaging make_texture_upload_staging(rhi::Format format, std::uint32_t width,
                                                               std::uint32_t height,
                                                               const std::vector<std::uint8_t>& source_bytes) {
    constexpr std::uint64_t row_pitch_alignment = 256;
    const auto texel_bytes = static_cast<std::uint64_t>(rhi::bytes_per_texel(format));
    const auto source_row_bytes = static_cast<std::uint64_t>(width) * texel_bytes;
    const auto staged_row_bytes = align_up(source_row_bytes, row_pitch_alignment);
    if (staged_row_bytes % texel_bytes != 0) {
        throw std::invalid_argument("runtime texture upload row pitch is not texel aligned");
    }

    const auto row_length = staged_row_bytes / texel_bytes;
    if (row_length > (std::numeric_limits<std::uint32_t>::max)()) {
        throw std::overflow_error("runtime texture upload row length overflowed");
    }
    const auto staged_bytes = staged_row_bytes * static_cast<std::uint64_t>(height);
    if (staged_bytes > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
        throw std::overflow_error("runtime texture upload byte count overflowed");
    }

    TextureUploadStaging staging;
    staging.bytes.resize(static_cast<std::size_t>(staged_bytes), std::uint8_t{0});
    const auto source_row_size = static_cast<std::size_t>(source_row_bytes);
    const auto staged_row_size = static_cast<std::size_t>(staged_row_bytes);
    for (std::uint32_t row = 0; row < height; ++row) {
        const auto source_offset = static_cast<std::size_t>(row) * source_row_size;
        const auto staged_offset = static_cast<std::size_t>(row) * staged_row_size;
        std::copy_n(source_bytes.begin() + static_cast<std::ptrdiff_t>(source_offset), source_row_size,
                    staging.bytes.begin() + static_cast<std::ptrdiff_t>(staged_offset));
    }
    staging.region = rhi::BufferTextureCopyRegion{
        .buffer_offset = 0,
        .buffer_row_length = static_cast<std::uint32_t>(row_length),
        .buffer_image_height = 0,
        .texture_offset = rhi::Offset3D{},
        .texture_extent = rhi::Extent3D{.width = width, .height = height, .depth = 1},
    };
    return staging;
}

[[nodiscard]] rhi::Format runtime_texture_format(TextureSourcePixelFormat format) {
    switch (format) {
    case TextureSourcePixelFormat::rgba8_unorm:
        return rhi::Format::rgba8_unorm;
    case TextureSourcePixelFormat::r8_unorm:
    case TextureSourcePixelFormat::rg8_unorm:
    case TextureSourcePixelFormat::unknown:
        break;
    }
    throw std::invalid_argument("runtime texture payload format is not supported by the RHI upload bridge");
}

[[nodiscard]] rhi::ShaderStageVisibility runtime_shader_visibility(MaterialShaderStageMask stages) noexcept {
    auto visibility = rhi::ShaderStageVisibility::none;
    const auto bits = static_cast<std::uint32_t>(stages);
    if ((bits & static_cast<std::uint32_t>(MaterialShaderStageMask::vertex)) != 0U) {
        visibility = visibility | rhi::ShaderStageVisibility::vertex;
    }
    if ((bits & static_cast<std::uint32_t>(MaterialShaderStageMask::fragment)) != 0U) {
        visibility = visibility | rhi::ShaderStageVisibility::fragment;
    }
    return visibility;
}

[[nodiscard]] rhi::DescriptorType runtime_descriptor_type(MaterialBindingResourceKind kind) {
    switch (kind) {
    case MaterialBindingResourceKind::uniform_buffer:
        return rhi::DescriptorType::uniform_buffer;
    case MaterialBindingResourceKind::sampled_texture:
        return rhi::DescriptorType::sampled_texture;
    case MaterialBindingResourceKind::sampler:
        return rhi::DescriptorType::sampler;
    case MaterialBindingResourceKind::unknown:
        break;
    }
    throw std::invalid_argument("runtime material binding resource kind is unsupported");
}

[[nodiscard]] std::uint64_t runtime_mesh_index_stride(rhi::IndexFormat format) {
    switch (format) {
    case rhi::IndexFormat::uint16:
        return 2;
    case rhi::IndexFormat::uint32:
        return 4;
    case rhi::IndexFormat::unknown:
        break;
    }
    throw std::invalid_argument("runtime mesh index format is invalid");
}

[[nodiscard]] std::uint64_t checked_runtime_mesh_bytes(std::uint32_t count, std::uint64_t stride,
                                                       const char* diagnostic) {
    if (stride == 0) {
        throw std::invalid_argument(diagnostic);
    }
    if (count > (std::numeric_limits<std::uint64_t>::max)() / stride) {
        throw std::overflow_error("runtime mesh byte count overflowed");
    }
    return static_cast<std::uint64_t>(count) * stride;
}

[[nodiscard]] rhi::BufferUsage ensure_buffer_usage(rhi::BufferUsage usage, rhi::BufferUsage required) noexcept {
    if (!rhi::has_flag(usage, required)) {
        return usage | required;
    }
    return usage;
}

[[nodiscard]] const RuntimeMaterialTextureResource*
find_texture_resource(const std::vector<RuntimeMaterialTextureResource>& textures, MaterialTextureSlot slot) noexcept {
    const auto found = std::ranges::find_if(
        textures, [slot](const RuntimeMaterialTextureResource& item) { return item.slot == slot; });
    return found == textures.end() ? nullptr : &*found;
}

[[nodiscard]] std::array<std::uint8_t, runtime_material_uniform_buffer_size_bytes>
pack_runtime_material_factors(const MaterialFactors& factors, MaterialShadingModel shading) {
    std::array<std::uint8_t, runtime_material_uniform_buffer_size_bytes> bytes{};
    const auto write_float = [&bytes](std::size_t offset, float value) {
        std::memcpy(&bytes[offset], &value, sizeof(float));
    };

    write_float(0, factors.base_color[0]);
    write_float(4, factors.base_color[1]);
    write_float(8, factors.base_color[2]);
    write_float(12, factors.base_color[3]);
    write_float(16, factors.emissive[0]);
    write_float(20, factors.emissive[1]);
    write_float(24, factors.emissive[2]);
    write_float(28, factors.metallic);
    write_float(32, factors.roughness);
    write_float(36, shading == MaterialShadingModel::lit ? 1.0F : 0.0F);
    return bytes;
}

} // namespace

RuntimeTextureUploadResult upload_runtime_texture(rhi::IRhiDevice& device,
                                                  const runtime::RuntimeTexturePayload& payload,
                                                  const RuntimeTextureUploadOptions& options) {
    try {
        const TextureSourceDocument document{
            .width = payload.width, .height = payload.height, .pixel_format = payload.pixel_format};
        if (!is_valid_texture_source_document(document)) {
            return texture_upload_failure("runtime texture payload is invalid");
        }
        if (payload.source_bytes != texture_source_uncompressed_bytes(document)) {
            return texture_upload_failure("runtime texture payload source byte count is invalid");
        }
        if (!payload.bytes.empty() && payload.bytes.size() != payload.source_bytes) {
            return texture_upload_failure("runtime texture payload byte data size is invalid");
        }

        const auto format = runtime_texture_format(payload.pixel_format);
        auto usage = options.usage;
        if (!rhi::has_flag(usage, rhi::TextureUsage::shader_resource)) {
            usage = usage | rhi::TextureUsage::shader_resource;
        }
        if (!payload.bytes.empty() && !rhi::has_flag(usage, rhi::TextureUsage::copy_destination)) {
            usage = usage | rhi::TextureUsage::copy_destination;
        }

        const auto desc =
            rhi::TextureDesc{.extent = rhi::Extent3D{.width = payload.width, .height = payload.height, .depth = 1},
                             .format = format,
                             .usage = usage};
        const auto staging =
            !payload.bytes.empty()
                ? make_texture_upload_staging(format, payload.width, payload.height, payload.bytes)
                : TextureUploadStaging{
                      .bytes = {},
                      .region = rhi::BufferTextureCopyRegion{
                          .buffer_offset = 0,
                          .buffer_row_length = 0,
                          .buffer_image_height = 0,
                          .texture_offset = rhi::Offset3D{},
                          .texture_extent = rhi::Extent3D{.width = payload.width, .height = payload.height, .depth = 1},
                      }};

        const auto texture = device.create_texture(desc);
        if (payload.bytes.empty()) {
            return RuntimeTextureUploadResult{.texture = texture,
                                              .upload_buffer = {},
                                              .texture_desc = desc,
                                              .copy_region = staging.region,
                                              .uploaded_bytes = 0,
                                              .owner_device = &device,
                                              .copy_recorded = false,
                                              .diagnostic = {},
                                              .submitted_fence = {}};
        }

        const auto upload_buffer = device.create_buffer(rhi::BufferDesc{
            .size_bytes = static_cast<std::uint64_t>(staging.bytes.size()), .usage = rhi::BufferUsage::copy_source});
        device.write_buffer(upload_buffer, 0, staging.bytes);
        auto commands = device.begin_command_list(options.queue);
        commands->transition_texture(texture, rhi::ResourceState::undefined, rhi::ResourceState::copy_destination);
        commands->copy_buffer_to_texture(upload_buffer, texture, staging.region);
        commands->transition_texture(texture, rhi::ResourceState::copy_destination, rhi::ResourceState::shader_read);
        commands->close();
        const auto fence = device.submit(*commands);
        if (options.wait_for_completion) {
            device.wait(fence);
        }

        return RuntimeTextureUploadResult{
            .texture = texture,
            .upload_buffer = upload_buffer,
            .texture_desc = desc,
            .copy_region = staging.region,
            .uploaded_bytes = static_cast<std::uint64_t>(staging.bytes.size()),
            .owner_device = &device,
            .copy_recorded = true,
            .diagnostic = {},
            .submitted_fence = fence,
        };
    } catch (const std::exception& error) {
        return texture_upload_failure(error.what());
    }
}

RuntimeMeshUploadResult upload_runtime_mesh(rhi::IRhiDevice& device, const runtime::RuntimeMeshPayload& payload,
                                            const RuntimeMeshUploadOptions& options) {
    try {
        const MeshSourceDocument document{
            .vertex_count = payload.vertex_count,
            .index_count = payload.index_count,
            .has_normals = payload.has_normals,
            .has_uvs = payload.has_uvs,
            .has_tangent_frame = payload.has_tangent_frame,
            .vertex_bytes = payload.vertex_bytes,
            .index_bytes = payload.index_bytes,
        };
        if (!is_valid_mesh_source_document(document)) {
            return mesh_upload_failure("runtime mesh payload is invalid");
        }
        if (payload.vertex_bytes.empty() || payload.index_bytes.empty()) {
            return mesh_upload_failure("runtime mesh payload byte data is required");
        }
        const auto payload_layout = make_runtime_mesh_vertex_layout_desc(payload);
        if (!payload_layout.succeeded()) {
            return mesh_upload_failure(payload_layout.diagnostic);
        }
        const auto upload_options = make_runtime_mesh_upload_options(payload, options);
        if (upload_options.vertex_stride == 0) {
            return mesh_upload_failure("runtime mesh vertex layout is invalid");
        }
        const auto expected_vertex_size = checked_runtime_mesh_bytes(payload.vertex_count, upload_options.vertex_stride,
                                                                     "runtime mesh vertex stride is invalid");
        if (static_cast<std::uint64_t>(payload.vertex_bytes.size()) != expected_vertex_size) {
            return mesh_upload_failure("runtime mesh payload byte data does not match vertex stride");
        }
        const auto index_stride = runtime_mesh_index_stride(upload_options.index_format);
        const auto expected_index_size =
            checked_runtime_mesh_bytes(payload.index_count, index_stride, "runtime mesh index format is invalid");
        if (static_cast<std::uint64_t>(payload.index_bytes.size()) != expected_index_size) {
            return mesh_upload_failure("runtime mesh payload byte data does not match index format");
        }

        auto vertex_usage = upload_options.vertex_usage;
        if (!rhi::has_flag(vertex_usage, rhi::BufferUsage::vertex)) {
            vertex_usage = vertex_usage | rhi::BufferUsage::vertex;
        }
        if (!rhi::has_flag(vertex_usage, rhi::BufferUsage::copy_destination)) {
            vertex_usage = vertex_usage | rhi::BufferUsage::copy_destination;
        }

        auto index_usage = upload_options.index_usage;
        if (!rhi::has_flag(index_usage, rhi::BufferUsage::index)) {
            index_usage = index_usage | rhi::BufferUsage::index;
        }
        if (!rhi::has_flag(index_usage, rhi::BufferUsage::copy_destination)) {
            index_usage = index_usage | rhi::BufferUsage::copy_destination;
        }

        const auto vertex_size = static_cast<std::uint64_t>(payload.vertex_bytes.size());
        const auto index_size = static_cast<std::uint64_t>(payload.index_bytes.size());
        const auto vertex_desc = rhi::BufferDesc{.size_bytes = vertex_size, .usage = vertex_usage};
        const auto index_desc = rhi::BufferDesc{.size_bytes = index_size, .usage = index_usage};
        const auto vertex_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = vertex_size};
        const auto index_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = index_size};

        const auto vertex_buffer = device.create_buffer(vertex_desc);
        const auto index_buffer = device.create_buffer(index_desc);
        const auto vertex_upload_buffer =
            device.create_buffer(rhi::BufferDesc{.size_bytes = vertex_size, .usage = rhi::BufferUsage::copy_source});
        const auto index_upload_buffer =
            device.create_buffer(rhi::BufferDesc{.size_bytes = index_size, .usage = rhi::BufferUsage::copy_source});
        device.write_buffer(vertex_upload_buffer, 0, payload.vertex_bytes);
        device.write_buffer(index_upload_buffer, 0, payload.index_bytes);

        auto commands = device.begin_command_list(upload_options.queue);
        commands->copy_buffer(vertex_upload_buffer, vertex_buffer, vertex_region);
        commands->copy_buffer(index_upload_buffer, index_buffer, index_region);
        commands->close();
        const auto fence = device.submit(*commands);
        if (upload_options.wait_for_completion) {
            device.wait(fence);
        }

        return RuntimeMeshUploadResult{
            .vertex_buffer = vertex_buffer,
            .index_buffer = index_buffer,
            .vertex_upload_buffer = vertex_upload_buffer,
            .index_upload_buffer = index_upload_buffer,
            .vertex_buffer_desc = vertex_desc,
            .index_buffer_desc = index_desc,
            .vertex_copy_region = vertex_region,
            .index_copy_region = index_region,
            .vertex_stride = upload_options.vertex_stride,
            .index_format = upload_options.index_format,
            .owner_device = &device,
            .vertex_count = payload.vertex_count,
            .index_count = payload.index_count,
            .uploaded_vertex_bytes = vertex_size,
            .uploaded_index_bytes = index_size,
            .copy_recorded = true,
            .diagnostic = {},
            .submitted_fence = fence,
        };
    } catch (const std::exception& error) {
        return mesh_upload_failure(error.what());
    }
}

MeshGpuBinding make_runtime_mesh_gpu_binding(const RuntimeMeshUploadResult& upload) noexcept {
    if (!upload.succeeded()) {
        return MeshGpuBinding{};
    }
    return MeshGpuBinding{
        .vertex_buffer = upload.vertex_buffer,
        .index_buffer = upload.index_buffer,
        .vertex_count = upload.vertex_count,
        .index_count = upload.index_count,
        .vertex_offset = 0,
        .index_offset = 0,
        .vertex_stride = upload.vertex_stride,
        .index_format = upload.index_format,
        .owner_device = upload.owner_device,
    };
}

RuntimeMeshVertexLayoutDesc
make_runtime_skinned_mesh_vertex_layout_desc(const runtime::RuntimeSkinnedMeshPayload& payload) {
    (void)payload;
    return RuntimeMeshVertexLayoutDesc{
        .layout = RuntimeMeshVertexLayout::skinned_mesh_tangent_space,
        .vertex_stride = runtime_skinned_mesh_vertex_stride_bytes,
        .vertex_buffers = {rhi::VertexBufferLayoutDesc{.binding = 0,
                                                       .stride = runtime_skinned_mesh_vertex_stride_bytes,
                                                       .input_rate = rhi::VertexInputRate::vertex}},
        .vertex_attributes =
            {
                rhi::VertexAttributeDesc{.location = 0,
                                         .binding = 0,
                                         .offset = 0,
                                         .format = rhi::VertexFormat::float32x3,
                                         .semantic = rhi::VertexSemantic::position,
                                         .semantic_index = 0},
                rhi::VertexAttributeDesc{.location = 1,
                                         .binding = 0,
                                         .offset = 12,
                                         .format = rhi::VertexFormat::float32x3,
                                         .semantic = rhi::VertexSemantic::normal,
                                         .semantic_index = 0},
                rhi::VertexAttributeDesc{.location = 2,
                                         .binding = 0,
                                         .offset = 24,
                                         .format = rhi::VertexFormat::float32x2,
                                         .semantic = rhi::VertexSemantic::texcoord,
                                         .semantic_index = 0},
                rhi::VertexAttributeDesc{.location = 3,
                                         .binding = 0,
                                         .offset = 32,
                                         .format = rhi::VertexFormat::float32x4,
                                         .semantic = rhi::VertexSemantic::tangent,
                                         .semantic_index = 0},
                rhi::VertexAttributeDesc{.location = 4,
                                         .binding = 0,
                                         .offset = 48,
                                         .format = rhi::VertexFormat::uint16x4,
                                         .semantic = rhi::VertexSemantic::joint_indices,
                                         .semantic_index = 0},
                rhi::VertexAttributeDesc{.location = 5,
                                         .binding = 0,
                                         .offset = 56,
                                         .format = rhi::VertexFormat::float32x4,
                                         .semantic = rhi::VertexSemantic::joint_weights,
                                         .semantic_index = 0},
            },
        .diagnostic = {},
    };
}

RuntimeSkinnedMeshUploadResult upload_runtime_skinned_mesh(rhi::IRhiDevice& device,
                                                           const runtime::RuntimeSkinnedMeshPayload& payload,
                                                           const RuntimeSkinnedMeshUploadOptions& options) {
    try {
        if (payload.joint_count == 0U) {
            return skinned_upload_failure("runtime skinned mesh joint_count must be non-zero");
        }
        if (payload.joint_count > runtime_skinned_mesh_max_joints) {
            return skinned_upload_failure("runtime skinned mesh joint_count exceeds maximum supported joints");
        }
        if (options.vertex_stride != runtime_skinned_mesh_vertex_stride_bytes) {
            return skinned_upload_failure("runtime skinned mesh vertex stride is invalid");
        }
        if (payload.vertex_bytes.empty() || payload.index_bytes.empty() || payload.joint_palette_bytes.empty()) {
            return skinned_upload_failure("runtime skinned mesh payload byte data is required");
        }
        const auto expected_vertex_size = checked_runtime_mesh_bytes(payload.vertex_count, options.vertex_stride,
                                                                     "runtime skinned mesh vertex stride is invalid");
        if (static_cast<std::uint64_t>(payload.vertex_bytes.size()) != expected_vertex_size) {
            return skinned_upload_failure("runtime skinned mesh vertex bytes do not match vertex_count");
        }
        const auto index_stride = runtime_mesh_index_stride(options.index_format);
        const auto expected_index_size = checked_runtime_mesh_bytes(payload.index_count, index_stride,
                                                                    "runtime skinned mesh index format is invalid");
        if (static_cast<std::uint64_t>(payload.index_bytes.size()) != expected_index_size) {
            return skinned_upload_failure("runtime skinned mesh index bytes do not match index format");
        }
        const auto expected_palette_raw = static_cast<std::uint64_t>(payload.joint_count) *
                                          static_cast<std::uint64_t>(runtime_skinned_mesh_joint_matrix_bytes);
        if (static_cast<std::uint64_t>(payload.joint_palette_bytes.size()) != expected_palette_raw) {
            return skinned_upload_failure("runtime skinned mesh joint_palette_bytes size is invalid");
        }
        constexpr std::uint64_t k_cbv_alignment_bytes = 256;
        const auto aligned_palette_size = align_up(expected_palette_raw, k_cbv_alignment_bytes);
        if (aligned_palette_size > 65536ULL) {
            return skinned_upload_failure("runtime skinned mesh joint palette exceeds constant buffer limit");
        }

        auto vertex_usage = options.vertex_usage;
        if (!rhi::has_flag(vertex_usage, rhi::BufferUsage::vertex)) {
            vertex_usage = vertex_usage | rhi::BufferUsage::vertex;
        }
        if (!rhi::has_flag(vertex_usage, rhi::BufferUsage::copy_destination)) {
            vertex_usage = vertex_usage | rhi::BufferUsage::copy_destination;
        }

        auto index_usage = options.index_usage;
        if (!rhi::has_flag(index_usage, rhi::BufferUsage::index)) {
            index_usage = index_usage | rhi::BufferUsage::index;
        }
        if (!rhi::has_flag(index_usage, rhi::BufferUsage::copy_destination)) {
            index_usage = index_usage | rhi::BufferUsage::copy_destination;
        }

        auto palette_usage = options.palette_usage;
        if (!rhi::has_flag(palette_usage, rhi::BufferUsage::uniform)) {
            palette_usage = palette_usage | rhi::BufferUsage::uniform;
        }
        if (!rhi::has_flag(palette_usage, rhi::BufferUsage::copy_destination)) {
            palette_usage = palette_usage | rhi::BufferUsage::copy_destination;
        }

        const auto vertex_size = static_cast<std::uint64_t>(payload.vertex_bytes.size());
        const auto index_size = static_cast<std::uint64_t>(payload.index_bytes.size());
        const auto vertex_desc = rhi::BufferDesc{.size_bytes = vertex_size, .usage = vertex_usage};
        const auto index_desc = rhi::BufferDesc{.size_bytes = index_size, .usage = index_usage};
        const auto joint_palette_desc = rhi::BufferDesc{.size_bytes = aligned_palette_size, .usage = palette_usage};
        const auto vertex_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = vertex_size};
        const auto index_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = index_size};
        const auto joint_palette_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = aligned_palette_size};

        const auto vertex_buffer = device.create_buffer(vertex_desc);
        const auto index_buffer = device.create_buffer(index_desc);
        const auto joint_palette_buffer = device.create_buffer(joint_palette_desc);
        const auto vertex_upload_buffer =
            device.create_buffer(rhi::BufferDesc{.size_bytes = vertex_size, .usage = rhi::BufferUsage::copy_source});
        const auto index_upload_buffer =
            device.create_buffer(rhi::BufferDesc{.size_bytes = index_size, .usage = rhi::BufferUsage::copy_source});
        const auto joint_palette_upload_buffer = device.create_buffer(
            rhi::BufferDesc{.size_bytes = aligned_palette_size, .usage = rhi::BufferUsage::copy_source});

        device.write_buffer(vertex_upload_buffer, 0, payload.vertex_bytes);
        device.write_buffer(index_upload_buffer, 0, payload.index_bytes);
        std::vector<std::uint8_t> padded_palette(static_cast<std::size_t>(aligned_palette_size), std::uint8_t{0});
        std::ranges::copy(payload.joint_palette_bytes, padded_palette.begin());
        device.write_buffer(joint_palette_upload_buffer, 0, padded_palette);

        auto commands = device.begin_command_list(options.queue);
        commands->copy_buffer(vertex_upload_buffer, vertex_buffer, vertex_region);
        commands->copy_buffer(index_upload_buffer, index_buffer, index_region);
        commands->copy_buffer(joint_palette_upload_buffer, joint_palette_buffer, joint_palette_region);
        commands->close();
        const auto fence = device.submit(*commands);
        if (options.wait_for_completion) {
            device.wait(fence);
        }

        return RuntimeSkinnedMeshUploadResult{
            .vertex_buffer = vertex_buffer,
            .index_buffer = index_buffer,
            .joint_palette_buffer = joint_palette_buffer,
            .vertex_upload_buffer = vertex_upload_buffer,
            .index_upload_buffer = index_upload_buffer,
            .joint_palette_upload_buffer = joint_palette_upload_buffer,
            .vertex_buffer_desc = vertex_desc,
            .index_buffer_desc = index_desc,
            .joint_palette_buffer_desc = joint_palette_desc,
            .vertex_copy_region = vertex_region,
            .index_copy_region = index_region,
            .joint_palette_copy_region = joint_palette_region,
            .vertex_stride = options.vertex_stride,
            .index_format = options.index_format,
            .vertex_count = payload.vertex_count,
            .index_count = payload.index_count,
            .joint_count = payload.joint_count,
            .joint_palette_uniform_allocation_bytes = aligned_palette_size,
            .owner_device = &device,
            .uploaded_vertex_bytes = vertex_size,
            .uploaded_index_bytes = index_size,
            .uploaded_joint_palette_bytes = aligned_palette_size,
            .copy_recorded = true,
            .diagnostic = {},
            .submitted_fence = fence,
        };
    } catch (const std::exception& error) {
        return skinned_upload_failure(error.what());
    }
}

SkinnedMeshGpuBinding make_runtime_skinned_mesh_gpu_binding(const RuntimeSkinnedMeshUploadResult& upload) noexcept {
    if (!upload.succeeded()) {
        return SkinnedMeshGpuBinding{};
    }
    const MeshGpuBinding mesh{
        .vertex_buffer = upload.vertex_buffer,
        .index_buffer = upload.index_buffer,
        .vertex_count = upload.vertex_count,
        .index_count = upload.index_count,
        .vertex_offset = 0,
        .index_offset = 0,
        .vertex_stride = upload.vertex_stride,
        .index_format = upload.index_format,
        .owner_device = upload.owner_device,
    };
    return SkinnedMeshGpuBinding{
        .mesh = mesh,
        .joint_palette_buffer = upload.joint_palette_buffer,
        .joint_palette_upload_buffer = upload.joint_palette_upload_buffer,
        .joint_descriptor_set = {},
        .joint_count = upload.joint_count,
        .joint_palette_uniform_allocation_bytes = upload.joint_palette_uniform_allocation_bytes,
        .owner_device = upload.owner_device,
    };
}

RuntimeMorphMeshUploadResult upload_runtime_morph_mesh_cpu(rhi::IRhiDevice& device,
                                                           const runtime::RuntimeMorphMeshCpuPayload& payload,
                                                           const RuntimeMorphMeshUploadOptions& options) {
    try {
        if (!is_valid_morph_mesh_cpu_source_document(payload.morph)) {
            return morph_upload_failure("runtime morph mesh CPU payload is invalid");
        }
        if (payload.morph.vertex_count == 0U || payload.morph.targets.empty()) {
            return morph_upload_failure("runtime morph mesh CPU payload requires vertices and targets");
        }
        if (payload.morph.targets.size() > (std::numeric_limits<std::uint32_t>::max)()) {
            return morph_upload_failure("runtime morph mesh CPU target count exceeds supported range");
        }

        const auto target_count = static_cast<std::uint32_t>(payload.morph.targets.size());
        const auto position_delta_bytes_per_target =
            checked_runtime_mesh_bytes(payload.morph.vertex_count, runtime_morph_position_delta_stride_bytes,
                                       "runtime morph mesh position delta stride is invalid");
        const auto normal_delta_bytes_per_target =
            checked_runtime_mesh_bytes(payload.morph.vertex_count, runtime_morph_normal_delta_stride_bytes,
                                       "runtime morph mesh normal delta stride is invalid");
        const auto tangent_delta_bytes_per_target =
            checked_runtime_mesh_bytes(payload.morph.vertex_count, runtime_morph_tangent_delta_stride_bytes,
                                       "runtime morph mesh tangent delta stride is invalid");
        const auto checked_delta_stream_size = [target_count](std::uint64_t bytes_per_target,
                                                              const char* overflow_diagnostic,
                                                              const char* host_size_diagnostic) -> std::uint64_t {
            if (target_count > (std::numeric_limits<std::uint64_t>::max)() / bytes_per_target) {
                throw std::overflow_error(overflow_diagnostic);
            }
            const auto size = bytes_per_target * static_cast<std::uint64_t>(target_count);
            if (size > static_cast<std::uint64_t>((std::numeric_limits<std::size_t>::max)())) {
                throw std::overflow_error(host_size_diagnostic);
            }
            return size;
        };
        const auto position_delta_size = checked_delta_stream_size(
            position_delta_bytes_per_target, "runtime morph mesh position delta byte count overflowed",
            "runtime morph mesh position delta byte count exceeds host size range");

        const bool has_normal_deltas = std::ranges::any_of(
            payload.morph.targets, [](const auto& target) { return !target.normal_delta_bytes.empty(); });
        const bool has_tangent_deltas = std::ranges::any_of(
            payload.morph.targets, [](const auto& target) { return !target.tangent_delta_bytes.empty(); });
        const auto normal_delta_size =
            has_normal_deltas
                ? checked_delta_stream_size(normal_delta_bytes_per_target,
                                            "runtime morph mesh normal delta byte count overflowed",
                                            "runtime morph mesh normal delta byte count exceeds host size range")
                : 0U;
        const auto tangent_delta_size =
            has_tangent_deltas
                ? checked_delta_stream_size(tangent_delta_bytes_per_target,
                                            "runtime morph mesh tangent delta byte count overflowed",
                                            "runtime morph mesh tangent delta byte count exceeds host size range")
                : 0U;

        std::vector<std::uint8_t> position_delta_bytes;
        position_delta_bytes.reserve(static_cast<std::size_t>(position_delta_size));
        std::vector<std::uint8_t> normal_delta_bytes;
        normal_delta_bytes.reserve(static_cast<std::size_t>(normal_delta_size));
        std::vector<std::uint8_t> tangent_delta_bytes;
        tangent_delta_bytes.reserve(static_cast<std::size_t>(tangent_delta_size));
        const std::vector<std::uint8_t> zero_normal_delta(static_cast<std::size_t>(normal_delta_bytes_per_target),
                                                          std::uint8_t{0});
        const std::vector<std::uint8_t> zero_tangent_delta(static_cast<std::size_t>(tangent_delta_bytes_per_target),
                                                           std::uint8_t{0});
        for (const auto& target : payload.morph.targets) {
            if (target.position_delta_bytes.empty()) {
                return morph_upload_failure("runtime morph mesh GPU upload requires POSITION delta bytes");
            }
            if (static_cast<std::uint64_t>(target.position_delta_bytes.size()) != position_delta_bytes_per_target) {
                return morph_upload_failure("runtime morph mesh POSITION delta byte size is invalid");
            }
            position_delta_bytes.insert(position_delta_bytes.end(), target.position_delta_bytes.begin(),
                                        target.position_delta_bytes.end());
            if (has_normal_deltas) {
                if (!target.normal_delta_bytes.empty()) {
                    if (static_cast<std::uint64_t>(target.normal_delta_bytes.size()) != normal_delta_bytes_per_target) {
                        return morph_upload_failure("runtime morph mesh NORMAL delta byte size is invalid");
                    }
                    normal_delta_bytes.insert(normal_delta_bytes.end(), target.normal_delta_bytes.begin(),
                                              target.normal_delta_bytes.end());
                } else {
                    normal_delta_bytes.insert(normal_delta_bytes.end(), zero_normal_delta.begin(),
                                              zero_normal_delta.end());
                }
            }
            if (has_tangent_deltas) {
                if (!target.tangent_delta_bytes.empty()) {
                    if (static_cast<std::uint64_t>(target.tangent_delta_bytes.size()) !=
                        tangent_delta_bytes_per_target) {
                        return morph_upload_failure("runtime morph mesh TANGENT delta byte size is invalid");
                    }
                    tangent_delta_bytes.insert(tangent_delta_bytes.end(), target.tangent_delta_bytes.begin(),
                                               target.tangent_delta_bytes.end());
                } else {
                    tangent_delta_bytes.insert(tangent_delta_bytes.end(), zero_tangent_delta.begin(),
                                               zero_tangent_delta.end());
                }
            }
        }

        const auto weight_raw_size = static_cast<std::uint64_t>(target_count) * sizeof(float);
        if (static_cast<std::uint64_t>(payload.morph.target_weight_bytes.size()) != weight_raw_size) {
            return morph_upload_failure("runtime morph mesh target weight byte size is invalid");
        }
        const auto aligned_weight_size =
            align_up(weight_raw_size, runtime_morph_weight_uniform_buffer_allocation_size_bytes);
        if (aligned_weight_size > 65536ULL) {
            return morph_upload_failure("runtime morph mesh weights exceed constant buffer limit");
        }

        auto position_delta_usage = ensure_buffer_usage(options.position_delta_usage, rhi::BufferUsage::storage);
        position_delta_usage = ensure_buffer_usage(position_delta_usage, rhi::BufferUsage::copy_destination);
        auto normal_delta_usage = ensure_buffer_usage(options.normal_delta_usage, rhi::BufferUsage::storage);
        normal_delta_usage = ensure_buffer_usage(normal_delta_usage, rhi::BufferUsage::copy_destination);
        auto tangent_delta_usage = ensure_buffer_usage(options.tangent_delta_usage, rhi::BufferUsage::storage);
        tangent_delta_usage = ensure_buffer_usage(tangent_delta_usage, rhi::BufferUsage::copy_destination);
        auto weight_usage = ensure_buffer_usage(options.weight_usage, rhi::BufferUsage::uniform);
        weight_usage = ensure_buffer_usage(weight_usage, rhi::BufferUsage::copy_destination);

        const auto position_delta_desc =
            rhi::BufferDesc{.size_bytes = position_delta_size, .usage = position_delta_usage};
        const auto normal_delta_desc = rhi::BufferDesc{.size_bytes = normal_delta_size, .usage = normal_delta_usage};
        const auto tangent_delta_desc = rhi::BufferDesc{.size_bytes = tangent_delta_size, .usage = tangent_delta_usage};
        const auto morph_weight_desc = rhi::BufferDesc{.size_bytes = aligned_weight_size, .usage = weight_usage};
        const auto position_delta_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = position_delta_size};
        const auto normal_delta_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = normal_delta_size};
        const auto tangent_delta_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = tangent_delta_size};
        const auto morph_weight_region =
            rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = aligned_weight_size};

        const auto position_delta_buffer = device.create_buffer(position_delta_desc);
        const auto normal_delta_buffer =
            has_normal_deltas ? device.create_buffer(normal_delta_desc) : rhi::BufferHandle{};
        const auto tangent_delta_buffer =
            has_tangent_deltas ? device.create_buffer(tangent_delta_desc) : rhi::BufferHandle{};
        const auto morph_weight_buffer = device.create_buffer(morph_weight_desc);
        const auto position_delta_upload_buffer = device.create_buffer(
            rhi::BufferDesc{.size_bytes = position_delta_size, .usage = rhi::BufferUsage::copy_source});
        const auto normal_delta_upload_buffer =
            has_normal_deltas ? device.create_buffer(rhi::BufferDesc{.size_bytes = normal_delta_size,
                                                                     .usage = rhi::BufferUsage::copy_source})
                              : rhi::BufferHandle{};
        const auto tangent_delta_upload_buffer =
            has_tangent_deltas ? device.create_buffer(rhi::BufferDesc{.size_bytes = tangent_delta_size,
                                                                      .usage = rhi::BufferUsage::copy_source})
                               : rhi::BufferHandle{};
        const auto morph_weight_upload_buffer = device.create_buffer(
            rhi::BufferDesc{.size_bytes = aligned_weight_size, .usage = rhi::BufferUsage::copy_source});

        std::vector<std::uint8_t> padded_weights(static_cast<std::size_t>(aligned_weight_size), std::uint8_t{0});
        std::ranges::copy(payload.morph.target_weight_bytes, padded_weights.begin());
        device.write_buffer(position_delta_upload_buffer, 0, position_delta_bytes);
        if (has_normal_deltas) {
            device.write_buffer(normal_delta_upload_buffer, 0, normal_delta_bytes);
        }
        if (has_tangent_deltas) {
            device.write_buffer(tangent_delta_upload_buffer, 0, tangent_delta_bytes);
        }
        device.write_buffer(morph_weight_upload_buffer, 0, padded_weights);

        auto commands = device.begin_command_list(options.queue);
        commands->copy_buffer(position_delta_upload_buffer, position_delta_buffer, position_delta_region);
        if (has_normal_deltas) {
            commands->copy_buffer(normal_delta_upload_buffer, normal_delta_buffer, normal_delta_region);
        }
        if (has_tangent_deltas) {
            commands->copy_buffer(tangent_delta_upload_buffer, tangent_delta_buffer, tangent_delta_region);
        }
        commands->copy_buffer(morph_weight_upload_buffer, morph_weight_buffer, morph_weight_region);
        commands->close();
        const auto fence = device.submit(*commands);
        if (options.wait_for_completion) {
            device.wait(fence);
        }

        return RuntimeMorphMeshUploadResult{
            .position_delta_buffer = position_delta_buffer,
            .normal_delta_buffer = normal_delta_buffer,
            .tangent_delta_buffer = tangent_delta_buffer,
            .morph_weight_buffer = morph_weight_buffer,
            .position_delta_upload_buffer = position_delta_upload_buffer,
            .normal_delta_upload_buffer = normal_delta_upload_buffer,
            .tangent_delta_upload_buffer = tangent_delta_upload_buffer,
            .morph_weight_upload_buffer = morph_weight_upload_buffer,
            .position_delta_buffer_desc = position_delta_desc,
            .normal_delta_buffer_desc = normal_delta_desc,
            .tangent_delta_buffer_desc = tangent_delta_desc,
            .morph_weight_buffer_desc = morph_weight_desc,
            .position_delta_copy_region = position_delta_region,
            .normal_delta_copy_region = normal_delta_region,
            .tangent_delta_copy_region = tangent_delta_region,
            .morph_weight_copy_region = morph_weight_region,
            .vertex_count = payload.morph.vertex_count,
            .target_count = target_count,
            .uploaded_position_delta_bytes = position_delta_size,
            .uploaded_normal_delta_bytes = normal_delta_size,
            .uploaded_tangent_delta_bytes = tangent_delta_size,
            .uploaded_weight_bytes = aligned_weight_size,
            .morph_weight_uniform_allocation_bytes = aligned_weight_size,
            .owner_device = &device,
            .copy_recorded = true,
            .diagnostic = {},
            .submitted_fence = fence,
        };
    } catch (const std::exception& error) {
        return morph_upload_failure(error.what());
    }
}

MorphMeshGpuBinding make_runtime_morph_mesh_gpu_binding(const RuntimeMorphMeshUploadResult& upload) noexcept {
    if (!upload.succeeded()) {
        return MorphMeshGpuBinding{};
    }
    return MorphMeshGpuBinding{
        .position_delta_buffer = upload.position_delta_buffer,
        .normal_delta_buffer = upload.normal_delta_buffer,
        .tangent_delta_buffer = upload.tangent_delta_buffer,
        .position_delta_upload_buffer = upload.position_delta_upload_buffer,
        .normal_delta_upload_buffer = upload.normal_delta_upload_buffer,
        .tangent_delta_upload_buffer = upload.tangent_delta_upload_buffer,
        .morph_weight_buffer = upload.morph_weight_buffer,
        .morph_weight_upload_buffer = upload.morph_weight_upload_buffer,
        .morph_descriptor_set = {},
        .vertex_count = upload.vertex_count,
        .target_count = upload.target_count,
        .position_delta_bytes = upload.uploaded_position_delta_bytes,
        .normal_delta_bytes = upload.uploaded_normal_delta_bytes,
        .tangent_delta_bytes = upload.uploaded_tangent_delta_bytes,
        .morph_weight_uniform_allocation_bytes = upload.morph_weight_uniform_allocation_bytes,
        .owner_device = upload.owner_device,
    };
}

RuntimeMorphMeshComputeBinding
create_runtime_morph_mesh_compute_binding(rhi::IRhiDevice& device, const RuntimeMeshUploadResult& mesh_upload,
                                          const RuntimeMorphMeshUploadResult& morph_upload,
                                          const RuntimeMorphMeshComputeBindingOptions& options) {
    try {
        if (!mesh_upload.succeeded()) {
            return morph_compute_binding_failure("runtime morph compute binding requires a succeeded mesh upload");
        }
        if (!morph_upload.succeeded()) {
            return morph_compute_binding_failure("runtime morph compute binding requires a succeeded morph upload");
        }
        if (mesh_upload.owner_device == nullptr || morph_upload.owner_device == nullptr ||
            mesh_upload.owner_device != &device || morph_upload.owner_device != &device) {
            return morph_compute_binding_failure("runtime morph compute binding requires matching owner rhi device");
        }
        if (mesh_upload.vertex_buffer.value == 0 || morph_upload.position_delta_buffer.value == 0 ||
            morph_upload.morph_weight_buffer.value == 0) {
            return morph_compute_binding_failure("runtime morph compute binding requires mesh and morph gpu buffers");
        }
        if (mesh_upload.vertex_count == 0 || morph_upload.vertex_count == 0 ||
            mesh_upload.vertex_count != morph_upload.vertex_count) {
            return morph_compute_binding_failure(
                "runtime morph compute binding requires matching non-zero vertex counts");
        }
        if (morph_upload.target_count == 0) {
            return morph_compute_binding_failure("runtime morph compute binding requires at least one morph target");
        }
        if (mesh_upload.vertex_stride < runtime_mesh_position_vertex_stride_bytes) {
            return morph_compute_binding_failure(
                "runtime morph compute binding requires runtime mesh vertices with POSITION at offset zero");
        }
        if (!rhi::has_flag(mesh_upload.vertex_buffer_desc.usage, rhi::BufferUsage::storage)) {
            return morph_compute_binding_failure(
                "runtime morph compute binding requires mesh vertex buffer storage usage");
        }
        if (!rhi::has_flag(morph_upload.position_delta_buffer_desc.usage, rhi::BufferUsage::storage)) {
            return morph_compute_binding_failure(
                "runtime morph compute binding requires morph position delta storage usage");
        }
        if (!rhi::has_flag(morph_upload.morph_weight_buffer_desc.usage, rhi::BufferUsage::uniform)) {
            return morph_compute_binding_failure("runtime morph compute binding requires morph weight uniform usage");
        }
        const bool output_normal_requested = options.output_normal_usage != rhi::BufferUsage::none;
        const bool output_tangent_requested = options.output_tangent_usage != rhi::BufferUsage::none;
        if (output_normal_requested) {
            if (morph_upload.normal_delta_buffer.value == 0 || morph_upload.uploaded_normal_delta_bytes == 0) {
                return morph_compute_binding_failure(
                    "runtime morph compute normal output requires morph normal delta buffer");
            }
            if (!rhi::has_flag(morph_upload.normal_delta_buffer_desc.usage, rhi::BufferUsage::storage)) {
                return morph_compute_binding_failure(
                    "runtime morph compute normal output requires morph normal delta storage usage");
            }
        }
        if (output_tangent_requested) {
            if (morph_upload.tangent_delta_buffer.value == 0 || morph_upload.uploaded_tangent_delta_bytes == 0) {
                return morph_compute_binding_failure(
                    "runtime morph compute tangent output requires morph tangent delta buffer");
            }
            if (!rhi::has_flag(morph_upload.tangent_delta_buffer_desc.usage, rhi::BufferUsage::storage)) {
                return morph_compute_binding_failure(
                    "runtime morph compute tangent output requires morph tangent delta storage usage");
            }
        }
        if (options.output_slot_count == 0) {
            return morph_compute_binding_failure("runtime morph compute output ring requires at least one slot");
        }
        if (options.output_slot_count > 1 && (output_normal_requested || output_tangent_requested)) {
            return morph_compute_binding_failure(
                "runtime morph compute output ring currently supports POSITION output slots only");
        }

        const auto output_position_bytes =
            checked_runtime_mesh_bytes(mesh_upload.vertex_count, runtime_morph_position_delta_stride_bytes,
                                       "runtime morph compute output position stride is invalid");
        auto output_usage = ensure_buffer_usage(options.output_position_usage, rhi::BufferUsage::storage);
        output_usage = ensure_buffer_usage(output_usage, rhi::BufferUsage::copy_source);
        const auto output_desc = rhi::BufferDesc{.size_bytes = output_position_bytes, .usage = output_usage};
        const auto output_normal_bytes =
            output_normal_requested
                ? checked_runtime_mesh_bytes(mesh_upload.vertex_count, runtime_morph_normal_delta_stride_bytes,
                                             "runtime morph compute output normal stride is invalid")
                : 0U;
        const auto output_tangent_bytes =
            output_tangent_requested
                ? checked_runtime_mesh_bytes(mesh_upload.vertex_count, runtime_morph_tangent_delta_stride_bytes,
                                             "runtime morph compute output tangent stride is invalid")
                : 0U;
        auto output_normal_desc = rhi::BufferDesc{};
        auto output_tangent_desc = rhi::BufferDesc{};
        if (output_normal_requested) {
            auto normal_usage = ensure_buffer_usage(options.output_normal_usage, rhi::BufferUsage::storage);
            normal_usage = ensure_buffer_usage(normal_usage, rhi::BufferUsage::copy_source);
            output_normal_desc = rhi::BufferDesc{.size_bytes = output_normal_bytes, .usage = normal_usage};
        }
        if (output_tangent_requested) {
            auto tangent_usage = ensure_buffer_usage(options.output_tangent_usage, rhi::BufferUsage::storage);
            tangent_usage = ensure_buffer_usage(tangent_usage, rhi::BufferUsage::copy_source);
            output_tangent_desc = rhi::BufferDesc{.size_bytes = output_tangent_bytes, .usage = tangent_usage};
        }

        std::vector<RuntimeMorphMeshComputeOutputSlot> output_slots;
        output_slots.reserve(options.output_slot_count);
        for (std::uint32_t slot_index = 0; slot_index < options.output_slot_count; ++slot_index) {
            RuntimeMorphMeshComputeOutputSlot slot;
            slot.output_position_buffer = device.create_buffer(output_desc);
            slot.output_position_buffer_desc = output_desc;
            slot.output_position_bytes = output_position_bytes;
            if (output_normal_requested) {
                slot.output_normal_buffer = device.create_buffer(output_normal_desc);
                slot.output_normal_buffer_desc = output_normal_desc;
                slot.output_normal_bytes = output_normal_bytes;
            }
            if (output_tangent_requested) {
                slot.output_tangent_buffer = device.create_buffer(output_tangent_desc);
                slot.output_tangent_buffer_desc = output_tangent_desc;
                slot.output_tangent_bytes = output_tangent_bytes;
            }
            output_slots.push_back(slot);
        }
        const auto& first_output_slot = output_slots.front();

        auto descriptor_set_layout = options.descriptor_set_layout;
        if (descriptor_set_layout.value == 0) {
            if (!options.create_descriptor_set_layout) {
                return morph_compute_binding_failure("runtime morph compute descriptor set layout is missing");
            }
            std::vector<rhi::DescriptorBindingDesc> bindings{
                rhi::DescriptorBindingDesc{
                    .binding = 0,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::compute,
                },
                rhi::DescriptorBindingDesc{
                    .binding = 1,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::compute,
                },
                rhi::DescriptorBindingDesc{
                    .binding = 2,
                    .type = rhi::DescriptorType::uniform_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::compute,
                },
                rhi::DescriptorBindingDesc{
                    .binding = 3,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = options.output_slot_count,
                    .stages = rhi::ShaderStageVisibility::compute,
                },
            };
            if (output_normal_requested) {
                bindings.push_back(rhi::DescriptorBindingDesc{
                    .binding = 4,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::compute,
                });
            }
            if (output_tangent_requested) {
                bindings.push_back(rhi::DescriptorBindingDesc{
                    .binding = 5,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::compute,
                });
            }
            if (output_normal_requested) {
                bindings.push_back(rhi::DescriptorBindingDesc{
                    .binding = 6,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::compute,
                });
            }
            if (output_tangent_requested) {
                bindings.push_back(rhi::DescriptorBindingDesc{
                    .binding = 7,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::compute,
                });
            }
            descriptor_set_layout =
                device.create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{std::move(bindings)});
        }

        const auto descriptor_set = device.allocate_descriptor_set(descriptor_set_layout);
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = 0,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                          mesh_upload.vertex_buffer)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = 1,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                          morph_upload.position_delta_buffer)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = 2,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer,
                                                          morph_upload.morph_weight_buffer)},
        });
        std::vector<rhi::DescriptorResource> output_position_resources;
        output_position_resources.reserve(output_slots.size());
        for (const auto& slot : output_slots) {
            output_position_resources.push_back(
                rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer, slot.output_position_buffer));
        }
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = 3,
            .array_element = 0,
            .resources = std::move(output_position_resources),
        });
        if (output_normal_requested) {
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = descriptor_set,
                .binding = 4,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                              morph_upload.normal_delta_buffer)},
            });
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = descriptor_set,
                .binding = 6,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                              first_output_slot.output_normal_buffer)},
            });
        }
        if (output_tangent_requested) {
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = descriptor_set,
                .binding = 5,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                              morph_upload.tangent_delta_buffer)},
            });
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = descriptor_set,
                .binding = 7,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                              first_output_slot.output_tangent_buffer)},
            });
        }

        RuntimeMorphMeshComputeBinding result;
        result.descriptor_set_layout = descriptor_set_layout;
        result.descriptor_set = descriptor_set;
        result.output_position_buffer = first_output_slot.output_position_buffer;
        result.output_normal_buffer = first_output_slot.output_normal_buffer;
        result.output_tangent_buffer = first_output_slot.output_tangent_buffer;
        result.output_position_buffer_desc = first_output_slot.output_position_buffer_desc;
        result.output_normal_buffer_desc = first_output_slot.output_normal_buffer_desc;
        result.output_tangent_buffer_desc = first_output_slot.output_tangent_buffer_desc;
        result.output_slots = std::move(output_slots);
        result.vertex_count = mesh_upload.vertex_count;
        result.target_count = morph_upload.target_count;
        result.output_position_bytes = output_position_bytes;
        result.output_normal_bytes = output_normal_bytes;
        result.output_tangent_bytes = output_tangent_bytes;
        result.owner_device = &device;
        return result;
    } catch (const std::exception& error) {
        return morph_compute_binding_failure(error.what());
    }
}

MeshGpuBinding make_runtime_compute_morph_output_mesh_gpu_binding(const RuntimeMeshUploadResult& mesh_upload,
                                                                  const RuntimeMorphMeshComputeBinding& compute_binding,
                                                                  std::uint32_t output_slot_index) noexcept {
    if (!mesh_upload.succeeded() || !compute_binding.succeeded()) {
        return MeshGpuBinding{};
    }
    if (mesh_upload.owner_device == nullptr || compute_binding.owner_device == nullptr ||
        mesh_upload.owner_device != compute_binding.owner_device) {
        return MeshGpuBinding{};
    }
    const auto output_slot = runtime_morph_compute_output_slot(compute_binding, output_slot_index);
    if (mesh_upload.index_buffer.value == 0 || output_slot.output_position_buffer.value == 0 ||
        mesh_upload.index_count == 0 || compute_binding.vertex_count == 0) {
        return MeshGpuBinding{};
    }
    if (mesh_upload.vertex_count != compute_binding.vertex_count ||
        mesh_upload.vertex_stride < runtime_mesh_position_vertex_stride_bytes) {
        return MeshGpuBinding{};
    }
    if (mesh_upload.index_format == rhi::IndexFormat::unknown) {
        return MeshGpuBinding{};
    }
    if (!rhi::has_flag(output_slot.output_position_buffer_desc.usage, rhi::BufferUsage::vertex)) {
        return MeshGpuBinding{};
    }

    return MeshGpuBinding{
        .vertex_buffer = output_slot.output_position_buffer,
        .index_buffer = mesh_upload.index_buffer,
        .vertex_count = compute_binding.vertex_count,
        .index_count = mesh_upload.index_count,
        .vertex_offset = 0,
        .index_offset = 0,
        .vertex_stride = runtime_mesh_position_vertex_stride_bytes,
        .index_format = mesh_upload.index_format,
        .owner_device = mesh_upload.owner_device,
    };
}

MeshGpuBinding
make_runtime_compute_morph_tangent_frame_output_mesh_gpu_binding(const RuntimeMeshUploadResult& mesh_upload,
                                                                 const RuntimeMorphMeshComputeBinding& compute_binding,
                                                                 std::uint32_t output_slot_index) noexcept {
    auto result = make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding, output_slot_index);
    if (result.owner_device == nullptr) {
        return MeshGpuBinding{};
    }
    const auto output_slot = runtime_morph_compute_output_slot(compute_binding, output_slot_index);
    if (output_slot.output_normal_buffer.value == 0 || output_slot.output_tangent_buffer.value == 0 ||
        output_slot.output_normal_bytes == 0 || output_slot.output_tangent_bytes == 0) {
        return MeshGpuBinding{};
    }
    if (!rhi::has_flag(output_slot.output_normal_buffer_desc.usage, rhi::BufferUsage::vertex) ||
        !rhi::has_flag(output_slot.output_tangent_buffer_desc.usage, rhi::BufferUsage::vertex)) {
        return MeshGpuBinding{};
    }

    result.normal_vertex_buffer = output_slot.output_normal_buffer;
    result.tangent_vertex_buffer = output_slot.output_tangent_buffer;
    result.normal_vertex_stride = runtime_morph_normal_delta_stride_bytes;
    result.tangent_vertex_stride = runtime_morph_tangent_delta_stride_bytes;
    return result;
}

SkinnedMeshGpuBinding
make_runtime_compute_morph_skinned_mesh_gpu_binding(const RuntimeSkinnedMeshUploadResult& skinned_upload,
                                                    const RuntimeMorphMeshComputeBinding& compute_binding,
                                                    std::uint32_t output_slot_index) noexcept {
    if (!skinned_upload.succeeded() || !compute_binding.succeeded()) {
        return SkinnedMeshGpuBinding{};
    }
    if (skinned_upload.owner_device == nullptr || compute_binding.owner_device == nullptr ||
        skinned_upload.owner_device != compute_binding.owner_device) {
        return SkinnedMeshGpuBinding{};
    }
    const auto output_slot = runtime_morph_compute_output_slot(compute_binding, output_slot_index);
    if (skinned_upload.vertex_buffer.value == 0 || skinned_upload.index_buffer.value == 0 ||
        skinned_upload.joint_palette_buffer.value == 0 || output_slot.output_position_buffer.value == 0) {
        return SkinnedMeshGpuBinding{};
    }
    if (skinned_upload.vertex_count == 0 || skinned_upload.index_count == 0 || skinned_upload.joint_count == 0 ||
        compute_binding.vertex_count == 0 || skinned_upload.vertex_count != compute_binding.vertex_count) {
        return SkinnedMeshGpuBinding{};
    }
    if (skinned_upload.vertex_stride == 0 || skinned_upload.index_format == rhi::IndexFormat::unknown ||
        skinned_upload.joint_palette_uniform_allocation_bytes == 0) {
        return SkinnedMeshGpuBinding{};
    }
    if (!rhi::has_flag(output_slot.output_position_buffer_desc.usage, rhi::BufferUsage::vertex)) {
        return SkinnedMeshGpuBinding{};
    }

    return SkinnedMeshGpuBinding{
        .mesh =
            MeshGpuBinding{
                .vertex_buffer = output_slot.output_position_buffer,
                .index_buffer = skinned_upload.index_buffer,
                .vertex_count = compute_binding.vertex_count,
                .index_count = skinned_upload.index_count,
                .vertex_offset = 0,
                .index_offset = 0,
                .vertex_stride = runtime_mesh_position_vertex_stride_bytes,
                .index_format = skinned_upload.index_format,
                .owner_device = skinned_upload.owner_device,
            },
        .joint_palette_buffer = skinned_upload.joint_palette_buffer,
        .joint_palette_upload_buffer = skinned_upload.joint_palette_upload_buffer,
        .joint_descriptor_set = {},
        .joint_count = skinned_upload.joint_count,
        .joint_palette_uniform_allocation_bytes = skinned_upload.joint_palette_uniform_allocation_bytes,
        .owner_device = skinned_upload.owner_device,
        .skin_attribute_vertex_buffer = skinned_upload.vertex_buffer,
        .skin_attribute_vertex_offset = 0,
        .skin_attribute_vertex_stride = skinned_upload.vertex_stride,
    };
}

std::string attach_skinned_mesh_joint_descriptor_set(rhi::IRhiDevice& device,
                                                     const RuntimeSkinnedMeshUploadResult& upload,
                                                     SkinnedMeshGpuBinding& out,
                                                     rhi::DescriptorSetLayoutHandle& shared_layout_inout) {
    try {
        if (!upload.succeeded()) {
            return "runtime skinned mesh joint descriptor attach requires a succeeded upload";
        }
        if (upload.joint_palette_buffer.value == 0 || upload.joint_palette_uniform_allocation_bytes == 0) {
            return "runtime skinned mesh joint descriptor attach requires a joint palette gpu buffer";
        }
        if (upload.owner_device == nullptr || upload.owner_device != &device) {
            return "runtime skinned mesh joint descriptor attach requires matching owner rhi device";
        }
        if (out.mesh.vertex_buffer.value == 0) {
            return "runtime skinned mesh joint descriptor attach requires mesh gpu binding fields to be populated";
        }
        if (shared_layout_inout.value == 0) {
            shared_layout_inout = device.create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
                rhi::DescriptorBindingDesc{
                    .binding = 0,
                    .type = rhi::DescriptorType::uniform_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::vertex,
                },
            }});
        }
        const auto set = device.allocate_descriptor_set(shared_layout_inout);
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = set,
            .binding = 0,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer,
                                                          upload.joint_palette_buffer)},
        });
        out.joint_descriptor_set = set;
        return {};
    } catch (const std::exception& error) {
        return error.what();
    }
}

std::string attach_morph_mesh_descriptor_set(rhi::IRhiDevice& device, const RuntimeMorphMeshUploadResult& upload,
                                             MorphMeshGpuBinding& out,
                                             rhi::DescriptorSetLayoutHandle& shared_layout_inout) {
    try {
        if (!upload.succeeded()) {
            return "runtime morph mesh descriptor attach requires a succeeded upload";
        }
        if (upload.position_delta_buffer.value == 0 || upload.morph_weight_buffer.value == 0 ||
            upload.morph_weight_uniform_allocation_bytes == 0) {
            return "runtime morph mesh descriptor attach requires morph gpu buffers";
        }
        const bool has_normal_deltas = upload.normal_delta_buffer.value != 0;
        const bool has_tangent_deltas = upload.tangent_delta_buffer.value != 0;
        if ((upload.uploaded_normal_delta_bytes != 0) != has_normal_deltas ||
            (upload.uploaded_tangent_delta_bytes != 0) != has_tangent_deltas) {
            return "runtime morph mesh descriptor attach requires optional morph stream buffers and byte counts to "
                   "match";
        }
        if (upload.owner_device == nullptr || upload.owner_device != &device) {
            return "runtime morph mesh descriptor attach requires matching owner rhi device";
        }
        if (out.position_delta_buffer.value == 0 || out.morph_weight_buffer.value == 0) {
            return "runtime morph mesh descriptor attach requires morph gpu binding fields to be populated";
        }
        if (has_normal_deltas && out.normal_delta_buffer.value == 0) {
            return "runtime morph mesh descriptor attach requires normal morph gpu binding fields to be populated";
        }
        if (has_tangent_deltas && out.tangent_delta_buffer.value == 0) {
            return "runtime morph mesh descriptor attach requires tangent morph gpu binding fields to be populated";
        }
        if (shared_layout_inout.value == 0) {
            std::vector<rhi::DescriptorBindingDesc> bindings{
                rhi::DescriptorBindingDesc{
                    .binding = 0,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::vertex,
                },
                rhi::DescriptorBindingDesc{
                    .binding = 1,
                    .type = rhi::DescriptorType::uniform_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::vertex,
                },
            };
            if (has_normal_deltas) {
                bindings.push_back(rhi::DescriptorBindingDesc{
                    .binding = 2,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::vertex,
                });
            }
            if (has_tangent_deltas) {
                bindings.push_back(rhi::DescriptorBindingDesc{
                    .binding = 3,
                    .type = rhi::DescriptorType::storage_buffer,
                    .count = 1,
                    .stages = rhi::ShaderStageVisibility::vertex,
                });
            }
            shared_layout_inout =
                device.create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{std::move(bindings)});
        }
        const auto set = device.allocate_descriptor_set(shared_layout_inout);
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = set,
            .binding = 0,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                          upload.position_delta_buffer)},
        });
        device.update_descriptor_set(rhi::DescriptorWrite{
            .set = set,
            .binding = 1,
            .array_element = 0,
            .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer,
                                                          upload.morph_weight_buffer)},
        });
        if (has_normal_deltas) {
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = set,
                .binding = 2,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                              upload.normal_delta_buffer)},
            });
        }
        if (has_tangent_deltas) {
            device.update_descriptor_set(rhi::DescriptorWrite{
                .set = set,
                .binding = 3,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::storage_buffer,
                                                              upload.tangent_delta_buffer)},
            });
        }
        out.morph_descriptor_set = set;
        return {};
    } catch (const std::exception& error) {
        return error.what();
    }
}

RuntimeMeshVertexLayoutDesc make_runtime_mesh_vertex_layout_desc(const runtime::RuntimeMeshPayload& payload) {
    if (payload.has_tangent_frame) {
        if (!payload.has_normals || !payload.has_uvs) {
            return mesh_layout_failure("tangent-space mesh payload requires normals and uvs together");
        }
        return RuntimeMeshVertexLayoutDesc{
            .layout = RuntimeMeshVertexLayout::position_normal_uv_tangent,
            .vertex_stride = runtime_mesh_tangent_space_vertex_stride_bytes,
            .vertex_buffers = {rhi::VertexBufferLayoutDesc{.binding = 0,
                                                           .stride = runtime_mesh_tangent_space_vertex_stride_bytes,
                                                           .input_rate = rhi::VertexInputRate::vertex}},
            .vertex_attributes =
                {
                    rhi::VertexAttributeDesc{.location = 0,
                                             .binding = 0,
                                             .offset = 0,
                                             .format = rhi::VertexFormat::float32x3,
                                             .semantic = rhi::VertexSemantic::position,
                                             .semantic_index = 0},
                    rhi::VertexAttributeDesc{.location = 1,
                                             .binding = 0,
                                             .offset = 12,
                                             .format = rhi::VertexFormat::float32x3,
                                             .semantic = rhi::VertexSemantic::normal,
                                             .semantic_index = 0},
                    rhi::VertexAttributeDesc{.location = 2,
                                             .binding = 0,
                                             .offset = 24,
                                             .format = rhi::VertexFormat::float32x2,
                                             .semantic = rhi::VertexSemantic::texcoord,
                                             .semantic_index = 0},
                    rhi::VertexAttributeDesc{.location = 3,
                                             .binding = 0,
                                             .offset = 32,
                                             .format = rhi::VertexFormat::float32x4,
                                             .semantic = rhi::VertexSemantic::tangent,
                                             .semantic_index = 0},
                },
            .diagnostic = {},
        };
    }
    if (payload.has_normals || payload.has_uvs) {
        return mesh_layout_failure(
            "runtime mesh payload must not set normals or uvs without tangent_frame (use GameEngine.CookedMesh.v2 "
            "tangent-space layout)");
    }
    return RuntimeMeshVertexLayoutDesc{
        .layout = RuntimeMeshVertexLayout::position,
        .vertex_stride = runtime_mesh_position_vertex_stride_bytes,
        .vertex_buffers = {rhi::VertexBufferLayoutDesc{.binding = 0,
                                                       .stride = runtime_mesh_position_vertex_stride_bytes,
                                                       .input_rate = rhi::VertexInputRate::vertex}},
        .vertex_attributes = {rhi::VertexAttributeDesc{.location = 0,
                                                       .binding = 0,
                                                       .offset = 0,
                                                       .format = rhi::VertexFormat::float32x3,
                                                       .semantic = rhi::VertexSemantic::position,
                                                       .semantic_index = 0}},
        .diagnostic = {},
    };
}

RuntimeMeshUploadOptions make_runtime_mesh_upload_options(const runtime::RuntimeMeshPayload& payload,
                                                          RuntimeMeshUploadOptions options) {
    if (options.derive_vertex_layout_from_payload) {
        const auto layout = make_runtime_mesh_vertex_layout_desc(payload);
        options.vertex_stride = layout.succeeded() ? layout.vertex_stride : 0;
    }
    return options;
}

RuntimeMaterialGpuBinding create_runtime_material_gpu_binding(
    rhi::IRhiDevice& device, const MaterialPipelineBindingMetadata& metadata, const MaterialFactors& factors,
    const std::vector<RuntimeMaterialTextureResource>& textures, const RuntimeMaterialGpuBindingOptions& options) {
    try {
        for (const auto& binding : metadata.bindings) {
            if (binding.resource_kind != MaterialBindingResourceKind::sampled_texture) {
                continue;
            }
            const auto* resource = find_texture_resource(textures, binding.texture_slot);
            if (resource == nullptr || resource->texture.value == 0) {
                return material_binding_failure("runtime material binding is missing a sampled texture resource");
            }
            if (resource->owner_device == nullptr) {
                return material_binding_failure("runtime material texture resource requires an owner rhi device");
            }
            if (resource->owner_device != &device) {
                return material_binding_failure("runtime material texture resource belongs to a different rhi device");
            }
        }

        const auto layout_desc = make_runtime_material_descriptor_set_layout_desc(metadata);
        if (!layout_desc.succeeded()) {
            return material_binding_failure(layout_desc.diagnostic);
        }

        RuntimeMaterialGpuBinding result;
        result.owner_device = &device;
        result.descriptor_set_layout = options.descriptor_set_layout;
        if (result.descriptor_set_layout.value == 0) {
            if (!options.create_descriptor_set_layout) {
                return material_binding_failure("runtime material descriptor set layout is missing");
            }
            result.descriptor_set_layout = device.create_descriptor_set_layout(layout_desc.desc);
        }
        result.descriptor_set = device.allocate_descriptor_set(result.descriptor_set_layout);

        for (const auto& binding : metadata.bindings) {
            if (binding.resource_kind == MaterialBindingResourceKind::uniform_buffer) {
                if (binding.semantic == "material_factors") {
                    if (result.uniform_buffer.value == 0) {
                        result.uniform_buffer = device.create_buffer(rhi::BufferDesc{
                            .size_bytes = runtime_material_uniform_buffer_allocation_size_bytes,
                            .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_destination,
                        });
                        result.uniform_upload_buffer = device.create_buffer(rhi::BufferDesc{
                            .size_bytes = runtime_material_uniform_buffer_allocation_size_bytes,
                            .usage = rhi::BufferUsage::copy_source,
                        });
                        const auto factor_bytes = pack_runtime_material_factors(factors, metadata.shading_model);
                        std::array<std::uint8_t, runtime_material_uniform_buffer_allocation_size_bytes> upload_bytes{};
                        std::ranges::copy(factor_bytes, upload_bytes.begin());
                        device.write_buffer(result.uniform_upload_buffer, 0, upload_bytes);
                        auto commands = device.begin_command_list(rhi::QueueKind::graphics);
                        commands->copy_buffer(result.uniform_upload_buffer, result.uniform_buffer,
                                              rhi::BufferCopyRegion{
                                                  .source_offset = 0,
                                                  .destination_offset = 0,
                                                  .size_bytes = runtime_material_uniform_buffer_allocation_size_bytes,
                                              });
                        commands->close();
                        const auto fence = device.submit(*commands);
                        device.wait(fence);
                        result.factor_bytes_uploaded = runtime_material_uniform_buffer_size_bytes;
                        result.submitted_fence = fence;
                        result.factor_copy_recorded = true;
                    }
                    rhi::DescriptorWrite write{
                        .set = result.descriptor_set,
                        .binding = binding.binding,
                        .array_element = 0,
                        .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer,
                                                                      result.uniform_buffer)},
                    };
                    device.update_descriptor_set(write);
                    result.writes.push_back(std::move(write));
                } else if (binding.semantic == "scene_pbr_frame") {
                    rhi::BufferHandle scene_buffer = options.shared_scene_pbr_frame_uniform;
                    if (scene_buffer.value == 0) {
                        scene_buffer = device.create_buffer(rhi::BufferDesc{
                            .size_bytes = runtime_scene_pbr_frame_uniform_size_bytes,
                            .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
                        });
                        result.scene_pbr_frame_buffer = scene_buffer;
                        std::array<std::uint8_t, runtime_scene_pbr_frame_uniform_size_bytes> zeros{};
                        device.write_buffer(scene_buffer, 0, zeros);
                    }
                    rhi::DescriptorWrite write{
                        .set = result.descriptor_set,
                        .binding = binding.binding,
                        .array_element = 0,
                        .resources = {rhi::DescriptorResource::buffer(rhi::DescriptorType::uniform_buffer,
                                                                      scene_buffer)},
                    };
                    device.update_descriptor_set(write);
                    result.writes.push_back(std::move(write));
                } else {
                    return material_binding_failure("runtime material uniform binding semantic is unsupported");
                }
            } else if (binding.resource_kind == MaterialBindingResourceKind::sampled_texture) {
                const auto* resource = find_texture_resource(textures, binding.texture_slot);
                rhi::DescriptorWrite write{
                    .set = result.descriptor_set,
                    .binding = binding.binding,
                    .array_element = 0,
                    .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture,
                                                                   resource->texture)},
                };
                device.update_descriptor_set(write);
                result.writes.push_back(std::move(write));
            } else if (binding.resource_kind == MaterialBindingResourceKind::sampler) {
                const auto sampler = device.create_sampler(rhi::SamplerDesc{});
                result.samplers.push_back(sampler);
                rhi::DescriptorWrite write{
                    .set = result.descriptor_set,
                    .binding = binding.binding,
                    .array_element = 0,
                    .resources = {rhi::DescriptorResource::sampler(sampler)},
                };
                device.update_descriptor_set(write);
                result.writes.push_back(std::move(write));
            }
        }

        return result;
    } catch (const std::exception& error) {
        return material_binding_failure(error.what());
    }
}

RuntimeMaterialDescriptorSetLayoutDescResult
make_runtime_material_descriptor_set_layout_desc(const MaterialPipelineBindingMetadata& metadata) {
    try {
        if (!is_valid_material_pipeline_binding_metadata(metadata)) {
            return material_layout_desc_failure("runtime material binding metadata is invalid");
        }
        for (const auto& binding : metadata.bindings) {
            if (binding.set != 0) {
                return material_layout_desc_failure(
                    "runtime material binding currently supports descriptor set 0 only");
            }
        }

        rhi::DescriptorSetLayoutDesc desc;
        desc.bindings.reserve(metadata.bindings.size());
        for (const auto& binding : metadata.bindings) {
            desc.bindings.push_back(rhi::DescriptorBindingDesc{
                .binding = binding.binding,
                .type = runtime_descriptor_type(binding.resource_kind),
                .count = 1,
                .stages = runtime_shader_visibility(binding.stages),
            });
        }
        return RuntimeMaterialDescriptorSetLayoutDescResult{.desc = std::move(desc), .diagnostic = {}};
    } catch (const std::exception& error) {
        return material_layout_desc_failure(error.what());
    }
}

} // namespace mirakana::runtime_rhi
