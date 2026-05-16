// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/frame_graph_rhi.hpp"

#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

struct PlannedTextureBarrier {
    std::size_t binding_index{0};
    rhi::ResourceState before{rhi::ResourceState::undefined};
    rhi::ResourceState after{rhi::ResourceState::undefined};
    const FrameGraphBarrier* barrier{nullptr};
};

struct PlannedTextureFinalState {
    std::size_t binding_index{0};
    rhi::ResourceState after{rhi::ResourceState::undefined};
    const FrameGraphTextureFinalState* final_state{nullptr};
};

struct PlannedTexturePassTargetState {
    std::size_t binding_index{0};
    rhi::ResourceState after{rhi::ResourceState::undefined};
    const FrameGraphTexturePassTargetState* pass_target_state{nullptr};
};

using TexturePassTargetAccessIndex = std::map<std::pair<std::string, std::string>, FrameGraphAccess>;

struct TransientTextureUse {
    std::string pass;
    std::size_t pass_index{0};
    FrameGraphAccess access{FrameGraphAccess::unknown};
};

struct TransientTextureResourceUse {
    std::string resource;
    std::vector<TransientTextureUse> uses;
};

struct TransientTextureAliasCandidate {
    std::string resource;
    rhi::TextureDesc desc;
    std::size_t first_pass_index{0};
    std::size_t last_pass_index{0};
    std::uint64_t estimated_bytes{0};
};

struct TransientTextureAliasGroupAccumulator {
    FrameGraphTransientTextureAliasGroup group;
    std::size_t last_pass_index{0};
};

template <typename Result>
void append_frame_graph_rhi_diagnostic(Result& result, FrameGraphDiagnosticCode code, std::string pass,
                                       std::string resource, std::string message) {
    result.diagnostics.push_back(FrameGraphDiagnostic{
        .code = code,
        .pass = std::move(pass),
        .resource = std::move(resource),
        .message = std::move(message),
    });
}

using FrameGraphRhiPassCallbackMap =
    std::map<std::string, std::function<FrameGraphExecutionCallbackResult(std::string_view pass_name)>>;

template <typename Result>
[[nodiscard]] std::map<std::string, std::size_t>
build_binding_index(Result& result, std::span<const FrameGraphTextureBinding> texture_bindings) {
    std::map<std::string, std::size_t> binding_indices;
    std::map<std::uint64_t, std::pair<rhi::ResourceState, std::string>> shared_texture_states;
    for (std::size_t index = 0; index < texture_bindings.size(); ++index) {
        const auto& binding = texture_bindings[index];
        if (binding.resource.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {},
                                              "frame graph texture binding resource name is empty");
            continue;
        }
        if (binding.texture.value == 0) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, binding.resource,
                                              "frame graph texture binding handle is empty");
            continue;
        }
        const auto [shared_state, shared_state_inserted] =
            shared_texture_states.emplace(binding.texture.value, std::pair{binding.current_state, binding.resource});
        if (!shared_state_inserted && shared_state->second.first != binding.current_state) {
            append_frame_graph_rhi_diagnostic(
                result, FrameGraphDiagnosticCode::invalid_resource, {}, binding.resource,
                "frame graph texture bindings sharing a handle disagree on current state");
        }
        const auto [_, inserted] = binding_indices.emplace(binding.resource, index);
        if (!inserted) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, binding.resource,
                                              "frame graph texture binding is declared more than once");
        }
    }
    return binding_indices;
}

[[nodiscard]] FrameGraphRhiPassCallbackMap
build_pass_callback_index(FrameGraphRhiTextureExecutionResult& result,
                          std::span<const FrameGraphPassExecutionBinding> pass_callbacks) {
    FrameGraphRhiPassCallbackMap callbacks;
    for (const auto& binding : pass_callbacks) {
        if (binding.pass_name.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                              "frame graph pass callback name is empty");
            continue;
        }
        if (!binding.callback) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, binding.pass_name, {},
                                              "frame graph pass callback is empty");
            continue;
        }
        const auto [_, inserted] = callbacks.emplace(binding.pass_name, binding.callback);
        if (!inserted) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, binding.pass_name, {},
                                              "frame graph pass callback is declared more than once");
        }
    }
    return callbacks;
}

[[nodiscard]] std::map<std::string, std::size_t>
build_scheduled_pass_index(std::span<const FrameGraphExecutionStep> schedule) {
    std::map<std::string, std::size_t> scheduled_passes;
    for (std::size_t index = 0; index < schedule.size(); ++index) {
        const auto& step = schedule[index];
        if (step.kind == FrameGraphExecutionStep::Kind::pass_invoke && !step.pass_name.empty()) {
            scheduled_passes.emplace(step.pass_name, index);
        }
    }
    return scheduled_passes;
}

