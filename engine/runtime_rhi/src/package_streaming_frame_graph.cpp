// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/package_streaming_frame_graph.hpp"

#include <algorithm>
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

[[nodiscard]] bool contains_resource(const std::vector<std::string>& resources, const std::string& resource) {
    return std::ranges::find(resources, resource) != resources.end();
}

} // namespace

RuntimePackageStreamingFrameGraphTextureBindingResult make_runtime_package_streaming_frame_graph_texture_bindings(
    const runtime::RuntimePackageStreamingExecutionResult& streaming_result,
    const runtime::RuntimeResourceCatalogV2& resident_catalog,
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

        const auto handle = runtime::find_runtime_resource_v2(resident_catalog, source.asset);
        if (!handle.has_value()) {
            add_diagnostic(result, source.asset, source.resource, "runtime-resource-not-live",
                           "texture asset is not live in the resident runtime resource catalog");
        } else {
            const auto* const record = runtime::runtime_resource_record_v2(resident_catalog, *handle);
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

} // namespace mirakana::runtime_rhi
