// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/package_streaming_frame_graph.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] float read_float(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    float value = 0.0F;
    const auto source = std::span<const std::uint8_t>{bytes};
    std::memcpy(&value, source.subspan(offset).data(), sizeof(float));
    return value;
}

void append_le_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
}

void append_le_u16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
}

void append_le_f32(std::vector<std::uint8_t>& bytes, float value) {
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(float));
    append_le_u32(bytes, bits);
}

void append_vec3(std::vector<std::uint8_t>& bytes, float x, float y, float z) {
    append_le_f32(bytes, x);
    append_le_f32(bytes, y);
    append_le_f32(bytes, z);
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_runtime_texture_record(mirakana::AssetId asset, mirakana::runtime::RuntimeAssetHandle handle) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = mirakana::AssetKind::texture,
        .path = "textures/" + std::to_string(asset.value) + ".geasset",
        .content_hash = static_cast<std::uint64_t>(asset.value) + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = "texture",
    };
}

[[nodiscard]] mirakana::runtime::RuntimeResourceCatalogV2
make_runtime_texture_catalog(std::vector<mirakana::runtime::RuntimeAssetRecord> records) {
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto build = mirakana::runtime::build_runtime_resource_catalog_v2(
        catalog, mirakana::runtime::RuntimeAssetPackage{std::move(records)});
    MK_REQUIRE(build.succeeded());
    return catalog;
}

[[nodiscard]] mirakana::runtime::RuntimePackageStreamingExecutionResult make_committed_package_streaming_result() {
    mirakana::runtime::RuntimePackageStreamingExecutionResult result;
    result.status = mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed;
    result.target_id = "packaged-scene-streaming";
    result.package_index_path = "runtime/game.geindex";
    result.runtime_scene_validation_target_id = "packaged-scene";
    result.committed = true;
    return result;
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeTextureUploadResult
make_runtime_texture_upload(mirakana::rhi::IRhiDevice& device, mirakana::AssetId texture) {
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = 10U},
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{0x22, 0x33, 0x44, 0xff},
    };
    return mirakana::runtime_rhi::upload_runtime_texture(device, payload);
}

} // namespace

MK_TEST("runtime rhi upload creates texture resource and records byte upload when payload bytes exist") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = mirakana::AssetId::from_name("textures/player_albedo");
    mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 32,
        .bytes = std::vector<std::uint8_t>(32, std::uint8_t{0x7f}),
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.texture.value == 1);
    MK_REQUIRE(result.upload_buffer.value == 1);
    MK_REQUIRE(result.owner_device == &device);
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.uploaded_bytes == 512);
    MK_REQUIRE(result.submitted_fence.value != 0);
    MK_REQUIRE(result.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(result.texture_desc.extent.width == 4);
    MK_REQUIRE(result.texture_desc.extent.height == 2);
    MK_REQUIRE(result.texture_desc.format == mirakana::rhi::Format::rgba8_unorm);
    MK_REQUIRE(result.copy_region.buffer_row_length == 64);

    const auto stats = device.stats();
    MK_REQUIRE(stats.textures_created == 1);
    MK_REQUIRE(stats.buffers_created == 1);
    MK_REQUIRE(stats.buffer_writes == 1);
    MK_REQUIRE(stats.bytes_written == 512);
    MK_REQUIRE(stats.buffer_texture_copies == 1);
    MK_REQUIRE(stats.command_lists_submitted == 1);
}

MK_TEST("runtime rhi upload creates texture resource without copy for metadata only payload") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = mirakana::AssetId::from_name("textures/player_albedo");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 32,
        .bytes = {},
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.texture.value == 1);
    MK_REQUIRE(result.upload_buffer.value == 0);
    MK_REQUIRE(result.owner_device == &device);
    MK_REQUIRE(!result.copy_recorded);
    MK_REQUIRE(result.uploaded_bytes == 0);
    MK_REQUIRE(result.submitted_fence.value == 0);

    const auto stats = device.stats();
    MK_REQUIRE(stats.textures_created == 1);
    MK_REQUIRE(stats.buffers_created == 0);
    MK_REQUIRE(stats.buffer_texture_copies == 0);
}

MK_TEST("runtime rhi upload rejects unsupported texture formats without creating resources") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = mirakana::AssetId::from_name("textures/greyscale");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 4,
        .height = 2,
        .pixel_format = mirakana::TextureSourcePixelFormat::r8_unorm,
        .source_bytes = 8,
        .bytes = std::vector<std::uint8_t>(8, std::uint8_t{0xff}),
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("format") != std::string::npos);
    MK_REQUIRE(device.stats().textures_created == 0);
}

MK_TEST("runtime rhi upload reports submitted fence without forcing wait") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = mirakana::AssetId::from_name("textures/no_wait_upload");
    const mirakana::runtime::RuntimeTexturePayload payload{
        .asset = texture,
        .handle = mirakana::runtime::RuntimeAssetHandle{13},
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{0x22, 0x33, 0x44, 0xff},
    };
    mirakana::runtime_rhi::RuntimeTextureUploadOptions options;
    options.wait_for_completion = false;

    const auto result = mirakana::runtime_rhi::upload_runtime_texture(device, payload, options);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.submitted_fence.value != 0);
    MK_REQUIRE(result.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(device.stats().command_lists_submitted == 1);
    MK_REQUIRE(device.stats().fence_waits == 0);
}

