// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/frame_graph.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <map>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

struct ResourceUse {
    std::size_t pass{static_cast<std::size_t>(-1)};
    FrameGraphAccess access{FrameGraphAccess::unknown};
};

struct ResourceState {
    bool declared{false};
    FrameGraphResourceLifetime lifetime{FrameGraphResourceLifetime::transient};
    std::size_t writer{static_cast<std::size_t>(-1)};
    FrameGraphAccess writer_access{FrameGraphAccess::unknown};
    std::vector<ResourceUse> readers;
};

[[nodiscard]] constexpr std::size_t invalid_index() noexcept {
    return static_cast<std::size_t>(-1);
}

void append_diagnostic(FrameGraphBuildResult& result, FrameGraphDiagnosticCode code, std::string pass,
                       std::string resource, std::string message) {
    result.diagnostics.push_back(FrameGraphDiagnostic{
        .code = code,
        .pass = std::move(pass),
        .resource = std::move(resource),
        .message = std::move(message),
    });
}

void append_diagnostic(FrameGraphExecutionResult& result, FrameGraphDiagnosticCode code, std::string pass,
                       std::string resource, std::string message) {
    result.diagnostics.push_back(FrameGraphDiagnostic{
        .code = code,
        .pass = std::move(pass),
        .resource = std::move(resource),
        .message = std::move(message),
    });
}

void append_diagnostic(FrameGraphProductionOwnershipPlan& result, FrameGraphDiagnosticCode code, std::string pass,
                       std::string resource, std::string message) {
    result.diagnostics.push_back(FrameGraphDiagnostic{
        .code = code,
        .pass = std::move(pass),
        .resource = std::move(resource),
        .message = std::move(message),
    });
}

[[nodiscard]] FrameGraphProductionOwnershipBoundary
supported_production_ownership_boundary(FrameGraphProductionOwnershipCapability capability) noexcept {
    switch (capability) {
    case FrameGraphProductionOwnershipCapability::texture_state_transitions:
    case FrameGraphProductionOwnershipCapability::texture_aliasing_barriers:
    case FrameGraphProductionOwnershipCapability::pass_target_state_preparation:
    case FrameGraphProductionOwnershipCapability::render_pass_envelopes:
    case FrameGraphProductionOwnershipCapability::pass_callbacks:
    case FrameGraphProductionOwnershipCapability::queue_waits:
    case FrameGraphProductionOwnershipCapability::multi_queue_command_submission:
    case FrameGraphProductionOwnershipCapability::runtime_upload_commands:
    case FrameGraphProductionOwnershipCapability::package_streaming_texture_binding_handoff:
    case FrameGraphProductionOwnershipCapability::vulkan_memory_aliasing:
        return FrameGraphProductionOwnershipBoundary::frame_graph_owned;
    case FrameGraphProductionOwnershipCapability::swapchain_acquire_present:
    case FrameGraphProductionOwnershipCapability::viewport_readback:
    case FrameGraphProductionOwnershipCapability::native_overlay_preparation:
        return FrameGraphProductionOwnershipBoundary::renderer_owned;
    case FrameGraphProductionOwnershipCapability::package_streaming_residency:
        return FrameGraphProductionOwnershipBoundary::runtime_host_owned;
    case FrameGraphProductionOwnershipCapability::metal_memory_aliasing:
        return FrameGraphProductionOwnershipBoundary::host_gated;
    case FrameGraphProductionOwnershipCapability::production_multi_queue_graph_adoption:
    case FrameGraphProductionOwnershipCapability::broad_background_package_streaming:
    case FrameGraphProductionOwnershipCapability::data_inheritance_preservation:
    case FrameGraphProductionOwnershipCapability::async_overlap_performance:
    case FrameGraphProductionOwnershipCapability::public_native_handles:
        return FrameGraphProductionOwnershipBoundary::unsupported;
    case FrameGraphProductionOwnershipCapability::none:
        break;
    }
    return FrameGraphProductionOwnershipBoundary::unspecified;
}

