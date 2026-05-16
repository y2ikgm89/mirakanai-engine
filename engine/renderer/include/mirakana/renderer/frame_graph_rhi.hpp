// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
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

struct FrameGraphTransientTextureDesc {
    std::string resource;
    rhi::TextureDesc desc;
};

struct FrameGraphTransientTextureLifetime {
    std::string resource;
    rhi::TextureDesc desc;
    std::size_t first_pass_index{0};
    std::size_t last_pass_index{0};
    std::size_t alias_group{0};
    std::uint64_t estimated_bytes{0};
};

struct FrameGraphTransientTextureAliasGroup {
    std::size_t index{0};
    rhi::TextureDesc desc;
    std::uint64_t estimated_bytes{0};
    std::vector<std::string> resources;
};

struct FrameGraphTransientTextureAliasPlan {
    std::vector<FrameGraphTransientTextureLifetime> lifetimes;
    std::vector<FrameGraphTransientTextureAliasGroup> alias_groups;
    std::uint64_t estimated_unaliased_bytes{0};
    std::uint64_t estimated_aliased_bytes{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct FrameGraphTextureBarrierRecordResult {
    std::size_t barriers_recorded{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct FrameGraphTextureFinalState {
    std::string resource;
    rhi::ResourceState state{rhi::ResourceState::undefined};
};

struct FrameGraphRhiTextureExecutionDesc {
    rhi::IRhiCommandList* commands{nullptr};
    std::span<const FrameGraphExecutionStep> schedule;
    std::span<FrameGraphTextureBinding> texture_bindings;
    std::span<const FrameGraphPassExecutionBinding> pass_callbacks;
    std::span<const FrameGraphTextureFinalState> final_states;
};

struct FrameGraphRhiTextureExecutionResult {
    std::size_t barriers_recorded{0};
    std::size_t final_state_barriers_recorded{0};
    std::size_t pass_callbacks_invoked{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] std::optional<rhi::ResourceState> frame_graph_texture_state_for_access(FrameGraphAccess access) noexcept;

[[nodiscard]] FrameGraphTransientTextureAliasPlan
plan_frame_graph_transient_texture_aliases(const FrameGraphV1Desc& desc,
                                           std::span<const FrameGraphTransientTextureDesc> texture_descs);

[[nodiscard]] FrameGraphTextureBarrierRecordResult
record_frame_graph_texture_barriers(rhi::IRhiCommandList& commands, std::span<const FrameGraphExecutionStep> schedule,
                                    std::span<FrameGraphTextureBinding> texture_bindings);

[[nodiscard]] FrameGraphRhiTextureExecutionResult
execute_frame_graph_rhi_texture_schedule(const FrameGraphRhiTextureExecutionDesc& desc);

} // namespace mirakana
