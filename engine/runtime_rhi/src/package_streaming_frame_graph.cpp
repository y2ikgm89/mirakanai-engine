// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/package_streaming_frame_graph.hpp"

#include "mirakana/rhi/upload_staging.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimePackageStreamingFrameGraphTextureBindingResult& result, AssetId asset, std::string resource,
                    std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageStreamingFrameGraphTextureBindingDiagnostic{
        .asset = asset,
        .resource = std::move(resource),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimePackageStreamingFrameGraphTextureUploadBindingResult& result, AssetId asset,
                    std::string resource, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageStreamingFrameGraphTextureBindingDiagnostic{
        .asset = asset,
        .resource = std::move(resource),
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimePackageStreamingMeshUploadBindingResult& result, AssetId asset, std::string code,
                    std::string message) {
    result.diagnostics.push_back(RuntimePackageStreamingMeshUploadDiagnostic{
        .asset = asset,
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimePackageStreamingSkinnedMeshUploadBindingResult& result, AssetId asset, std::string code,
                    std::string message) {
    result.diagnostics.push_back(RuntimePackageStreamingMeshUploadDiagnostic{
        .asset = asset,
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimePackageStreamingMorphMeshUploadBindingResult& result, AssetId asset, std::string code,
                    std::string message) {
    result.diagnostics.push_back(RuntimePackageStreamingMeshUploadDiagnostic{
        .asset = asset,
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimePackageUploadStagingEvidence& result, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageUploadStagingEvidenceDiagnostic{
        .code = std::move(code),
        .message = std::move(message),
    });
}

void add_diagnostic(RuntimePackageResourceUpdateReadinessResult& result, AssetId asset,
                    RuntimePackageResourceUpdateKind kind, std::string code, std::string message) {
    result.diagnostics.push_back(RuntimePackageResourceUpdateDiagnostic{
        .asset = asset,
        .kind = kind,
        .code = std::move(code),
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_resource(const std::vector<std::string>& resources, const std::string& resource) {
    return std::ranges::find(resources, resource) != resources.end();
}

[[nodiscard]] bool contains_asset(const std::vector<AssetId>& assets, AssetId asset) {
    return std::ranges::find(assets, asset) != assets.end();
}

[[nodiscard]] bool valid_runtime_resource_update_queue_kind(rhi::QueueKind queue) noexcept {
    switch (queue) {
    case rhi::QueueKind::graphics:
    case rhi::QueueKind::compute:
    case rhi::QueueKind::copy:
        return true;
    }
    return false;
}

[[nodiscard]] std::string runtime_upload_queue_wait_diagnostic(const RuntimeUploadQueueWaitResult& wait_result) {
    if (wait_result.diagnostics.empty()) {
        return "runtime upload queue wait failed";
    }
    if (wait_result.diagnostics.front().message.empty()) {
        return "runtime upload queue wait failed";
    }
    return wait_result.diagnostics.front().message;
}

void append_le_u32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
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

[[nodiscard]] runtime::RuntimeAssetRecord make_upload_staging_evidence_record(AssetId asset,
                                                                              runtime::RuntimeAssetHandle handle,
                                                                              AssetKind kind, std::string content) {
    return runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = "runtime/package-upload-staging/" + std::to_string(asset.value) + ".geasset",
        .content_hash = static_cast<std::uint64_t>(asset.value) + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = std::move(content),
    };
}

[[nodiscard]] runtime::RuntimeTexturePayload make_upload_staging_texture_payload(AssetId asset,
                                                                                 runtime::RuntimeAssetHandle handle) {
    return runtime::RuntimeTexturePayload{
        .asset = asset,
        .handle = handle,
        .width = 1,
        .height = 1,
        .pixel_format = TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{0x22, 0x33, 0x44, 0xff},
    };
}

[[nodiscard]] runtime::RuntimeMeshPayload make_upload_staging_mesh_payload(AssetId asset,
                                                                           runtime::RuntimeAssetHandle handle) {
    return runtime::RuntimeMeshPayload{
        .asset = asset,
        .handle = handle,
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(36, std::uint8_t{0x61}),
        .index_bytes =
            std::vector<std::uint8_t>{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
    };
}

[[nodiscard]] runtime::RuntimeSkinnedMeshPayload
make_upload_staging_skinned_mesh_payload(AssetId asset, runtime::RuntimeAssetHandle handle) {
    return runtime::RuntimeSkinnedMeshPayload{
        .asset = asset,
        .handle = handle,
        .vertex_count = 3,
        .index_count = 3,
        .joint_count = 1,
        .vertex_bytes = std::vector<std::uint8_t>(3U * runtime_skinned_mesh_vertex_stride_bytes, std::uint8_t{0x62}),
        .index_bytes =
            std::vector<std::uint8_t>{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
        .joint_palette_bytes = std::vector<std::uint8_t>(runtime_skinned_mesh_joint_matrix_bytes, std::uint8_t{0x00}),
    };
}

[[nodiscard]] runtime::RuntimeMorphMeshCpuPayload
make_upload_staging_morph_mesh_payload(AssetId asset, runtime::RuntimeAssetHandle handle) {
    MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    append_vec3(morph.bind_position_bytes, -0.6F, 0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, 0.15F, -0.75F, 0.0F);
    append_vec3(morph.bind_position_bytes, -1.35F, -0.75F, 0.0F);

    MorphMeshCpuTargetSourceDocument target;
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    append_vec3(target.position_delta_bytes, 0.6F, 0.0F, 0.0F);
    morph.targets.push_back(std::move(target));
    append_le_f32(morph.target_weight_bytes, 1.0F);

    return runtime::RuntimeMorphMeshCpuPayload{
        .asset = asset,
        .handle = handle,
        .morph = std::move(morph),
    };
}

[[nodiscard]] runtime::RuntimePackageStreamingExecutionResult make_upload_staging_streaming_result() {
    runtime::RuntimePackageStreamingExecutionResult result;
    result.status = runtime::RuntimePackageStreamingExecutionStatus::committed;
    result.target_id = "package-upload-staging-evidence";
    result.package_index_path = "runtime/package_upload_staging.geindex";
    result.runtime_scene_validation_target_id = "package-upload-staging";
    result.committed = true;
    return result;
}

} // namespace

RuntimePackageStreamingFrameGraphTextureBindingResult make_runtime_package_streaming_frame_graph_texture_bindings(
    const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalog& resident_catalog,
    std::span<const RuntimePackageStreamingFrameGraphTextureBindingSource> sources) {
    RuntimePackageStreamingFrameGraphTextureBindingResult result;

    if (!streaming_result.succeeded()) {
        add_diagnostic(result, {}, {}, "package-streaming-not-committed",
                       "package streaming result must be committed before building frame graph texture bindings");
        return result;
    }

    std::vector<std::string> seen_resources;
    seen_resources.reserve(sources.size());
    for (const auto& source : sources) {
        if (source.resource.empty()) {
            add_diagnostic(result, source.asset, source.resource, "empty-frame-graph-resource",
                           "frame graph texture binding resource name is required");
        } else if (contains_resource(seen_resources, source.resource)) {
            add_diagnostic(result, source.asset, source.resource, "duplicate-frame-graph-resource",
                           "frame graph texture binding resource names must be unique");
        } else {
            seen_resources.push_back(source.resource);
        }

        const auto handle = runtime::find_runtime_resource(resident_catalog, source.asset);
        if (!handle.has_value()) {
            add_diagnostic(result, source.asset, source.resource, "runtime-resource-not-live",
                           "texture asset is not live in the resident runtime resource catalog");
        } else {
            const auto* const record = runtime::runtime_resource_record(resident_catalog, *handle);
            if (record == nullptr) {
                add_diagnostic(result, source.asset, source.resource, "runtime-resource-not-live",
                               "texture asset catalog handle is stale");
            } else if (record->kind != AssetKind::texture) {
                add_diagnostic(result, source.asset, source.resource, "runtime-resource-not-texture",
                               "resident runtime resource is not a texture asset");
            }
        }

        if (source.upload == nullptr) {
            add_diagnostic(result, source.asset, source.resource, "texture-upload-missing",
                           "runtime texture upload result is required");
        } else if (!source.upload->succeeded()) {
            add_diagnostic(result, source.asset, source.resource, "texture-upload-failed",
                           "runtime texture upload result must have succeeded");
        } else if (source.upload->texture.value == 0) {
            add_diagnostic(result, source.asset, source.resource, "texture-upload-empty",
                           "runtime texture upload result must contain a non-null texture handle");
        } else if (source.upload->owner_device == nullptr) {
            add_diagnostic(result, source.asset, source.resource, "texture-upload-owner-missing",
                           "runtime texture upload result must include owner device provenance");
        }
    }

    if (!result.succeeded()) {
        return result;
    }

    result.texture_bindings.reserve(sources.size());
    for (const auto& source : sources) {
        result.texture_bindings.push_back(FrameGraphTextureBinding{
            .resource = source.resource,
            .texture = source.upload->texture,
            .current_state = source.current_state,
        });
    }
    return result;
}

RuntimePackageStreamingFrameGraphTextureUploadBindingResult
upload_runtime_package_streaming_frame_graph_texture_bindings(
    rhi::IRhiDevice& device, const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalog& resident_catalog,
    std::span<const RuntimePackageStreamingFrameGraphTextureUploadSource> sources) {
    RuntimePackageStreamingFrameGraphTextureUploadBindingResult result;

    if (!streaming_result.succeeded()) {
        add_diagnostic(result, {}, {}, "package-streaming-not-committed",
                       "package streaming result must be committed before uploading frame graph texture bindings");
        return result;
    }

    std::vector<std::string> seen_resources;
    seen_resources.reserve(sources.size());
    for (const auto& source : sources) {
        if (source.resource.empty()) {
            add_diagnostic(result, source.asset, source.resource, "empty-frame-graph-resource",
                           "frame graph texture binding resource name is required");
        } else if (contains_resource(seen_resources, source.resource)) {
            add_diagnostic(result, source.asset, source.resource, "duplicate-frame-graph-resource",
                           "frame graph texture binding resource names must be unique");
        } else {
            seen_resources.push_back(source.resource);
        }

        const auto handle = runtime::find_runtime_resource(resident_catalog, source.asset);
        const runtime::RuntimeResourceRecord* record = nullptr;
        if (!handle.has_value()) {
            add_diagnostic(result, source.asset, source.resource, "runtime-resource-not-live",
                           "texture asset is not live in the resident runtime resource catalog");
        } else {
            record = runtime::runtime_resource_record(resident_catalog, *handle);
            if (record == nullptr) {
                add_diagnostic(result, source.asset, source.resource, "runtime-resource-not-live",
                               "texture asset catalog handle is stale");
            } else if (record->kind != AssetKind::texture) {
                add_diagnostic(result, source.asset, source.resource, "runtime-resource-not-texture",
                               "resident runtime resource is not a texture asset");
            }
        }

        if (source.payload == nullptr) {
            add_diagnostic(result, source.asset, source.resource, "texture-payload-missing",
                           "runtime texture payload is required before uploading frame graph texture bindings");
            continue;
        }
        if (source.payload->asset != source.asset) {
            add_diagnostic(result, source.asset, source.resource, "texture-payload-asset-mismatch",
                           "runtime texture payload asset must match the selected resident texture asset");
        }
        if (record != nullptr && source.payload->handle != record->package_handle) {
            add_diagnostic(result, source.asset, source.resource, "texture-payload-handle-mismatch",
                           "runtime texture payload handle must match the selected resident texture handle");
        }
    }

    if (!result.succeeded()) {
        return result;
    }

    result.uploads.reserve(sources.size());
    for (const auto& source : sources) {
        auto upload = upload_runtime_texture(device, *source.payload, source.upload_options);
        if (!upload.succeeded()) {
            add_diagnostic(result, source.asset, source.resource, "texture-upload-failed", upload.diagnostic);
        }
        result.uploaded_bytes += upload.uploaded_bytes;
        result.frame_graph_barriers_recorded += upload.frame_graph_barriers_recorded;
        result.frame_graph_pass_target_state_barriers_recorded +=
            upload.frame_graph_pass_target_state_barriers_recorded;
        result.frame_graph_final_state_barriers_recorded += upload.frame_graph_final_state_barriers_recorded;
        result.frame_graph_pass_callbacks_invoked += upload.frame_graph_pass_callbacks_invoked;
        if (upload.submitted_fence.value != 0) {
            result.submitted_fences.push_back(upload.submitted_fence);
        }
        result.uploads.push_back(std::move(upload));
    }

    if (!result.succeeded()) {
        return result;
    }

    std::vector<rhi::FenceValue> async_upload_fences;
    async_upload_fences.reserve(result.submitted_fences.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        const auto& upload = result.uploads[index];
        if (!sources[index].upload_options.wait_for_completion && upload.submitted_fence.value != 0) {
            async_upload_fences.push_back(upload.submitted_fence);
        }
    }

    const auto queue_wait = wait_for_runtime_uploads_on_queue(device, rhi::QueueKind::graphics, async_upload_fences);
    result.upload_queue_waits_recorded += queue_wait.queue_waits_recorded;
    if (!queue_wait.succeeded()) {
        add_diagnostic(result, {}, {}, "texture-upload-queue-wait-failed",
                       runtime_upload_queue_wait_diagnostic(queue_wait));
        return result;
    }

    result.texture_bindings.reserve(sources.size());
    result.texture_resource_bindings.reserve(sources.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        result.texture_bindings.push_back(FrameGraphTextureBinding{
            .resource = sources[index].resource,
            .texture = result.uploads[index].texture,
            .current_state = sources[index].current_state,
        });
        result.texture_resource_bindings.push_back(RuntimePackageStreamingFrameGraphTextureGpuBinding{
            .asset = sources[index].asset,
            .resource = sources[index].resource,
            .binding = result.texture_bindings.back(),
        });
    }
    return result;
}

RuntimePackageStreamingMeshUploadBindingResult upload_runtime_package_streaming_mesh_gpu_bindings(
    rhi::IRhiDevice& device, const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalog& resident_catalog,
    std::span<const RuntimePackageStreamingMeshUploadSource> sources) {
    RuntimePackageStreamingMeshUploadBindingResult result;

    if (!streaming_result.succeeded()) {
        add_diagnostic(result, {}, "package-streaming-not-committed",
                       "package streaming result must be committed before uploading mesh GPU bindings");
        return result;
    }

    std::vector<AssetId> seen_assets;
    seen_assets.reserve(sources.size());
    for (const auto& source : sources) {
        if (contains_asset(seen_assets, source.asset)) {
            add_diagnostic(result, source.asset, "duplicate-mesh-asset",
                           "package streaming mesh upload source assets must be unique");
        } else {
            seen_assets.push_back(source.asset);
        }

        const auto handle = runtime::find_runtime_resource(resident_catalog, source.asset);
        const runtime::RuntimeResourceRecord* record = nullptr;
        if (!handle.has_value()) {
            add_diagnostic(result, source.asset, "runtime-resource-not-live",
                           "mesh asset is not live in the resident runtime resource catalog");
        } else {
            record = runtime::runtime_resource_record(resident_catalog, *handle);
            if (record == nullptr) {
                add_diagnostic(result, source.asset, "runtime-resource-not-live", "mesh asset catalog handle is stale");
            } else if (record->kind != AssetKind::mesh) {
                add_diagnostic(result, source.asset, "runtime-resource-not-mesh",
                               "resident runtime resource is not a static mesh asset");
            }
        }

        if (source.payload == nullptr) {
            add_diagnostic(result, source.asset, "mesh-payload-missing",
                           "runtime mesh payload is required before uploading mesh GPU bindings");
            continue;
        }
        if (source.payload->asset != source.asset) {
            add_diagnostic(result, source.asset, "mesh-payload-asset-mismatch",
                           "runtime mesh payload asset must match the selected resident mesh asset");
        }
        if (record != nullptr && source.payload->handle != record->package_handle) {
            add_diagnostic(result, source.asset, "mesh-payload-handle-mismatch",
                           "runtime mesh payload handle must match the selected resident mesh handle");
        }
    }

    if (!result.succeeded()) {
        return result;
    }

    result.uploads.reserve(sources.size());
    for (const auto& source : sources) {
        auto upload = upload_runtime_mesh(device, *source.payload, source.upload_options);
        if (!upload.succeeded()) {
            add_diagnostic(result, source.asset, "mesh-upload-failed", upload.diagnostic);
        }
        result.uploaded_bytes += upload.uploaded_vertex_bytes + upload.uploaded_index_bytes;
        result.frame_graph_command_lists_submitted += upload.frame_graph_command_lists_submitted;
        result.frame_graph_queue_waits_recorded += upload.frame_graph_queue_waits_recorded;
        result.frame_graph_barriers_recorded += upload.frame_graph_barriers_recorded;
        result.frame_graph_pass_callbacks_invoked += upload.frame_graph_pass_callbacks_invoked;
        if (upload.submitted_fence.value != 0) {
            result.submitted_fences.push_back(upload.submitted_fence);
        }
        result.uploads.push_back(std::move(upload));
    }

    if (!result.succeeded()) {
        return result;
    }

    std::vector<rhi::FenceValue> async_upload_fences;
    async_upload_fences.reserve(result.submitted_fences.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        const auto& upload = result.uploads[index];
        if (!sources[index].upload_options.wait_for_completion && upload.submitted_fence.value != 0) {
            async_upload_fences.push_back(upload.submitted_fence);
        }
    }

    const auto queue_wait = wait_for_runtime_uploads_on_queue(device, rhi::QueueKind::graphics, async_upload_fences);
    result.upload_queue_waits_recorded += queue_wait.queue_waits_recorded;
    if (!queue_wait.succeeded()) {
        add_diagnostic(result, {}, "mesh-upload-queue-wait-failed", runtime_upload_queue_wait_diagnostic(queue_wait));
        return result;
    }

    result.mesh_bindings.reserve(sources.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        auto binding = make_runtime_mesh_gpu_binding(result.uploads[index]);
        if (binding.owner_device == nullptr) {
            add_diagnostic(result, sources[index].asset, "mesh-binding-empty",
                           "runtime mesh upload did not produce a renderer mesh binding");
            continue;
        }
        result.mesh_bindings.push_back(RuntimePackageStreamingMeshGpuBinding{
            .asset = sources[index].asset,
            .binding = binding,
        });
    }

    if (!result.succeeded()) {
        result.mesh_bindings.clear();
    }
    return result;
}

RuntimePackageStreamingSkinnedMeshUploadBindingResult upload_runtime_package_streaming_skinned_mesh_gpu_bindings(
    rhi::IRhiDevice& device, const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalog& resident_catalog,
    std::span<const RuntimePackageStreamingSkinnedMeshUploadSource> sources) {
    RuntimePackageStreamingSkinnedMeshUploadBindingResult result;

    if (!streaming_result.succeeded()) {
        add_diagnostic(result, {}, "package-streaming-not-committed",
                       "package streaming result must be committed before uploading skinned mesh GPU bindings");
        return result;
    }

    std::vector<AssetId> seen_assets;
    seen_assets.reserve(sources.size());
    for (const auto& source : sources) {
        if (contains_asset(seen_assets, source.asset)) {
            add_diagnostic(result, source.asset, "duplicate-skinned-mesh-asset",
                           "package streaming skinned mesh upload source assets must be unique");
        } else {
            seen_assets.push_back(source.asset);
        }

        const auto handle = runtime::find_runtime_resource(resident_catalog, source.asset);
        const runtime::RuntimeResourceRecord* record = nullptr;
        if (!handle.has_value()) {
            add_diagnostic(result, source.asset, "runtime-resource-not-live",
                           "skinned mesh asset is not live in the resident runtime resource catalog");
        } else {
            record = runtime::runtime_resource_record(resident_catalog, *handle);
            if (record == nullptr) {
                add_diagnostic(result, source.asset, "runtime-resource-not-live",
                               "skinned mesh asset catalog handle is stale");
            } else if (record->kind != AssetKind::skinned_mesh) {
                add_diagnostic(result, source.asset, "runtime-resource-not-skinned-mesh",
                               "resident runtime resource is not a skinned mesh asset");
            }
        }

        if (source.payload == nullptr) {
            add_diagnostic(result, source.asset, "skinned-mesh-payload-missing",
                           "runtime skinned mesh payload is required before uploading skinned mesh GPU bindings");
            continue;
        }
        if (source.payload->asset != source.asset) {
            add_diagnostic(result, source.asset, "skinned-mesh-payload-asset-mismatch",
                           "runtime skinned mesh payload asset must match the selected resident skinned mesh asset");
        }
        if (record != nullptr && source.payload->handle != record->package_handle) {
            add_diagnostic(result, source.asset, "skinned-mesh-payload-handle-mismatch",
                           "runtime skinned mesh payload handle must match the selected resident skinned mesh handle");
        }
    }

    if (!result.succeeded()) {
        return result;
    }

    result.uploads.reserve(sources.size());
    for (const auto& source : sources) {
        auto upload = upload_runtime_skinned_mesh(device, *source.payload, source.upload_options);
        if (!upload.succeeded()) {
            add_diagnostic(result, source.asset, "skinned-mesh-upload-failed", upload.diagnostic);
        }
        result.uploaded_bytes +=
            upload.uploaded_vertex_bytes + upload.uploaded_index_bytes + upload.uploaded_joint_palette_bytes;
        result.frame_graph_command_lists_submitted += upload.frame_graph_command_lists_submitted;
        result.frame_graph_queue_waits_recorded += upload.frame_graph_queue_waits_recorded;
        result.frame_graph_barriers_recorded += upload.frame_graph_barriers_recorded;
        result.frame_graph_pass_callbacks_invoked += upload.frame_graph_pass_callbacks_invoked;
        if (upload.submitted_fence.value != 0) {
            result.submitted_fences.push_back(upload.submitted_fence);
        }
        result.uploads.push_back(std::move(upload));
    }

    if (!result.succeeded()) {
        return result;
    }

    std::vector<rhi::FenceValue> async_upload_fences;
    async_upload_fences.reserve(result.submitted_fences.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        const auto& upload = result.uploads[index];
        if (!sources[index].upload_options.wait_for_completion && upload.submitted_fence.value != 0) {
            async_upload_fences.push_back(upload.submitted_fence);
        }
    }

    const auto queue_wait = wait_for_runtime_uploads_on_queue(device, rhi::QueueKind::graphics, async_upload_fences);
    result.upload_queue_waits_recorded += queue_wait.queue_waits_recorded;
    if (!queue_wait.succeeded()) {
        add_diagnostic(result, {}, "skinned-mesh-upload-queue-wait-failed",
                       runtime_upload_queue_wait_diagnostic(queue_wait));
        return result;
    }

    result.skinned_mesh_bindings.reserve(sources.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        auto binding = make_runtime_skinned_mesh_gpu_binding(result.uploads[index]);
        if (binding.owner_device == nullptr) {
            add_diagnostic(result, sources[index].asset, "skinned-mesh-binding-empty",
                           "runtime skinned mesh upload did not produce a renderer skinned mesh binding");
            continue;
        }
        result.skinned_mesh_bindings.push_back(RuntimePackageStreamingSkinnedMeshGpuBinding{
            .asset = sources[index].asset,
            .binding = binding,
        });
    }

    if (!result.succeeded()) {
        result.skinned_mesh_bindings.clear();
    }
    return result;
}

RuntimePackageStreamingMorphMeshUploadBindingResult upload_runtime_package_streaming_morph_mesh_gpu_bindings(
    rhi::IRhiDevice& device, const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalog& resident_catalog,
    std::span<const RuntimePackageStreamingMorphMeshUploadSource> sources) {
    RuntimePackageStreamingMorphMeshUploadBindingResult result;

    if (!streaming_result.succeeded()) {
        add_diagnostic(result, {}, "package-streaming-not-committed",
                       "package streaming result must be committed before uploading morph mesh GPU bindings");
        return result;
    }

    std::vector<AssetId> seen_assets;
    seen_assets.reserve(sources.size());
    for (const auto& source : sources) {
        if (contains_asset(seen_assets, source.asset)) {
            add_diagnostic(result, source.asset, "duplicate-morph-mesh-asset",
                           "package streaming morph mesh upload source assets must be unique");
        } else {
            seen_assets.push_back(source.asset);
        }

        const auto handle = runtime::find_runtime_resource(resident_catalog, source.asset);
        const runtime::RuntimeResourceRecord* record = nullptr;
        if (!handle.has_value()) {
            add_diagnostic(result, source.asset, "runtime-resource-not-live",
                           "morph mesh asset is not live in the resident runtime resource catalog");
        } else {
            record = runtime::runtime_resource_record(resident_catalog, *handle);
            if (record == nullptr) {
                add_diagnostic(result, source.asset, "runtime-resource-not-live",
                               "morph mesh asset catalog handle is stale");
            } else if (record->kind != AssetKind::morph_mesh_cpu) {
                add_diagnostic(result, source.asset, "runtime-resource-not-morph-mesh",
                               "resident runtime resource is not a morph mesh CPU asset");
            }
        }

        if (source.payload == nullptr) {
            add_diagnostic(result, source.asset, "morph-mesh-payload-missing",
                           "runtime morph mesh CPU payload is required before uploading morph mesh GPU bindings");
            continue;
        }
        if (source.payload->asset != source.asset) {
            add_diagnostic(result, source.asset, "morph-mesh-payload-asset-mismatch",
                           "runtime morph mesh CPU payload asset must match the selected resident morph mesh asset");
        }
        if (record != nullptr && source.payload->handle != record->package_handle) {
            add_diagnostic(result, source.asset, "morph-mesh-payload-handle-mismatch",
                           "runtime morph mesh CPU payload handle must match the selected resident morph mesh handle");
        }
    }

    if (!result.succeeded()) {
        return result;
    }

    result.uploads.reserve(sources.size());
    for (const auto& source : sources) {
        auto upload = upload_runtime_morph_mesh_cpu(device, *source.payload, source.upload_options);
        if (!upload.succeeded()) {
            add_diagnostic(result, source.asset, "morph-mesh-upload-failed", upload.diagnostic);
        }
        result.uploaded_bytes += upload.uploaded_position_delta_bytes + upload.uploaded_normal_delta_bytes +
                                 upload.uploaded_tangent_delta_bytes + upload.uploaded_weight_bytes;
        result.frame_graph_command_lists_submitted += upload.frame_graph_command_lists_submitted;
        result.frame_graph_queue_waits_recorded += upload.frame_graph_queue_waits_recorded;
        result.frame_graph_barriers_recorded += upload.frame_graph_barriers_recorded;
        result.frame_graph_pass_callbacks_invoked += upload.frame_graph_pass_callbacks_invoked;
        if (upload.submitted_fence.value != 0) {
            result.submitted_fences.push_back(upload.submitted_fence);
        }
        result.uploads.push_back(std::move(upload));
    }

    if (!result.succeeded()) {
        return result;
    }

    std::vector<rhi::FenceValue> async_upload_fences;
    async_upload_fences.reserve(result.submitted_fences.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        const auto& upload = result.uploads[index];
        if (!sources[index].upload_options.wait_for_completion && upload.submitted_fence.value != 0) {
            async_upload_fences.push_back(upload.submitted_fence);
        }
    }

    const auto queue_wait = wait_for_runtime_uploads_on_queue(device, rhi::QueueKind::graphics, async_upload_fences);
    result.upload_queue_waits_recorded += queue_wait.queue_waits_recorded;
    if (!queue_wait.succeeded()) {
        add_diagnostic(result, {}, "morph-mesh-upload-queue-wait-failed",
                       runtime_upload_queue_wait_diagnostic(queue_wait));
        return result;
    }

    result.morph_mesh_bindings.reserve(sources.size());
    for (std::size_t index = 0; index < sources.size(); ++index) {
        auto binding = make_runtime_morph_mesh_gpu_binding(result.uploads[index]);
        if (binding.owner_device == nullptr) {
            add_diagnostic(result, sources[index].asset, "morph-mesh-binding-empty",
                           "runtime morph mesh upload did not produce a renderer morph mesh binding");
            continue;
        }
        result.morph_mesh_bindings.push_back(RuntimePackageStreamingMorphMeshGpuBinding{
            .asset = sources[index].asset,
            .binding = binding,
        });
    }

    if (!result.succeeded()) {
        result.morph_mesh_bindings.clear();
    }
    return result;
}

namespace {

void reset_resource_update_counters(RuntimePackageResourceUpdateReadinessResult& result) {
    result.updates.clear();
    result.texture_updates = 0;
    result.mesh_updates = 0;
    result.skinned_mesh_updates = 0;
    result.morph_mesh_updates = 0;
    result.submitted_fences = 0;
    result.graphics_queue_ready_updates = 0;
    result.graphics_queue_waits_recorded = 0;
    result.same_queue_graphics_updates = 0;
    result.ready = false;
}

void record_resource_update(RuntimePackageResourceUpdateReadinessResult& result, RuntimePackageResourceUpdate update) {
    switch (update.kind) {
    case RuntimePackageResourceUpdateKind::texture:
        ++result.texture_updates;
        break;
    case RuntimePackageResourceUpdateKind::static_mesh:
        ++result.mesh_updates;
        break;
    case RuntimePackageResourceUpdateKind::skinned_mesh:
        ++result.skinned_mesh_updates;
        break;
    case RuntimePackageResourceUpdateKind::morph_mesh:
        ++result.morph_mesh_updates;
        break;
    }

    ++result.submitted_fences;
    if (update.graphics_queue_ready) {
        ++result.graphics_queue_ready_updates;
    }
    if (update.graphics_queue_wait_recorded) {
        ++result.graphics_queue_waits_recorded;
    }
    if (update.same_queue_graphics_order) {
        ++result.same_queue_graphics_updates;
    }
    result.updates.push_back(std::move(update));
}

void append_resource_update(RuntimePackageResourceUpdateReadinessResult& result,
                            const runtime::RuntimeResourceCatalog& resident_catalog, AssetId asset,
                            RuntimePackageResourceUpdateKind update_kind, AssetKind expected_kind, std::string resource,
                            rhi::FenceValue fence, std::size_t& graphics_waits_remaining) {
    const auto handle = runtime::find_runtime_resource(resident_catalog, asset);
    const runtime::RuntimeResourceRecord* record = nullptr;
    if (!handle.has_value()) {
        add_diagnostic(result, asset, update_kind, "runtime-resource-not-live",
                       "package resource update asset is not live in the resident runtime resource catalog");
        return;
    }

    record = runtime::runtime_resource_record(resident_catalog, *handle);
    if (record == nullptr) {
        add_diagnostic(result, asset, update_kind, "runtime-resource-not-live",
                       "package resource update catalog handle is stale");
        return;
    }
    if (record->kind != expected_kind) {
        add_diagnostic(result, asset, update_kind, "runtime-resource-kind-mismatch",
                       "package resource update resident catalog kind does not match the uploaded binding kind");
        return;
    }

    if (fence.value == 0) {
        add_diagnostic(result, asset, update_kind, "resource-update-upload-fence-missing",
                       "package resource update requires a submitted upload fence");
        return;
    }
    if (!valid_runtime_resource_update_queue_kind(fence.queue)) {
        add_diagnostic(result, asset, update_kind, "resource-update-upload-fence-invalid-queue",
                       "package resource update upload fence queue is invalid");
        return;
    }

    bool same_queue_graphics_order = false;
    bool graphics_queue_wait_recorded = false;
    if (fence.queue == rhi::QueueKind::graphics) {
        same_queue_graphics_order = true;
    } else if (graphics_waits_remaining > 0) {
        --graphics_waits_remaining;
        graphics_queue_wait_recorded = true;
    } else {
        add_diagnostic(result, asset, update_kind, "resource-update-upload-not-waited",
                       "package resource update requires graphics queue wait evidence for async non-graphics uploads");
        return;
    }

    record_resource_update(result, RuntimePackageResourceUpdate{
                                       .asset = asset,
                                       .kind = update_kind,
                                       .catalog_kind = record->kind,
                                       .resource_handle = record->handle,
                                       .package_handle = record->package_handle,
                                       .resource = std::move(resource),
                                       .submitted_upload_fence = fence,
                                       .graphics_queue_ready = true,
                                       .graphics_queue_wait_recorded = graphics_queue_wait_recorded,
                                       .same_queue_graphics_order = same_queue_graphics_order,
                                   });
}

} // namespace

RuntimePackageResourceUpdateReadinessResult
make_runtime_package_resource_update_readiness(const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
                                               const runtime::RuntimeResourceCatalog& resident_catalog,
                                               const RuntimePackageResourceUpdateReadinessSources& sources) {
    RuntimePackageResourceUpdateReadinessResult result;

    if (!streaming_result.succeeded()) {
        add_diagnostic(result, {}, RuntimePackageResourceUpdateKind::texture, "package-streaming-not-committed",
                       "package streaming result must be committed before publishing package resource updates");
        return result;
    }

    std::size_t source_groups = 0;
    if (sources.texture_uploads != nullptr) {
        ++source_groups;
        const auto& transaction = *sources.texture_uploads;
        if (!transaction.succeeded()) {
            const auto diagnostic = transaction.diagnostics.empty()
                                        ? RuntimePackageStreamingFrameGraphTextureBindingDiagnostic{}
                                        : transaction.diagnostics.front();
            add_diagnostic(result, diagnostic.asset, RuntimePackageResourceUpdateKind::texture,
                           "resource-update-transaction-failed",
                           diagnostic.message.empty() ? "texture package upload transaction failed"
                                                      : diagnostic.message);
        } else if (transaction.uploads.size() != transaction.texture_resource_bindings.size()) {
            add_diagnostic(result, {}, RuntimePackageResourceUpdateKind::texture,
                           "resource-update-binding-count-mismatch",
                           "texture package resource update rows must match uploaded texture rows");
        } else {
            auto graphics_waits_remaining = transaction.upload_queue_waits_recorded;
            for (std::size_t index = 0; index < transaction.texture_resource_bindings.size(); ++index) {
                append_resource_update(result, resident_catalog, transaction.texture_resource_bindings[index].asset,
                                       RuntimePackageResourceUpdateKind::texture, AssetKind::texture,
                                       transaction.texture_resource_bindings[index].resource,
                                       transaction.uploads[index].submitted_fence, graphics_waits_remaining);
            }
        }
    }

    if (sources.mesh_uploads != nullptr) {
        ++source_groups;
        const auto& transaction = *sources.mesh_uploads;
        if (!transaction.succeeded()) {
            const auto diagnostic = transaction.diagnostics.empty() ? RuntimePackageStreamingMeshUploadDiagnostic{}
                                                                    : transaction.diagnostics.front();
            add_diagnostic(result, diagnostic.asset, RuntimePackageResourceUpdateKind::static_mesh,
                           "resource-update-transaction-failed",
                           diagnostic.message.empty() ? "static mesh package upload transaction failed"
                                                      : diagnostic.message);
        } else if (transaction.uploads.size() != transaction.mesh_bindings.size()) {
            add_diagnostic(result, {}, RuntimePackageResourceUpdateKind::static_mesh,
                           "resource-update-binding-count-mismatch",
                           "static mesh package resource update rows must match uploaded mesh rows");
        } else {
            auto graphics_waits_remaining = transaction.upload_queue_waits_recorded;
            for (std::size_t index = 0; index < transaction.mesh_bindings.size(); ++index) {
                append_resource_update(result, resident_catalog, transaction.mesh_bindings[index].asset,
                                       RuntimePackageResourceUpdateKind::static_mesh, AssetKind::mesh, {},
                                       transaction.uploads[index].submitted_fence, graphics_waits_remaining);
            }
        }
    }

    if (sources.skinned_mesh_uploads != nullptr) {
        ++source_groups;
        const auto& transaction = *sources.skinned_mesh_uploads;
        if (!transaction.succeeded()) {
            const auto diagnostic = transaction.diagnostics.empty() ? RuntimePackageStreamingMeshUploadDiagnostic{}
                                                                    : transaction.diagnostics.front();
            add_diagnostic(result, diagnostic.asset, RuntimePackageResourceUpdateKind::skinned_mesh,
                           "resource-update-transaction-failed",
                           diagnostic.message.empty() ? "skinned mesh package upload transaction failed"
                                                      : diagnostic.message);
        } else if (transaction.uploads.size() != transaction.skinned_mesh_bindings.size()) {
            add_diagnostic(result, {}, RuntimePackageResourceUpdateKind::skinned_mesh,
                           "resource-update-binding-count-mismatch",
                           "skinned mesh package resource update rows must match uploaded skinned mesh rows");
        } else {
            auto graphics_waits_remaining = transaction.upload_queue_waits_recorded;
            for (std::size_t index = 0; index < transaction.skinned_mesh_bindings.size(); ++index) {
                append_resource_update(result, resident_catalog, transaction.skinned_mesh_bindings[index].asset,
                                       RuntimePackageResourceUpdateKind::skinned_mesh, AssetKind::skinned_mesh, {},
                                       transaction.uploads[index].submitted_fence, graphics_waits_remaining);
            }
        }
    }

    if (sources.morph_mesh_uploads != nullptr) {
        ++source_groups;
        const auto& transaction = *sources.morph_mesh_uploads;
        if (!transaction.succeeded()) {
            const auto diagnostic = transaction.diagnostics.empty() ? RuntimePackageStreamingMeshUploadDiagnostic{}
                                                                    : transaction.diagnostics.front();
            add_diagnostic(result, diagnostic.asset, RuntimePackageResourceUpdateKind::morph_mesh,
                           "resource-update-transaction-failed",
                           diagnostic.message.empty() ? "morph mesh package upload transaction failed"
                                                      : diagnostic.message);
        } else if (transaction.uploads.size() != transaction.morph_mesh_bindings.size()) {
            add_diagnostic(result, {}, RuntimePackageResourceUpdateKind::morph_mesh,
                           "resource-update-binding-count-mismatch",
                           "morph mesh package resource update rows must match uploaded morph mesh rows");
        } else {
            auto graphics_waits_remaining = transaction.upload_queue_waits_recorded;
            for (std::size_t index = 0; index < transaction.morph_mesh_bindings.size(); ++index) {
                append_resource_update(result, resident_catalog, transaction.morph_mesh_bindings[index].asset,
                                       RuntimePackageResourceUpdateKind::morph_mesh, AssetKind::morph_mesh_cpu, {},
                                       transaction.uploads[index].submitted_fence, graphics_waits_remaining);
            }
        }
    }

    if (source_groups == 0) {
        add_diagnostic(result, {}, RuntimePackageResourceUpdateKind::texture, "resource-update-source-missing",
                       "package resource update readiness requires at least one upload transaction source");
    }

    if (!result.succeeded()) {
        reset_resource_update_counters(result);
        return result;
    }

    result.ready = !result.updates.empty() && result.submitted_fences == result.updates.size() &&
                   result.graphics_queue_ready_updates == result.updates.size();
    if (!result.ready) {
        add_diagnostic(result, {}, RuntimePackageResourceUpdateKind::texture, "resource-update-counters-mismatch",
                       "package resource update readiness counters did not meet expected values");
        reset_resource_update_counters(result);
    }
    return result;
}

RuntimePackageUploadStagingEvidence execute_runtime_package_upload_staging_evidence(rhi::IRhiDevice& device) {
    RuntimePackageUploadStagingEvidence result;
    const auto stats_before = device.stats();

    constexpr std::uint64_t upload_ring_bytes = 16U * 1024U;
    constexpr std::uint32_t upload_ring_count = 4;
    rhi::RhiStagingBufferPool pool(device, rhi::RhiStagingBufferPoolDesc{.buffer_count = upload_ring_count,
                                                                         .chunk_size_bytes = upload_ring_bytes});
    std::vector<rhi::RhiUploadRing> upload_rings;
    upload_rings.reserve(upload_ring_count);
    for (std::uint32_t ring_index = 0; ring_index < upload_ring_count; ++ring_index) {
        const auto lease = pool.try_acquire_lease();
        if (!lease.has_value()) {
            add_diagnostic(result, "staging-pool-lease-unavailable",
                           "runtime package upload staging evidence requires four staging pool leases");
            return result;
        }
        upload_rings.emplace_back(
            device,
            rhi::RhiUploadRingDesc{.size_bytes = lease->size_bytes, .min_alignment = 256, .buffer = lease->buffer});
        ++result.staging_pool_leases;
    }

    const auto texture_asset = AssetId::from_name("package/upload_staging/texture");
    const auto mesh_asset = AssetId::from_name("package/upload_staging/static_mesh");
    const auto skinned_mesh_asset = AssetId::from_name("package/upload_staging/skinned_mesh");
    const auto morph_mesh_asset = AssetId::from_name("package/upload_staging/morph_mesh");
    const auto texture_handle = runtime::RuntimeAssetHandle{.value = 1};
    const auto mesh_handle = runtime::RuntimeAssetHandle{.value = 2};
    const auto skinned_mesh_handle = runtime::RuntimeAssetHandle{.value = 3};
    const auto morph_mesh_handle = runtime::RuntimeAssetHandle{.value = 4};

    std::vector<runtime::RuntimeAssetRecord> records;
    records.push_back(
        make_upload_staging_evidence_record(texture_asset, texture_handle, AssetKind::texture, "texture"));
    records.push_back(make_upload_staging_evidence_record(mesh_asset, mesh_handle, AssetKind::mesh, "mesh"));
    records.push_back(make_upload_staging_evidence_record(skinned_mesh_asset, skinned_mesh_handle,
                                                          AssetKind::skinned_mesh, "skinned_mesh"));
    records.push_back(make_upload_staging_evidence_record(morph_mesh_asset, morph_mesh_handle,
                                                          AssetKind::morph_mesh_cpu, "morph_mesh_cpu"));

    runtime::RuntimeResourceCatalog catalog;
    const auto catalog_build =
        runtime::build_runtime_resource_catalog(catalog, runtime::RuntimeAssetPackage{std::move(records)});
    if (!catalog_build.succeeded()) {
        add_diagnostic(result, "catalog-build-failed",
                       catalog_build.diagnostics.empty() ? "runtime package upload staging catalog build failed"
                                                         : catalog_build.diagnostics.front().diagnostic);
        return result;
    }

    const auto streaming = make_upload_staging_streaming_result();
    const auto texture_payload = make_upload_staging_texture_payload(texture_asset, texture_handle);
    const auto mesh_payload = make_upload_staging_mesh_payload(mesh_asset, mesh_handle);
    const auto skinned_mesh_payload = make_upload_staging_skinned_mesh_payload(skinned_mesh_asset, skinned_mesh_handle);
    const auto morph_mesh_payload = make_upload_staging_morph_mesh_payload(morph_mesh_asset, morph_mesh_handle);

    RuntimeTextureUploadOptions texture_options;
    texture_options.queue = rhi::QueueKind::graphics;
    texture_options.upload_ring = upload_rings.data();
    texture_options.wait_for_completion = false;
    RuntimeMeshUploadOptions mesh_options;
    mesh_options.queue = rhi::QueueKind::copy;
    mesh_options.upload_ring = upload_rings.data() + 1;
    mesh_options.wait_for_completion = false;
    RuntimeSkinnedMeshUploadOptions skinned_mesh_options;
    skinned_mesh_options.queue = rhi::QueueKind::copy;
    skinned_mesh_options.upload_ring = upload_rings.data() + 2;
    skinned_mesh_options.wait_for_completion = false;
    RuntimeMorphMeshUploadOptions morph_mesh_options;
    morph_mesh_options.queue = rhi::QueueKind::copy;
    morph_mesh_options.upload_ring = upload_rings.data() + 3;
    morph_mesh_options.wait_for_completion = false;

    const auto texture_upload = upload_runtime_package_streaming_frame_graph_texture_bindings(
        device, streaming, catalog,
        std::vector<RuntimePackageStreamingFrameGraphTextureUploadSource>{
            RuntimePackageStreamingFrameGraphTextureUploadSource{
                .asset = texture_asset,
                .resource = "package.upload_staging.texture",
                .payload = &texture_payload,
                .upload_options = texture_options,
                .current_state = rhi::ResourceState::shader_read,
            },
        });
    if (!texture_upload.succeeded()) {
        const auto& diagnostic = texture_upload.diagnostics.front();
        add_diagnostic(result, diagnostic.code, diagnostic.message);
        return result;
    }
    ++result.package_transactions;
    result.texture_uploads = texture_upload.uploads.size();
    result.texture_bindings = texture_upload.texture_bindings.size();
    result.uploaded_bytes += texture_upload.uploaded_bytes;
    for (const auto& upload : texture_upload.uploads) {
        if (upload.upload_buffer_caller_owned) {
            ++result.ring_backed_uploads;
        }
        if (upload.submitted_fence.value != 0) {
            ++result.submitted_fences;
        }
    }

    const auto mesh_upload = upload_runtime_package_streaming_mesh_gpu_bindings(
        device, streaming, catalog,
        std::vector<RuntimePackageStreamingMeshUploadSource>{RuntimePackageStreamingMeshUploadSource{
            .asset = mesh_asset,
            .payload = &mesh_payload,
            .upload_options = mesh_options,
        }});
    if (!mesh_upload.succeeded()) {
        const auto& diagnostic = mesh_upload.diagnostics.front();
        add_diagnostic(result, diagnostic.code, diagnostic.message);
        return result;
    }
    ++result.package_transactions;
    result.mesh_uploads = mesh_upload.uploads.size();
    result.mesh_bindings = mesh_upload.mesh_bindings.size();
    result.uploaded_bytes += mesh_upload.uploaded_bytes;
    result.submitted_fences += mesh_upload.submitted_fences.size();
    result.upload_queue_waits_recorded += mesh_upload.upload_queue_waits_recorded;
    for (const auto& upload : mesh_upload.uploads) {
        if (upload.upload_buffers_caller_owned) {
            ++result.ring_backed_uploads;
        }
    }

    const auto skinned_mesh_upload = upload_runtime_package_streaming_skinned_mesh_gpu_bindings(
        device, streaming, catalog,
        std::vector<RuntimePackageStreamingSkinnedMeshUploadSource>{
            RuntimePackageStreamingSkinnedMeshUploadSource{
                .asset = skinned_mesh_asset,
                .payload = &skinned_mesh_payload,
                .upload_options = skinned_mesh_options,
            },
        });
    if (!skinned_mesh_upload.succeeded()) {
        const auto& diagnostic = skinned_mesh_upload.diagnostics.front();
        add_diagnostic(result, diagnostic.code, diagnostic.message);
        return result;
    }
    ++result.package_transactions;
    result.skinned_mesh_uploads = skinned_mesh_upload.uploads.size();
    result.skinned_mesh_bindings = skinned_mesh_upload.skinned_mesh_bindings.size();
    result.uploaded_bytes += skinned_mesh_upload.uploaded_bytes;
    result.submitted_fences += skinned_mesh_upload.submitted_fences.size();
    result.upload_queue_waits_recorded += skinned_mesh_upload.upload_queue_waits_recorded;
    for (const auto& upload : skinned_mesh_upload.uploads) {
        if (upload.upload_buffers_caller_owned) {
            ++result.ring_backed_uploads;
        }
    }

    const auto morph_mesh_upload = upload_runtime_package_streaming_morph_mesh_gpu_bindings(
        device, streaming, catalog,
        std::vector<RuntimePackageStreamingMorphMeshUploadSource>{RuntimePackageStreamingMorphMeshUploadSource{
            .asset = morph_mesh_asset,
            .payload = &morph_mesh_payload,
            .upload_options = morph_mesh_options,
        }});
    if (!morph_mesh_upload.succeeded()) {
        const auto& diagnostic = morph_mesh_upload.diagnostics.front();
        add_diagnostic(result, diagnostic.code, diagnostic.message);
        return result;
    }
    ++result.package_transactions;
    result.morph_mesh_uploads = morph_mesh_upload.uploads.size();
    result.morph_mesh_bindings = morph_mesh_upload.morph_mesh_bindings.size();
    result.uploaded_bytes += morph_mesh_upload.uploaded_bytes;
    result.submitted_fences += morph_mesh_upload.submitted_fences.size();
    result.upload_queue_waits_recorded += morph_mesh_upload.upload_queue_waits_recorded;
    for (const auto& upload : morph_mesh_upload.uploads) {
        if (upload.upload_buffers_caller_owned) {
            ++result.ring_backed_uploads;
        }
    }

    const auto resource_updates =
        make_runtime_package_resource_update_readiness(streaming, catalog,
                                                       RuntimePackageResourceUpdateReadinessSources{
                                                           .texture_uploads = &texture_upload,
                                                           .mesh_uploads = &mesh_upload,
                                                           .skinned_mesh_uploads = &skinned_mesh_upload,
                                                           .morph_mesh_uploads = &morph_mesh_upload,
                                                       });
    if (!resource_updates.succeeded()) {
        const auto& diagnostic = resource_updates.diagnostics.front();
        add_diagnostic(result, diagnostic.code, diagnostic.message);
        return result;
    }
    result.resource_updates_ready = resource_updates.ready;
    result.resource_updates = resource_updates.updates.size();
    result.resource_update_submitted_fences = resource_updates.submitted_fences;
    result.resource_update_graphics_ready_updates = resource_updates.graphics_queue_ready_updates;
    result.resource_update_graphics_queue_waits_recorded = resource_updates.graphics_queue_waits_recorded;
    result.resource_update_same_queue_graphics_updates = resource_updates.same_queue_graphics_updates;

    const auto stats_after = device.stats();
    result.copy_queue_submits = stats_after.copy_queue_submits - stats_before.copy_queue_submits;
    result.graphics_queue_submits = stats_after.graphics_queue_submits - stats_before.graphics_queue_submits;
    result.queue_waits = stats_after.queue_waits - stats_before.queue_waits;
    result.fence_waits = stats_after.fence_waits - stats_before.fence_waits;
    result.graphics_waited_for_copy = result.queue_waits > 0 && stats_after.last_graphics_queue_wait_fence_value > 0 &&
                                      stats_after.last_graphics_queue_wait_fence_queue == rhi::QueueKind::copy;

    result.ready = result.package_transactions == 4 && result.texture_uploads == 1 && result.mesh_uploads == 1 &&
                   result.skinned_mesh_uploads == 1 && result.morph_mesh_uploads == 1 && result.texture_bindings == 1 &&
                   result.mesh_bindings == 1 && result.skinned_mesh_bindings == 1 && result.morph_mesh_bindings == 1 &&
                   result.staging_pool_leases == upload_ring_count && result.ring_backed_uploads == 4 &&
                   result.uploaded_bytes > 0 && result.submitted_fences == 4 &&
                   result.upload_queue_waits_recorded == 3 && result.copy_queue_submits >= 3 &&
                   result.graphics_queue_submits >= 1 && result.queue_waits == 3 && result.fence_waits == 0 &&
                   result.graphics_waited_for_copy && result.resource_updates_ready && result.resource_updates == 4 &&
                   result.resource_update_submitted_fences == 4 && result.resource_update_graphics_ready_updates == 4 &&
                   result.resource_update_graphics_queue_waits_recorded == 3 &&
                   result.resource_update_same_queue_graphics_updates == 1;
    if (!result.ready) {
        add_diagnostic(result, "package-upload-staging-counters-mismatch",
                       "runtime package upload staging evidence did not meet expected counters");
    }
    return result;
}

} // namespace mirakana::runtime_rhi