void increment_production_ownership_boundary_count(FrameGraphProductionOwnershipPlan& result,
                                                   FrameGraphProductionOwnershipBoundary boundary) noexcept {
    switch (boundary) {
    case FrameGraphProductionOwnershipBoundary::frame_graph_owned:
        ++result.frame_graph_owned_count;
        break;
    case FrameGraphProductionOwnershipBoundary::renderer_owned:
        ++result.renderer_owned_count;
        break;
    case FrameGraphProductionOwnershipBoundary::runtime_host_owned:
        ++result.runtime_host_owned_count;
        break;
    case FrameGraphProductionOwnershipBoundary::host_gated:
        ++result.host_gated_count;
        break;
    case FrameGraphProductionOwnershipBoundary::unsupported:
        ++result.unsupported_count;
        break;
    case FrameGraphProductionOwnershipBoundary::unspecified:
        break;
    }
}

void clear_production_ownership_boundary_plan(FrameGraphProductionOwnershipPlan& result) noexcept {
    result.selections.clear();
    result.frame_graph_owned_count = 0;
    result.renderer_owned_count = 0;
    result.runtime_host_owned_count = 0;
    result.host_gated_count = 0;
    result.unsupported_count = 0;
}

[[nodiscard]] bool validate_access(FrameGraphBuildResult& result, const std::map<std::string, ResourceState>& resources,
                                   std::string_view pass_name, const FrameGraphResourceAccess& access,
                                   bool write_access) {
    if (access.resource.empty()) {
        append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, std::string(pass_name), {},
                          write_access ? "pass write resource name is empty" : "pass read resource name is empty");
        return false;
    }
    const auto resource = resources.find(access.resource);
    if (resource == resources.end() || !resource->second.declared) {
        append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, std::string(pass_name), access.resource,
                          "pass references an undeclared frame graph resource");
        return false;
    }
    if (access.access == FrameGraphAccess::unknown) {
        append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, std::string(pass_name), access.resource,
                          "frame graph resource access must be explicit");
        return false;
    }
    return true;
}

} // namespace