[[nodiscard]] std::map<std::string, std::vector<PlannedTexturePassTargetState>>
plan_pass_target_states(FrameGraphRhiTextureExecutionResult& result,
                        const std::map<std::string, std::size_t>& binding_indices,
                        const std::map<std::string, std::size_t>& scheduled_passes,
                        const TexturePassTargetAccessIndex& pass_target_accesses,
                        std::span<const FrameGraphTexturePassTargetState> pass_target_states) {
    std::map<std::pair<std::string, std::string>, std::size_t> pass_resource_indices;
    std::map<std::string, std::vector<PlannedTexturePassTargetState>> planned_pass_target_states;

    for (std::size_t index = 0; index < pass_target_states.size(); ++index) {
        const auto& pass_target_state = pass_target_states[index];
        if (pass_target_state.pass_name.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                              "frame graph texture pass target state pass name is empty");
            continue;
        }
        if (pass_target_state.resource.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_state.pass_name, {},
                                              "frame graph texture pass target state resource name is empty");
            continue;
        }

        const auto [_, inserted] =
            pass_resource_indices.emplace(std::pair{pass_target_state.pass_name, pass_target_state.resource}, index);
        if (!inserted) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_state.pass_name, pass_target_state.resource,
                                              "frame graph texture pass target state is declared more than once");
            continue;
        }

        if (!scheduled_passes.contains(pass_target_state.pass_name)) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass,
                                              pass_target_state.pass_name, pass_target_state.resource,
                                              "frame graph texture pass target state targets an unscheduled pass");
            continue;
        }

        const auto binding = binding_indices.find(pass_target_state.resource);
        if (binding == binding_indices.end()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_state.pass_name, pass_target_state.resource,
                                              "frame graph texture pass target state has no texture binding");
            continue;
        }

        if (pass_target_state.state == rhi::ResourceState::undefined) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_state.pass_name, pass_target_state.resource,
                                              "frame graph texture pass target state cannot be undefined");
            continue;
        }

        const auto access =
            pass_target_accesses.find(std::pair{pass_target_state.pass_name, pass_target_state.resource});
        if (access == pass_target_accesses.end()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_state.pass_name, pass_target_state.resource,
                                              "frame graph texture pass target state has no declared writer access");
            continue;
        }

        const auto expected_state = frame_graph_texture_state_for_access(access->second);
        if (!expected_state.has_value() || *expected_state != pass_target_state.state) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_state.pass_name, pass_target_state.resource,
                                              "frame graph texture pass target state disagrees with writer access");
            continue;
        }

        planned_pass_target_states[pass_target_state.pass_name].push_back(PlannedTexturePassTargetState{
            .binding_index = binding->second,
            .after = pass_target_state.state,
            .pass_target_state = &pass_target_state,
        });
    }

    return planned_pass_target_states;
}

[[nodiscard]] TexturePassTargetAccessIndex
build_pass_target_access_index(FrameGraphRhiTextureExecutionResult& result,
                               std::span<const FrameGraphTexturePassTargetAccess> pass_target_accesses) {
    TexturePassTargetAccessIndex pass_target_access_index;
    for (const auto& pass_target_access : pass_target_accesses) {
        if (pass_target_access.pass_name.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                              "frame graph texture pass target access pass name is empty");
            continue;
        }
        if (pass_target_access.resource.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_access.pass_name, {},
                                              "frame graph texture pass target access resource name is empty");
            continue;
        }
        if (!frame_graph_texture_state_for_access(pass_target_access.access).has_value()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_access.pass_name, pass_target_access.resource,
                                              "frame graph texture pass target access cannot be unknown");
            continue;
        }

        const auto [_, inserted] = pass_target_access_index.emplace(
            std::pair{pass_target_access.pass_name, pass_target_access.resource}, pass_target_access.access);
        if (!inserted) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                              pass_target_access.pass_name, pass_target_access.resource,
                                              "frame graph texture pass target access is declared more than once");
        }
    }
    return pass_target_access_index;
}

[[nodiscard]] bool texture_descs_match(const rhi::TextureDesc& left, const rhi::TextureDesc& right) noexcept {
    return left.extent.width == right.extent.width && left.extent.height == right.extent.height &&
           left.extent.depth == right.extent.depth && left.format == right.format && left.usage == right.usage;
}

[[nodiscard]] std::uint32_t texture_format_bytes_per_pixel(rhi::Format format) noexcept {
    switch (format) {
    case rhi::Format::rgba8_unorm:
    case rhi::Format::bgra8_unorm:
    case rhi::Format::depth24_stencil8:
        return 4U;
    case rhi::Format::unknown:
        break;
    }
    return 0U;
}

[[nodiscard]] std::optional<std::uint64_t> checked_mul_u64(std::uint64_t left, std::uint64_t right) noexcept {
    if (left != 0U && right > std::numeric_limits<std::uint64_t>::max() / left) {
        return std::nullopt;
    }
    return left * right;
}

[[nodiscard]] std::optional<std::uint64_t> checked_add_u64(std::uint64_t left, std::uint64_t right) noexcept {
    if (right > std::numeric_limits<std::uint64_t>::max() - left) {
        return std::nullopt;
    }
    return left + right;
}

[[nodiscard]] std::optional<std::uint64_t> estimate_texture_size_bytes(const rhi::TextureDesc& desc) noexcept {
    const auto bytes_per_pixel = texture_format_bytes_per_pixel(desc.format);
    const auto row_bytes = checked_mul_u64(desc.extent.width, bytes_per_pixel);
    if (!row_bytes.has_value()) {
        return std::nullopt;
    }
    const auto slice_bytes = checked_mul_u64(*row_bytes, desc.extent.height);
    if (!slice_bytes.has_value()) {
        return std::nullopt;
    }
    return checked_mul_u64(*slice_bytes, desc.extent.depth);
}