MK_TEST("runtime package streaming frame graph handoff builds imported texture binding and executor callback") {
    const auto texture = mirakana::AssetId::from_name("textures/streamed/albedo");
    const auto catalog = make_runtime_texture_catalog(
        {make_runtime_texture_record(texture, mirakana::runtime::RuntimeAssetHandle{.value = 1})});
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const auto upload = make_runtime_texture_upload(device, texture);
    MK_REQUIRE(upload.succeeded());

    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = texture,
            .resource = "package_albedo",
            .upload = &upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.size() == 1);
    MK_REQUIRE(handoff.texture_bindings[0].resource == "package_albedo");
    MK_REQUIRE(handoff.texture_bindings[0].texture.value == upload.texture.value);
    MK_REQUIRE(handoff.texture_bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);

    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "package_albedo", .lifetime = mirakana::FrameGraphResourceLifetime::imported});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "sample_package_texture",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "package_albedo",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });
    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "sample_package_texture",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto execution =
        mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
            .commands = commands.get(),
            .schedule = schedule,
            .texture_bindings = handoff.texture_bindings,
            .pass_callbacks = callbacks,
            .pass_target_accesses = {},
            .pass_target_states = {},
            .final_states = {},
        });

    MK_REQUIRE(execution.succeeded());
    MK_REQUIRE(execution.pass_callbacks_invoked == 1);
    MK_REQUIRE(callbacks_invoked == 1);
    MK_REQUIRE(execution.barriers_recorded == 0);
}

MK_TEST("runtime package streaming frame graph handoff rejects non committed streaming result") {
    mirakana::runtime::RuntimePackageStreamingExecutionResult streaming;
    streaming.status = mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required;
    const mirakana::runtime::RuntimeResourceCatalogV2 catalog;

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, {});

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 1);
    MK_REQUIRE(handoff.diagnostics[0].code == "package-streaming-not-committed");
}

MK_TEST("runtime package streaming frame graph handoff rejects texture assets missing from resident catalog") {
    const auto texture = mirakana::AssetId::from_name("textures/missing/albedo");
    const auto streaming = make_committed_package_streaming_result();
    const mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    mirakana::rhi::NullRhiDevice device;
    const auto upload = make_runtime_texture_upload(device, texture);

    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = texture,
            .resource = "missing_albedo",
            .upload = &upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 1);
    MK_REQUIRE(handoff.diagnostics[0].asset == texture);
    MK_REQUIRE(handoff.diagnostics[0].code == "runtime-resource-not-live");
}

MK_TEST("runtime package streaming frame graph handoff rejects failed and empty texture uploads") {
    const auto failed_texture = mirakana::AssetId::from_name("textures/failed_upload");
    const auto empty_texture = mirakana::AssetId::from_name("textures/empty_upload");
    const auto catalog = make_runtime_texture_catalog({
        make_runtime_texture_record(failed_texture, mirakana::runtime::RuntimeAssetHandle{.value = 1}),
        make_runtime_texture_record(empty_texture, mirakana::runtime::RuntimeAssetHandle{.value = 2}),
    });
    const auto streaming = make_committed_package_streaming_result();
    mirakana::runtime_rhi::RuntimeTextureUploadResult failed_upload;
    failed_upload.diagnostic = "upload failed";
    const mirakana::runtime_rhi::RuntimeTextureUploadResult empty_upload;

    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = failed_texture,
            .resource = "failed_upload",
            .upload = &failed_upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = empty_texture,
            .resource = "empty_upload",
            .upload = &empty_upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 2);
    MK_REQUIRE(handoff.diagnostics[0].code == "texture-upload-failed");
    MK_REQUIRE(handoff.diagnostics[1].code == "texture-upload-empty");
}