FrameGraphBuildResult compile_frame_graph(const FrameGraphDesc& desc) {
    FrameGraphBuildResult result;
    result.pass_count = desc.passes.size();

    std::map<std::string, ResourceState> resources;
    for (const auto& resource : desc.resources) {
        if (resource.name.empty()) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {}, "resource name is empty");
            continue;
        }
        auto [entry, inserted] = resources.emplace(resource.name, ResourceState{});
        if (!inserted && entry->second.declared) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, resource.name,
                              "resource is declared more than once");
            continue;
        }
        entry->second.declared = true;
        entry->second.lifetime = resource.lifetime;
    }

    std::map<std::string, std::size_t> pass_indices;
    for (std::size_t pass_index = 0; pass_index < desc.passes.size(); ++pass_index) {
        const auto& pass = desc.passes[pass_index];
        if (pass.name.empty()) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {}, "pass name is empty");
            continue;
        }
        auto [_, inserted] = pass_indices.emplace(pass.name, pass_index);
        if (!inserted) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, pass.name, {},
                              "pass is declared more than once");
        }

        for (const auto& read : pass.reads) {
            if (!validate_access(result, resources, pass.name, read, false)) {
                continue;
            }
            resources[read.resource].readers.push_back(ResourceUse{.pass = pass_index, .access = read.access});
        }
        for (const auto& write : pass.writes) {
            if (!validate_access(result, resources, pass.name, write, true)) {
                continue;
            }
            auto& resource = resources[write.resource];
            if (resource.writer != invalid_index()) {
                append_diagnostic(result, FrameGraphDiagnosticCode::write_write_hazard, pass.name, write.resource,
                                  "resource has more than one writer");
                continue;
            }
            resource.writer = pass_index;
            resource.writer_access = write.access;
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<std::set<std::size_t>> edges(desc.passes.size());
    std::vector<std::size_t> indegrees(desc.passes.size(), 0);
    for (const auto& [name, resource] : resources) {
        for (const auto& reader : resource.readers) {
            const auto writer = resource.writer;
            if (writer == invalid_index()) {
                if (resource.lifetime == FrameGraphResourceLifetime::transient) {
                    append_diagnostic(result, FrameGraphDiagnosticCode::missing_producer, desc.passes[reader.pass].name,
                                      name, "transient resource read has no producer");
                }
                continue;
            }
            if (writer == reader.pass) {
                append_diagnostic(result, FrameGraphDiagnosticCode::read_write_hazard, desc.passes[reader.pass].name,
                                  name, "pass reads and writes the same resource");
                continue;
            }
            const auto [_, inserted] = edges[writer].insert(reader.pass);
            if (inserted) {
                ++indegrees[reader.pass];
            }
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<bool> queued(desc.passes.size(), false);
    for (std::size_t step = 0; step < desc.passes.size(); ++step) {
        auto next = invalid_index();
        for (std::size_t pass_index = 0; pass_index < desc.passes.size(); ++pass_index) {
            if (!queued[pass_index] && indegrees[pass_index] == 0) {
                next = pass_index;
                break;
            }
        }
        if (next == invalid_index()) {
            append_diagnostic(result, FrameGraphDiagnosticCode::cycle, {}, {},
                              "frame graph contains a dependency cycle");
            result.ordered_passes.clear();
            return result;
        }
        queued[next] = true;
        result.ordered_passes.push_back(desc.passes[next].name);
        for (const auto dependent : edges[next]) {
            --indegrees[dependent];
        }
    }

    for (const auto& [name, resource] : resources) {
        if (resource.writer == invalid_index()) {
            continue;
        }
        for (const auto& reader : resource.readers) {
            if (reader.pass == resource.writer || reader.access == resource.writer_access) {
                continue;
            }
            result.barriers.push_back(FrameGraphBarrier{
                .resource = name,
                .from_pass = desc.passes[resource.writer].name,
                .to_pass = desc.passes[reader.pass].name,
                .from = resource.writer_access,
                .to = reader.access,
            });
        }
    }

    return result;
}

// Turns a successful `compile_frame_graph` result into a host-submittable step list. For each pass in
// `ordered_passes`, collect barrier rows whose `to_pass` matches that pass, sort them by `(resource, from_pass)`
// so duplicate resources stay deterministic, append barrier steps, then append one pass_invoke step. Failed
// plans yield an empty vector so callers never execute diagnostics-only results.
std::vector<FrameGraphExecutionStep> schedule_frame_graph_execution(const FrameGraphBuildResult& built) {
    std::vector<FrameGraphExecutionStep> schedule;
    if (!built.succeeded()) {
        return schedule;
    }

    for (const auto& pass_name : built.ordered_passes) {
        std::vector<const FrameGraphBarrier*> incoming;
        incoming.reserve(built.barriers.size());
        for (const auto& barrier : built.barriers) {
            if (barrier.to_pass == pass_name) {
                incoming.push_back(&barrier);
            }
        }
        std::ranges::sort(incoming, [](const FrameGraphBarrier* left, const FrameGraphBarrier* right) {
            if (left->resource != right->resource) {
                return left->resource < right->resource;
            }
            return left->from_pass < right->from_pass;
        });
        for (const auto* barrier : incoming) {
            schedule.push_back(FrameGraphExecutionStep::make_barrier(*barrier));
        }
        schedule.push_back(FrameGraphExecutionStep::make_pass_invoke(pass_name));
    }
    return schedule;
}

FrameGraphExecutionResult execute_frame_graph_schedule(std::span<const FrameGraphExecutionStep> schedule,
                                                       const FrameGraphExecutionCallbacks& callbacks) {
    FrameGraphExecutionResult result;
    std::map<std::string, std::function<FrameGraphExecutionCallbackResult(std::string_view pass_name)>> pass_callbacks;
    for (const auto& binding : callbacks.pass_callbacks) {
        if (binding.pass_name.empty()) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                              "frame graph pass callback name is empty");
            continue;
        }
        if (!binding.callback) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, binding.pass_name, {},
                              "frame graph pass callback is empty");
            continue;
        }
        const auto [_, inserted] = pass_callbacks.emplace(binding.pass_name, binding.callback);
        if (!inserted) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, binding.pass_name, {},
                              "frame graph pass callback is declared more than once");
        }
    }
    if (!result.succeeded()) {
        return result;
    }

    for (const auto& step : schedule) {
        switch (step.kind) {
        case FrameGraphExecutionStep::Kind::barrier: {
            if (!callbacks.barrier_callback) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, step.barrier.to_pass,
                                  step.barrier.resource, "frame graph barrier callback is missing");
                return result;
            }

            FrameGraphExecutionCallbackResult callback_result;
            try {
                callback_result = callbacks.barrier_callback(step.barrier);
            } catch (const std::exception&) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, step.barrier.to_pass,
                                  step.barrier.resource, "frame graph barrier callback threw an exception");
                return result;
            } catch (...) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, step.barrier.to_pass,
                                  step.barrier.resource, "frame graph barrier callback threw an exception");
                return result;
            }
            if (!callback_result.succeeded()) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, step.barrier.to_pass,
                                  step.barrier.resource,
                                  callback_result.message.empty() ? "frame graph barrier callback failed"
                                                                  : std::move(callback_result.message));
                return result;
            }
            ++result.barrier_callbacks_invoked;
            break;
        }
        case FrameGraphExecutionStep::Kind::pass_invoke: {
            if (step.pass_name.empty()) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                                  "frame graph schedule pass name is empty");
                return result;
            }
            const auto callback = pass_callbacks.find(step.pass_name);
            if (callback == pass_callbacks.end()) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                  "frame graph pass callback is missing");
                return result;
            }

            FrameGraphExecutionCallbackResult callback_result;
            try {
                callback_result = callback->second(callback->first);
            } catch (const std::exception& ex) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                  std::string{"frame graph pass callback threw an exception: "} + ex.what());
                return result;
            } catch (...) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                  "frame graph pass callback threw an exception");
                return result;
            }
            if (!callback_result.succeeded()) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                  callback_result.message.empty() ? "frame graph pass callback failed"
                                                                  : std::move(callback_result.message));
                return result;
            }
            ++result.pass_callbacks_invoked;
            break;
        }
        default:
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, {}, {},
                              "frame graph execution step kind is invalid");
            return result;
        }
    }

    return result;
}

