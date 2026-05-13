// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {

enum class FrameGraphDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_pass,
    invalid_resource,
    missing_producer,
    missing_consumer,
    write_write_hazard,
    read_write_hazard,
    cycle,
};

struct FrameGraphResourceDesc {
    std::string name;
    bool imported{false};
    bool requires_consumer{false};
};

struct FrameGraphPassDesc {
    std::string name;
    std::vector<std::string> reads;
    std::vector<std::string> writes;
};

struct FrameGraphDesc {
    std::vector<FrameGraphResourceDesc> resources;
    std::vector<FrameGraphPassDesc> passes;
};

struct FrameGraphDiagnostic {
    FrameGraphDiagnosticCode code{FrameGraphDiagnosticCode::none};
    std::string pass;
    std::string resource;
    std::string message;
};

struct FrameGraphBuildResult {
    std::vector<std::string> ordered_passes;
    std::vector<FrameGraphDiagnostic> diagnostics;
    std::size_t pass_count{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] FrameGraphBuildResult compile_frame_graph_v0(const FrameGraphDesc& desc);

enum class FrameGraphResourceLifetime : std::uint8_t { imported = 0, transient };

enum class FrameGraphAccess : std::uint8_t {
    unknown = 0,
    color_attachment_write,
    depth_attachment_write,
    shader_read,
    copy_source,
    copy_destination,
    present,
};

struct FrameGraphResourceV1Desc {
    std::string name;
    FrameGraphResourceLifetime lifetime{FrameGraphResourceLifetime::transient};
};

struct FrameGraphResourceAccess {
    std::string resource;
    FrameGraphAccess access{FrameGraphAccess::unknown};
};

struct FrameGraphPassV1Desc {
    std::string name;
    std::vector<FrameGraphResourceAccess> reads;
    std::vector<FrameGraphResourceAccess> writes;
};

struct FrameGraphV1Desc {
    std::vector<FrameGraphResourceV1Desc> resources;
    std::vector<FrameGraphPassV1Desc> passes;
};

struct FrameGraphBarrier {
    std::string resource;
    std::string from_pass;
    std::string to_pass;
    FrameGraphAccess from{FrameGraphAccess::unknown};
    FrameGraphAccess to{FrameGraphAccess::unknown};
};

struct FrameGraphV1BuildResult {
    std::vector<std::string> ordered_passes;
    std::vector<FrameGraphBarrier> barriers;
    std::vector<FrameGraphDiagnostic> diagnostics;
    std::size_t pass_count{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] FrameGraphV1BuildResult compile_frame_graph_v1(const FrameGraphV1Desc& desc);

/// Single step in a backend-neutral execution schedule derived from a successful v1 plan.
struct FrameGraphExecutionStep {
    enum class Kind : std::uint8_t { barrier, pass_invoke } kind{Kind::pass_invoke};
    /// Valid when `kind == barrier`; encodes the intended resource access transition before `to_pass`.
    FrameGraphBarrier barrier{};
    /// Valid when `kind == pass_invoke`; matches an entry in `FrameGraphV1BuildResult::ordered_passes`.
    std::string pass_name;

    [[nodiscard]] static FrameGraphExecutionStep make_barrier(FrameGraphBarrier value) {
        FrameGraphExecutionStep step;
        step.kind = Kind::barrier;
        step.barrier = std::move(value);
        return step;
    }

    [[nodiscard]] static FrameGraphExecutionStep make_pass_invoke(std::string name) {
        FrameGraphExecutionStep step;
        step.kind = Kind::pass_invoke;
        step.pass_name = std::move(name);
        return step;
    }
};

struct FrameGraphExecutionCallbackResult {
    bool ok{true};
    std::string message;

    [[nodiscard]] bool succeeded() const noexcept {
        return ok;
    }
};

struct FrameGraphPassExecutionBinding {
    std::string pass_name;
    std::function<FrameGraphExecutionCallbackResult(std::string_view pass_name)> callback;
};

struct FrameGraphExecutionCallbacks {
    std::span<const FrameGraphPassExecutionBinding> pass_callbacks;
    std::function<FrameGraphExecutionCallbackResult(const FrameGraphBarrier& barrier)> barrier_callback;
};

struct FrameGraphExecutionResult {
    std::size_t barrier_callbacks_invoked{0};
    std::size_t pass_callbacks_invoked{0};
    std::vector<FrameGraphDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

/// Builds a linear schedule: for each pass in topological order, emit all barriers targeting that pass
/// (sorted by resource name, then writer pass name) followed by one pass-invoke step.
/// Returns an empty vector when `built` did not succeed.
[[nodiscard]] std::vector<FrameGraphExecutionStep>
schedule_frame_graph_v1_execution(const FrameGraphV1BuildResult& built);

/// Dispatches a linear v1 schedule to caller-owned callbacks. Barrier steps require `barrier_callback`;
/// pass-invoke steps require a unique named binding in `pass_callbacks`. Callback failures and exceptions are
/// converted to deterministic diagnostics and stop execution before later steps.
[[nodiscard]] FrameGraphExecutionResult
execute_frame_graph_v1_schedule(std::span<const FrameGraphExecutionStep> schedule,
                                const FrameGraphExecutionCallbacks& callbacks);

} // namespace mirakana