MK_TEST("runtime package streaming frame graph handoff rejects duplicate frame graph resource names") {
    const auto albedo = mirakana::AssetId::from_name("textures/duplicate/albedo");
    const auto normal = mirakana::AssetId::from_name("textures/duplicate/normal");
    const auto catalog = make_runtime_texture_catalog({
        make_runtime_texture_record(albedo, mirakana::runtime::RuntimeAssetHandle{.value = 1}),
        make_runtime_texture_record(normal, mirakana::runtime::RuntimeAssetHandle{.value = 2}),
    });
    const auto streaming = make_committed_package_streaming_result();
    mirakana::rhi::NullRhiDevice device;
    const auto albedo_upload = make_runtime_texture_upload(device, albedo);
    const auto normal_upload = make_runtime_texture_upload(device, normal);

    const std::vector<mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource> sources{
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = albedo,
            .resource = "package_texture",
            .upload = &albedo_upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
        mirakana::runtime_rhi::RuntimePackageStreamingFrameGraphTextureBindingSource{
            .asset = normal,
            .resource = "package_texture",
            .upload = &normal_upload,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    const auto handoff =
        mirakana::runtime_rhi::make_runtime_package_streaming_frame_graph_texture_bindings(streaming, catalog, sources);

    MK_REQUIRE(!handoff.succeeded());
    MK_REQUIRE(handoff.texture_bindings.empty());
    MK_REQUIRE(handoff.diagnostics.size() == 1);
    MK_REQUIRE(handoff.diagnostics[0].code == "duplicate-frame-graph-resource");
    MK_REQUIRE(handoff.diagnostics[0].resource == "package_texture");
}

MK_TEST("runtime rhi upload creates mesh buffers and records vertex index byte uploads") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/triangle");
    const std::vector<std::uint8_t> vertex_bytes(36, std::uint8_t{0x7a});
    const std::vector<std::uint8_t> index_bytes{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{2},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);
    const auto uploaded_vertices = device.read_buffer(result.vertex_buffer, 0, result.uploaded_vertex_bytes);
    const auto uploaded_indices = device.read_buffer(result.index_buffer, 0, result.uploaded_index_bytes);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.vertex_buffer.value != 0);
    MK_REQUIRE(result.index_buffer.value != 0);
    MK_REQUIRE(result.vertex_upload_buffer.value != 0);
    MK_REQUIRE(result.index_upload_buffer.value != 0);
    MK_REQUIRE(result.copy_recorded);
    MK_REQUIRE(result.vertex_count == 3);
    MK_REQUIRE(result.index_count == 3);
    MK_REQUIRE(result.vertex_stride == 12);
    MK_REQUIRE(result.index_format == mirakana::rhi::IndexFormat::uint32);
    MK_REQUIRE(result.owner_device == &device);
    MK_REQUIRE(result.uploaded_vertex_bytes == 36);
    MK_REQUIRE(result.uploaded_index_bytes == 12);
    MK_REQUIRE(result.submitted_fence.value != 0);
    MK_REQUIRE(result.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(result.vertex_copy_region.size_bytes == 36);
    MK_REQUIRE(result.index_copy_region.size_bytes == 12);
    MK_REQUIRE(uploaded_vertices == vertex_bytes);
    MK_REQUIRE(uploaded_indices == index_bytes);
    MK_REQUIRE(mirakana::rhi::has_flag(result.vertex_buffer_desc.usage, mirakana::rhi::BufferUsage::vertex));
    MK_REQUIRE(mirakana::rhi::has_flag(result.vertex_buffer_desc.usage, mirakana::rhi::BufferUsage::copy_destination));
    MK_REQUIRE(mirakana::rhi::has_flag(result.index_buffer_desc.usage, mirakana::rhi::BufferUsage::index));
    MK_REQUIRE(mirakana::rhi::has_flag(result.index_buffer_desc.usage, mirakana::rhi::BufferUsage::copy_destination));

    const auto stats = device.stats();
    MK_REQUIRE(stats.buffers_created == 4);
    MK_REQUIRE(stats.buffer_copies == 2);
    MK_REQUIRE(stats.bytes_copied == 48);
    MK_REQUIRE(stats.buffer_writes == 2);
    MK_REQUIRE(stats.bytes_written == 48);
    MK_REQUIRE(stats.command_lists_submitted == 1);
    MK_REQUIRE(stats.fence_waits == 1);

    const auto mesh_binding = mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(result);
    MK_REQUIRE(mesh_binding.vertex_buffer.value == result.vertex_buffer.value);
    MK_REQUIRE(mesh_binding.index_buffer.value == result.index_buffer.value);
    MK_REQUIRE(mesh_binding.vertex_count == result.vertex_count);
    MK_REQUIRE(mesh_binding.index_count == result.index_count);
    MK_REQUIRE(mesh_binding.vertex_stride == result.vertex_stride);
    MK_REQUIRE(mesh_binding.index_format == result.index_format);
    MK_REQUIRE(mesh_binding.owner_device == &device);
}

MK_TEST("runtime rhi derives tangent-space mesh vertex layout from cooked mesh metadata") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/lit_triangle");
    const std::vector<std::uint8_t> vertex_bytes(144, std::uint8_t{0x5a});
    const std::vector<std::uint8_t> index_bytes{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{20},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };

    const auto layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(payload);
    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(layout.succeeded());
    MK_REQUIRE(layout.layout == mirakana::runtime_rhi::RuntimeMeshVertexLayout::position_normal_uv_tangent);
    MK_REQUIRE(layout.vertex_stride == mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);
    MK_REQUIRE(layout.vertex_buffers.size() == 1);
    MK_REQUIRE(layout.vertex_buffers[0].stride == 48);
    MK_REQUIRE(layout.vertex_attributes.size() == 4);
    MK_REQUIRE(layout.vertex_attributes[0].semantic == mirakana::rhi::VertexSemantic::position);
    MK_REQUIRE(layout.vertex_attributes[0].offset == 0);
    MK_REQUIRE(layout.vertex_attributes[0].format == mirakana::rhi::VertexFormat::float32x3);
    MK_REQUIRE(layout.vertex_attributes[1].semantic == mirakana::rhi::VertexSemantic::normal);
    MK_REQUIRE(layout.vertex_attributes[1].offset == 12);
    MK_REQUIRE(layout.vertex_attributes[1].format == mirakana::rhi::VertexFormat::float32x3);
    MK_REQUIRE(layout.vertex_attributes[2].semantic == mirakana::rhi::VertexSemantic::texcoord);
    MK_REQUIRE(layout.vertex_attributes[2].offset == 24);
    MK_REQUIRE(layout.vertex_attributes[2].format == mirakana::rhi::VertexFormat::float32x2);
    MK_REQUIRE(layout.vertex_attributes[3].semantic == mirakana::rhi::VertexSemantic::tangent);
    MK_REQUIRE(layout.vertex_attributes[3].offset == 32);
    MK_REQUIRE(layout.vertex_attributes[3].format == mirakana::rhi::VertexFormat::float32x4);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.vertex_stride == 48);
    MK_REQUIRE(result.uploaded_vertex_bytes == 144);
    MK_REQUIRE(device.read_buffer(result.vertex_buffer, 0, result.uploaded_vertex_bytes) == vertex_bytes);
}

MK_TEST("runtime rhi upload defaults to graphics queue for backend neutral upload commands") {
    const mirakana::runtime_rhi::RuntimeTextureUploadOptions texture_options;
    const mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;

    MK_REQUIRE(texture_options.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(mesh_options.queue == mirakana::rhi::QueueKind::graphics);
}

MK_TEST("runtime rhi upload rejects mesh payloads without vertex and index bytes") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/metadata_only");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{3},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = {},
        .index_bytes = {},
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("byte") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
    MK_REQUIRE(device.stats().buffer_copies == 0);
    MK_REQUIRE(device.stats().command_lists_begun == 0);
}