template <typename Result>
void validate_transient_texture_desc_shape(Result& result, const std::string& resource, const rhi::TextureDesc& desc) {
    if (desc.extent.width == 0U || desc.extent.height == 0U || desc.extent.depth == 0U) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture extent must be non-zero");
    }
    if (desc.format == rhi::Format::unknown) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture format cannot be unknown");
    }
    if (desc.usage == rhi::TextureUsage::none) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture usage cannot be none");
    }
    if (rhi::has_flag(desc.usage, rhi::TextureUsage::present)) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture usage cannot include present");
    }
    constexpr auto sampled_depth_usage = rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource;
    if (desc.format == rhi::Format::depth24_stencil8) {
        if (desc.extent.depth != 1U) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                              "transient depth texture extent must be 2D");
        }
        if (desc.usage != rhi::TextureUsage::depth_stencil && desc.usage != sampled_depth_usage) {
            append_frame_graph_rhi_diagnostic(
                result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                "transient depth texture usage supports only depth_stencil or sampled depth");
        }
        return;
    }
    if (rhi::has_flag(desc.usage, rhi::TextureUsage::depth_stencil)) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture depth_stencil usage requires depth24_stencil8 format");
    }
}

template <typename Result>
void validate_transient_texture_usage(Result& result, const std::string& resource,
                                      const std::vector<TransientTextureUse>& uses, const rhi::TextureDesc& desc) {
    bool needs_render_target = false;
    bool needs_depth_stencil = false;
    bool needs_shader_resource = false;
    bool needs_copy_source = false;
    bool needs_copy_destination = false;
    for (const auto& use : uses) {
        switch (use.access) {
        case FrameGraphAccess::color_attachment_write:
            needs_render_target = true;
            break;
        case FrameGraphAccess::depth_attachment_write:
            needs_depth_stencil = true;
            break;
        case FrameGraphAccess::shader_read:
            needs_shader_resource = true;
            break;
        case FrameGraphAccess::copy_source:
            needs_copy_source = true;
            break;
        case FrameGraphAccess::copy_destination:
            needs_copy_destination = true;
            break;
        case FrameGraphAccess::present:
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, use.pass, resource,
                                              "transient texture access cannot be present");
            break;
        case FrameGraphAccess::unknown:
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, use.pass, resource,
                                              "transient texture access cannot be unknown");
            break;
        }
    }

    if (needs_render_target && !rhi::has_flag(desc.usage, rhi::TextureUsage::render_target)) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture usage is missing render_target");
    }
    if (needs_depth_stencil && !rhi::has_flag(desc.usage, rhi::TextureUsage::depth_stencil)) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture usage is missing depth_stencil");
    }
    if (needs_shader_resource && !rhi::has_flag(desc.usage, rhi::TextureUsage::shader_resource)) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture usage is missing shader_resource");
    }
    if (needs_copy_source && !rhi::has_flag(desc.usage, rhi::TextureUsage::copy_source)) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture usage is missing copy_source");
    }
    if (needs_copy_destination && !rhi::has_flag(desc.usage, rhi::TextureUsage::copy_destination)) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                          "transient texture usage is missing copy_destination");
    }
}

[[nodiscard]] bool alias_group_available_for(const TransientTextureAliasGroupAccumulator& group,
                                             const TransientTextureAliasCandidate& candidate) noexcept {
    return texture_descs_match(candidate.desc, group.group.desc) && group.last_pass_index < candidate.first_pass_index;
}

[[nodiscard]] std::vector<PlannedTextureFinalState>
plan_final_texture_states(FrameGraphRhiTextureExecutionResult& result,
                          const std::map<std::string, std::size_t>& binding_indices,
                          std::span<const FrameGraphTextureFinalState> final_states) {
    std::map<std::string, std::size_t> final_state_indices;
    std::vector<PlannedTextureFinalState> planned_final_states;
    planned_final_states.reserve(final_states.size());

    for (std::size_t index = 0; index < final_states.size(); ++index) {
        const auto& final_state = final_states[index];
        if (final_state.resource.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {},
                                              "frame graph texture final state resource name is empty");
            continue;
        }
        const auto [_, inserted] = final_state_indices.emplace(final_state.resource, index);
        if (!inserted) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                              final_state.resource,
                                              "frame graph texture final state is declared more than once");
            continue;
        }
        const auto binding = binding_indices.find(final_state.resource);
        if (binding == binding_indices.end()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                              final_state.resource,
                                              "frame graph texture final state has no texture binding");
            continue;
        }
        if (final_state.state == rhi::ResourceState::undefined) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                              final_state.resource,
                                              "frame graph texture final state cannot be undefined");
            continue;
        }
        planned_final_states.push_back(PlannedTextureFinalState{
            .binding_index = binding->second,
            .after = final_state.state,
            .final_state = &final_state,
        });
    }

    return planned_final_states;
}

void propagate_shared_simulated_texture_state(std::span<const FrameGraphTextureBinding> texture_bindings,
                                              std::span<rhi::ResourceState> states, std::size_t binding_index,
                                              rhi::ResourceState state);

