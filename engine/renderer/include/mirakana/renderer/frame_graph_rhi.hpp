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

struct FrameGraphTransientTextureLease {
    std::size_t alias_group{0};
    rhi::TransientTextureAliasGroup transient_alias_group;
};

struct FrameGraphTransientTextureLeaseBindingResult {
    std::vector<FrameGraphTransientTextureLease> leases;
    std::vector<FrameGraphTextureBinding> texture_bindings;
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

struct FrameGraphTextureAliasingBarrier {
    std::string before_resource;
    std::string after_resource;
};

struct FrameGraphTextureAliasingBarrierRecordResult {
    std::size_t aliasing_barriers_recorded{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct FrameGraphTextureFinalState {
    std::string resource;
    rhi::ResourceState state{rhi::ResourceState::undefined};
};

struct FrameGraphTexturePassTargetState {
    std::string pass_name;
    std::string resource;
    rhi::ResourceState state{rhi::ResourceState::undefined};
};

struct FrameGraphTexturePassTargetAccess {
    std::string pass_name;
    std::string resource;
    FrameGraphAccess access{FrameGraphAccess::unknown};
};

struct FrameGraphRhiRenderPassColorAttachment {
    std::string resource;
    rhi::TextureHandle texture;
    rhi::SwapchainFrameHandle swapchain_frame;
    rhi::LoadAction load_action{rhi::LoadAction::clear};
    rhi::StoreAction store_action{rhi::StoreAction::store};
    rhi::ClearColorValue clear_color;
};

struct FrameGraphRhiRenderPassDepthAttachment {
    std::string resource;
    rhi::TextureHandle texture;
    rhi::LoadAction load_action{rhi::LoadAction::clear};
    rhi::StoreAction store_action{rhi::StoreAction::store};
    rhi::ClearDepthValue clear_depth;
};

struct FrameGraphRhiRenderPassDesc {
    std::string pass_name;
    FrameGraphRhiRenderPassColorAttachment color;
    FrameGraphRhiRenderPassDepthAttachment depth;
};

struct FrameGraphRhiTextureExecutionDesc {
    rhi::IRhiCommandList* commands{nullptr};
    std::span<const FrameGraphExecutionStep> schedule;
    std::span<FrameGraphTextureBinding> texture_bindings;
    std::span<const FrameGraphPassExecutionBinding> pass_callbacks;
    std::span<const FrameGraphTexturePassTargetAccess> pass_target_accesses;
    std::span<const FrameGraphTexturePassTargetState> pass_target_states;
    std::span<const FrameGraphRhiRenderPassDesc> render_passes;
    std::span<const FrameGraphTextureFinalState> final_states;
    std::span<const FrameGraphTransientTextureLifetime> transient_texture_lifetimes;
};

struct FrameGraphRhiTextureExecutionResult {
    std::size_t barriers_recorded{0};
    std::size_t aliasing_barriers_recorded{0};
    std::size_t pass_target_state_barriers_recorded{0};
    std::size_t render_passes_recorded{0};
    std::size_t final_state_barriers_recorded{0};
    std::size_t pass_callbacks_invoked{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] std::optional<rhi::ResourceState> frame_graph_texture_state_for_access(FrameGraphAccess access) noexcept;

[[nodiscard]] std::vector<FrameGraphTexturePassTargetAccess>
build_frame_graph_texture_pass_target_accesses(const FrameGraphV1Desc& desc);

[[nodiscard]] FrameGraphTransientTextureAliasPlan
plan_frame_graph_transient_texture_aliases(const FrameGraphV1Desc& desc,
                                           std::span<const FrameGraphTransientTextureDesc> texture_descs);

[[nodiscard]] FrameGraphTransientTextureLeaseBindingResult
acquire_frame_graph_transient_texture_lease_bindings(rhi::IRhiDevice& device,
                                                     const FrameGraphTransientTextureAliasPlan& plan);

void release_frame_graph_transient_texture_lease_bindings(rhi::IRhiDevice& device,
                                                          std::span<const FrameGraphTransientTextureLease> leases);

[[nodiscard]] FrameGraphTextureBarrierRecordResult
record_frame_graph_texture_barriers(rhi::IRhiCommandList& commands, std::span<const FrameGraphExecutionStep> schedule,
                                    std::span<FrameGraphTextureBinding> texture_bindings);

[[nodiscard]] FrameGraphTextureAliasingBarrierRecordResult
record_frame_graph_texture_aliasing_barriers(rhi::IRhiCommandList& commands,
                                             std::span<const FrameGraphTextureAliasingBarrier> aliasing_barriers,
                                             std::span<const FrameGraphTextureBinding> texture_bindings);

[[nodiscard]] FrameGraphRhiTextureExecutionResult
execute_frame_graph_rhi_texture_schedule(const FrameGraphRhiTextureExecutionDesc& desc);

} // namespace mirakana