MK_TEST("runtime rhi upload rejects invalid mesh payload metadata before creating buffers") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/broken");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{4},
        .vertex_count = 0,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = {0x00, 0x01, 0x02},
        .index_bytes = {0x03, 0x04, 0x05},
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("invalid") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("runtime rhi upload rejects mesh bytes that do not match draw binding metadata") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/misaligned");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{5},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(35, std::uint8_t{0x00}),
        .index_bytes = std::vector<std::uint8_t>(12, std::uint8_t{0x00}),
    };

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("invalid") != std::string::npos);
    MK_REQUIRE(mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(result).vertex_buffer.value == 0);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("runtime rhi upload rejects partial lit mesh vertex layouts before creating buffers") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/partial_lit_triangle");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{21},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(72, std::uint8_t{0x00}),
        .index_bytes = std::vector<std::uint8_t>(12, std::uint8_t{0x00}),
    };

    const auto layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(payload);
    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload);

    MK_REQUIRE(!layout.succeeded());
    MK_REQUIRE(layout.diagnostic.find("tangent_frame") != std::string::npos);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("invalid") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("runtime rhi upload rejects partial lit mesh vertex layouts when derivation is disabled") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/manual_partial_lit_triangle");
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{22},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(36, std::uint8_t{0x00}),
        .index_bytes = std::vector<std::uint8_t>(12, std::uint8_t{0x00}),
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions options;
    options.derive_vertex_layout_from_payload = false;
    options.vertex_stride = mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes;

    const auto result = mirakana::runtime_rhi::upload_runtime_mesh(device, payload, options);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostic.find("invalid") != std::string::npos);
    MK_REQUIRE(device.stats().buffers_created == 0);
}

MK_TEST("runtime material gpu binding creates descriptor resources for material textures") {
    mirakana::rhi::NullRhiDevice device;
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto texture_id = mirakana::AssetId::from_name("textures/player_albedo");
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 2, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_id}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);

    const auto binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{
            .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture, .owner_device = &device}});

    MK_REQUIRE(binding.succeeded());
    MK_REQUIRE(binding.descriptor_set_layout.value == 1);
    MK_REQUIRE(binding.descriptor_set.value == 1);
    MK_REQUIRE(binding.uniform_buffer.value == 1);
    MK_REQUIRE(binding.owner_device == &device);
    MK_REQUIRE(binding.samplers.size() == 1);
    MK_REQUIRE(binding.writes.size() == 4);
    MK_REQUIRE(binding.writes[0].binding == 0);
    MK_REQUIRE(binding.writes[0].resources[0].type == mirakana::rhi::DescriptorType::uniform_buffer);
    MK_REQUIRE(binding.writes[1].binding == 6);
    MK_REQUIRE(binding.writes[1].resources[0].type == mirakana::rhi::DescriptorType::uniform_buffer);
    MK_REQUIRE(binding.writes[2].binding == 1);
    MK_REQUIRE(binding.writes[2].resources[0].type == mirakana::rhi::DescriptorType::sampled_texture);
    MK_REQUIRE(binding.writes[3].binding == 16);
    MK_REQUIRE(binding.writes[3].resources[0].type == mirakana::rhi::DescriptorType::sampler);

    const auto stats = device.stats();
    MK_REQUIRE(stats.textures_created == 1);
    MK_REQUIRE(stats.samplers_created == 1);
    MK_REQUIRE(stats.buffers_created == 3);
    MK_REQUIRE(stats.descriptor_set_layouts_created == 1);
    MK_REQUIRE(stats.descriptor_sets_allocated == 1);
    MK_REQUIRE(stats.descriptor_writes == 4);
}

MK_TEST("runtime material gpu binding uploads material factors into uniform buffer") {
    mirakana::rhi::NullRhiDevice device;
    const auto material_id = mirakana::AssetId::from_name("materials/factors");
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Factors",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.25F, 0.5F, 0.75F, 1.0F},
                .emissive = {2.0F, 3.0F, 4.0F},
                .metallic = 0.125F,
                .roughness = 0.875F,
            },
        .texture_bindings = {},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);

    const auto binding =
        mirakana::runtime_rhi::create_runtime_material_gpu_binding(device, metadata, material.factors, {});
    const auto bytes = device.read_buffer(binding.uniform_buffer, 0,
                                          mirakana::runtime_rhi::runtime_material_uniform_buffer_size_bytes);
    const auto allocation_bytes = device.read_buffer(binding.uniform_buffer, 0, 256);

    MK_REQUIRE(binding.succeeded());
    MK_REQUIRE(bytes.size() == mirakana::runtime_rhi::runtime_material_uniform_buffer_size_bytes);
    MK_REQUIRE(allocation_bytes.size() == 256);
    MK_REQUIRE(read_float(bytes, 0) == 0.25F);
    MK_REQUIRE(read_float(bytes, 4) == 0.5F);
    MK_REQUIRE(read_float(bytes, 8) == 0.75F);
    MK_REQUIRE(read_float(bytes, 12) == 1.0F);
    MK_REQUIRE(read_float(bytes, 16) == 2.0F);
    MK_REQUIRE(read_float(bytes, 20) == 3.0F);
    MK_REQUIRE(read_float(bytes, 24) == 4.0F);
    MK_REQUIRE(read_float(bytes, 28) == 0.125F);
    MK_REQUIRE(read_float(bytes, 32) == 0.875F);
    MK_REQUIRE(binding.factor_bytes_uploaded == mirakana::runtime_rhi::runtime_material_uniform_buffer_size_bytes);
    MK_REQUIRE(binding.submitted_fence.value != 0);
    MK_REQUIRE(binding.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);

    const auto stats = device.stats();
    MK_REQUIRE(stats.buffers_created == 3);
    MK_REQUIRE(stats.buffer_writes == 2);
    MK_REQUIRE(stats.buffer_copies == 1);
}

