// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana::runtime_scene_rhi {
namespace {

[[nodiscard]] bool contains_asset(const std::vector<AssetId>& assets, AssetId asset) noexcept {
    return std::ranges::contains(assets, asset);
}

void record_submitted_upload_fence(RuntimeSceneGpuUploadExecutionReport& report, rhi::FenceValue fence) noexcept {
    if (fence.value == 0) {
        return;
    }
    ++report.submitted_upload_fence_count;
    report.last_submitted_upload_fence = fence;
}

void record_submitted_upload_fence(RuntimeSceneGpuBindingResult& result, rhi::FenceValue fence) {
    if (fence.value != 0) {
        result.submitted_upload_fences.push_back(fence);
    }
}

void append_unique_asset(std::vector<AssetId>& assets, AssetId asset) {
    if (!contains_asset(assets, asset)) {
        assets.push_back(asset);
    }
}

[[nodiscard]] bool has_failure_for(const RuntimeSceneGpuBindingResult& result, AssetId asset) noexcept {
    return std::ranges::any_of(
        result.failures, [asset](const RuntimeSceneGpuBindingFailure& failure) { return failure.asset == asset; });
}

void add_failure(RuntimeSceneGpuBindingResult& result, AssetId asset, std::string diagnostic) {
    if (!has_failure_for(result, asset)) {
        result.failures.push_back(RuntimeSceneGpuBindingFailure{.asset = asset, .diagnostic = std::move(diagnostic)});
    }
}

[[nodiscard]] RuntimeSceneMeshGpuResource* find_mesh_upload(RuntimeSceneGpuBindingResult& result,
                                                            AssetId mesh) noexcept {
    const auto it = std::ranges::find_if(
        result.mesh_uploads, [mesh](const RuntimeSceneMeshGpuResource& upload) { return upload.mesh == mesh; });
    return it == result.mesh_uploads.end() ? nullptr : &*it;
}

[[nodiscard]] RuntimeSceneSkinnedMeshGpuResource* find_skinned_mesh_upload(RuntimeSceneGpuBindingResult& result,
                                                                           AssetId mesh) noexcept {
    const auto it =
        std::ranges::find_if(result.skinned_mesh_uploads,
                             [mesh](const RuntimeSceneSkinnedMeshGpuResource& upload) { return upload.mesh == mesh; });
    return it == result.skinned_mesh_uploads.end() ? nullptr : &*it;
}

[[nodiscard]] RuntimeSceneMorphMeshGpuResource* find_morph_mesh_upload(RuntimeSceneGpuBindingResult& result,
                                                                       AssetId morph_mesh) noexcept {
    const auto it =
        std::ranges::find_if(result.morph_mesh_uploads, [morph_mesh](const RuntimeSceneMorphMeshGpuResource& upload) {
            return upload.morph_mesh == morph_mesh;
        });
    return it == result.morph_mesh_uploads.end() ? nullptr : &*it;
}

[[nodiscard]] RuntimeSceneTextureGpuResource* find_texture_upload(RuntimeSceneGpuBindingResult& result,
                                                                  AssetId texture) noexcept {
    const auto it =
        std::ranges::find_if(result.texture_uploads, [texture](const RuntimeSceneTextureGpuResource& upload) {
            return upload.texture == texture;
        });
    return it == result.texture_uploads.end() ? nullptr : &*it;
}

[[nodiscard]] RuntimeSceneMaterialGpuResource* find_material_binding(RuntimeSceneGpuBindingResult& result,
                                                                     AssetId material) noexcept {
    const auto it =
        std::ranges::find_if(result.material_bindings, [material](const RuntimeSceneMaterialGpuResource& binding) {
            return binding.material == material;
        });
    return it == result.material_bindings.end() ? nullptr : &*it;
}

[[nodiscard]] bool same_descriptor_binding(const rhi::DescriptorBindingDesc& lhs,
                                           const rhi::DescriptorBindingDesc& rhs) noexcept {
    return lhs.binding == rhs.binding && lhs.type == rhs.type && lhs.count == rhs.count && lhs.stages == rhs.stages;
}

[[nodiscard]] bool same_descriptor_set_layout_desc(const rhi::DescriptorSetLayoutDesc& lhs,
                                                   const rhi::DescriptorSetLayoutDesc& rhs) noexcept {
    if (lhs.bindings.size() != rhs.bindings.size()) {
        return false;
    }
    for (const auto& lhs_binding : lhs.bindings) {
        const auto found =
            std::ranges::find_if(rhs.bindings, [&lhs_binding](const rhi::DescriptorBindingDesc& rhs_binding) {
                return same_descriptor_binding(lhs_binding, rhs_binding);
            });
        if (found == rhs.bindings.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::vector<AssetId> unique_referenced_meshes(const SceneRenderPacket& packet) {
    std::vector<AssetId> meshes;
    for (const auto& mesh : packet.meshes) {
        append_unique_asset(meshes, mesh.renderer.mesh);
    }
    return meshes;
}

[[nodiscard]] std::vector<AssetId> unique_referenced_materials(const SceneRenderPacket& packet) {
    std::vector<AssetId> materials;
    for (const auto& mesh : packet.meshes) {
        append_unique_asset(materials, mesh.renderer.material);
    }
    return materials;
}

[[nodiscard]] std::vector<AssetId> unique_selected_morph_meshes(const std::vector<AssetId>& selected_morph_meshes) {
    std::vector<AssetId> morph_meshes;
    morph_meshes.reserve(selected_morph_meshes.size());
    for (const auto morph_mesh : selected_morph_meshes) {
        append_unique_asset(morph_meshes, morph_mesh);
    }
    return morph_meshes;
}

[[nodiscard]] bool has_compute_morph_skinned_binding(const RuntimeSceneGpuBindingOptions& options,
                                                     AssetId mesh) noexcept {
    return std::ranges::any_of(
        options.compute_morph_skinned_mesh_bindings,
        [mesh](const RuntimeSceneComputeMorphSkinnedMeshBinding& binding) { return binding.mesh == mesh; });
}

[[nodiscard]] bool has_compute_morph_skinned_resource(const RuntimeSceneGpuBindingResult& result,
                                                      AssetId mesh) noexcept {
    return std::ranges::any_of(
        result.compute_morph_skinned_mesh_bindings,
        [mesh](const RuntimeSceneComputeMorphSkinnedMeshGpuResource& resource) { return resource.mesh == mesh; });
}

struct RuntimeSceneSkinnedPositionPayloadResult {
    runtime::RuntimeMeshPayload payload;
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

[[nodiscard]] RuntimeSceneSkinnedPositionPayloadResult
make_skinned_position_payload(const runtime::RuntimeSkinnedMeshPayload& skinned_payload) {
    if (skinned_payload.vertex_count == 0 || skinned_payload.index_count == 0) {
        return RuntimeSceneSkinnedPositionPayloadResult{
            .payload = {},
            .diagnostic = "runtime scene compute morph skinned binding requires non-zero skinned mesh counts",
        };
    }

    const auto expected_skinned_vertex_bytes =
        static_cast<std::uint64_t>(skinned_payload.vertex_count) *
        static_cast<std::uint64_t>(runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes);
    if (static_cast<std::uint64_t>(skinned_payload.vertex_bytes.size()) != expected_skinned_vertex_bytes) {
        return RuntimeSceneSkinnedPositionPayloadResult{
            .payload = {},
            .diagnostic = "runtime scene compute morph skinned binding requires fixed-stride skinned vertices",
        };
    }
    if (skinned_payload.index_bytes.empty()) {
        return RuntimeSceneSkinnedPositionPayloadResult{
            .payload = {},
            .diagnostic = "runtime scene compute morph skinned binding requires skinned mesh indices",
        };
    }

    std::vector<std::uint8_t> position_bytes;
    position_bytes.reserve(static_cast<std::size_t>(skinned_payload.vertex_count) *
                           runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    for (std::uint32_t vertex = 0; vertex < skinned_payload.vertex_count; ++vertex) {
        const auto offset = static_cast<std::size_t>(vertex) * runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes;
        position_bytes.insert(position_bytes.end(), skinned_payload.vertex_bytes.begin() + offset,
                              skinned_payload.vertex_bytes.begin() + offset +
                                  runtime_rhi::runtime_mesh_position_vertex_stride_bytes);
    }

    return RuntimeSceneSkinnedPositionPayloadResult{
        .payload =
            runtime::RuntimeMeshPayload{
                .asset = skinned_payload.asset,
                .handle = skinned_payload.handle,
                .vertex_count = skinned_payload.vertex_count,
                .index_count = skinned_payload.index_count,
                .has_normals = false,
                .has_uvs = false,
                .has_tangent_frame = false,
                .vertex_bytes = std::move(position_bytes),
                .index_bytes = skinned_payload.index_bytes,
            },
        .diagnostic = {},
    };
}

[[nodiscard]] const runtime::RuntimeAssetRecord* find_package_record(RuntimeSceneGpuBindingResult& result,
                                                                     const runtime::RuntimeAssetPackage& package,
                                                                     AssetId asset, AssetKind kind,
                                                                     std::string_view diagnostic_name) {
    if (asset.value == 0) {
        add_failure(result, asset,
                    std::string("runtime scene ") + std::string(diagnostic_name) + " asset id is invalid");
        return nullptr;
    }

    const auto* record = package.find(asset);
    if (record == nullptr) {
        add_failure(result, asset,
                    std::string("runtime scene referenced ") + std::string(diagnostic_name) + " payload is missing");
        return nullptr;
    }
    if (record->kind != kind) {
        add_failure(result, asset,
                    std::string("runtime scene referenced asset is not a ") + std::string(diagnostic_name));
        return nullptr;
    }
    return record;
}

[[nodiscard]] RuntimeSceneTextureGpuResource* ensure_texture_upload(rhi::IRhiDevice& device,
                                                                    RuntimeSceneGpuBindingResult& result,
                                                                    const runtime::RuntimeAssetPackage& package,
                                                                    AssetId texture,
                                                                    const RuntimeSceneGpuBindingOptions& options) {
    if (auto* existing = find_texture_upload(result, texture); existing != nullptr) {
        return existing;
    }
    if (has_failure_for(result, texture)) {
        return nullptr;
    }

    const auto* record = find_package_record(result, package, texture, AssetKind::texture, "texture");
    if (record == nullptr) {
        return nullptr;
    }

    const auto payload = runtime::runtime_texture_payload(*record);
    if (!payload.succeeded()) {
        add_failure(result, texture, payload.diagnostic);
        return nullptr;
    }

    auto upload = runtime_rhi::upload_runtime_texture(device, payload.payload, options.texture_upload);
    if (!upload.succeeded()) {
        add_failure(result, texture, upload.diagnostic);
        return nullptr;
    }

    record_submitted_upload_fence(result, upload.submitted_fence);
    result.texture_uploads.push_back(RuntimeSceneTextureGpuResource{.texture = texture, .upload = std::move(upload)});
    return &result.texture_uploads.back();
}

void add_mesh_upload(rhi::IRhiDevice& device, RuntimeSceneGpuBindingResult& result,
                     const runtime::RuntimeAssetPackage& package, AssetId mesh,
                     const RuntimeSceneGpuBindingOptions& options) {
    if (find_mesh_upload(result, mesh) != nullptr || find_skinned_mesh_upload(result, mesh) != nullptr ||
        has_failure_for(result, mesh)) {
        return;
    }

    if (mesh.value == 0) {
        add_failure(result, mesh, "runtime scene mesh asset id is invalid");
        return;
    }

    const auto* record = package.find(mesh);
    if (record == nullptr) {
        add_failure(result, mesh, "runtime scene referenced mesh payload is missing");
        return;
    }

    if (record->kind == AssetKind::mesh) {
        const auto payload = runtime::runtime_mesh_payload(*record);
        if (!payload.succeeded()) {
            add_failure(result, mesh, payload.diagnostic);
            return;
        }

        auto upload = runtime_rhi::upload_runtime_mesh(device, payload.payload, options.mesh_upload);
        if (!upload.succeeded()) {
            add_failure(result, mesh, upload.diagnostic);
            return;
        }
        record_submitted_upload_fence(result, upload.submitted_fence);

        const auto binding = runtime_rhi::make_runtime_mesh_gpu_binding(upload);
        if (!result.palette.try_add_mesh(mesh, binding)) {
            add_failure(result, mesh, "runtime scene gpu palette rejected mesh binding");
            return;
        }

        result.mesh_uploads.push_back(RuntimeSceneMeshGpuResource{.mesh = mesh, .upload = std::move(upload)});
        return;
    }

    if (record->kind == AssetKind::skinned_mesh) {
        const auto payload = runtime::runtime_skinned_mesh_payload(*record);
        if (!payload.succeeded()) {
            add_failure(result, mesh, payload.diagnostic);
            return;
        }

        auto upload = runtime_rhi::upload_runtime_skinned_mesh(device, payload.payload, options.skinned_mesh_upload);
        if (!upload.succeeded()) {
            add_failure(result, mesh, upload.diagnostic);
            return;
        }
        record_submitted_upload_fence(result, upload.submitted_fence);

        if (!has_compute_morph_skinned_binding(options, mesh)) {
            auto binding = runtime_rhi::make_runtime_skinned_mesh_gpu_binding(upload);
            const auto attach_diagnostic = runtime_rhi::attach_skinned_mesh_joint_descriptor_set(
                device, upload, binding, result.skinned_joint_descriptor_set_layout);
            if (!attach_diagnostic.empty()) {
                add_failure(result, mesh, attach_diagnostic);
                return;
            }
            if (!result.skinned_palette.try_add_skinned_mesh(mesh, binding)) {
                add_failure(result, mesh, "runtime scene gpu palette rejected skinned mesh binding");
                return;
            }
        }

        result.skinned_mesh_uploads.push_back(
            RuntimeSceneSkinnedMeshGpuResource{.mesh = mesh, .upload = std::move(upload)});
        return;
    }

    add_failure(result, mesh, "runtime scene referenced mesh asset is not a mesh or skinned_mesh");
}

RuntimeSceneMorphMeshGpuResource* ensure_morph_mesh_upload(rhi::IRhiDevice& device,
                                                           RuntimeSceneGpuBindingResult& result,
                                                           const runtime::RuntimeAssetPackage& package,
                                                           AssetId morph_mesh,
                                                           const RuntimeSceneGpuBindingOptions& options) {
    if (auto* existing = find_morph_mesh_upload(result, morph_mesh); existing != nullptr) {
        return existing;
    }
    if (has_failure_for(result, morph_mesh)) {
        return nullptr;
    }

    const auto* record = find_package_record(result, package, morph_mesh, AssetKind::morph_mesh_cpu, "morph_mesh_cpu");
    if (record == nullptr) {
        return nullptr;
    }

    const auto payload = runtime::runtime_morph_mesh_cpu_payload(*record);
    if (!payload.succeeded()) {
        add_failure(result, morph_mesh, payload.diagnostic);
        return nullptr;
    }

    auto upload = runtime_rhi::upload_runtime_morph_mesh_cpu(device, payload.payload, options.morph_mesh_upload);
    if (!upload.succeeded()) {
        add_failure(result, morph_mesh, upload.diagnostic);
        return nullptr;
    }

    record_submitted_upload_fence(result, upload.submitted_fence);
    result.morph_mesh_uploads.push_back(
        RuntimeSceneMorphMeshGpuResource{.morph_mesh = morph_mesh, .upload = std::move(upload)});
    return &result.morph_mesh_uploads.back();
}

void add_morph_mesh_upload(rhi::IRhiDevice& device, RuntimeSceneGpuBindingResult& result,
                           const runtime::RuntimeAssetPackage& package, AssetId morph_mesh,
                           const RuntimeSceneGpuBindingOptions& options) {
    auto* upload = ensure_morph_mesh_upload(device, result, package, morph_mesh, options);
    if (upload == nullptr || result.morph_palette.find_morph_mesh(morph_mesh) != nullptr) {
        return;
    }

    auto binding = runtime_rhi::make_runtime_morph_mesh_gpu_binding(upload->upload);
    const auto attach_diagnostic = runtime_rhi::attach_morph_mesh_descriptor_set(device, upload->upload, binding,
                                                                                 result.morph_descriptor_set_layout);
    if (!attach_diagnostic.empty()) {
        add_failure(result, morph_mesh, attach_diagnostic);
        return;
    }
    if (!result.morph_palette.try_add_morph_mesh(morph_mesh, binding)) {
        add_failure(result, morph_mesh, "runtime scene gpu palette rejected morph mesh binding");
        return;
    }
}

void add_compute_morph_skinned_mesh_binding(rhi::IRhiDevice& device, RuntimeSceneGpuBindingResult& result,
                                            const runtime::RuntimeAssetPackage& package,
                                            const RuntimeSceneComputeMorphSkinnedMeshBinding& selected,
                                            const RuntimeSceneGpuBindingOptions& options) {
    if (selected.mesh.value == 0 || selected.morph_mesh.value == 0) {
        add_failure(result, selected.mesh,
                    "runtime scene compute morph skinned binding requires valid mesh and morph assets");
        return;
    }
    if (has_compute_morph_skinned_resource(result, selected.mesh)) {
        add_failure(result, selected.mesh,
                    "runtime scene compute morph skinned binding was selected more than once for the mesh");
        return;
    }
    if (result.skinned_palette.find_skinned_mesh(selected.mesh) != nullptr) {
        add_failure(result, selected.mesh,
                    "runtime scene compute morph skinned binding conflicts with an existing skinned binding");
        return;
    }

    if (find_skinned_mesh_upload(result, selected.mesh) == nullptr) {
        add_mesh_upload(device, result, package, selected.mesh, options);
    }
    auto* skinned_upload = find_skinned_mesh_upload(result, selected.mesh);
    if (skinned_upload == nullptr) {
        return;
    }

    const auto* skinned_record =
        find_package_record(result, package, selected.mesh, AssetKind::skinned_mesh, "skinned_mesh");
    if (skinned_record == nullptr) {
        return;
    }
    const auto skinned_payload = runtime::runtime_skinned_mesh_payload(*skinned_record);
    if (!skinned_payload.succeeded()) {
        add_failure(result, selected.mesh, skinned_payload.diagnostic);
        return;
    }

    auto base_payload = make_skinned_position_payload(skinned_payload.payload);
    if (!base_payload.succeeded()) {
        add_failure(result, selected.mesh, base_payload.diagnostic);
        return;
    }

    auto base_upload_options = options.compute_morph_base_mesh_upload;
    base_upload_options.vertex_usage =
        base_upload_options.vertex_usage | rhi::BufferUsage::storage | rhi::BufferUsage::copy_destination;
    auto base_upload = runtime_rhi::upload_runtime_mesh(device, base_payload.payload, base_upload_options);
    if (!base_upload.succeeded()) {
        add_failure(result, selected.mesh, base_upload.diagnostic);
        return;
    }
    record_submitted_upload_fence(result, base_upload.submitted_fence);

    auto* morph_upload = ensure_morph_mesh_upload(device, result, package, selected.morph_mesh, options);
    if (morph_upload == nullptr) {
        return;
    }

    auto compute_options = options.compute_morph;
    compute_options.output_position_usage = compute_options.output_position_usage | rhi::BufferUsage::storage |
                                            rhi::BufferUsage::copy_source | rhi::BufferUsage::vertex;
    auto compute_binding = runtime_rhi::create_runtime_morph_mesh_compute_binding(
        device, base_upload, morph_upload->upload, compute_options);
    if (!compute_binding.succeeded()) {
        add_failure(result, selected.mesh, compute_binding.diagnostic);
        return;
    }

    auto skinned_binding =
        runtime_rhi::make_runtime_compute_morph_skinned_mesh_gpu_binding(skinned_upload->upload, compute_binding);
    if (skinned_binding.owner_device == nullptr) {
        add_failure(result, selected.mesh,
                    "runtime scene compute morph skinned binding failed to compose renderer resources");
        return;
    }

    const auto attach_diagnostic = runtime_rhi::attach_skinned_mesh_joint_descriptor_set(
        device, skinned_upload->upload, skinned_binding, result.skinned_joint_descriptor_set_layout);
    if (!attach_diagnostic.empty()) {
        add_failure(result, selected.mesh, attach_diagnostic);
        return;
    }
    if (!result.skinned_palette.try_add_skinned_mesh(selected.mesh, skinned_binding)) {
        add_failure(result, selected.mesh, "runtime scene gpu palette rejected compute morph skinned mesh binding");
        return;
    }

    result.compute_morph_skinned_mesh_bindings.push_back(RuntimeSceneComputeMorphSkinnedMeshGpuResource{
        .mesh = selected.mesh,
        .morph_mesh = selected.morph_mesh,
        .base_position_upload = std::move(base_upload),
        .compute_binding = std::move(compute_binding),
    });
}

[[nodiscard]] bool collect_material_texture_resources(
    rhi::IRhiDevice& device, RuntimeSceneGpuBindingResult& result, const runtime::RuntimeAssetPackage& package,
    const runtime::RuntimeMaterialPayload& material, const RuntimeSceneGpuBindingOptions& options,
    std::vector<runtime_rhi::RuntimeMaterialTextureResource>& textures) {
    for (const auto& texture_binding : material.material.texture_bindings) {
        auto* upload = ensure_texture_upload(device, result, package, texture_binding.texture, options);
        if (upload == nullptr) {
            return false;
        }
        textures.push_back(runtime_rhi::RuntimeMaterialTextureResource{
            .slot = texture_binding.slot,
            .texture = upload->upload.texture,
            .owner_device = upload->upload.owner_device,
        });
    }
    return true;
}

struct RuntimeSceneMaterialLayoutState {
    rhi::DescriptorSetLayoutDesc desc;
    rhi::DescriptorSetLayoutHandle descriptor_set_layout;
    rhi::PipelineLayoutHandle pipeline_layout;
    rhi::BufferHandle scene_pbr_frame_uniform{};
    bool has_desc{false};
};

[[nodiscard]] bool ensure_material_layout(rhi::IRhiDevice& device, RuntimeSceneGpuBindingResult& result,
                                          RuntimeSceneMaterialLayoutState& state,
                                          const rhi::DescriptorSetLayoutDesc& desc,
                                          const RuntimeSceneGpuBindingOptions& options, AssetId material) {
    if (state.descriptor_set_layout.value != 0) {
        if (options.shared_material_descriptor_set_layout.value == 0 &&
            !same_descriptor_set_layout_desc(state.desc, desc)) {
            add_failure(result, material, "runtime scene material descriptor layout differs from scene shared layout");
            return false;
        }
        return true;
    }

    state.desc = desc;
    state.has_desc = true;
    state.descriptor_set_layout = options.shared_material_descriptor_set_layout;
    if (state.descriptor_set_layout.value == 0) {
        state.descriptor_set_layout = device.create_descriptor_set_layout(desc);
    }
    std::vector<rhi::DescriptorSetLayoutHandle> pipeline_descriptor_sets;
    pipeline_descriptor_sets.reserve(1U + options.additional_pipeline_descriptor_set_layouts.size());
    pipeline_descriptor_sets.push_back(state.descriptor_set_layout);
    pipeline_descriptor_sets.insert(pipeline_descriptor_sets.end(),
                                    options.additional_pipeline_descriptor_set_layouts.begin(),
                                    options.additional_pipeline_descriptor_set_layouts.end());
    state.pipeline_layout = device.create_pipeline_layout(
        rhi::PipelineLayoutDesc{.descriptor_sets = std::move(pipeline_descriptor_sets), .push_constant_bytes = 0});
    result.material_pipeline_layouts.push_back(state.pipeline_layout);
    return true;
}

void add_material_binding(rhi::IRhiDevice& device, RuntimeSceneGpuBindingResult& result,
                          const runtime::RuntimeAssetPackage& package, AssetId material,
                          const RuntimeSceneGpuBindingOptions& options,
                          RuntimeSceneMaterialLayoutState& material_layout) {
    if (find_material_binding(result, material) != nullptr || has_failure_for(result, material)) {
        return;
    }

    const auto* record = find_package_record(result, package, material, AssetKind::material, "material");
    if (record == nullptr) {
        return;
    }

    const auto payload = runtime::runtime_material_payload(*record);
    if (!payload.succeeded()) {
        add_failure(result, material, payload.diagnostic);
        return;
    }

    std::vector<runtime_rhi::RuntimeMaterialTextureResource> textures;
    if (!collect_material_texture_resources(device, result, package, payload.payload, options, textures)) {
        return;
    }

    const auto layout_desc =
        runtime_rhi::make_runtime_material_descriptor_set_layout_desc(payload.payload.binding_metadata);
    if (!layout_desc.succeeded()) {
        add_failure(result, material, layout_desc.diagnostic);
        return;
    }
    if (!ensure_material_layout(device, result, material_layout, layout_desc.desc, options, material)) {
        return;
    }

    if (material_layout.scene_pbr_frame_uniform.value == 0) {
        material_layout.scene_pbr_frame_uniform = device.create_buffer(rhi::BufferDesc{
            .size_bytes = runtime_rhi::runtime_scene_pbr_frame_uniform_size_bytes,
            .usage = rhi::BufferUsage::uniform | rhi::BufferUsage::copy_source,
        });
        std::array<std::uint8_t, runtime_rhi::runtime_scene_pbr_frame_uniform_size_bytes> clear{};
        device.write_buffer(material_layout.scene_pbr_frame_uniform, 0, clear);
        if (result.scene_pbr_frame_uniform_buffer.value == 0) {
            result.scene_pbr_frame_uniform_buffer = material_layout.scene_pbr_frame_uniform;
        }
    }

    const auto binding_options = runtime_rhi::RuntimeMaterialGpuBindingOptions{
        .descriptor_set_layout = material_layout.descriptor_set_layout,
        .create_descriptor_set_layout = false,
        .shared_scene_pbr_frame_uniform = material_layout.scene_pbr_frame_uniform,
    };
    auto binding = runtime_rhi::create_runtime_material_gpu_binding(
        device, payload.payload.binding_metadata, payload.payload.material.factors, textures, binding_options);
    if (!binding.succeeded()) {
        add_failure(result, material, binding.diagnostic);
        return;
    }
    record_submitted_upload_fence(result, binding.submitted_fence);

    const auto palette_binding = MaterialGpuBinding{
        .pipeline_layout = material_layout.pipeline_layout,
        .descriptor_set = binding.descriptor_set,
        .descriptor_set_index = 0,
        .owner_device = &device,
    };
    if (!result.palette.try_add_material(material, palette_binding)) {
        add_failure(result, material, "runtime scene gpu palette rejected material binding");
        return;
    }

    result.material_bindings.push_back(RuntimeSceneMaterialGpuResource{
        .material = material, .binding = std::move(binding), .pipeline_layout = material_layout.pipeline_layout});
}

} // namespace

RuntimeSceneGpuBindingResult build_runtime_scene_gpu_binding_palette(rhi::IRhiDevice& device,
                                                                     const runtime::RuntimeAssetPackage& package,
                                                                     const SceneRenderPacket& packet,
                                                                     const RuntimeSceneGpuBindingOptions& options) {
    RuntimeSceneGpuBindingResult result;

    try {
        for (const auto mesh : unique_referenced_meshes(packet)) {
            add_mesh_upload(device, result, package, mesh, options);
        }
        for (const auto morph_mesh : unique_selected_morph_meshes(options.morph_mesh_assets)) {
            add_morph_mesh_upload(device, result, package, morph_mesh, options);
        }
        for (const auto& binding : options.compute_morph_skinned_mesh_bindings) {
            add_compute_morph_skinned_mesh_binding(device, result, package, binding, options);
        }

        /// Material pipeline layouts append extra descriptor sets after the material descriptor set (set index 0).
        /// GPU skinning keeps joint palette at set index 1. Morph-only palettes use the same index for graphics morph
        /// deltas and weights. Compute-morphed skinned output composes at the runtime/renderer boundary, and the
        /// runtime_scene_rhi host-owned palette bridge is tracked by the active focused plan.
        RuntimeSceneGpuBindingOptions material_options = options;
        if (result.skinned_joint_descriptor_set_layout.value != 0 || result.morph_descriptor_set_layout.value != 0) {
            std::vector<rhi::DescriptorSetLayoutHandle> merged;
            merged.reserve(1U + options.additional_pipeline_descriptor_set_layouts.size());
            if (result.skinned_joint_descriptor_set_layout.value != 0) {
                merged.push_back(result.skinned_joint_descriptor_set_layout);
            } else {
                merged.push_back(result.morph_descriptor_set_layout);
            }
            merged.insert(merged.end(), options.additional_pipeline_descriptor_set_layouts.begin(),
                          options.additional_pipeline_descriptor_set_layouts.end());
            material_options.additional_pipeline_descriptor_set_layouts = std::move(merged);
        }

        RuntimeSceneMaterialLayoutState material_layout;
        for (const auto material : unique_referenced_materials(packet)) {
            add_material_binding(device, result, package, material, material_options, material_layout);
        }
    } catch (const std::exception& error) {
        add_failure(result, AssetId{}, std::string("runtime scene gpu binding failed: ") + error.what());
    }

    return result;
}

std::string_view runtime_scene_gpu_upload_execution_status_name(RuntimeSceneGpuUploadExecutionStatus status) noexcept {
    switch (status) {
    case RuntimeSceneGpuUploadExecutionStatus::not_requested:
        return "not_requested";
    case RuntimeSceneGpuUploadExecutionStatus::invalid_request:
        return "invalid_request";
    case RuntimeSceneGpuUploadExecutionStatus::failed:
        return "failed";
    case RuntimeSceneGpuUploadExecutionStatus::ready:
        return "ready";
    }
    return "unknown";
}

RuntimeSceneGpuUploadExecutionReport
make_runtime_scene_gpu_upload_execution_report(const RuntimeSceneGpuBindingResult& bindings) {
    RuntimeSceneGpuUploadExecutionReport report;
    report.mesh_uploads = bindings.mesh_uploads.size();
    report.skinned_mesh_uploads = bindings.skinned_mesh_uploads.size();
    report.morph_mesh_uploads = bindings.morph_mesh_uploads.size();
    report.compute_morph_skinned_mesh_bindings = bindings.compute_morph_skinned_mesh_bindings.size();
    report.texture_uploads = bindings.texture_uploads.size();
    report.material_bindings = bindings.material_bindings.size();
    report.material_pipeline_layouts = bindings.material_pipeline_layouts.size();

    for (const auto& mesh : bindings.mesh_uploads) {
        report.uploaded_mesh_bytes += mesh.upload.uploaded_vertex_bytes + mesh.upload.uploaded_index_bytes;
    }
    for (const auto& skinned : bindings.skinned_mesh_uploads) {
        report.uploaded_mesh_bytes += skinned.upload.uploaded_vertex_bytes + skinned.upload.uploaded_index_bytes +
                                      skinned.upload.uploaded_joint_palette_bytes;
    }
    for (const auto& morph_mesh : bindings.morph_mesh_uploads) {
        report.uploaded_morph_bytes +=
            morph_mesh.upload.uploaded_position_delta_bytes + morph_mesh.upload.uploaded_normal_delta_bytes +
            morph_mesh.upload.uploaded_tangent_delta_bytes + morph_mesh.upload.uploaded_weight_bytes;
    }
    for (const auto& compute_morph : bindings.compute_morph_skinned_mesh_bindings) {
        report.uploaded_compute_morph_base_position_bytes += compute_morph.base_position_upload.uploaded_vertex_bytes;
        report.compute_morph_output_position_bytes += compute_morph.compute_binding.output_position_bytes;
    }
    for (const auto& texture : bindings.texture_uploads) {
        report.uploaded_texture_bytes += texture.upload.uploaded_bytes;
    }
    for (const auto& material : bindings.material_bindings) {
        report.uploaded_material_factor_bytes += material.binding.factor_bytes_uploaded;
        report.material_descriptor_writes += material.binding.writes.size();
    }
    for (const auto fence : bindings.submitted_upload_fences) {
        record_submitted_upload_fence(report, fence);
    }
    for (const auto& failure : bindings.failures) {
        report.diagnostics.push_back(RuntimeSceneGpuUploadExecutionDiagnostic{
            .status = RuntimeSceneGpuUploadExecutionStatus::failed,
            .asset = failure.asset,
            .message = failure.diagnostic,
        });
    }
    report.status = report.diagnostics.empty() ? RuntimeSceneGpuUploadExecutionStatus::ready
                                               : RuntimeSceneGpuUploadExecutionStatus::failed;
    return report;
}

RuntimeSceneGpuUploadExecutionResult execute_runtime_scene_gpu_upload(const RuntimeSceneGpuUploadExecutionDesc& desc) {
    RuntimeSceneGpuUploadExecutionResult result;
    auto invalid_request = [&result](AssetId asset, std::string message) {
        result.report.status = RuntimeSceneGpuUploadExecutionStatus::invalid_request;
        result.report.diagnostics.push_back(RuntimeSceneGpuUploadExecutionDiagnostic{
            .status = RuntimeSceneGpuUploadExecutionStatus::invalid_request,
            .asset = asset,
            .message = std::move(message),
        });
        return result;
    };

    if (desc.device == nullptr) {
        return invalid_request(AssetId{}, "runtime scene gpu upload execution requires an rhi device");
    }
    if (desc.package == nullptr) {
        return invalid_request(AssetId{}, "runtime scene gpu upload execution requires a runtime asset package");
    }
    if (desc.packet == nullptr) {
        return invalid_request(AssetId{}, "runtime scene gpu upload execution requires a scene render packet");
    }

    result.bindings =
        build_runtime_scene_gpu_binding_palette(*desc.device, *desc.package, *desc.packet, desc.binding_options);
    result.report = make_runtime_scene_gpu_upload_execution_report(result.bindings);
    return result;
}

[[nodiscard]] bool validate_runtime_scene_gpu_binding_device_ownership(const rhi::IRhiDevice& device,
                                                                       const RuntimeSceneGpuBindingResult& bindings,
                                                                       std::string& diagnostic_out) {
    for (const auto& mesh : bindings.mesh_uploads) {
        if (mesh.upload.owner_device != nullptr && mesh.upload.owner_device != &device) {
            diagnostic_out = "runtime scene mesh gpu upload owner_device does not match safe-point teardown device";
            return false;
        }
    }
    for (const auto& skinned : bindings.skinned_mesh_uploads) {
        if (skinned.upload.owner_device != nullptr && skinned.upload.owner_device != &device) {
            diagnostic_out =
                "runtime scene skinned mesh gpu upload owner_device does not match safe-point teardown device";
            return false;
        }
    }
    for (const auto& morph_mesh : bindings.morph_mesh_uploads) {
        if (morph_mesh.upload.owner_device != nullptr && morph_mesh.upload.owner_device != &device) {
            diagnostic_out =
                "runtime scene morph mesh gpu upload owner_device does not match safe-point teardown device";
            return false;
        }
    }
    for (const auto& compute_morph : bindings.compute_morph_skinned_mesh_bindings) {
        if (compute_morph.base_position_upload.owner_device != nullptr &&
            compute_morph.base_position_upload.owner_device != &device) {
            diagnostic_out = "runtime scene compute morph skinned base upload owner_device does not match safe-point "
                             "teardown device";
            return false;
        }
        if (compute_morph.compute_binding.owner_device != nullptr &&
            compute_morph.compute_binding.owner_device != &device) {
            diagnostic_out =
                "runtime scene compute morph skinned binding owner_device does not match safe-point teardown device";
            return false;
        }
    }
    for (const auto& texture : bindings.texture_uploads) {
        if (texture.upload.owner_device != nullptr && texture.upload.owner_device != &device) {
            diagnostic_out = "runtime scene texture gpu upload owner_device does not match safe-point teardown device";
            return false;
        }
    }
    for (const auto& material : bindings.material_bindings) {
        if (material.binding.owner_device != nullptr && material.binding.owner_device != &device) {
            diagnostic_out =
                "runtime scene material gpu binding owner_device does not match safe-point teardown device";
            return false;
        }
    }
    return true;
}

void release_descriptor_write_resources(rhi::NullRhiDevice& device,
                                        const mirakana::runtime_rhi::RuntimeMaterialGpuBinding& binding,
                                        RuntimeSceneGpuSafePointTeardownReport& report) {
    for (const auto& write : binding.writes) {
        for (const auto& resource : write.resources) {
            if (resource.type == rhi::DescriptorType::uniform_buffer ||
                resource.type == rhi::DescriptorType::storage_buffer) {
                if (resource.buffer_handle.value != 0 && device.null_mark_buffer_released(resource.buffer_handle)) {
                    ++report.buffers_released;
                }
            } else if (resource.type == rhi::DescriptorType::sampled_texture ||
                       resource.type == rhi::DescriptorType::storage_texture) {
                if (resource.texture_handle.value != 0 && device.null_mark_texture_released(resource.texture_handle)) {
                    ++report.textures_released;
                }
            } else if (resource.type == rhi::DescriptorType::sampler) {
                if (resource.sampler_handle.value != 0 && device.null_mark_sampler_released(resource.sampler_handle)) {
                    ++report.samplers_released;
                }
            }
        }
    }
}

void teardown_runtime_scene_gpu_bindings_on_null_device(rhi::NullRhiDevice& device,
                                                        const RuntimeSceneGpuBindingResult& bindings,
                                                        RuntimeSceneGpuSafePointTeardownReport& report) {
    std::unordered_set<std::uint32_t> released_pipeline_layouts;
    std::unordered_set<std::uint32_t> released_descriptor_set_layouts;

    for (auto it = bindings.material_bindings.rbegin(); it != bindings.material_bindings.rend(); ++it) {
        const auto& material = *it;
        if (device.null_mark_descriptor_set_released(material.binding.descriptor_set)) {
            ++report.descriptor_sets_released;
        }

        if (released_pipeline_layouts.insert(material.pipeline_layout.value).second) {
            if (device.null_mark_pipeline_layout_released(material.pipeline_layout)) {
                ++report.pipeline_layouts_released;
            }
        }

        if (device.null_mark_buffer_released(material.binding.uniform_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(material.binding.uniform_upload_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(material.binding.scene_pbr_frame_buffer)) {
            ++report.buffers_released;
        }

        for (const auto& sampler : material.binding.samplers) {
            if (device.null_mark_sampler_released(sampler)) {
                ++report.samplers_released;
            }
        }

        release_descriptor_write_resources(device, material.binding, report);

        const auto layout = material.binding.descriptor_set_layout;
        if (layout.value != 0 && released_descriptor_set_layouts.insert(layout.value).second) {
            if (device.null_mark_descriptor_set_layout_released(layout)) {
                ++report.descriptor_set_layouts_released;
            }
        }
    }

    for (auto it = bindings.material_pipeline_layouts.rbegin(); it != bindings.material_pipeline_layouts.rend(); ++it) {
        if (released_pipeline_layouts.contains(it->value)) {
            continue;
        }
        if (device.null_mark_pipeline_layout_released(*it)) {
            ++report.pipeline_layouts_released;
        }
    }

    for (const auto& skinned_entry : bindings.skinned_palette.skinned_entries()) {
        if (device.null_mark_descriptor_set_released(skinned_entry.binding.joint_descriptor_set)) {
            ++report.descriptor_sets_released;
        }
    }
    if (bindings.skinned_joint_descriptor_set_layout.value != 0) {
        if (device.null_mark_descriptor_set_layout_released(bindings.skinned_joint_descriptor_set_layout)) {
            ++report.descriptor_set_layouts_released;
        }
    }

    for (auto it = bindings.skinned_mesh_uploads.rbegin(); it != bindings.skinned_mesh_uploads.rend(); ++it) {
        const auto& upload = it->upload;
        if (device.null_mark_buffer_released(upload.vertex_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.index_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.joint_palette_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.vertex_upload_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.index_upload_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.joint_palette_upload_buffer)) {
            ++report.buffers_released;
        }
    }

    for (auto it = bindings.compute_morph_skinned_mesh_bindings.rbegin();
         it != bindings.compute_morph_skinned_mesh_bindings.rend(); ++it) {
        const auto& base_upload = it->base_position_upload;
        const auto& compute_binding = it->compute_binding;

        if (device.null_mark_descriptor_set_released(compute_binding.descriptor_set)) {
            ++report.descriptor_sets_released;
        }
        const auto compute_layout = compute_binding.descriptor_set_layout;
        if (compute_layout.value != 0 && released_descriptor_set_layouts.insert(compute_layout.value).second) {
            if (device.null_mark_descriptor_set_layout_released(compute_layout)) {
                ++report.descriptor_set_layouts_released;
            }
        }

        if (!compute_binding.output_slots.empty()) {
            for (const auto& slot : compute_binding.output_slots) {
                if (device.null_mark_buffer_released(slot.output_position_buffer)) {
                    ++report.buffers_released;
                }
                if (device.null_mark_buffer_released(slot.output_normal_buffer)) {
                    ++report.buffers_released;
                }
                if (device.null_mark_buffer_released(slot.output_tangent_buffer)) {
                    ++report.buffers_released;
                }
            }
        } else {
            if (device.null_mark_buffer_released(compute_binding.output_position_buffer)) {
                ++report.buffers_released;
            }
            if (device.null_mark_buffer_released(compute_binding.output_normal_buffer)) {
                ++report.buffers_released;
            }
            if (device.null_mark_buffer_released(compute_binding.output_tangent_buffer)) {
                ++report.buffers_released;
            }
        }
        if (device.null_mark_buffer_released(base_upload.vertex_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(base_upload.index_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(base_upload.vertex_upload_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(base_upload.index_upload_buffer)) {
            ++report.buffers_released;
        }
    }

    for (auto it = bindings.mesh_uploads.rbegin(); it != bindings.mesh_uploads.rend(); ++it) {
        const auto& upload = it->upload;
        if (device.null_mark_buffer_released(upload.vertex_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.index_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.vertex_upload_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.index_upload_buffer)) {
            ++report.buffers_released;
        }
    }

    for (const auto& morph_entry : bindings.morph_palette.morph_entries()) {
        if (device.null_mark_descriptor_set_released(morph_entry.binding.morph_descriptor_set)) {
            ++report.descriptor_sets_released;
        }
    }
    if (bindings.morph_descriptor_set_layout.value != 0) {
        if (device.null_mark_descriptor_set_layout_released(bindings.morph_descriptor_set_layout)) {
            ++report.descriptor_set_layouts_released;
        }
    }

    for (auto it = bindings.morph_mesh_uploads.rbegin(); it != bindings.morph_mesh_uploads.rend(); ++it) {
        const auto& upload = it->upload;
        if (device.null_mark_buffer_released(upload.position_delta_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.normal_delta_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.tangent_delta_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.morph_weight_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.position_delta_upload_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.normal_delta_upload_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.tangent_delta_upload_buffer)) {
            ++report.buffers_released;
        }
        if (device.null_mark_buffer_released(upload.morph_weight_upload_buffer)) {
            ++report.buffers_released;
        }
    }

    for (auto it = bindings.texture_uploads.rbegin(); it != bindings.texture_uploads.rend(); ++it) {
        const auto& upload = it->upload;
        if (device.null_mark_texture_released(upload.texture)) {
            ++report.textures_released;
        }
        if (device.null_mark_buffer_released(upload.upload_buffer)) {
            ++report.buffers_released;
        }
    }
}

RuntimeSceneGpuSafePointTeardownReport
execute_runtime_scene_gpu_binding_safe_point_teardown(rhi::IRhiDevice& device,
                                                      const RuntimeSceneGpuBindingResult& bindings) {
    RuntimeSceneGpuSafePointTeardownReport report;
    if (!bindings.succeeded()) {
        report.status = RuntimeSceneGpuSafePointTeardownStatus::skipped_binding_failed;
        report.diagnostics.emplace_back("runtime scene gpu binding contains failures; safe-point teardown skipped");
        return report;
    }

    std::string ownership_diagnostic;
    if (!validate_runtime_scene_gpu_binding_device_ownership(device, bindings, ownership_diagnostic)) {
        report.status = RuntimeSceneGpuSafePointTeardownStatus::ownership_mismatch;
        report.diagnostics.push_back(std::move(ownership_diagnostic));
        return report;
    }

    auto* null_device = dynamic_cast<rhi::NullRhiDevice*>(&device);
    if (null_device == nullptr) {
        report.status = RuntimeSceneGpuSafePointTeardownStatus::host_native_destroy_pipeline_required;
        report.diagnostics.emplace_back(
            "runtime scene gpu safe-point teardown requires host-native IRhiDevice resource destruction after the "
            "final submitted GPU frame; gameplay APIs do not expose native handles");
        return report;
    }

    teardown_runtime_scene_gpu_bindings_on_null_device(*null_device, bindings, report);
    report.status = RuntimeSceneGpuSafePointTeardownStatus::completed_on_null_device;
    return report;
}

} // namespace mirakana::runtime_scene_rhi
