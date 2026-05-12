// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

struct FrameGraphTextureBinding {
    std::string resource;
    rhi::TextureHandle texture;
    rhi::ResourceState current_state{rhi::ResourceState::undefined};
};

struct FrameGraphTextureBarrierRecordResult {
    std::size_t barriers_recorded{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct FrameGraphRhiTextureExecutionDesc {
    rhi::IRhiCommandList* commands{nullptr};
    std::span<const FrameGraphExecutionStep> schedule;
    std::span<FrameGraphTextureBinding> texture_bindings;
    std::span<const FrameGraphPassExecutionBinding> pass_callbacks;
};

struct FrameGraphRhiTextureExecutionResult {
    std::size_t barriers_recorded{0};
    std::size_t pass_callbacks_invoked{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] std::optional<rhi::ResourceState> frame_graph_texture_state_for_access(FrameGraphAccess access) noexcept;

[[nodiscard]] FrameGraphTextureBarrierRecordResult
record_frame_graph_texture_barriers(rhi::IRhiCommandList& commands, std::span<const FrameGraphExecutionStep> schedule,
                                    std::span<FrameGraphTextureBinding> texture_bindings);

[[nodiscard]] FrameGraphRhiTextureExecutionResult
execute_frame_graph_rhi_texture_schedule(const FrameGraphRhiTextureExecutionDesc& desc);

} // namespace mirakana