MK_TEST("runtime material gpu binding can allocate descriptors from a caller owned layout") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture_id = mirakana::AssetId::from_name("textures/external_layout");
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/external_layout"),
        .name = "ExternalLayout",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_id}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);
    const auto layout_desc = mirakana::runtime_rhi::make_runtime_material_descriptor_set_layout_desc(metadata);
    MK_REQUIRE(layout_desc.succeeded());
    const auto layout = device.create_descriptor_set_layout(layout_desc.desc);

    const auto binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{
            .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture, .owner_device = &device}},
        mirakana::runtime_rhi::RuntimeMaterialGpuBindingOptions{.descriptor_set_layout = layout,
                                                                .create_descriptor_set_layout = false});

    MK_REQUIRE(binding.succeeded());
    MK_REQUIRE(binding.descriptor_set_layout.value == layout.value);
    MK_REQUIRE(binding.descriptor_set.value != 0);
    MK_REQUIRE(binding.writes.size() == 4);
    MK_REQUIRE(device.stats().descriptor_set_layouts_created == 1);
    MK_REQUIRE(device.stats().descriptor_sets_allocated == 1);
    MK_REQUIRE(device.stats().descriptor_writes == 4);
}

MK_TEST("runtime material gpu binding rejects texture resources from another rhi device") {
    mirakana::rhi::NullRhiDevice texture_device;
    mirakana::rhi::NullRhiDevice material_device;
    const auto texture = texture_device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/cross_device"),
        .name = "CrossDevice",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{
            .slot = mirakana::MaterialTextureSlot::base_color,
            .texture = mirakana::AssetId::from_name("textures/cross_device")}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);

    const auto binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        material_device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{
            .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture, .owner_device = &texture_device}});

    MK_REQUIRE(!binding.succeeded());
    MK_REQUIRE(binding.diagnostic.find("different rhi device") != std::string::npos);
    MK_REQUIRE(material_device.stats().descriptor_set_layouts_created == 0);
}

MK_TEST("runtime material gpu binding rejects unsupported nonzero material descriptor sets") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::MaterialPipelineBindingMetadata metadata;
    metadata.material = mirakana::AssetId::from_name("materials/nonzero_set");
    metadata.shading_model = mirakana::MaterialShadingModel::lit;
    metadata.surface_mode = mirakana::MaterialSurfaceMode::opaque;
    metadata.bindings.push_back(mirakana::MaterialPipelineBinding{
        .set = 1,
        .binding = 0,
        .resource_kind = mirakana::MaterialBindingResourceKind::uniform_buffer,
        .stages = mirakana::MaterialShaderStageMask::fragment,
        .texture_slot = mirakana::MaterialTextureSlot::unknown,
        .semantic = "material.factors",
    });

    const auto binding =
        mirakana::runtime_rhi::create_runtime_material_gpu_binding(device, metadata, mirakana::MaterialFactors{}, {});

    MK_REQUIRE(!binding.succeeded());
    MK_REQUIRE(binding.diagnostic.find("set 0") != std::string::npos);
    MK_REQUIRE(device.stats().descriptor_set_layouts_created == 0);
}

MK_TEST("runtime rhi skinned mesh upload binds joint palette descriptor set") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/skinned_runtime_upload");
    std::vector<std::uint8_t> vertex_bytes(216, std::uint8_t{0x2a});
    const std::vector<std::uint8_t> index_bytes{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    std::vector<std::uint8_t> palette(64, std::uint8_t{0});
    palette[0] = 0x00;
    palette[1] = 0x00;
    palette[2] = 0x80;
    palette[3] = 0x3f; // float 1.0 LE at m00
    palette[20] = 0x00;
    palette[21] = 0x00;
    palette[22] = 0x80;
    palette[23] = 0x3f; // m11
    palette[40] = 0x00;
    palette[41] = 0x00;
    palette[42] = 0x80;
    palette[43] = 0x3f; // m22
    palette[60] = 0x00;
    palette[61] = 0x00;
    palette[62] = 0x80;
    palette[63] = 0x3f; // m33
    const mirakana::runtime::RuntimeSkinnedMeshPayload payload{
        .asset = mesh,
        .handle = mirakana::runtime::RuntimeAssetHandle{9},
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
        .joint_palette_bytes = palette,
    };

    const auto upload = mirakana::runtime_rhi::upload_runtime_skinned_mesh(device, payload);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.submitted_fence.value != 0);
    MK_REQUIRE(upload.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);
    auto binding = mirakana::runtime_rhi::make_runtime_skinned_mesh_gpu_binding(upload);
    mirakana::rhi::DescriptorSetLayoutHandle shared_layout{};
    const auto diagnostic =
        mirakana::runtime_rhi::attach_skinned_mesh_joint_descriptor_set(device, upload, binding, shared_layout);
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(binding.joint_descriptor_set.value != 0);
    MK_REQUIRE(shared_layout.value != 0);
    MK_REQUIRE(device.stats().descriptor_sets_allocated >= 1);
}

