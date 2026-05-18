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

struct RuntimePackageStreamingFrameGraphTextureBindingDiagnostic {
    AssetId asset;
    std::string resource;
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
    std::vector<RuntimePackageStreamingFrameGraphTextureBindingDiagnostic> diagnostics;
    std::uint64_t uploaded_bytes{0};
    std::size_t frame_graph_barriers_recorded{0};
    std::size_t frame_graph_pass_target_state_barriers_recorded{0};
    std::size_t frame_graph_final_state_barriers_recorded{0};
    std::size_t frame_graph_pass_callbacks_invoked{0};

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

} // namespace mirakana::runtime_rhi