template <typename Result>
[[nodiscard]] bool plan_barrier(Result& result, std::vector<PlannedTextureBarrier>& plan,
                                const FrameGraphBarrier& barrier,
                                const std::map<std::string, std::size_t>& binding_indices,
                                std::span<const FrameGraphTextureBinding> texture_bindings,
                                std::span<rhi::ResourceState> simulated_states) {
    const auto before = frame_graph_texture_state_for_access(barrier.from);
    const auto after = frame_graph_texture_state_for_access(barrier.to);
    if (!before.has_value() || !after.has_value()) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, barrier.to_pass,
                                          barrier.resource,
                                          "frame graph texture barrier access is not texture-recordable");
        return false;
    }
    if (*before == *after) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, barrier.to_pass,
                                          barrier.resource, "frame graph texture barrier maps to identical RHI states");
        return false;
    }

    const auto binding = binding_indices.find(barrier.resource);
    if (binding == binding_indices.end()) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, barrier.to_pass,
                                          barrier.resource, "frame graph texture barrier has no texture binding");
        return false;
    }

    auto& simulated_state = simulated_states[binding->second];
    if (simulated_state == *after) {
        return true;
    }
    if (simulated_state != *before) {
        append_frame_graph_rhi_diagnostic(
            result, FrameGraphDiagnosticCode::invalid_resource, barrier.to_pass, barrier.resource,
            "frame graph texture binding current state does not match barrier before state");
        return false;
    }
    plan.push_back(PlannedTextureBarrier{
        .binding_index = binding->second,
        .before = *before,
        .after = *after,
        .barrier = &barrier,
    });
    propagate_shared_simulated_texture_state(texture_bindings, simulated_states, binding->second, *after);
    return true;
}

[[nodiscard]] std::vector<rhi::ResourceState>
copy_binding_states(std::span<const FrameGraphTextureBinding> texture_bindings) {
    std::vector<rhi::ResourceState> states;
    states.reserve(texture_bindings.size());
    for (const auto& binding : texture_bindings) {
        states.push_back(binding.current_state);
    }
    return states;
}

void propagate_shared_simulated_texture_state(std::span<const FrameGraphTextureBinding> texture_bindings,
                                              std::span<rhi::ResourceState> states, std::size_t binding_index,
                                              rhi::ResourceState state) {
    const auto texture = texture_bindings[binding_index].texture;
    for (std::size_t index = 0; index < texture_bindings.size(); ++index) {
        if (texture_bindings[index].texture.value == texture.value) {
            states[index] = state;
        }
    }
}

void propagate_shared_texture_binding_state(std::span<FrameGraphTextureBinding> texture_bindings,
                                            rhi::TextureHandle texture, rhi::ResourceState state) {
    for (auto& binding : texture_bindings) {
        if (binding.texture.value == texture.value) {
            binding.current_state = state;
        }
    }
}

template <typename Result>
[[nodiscard]] bool record_planned_texture_barrier(Result& result, rhi::IRhiCommandList& commands,
                                                  std::span<FrameGraphTextureBinding> texture_bindings,
                                                  const PlannedTextureBarrier& planned) {
    if (planned.barrier == nullptr) {
        return true;
    }

    auto& texture_binding = texture_bindings[planned.binding_index];
    try {
        commands.transition_texture(texture_binding.texture, planned.before, planned.after);
        propagate_shared_texture_binding_state(texture_bindings, texture_binding.texture, planned.after);
        ++result.barriers_recorded;
        return true;
    } catch (const std::exception& ex) {
        (void)ex;
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, planned.barrier->to_pass,
                                          planned.barrier->resource, "frame graph texture barrier recording failed");
        return false;
    } catch (...) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, planned.barrier->to_pass,
                                          planned.barrier->resource, "frame graph texture barrier recording failed");
        return false;
    }
}

[[nodiscard]] bool record_planned_texture_pass_target_state(FrameGraphRhiTextureExecutionResult& result,
                                                            rhi::IRhiCommandList& commands,
                                                            std::span<FrameGraphTextureBinding> texture_bindings,
                                                            const PlannedTexturePassTargetState& planned) {
    if (planned.pass_target_state == nullptr) {
        return true;
    }

    auto& texture_binding = texture_bindings[planned.binding_index];
    if (texture_binding.current_state == planned.after) {
        return true;
    }

    try {
        commands.transition_texture(texture_binding.texture, texture_binding.current_state, planned.after);
        propagate_shared_texture_binding_state(texture_bindings, texture_binding.texture, planned.after);
        ++result.barriers_recorded;
        ++result.pass_target_state_barriers_recorded;
        return true;
    } catch (const std::exception& ex) {
        (void)ex;
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                          planned.pass_target_state->pass_name, planned.pass_target_state->resource,
                                          "frame graph texture pass target-state barrier recording failed");
        return false;
    } catch (...) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource,
                                          planned.pass_target_state->pass_name, planned.pass_target_state->resource,
                                          "frame graph texture pass target-state barrier recording failed");
        return false;
    }
}

[[nodiscard]] bool record_planned_texture_final_state(FrameGraphRhiTextureExecutionResult& result,
                                                      rhi::IRhiCommandList& commands,
                                                      std::span<FrameGraphTextureBinding> texture_bindings,
                                                      const PlannedTextureFinalState& planned) {
    if (planned.final_state == nullptr) {
        return true;
    }

    auto& texture_binding = texture_bindings[planned.binding_index];
    if (texture_binding.current_state == planned.after) {
        return true;
    }

    try {
        commands.transition_texture(texture_binding.texture, texture_binding.current_state, planned.after);
        propagate_shared_texture_binding_state(texture_bindings, texture_binding.texture, planned.after);
        ++result.barriers_recorded;
        ++result.final_state_barriers_recorded;
        return true;
    } catch (const std::exception& ex) {
        (void)ex;
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                          planned.final_state->resource,
                                          "frame graph texture final-state barrier recording failed");
        return false;
    } catch (...) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                          planned.final_state->resource,
                                          "frame graph texture final-state barrier recording failed");
        return false;
    }
}

} // namespace