MK_TEST("runtime rhi composes compute morph output with skinned mesh attributes") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> skinned_vertex_bytes;
    const auto append_skinned_vertex = [&skinned_vertex_bytes](float x, float y, float z) {
        append_vec3(skinned_vertex_bytes, x, y, z);
        append_vec3(skinned_vertex_bytes, 0.0F, 0.0F, 1.0F);
        append_le_f32(skinned_vertex_bytes, 0.5F);
        append_le_f32(skinned_vertex_bytes, 0.5F);
        append_vec3(skinned_vertex_bytes, 1.0F, 0.0F, 0.0F);
        append_le_f32(skinned_vertex_bytes, 1.0F);
        append_le_u16(skinned_vertex_bytes, 0);
        append_le_u16(skinned_vertex_bytes, 0);
        append_le_u16(skinned_vertex_bytes, 0);
        append_le_u16(skinned_vertex_bytes, 0);
        append_le_f32(skinned_vertex_bytes, 1.0F);
        append_le_f32(skinned_vertex_bytes, 0.0F);
        append_le_f32(skinned_vertex_bytes, 0.0F);
        append_le_f32(skinned_vertex_bytes, 0.0F);
    };
    append_skinned_vertex(-1.4F, 0.6F, 0.0F);
    append_skinned_vertex(-0.7F, -0.6F, 0.0F);
    append_skinned_vertex(-1.9F, -0.6F, 0.0F);
    MK_REQUIRE(skinned_vertex_bytes.size() == 3U * mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    std::vector<std::uint8_t> joint_palette_bytes(mirakana::runtime_rhi::runtime_skinned_mesh_joint_matrix_bytes,
                                                  std::uint8_t{0});
    const mirakana::runtime::RuntimeSkinnedMeshPayload skinned_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_skinned_runtime_upload"),
        .handle = mirakana::runtime::RuntimeAssetHandle{21},
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = skinned_vertex_bytes,
        .index_bytes = index_bytes,
        .joint_palette_bytes = joint_palette_bytes,
    };
    const auto skinned_upload = mirakana::runtime_rhi::upload_runtime_skinned_mesh(device, skinned_payload);
    MK_REQUIRE(skinned_upload.succeeded());

    std::vector<std::uint8_t> position_bytes;
    append_vec3(position_bytes, -1.4F, 0.6F, 0.0F);
    append_vec3(position_bytes, -0.7F, -0.6F, 0.0F);
    append_vec3(position_bytes, -1.9F, -0.6F, 0.0F);
    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = skinned_payload.asset,
        .handle = mirakana::runtime::RuntimeAssetHandle{22},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = position_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = position_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    }
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_skinned_runtime_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{23},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());

    auto binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_skinned_mesh_gpu_binding(skinned_upload, compute_binding);

    MK_REQUIRE(binding.mesh.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(binding.mesh.index_buffer.value == skinned_upload.index_buffer.value);
    MK_REQUIRE(binding.mesh.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(binding.skin_attribute_vertex_buffer.value == skinned_upload.vertex_buffer.value);
    MK_REQUIRE(binding.skin_attribute_vertex_stride == mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);
    MK_REQUIRE(binding.joint_palette_buffer.value == skinned_upload.joint_palette_buffer.value);
    MK_REQUIRE(binding.owner_device == &device);

    mirakana::rhi::DescriptorSetLayoutHandle joint_layout{};
    const auto diagnostic =
        mirakana::runtime_rhi::attach_skinned_mesh_joint_descriptor_set(device, skinned_upload, binding, joint_layout);
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(binding.joint_descriptor_set.value != 0);
    MK_REQUIRE(joint_layout.value != 0);
}

MK_TEST("runtime rhi morph mesh upload binds position delta storage and weight descriptor set") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/morph_runtime_upload");

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    append_vec3(morph.bind_position_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, -1.35F, -0.75F, 0.0F);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    MK_REQUIRE(read_float(morph.target_weight_bytes, 0) == 1.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload payload{
        .asset = mesh, .handle = mirakana::runtime::RuntimeAssetHandle{11}, .morph = morph};

    const auto upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, payload);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.vertex_count == 3);
    MK_REQUIRE(upload.target_count == 1);
    MK_REQUIRE(upload.uploaded_position_delta_bytes == 36);
    MK_REQUIRE(upload.morph_weight_uniform_allocation_bytes == 256);
    MK_REQUIRE(upload.submitted_fence.value != 0);
    MK_REQUIRE(upload.submitted_fence.queue == mirakana::rhi::QueueKind::graphics);

    auto binding = mirakana::runtime_rhi::make_runtime_morph_mesh_gpu_binding(upload);
    mirakana::rhi::DescriptorSetLayoutHandle shared_layout{};
    const auto diagnostic =
        mirakana::runtime_rhi::attach_morph_mesh_descriptor_set(device, upload, binding, shared_layout);
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(binding.morph_descriptor_set.value != 0);
    MK_REQUIRE(shared_layout.value != 0);
    MK_REQUIRE(device.stats().descriptor_sets_allocated >= 1);
}

MK_TEST("runtime rhi morph mesh upload binds optional normal and tangent delta streams") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/morph_runtime_upload_tangent_space");

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    append_vec3(morph.bind_position_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, -1.35F, -0.75F, 0.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    append_vec3(target.tangent_delta_bytes, 0.0F, 1.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload payload{
        .asset = mesh, .handle = mirakana::runtime::RuntimeAssetHandle{12}, .morph = morph};

    const auto upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, payload);
    MK_REQUIRE(upload.succeeded());
    MK_REQUIRE(upload.uploaded_position_delta_bytes == 36);
    MK_REQUIRE(upload.uploaded_normal_delta_bytes == 36);
    MK_REQUIRE(upload.uploaded_tangent_delta_bytes == 36);
    MK_REQUIRE(upload.normal_delta_buffer.value != 0);
    MK_REQUIRE(upload.tangent_delta_buffer.value != 0);

    auto binding = mirakana::runtime_rhi::make_runtime_morph_mesh_gpu_binding(upload);
    MK_REQUIRE(binding.normal_delta_buffer.value == upload.normal_delta_buffer.value);
    MK_REQUIRE(binding.tangent_delta_buffer.value == upload.tangent_delta_buffer.value);
    MK_REQUIRE(binding.normal_delta_bytes == 36);
    MK_REQUIRE(binding.tangent_delta_bytes == 36);

    mirakana::rhi::DescriptorSetLayoutHandle shared_layout{};
    const auto diagnostic =
        mirakana::runtime_rhi::attach_morph_mesh_descriptor_set(device, upload, binding, shared_layout);
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(binding.morph_descriptor_set.value != 0);
    MK_REQUIRE(shared_layout.value != 0);
    MK_REQUIRE(device.stats().descriptor_sets_allocated >= 1);
}

