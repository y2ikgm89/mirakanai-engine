// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_scene_rhi {

struct RuntimeSceneGpuBindingFailure {
    AssetId asset;
    std::string diagnostic;
};

struct RuntimeSceneMeshGpuResource {
    AssetId mesh;
    runtime_rhi::RuntimeMeshUploadResult upload;
};

struct RuntimeSceneSkinnedMeshGpuResource {
    AssetId mesh;
    runtime_rhi::RuntimeSkinnedMeshUploadResult upload;
};

struct RuntimeSceneMorphMeshGpuResource {
    AssetId morph_mesh;
    runtime_rhi::RuntimeMorphMeshUploadResult upload;
};

struct RuntimeSceneComputeMorphSkinnedMeshBinding {
    AssetId mesh;
    AssetId morph_mesh;
};

struct RuntimeSceneComputeMorphSkinnedMeshGpuResource {
    AssetId mesh;
    AssetId morph_mesh;
    runtime_rhi::RuntimeMeshUploadResult base_position_upload;
    runtime_rhi::RuntimeMorphMeshComputeBinding compute_binding;
};

struct RuntimeSceneTextureGpuResource {
    AssetId texture;
    runtime_rhi::RuntimeTextureUploadResult upload;
};

struct RuntimeSceneMaterialGpuResource {
    AssetId material;
    runtime_rhi::RuntimeMaterialGpuBinding binding;
    rhi::PipelineLayoutHandle pipeline_layout;
};

struct RuntimeSceneGpuBindingOptions {
    runtime_rhi::RuntimeMeshUploadOptions mesh_upload;
    runtime_rhi::RuntimeSkinnedMeshUploadOptions skinned_mesh_upload;
    runtime_rhi::RuntimeMorphMeshUploadOptions morph_mesh_upload;
    runtime_rhi::RuntimeMeshUploadOptions compute_morph_base_mesh_upload;
    runtime_rhi::RuntimeMorphMeshComputeBindingOptions compute_morph;
    runtime_rhi::RuntimeTextureUploadOptions texture_upload;
    std::vector<AssetId> morph_mesh_assets;
    std::vector<RuntimeSceneComputeMorphSkinnedMeshBinding> compute_morph_skinned_mesh_bindings;
    rhi::DescriptorSetLayoutHandle shared_material_descriptor_set_layout;
    std::vector<rhi::DescriptorSetLayoutHandle> additional_pipeline_descriptor_set_layouts;
    rhi::BufferHandle shared_scene_pbr_frame_uniform{};
};

struct RuntimeSceneGpuBindingResult {
    SceneGpuBindingPalette palette;
    SceneSkinnedGpuBindingPalette skinned_palette;
    SceneMorphGpuBindingPalette morph_palette;
    std::vector<RuntimeSceneGpuBindingFailure> failures;
    std::vector<RuntimeSceneMeshGpuResource> mesh_uploads;
    std::vector<RuntimeSceneSkinnedMeshGpuResource> skinned_mesh_uploads;
    std::vector<RuntimeSceneMorphMeshGpuResource> morph_mesh_uploads;
    std::vector<RuntimeSceneComputeMorphSkinnedMeshGpuResource> compute_morph_skinned_mesh_bindings;
    std::vector<RuntimeSceneTextureGpuResource> texture_uploads;
    std::vector<RuntimeSceneMaterialGpuResource> material_bindings;
    std::vector<rhi::PipelineLayoutHandle> material_pipeline_layouts;
    std::vector<rhi::FenceValue> submitted_upload_fences;
    rhi::BufferHandle scene_pbr_frame_uniform_buffer{};
    rhi::DescriptorSetLayoutHandle skinned_joint_descriptor_set_layout{};
    rhi::DescriptorSetLayoutHandle morph_descriptor_set_layout{};

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

enum class RuntimeSceneGpuUploadExecutionStatus : std::uint8_t {
    not_requested = 0,
    invalid_request,
    failed,
    ready,
};

struct RuntimeSceneGpuUploadExecutionDiagnostic {
    RuntimeSceneGpuUploadExecutionStatus status{RuntimeSceneGpuUploadExecutionStatus::not_requested};
    AssetId asset;
    std::string message;
};

struct RuntimeSceneGpuUploadExecutionReport {
    RuntimeSceneGpuUploadExecutionStatus status{RuntimeSceneGpuUploadExecutionStatus::not_requested};
    std::size_t mesh_uploads{0};
    std::size_t skinned_mesh_uploads{0};
    std::size_t morph_mesh_uploads{0};
    std::size_t compute_morph_skinned_mesh_bindings{0};
    std::size_t texture_uploads{0};
    std::size_t material_bindings{0};
    std::size_t material_pipeline_layouts{0};
    std::uint64_t uploaded_texture_bytes{0};
    std::uint64_t uploaded_mesh_bytes{0};
    std::uint64_t uploaded_morph_bytes{0};
    std::uint64_t uploaded_compute_morph_base_position_bytes{0};
    std::uint64_t compute_morph_output_position_bytes{0};
    std::uint64_t uploaded_material_factor_bytes{0};
    std::size_t material_descriptor_writes{0};
    std::size_t submitted_upload_fence_count{0};
    rhi::FenceValue last_submitted_upload_fence{};
    std::vector<RuntimeSceneGpuUploadExecutionDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return status == RuntimeSceneGpuUploadExecutionStatus::ready;
    }
};

