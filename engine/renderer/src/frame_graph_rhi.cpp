// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/frame_graph_rhi.hpp"

#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <exception>
#include <functional>
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

template <typename Result>
[[nodiscard]] bool plan_barrier(Result& result, std::vector<PlannedTextureBarrier>& plan,
                                const FrameGraphBarrier& barrier,
                                const std::map<std::string, std::size_t>& binding_indices,
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
    simulated_state = *after;
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
        texture_binding.current_state = planned.after;
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
        (void)plan_barrier(result, planned_barriers, step.barrier, binding_indices, simulated_states);
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
    auto simulated_states = copy_binding_states(desc.texture_bindings);
    std::vector<PlannedTextureBarrier> planned_barriers;
    planned_barriers.reserve(desc.schedule.size());
    for (const auto& step : desc.schedule) {
        switch (step.kind) {
        case FrameGraphExecutionStep::Kind::barrier:
            (void)plan_barrier(result, planned_barriers, step.barrier, binding_indices, simulated_states);
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

    std::size_t next_planned_barrier = 0;
    for (const auto& step : desc.schedule) {
        switch (step.kind) {
        case FrameGraphExecutionStep::Kind::barrier:
            if (next_planned_barrier < planned_barriers.size() &&
                planned_barriers[next_planned_barrier].barrier == &step.barrier) {
                if (!record_planned_texture_barrier(result, *desc.commands, desc.texture_bindings,
                                                    planned_barriers[next_planned_barrier])) {
                    return result;
                }
                ++next_planned_barrier;
            }
            break;
        case FrameGraphExecutionStep::Kind::pass_invoke: {
            const auto callback = pass_callbacks.find(step.pass_name);
            FrameGraphExecutionCallbackResult callback_result;
            try {
                callback_result = callback->second(callback->first);
            } catch (const std::exception&) {
                append_frame_graph_rhi_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                                  "frame graph pass callback threw an exception");
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

    return result;
}

} // namespace mirakana
