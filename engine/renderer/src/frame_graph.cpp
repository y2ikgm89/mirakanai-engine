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

struct ResourceState {
    bool declared{false};
    bool imported{false};
    bool requires_consumer{false};
    std::size_t writer{static_cast<std::size_t>(-1)};
    std::vector<std::size_t> readers;
};

struct ResourceUseV1 {
    std::size_t pass{static_cast<std::size_t>(-1)};
    FrameGraphAccess access{FrameGraphAccess::unknown};
};

struct ResourceStateV1 {
    bool declared{false};
    FrameGraphResourceLifetime lifetime{FrameGraphResourceLifetime::transient};
    std::size_t writer{static_cast<std::size_t>(-1)};
    FrameGraphAccess writer_access{FrameGraphAccess::unknown};
    std::vector<ResourceUseV1> readers;
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

void append_diagnostic(FrameGraphV1BuildResult& result, FrameGraphDiagnosticCode code, std::string pass,
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

[[nodiscard]] bool validate_access(FrameGraphV1BuildResult& result,
                                   const std::map<std::string, ResourceStateV1>& resources, std::string_view pass_name,
                                   const FrameGraphResourceAccess& access, bool write_access) {
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

FrameGraphBuildResult compile_frame_graph_v0(const FrameGraphDesc& desc) {
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
        entry->second.imported = resource.imported;
        entry->second.requires_consumer = resource.requires_consumer;
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
            if (read.empty()) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, pass.name, {},
                                  "pass read resource name is empty");
                continue;
            }
            resources[read].readers.push_back(pass_index);
        }
        for (const auto& write : pass.writes) {
            if (write.empty()) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, pass.name, {},
                                  "pass write resource name is empty");
                continue;
            }
            auto& resource = resources[write];
            if (resource.writer != invalid_index()) {
                append_diagnostic(result, FrameGraphDiagnosticCode::write_write_hazard, pass.name, write,
                                  "resource has more than one writer");
                continue;
            }
            resource.writer = pass_index;
        }
    }

    if (!result.diagnostics.empty()) {
        return result;
    }

    std::vector<std::set<std::size_t>> edges(desc.passes.size());
    std::vector<std::size_t> indegrees(desc.passes.size(), 0);
    for (const auto& [name, resource] : resources) {
        for (const auto reader : resource.readers) {
            const auto writer = resource.writer;
            if (writer == invalid_index()) {
                if (!resource.imported) {
                    append_diagnostic(result, FrameGraphDiagnosticCode::missing_producer, desc.passes[reader].name,
                                      name, "resource read has no producer");
                }
                continue;
            }
            if (writer == reader) {
                append_diagnostic(result, FrameGraphDiagnosticCode::read_write_hazard, desc.passes[reader].name, name,
                                  "pass reads and writes the same resource");
                continue;
            }
            const auto [_, inserted] = edges[writer].insert(reader);
            if (inserted) {
                ++indegrees[reader];
            }
        }
        if (resource.requires_consumer && resource.writer != invalid_index() && resource.readers.empty()) {
            append_diagnostic(result, FrameGraphDiagnosticCode::missing_consumer, desc.passes[resource.writer].name,
                              name, "resource write has no required consumer");
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

    return result;
}

FrameGraphV1BuildResult compile_frame_graph_v1(const FrameGraphV1Desc& desc) {
    FrameGraphV1BuildResult result;
    result.pass_count = desc.passes.size();

    std::map<std::string, ResourceStateV1> resources;
    for (const auto& resource : desc.resources) {
        if (resource.name.empty()) {
            append_diagnostic(result, FrameGraphDiagnosticCode::invalid_resource, {}, {}, "resource name is empty");
            continue;
        }
        auto [entry, inserted] = resources.emplace(resource.name, ResourceStateV1{});
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
            resources[read.resource].readers.push_back(ResourceUseV1{.pass = pass_index, .access = read.access});
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

// Turns a successful `compile_frame_graph_v1` result into a host-submittable step list. For each pass in
// `ordered_passes`, collect barrier rows whose `to_pass` matches that pass, sort them by `(resource, from_pass)`
// so duplicate resources stay deterministic, append barrier steps, then append one pass_invoke step. Failed
// plans yield an empty vector so callers never execute diagnostics-only results.
std::vector<FrameGraphExecutionStep> schedule_frame_graph_v1_execution(const FrameGraphV1BuildResult& built) {
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

FrameGraphExecutionResult execute_frame_graph_v1_schedule(std::span<const FrameGraphExecutionStep> schedule,
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
            } catch (const std::exception&) {
                append_diagnostic(result, FrameGraphDiagnosticCode::invalid_pass, step.pass_name, {},
                                  "frame graph pass callback threw an exception");
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

} // namespace mirakana