std::optional<rhi::ResourceState> frame_graph_texture_state_for_access(FrameGraphAccess access) noexcept {
    switch (access) {
    case FrameGraphAccess::color_attachment_write:
        return rhi::ResourceState::render_target;
    case FrameGraphAccess::depth_attachment_write:
        return rhi::ResourceState::depth_write;
    case FrameGraphAccess::shader_read:
        return rhi::ResourceState::shader_read;
    case FrameGraphAccess::copy_source:
        return rhi::ResourceState::copy_source;
    case FrameGraphAccess::copy_destination:
        return rhi::ResourceState::copy_destination;
    case FrameGraphAccess::present:
        return rhi::ResourceState::present;
    case FrameGraphAccess::unknown:
        break;
    }
    return std::nullopt;
}

std::vector<FrameGraphTexturePassTargetAccess>
build_frame_graph_texture_pass_target_accesses(const FrameGraphV1Desc& desc) {
    std::vector<FrameGraphTexturePassTargetAccess> pass_target_accesses;
    for (const auto& pass : desc.passes) {
        for (const auto& write : pass.writes) {
            if (!frame_graph_texture_state_for_access(write.access).has_value()) {
                continue;
            }
            pass_target_accesses.push_back(FrameGraphTexturePassTargetAccess{
                .pass_name = pass.name,
                .resource = write.resource,
                .access = write.access,
            });
        }
    }
    return pass_target_accesses;
}

FrameGraphTransientTextureAliasPlan
plan_frame_graph_transient_texture_aliases(const FrameGraphV1Desc& desc,
                                           std::span<const FrameGraphTransientTextureDesc> texture_descs) {
    FrameGraphTransientTextureAliasPlan result;
    const auto built = compile_frame_graph_v1(desc);
    if (!built.succeeded()) {
        result.diagnostics = built.diagnostics;
        return result;
    }

    std::map<std::string, FrameGraphResourceLifetime> declared_resources;
    for (const auto& resource : desc.resources) {
        declared_resources.emplace(resource.name, resource.lifetime);
    }

    std::map<std::string, std::size_t> ordered_pass_indices;
    for (std::size_t index = 0; index < built.ordered_passes.size(); ++index) {
        ordered_pass_indices.emplace(built.ordered_passes[index], index);
    }

    std::map<std::string, TransientTextureResourceUse> used_transient_resources;
    for (const auto& pass : desc.passes) {
        const auto ordered_pass = ordered_pass_indices.find(pass.name);
        if (ordered_pass == ordered_pass_indices.end()) {
            continue;
        }

        const auto collect_use = [&](const FrameGraphResourceAccess& access) {
            const auto declared = declared_resources.find(access.resource);
            if (declared == declared_resources.end() || declared->second != FrameGraphResourceLifetime::transient) {
                return;
            }
            auto& resource_use = used_transient_resources[access.resource];
            if (resource_use.resource.empty()) {
                resource_use.resource = access.resource;
            }
            resource_use.uses.push_back(TransientTextureUse{
                .pass = pass.name,
                .pass_index = ordered_pass->second,
                .access = access.access,
            });
        };

        for (const auto& read : pass.reads) {
            collect_use(read);
        }
        for (const auto& write : pass.writes) {
            collect_use(write);
        }
    }

    std::map<std::string, rhi::TextureDesc> texture_desc_index;
    for (const auto& texture : texture_descs) {
        if (texture.resource.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {},
                                              "transient texture descriptor resource name is empty");
            continue;
        }

        const auto declared = declared_resources.find(texture.resource);
        if (declared == declared_resources.end()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, texture.resource,
                                              "transient texture descriptor targets an undeclared resource");
            continue;
        }
        if (declared->second == FrameGraphResourceLifetime::imported) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, texture.resource,
                                              "transient texture descriptor targets an imported resource");
            continue;
        }
        if (!used_transient_resources.contains(texture.resource)) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, texture.resource,
                                              "transient texture descriptor targets an unused resource");
            continue;
        }

        const auto [_, inserted] = texture_desc_index.emplace(texture.resource, texture.desc);
        if (!inserted) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, texture.resource,
                                              "transient texture descriptor is declared more than once");
            continue;
        }
        validate_transient_texture_desc_shape(result, texture.resource, texture.desc);
    }

    std::vector<TransientTextureAliasCandidate> candidates;
    candidates.reserve(used_transient_resources.size());
    for (const auto& [resource, use] : used_transient_resources) {
        const auto texture = texture_desc_index.find(resource);
        if (texture == texture_desc_index.end()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                              "used transient texture resource has no descriptor");
            continue;
        }

        validate_transient_texture_usage(result, resource, use.uses, texture->second);

        std::size_t first_pass_index = std::numeric_limits<std::size_t>::max();
        std::size_t last_pass_index = 0;
        for (const auto& texture_use : use.uses) {
            first_pass_index = std::min(first_pass_index, texture_use.pass_index);
            last_pass_index = std::max(last_pass_index, texture_use.pass_index);
        }
        const auto estimated_bytes = estimate_texture_size_bytes(texture->second);
        if (!estimated_bytes.has_value()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource,
                                              "transient texture byte estimate overflowed");
            continue;
        }

        candidates.push_back(TransientTextureAliasCandidate{
            .resource = resource,
            .desc = texture->second,
            .first_pass_index = first_pass_index,
            .last_pass_index = last_pass_index,
            .estimated_bytes = *estimated_bytes,
        });
    }

    if (!result.succeeded()) {
        return result;
    }

    std::ranges::sort(candidates,
                      [](const TransientTextureAliasCandidate& left, const TransientTextureAliasCandidate& right) {
                          if (left.first_pass_index != right.first_pass_index) {
                              return left.first_pass_index < right.first_pass_index;
                          }
                          return left.resource < right.resource;
                      });

    std::vector<TransientTextureAliasGroupAccumulator> group_accumulators;
    for (const auto& candidate : candidates) {
        FrameGraphTransientTextureLifetime lifetime{
            .resource = candidate.resource,
            .desc = candidate.desc,
            .first_pass_index = candidate.first_pass_index,
            .last_pass_index = candidate.last_pass_index,
            .alias_group = 0,
            .estimated_bytes = candidate.estimated_bytes,
        };
        const auto estimated_unaliased_bytes =
            checked_add_u64(result.estimated_unaliased_bytes, candidate.estimated_bytes);
        if (!estimated_unaliased_bytes.has_value()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                              candidate.resource, "transient texture byte estimate overflowed");
            return result;
        }
        result.estimated_unaliased_bytes = *estimated_unaliased_bytes;

        bool assigned = false;
        for (auto& group_accumulator : group_accumulators) {
            if (!alias_group_available_for(group_accumulator, candidate)) {
                continue;
            }
            lifetime.alias_group = group_accumulator.group.index;
            group_accumulator.group.resources.push_back(candidate.resource);
            group_accumulator.last_pass_index = candidate.last_pass_index;
            assigned = true;
            break;
        }

        if (!assigned) {
            lifetime.alias_group = group_accumulators.size();
            group_accumulators.push_back(TransientTextureAliasGroupAccumulator{
                .group =
                    FrameGraphTransientTextureAliasGroup{
                        .index = lifetime.alias_group,
                        .desc = candidate.desc,
                        .estimated_bytes = candidate.estimated_bytes,
                        .resources = {candidate.resource},
                    },
                .last_pass_index = candidate.last_pass_index,
            });
        }

        result.lifetimes.push_back(lifetime);
    }

    result.alias_groups.reserve(group_accumulators.size());
    for (const auto& group_accumulator : group_accumulators) {
        result.alias_groups.push_back(group_accumulator.group);
        const auto estimated_aliased_bytes =
            checked_add_u64(result.estimated_aliased_bytes, group_accumulator.group.estimated_bytes);
        if (!estimated_aliased_bytes.has_value()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {},
                                              "transient texture byte estimate overflowed");
            return result;
        }
        result.estimated_aliased_bytes = *estimated_aliased_bytes;
    }

    return result;
}

