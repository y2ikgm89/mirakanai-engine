// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime_rhi {

struct RuntimePackageStreamingFrameGraphTextureBindingSource {
    AssetId asset;
    std::string resource;
    const RuntimeTextureUploadResult* upload{nullptr};
    rhi::ResourceState current_state{rhi::ResourceState::shader_read};
};

struct RuntimePackageStreamingFrameGraphTextureUploadSource {
    AssetId asset;
    std::string resource;
    const runtime::RuntimeTexturePayload* payload{nullptr};
    RuntimeTextureUploadOptions upload_options{};
    rhi::ResourceState current_state{rhi::ResourceState::shader_read};
};

struct RuntimePackageStreamingMeshUploadSource {
    AssetId asset;
    const runtime::RuntimeMeshPayload* payload{nullptr};
    RuntimeMeshUploadOptions upload_options{};
};

struct RuntimePackageStreamingSkinnedMeshUploadSource {
    AssetId asset;
    const runtime::RuntimeSkinnedMeshPayload* payload{nullptr};
    RuntimeSkinnedMeshUploadOptions upload_options{};
};

struct RuntimePackageStreamingMorphMeshUploadSource {
    AssetId asset;
    const runtime::RuntimeMorphMeshCpuPayload* payload{nullptr};
    RuntimeMorphMeshUploadOptions upload_options{};
};

struct RuntimePackageStreamingMeshGpuBinding {
    AssetId asset;
    MeshGpuBinding binding;
};

struct RuntimePackageStreamingFrameGraphTextureGpuBinding {
    AssetId asset;
    std::string resource;
    FrameGraphTextureBinding binding;
};

struct RuntimePackageStreamingSkinnedMeshGpuBinding {
    AssetId asset;
    SkinnedMeshGpuBinding binding;
};

struct RuntimePackageStreamingMorphMeshGpuBinding {
    AssetId asset;
    MorphMeshGpuBinding binding;
};

struct RuntimePackageStreamingFrameGraphTextureBindingDiagnostic {
    AssetId asset;
    std::string resource;
    std::string code;
    std::string message;
};

struct RuntimePackageStreamingMeshUploadDiagnostic {
    AssetId asset;
    std::string code;
    std::string message;
};