MK_TEST("runtime rhi creates compute morph binding for position output") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    append_vec3(vertex_bytes, 1.0F, 2.0F, 0.0F);
    append_vec3(vertex_bytes, -2.0F, 4.0F, 0.5F);
    append_vec3(vertex_bytes, 0.0F, -1.0F, 2.0F);
    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_runtime_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{13},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_runtime_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{14},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    const auto compute_binding =
        mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(device, mesh_upload, morph_upload);

    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.descriptor_set_layout.value != 0);
    MK_REQUIRE(compute_binding.descriptor_set.value != 0);
    MK_REQUIRE(compute_binding.output_position_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_position_bytes == 36);
    MK_REQUIRE(compute_binding.vertex_count == 3);
    MK_REQUIRE(compute_binding.target_count == 1);
    MK_REQUIRE(compute_binding.owner_device == &device);
    MK_REQUIRE(device.stats().descriptor_sets_allocated >= 1);
    MK_REQUIRE(device.stats().descriptor_writes >= 4);
}

MK_TEST("runtime rhi creates compute morph output ring with distinct position slots") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    append_vec3(vertex_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(vertex_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(vertex_bytes, -1.35F, -0.75F, 0.0F);
    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_output_ring_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{121},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 1.0F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_output_ring_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{122},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    compute_options.output_slot_count = 2;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);

    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_slots.size() == 2);
    MK_REQUIRE(compute_binding.output_slots[0].output_position_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_slots[1].output_position_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_slots[0].output_position_buffer.value !=
               compute_binding.output_slots[1].output_position_buffer.value);
    MK_REQUIRE(compute_binding.output_position_buffer.value ==
               compute_binding.output_slots[0].output_position_buffer.value);
    MK_REQUIRE(compute_binding.output_slots[1].output_position_bytes == 36);

    const auto slot_one_mesh_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding, 1);
    MK_REQUIRE(slot_one_mesh_binding.vertex_buffer.value ==
               compute_binding.output_slots[1].output_position_buffer.value);
    MK_REQUIRE(slot_one_mesh_binding.index_buffer.value == mesh_upload.index_buffer.value);
    MK_REQUIRE(slot_one_mesh_binding.vertex_count == compute_binding.vertex_count);
    MK_REQUIRE(slot_one_mesh_binding.owner_device == &device);

    const auto invalid_slot_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding, 2);
    MK_REQUIRE(invalid_slot_binding.vertex_buffer.value == 0);
}