void release_frame_graph_transient_texture_lease_bindings(rhi::IRhiDevice& device,
                                                          std::span<const FrameGraphTransientTextureLease> leases) {
    for (const auto& lease : leases) {
        device.release_transient(lease.transient_alias_group.lease);
    }
}

FrameGraphTransientTextureLeaseBindingResult
acquire_frame_graph_transient_texture_lease_bindings(rhi::IRhiDevice& device,
                                                     const FrameGraphTransientTextureAliasPlan& plan) {
    FrameGraphTransientTextureLeaseBindingResult result;
    if (!plan.succeeded()) {
        result.diagnostics = plan.diagnostics;
        return result;
    }

    for (const auto& group : plan.alias_groups) {
        const auto group_resource = "alias-group-" + std::to_string(group.index);
        if (group.resources.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, group_resource,
                                              "frame graph transient texture alias group has no resources");
            continue;
        }
        for (const auto& resource : group.resources) {
            if (resource.empty()) {
                append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                                  group_resource,
                                                  "frame graph transient texture alias group resource name is empty");
            }
        }
    }
    if (!result.succeeded()) {
        return result;
    }

    for (const auto& group : plan.alias_groups) {
        try {
            auto transient = device.acquire_transient_texture_alias_group(group.desc, group.resources.size());
            if (transient.lease.value == 0 || transient.textures.size() != group.resources.size()) {
                if (transient.lease.value != 0) {
                    device.release_transient(transient.lease);
                }
                release_frame_graph_transient_texture_lease_bindings(device, result.leases);
                result.leases.clear();
                result.texture_bindings.clear();
                append_frame_graph_rhi_diagnostic(
                    result, FrameGraphDiagnosticCode::invalid_resource, {},
                    "alias-group-" + std::to_string(group.index),
                    "frame graph transient texture alias group returned an invalid texture count");
                return result;
            }
            for (std::size_t texture_index = 0; texture_index < transient.textures.size(); ++texture_index) {
                const auto texture = transient.textures[texture_index];
                if (texture.value == 0) {
                    device.release_transient(transient.lease);
                    release_frame_graph_transient_texture_lease_bindings(device, result.leases);
                    result.leases.clear();
                    result.texture_bindings.clear();
                    append_frame_graph_rhi_diagnostic(
                        result, FrameGraphDiagnosticCode::invalid_resource, {}, group.resources[texture_index],
                        "frame graph transient texture alias group returned an invalid texture handle");
                    return result;
                }
                const auto duplicate = std::ranges::any_of(
                    std::span<const rhi::TextureHandle>{transient.textures.data(), texture_index},
                    [texture](rhi::TextureHandle existing) noexcept { return existing.value == texture.value; });
                if (duplicate) {
                    device.release_transient(transient.lease);
                    release_frame_graph_transient_texture_lease_bindings(device, result.leases);
                    result.leases.clear();
                    result.texture_bindings.clear();
                    append_frame_graph_rhi_diagnostic(
                        result, FrameGraphDiagnosticCode::invalid_resource, {}, group.resources[texture_index],
                        "frame graph transient texture alias group returned duplicate texture handles");
                    return result;
                }
            }
            result.leases.push_back(FrameGraphTransientTextureLease{
                .alias_group = group.index,
                .transient_alias_group = transient,
            });

            for (std::size_t resource_index = 0; resource_index < group.resources.size(); ++resource_index) {
                result.texture_bindings.push_back(FrameGraphTextureBinding{
                    .resource = group.resources[resource_index],
                    .texture = transient.textures[resource_index],
                    .current_state = rhi::ResourceState::undefined,
                });
            }
        } catch (const std::exception& exception) {
            release_frame_graph_transient_texture_lease_bindings(device, result.leases);
            result.leases.clear();
            result.texture_bindings.clear();
            append_frame_graph_rhi_diagnostic(
                result, FrameGraphDiagnosticCode::invalid_resource, {}, "alias-group-" + std::to_string(group.index),
                "frame graph transient texture alias group acquisition failed: " + std::string(exception.what()));
            return result;
        } catch (...) {
            release_frame_graph_transient_texture_lease_bindings(device, result.leases);
            result.leases.clear();
            result.texture_bindings.clear();
            append_frame_graph_rhi_diagnostic(
                result, FrameGraphDiagnosticCode::invalid_resource, {}, "alias-group-" + std::to_string(group.index),
                "frame graph transient texture alias group acquisition failed: unknown exception");
            return result;
        }
    }

    return result;
}