struct RuntimeSceneGpuUploadExecutionDesc {
    rhi::IRhiDevice* device{nullptr};
    const runtime::RuntimeAssetPackage* package{nullptr};
    const SceneRenderPacket* packet{nullptr};
    RuntimeSceneGpuBindingOptions binding_options;
};

struct RuntimeSceneGpuUploadExecutionResult {
    RuntimeSceneGpuBindingResult bindings;
    RuntimeSceneGpuUploadExecutionReport report;

    [[nodiscard]] bool succeeded() const noexcept {
        return report.succeeded();
    }
};

[[nodiscard]] RuntimeSceneGpuBindingResult
build_runtime_scene_gpu_binding_palette(rhi::IRhiDevice& device, const runtime::RuntimeAssetPackage& package,
                                        const SceneRenderPacket& packet,
                                        const RuntimeSceneGpuBindingOptions& options = {});

[[nodiscard]] std::string_view
runtime_scene_gpu_upload_execution_status_name(RuntimeSceneGpuUploadExecutionStatus status) noexcept;

[[nodiscard]] RuntimeSceneGpuUploadExecutionReport
make_runtime_scene_gpu_upload_execution_report(const RuntimeSceneGpuBindingResult& bindings);

[[nodiscard]] RuntimeSceneGpuUploadExecutionResult
execute_runtime_scene_gpu_upload(const RuntimeSceneGpuUploadExecutionDesc& desc);

enum class RuntimeSceneGpuSafePointTeardownStatus : std::uint8_t {
    skipped_binding_failed = 0,
    ownership_mismatch,
    completed_on_null_device,
    host_native_destroy_pipeline_required,
};

struct RuntimeSceneGpuSafePointTeardownReport {
    RuntimeSceneGpuSafePointTeardownStatus status{RuntimeSceneGpuSafePointTeardownStatus::skipped_binding_failed};
    std::size_t buffers_released{0};
    std::size_t textures_released{0};
    std::size_t samplers_released{0};
    std::size_t descriptor_sets_released{0};
    std::size_t pipeline_layouts_released{0};
    std::size_t descriptor_set_layouts_released{0};
    std::vector<std::string> diagnostics;

    [[nodiscard]] bool null_device_teardown_completed() const noexcept {
        return status == RuntimeSceneGpuSafePointTeardownStatus::completed_on_null_device;
    }
};

/// Host-owned safe-point teardown for scene GPU binding results produced from `device`. On `NullRhiDevice`, marks
/// created handles inactive in deterministic order. On real backends, returns `host_native_destroy_pipeline_required`
/// so hosts destroy native resources outside gameplay APIs.
[[nodiscard]] RuntimeSceneGpuSafePointTeardownReport
execute_runtime_scene_gpu_binding_safe_point_teardown(rhi::IRhiDevice& device,
                                                      const RuntimeSceneGpuBindingResult& bindings);

} // namespace mirakana::runtime_scene_rhi