MK_TEST("runtime rhi creates compute morph binding for normal and tangent outputs") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    auto append_lit_vertex = [&vertex_bytes](float px, float py, float pz) {
        append_vec3(vertex_bytes, px, py, pz);
        append_vec3(vertex_bytes, 0.0F, 0.0F, 1.0F);
        append_le_f32(vertex_bytes, 0.5F);
        append_le_f32(vertex_bytes, 0.5F);
        append_vec3(vertex_bytes, 1.0F, 0.0F, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
    };
    append_lit_vertex(-0.6F, 0.75F, 0.0F);
    append_lit_vertex(0.15F, -0.75F, 0.0F);
    append_lit_vertex(-1.35F, -0.75F, 0.0F);

    std::vector<std::uint8_t> bind_positions;
    append_vec3(bind_positions, -0.6F, 0.75F, 0.0F);
    append_vec3(bind_positions, 0.15F, -0.75F, 0.0F);
    append_vec3(bind_positions, -1.35F, -0.75F, 0.0F);

    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_tangent_frame_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{19},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = bind_positions;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_vec3(morph.bind_normal_bytes, 0.0F, 0.0F, 1.0F);
        append_vec3(morph.bind_tangent_bytes, 1.0F, 0.0F, 0.0F);
    }
    mirakana::MorphMeshCpuTargetSourceDocument target;
    for (std::uint32_t index = 0; index < 3; ++index) {
        append_vec3(target.position_delta_bytes, 0.0F, 0.0F, 0.0F);
        append_vec3(target.normal_delta_bytes, 0.0F, 1.0F, -1.0F);
        append_vec3(target.tangent_delta_bytes, -1.0F, 0.0F, 1.0F);
    }
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);

    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_tangent_frame_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{20},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());
    MK_REQUIRE(morph_upload.normal_delta_buffer.value != 0);
    MK_REQUIRE(morph_upload.tangent_delta_buffer.value != 0);

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    compute_options.output_normal_usage = mirakana::rhi::BufferUsage::storage |
                                          mirakana::rhi::BufferUsage::copy_source | mirakana::rhi::BufferUsage::vertex;
    compute_options.output_tangent_usage = mirakana::rhi::BufferUsage::storage |
                                           mirakana::rhi::BufferUsage::copy_source | mirakana::rhi::BufferUsage::vertex;

    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);

    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(compute_binding.output_position_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_normal_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_tangent_buffer.value != 0);
    MK_REQUIRE(compute_binding.output_position_bytes == 36);
    MK_REQUIRE(compute_binding.output_normal_bytes == 36);
    MK_REQUIRE(compute_binding.output_tangent_bytes == 36);
    MK_REQUIRE(
        mirakana::rhi::has_flag(compute_binding.output_normal_buffer_desc.usage, mirakana::rhi::BufferUsage::storage));
    MK_REQUIRE(
        mirakana::rhi::has_flag(compute_binding.output_tangent_buffer_desc.usage, mirakana::rhi::BufferUsage::storage));

    const auto mesh_binding = mirakana::runtime_rhi::make_runtime_compute_morph_tangent_frame_output_mesh_gpu_binding(
        mesh_upload, compute_binding);
    MK_REQUIRE(mesh_binding.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(mesh_binding.normal_vertex_buffer.value == compute_binding.output_normal_buffer.value);
    MK_REQUIRE(mesh_binding.tangent_vertex_buffer.value == compute_binding.output_tangent_buffer.value);
    MK_REQUIRE(mesh_binding.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(mesh_binding.normal_vertex_stride == mirakana::runtime_rhi::runtime_morph_normal_delta_stride_bytes);
    MK_REQUIRE(mesh_binding.tangent_vertex_stride == mirakana::runtime_rhi::runtime_morph_tangent_delta_stride_bytes);
    MK_REQUIRE(mesh_binding.owner_device == &device);
    MK_REQUIRE(device.stats().descriptor_writes >= 8);
}

MK_TEST("runtime rhi exposes compute morph output as mesh binding") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    append_vec3(vertex_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(vertex_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(vertex_bytes, -1.35F, -0.75F, 0.0F);
    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_renderer_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{15},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = vertex_bytes;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_renderer_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{16},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());
    MK_REQUIRE(
        mirakana::rhi::has_flag(compute_binding.output_position_buffer_desc.usage, mirakana::rhi::BufferUsage::vertex));

    const auto mesh_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding);

    MK_REQUIRE(mesh_binding.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(mesh_binding.index_buffer.value == mesh_upload.index_buffer.value);
    MK_REQUIRE(mesh_binding.vertex_count == compute_binding.vertex_count);
    MK_REQUIRE(mesh_binding.index_count == mesh_upload.index_count);
    MK_REQUIRE(mesh_binding.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(mesh_binding.index_format == mesh_upload.index_format);
    MK_REQUIRE(mesh_binding.owner_device == &device);
}

MK_TEST("runtime rhi exposes interleaved compute morph output as position mesh binding") {
    mirakana::rhi::NullRhiDevice device;

    std::vector<std::uint8_t> vertex_bytes;
    auto append_lit_vertex = [&vertex_bytes](float px, float py, float pz) {
        append_vec3(vertex_bytes, px, py, pz);
        append_vec3(vertex_bytes, 0.0F, 0.0F, 1.0F);
        append_le_f32(vertex_bytes, 0.5F);
        append_le_f32(vertex_bytes, 0.5F);
        append_vec3(vertex_bytes, 1.0F, 0.0F, 0.0F);
        append_le_f32(vertex_bytes, 1.0F);
    };
    append_lit_vertex(-0.6F, 0.75F, 0.0F);
    append_lit_vertex(0.15F, -0.75F, 0.0F);
    append_lit_vertex(-1.35F, -0.75F, 0.0F);
    std::vector<std::uint8_t> bind_positions;
    append_vec3(bind_positions, -0.6F, 0.75F, 0.0F);
    append_vec3(bind_positions, 0.15F, -0.75F, 0.0F);
    append_vec3(bind_positions, -1.35F, -0.75F, 0.0F);
    std::vector<std::uint8_t> index_bytes;
    append_le_u32(index_bytes, 0);
    append_le_u32(index_bytes, 1);
    append_le_u32(index_bytes, 2);

    const mirakana::runtime::RuntimeMeshPayload mesh_payload{
        .asset = mirakana::AssetId::from_name("meshes/compute_morph_interleaved_base"),
        .handle = mirakana::runtime::RuntimeAssetHandle{17},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = true,
        .has_uvs = true,
        .has_tangent_frame = true,
        .vertex_bytes = vertex_bytes,
        .index_bytes = index_bytes,
    };
    mirakana::runtime_rhi::RuntimeMeshUploadOptions mesh_options;
    mesh_options.vertex_usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage |
                                mirakana::rhi::BufferUsage::copy_destination;
    const auto mesh_upload = mirakana::runtime_rhi::upload_runtime_mesh(device, mesh_payload, mesh_options);
    MK_REQUIRE(mesh_upload.succeeded());
    MK_REQUIRE(mesh_upload.vertex_stride == mirakana::runtime_rhi::runtime_mesh_tangent_space_vertex_stride_bytes);

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes = bind_positions;
    mirakana::MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload morph_payload{
        .asset = mirakana::AssetId::from_name("morphs/compute_morph_interleaved_delta"),
        .handle = mirakana::runtime::RuntimeAssetHandle{18},
        .morph = morph,
    };
    const auto morph_upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, morph_payload);
    MK_REQUIRE(morph_upload.succeeded());

    mirakana::runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_options;
    compute_options.output_position_usage = mirakana::rhi::BufferUsage::storage |
                                            mirakana::rhi::BufferUsage::copy_source |
                                            mirakana::rhi::BufferUsage::vertex;
    const auto compute_binding = mirakana::runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, mesh_upload, morph_upload, compute_options);
    MK_REQUIRE(compute_binding.succeeded());

    const auto mesh_binding =
        mirakana::runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload, compute_binding);

    MK_REQUIRE(mesh_binding.vertex_buffer.value == compute_binding.output_position_buffer.value);
    MK_REQUIRE(mesh_binding.index_buffer.value == mesh_upload.index_buffer.value);
    MK_REQUIRE(mesh_binding.vertex_count == compute_binding.vertex_count);
    MK_REQUIRE(mesh_binding.index_count == mesh_upload.index_count);
    MK_REQUIRE(mesh_binding.vertex_stride == mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    MK_REQUIRE(mesh_binding.index_format == mesh_upload.index_format);
    MK_REQUIRE(mesh_binding.owner_device == &device);
}

int main() {
    return mirakana::test::run_all();
}