struct RuntimePackageStreamingFrameGraphTextureBindingResult {
    std::vector<FrameGraphTextureBinding> texture_bindings;
    std::vector<RuntimePackageStreamingFrameGraphTextureBindingDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimePackageStreamingFrameGraphTextureUploadBindingResult {
    std::vector<RuntimeTextureUploadResult> uploads;
    std::vector<FrameGraphTextureBinding> texture_bindings;
    std::vector<RuntimePackageStreamingFrameGraphTextureGpuBinding> texture_resource_bindings;
    std::vector<RuntimePackageStreamingFrameGraphTextureBindingDiagnostic> diagnostics;
    std::vector<rhi::FenceValue> submitted_fences;
    std::uint64_t uploaded_bytes{0};
    std::size_t upload_queue_waits_recorded{0};
    std::size_t frame_graph_barriers_recorded{0};
    std::size_t frame_graph_pass_target_state_barriers_recorded{0};
    std::size_t frame_graph_final_state_barriers_recorded{0};
    std::size_t frame_graph_pass_callbacks_invoked{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimePackageStreamingMeshUploadBindingResult {
    std::vector<RuntimeMeshUploadResult> uploads;
    std::vector<RuntimePackageStreamingMeshGpuBinding> mesh_bindings;
    std::vector<RuntimePackageStreamingMeshUploadDiagnostic> diagnostics;
    std::vector<rhi::FenceValue> submitted_fences;
    std::uint64_t uploaded_bytes{0};
    std::size_t frame_graph_command_lists_submitted{0};
    std::size_t upload_queue_waits_recorded{0};
    std::size_t frame_graph_queue_waits_recorded{0};
    std::size_t frame_graph_barriers_recorded{0};
    std::size_t frame_graph_pass_callbacks_invoked{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimePackageStreamingSkinnedMeshUploadBindingResult {
    std::vector<RuntimeSkinnedMeshUploadResult> uploads;
    std::vector<RuntimePackageStreamingSkinnedMeshGpuBinding> skinned_mesh_bindings;
    std::vector<RuntimePackageStreamingMeshUploadDiagnostic> diagnostics;
    std::vector<rhi::FenceValue> submitted_fences;
    std::uint64_t uploaded_bytes{0};
    std::size_t frame_graph_command_lists_submitted{0};
    std::size_t upload_queue_waits_recorded{0};
    std::size_t frame_graph_queue_waits_recorded{0};
    std::size_t frame_graph_barriers_recorded{0};
    std::size_t frame_graph_pass_callbacks_invoked{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimePackageStreamingMorphMeshUploadBindingResult {
    std::vector<RuntimeMorphMeshUploadResult> uploads;
    std::vector<RuntimePackageStreamingMorphMeshGpuBinding> morph_mesh_bindings;
    std::vector<RuntimePackageStreamingMeshUploadDiagnostic> diagnostics;
    std::vector<rhi::FenceValue> submitted_fences;
    std::uint64_t uploaded_bytes{0};
    std::size_t frame_graph_command_lists_submitted{0};
    std::size_t upload_queue_waits_recorded{0};
    std::size_t frame_graph_queue_waits_recorded{0};
    std::size_t frame_graph_barriers_recorded{0};
    std::size_t frame_graph_pass_callbacks_invoked{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

enum class RuntimePackageResourceUpdateKind : std::uint8_t { texture = 0, static_mesh, skinned_mesh, morph_mesh };

struct RuntimePackageResourceUpdate {
    AssetId asset;
    RuntimePackageResourceUpdateKind kind{RuntimePackageResourceUpdateKind::texture};
    AssetKind catalog_kind{AssetKind::unknown};
    runtime::RuntimeResourceHandleV2 resource_handle;
    runtime::RuntimeAssetHandle package_handle;
    std::string resource;
    rhi::FenceValue submitted_upload_fence{};
    bool graphics_queue_ready{false};
    bool graphics_queue_wait_recorded{false};
    bool same_queue_graphics_order{false};
};

struct RuntimePackageResourceUpdateDiagnostic {
    AssetId asset;
    RuntimePackageResourceUpdateKind kind{RuntimePackageResourceUpdateKind::texture};
    std::string code;
    std::string message;
};

struct RuntimePackageResourceUpdateReadinessSources {
    const RuntimePackageStreamingFrameGraphTextureUploadBindingResult* texture_uploads{nullptr};
    const RuntimePackageStreamingMeshUploadBindingResult* mesh_uploads{nullptr};
    const RuntimePackageStreamingSkinnedMeshUploadBindingResult* skinned_mesh_uploads{nullptr};
    const RuntimePackageStreamingMorphMeshUploadBindingResult* morph_mesh_uploads{nullptr};
};

struct RuntimePackageResourceUpdateReadinessResult {
    bool ready{false};
    std::vector<RuntimePackageResourceUpdate> updates;
    std::vector<RuntimePackageResourceUpdateDiagnostic> diagnostics;
    std::size_t texture_updates{0};
    std::size_t mesh_updates{0};
    std::size_t skinned_mesh_updates{0};
    std::size_t morph_mesh_updates{0};
    std::size_t submitted_fences{0};
    std::size_t graphics_queue_ready_updates{0};
    std::size_t graphics_queue_waits_recorded{0};
    std::size_t same_queue_graphics_updates{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RuntimePackageUploadStagingEvidenceDiagnostic {
    std::string code;
    std::string message;
};

struct RuntimePackageUploadStagingEvidence {
    bool ready{false};
    std::size_t package_transactions{0};
    std::size_t texture_uploads{0};
    std::size_t mesh_uploads{0};
    std::size_t skinned_mesh_uploads{0};
    std::size_t morph_mesh_uploads{0};
    std::size_t texture_bindings{0};
    std::size_t mesh_bindings{0};
    std::size_t skinned_mesh_bindings{0};
    std::size_t morph_mesh_bindings{0};
    std::size_t staging_pool_leases{0};
    std::size_t ring_backed_uploads{0};
    bool resource_updates_ready{false};
    std::size_t resource_updates{0};
    std::size_t resource_update_submitted_fences{0};
    std::size_t resource_update_graphics_ready_updates{0};
    std::size_t resource_update_graphics_queue_waits_recorded{0};
    std::size_t resource_update_same_queue_graphics_updates{0};
    std::uint64_t uploaded_bytes{0};
    std::size_t submitted_fences{0};
    std::size_t upload_queue_waits_recorded{0};
    std::uint64_t copy_queue_submits{0};
    std::uint64_t graphics_queue_submits{0};
    std::uint64_t queue_waits{0};
    std::uint64_t fence_waits{0};
    bool graphics_waited_for_copy{false};
    std::vector<RuntimePackageUploadStagingEvidenceDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimePackageStreamingFrameGraphTextureBindingResult
make_runtime_package_streaming_frame_graph_texture_bindings(
    const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalogV2& resident_catalog,
    std::span<const RuntimePackageStreamingFrameGraphTextureBindingSource> sources);

[[nodiscard]] RuntimePackageStreamingFrameGraphTextureUploadBindingResult
upload_runtime_package_streaming_frame_graph_texture_bindings(
    rhi::IRhiDevice& device, const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalogV2& resident_catalog,
    std::span<const RuntimePackageStreamingFrameGraphTextureUploadSource> sources);

[[nodiscard]] RuntimePackageStreamingMeshUploadBindingResult upload_runtime_package_streaming_mesh_gpu_bindings(
    rhi::IRhiDevice& device, const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalogV2& resident_catalog,
    std::span<const RuntimePackageStreamingMeshUploadSource> sources);

[[nodiscard]] RuntimePackageStreamingSkinnedMeshUploadBindingResult
upload_runtime_package_streaming_skinned_mesh_gpu_bindings(
    rhi::IRhiDevice& device, const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalogV2& resident_catalog,
    std::span<const RuntimePackageStreamingSkinnedMeshUploadSource> sources);

[[nodiscard]] RuntimePackageStreamingMorphMeshUploadBindingResult
upload_runtime_package_streaming_morph_mesh_gpu_bindings(
    rhi::IRhiDevice& device, const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalogV2& resident_catalog,
    std::span<const RuntimePackageStreamingMorphMeshUploadSource> sources);

[[nodiscard]] RuntimePackageResourceUpdateReadinessResult
make_runtime_package_resource_update_readiness(const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
                                               const runtime::RuntimeResourceCatalogV2& resident_catalog,
                                               const RuntimePackageResourceUpdateReadinessSources& sources);

[[nodiscard]] RuntimePackageUploadStagingEvidence
execute_runtime_package_upload_staging_evidence(rhi::IRhiDevice& device);

} // namespace mirakana::runtime_rhi