FrameGraphTextureBarrierRecordResult
record_frame_graph_texture_barriers(rhi::IRhiCommandList& commands, std::span<const FrameGraphExecutionStep> schedule,
                                    std::span<FrameGraphTextureBinding> texture_bindings) {
    FrameGraphTextureBarrierRecordResult result;
    if (commands.closed()) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                          "frame graph texture barriers cannot record to a closed command list");
        return result;
    }

    const auto binding_indices = build_binding_index(result, texture_bindings);
    if (!result.succeeded()) {
        return result;
    }

    auto simulated_states = copy_binding_states(texture_bindings);

    std::vector<PlannedTextureBarrier> planned_barriers;
    planned_barriers.reserve(schedule.size());
    for (const auto& step : schedule) {
        if (step.kind != FrameGraphExecutionStep::Kind::barrier) {
            continue;
        }
        (void)plan_barrier(result, planned_barriers, step.barrier, binding_indices, texture_bindings, simulated_states);
    }
    if (!result.succeeded()) {
        return result;
    }

    for (const auto& planned : planned_barriers) {
        if (planned.barrier == nullptr) {
            continue;
        }

        if (!record_planned_texture_barrier(result, commands, texture_bindings, planned)) {
            return result;
        }
    }

    return result;
}

FrameGraphTextureAliasingBarrierRecordResult
record_frame_graph_texture_aliasing_barriers(rhi::IRhiCommandList& commands,
                                             std::span<const FrameGraphTextureAliasingBarrier> aliasing_barriers,
                                             std::span<const FrameGraphTextureBinding> texture_bindings) {
    FrameGraphTextureAliasingBarrierRecordResult result;
    if (commands.closed()) {
        append_frame_graph_rhi_diagnostic(
            result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
            "frame graph texture aliasing barriers cannot record to a closed command list");
        return result;
    }

    const auto binding_indices = build_binding_index(result, texture_bindings);
    if (!result.succeeded()) {
        return result;
    }

    std::vector<std::pair<rhi::TextureHandle, rhi::TextureHandle>> planned_barriers;
    planned_barriers.reserve(aliasing_barriers.size());
    for (const auto& barrier : aliasing_barriers) {
        if (barrier.before_resource.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {},
                                              "frame graph texture aliasing barrier before resource name is empty");
            continue;
        }
        if (barrier.after_resource.empty()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {},
                                              "frame graph texture aliasing barrier after resource name is empty");
            continue;
        }

        const auto before = binding_indices.find(barrier.before_resource);
        const auto after = binding_indices.find(barrier.after_resource);
        if (before == binding_indices.end()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                              barrier.before_resource,
                                              "frame graph texture aliasing barrier references an unknown resource");
            continue;
        }
        if (after == binding_indices.end()) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                              barrier.after_resource,
                                              "frame graph texture aliasing barrier references an unknown resource");
            continue;
        }

        const auto before_handle = texture_bindings[before->second].texture;
        const auto after_handle = texture_bindings[after->second].texture;
        if (before_handle.value == after_handle.value) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                              barrier.after_resource,
                                              "frame graph texture aliasing barrier requires distinct texture handles");
            continue;
        }
        planned_barriers.emplace_back(before_handle, after_handle);
    }
    if (!result.succeeded()) {
        return result;
    }

    for (std::size_t index = 0; index < planned_barriers.size(); ++index) {
        const auto& planned = planned_barriers[index];
        const auto& barrier = aliasing_barriers[index];
        try {
            commands.texture_aliasing_barrier(planned.first, planned.second);
        } catch (const std::exception& error) {
            append_frame_graph_rhi_diagnostic(
                result, FrameGraphDiagnosticCode::invalid_resource, {}, barrier.after_resource,
                std::string{"frame graph texture aliasing barrier recording failed: "} + error.what());
            return result;
        } catch (...) {
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {},
                                              barrier.after_resource,
                                              "frame graph texture aliasing barrier recording failed");
            return result;
        }
        ++result.aliasing_barriers_recorded;
    }

    return result;
}