FrameGraphProductionOwnershipPlan
plan_frame_graph_production_ownership_boundary(std::span<const FrameGraphProductionOwnershipCandidate> candidates) {
    FrameGraphProductionOwnershipPlan result;
    std::map<std::string, std::size_t> candidate_indices;

    for (std::size_t index = 0; index < candidates.size(); ++index) {
        const auto& candidate = candidates[index];
        if (candidate.id.empty()) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {},
                              "frame graph production ownership candidate id is empty");
            continue;
        }

        const auto [_, inserted] = candidate_indices.emplace(candidate.id, index);
        if (!inserted) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, candidate.id,
                              "frame graph production ownership candidate is declared more than once");
            continue;
        }

        const auto supported_boundary = supported_production_ownership_boundary(candidate.capability);
        if (supported_boundary == FrameGraphProductionOwnershipBoundary::unspecified) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, candidate.id,
                              "frame graph production ownership capability is invalid");
            continue;
        }

        if (candidate.requested_boundary != FrameGraphProductionOwnershipBoundary::unspecified &&
            candidate.requested_boundary != supported_boundary) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, candidate.id,
                              "frame graph production ownership boundary request disagrees with supported boundary");
            continue;
        }

        result.selections.push_back(FrameGraphProductionOwnershipSelection{
            .id = candidate.id,
            .capability = candidate.capability,
            .boundary = supported_boundary,
        });
        increment_production_ownership_boundary_count(result, supported_boundary);
    }

    if (!result.succeeded()) {
        clear_production_ownership_boundary_plan(result);
    }
    return result;
}

} // namespace mirakana