FrameGraphRhiTextureExecutionResult
execute_frame_graph_rhi_texture_schedule(const FrameGraphRhiTextureExecutionDesc& desc) {
    FrameGraphRhiTextureExecutionResult result;
    if (desc.commands == nullptr) {
        append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                          "frame graph rhi texture schedule execution requires a command list");
        return result;
    }
    if (desc.commands->closed()) {
        append_frame_graph_rhi_diagnostic(
            result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
            "frame graph rhi texture schedule execution cannot use a closed command list");
        return result;
    }

    const auto binding_indices = build_binding_index(result, desc.texture_bindings);
    const auto pass_callbacks = build_pass_callback_index(result, desc.pass_callbacks);
    const auto scheduled_passes = build_scheduled_pass_index(desc.schedule);
    const auto pass_target_accesses = build_pass_target_access_index(result, desc.pass_target_accesses);
    const auto planned_pass_target_states = result.succeeded()
                                                ? plan_pass_target_states(result, binding_indices, scheduled_passes,
                                                                          pass_target_accesses, desc.pass_target_states)
                                                : std::map<std::string, std::vector<PlannedTexturePassTargetState>>{};
    const auto planned_final_states = plan_final_texture_states(result, binding_indices, desc.final_states);
    for (const auto& step : desc.schedule) {
        switch (step.kind) {
        case FrameGraphExecutionStep::Kind::barrier:
            break;
        case FrameGraphExecutionStep::Kind::pass_invoke:
            if (step.pass_name.empty()) {
                append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                                  "frame graph schedule pass name is empty");
            } else if (!pass_callbacks.contains(step.pass_name)) {
                append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                                  "frame graph pass callback is missing");
            }
            break;
        default:
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                              "frame graph execution step kind is invalid");
            break;
        }
    }
    if (!result.succeeded()) {
        return result;
    }

    auto simulated_states = copy_binding_states(desc.texture_bindings);
    std::vector<PlannedTextureBarrier> validation_barriers;
    validation_barriers.reserve(desc.schedule.size());
    for (const auto& step : desc.schedule) {
        switch (step.kind) {
        case FrameGraphExecutionStep::Kind::barrier:
            (void)plan_barrier(result, validation_barriers, step.barrier, binding_indices, desc.texture_bindings,
                               simulated_states);
            break;
        case FrameGraphExecutionStep::Kind::pass_invoke: {
            const auto target_states = planned_pass_target_states.find(step.pass_name);
            if (target_states == planned_pass_target_states.end()) {
                break;
            }
            for (const auto& planned_target_state : target_states->second) {
                propagate_shared_simulated_texture_state(desc.texture_bindings, simulated_states,
                                                         planned_target_state.binding_index,
                                                         planned_target_state.after);
            }
        } break;
        default:
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                              "frame graph execution step kind is invalid");
            break;
        }
    }
    if (!result.succeeded()) {
        return result;
    }

    for (const auto& step : desc.schedule) {
        switch (step.kind) {
        case FrameGraphExecutionStep::Kind::barrier: {
            auto current_states = copy_binding_states(desc.texture_bindings);
            std::vector<PlannedTextureBarrier> planned_barriers;
            planned_barriers.reserve(1);
            if (!plan_barrier(result, planned_barriers, step.barrier, binding_indices, desc.texture_bindings,
                              current_states)) {
                return result;
            }
            for (const auto& planned_barrier : planned_barriers) {
                if (!record_planned_texture_barrier(result, *desc.commands, desc.texture_bindings, planned_barrier)) {
                    return result;
                }
            }
        } break;
        case FrameGraphExecutionStep::Kind::pass_invoke: {
            const auto target_states = planned_pass_target_states.find(step.pass_name);
            if (target_states != planned_pass_target_states.end()) {
                for (const auto& planned_target_state : target_states->second) {
                    if (!record_planned_texture_pass_target_state(result, *desc.commands, desc.texture_bindings,
                                                                  planned_target_state)) {
                        return result;
                    }
                }
            }

            const auto callback = pass_callbacks.find(step.pass_name);
            FrameGraphExecutionCallbackResult callback_result;
            try {
                callback_result = callback->second(callback->first);
            } catch (const std::exception& ex) {
                append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                                  std::string{"frame graph pass callback threw an exception: "} +
                                                      ex.what());
                return result;
            } catch (...) {
                append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                                  "frame graph pass callback threw an exception");
                return result;
            }
            if (!callback_result.succeeded()) {
                append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                                  callback_result.message.empty() ? "frame graph pass callback failed"
                                                                                  : std::move(callback_result.message));
                return result;
            }
            ++result.pass_callbacks_invoked;
            break;
        }
        default:
            append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                              "frame graph execution step kind is invalid");
            return result;
        }
    }

    for (const auto& planned_final_state : planned_final_states) {
        if (!record_planned_texture_final_state(result, *desc.commands, desc.texture_bindings, planned_final_state)) {
            return result;
        }
    }

    return result;
}

} // namespace mirakana
