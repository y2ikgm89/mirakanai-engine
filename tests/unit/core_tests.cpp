// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

#include "mirakana/animation/chain_ik.hpp"
#include "mirakana/animation/keyframe_animation.hpp"
#include "mirakana/animation/morph.hpp"
#include "mirakana/animation/skeleton.hpp"
#include "mirakana/animation/skin.hpp"
#include "mirakana/animation/state_machine.hpp"
#include "mirakana/animation/timeline.hpp"
#include "mirakana/animation/two_bone_ik.hpp"
#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/assets/asset_import_metadata.hpp"
#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/assets/shader_metadata.hpp"
#include "mirakana/assets/shader_pipeline.hpp"
#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/core/diagnostics.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/core/time.hpp"
#include "mirakana/core/version.hpp"
#include "mirakana/math/mat4.hpp"
#include "mirakana/math/quat.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/math/vec.hpp"
#include "mirakana/physics/physics2d.hpp"
#include "mirakana/physics/physics3d.hpp"
#include "mirakana/platform/clipboard.hpp"
#include "mirakana/platform/cursor.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/platform/file_watcher.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/linux_file_watcher.hpp"
#include "mirakana/platform/macos_file_watcher.hpp"
#include "mirakana/platform/mobile.hpp"
#include "mirakana/platform/process.hpp"
#include "mirakana/platform/window.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/scene/components.hpp"
#include "mirakana/scene/prefab.hpp"
#include "mirakana/scene/prefab_overrides.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/ui/ui.hpp"

#include <cmath>
#include <filesystem>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace {

struct Position {
    float x;
    float y;
};

static_assert(std::is_same_v<decltype(&mirakana::LinuxFileWatcher::active),
                             bool (mirakana::LinuxFileWatcher::*)() const noexcept>);
static_assert(std::is_same_v<decltype(&mirakana::MacOSFileWatcher::active),
                             bool (mirakana::MacOSFileWatcher::*)() const noexcept>);

// Test double: public fields are intentional for assertion in MK_TEST.
// NOLINTBEGIN(misc-non-private-member-variables-in-classes)
struct CountingApp final : public mirakana::GameApp {
    void on_start(mirakana::EngineContext& context) override {
        started = true;
        context.logger.write(
            mirakana::LogRecord{.level = mirakana::LogLevel::info, .category = "test", .message = "start"});
    }

    bool on_update(mirakana::EngineContext& /*context*/, double delta_seconds) override {
        MK_REQUIRE(std::abs(delta_seconds - (1.0 / 60.0)) < 0.0001);
        ++updates;
        return updates < 3;
    }

    void on_stop(mirakana::EngineContext& /*context*/) override {
        stopped = true;
    }

    bool started{false};
    bool stopped{false};
    int updates{0};
};
// NOLINTEND(misc-non-private-member-variables-in-classes)

} // namespace

MK_TEST("engine version exposes initial semantic version") {
    constexpr auto version = mirakana::engine_version();
    MK_REQUIRE(version.major == 0);
    MK_REQUIRE(version.minor == 1);
    MK_REQUIRE(version.patch == 0);
    MK_REQUIRE(version.name == "GameEngine");
}

MK_TEST("fixed timestep accumulates fractional frames") {
    mirakana::FixedTimestep timestep(0.5, 2.0);

    const auto first = timestep.begin_frame(0.25);
    MK_REQUIRE(first.fixed_steps == 0);
    MK_REQUIRE(std::abs(first.interpolation_alpha - 0.5) < 0.0001);

    const auto second = timestep.begin_frame(0.25);
    MK_REQUIRE(second.fixed_steps == 1);
    MK_REQUIRE(std::abs(second.interpolation_alpha) < 0.0001);
}

MK_TEST("fixed timestep clamps large frame deltas") {
    mirakana::FixedTimestep timestep(0.5, 1.0);

    const auto step = timestep.begin_frame(5.0);
    MK_REQUIRE(std::abs(step.frame_delta_seconds - 1.0) < 0.0001);
    MK_REQUIRE(step.fixed_steps == 2);
}

MK_TEST("ring buffer logger keeps newest records") {
    mirakana::RingBufferLogger logger(2);

    logger.log(mirakana::LogLevel::info, "test", "one");
    logger.log(mirakana::LogLevel::warn, "test", "two");
    logger.log(mirakana::LogLevel::error, "test", "three");

    const auto records = logger.records();
    MK_REQUIRE(records.size() == 2);
    MK_REQUIRE(records[0].message == "two");
    MK_REQUIRE(records[1].message == "three");
}

MK_TEST("diagnostics recorder keeps bounded events counters and profile samples") {
    mirakana::DiagnosticsRecorder recorder(2);

    recorder.record_event(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::info,
        .category = "runtime",
        .message = "startup",
        .frame_index = 1,
    });
    recorder.record_event(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::warning,
        .category = "runtime",
        .message = "slow frame",
        .frame_index = 2,
    });
    recorder.record_event(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::error,
        .category = "runtime",
        .message = "missing asset",
        .frame_index = 3,
    });
    recorder.record_counter(mirakana::CounterSample{.name = "entities.active", .value = 40.0, .frame_index = 1});
    recorder.record_counter(mirakana::CounterSample{.name = "entities.visible", .value = 41.0, .frame_index = 2});
    recorder.record_counter(mirakana::CounterSample{.name = "entities.drawn", .value = 42.0, .frame_index = 3});
    recorder.record_profile_sample(
        mirakana::ProfileSample{.name = "frame", .frame_index = 1, .start_time_ns = 0, .duration_ns = 10, .depth = 0});
    recorder.record_profile_sample(mirakana::ProfileSample{
        .name = "game.update", .frame_index = 2, .start_time_ns = 100, .duration_ns = 20, .depth = 0});
    recorder.record_profile_sample(mirakana::ProfileSample{
        .name = "game.render", .frame_index = 3, .start_time_ns = 200, .duration_ns = 30, .depth = 0});

    const auto capture = recorder.snapshot();
    MK_REQUIRE(capture.events.size() == 2);
    MK_REQUIRE(capture.events[0].message == "slow frame");
    MK_REQUIRE(capture.events[1].message == "missing asset");
    MK_REQUIRE(capture.counters.size() == 2);
    MK_REQUIRE(capture.counters[0].name == "entities.visible");
    MK_REQUIRE(capture.counters[1].value == 42.0);
    MK_REQUIRE(capture.profiles.size() == 2);
    MK_REQUIRE(capture.profiles[0].name == "game.update");
    MK_REQUIRE(capture.profiles[0].duration_ns == 20);
    MK_REQUIRE(capture.profiles[1].duration_ns == 30);

    const auto summary = mirakana::summarize_diagnostics(capture);
    MK_REQUIRE(summary.event_count == 2);
    MK_REQUIRE(summary.warning_count == 1);
    MK_REQUIRE(summary.error_count == 1);
    MK_REQUIRE(summary.counter_count == 2);
    MK_REQUIRE(summary.profile_count == 2);
    MK_REQUIRE(summary.total_profile_time_ns == 50);
    MK_REQUIRE(summary.min_profile_time_ns == 20);
    MK_REQUIRE(summary.max_profile_time_ns == 30);
}

MK_TEST("diagnostics recorder rejects invalid labels and non finite counters") {
    mirakana::DiagnosticsRecorder recorder(4);

    recorder.record_event(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::info,
        .category = "",
        .message = "missing category",
        .frame_index = 1,
    });
    recorder.record_counter(mirakana::CounterSample{.name = "", .value = 1.0, .frame_index = 2});
    recorder.record_counter(mirakana::CounterSample{
        .name = "bad.infinity", .value = std::numeric_limits<double>::infinity(), .frame_index = 3});
    recorder.record_profile_sample(
        mirakana::ProfileSample{.name = "", .frame_index = 4, .start_time_ns = 100, .duration_ns = 25, .depth = 0});

    const auto capture = recorder.snapshot();
    MK_REQUIRE(capture.events.size() == 4);
    MK_REQUIRE(capture.counters.empty());
    MK_REQUIRE(capture.profiles.empty());
    for (const auto& event : capture.events) {
        MK_REQUIRE(event.severity == mirakana::DiagnosticSeverity::warning);
        MK_REQUIRE(event.category == "diagnostics");
    }

    const auto summary = mirakana::summarize_diagnostics(capture);
    MK_REQUIRE(summary.warning_count == 4);
    MK_REQUIRE(summary.error_count == 0);
    MK_REQUIRE(summary.counter_count == 0);
    MK_REQUIRE(summary.profile_count == 0);
}

MK_TEST("scoped profile zones use manual clock and preserve nested depth") {
    mirakana::DiagnosticsRecorder recorder(8);
    mirakana::ManualProfileClock clock(100);

    {
        mirakana::ScopedProfileZone frame(recorder, clock, "frame", 9);
        clock.advance(10);
        {
            mirakana::ScopedProfileZone update(recorder, clock, "game.update", 9);
            clock.advance(25);
        }
        clock.advance(15);
    }

    const auto capture = recorder.snapshot();
    MK_REQUIRE(capture.profiles.size() == 2);
    MK_REQUIRE(capture.profiles[0].name == "game.update");
    MK_REQUIRE(capture.profiles[0].frame_index == 9);
    MK_REQUIRE(capture.profiles[0].duration_ns == 25);
    MK_REQUIRE(capture.profiles[0].depth == 1);
    MK_REQUIRE(capture.profiles[1].name == "frame");
    MK_REQUIRE(capture.profiles[1].duration_ns == 50);
    MK_REQUIRE(capture.profiles[1].depth == 0);
}

MK_TEST("diagnostics trace export writes deterministic trace event json") {
    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::warning,
        .category = "runtime",
        .message = "slow frame",
        .frame_index = 2,
    });
    capture.counters.push_back(
        mirakana::CounterSample{.name = "renderer.frames_started", .value = 1.0, .frame_index = 3});
    capture.profiles.push_back(mirakana::ProfileSample{
        .name = "runtime_host.frame", .frame_index = 4, .start_time_ns = 1500, .duration_ns = 2500, .depth = 1});

    mirakana::DiagnosticsTraceExportOptions options;
    options.include_metadata = false;
    options.process_id = 7;
    options.thread_id = 3;

    const auto json = mirakana::export_diagnostics_trace_json(capture, options);
    const std::string expected =
        R"({"traceEvents":[{"name":"slow frame","cat":"runtime","ph":"i","s":"t","ts":2,"pid":7,"tid":3,"args":{"severity":"warning","frame_index":2}},{"name":"renderer.frames_started","cat":"counter","ph":"C","ts":3,"pid":7,"tid":3,"args":{"value":1,"frame_index":3}},{"name":"runtime_host.frame","cat":"profile","ph":"X","ts":1,"pid":7,"tid":3,"dur":2,"args":{"frame_index":4,"depth":1}}]})";

    if (json != expected) {
        throw std::runtime_error(json);
    }
}

MK_TEST("diagnostics trace export escapes strings and skips non finite counters") {
    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::info,
        .category = R"(runtime\io)",
        .message = "quote \"mark\"\nline",
        .frame_index = 5,
    });
    capture.counters.push_back(mirakana::CounterSample{
        .name = "bad.counter", .value = std::numeric_limits<double>::infinity(), .frame_index = 6});
    capture.profiles.push_back(mirakana::ProfileSample{
        .name = R"(frame\scope)", .frame_index = 7, .start_time_ns = 0, .duration_ns = 1000, .depth = 0});

    mirakana::DiagnosticsTraceExportOptions options;
    options.trace_name = "GameEngine \"Trace\"";
    options.thread_name = R"(main\thread)";

    const auto json = mirakana::export_diagnostics_trace_json(capture, options);

    MK_REQUIRE(json.contains(R"("name":"process_name")"));
    MK_REQUIRE(json.contains(R"(GameEngine \"Trace\")"));
    MK_REQUIRE(json.contains(R"(main\\thread)"));
    MK_REQUIRE(json.contains(R"(runtime\\io)"));
    MK_REQUIRE(json.contains(R"(quote \"mark\"\nline)"));
    MK_REQUIRE(json.contains(R"(frame\\scope)"));
    MK_REQUIRE(!json.contains("bad.counter"));
    MK_REQUIRE(!json.contains("Infinity"));
    MK_REQUIRE(!json.contains("NaN"));
    MK_REQUIRE(!json.contains("nan"));
}

MK_TEST("diagnostics trace export handles empty capture nan counters and control characters") {
    mirakana::DiagnosticsTraceExportOptions options;
    options.include_metadata = false;
    MK_REQUIRE(mirakana::export_diagnostics_trace_json(mirakana::DiagnosticCapture{}, options) ==
               R"({"traceEvents":[]})");

    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::debug,
        .category = std::string("control") + static_cast<char>(1),
        .message = std::string("tab\tunit") + static_cast<char>(2),
        .frame_index = 0,
    });
    capture.counters.push_back(mirakana::CounterSample{
        .name = "nan.counter", .value = std::numeric_limits<double>::quiet_NaN(), .frame_index = 0});

    const auto json = mirakana::export_diagnostics_trace_json(capture, options);

    MK_REQUIRE(json.contains(R"(control\u0001)"));
    MK_REQUIRE(json.contains(R"(tab\tunit\u0002)"));
    MK_REQUIRE(!json.contains("nan.counter"));
    MK_REQUIRE(!json.contains("NaN"));
}

MK_TEST("diagnostics trace import review classifies trace event json") {
    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::warning,
        .category = "runtime",
        .message = "slow frame",
        .frame_index = 2,
    });
    capture.counters.push_back(
        mirakana::CounterSample{.name = "renderer.frames_started", .value = 1.0, .frame_index = 3});
    capture.profiles.push_back(mirakana::ProfileSample{
        .name = "runtime_host.frame", .frame_index = 4, .start_time_ns = 1500, .duration_ns = 2500, .depth = 1});

    mirakana::DiagnosticsTraceExportOptions options;
    options.include_metadata = true;
    options.process_id = 7;
    options.thread_id = 3;

    const auto json = mirakana::export_diagnostics_trace_json(capture, options);
    const auto review = mirakana::review_diagnostics_trace_json(json);

    MK_REQUIRE(review.valid);
    MK_REQUIRE(review.payload_bytes == json.size());
    MK_REQUIRE(review.trace_event_count == 5);
    MK_REQUIRE(review.metadata_event_count == 2);
    MK_REQUIRE(review.instant_event_count == 1);
    MK_REQUIRE(review.counter_event_count == 1);
    MK_REQUIRE(review.profile_event_count == 1);
    MK_REQUIRE(review.unknown_event_count == 0);
    MK_REQUIRE(review.diagnostics.empty());

    const auto empty = mirakana::review_diagnostics_trace_json("");
    MK_REQUIRE(!empty.valid);
    MK_REQUIRE(empty.payload_bytes == 0);
    MK_REQUIRE(empty.diagnostics.size() == 1U);
    MK_REQUIRE(empty.diagnostics[0] == "trace import review requires non-empty JSON");

    const auto missing_trace_events = mirakana::review_diagnostics_trace_json(R"({"other":[]})");
    MK_REQUIRE(!missing_trace_events.valid);
    MK_REQUIRE(missing_trace_events.diagnostics.size() == 1U);
    MK_REQUIRE(missing_trace_events.diagnostics[0] == "trace import review requires a traceEvents array");

    const auto malformed = mirakana::review_diagnostics_trace_json(R"({"traceEvents":[)");
    MK_REQUIRE(!malformed.valid);
    MK_REQUIRE(!malformed.diagnostics.empty());
    MK_REQUIRE(malformed.diagnostics[0].contains("malformed trace JSON"));

    const auto non_object_event = mirakana::review_diagnostics_trace_json(R"({"traceEvents":[1]})");
    MK_REQUIRE(!non_object_event.valid);
    MK_REQUIRE(non_object_event.diagnostics.size() == 1U);
    MK_REQUIRE(non_object_event.diagnostics[0] == "traceEvents entries must be objects");
}

MK_TEST("diagnostics trace import reconstructs exported capture subset") {
    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::error,
        .category = "runtime",
        .message = "asset package load failed",
        .frame_index = 11,
    });
    capture.counters.push_back(
        mirakana::CounterSample{.name = "renderer.frames_started", .value = 42.5, .frame_index = 12});
    capture.profiles.push_back(mirakana::ProfileSample{
        .name = "runtime_host.frame", .frame_index = 13, .start_time_ns = 2000, .duration_ns = 5000, .depth = 1});

    mirakana::DiagnosticsTraceExportOptions options;
    options.include_metadata = true;
    options.process_id = 7;
    options.thread_id = 3;

    const auto json = mirakana::export_diagnostics_trace_json(capture, options);
    const auto imported = mirakana::import_diagnostics_trace_json(json);

    MK_REQUIRE(imported.valid);
    MK_REQUIRE(imported.review.valid);
    MK_REQUIRE(imported.review.trace_event_count == 5);
    MK_REQUIRE(imported.review.metadata_event_count == 2);
    MK_REQUIRE(imported.capture.events.size() == 1U);
    MK_REQUIRE(imported.capture.events[0].severity == mirakana::DiagnosticSeverity::error);
    MK_REQUIRE(imported.capture.events[0].category == "runtime");
    MK_REQUIRE(imported.capture.events[0].message == "asset package load failed");
    MK_REQUIRE(imported.capture.events[0].frame_index == 11);
    MK_REQUIRE(imported.capture.counters.size() == 1U);
    MK_REQUIRE(imported.capture.counters[0].name == "renderer.frames_started");
    MK_REQUIRE(imported.capture.counters[0].value == 42.5);
    MK_REQUIRE(imported.capture.counters[0].frame_index == 12);
    MK_REQUIRE(imported.capture.profiles.size() == 1U);
    MK_REQUIRE(imported.capture.profiles[0].name == "runtime_host.frame");
    MK_REQUIRE(imported.capture.profiles[0].frame_index == 13);
    MK_REQUIRE(imported.capture.profiles[0].start_time_ns == 2000);
    MK_REQUIRE(imported.capture.profiles[0].duration_ns == 5000);
    MK_REQUIRE(imported.capture.profiles[0].depth == 1);
    MK_REQUIRE(imported.diagnostics.empty());

    const auto unsupported =
        mirakana::import_diagnostics_trace_json(R"({"traceEvents":[{"name":"async","ph":"B","ts":1}]})");
    MK_REQUIRE(!unsupported.valid);
    MK_REQUIRE(unsupported.review.valid);
    MK_REQUIRE(unsupported.review.unknown_event_count == 1);
    MK_REQUIRE(unsupported.diagnostics.size() == 1U);
    MK_REQUIRE(unsupported.diagnostics[0].contains("cannot be reconstructed"));
}

MK_TEST("diagnostics ops plan reports trace summary and unsupported upload boundaries") {
    mirakana::DiagnosticCapture capture;
    capture.events.push_back(mirakana::DiagnosticEvent{
        .severity = mirakana::DiagnosticSeverity::error,
        .category = "runtime",
        .message = "asset package load failed",
        .frame_index = 11,
    });
    capture.counters.push_back(
        mirakana::CounterSample{.name = "renderer.frames_started", .value = 4.0, .frame_index = 12});
    capture.profiles.push_back(mirakana::ProfileSample{
        .name = "runtime_host.frame", .frame_index = 13, .start_time_ns = 2000, .duration_ns = 5000, .depth = 0});

    mirakana::DiagnosticsOpsPlanOptions options;
    options.host_status.debugging_tools_for_windows_available = false;
    options.host_status.telemetry_backend_configured = false;

    const auto plan = mirakana::build_diagnostics_ops_plan(capture, options);

    const auto find_artifact =
        [&plan](mirakana::DiagnosticsOpsArtifactKind kind) -> const mirakana::DiagnosticsOpsArtifact* {
        for (const auto& artifact : plan.artifacts) {
            if (artifact.kind == kind) {
                return &artifact;
            }
        }
        return nullptr;
    };

    MK_REQUIRE(plan.summary.event_count == 1);
    MK_REQUIRE(plan.summary.error_count == 1);
    MK_REQUIRE(plan.summary.counter_count == 1);
    MK_REQUIRE(plan.summary.profile_count == 1);

    const auto* summary = find_artifact(mirakana::DiagnosticsOpsArtifactKind::summary);
    MK_REQUIRE(summary != nullptr);
    MK_REQUIRE(summary->status == mirakana::DiagnosticsOpsArtifactStatus::ready);
    MK_REQUIRE(summary->producer == "summarize_diagnostics");
    MK_REQUIRE(summary->event_count == 1);
    MK_REQUIRE(summary->counter_count == 1);
    MK_REQUIRE(summary->profile_count == 1);

    const auto* trace = find_artifact(mirakana::DiagnosticsOpsArtifactKind::trace_event_json);
    MK_REQUIRE(trace != nullptr);
    MK_REQUIRE(trace->status == mirakana::DiagnosticsOpsArtifactStatus::ready);
    MK_REQUIRE(trace->producer == "export_diagnostics_trace_json");
    MK_REQUIRE(trace->format == "Chrome Trace Event JSON");
    MK_REQUIRE(trace->profile_count == 1);

    const auto* crash = find_artifact(mirakana::DiagnosticsOpsArtifactKind::crash_dump_review);
    MK_REQUIRE(crash != nullptr);
    MK_REQUIRE(crash->status == mirakana::DiagnosticsOpsArtifactStatus::host_gated);
    MK_REQUIRE(crash->blocker.contains("Debugging Tools for Windows"));

    const auto* telemetry = find_artifact(mirakana::DiagnosticsOpsArtifactKind::telemetry_upload);
    MK_REQUIRE(telemetry != nullptr);
    MK_REQUIRE(telemetry->status == mirakana::DiagnosticsOpsArtifactStatus::unsupported);
    MK_REQUIRE(telemetry->blocker.contains("telemetry backend"));
    MK_REQUIRE(telemetry->producer.empty());
}

MK_TEST("diagnostics ops plan marks host supplied crash and telemetry adapters ready") {
    mirakana::DiagnosticsOpsPlanOptions options;
    options.host_status.debugging_tools_for_windows_available = true;
    options.host_status.telemetry_backend_configured = true;

    const auto plan = mirakana::build_diagnostics_ops_plan(mirakana::DiagnosticCapture{}, options);

    const auto find_artifact =
        [&plan](mirakana::DiagnosticsOpsArtifactKind kind) -> const mirakana::DiagnosticsOpsArtifact* {
        for (const auto& artifact : plan.artifacts) {
            if (artifact.kind == kind) {
                return &artifact;
            }
        }
        return nullptr;
    };

    const auto* crash = find_artifact(mirakana::DiagnosticsOpsArtifactKind::crash_dump_review);
    MK_REQUIRE(crash != nullptr);
    MK_REQUIRE(crash->status == mirakana::DiagnosticsOpsArtifactStatus::ready);
    MK_REQUIRE(crash->producer == "Debugging Tools for Windows");

    const auto* telemetry = find_artifact(mirakana::DiagnosticsOpsArtifactKind::telemetry_upload);
    MK_REQUIRE(telemetry != nullptr);
    MK_REQUIRE(telemetry->status == mirakana::DiagnosticsOpsArtifactStatus::ready);
    MK_REQUIRE(telemetry->producer == "caller-provided telemetry backend");
}

MK_TEST("registry invalidates destroyed entities") {
    mirakana::Registry registry;

    const auto entity = registry.create();
    MK_REQUIRE(registry.alive(entity));
    MK_REQUIRE(registry.living_count() == 1);

    registry.destroy(entity);
    MK_REQUIRE(!registry.alive(entity));
    MK_REQUIRE(registry.living_count() == 0);
}

MK_TEST("registry stores and removes components") {
    mirakana::Registry registry;
    const auto entity = registry.create();

    auto& position = registry.emplace<Position>(entity, 1.0F, 2.0F);
    MK_REQUIRE(position.x == 1.0F);
    MK_REQUIRE(position.y == 2.0F);

    auto* found = registry.try_get<Position>(entity);
    MK_REQUIRE(found != nullptr);
    MK_REQUIRE(found->x == 1.0F);

    MK_REQUIRE(registry.remove<Position>(entity));
    MK_REQUIRE(registry.try_get<Position>(entity) == nullptr);
}

MK_TEST("destroying entity removes its components") {
    mirakana::Registry registry;
    const auto entity = registry.create();

    registry.emplace<Position>(entity, 1.0F, 2.0F);
    registry.destroy(entity);

    MK_REQUIRE(registry.try_get<Position>(entity) == nullptr);
}

MK_TEST("headless runner drives lifecycle until app stops") {
    mirakana::RingBufferLogger logger(8);
    mirakana::Registry registry;
    mirakana::HeadlessRunner runner(logger, registry);
    CountingApp app;

    const auto result = runner.run(app, mirakana::RunConfig{.max_frames = 10, .fixed_delta_seconds = 1.0 / 60.0});

    MK_REQUIRE(result.status == mirakana::RunStatus::stopped_by_app);
    MK_REQUIRE(result.frames_run == 3);
    MK_REQUIRE(app.started);
    MK_REQUIRE(app.stopped);
    MK_REQUIRE(app.updates == 3);
    MK_REQUIRE(logger.records().size() == 1);
}

MK_TEST("vec3 supports dot and cross products") {
    const mirakana::Vec3 x{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    const mirakana::Vec3 y{.x = 0.0F, .y = 1.0F, .z = 0.0F};

    MK_REQUIRE(mirakana::dot(x, y) == 0.0F);
    MK_REQUIRE(mirakana::cross(x, y) == (mirakana::Vec3{0.0F, 0.0F, 1.0F}));
}

MK_TEST("mat4 transforms points with scale then translation") {
    const auto matrix = mirakana::Mat4::translation(mirakana::Vec3{.x = 10.0F, .y = 20.0F, .z = 30.0F}) *
                        mirakana::Mat4::scale(mirakana::Vec3{.x = 2.0F, .y = 3.0F, .z = 4.0F});

    const auto point = mirakana::transform_point(matrix, mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F});

    MK_REQUIRE(point == (mirakana::Vec3{12.0F, 26.0F, 42.0F}));
}

MK_TEST("mat4 rotates points around z axis") {
    const auto point = mirakana::transform_point(mirakana::Mat4::rotation_z(1.57079637F),
                                                 mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});

    MK_REQUIRE(std::abs(point.x) < 0.0001F);
    MK_REQUIRE(std::abs(point.y - 1.0F) < 0.0001F);
    MK_REQUIRE(point.z == 0.0F);
}

MK_TEST("quat rotates vectors and converts to matrices deterministically") {
    const auto near = [](float lhs, float rhs) { return std::abs(lhs - rhs) < 0.0001F; };
    const auto near_vec = [&](mirakana::Vec3 lhs, mirakana::Vec3 rhs) {
        return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
    };

    const auto z_quarter_turn =
        mirakana::Quat::from_axis_angle(mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}, 1.57079637F);
    MK_REQUIRE(mirakana::is_finite_quat(z_quarter_turn));
    MK_REQUIRE(mirakana::is_normalized_quat(z_quarter_turn));
    MK_REQUIRE(
        near_vec(mirakana::rotate(z_quarter_turn, mirakana::Vec3{1.0F, 0.0F, 0.0F}), mirakana::Vec3{0.0F, 1.0F, 0.0F}));

    const auto matrix = mirakana::Mat4::rotation_quat(z_quarter_turn);
    MK_REQUIRE(near_vec(mirakana::transform_direction(matrix, mirakana::Vec3{1.0F, 0.0F, 0.0F}),
                        mirakana::Vec3{0.0F, 1.0F, 0.0F}));

    const auto y_quarter_turn =
        mirakana::Quat::from_axis_angle(mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}, 1.57079637F);
    const auto composed = mirakana::normalize_quat(y_quarter_turn * z_quarter_turn);
    MK_REQUIRE(
        near_vec(mirakana::rotate(composed, mirakana::Vec3{1.0F, 0.0F, 0.0F}), mirakana::Vec3{0.0F, 1.0F, 0.0F}));
    MK_REQUIRE(
        near_vec(mirakana::rotate(composed, mirakana::Vec3{0.0F, 0.0F, 1.0F}), mirakana::Vec3{1.0F, 0.0F, 0.0F}));
}

MK_TEST("2d transform produces 3d-compatible matrix") {
    const mirakana::Transform2D transform{.position = mirakana::Vec2{.x = 5.0F, .y = 6.0F},
                                          .scale = mirakana::Vec2{.x = 2.0F, .y = 3.0F}};

    const auto point = mirakana::transform_point(transform.matrix(), mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 0.0F});

    MK_REQUIRE(point == (mirakana::Vec3{7.0F, 9.0F, 0.0F}));
}

MK_TEST("2d transform rotates between scale and translation") {
    const mirakana::Transform2D transform{.position = mirakana::Vec2{.x = 5.0F, .y = 6.0F},
                                          .scale = mirakana::Vec2{.x = 2.0F, .y = 2.0F},
                                          .rotation_radians = 1.57079637F};

    const auto point = mirakana::transform_point(transform.matrix(), mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});

    MK_REQUIRE(std::abs(point.x - 5.0F) < 0.0001F);
    MK_REQUIRE(std::abs(point.y - 8.0F) < 0.0001F);
    MK_REQUIRE(point.z == 0.0F);
}

MK_TEST("3d transform scales and translates points") {
    const mirakana::Transform3D transform{.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
                                          .scale = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F}};

    const auto point = mirakana::transform_point(transform.matrix(), mirakana::Vec3{.x = 3.0F, .y = 4.0F, .z = 5.0F});

    MK_REQUIRE(point == (mirakana::Vec3{7.0F, 10.0F, 13.0F}));
}

MK_TEST("3d transform rotates around z between scale and translation") {
    const mirakana::Transform3D transform{
        .position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
        .scale = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F},
        .rotation_radians = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.57079637F},
    };

    const auto point = mirakana::transform_point(transform.matrix(), mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});

    MK_REQUIRE(std::abs(point.x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(point.y - 4.0F) < 0.0001F);
    MK_REQUIRE(point.z == 3.0F);
}

MK_TEST("null renderer tracks frame lifecycle") {
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 640, .height = 480});

    MK_REQUIRE(renderer.backend_name() == "null");
    MK_REQUIRE(renderer.backbuffer_extent().width == 640);
    MK_REQUIRE(renderer.backbuffer_extent().height == 480);

    renderer.begin_frame();
    MK_REQUIRE(renderer.frame_active());
    renderer.end_frame();
    MK_REQUIRE(!renderer.frame_active());

    const auto stats = renderer.stats();
    MK_REQUIRE(stats.frames_started == 1);
    MK_REQUIRE(stats.frames_finished == 1);
}

MK_TEST("null renderer stores clear color and resize state") {
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 240});

    renderer.set_clear_color(mirakana::Color{.r = 0.1F, .g = 0.2F, .b = 0.3F, .a = 1.0F});
    renderer.resize(mirakana::Extent2D{.width = 800, .height = 600});

    MK_REQUIRE(renderer.clear_color().r == 0.1F);
    MK_REQUIRE(renderer.clear_color().g == 0.2F);
    MK_REQUIRE(renderer.backbuffer_extent().width == 800);
    MK_REQUIRE(renderer.backbuffer_extent().height == 600);
}

MK_TEST("null renderer counts submitted 2d and 3d draw commands") {
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 240});

    renderer.begin_frame();
    renderer.draw_sprite(mirakana::SpriteCommand{.transform = mirakana::Transform2D{},
                                                 .color = mirakana::Color{.r = 1.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F},
                                                 .texture = {}});
    renderer.draw_mesh(mirakana::MeshCommand{.transform = mirakana::Transform3D{},
                                             .color = mirakana::Color{.r = 0.0F, .g = 1.0F, .b = 0.0F, .a = 1.0F},
                                             .mesh = {},
                                             .material = {},
                                             .world_from_node = mirakana::Mat4::identity(),
                                             .mesh_binding = {},
                                             .material_binding = {},
                                             .gpu_skinning = false,
                                             .skinned_mesh = {},
                                             .gpu_morphing = false,
                                             .morph_mesh = {}});
    renderer.end_frame();

    const auto stats = renderer.stats();
    MK_REQUIRE(stats.sprites_submitted == 1);
    MK_REQUIRE(stats.meshes_submitted == 1);
}

MK_TEST("ui document traverses retained hierarchy deterministically") {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1280.0F, .height = 720.0F};
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc hud;
    hud.id = mirakana::ui::ElementId{"hud"};
    hud.parent = root.id;
    hud.role = mirakana::ui::SemanticRole::panel;
    hud.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1280.0F, .height = 96.0F};
    MK_REQUIRE(document.try_add_element(hud));

    mirakana::ui::ElementDesc score;
    score.id = mirakana::ui::ElementId{"score"};
    score.parent = hud.id;
    score.role = mirakana::ui::SemanticRole::label;
    score.text.label = "Score: 000";
    score.text.localization_key = "hud.score";
    score.bounds = mirakana::ui::Rect{.x = 16.0F, .y = 16.0F, .width = 180.0F, .height = 32.0F};
    MK_REQUIRE(document.try_add_element(score));

    mirakana::ui::ElementDesc pause;
    pause.id = mirakana::ui::ElementId{"pause"};
    pause.parent = hud.id;
    pause.role = mirakana::ui::SemanticRole::button;
    pause.text.label = "Pause";
    pause.bounds = mirakana::ui::Rect{.x = 1120.0F, .y = 16.0F, .width = 120.0F, .height = 32.0F};
    MK_REQUIRE(document.try_add_element(pause));
    MK_REQUIRE(!document.try_add_element(pause));

    const auto traversal = document.traverse();
    MK_REQUIRE(traversal.size() == 4);
    MK_REQUIRE(traversal[0].id == mirakana::ui::ElementId{"root"});
    MK_REQUIRE(traversal[1].id == mirakana::ui::ElementId{"hud"});
    MK_REQUIRE(traversal[2].id == mirakana::ui::ElementId{"score"});
    MK_REQUIRE(traversal[3].id == mirakana::ui::ElementId{"pause"});
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"score"})->parent == mirakana::ui::ElementId{"hud"});

    MK_REQUIRE(document.set_visible(mirakana::ui::ElementId{"score"}, false));
    MK_REQUIRE(!document.find(mirakana::ui::ElementId{"score"})->visible);
    MK_REQUIRE(document.set_enabled(mirakana::ui::ElementId{"pause"}, false));
    MK_REQUIRE(!document.find(mirakana::ui::ElementId{"pause"})->enabled);
}

MK_TEST("ui style text and adapter contracts stay renderer independent") {
    mirakana::ui::Style style;
    style.layout = mirakana::ui::LayoutMode::row;
    style.anchor = mirakana::ui::AnchorMode::top_left;
    style.margin = mirakana::ui::EdgeInsets{.top = 4.0F, .right = 8.0F, .bottom = 4.0F, .left = 8.0F};
    style.padding = mirakana::ui::EdgeInsets{.top = 2.0F, .right = 6.0F, .bottom = 2.0F, .left = 6.0F};
    style.size = mirakana::ui::SizeConstraints{
        .min_width = 64.0F, .min_height = 24.0F, .max_width = 320.0F, .max_height = 48.0F};
    style.dpi_scale = 2.0F;
    style.background_token = "panel.background";
    style.foreground_token = "text.primary";

    MK_REQUIRE(mirakana::ui::is_valid_style(style));
    const auto constrained =
        mirakana::ui::constrain_size(mirakana::ui::Size{.width = 512.0F, .height = 12.0F}, style.size);
    MK_REQUIRE(constrained.width == 320.0F);
    MK_REQUIRE(constrained.height == 24.0F);

    mirakana::ui::Style child_style;
    child_style.anchor = mirakana::ui::AnchorMode::fill;
    child_style.foreground_token = "text.warning";
    const auto resolved = mirakana::ui::resolve_style(style, child_style);
    MK_REQUIRE(resolved.layout == mirakana::ui::LayoutMode::row);
    MK_REQUIRE(resolved.anchor == mirakana::ui::AnchorMode::fill);
    MK_REQUIRE(resolved.background_token == "panel.background");
    MK_REQUIRE(resolved.foreground_token == "text.warning");
    MK_REQUIRE(resolved.dpi_scale == 2.0F);

    mirakana::ui::TextContent text;
    text.label = "Inventory";
    text.localization_key = "menu.inventory";
    text.font_family = "ui/body";
    text.direction = mirakana::ui::TextDirection::automatic;
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    MK_REQUIRE(mirakana::ui::is_valid_text_content(text));

    const auto contracts = mirakana::ui::required_adapter_contracts();
    MK_REQUIRE(contracts.size() >= 11);
    MK_REQUIRE(contracts[0].boundary == mirakana::ui::AdapterBoundary::text_shaping);
    MK_REQUIRE(contracts[1].boundary == mirakana::ui::AdapterBoundary::bidirectional_text);
    MK_REQUIRE(contracts[5].boundary == mirakana::ui::AdapterBoundary::ime_composition);
    MK_REQUIRE(contracts[7].boundary == mirakana::ui::AdapterBoundary::image_decoding);
    MK_REQUIRE(contracts[8].boundary == mirakana::ui::AdapterBoundary::renderer_submission);
    MK_REQUIRE(contracts[9].boundary == mirakana::ui::AdapterBoundary::clipboard);
    MK_REQUIRE(contracts[10].boundary == mirakana::ui::AdapterBoundary::platform_integration);
}

MK_TEST("ui layout solver resolves column rows with padding margins and constraints") {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.style.layout = mirakana::ui::LayoutMode::column;
    root.style.padding = mirakana::ui::EdgeInsets{.top = 10.0F, .right = 12.0F, .bottom = 10.0F, .left = 12.0F};
    root.style.gap = 3.0F;
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc header;
    header.id = mirakana::ui::ElementId{"header"};
    header.parent = root.id;
    header.role = mirakana::ui::SemanticRole::panel;
    header.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 40.0F};
    header.style.size =
        mirakana::ui::SizeConstraints{.min_width = 0.0F, .min_height = 32.0F, .max_width = 0.0F, .max_height = 64.0F};
    MK_REQUIRE(document.try_add_element(header));

    mirakana::ui::ElementDesc content;
    content.id = mirakana::ui::ElementId{"content"};
    content.parent = root.id;
    content.role = mirakana::ui::SemanticRole::panel;
    content.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 80.0F};
    content.style.margin = mirakana::ui::EdgeInsets{.top = 6.0F, .right = 0.0F, .bottom = 0.0F, .left = 0.0F};
    MK_REQUIRE(document.try_add_element(content));

    const auto layout = mirakana::ui::solve_layout(
        document, root.id, mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 320.0F, .height = 200.0F});
    const auto* header_layout = mirakana::ui::find_layout(layout, header.id);
    const auto* content_layout = mirakana::ui::find_layout(layout, content.id);

    MK_REQUIRE(header_layout != nullptr);
    MK_REQUIRE(header_layout->bounds.x == 12.0F);
    MK_REQUIRE(header_layout->bounds.y == 10.0F);
    MK_REQUIRE(header_layout->bounds.width == 296.0F);
    MK_REQUIRE(header_layout->bounds.height == 40.0F);
    MK_REQUIRE(content_layout != nullptr);
    MK_REQUIRE(content_layout->bounds.x == 12.0F);
    MK_REQUIRE(content_layout->bounds.y == 59.0F);
    MK_REQUIRE(content_layout->bounds.width == 296.0F);
    MK_REQUIRE(content_layout->bounds.height == 80.0F);
}

MK_TEST("ui layout solver resolves stack anchors and fill without renderer adapters") {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.style.layout = mirakana::ui::LayoutMode::stack;
    root.style.padding = mirakana::ui::EdgeInsets{.top = 8.0F, .right = 8.0F, .bottom = 8.0F, .left = 8.0F};
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc fill;
    fill.id = mirakana::ui::ElementId{"fill"};
    fill.parent = root.id;
    fill.role = mirakana::ui::SemanticRole::panel;
    fill.style.anchor = mirakana::ui::AnchorMode::fill;
    fill.style.margin = mirakana::ui::EdgeInsets{.top = 2.0F, .right = 4.0F, .bottom = 6.0F, .left = 8.0F};
    MK_REQUIRE(document.try_add_element(fill));

    mirakana::ui::ElementDesc centered;
    centered.id = mirakana::ui::ElementId{"centered"};
    centered.parent = root.id;
    centered.role = mirakana::ui::SemanticRole::button;
    centered.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 80.0F, .height = 30.0F};
    centered.style.anchor = mirakana::ui::AnchorMode::center;
    MK_REQUIRE(document.try_add_element(centered));

    const auto layout = mirakana::ui::solve_layout(
        document, root.id, mirakana::ui::Rect{.x = 10.0F, .y = 20.0F, .width = 200.0F, .height = 100.0F});
    const auto* fill_layout = mirakana::ui::find_layout(layout, fill.id);
    const auto* centered_layout = mirakana::ui::find_layout(layout, centered.id);

    MK_REQUIRE(fill_layout != nullptr);
    MK_REQUIRE(fill_layout->bounds.x == 26.0F);
    MK_REQUIRE(fill_layout->bounds.y == 30.0F);
    MK_REQUIRE(fill_layout->bounds.width == 172.0F);
    MK_REQUIRE(fill_layout->bounds.height == 76.0F);
    MK_REQUIRE(centered_layout != nullptr);
    MK_REQUIRE(centered_layout->bounds.x == 70.0F);
    MK_REQUIRE(centered_layout->bounds.y == 55.0F);
    MK_REQUIRE(centered_layout->bounds.width == 80.0F);
    MK_REQUIRE(centered_layout->bounds.height == 30.0F);
}

MK_TEST("ui renderer submission uses solved layout and accessibility metadata") {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.style.layout = mirakana::ui::LayoutMode::column;
    root.style.padding = mirakana::ui::EdgeInsets{.top = 4.0F, .right = 4.0F, .bottom = 4.0F, .left = 4.0F};
    root.style.background_token = "screen.background";
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc start;
    start.id = mirakana::ui::ElementId{"start"};
    start.parent = root.id;
    start.role = mirakana::ui::SemanticRole::button;
    start.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 120.0F, .height = 32.0F};
    start.text.label = "Start";
    start.text.localization_key = "menu.start";
    start.style.background_token = "button.primary";
    start.style.foreground_token = "text.on_primary";
    start.accessibility_label = "Start game";
    MK_REQUIRE(document.try_add_element(start));

    mirakana::ui::ElementDesc hidden;
    hidden.id = mirakana::ui::ElementId{"hidden"};
    hidden.parent = root.id;
    hidden.role = mirakana::ui::SemanticRole::label;
    hidden.text.label = "Hidden";
    hidden.visible = false;
    MK_REQUIRE(document.try_add_element(hidden));

    const auto layout = mirakana::ui::solve_layout(
        document, root.id, mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 240.0F, .height = 120.0F});
    const auto submission = mirakana::ui::build_renderer_submission(document, layout);

    MK_REQUIRE(submission.elements.size() == 2);
    MK_REQUIRE(submission.layouts.size() == 2);
    MK_REQUIRE(submission.elements[0].id == root.id);
    MK_REQUIRE(submission.elements[1].id == start.id);
    MK_REQUIRE(submission.elements[1].bounds.x == 4.0F);
    MK_REQUIRE(submission.elements[1].bounds.y == 4.0F);
    MK_REQUIRE(submission.elements[1].bounds.width == 120.0F);
    MK_REQUIRE(submission.elements[1].bounds.height == 32.0F);
    MK_REQUIRE(submission.elements[1].style.background_token == "button.primary");
    MK_REQUIRE(submission.elements[1].style.foreground_token == "text.on_primary");

    MK_REQUIRE(submission.boxes.size() == 2);
    MK_REQUIRE(submission.boxes[1].id == start.id);
    MK_REQUIRE(submission.boxes[1].role == mirakana::ui::SemanticRole::button);
    MK_REQUIRE(submission.boxes[1].bounds.width == 120.0F);
    MK_REQUIRE(submission.boxes[1].background_token == "button.primary");
    MK_REQUIRE(submission.boxes[1].foreground_token == "text.on_primary");
    MK_REQUIRE(submission.boxes[1].enabled);

    MK_REQUIRE(submission.text_runs.size() == 1);
    MK_REQUIRE(submission.text_runs[0].id == start.id);
    MK_REQUIRE(submission.text_runs[0].text.label == "Start");
    MK_REQUIRE(submission.text_runs[0].text.localization_key == "menu.start");
    MK_REQUIRE(submission.text_runs[0].bounds.height == 32.0F);
    MK_REQUIRE(submission.text_runs[0].foreground_token == "text.on_primary");

    MK_REQUIRE(submission.accessibility_nodes.size() == 1);
    MK_REQUIRE(submission.accessibility_nodes[0].id == start.id);
    MK_REQUIRE(submission.accessibility_nodes[0].role == mirakana::ui::SemanticRole::button);
    MK_REQUIRE(submission.accessibility_nodes[0].label == "Start game");
    MK_REQUIRE(submission.accessibility_nodes[0].bounds.width == 120.0F);
    MK_REQUIRE(submission.accessibility_nodes[0].localization_key == "menu.start");
    MK_REQUIRE(submission.accessibility_nodes[0].enabled);
    MK_REQUIRE(submission.accessibility_nodes[0].focusable);
}

MK_TEST("ui focus navigation respects visibility enabled state and modal layers") {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 800.0F, .height = 600.0F};
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc play;
    play.id = mirakana::ui::ElementId{"play"};
    play.parent = root.id;
    play.role = mirakana::ui::SemanticRole::button;
    MK_REQUIRE(document.try_add_element(play));

    mirakana::ui::ElementDesc disabled;
    disabled.id = mirakana::ui::ElementId{"disabled"};
    disabled.parent = root.id;
    disabled.role = mirakana::ui::SemanticRole::button;
    disabled.enabled = false;
    MK_REQUIRE(document.try_add_element(disabled));

    mirakana::ui::ElementDesc options;
    options.id = mirakana::ui::ElementId{"options"};
    options.parent = root.id;
    options.role = mirakana::ui::SemanticRole::button;
    MK_REQUIRE(document.try_add_element(options));

    mirakana::ui::ElementDesc modal;
    modal.id = mirakana::ui::ElementId{"modal"};
    modal.parent = root.id;
    modal.role = mirakana::ui::SemanticRole::dialog;
    MK_REQUIRE(document.try_add_element(modal));

    mirakana::ui::ElementDesc confirm;
    confirm.id = mirakana::ui::ElementId{"modal.confirm"};
    confirm.parent = modal.id;
    confirm.role = mirakana::ui::SemanticRole::button;
    MK_REQUIRE(document.try_add_element(confirm));

    mirakana::ui::InteractionState interaction;
    MK_REQUIRE(interaction.set_focus(document, play.id));
    MK_REQUIRE(interaction.focused() == play.id);
    MK_REQUIRE(interaction.route_navigation(document, mirakana::ui::NavigationDirection::next));
    MK_REQUIRE(interaction.focused() == options.id);

    interaction.push_modal_layer(modal.id);
    MK_REQUIRE(!interaction.set_focus(document, play.id));
    MK_REQUIRE(interaction.set_focus(document, confirm.id));
    MK_REQUIRE(interaction.focused() == confirm.id);
    MK_REQUIRE(interaction.modal_layer() == modal.id);
    MK_REQUIRE(interaction.pop_modal_layer());
}

MK_TEST("ui transitions text bindings and command invocation are deterministic") {
    mirakana::ui::TransitionState transition;
    transition.element = mirakana::ui::ElementId{"panel"};
    transition.property = "opacity";
    transition.start_value = 0.0F;
    transition.end_value = 1.0F;
    transition.duration_seconds = 2.0F;

    auto sample = mirakana::ui::advance_transition(transition, 0.5F);
    MK_REQUIRE(sample.value == 0.25F);
    MK_REQUIRE(sample.progress == 0.25F);
    MK_REQUIRE(!sample.finished);

    sample = mirakana::ui::advance_transition(transition, 2.0F);
    MK_REQUIRE(sample.value == 1.0F);
    MK_REQUIRE(sample.progress == 1.0F);
    MK_REQUIRE(sample.finished);

    mirakana::ui::UiDocument document;
    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc label;
    label.id = mirakana::ui::ElementId{"score"};
    label.parent = root.id;
    label.role = mirakana::ui::SemanticRole::label;
    MK_REQUIRE(document.try_add_element(label));

    mirakana::ui::BindingContext binding_context;
    binding_context.set_value("hud.score", "Score: 100");
    MK_REQUIRE(mirakana::ui::apply_text_binding(
        document, mirakana::ui::TextBinding{mirakana::ui::ElementId{"score"}, "hud.score"}, binding_context));
    MK_REQUIRE(document.find(mirakana::ui::ElementId{"score"})->text.label == "Score: 100");

    int invoked = 0;
    mirakana::ui::CommandRegistry commands;
    MK_REQUIRE(commands.try_add(mirakana::ui::CommandBinding{"menu.pause", [&invoked]() { ++invoked; }}));
    MK_REQUIRE(commands.execute("menu.pause"));
    MK_REQUIRE(invoked == 1);
    MK_REQUIRE(!commands.execute("menu.missing"));
}

MK_TEST("virtual input reports pressed down and released states") {
    mirakana::VirtualInput input;

    input.press(mirakana::Key::space);
    MK_REQUIRE(input.key_pressed(mirakana::Key::space));
    MK_REQUIRE(input.key_down(mirakana::Key::space));
    MK_REQUIRE(!input.key_released(mirakana::Key::space));

    input.begin_frame();
    MK_REQUIRE(!input.key_pressed(mirakana::Key::space));
    MK_REQUIRE(input.key_down(mirakana::Key::space));

    input.release(mirakana::Key::space);
    MK_REQUIRE(!input.key_down(mirakana::Key::space));
    MK_REQUIRE(input.key_released(mirakana::Key::space));
}

MK_TEST("virtual input computes digital movement axis") {
    mirakana::VirtualInput input;

    input.press(mirakana::Key::right);
    input.press(mirakana::Key::up);

    const auto axis =
        input.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
    MK_REQUIRE(axis == (mirakana::Vec2{1.0F, 1.0F}));
}

MK_TEST("virtual pointer input tracks press move release and frame deltas") {
    mirakana::VirtualPointerInput input;

    input.press(mirakana::PointerSample{.id = mirakana::primary_pointer_id,
                                        .kind = mirakana::PointerKind::mouse,
                                        .position = mirakana::Vec2{.x = 10.0F, .y = 20.0F}});
    MK_REQUIRE(input.pointer_pressed(mirakana::primary_pointer_id));
    MK_REQUIRE(input.pointer_down(mirakana::primary_pointer_id));
    MK_REQUIRE(!input.pointer_released(mirakana::primary_pointer_id));
    MK_REQUIRE(input.pointer_position(mirakana::primary_pointer_id) == (mirakana::Vec2{10.0F, 20.0F}));

    input.move(mirakana::PointerSample{.id = mirakana::primary_pointer_id,
                                       .kind = mirakana::PointerKind::mouse,
                                       .position = mirakana::Vec2{.x = 16.0F, .y = 18.0F}});
    MK_REQUIRE(input.pointer_delta(mirakana::primary_pointer_id) == (mirakana::Vec2{6.0F, -2.0F}));

    input.begin_frame();
    MK_REQUIRE(!input.pointer_pressed(mirakana::primary_pointer_id));
    MK_REQUIRE(input.pointer_delta(mirakana::primary_pointer_id) == mirakana::Vec2{});
    MK_REQUIRE(input.pointer_down(mirakana::primary_pointer_id));

    input.release(mirakana::primary_pointer_id);
    MK_REQUIRE(!input.pointer_down(mirakana::primary_pointer_id));
    MK_REQUIRE(input.pointer_released(mirakana::primary_pointer_id));

    input.begin_frame();
    MK_REQUIRE(!input.pointer_released(mirakana::primary_pointer_id));
    MK_REQUIRE(input.pointers().empty());
}

MK_TEST("virtual pointer input keeps deterministic pointer order") {
    mirakana::VirtualPointerInput input;

    input.press(mirakana::PointerSample{
        .id = 2, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 20.0F, .y = 10.0F}});
    input.press(mirakana::PointerSample{
        .id = 1, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 10.0F, .y = 20.0F}});

    const auto& pointers = input.pointers();
    MK_REQUIRE(pointers.size() == 2);
    MK_REQUIRE(pointers[0].id == 1);
    MK_REQUIRE(pointers[1].id == 2);
}

MK_TEST("virtual pointer input reports touch gesture centroid delta and pinch scale") {
    mirakana::VirtualPointerInput input;

    input.press(mirakana::PointerSample{
        .id = 1, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    input.press(mirakana::PointerSample{
        .id = 2, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 0.0F, .y = 10.0F}});

    auto gesture = input.touch_gesture();
    MK_REQUIRE(gesture.touch_count == 2);
    MK_REQUIRE(gesture.centroid == (mirakana::Vec2{.x = 0.0F, .y = 5.0F}));
    MK_REQUIRE(gesture.centroid_delta == mirakana::Vec2{});
    MK_REQUIRE(gesture.pinch_scale_available);
    MK_REQUIRE(gesture.pinch_scale == 1.0F);

    input.begin_frame();
    input.move(mirakana::PointerSample{
        .id = 1, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 2.0F, .y = 0.0F}});
    input.move(mirakana::PointerSample{
        .id = 2, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 2.0F, .y = 20.0F}});

    gesture = input.touch_gesture();
    MK_REQUIRE(gesture.touch_count == 2);
    MK_REQUIRE(gesture.centroid == (mirakana::Vec2{.x = 2.0F, .y = 10.0F}));
    MK_REQUIRE(gesture.centroid_delta == (mirakana::Vec2{.x = 2.0F, .y = 5.0F}));
    MK_REQUIRE(gesture.pinch_scale_available);
    MK_REQUIRE(gesture.pinch_scale == 2.0F);
}

MK_TEST("touch gesture recognizer reports tap double tap and long press") {
    const mirakana::TouchGestureRecognizerConfig config{.tap_max_seconds = 0.25F,
                                                        .double_tap_max_seconds = 0.30F,
                                                        .long_press_seconds = 0.50F,
                                                        .tap_slop = 6.0F,
                                                        .pan_start_slop = 8.0F,
                                                        .swipe_min_distance = 40.0F,
                                                        .swipe_min_velocity = 500.0F,
                                                        .pinch_scale_threshold = 0.10F,
                                                        .rotate_radians_threshold = 0.20F};
    mirakana::VirtualPointerInput input;
    mirakana::TouchGestureRecognizer recognizer{config};

    input.press(mirakana::PointerSample{
        .id = 11, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 100.0F, .y = 120.0F}});
    auto events = recognizer.update(input, 0.0F);
    MK_REQUIRE(events.empty());

    input.release(11);
    events = recognizer.update(input, 0.05F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::tap);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::ended);
    MK_REQUIRE(events[0].primary_pointer_id == 11);
    MK_REQUIRE(events[0].centroid == (mirakana::Vec2{.x = 100.0F, .y = 120.0F}));

    input.begin_frame();
    events = recognizer.update(input, 0.10F);
    MK_REQUIRE(events.empty());

    input.press(mirakana::PointerSample{
        .id = 11, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 102.0F, .y = 121.0F}});
    events = recognizer.update(input, 0.0F);
    MK_REQUIRE(events.empty());

    input.release(11);
    events = recognizer.update(input, 0.05F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::double_tap);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::ended);

    input.begin_frame();
    input.press(mirakana::PointerSample{
        .id = 12, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 40.0F, .y = 50.0F}});
    events = recognizer.update(input, 0.0F);
    MK_REQUIRE(events.empty());

    input.begin_frame();
    events = recognizer.update(input, 0.51F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::long_press);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::began);
    MK_REQUIRE(events[0].primary_pointer_id == 12);

    input.release(12);
    events = recognizer.update(input, 0.0F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::long_press);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::ended);
}

MK_TEST("touch gesture recognizer reports pan and release swipe") {
    const mirakana::TouchGestureRecognizerConfig config{.tap_max_seconds = 0.25F,
                                                        .double_tap_max_seconds = 0.30F,
                                                        .long_press_seconds = 0.50F,
                                                        .tap_slop = 6.0F,
                                                        .pan_start_slop = 8.0F,
                                                        .swipe_min_distance = 40.0F,
                                                        .swipe_min_velocity = 500.0F,
                                                        .pinch_scale_threshold = 0.10F,
                                                        .rotate_radians_threshold = 0.20F};
    mirakana::VirtualPointerInput input;
    mirakana::TouchGestureRecognizer recognizer{config};

    input.press(mirakana::PointerSample{
        .id = 21, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    auto events = recognizer.update(input, 0.0F);
    MK_REQUIRE(events.empty());

    input.begin_frame();
    input.move(mirakana::PointerSample{
        .id = 21, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 12.0F, .y = 0.0F}});
    events = recognizer.update(input, 0.016F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::pan);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::began);
    MK_REQUIRE(events[0].delta == (mirakana::Vec2{.x = 12.0F, .y = 0.0F}));

    input.begin_frame();
    input.move(mirakana::PointerSample{
        .id = 21, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 60.0F, .y = 0.0F}});
    events = recognizer.update(input, 0.016F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::pan);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::changed);
    MK_REQUIRE(events[0].delta == (mirakana::Vec2{.x = 48.0F, .y = 0.0F}));

    input.release(21);
    events = recognizer.update(input, 0.016F);
    MK_REQUIRE(events.size() == 2);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::pan);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::ended);
    MK_REQUIRE(events[1].kind == mirakana::TouchGestureKind::swipe);
    MK_REQUIRE(events[1].phase == mirakana::TouchGesturePhase::ended);
    MK_REQUIRE(events[1].velocity.x > config.swipe_min_velocity);
}

MK_TEST("touch gesture recognizer reports pinch rotate and cancel phases") {
    const mirakana::TouchGestureRecognizerConfig config{.tap_max_seconds = 0.25F,
                                                        .double_tap_max_seconds = 0.30F,
                                                        .long_press_seconds = 0.50F,
                                                        .tap_slop = 6.0F,
                                                        .pan_start_slop = 8.0F,
                                                        .swipe_min_distance = 40.0F,
                                                        .swipe_min_velocity = 500.0F,
                                                        .pinch_scale_threshold = 0.10F,
                                                        .rotate_radians_threshold = 0.20F};
    mirakana::VirtualPointerInput input;
    mirakana::TouchGestureRecognizer recognizer{config};

    input.press(mirakana::PointerSample{
        .id = 31, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = -10.0F, .y = 0.0F}});
    input.press(mirakana::PointerSample{
        .id = 32, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 10.0F, .y = 0.0F}});
    auto events = recognizer.update(input, 0.0F);
    MK_REQUIRE(events.empty());

    input.begin_frame();
    input.move(mirakana::PointerSample{
        .id = 31, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = -20.0F, .y = 0.0F}});
    input.move(mirakana::PointerSample{
        .id = 32, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 20.0F, .y = 0.0F}});
    events = recognizer.update(input, 0.016F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::pinch);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::began);
    MK_REQUIRE(events[0].scale == 2.0F);

    input.begin_frame();
    input.move(mirakana::PointerSample{
        .id = 31, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 0.0F, .y = -20.0F}});
    input.move(mirakana::PointerSample{
        .id = 32, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 0.0F, .y = 20.0F}});
    events = recognizer.update(input, 0.016F);
    MK_REQUIRE(events.size() == 2);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::pinch);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::changed);
    MK_REQUIRE(events[1].kind == mirakana::TouchGestureKind::rotate);
    MK_REQUIRE(events[1].phase == mirakana::TouchGesturePhase::began);
    MK_REQUIRE(std::fabs(events[1].rotation_radians - (std::numbers::pi_v<float> * 0.5F)) < 0.001F);

    input.cancel(31);
    events = recognizer.update(input, 0.0F);
    MK_REQUIRE(events.size() == 2);
    MK_REQUIRE(events[0].kind == mirakana::TouchGestureKind::pinch);
    MK_REQUIRE(events[0].phase == mirakana::TouchGesturePhase::canceled);
    MK_REQUIRE(events[1].kind == mirakana::TouchGestureKind::rotate);
    MK_REQUIRE(events[1].phase == mirakana::TouchGesturePhase::canceled);
    MK_REQUIRE(input.pointer_canceled(31));
}

MK_TEST("virtual pointer input ignores invalid pointer samples") {
    mirakana::VirtualPointerInput input;

    input.press(mirakana::PointerSample{
        .id = 0, .kind = mirakana::PointerKind::mouse, .position = mirakana::Vec2{.x = 10.0F, .y = 20.0F}});
    input.press(mirakana::PointerSample{
        .id = 1, .kind = mirakana::PointerKind::unknown, .position = mirakana::Vec2{.x = 10.0F, .y = 20.0F}});
    input.move(mirakana::PointerSample{
        .id = 1, .kind = mirakana::PointerKind::touch, .position = mirakana::Vec2{.x = 30.0F, .y = 40.0F}});

    MK_REQUIRE(input.pointers().empty());
    MK_REQUIRE(!input.pointer_down(0));
    MK_REQUIRE(input.pointer_position(1) == mirakana::Vec2{});
}

MK_TEST("virtual gamepad input tracks buttons axes and frame states") {
    mirakana::VirtualGamepadInput input;

    input.press(7, mirakana::GamepadButton::south);
    MK_REQUIRE(input.gamepad_connected(7));
    MK_REQUIRE(input.button_pressed(7, mirakana::GamepadButton::south));
    MK_REQUIRE(input.button_down(7, mirakana::GamepadButton::south));
    MK_REQUIRE(!input.button_released(7, mirakana::GamepadButton::south));

    input.set_axis(7, mirakana::GamepadAxis::left_x, 1.25F);
    input.set_axis(7, mirakana::GamepadAxis::left_y, -0.5F);
    MK_REQUIRE(input.axis_value(7, mirakana::GamepadAxis::left_x) == 1.0F);
    MK_REQUIRE(input.stick(7, mirakana::GamepadAxis::left_x, mirakana::GamepadAxis::left_y) ==
               (mirakana::Vec2{1.0F, -0.5F}));

    input.begin_frame();
    MK_REQUIRE(!input.button_pressed(7, mirakana::GamepadButton::south));
    MK_REQUIRE(input.button_down(7, mirakana::GamepadButton::south));
    MK_REQUIRE(input.axis_value(7, mirakana::GamepadAxis::left_x) == 1.0F);

    input.release(7, mirakana::GamepadButton::south);
    MK_REQUIRE(!input.button_down(7, mirakana::GamepadButton::south));
    MK_REQUIRE(input.button_released(7, mirakana::GamepadButton::south));

    input.disconnect(7);
    MK_REQUIRE(!input.gamepad_connected(7));
    MK_REQUIRE(input.gamepads().empty());
}

MK_TEST("virtual gamepad input keeps deterministic gamepad order") {
    mirakana::VirtualGamepadInput input;

    input.connect(3);
    input.connect(1);

    const auto& gamepads = input.gamepads();
    MK_REQUIRE(gamepads.size() == 2);
    MK_REQUIRE(gamepads[0].id == 1);
    MK_REQUIRE(gamepads[1].id == 3);
}

MK_TEST("virtual gamepad input ignores invalid gamepad samples") {
    mirakana::VirtualGamepadInput input;

    input.connect(0);
    input.press(1, mirakana::GamepadButton::unknown);
    input.set_axis(2, mirakana::GamepadAxis::unknown, 1.0F);

    MK_REQUIRE(input.gamepads().empty());
    MK_REQUIRE(!input.button_down(1, mirakana::GamepadButton::south));
    MK_REQUIRE(input.axis_value(2, mirakana::GamepadAxis::left_x) == 0.0F);
}

MK_TEST("mobile lifecycle events map into first party lifecycle state") {
    mirakana::VirtualLifecycle lifecycle;

    MK_REQUIRE(mirakana::push_mobile_lifecycle_event(lifecycle, mirakana::MobileLifecycleEventKind::started));
    MK_REQUIRE(!lifecycle.state().backgrounded);
    MK_REQUIRE(!lifecycle.state().interactive);

    MK_REQUIRE(mirakana::push_mobile_lifecycle_event(lifecycle, mirakana::MobileLifecycleEventKind::resumed));
    MK_REQUIRE(lifecycle.state().interactive);

    MK_REQUIRE(mirakana::push_mobile_lifecycle_event(lifecycle, mirakana::MobileLifecycleEventKind::paused));
    MK_REQUIRE(!lifecycle.state().interactive);

    MK_REQUIRE(mirakana::push_mobile_lifecycle_event(lifecycle, mirakana::MobileLifecycleEventKind::stopped));
    MK_REQUIRE(lifecycle.state().backgrounded);

    lifecycle.begin_frame();
    MK_REQUIRE(mirakana::push_mobile_lifecycle_event(lifecycle, mirakana::MobileLifecycleEventKind::low_memory));
    MK_REQUIRE(lifecycle.state().low_memory);

    MK_REQUIRE(mirakana::push_mobile_lifecycle_event(lifecycle, mirakana::MobileLifecycleEventKind::back_requested));
    MK_REQUIRE(lifecycle.state().quit_requested);

    MK_REQUIRE(mirakana::push_mobile_lifecycle_event(lifecycle, mirakana::MobileLifecycleEventKind::destroyed));
    MK_REQUIRE(lifecycle.state().terminating);
}

MK_TEST("mobile viewport safe area orientation and storage contracts validate deterministically") {
    const mirakana::MobileViewportState viewport{
        .pixel_extent = mirakana::WindowExtent{.width = 1080, .height = 2400},
        .safe_area = mirakana::MobileSafeArea{.left = 0.0F, .top = 80.0F, .right = 0.0F, .bottom = 32.0F},
        .orientation = mirakana::MobileOrientation::portrait,
        .pixel_density = 3.0F,
    };

    MK_REQUIRE(mirakana::is_valid_mobile_safe_area(viewport.safe_area));
    MK_REQUIRE(mirakana::is_valid_mobile_viewport_state(viewport));
    const auto landscape_extent = mirakana::oriented_mobile_extent(
        mirakana::WindowExtent{.width = 1080, .height = 2400}, mirakana::MobileOrientation::landscape_left);
    MK_REQUIRE(landscape_extent.width == 2400);
    MK_REQUIRE(landscape_extent.height == 1080);
    MK_REQUIRE(!mirakana::is_valid_mobile_safe_area(mirakana::MobileSafeArea{0.0F, -1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(!mirakana::is_valid_mobile_viewport_state(mirakana::MobileViewportState{
        mirakana::WindowExtent{0, 2400}, viewport.safe_area, viewport.orientation, 3.0F}));

    const mirakana::MobileStorageRoots storage{.save_data = "save", .cache = "cache", .shared = "shared"};
    MK_REQUIRE(mirakana::is_valid_mobile_storage_roots(storage));
    MK_REQUIRE(!mirakana::is_valid_mobile_storage_roots(mirakana::MobileStorageRoots{"", "cache", "shared"}));
}

MK_TEST("mobile touch samples map to virtual pointer samples without native handles") {
    const auto sample = mirakana::map_mobile_touch_sample(
        mirakana::MobileTouchSample{.id = 7,
                                    .phase = mirakana::MobileTouchPhase::pressed,
                                    .position = mirakana::Vec2{.x = 12.0F, .y = 24.0F},
                                    .pressure = 0.5F});

    MK_REQUIRE(sample.has_value());
    MK_REQUIRE(sample->id == 7);
    MK_REQUIRE(sample->kind == mirakana::PointerKind::touch);
    MK_REQUIRE(sample->position == (mirakana::Vec2{12.0F, 24.0F}));
    MK_REQUIRE(!mirakana::map_mobile_touch_sample(mirakana::MobileTouchSample{0, mirakana::MobileTouchPhase::pressed,
                                                                              mirakana::Vec2{12.0F, 24.0F}, 0.5F})
                    .has_value());
}

MK_TEST("mobile permission registry reports missing permissions in stable order") {
    mirakana::MobilePermissionRegistry permissions;
    permissions.set_status(mirakana::MobilePermissionKind::storage, mirakana::MobilePermissionStatus::granted);
    permissions.set_status(mirakana::MobilePermissionKind::microphone, mirakana::MobilePermissionStatus::denied);
    permissions.set_status(mirakana::MobilePermissionKind::notifications,
                           mirakana::MobilePermissionStatus::not_determined);

    MK_REQUIRE(permissions.status(mirakana::MobilePermissionKind::storage) ==
               mirakana::MobilePermissionStatus::granted);
    MK_REQUIRE(permissions.granted(mirakana::MobilePermissionKind::storage));
    MK_REQUIRE(!permissions.granted(mirakana::MobilePermissionKind::microphone));

    const auto missing = permissions.missing_permissions({
        mirakana::MobilePermissionKind::notifications,
        mirakana::MobilePermissionKind::storage,
        mirakana::MobilePermissionKind::microphone,
    });
    MK_REQUIRE(missing.size() == 2);
    MK_REQUIRE(missing[0] == mirakana::MobilePermissionKind::microphone);
    MK_REQUIRE(missing[1] == mirakana::MobilePermissionKind::notifications);
}

MK_TEST("headless window tracks title size and close state") {
    mirakana::HeadlessWindow window(
        mirakana::WindowDesc{.title = "Editor", .extent = mirakana::WindowExtent{.width = 1280, .height = 720}});

    MK_REQUIRE(window.title() == "Editor");
    MK_REQUIRE(window.extent().width == 1280);
    MK_REQUIRE(window.extent().height == 720);
    MK_REQUIRE(window.is_open());

    window.resize(mirakana::WindowExtent{.width = 1920, .height = 1080});
    window.request_close();

    MK_REQUIRE(window.extent().width == 1920);
    MK_REQUIRE(window.extent().height == 1080);
    MK_REQUIRE(!window.is_open());
}

MK_TEST("display info validation accepts finite positive monitor metrics") {
    const mirakana::DisplayInfo display{
        .id = 1,
        .name = "Primary",
        .bounds = mirakana::DisplayRect{.x = -1920, .y = 0, .width = 1920, .height = 1080},
        .usable_bounds = mirakana::DisplayRect{.x = -1920, .y = 40, .width = 1920, .height = 1040},
        .content_scale = 1.25F,
        .primary = true,
    };

    MK_REQUIRE(mirakana::is_valid_display_rect(display.bounds));
    MK_REQUIRE(mirakana::is_valid_display_rect(display.usable_bounds));
    MK_REQUIRE(mirakana::is_valid_display_info(display));

    MK_REQUIRE(!mirakana::is_valid_display_info(mirakana::DisplayInfo{
        0,
        "Invalid",
        mirakana::DisplayRect{0, 0, 1920, 1080},
        mirakana::DisplayRect{0, 0, 1920, 1080},
        1.0F,
        false,
    }));
    MK_REQUIRE(!mirakana::is_valid_display_info(mirakana::DisplayInfo{
        1,
        "Invalid",
        mirakana::DisplayRect{0, 0, 0, 1080},
        mirakana::DisplayRect{0, 0, 1920, 1080},
        1.0F,
        false,
    }));
    MK_REQUIRE(!mirakana::is_valid_display_info(mirakana::DisplayInfo{
        1,
        "Invalid",
        mirakana::DisplayRect{0, 0, 1920, 1080},
        mirakana::DisplayRect{0, 0, 1920, 1080},
        0.0F,
        false,
    }));
}

MK_TEST("window display state validation accepts scale density and safe area") {
    const mirakana::WindowDisplayState state{
        .display_id = 3,
        .content_scale = 1.5F,
        .pixel_density = 2.0F,
        .safe_area = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1280, .height = 720},
    };

    MK_REQUIRE(mirakana::is_valid_window_display_state(state));

    MK_REQUIRE(!mirakana::is_valid_window_display_state(mirakana::WindowDisplayState{
        0,
        1.0F,
        1.0F,
        mirakana::DisplayRect{0, 0, 1280, 720},
    }));
    MK_REQUIRE(!mirakana::is_valid_window_display_state(mirakana::WindowDisplayState{
        1,
        0.0F,
        1.0F,
        mirakana::DisplayRect{0, 0, 1280, 720},
    }));
    MK_REQUIRE(!mirakana::is_valid_window_display_state(mirakana::WindowDisplayState{
        1,
        1.0F,
        -1.0F,
        mirakana::DisplayRect{0, 0, 1280, 720},
    }));
}

MK_TEST("display selection policy chooses primary specific scale and usable area deterministically") {
    const std::vector<mirakana::DisplayInfo> displays{
        mirakana::DisplayInfo{.id = 3,
                              .name = "B",
                              .bounds = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1920, .height = 1080},
                              .usable_bounds = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1600, .height = 900},
                              .content_scale = 1.0F,
                              .primary = false},
        mirakana::DisplayInfo{.id = 2,
                              .name = "A",
                              .bounds = mirakana::DisplayRect{.x = -1920, .y = 0, .width = 1920, .height = 1080},
                              .usable_bounds = mirakana::DisplayRect{.x = -1920, .y = 0, .width = 1920, .height = 1040},
                              .content_scale = 2.0F,
                              .primary = true},
        mirakana::DisplayInfo{.id = 5,
                              .name = "C",
                              .bounds = mirakana::DisplayRect{.x = 1920, .y = 0, .width = 2560, .height = 1440},
                              .usable_bounds = mirakana::DisplayRect{.x = 1920, .y = 0, .width = 2560, .height = 1400},
                              .content_scale = 1.5F,
                              .primary = false},
    };

    auto selected = mirakana::select_display(
        displays, mirakana::DisplaySelectionRequest{.policy = mirakana::DisplaySelectionPolicy::primary});
    MK_REQUIRE(selected.has_value());
    MK_REQUIRE(selected->id == 2);

    selected = mirakana::select_display(
        displays,
        mirakana::DisplaySelectionRequest{.policy = mirakana::DisplaySelectionPolicy::specific, .display_id = 5});
    MK_REQUIRE(selected.has_value());
    MK_REQUIRE(selected->id == 5);

    selected = mirakana::select_display(
        displays, mirakana::DisplaySelectionRequest{.policy = mirakana::DisplaySelectionPolicy::highest_content_scale});
    MK_REQUIRE(selected.has_value());
    MK_REQUIRE(selected->id == 2);

    selected = mirakana::select_display(
        displays, mirakana::DisplaySelectionRequest{.policy = mirakana::DisplaySelectionPolicy::largest_usable_area});
    MK_REQUIRE(selected.has_value());
    MK_REQUIRE(selected->id == 5);
}

MK_TEST("display selection policy rejects invalid and missing displays") {
    const std::vector<mirakana::DisplayInfo> displays{
        mirakana::DisplayInfo{.id = 0,
                              .name = "Invalid",
                              .bounds = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1920, .height = 1080},
                              .usable_bounds = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1920, .height = 1080},
                              .content_scale = 1.0F,
                              .primary = true},
        mirakana::DisplayInfo{.id = 7,
                              .name = "Valid",
                              .bounds = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1280, .height = 720},
                              .usable_bounds = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1280, .height = 720},
                              .content_scale = 1.0F,
                              .primary = false},
    };

    MK_REQUIRE(
        !mirakana::select_display({}, mirakana::DisplaySelectionRequest{mirakana::DisplaySelectionPolicy::primary})
             .has_value());
    MK_REQUIRE(!mirakana::select_display(
                    displays, mirakana::DisplaySelectionRequest{mirakana::DisplaySelectionPolicy::specific, 4})
                    .has_value());

    const auto selected = mirakana::select_display(
        displays, mirakana::DisplaySelectionRequest{.policy = mirakana::DisplaySelectionPolicy::primary});
    MK_REQUIRE(selected.has_value());
    MK_REQUIRE(selected->id == 7);
}

MK_TEST("window placement policy centers and anchors windows on selected displays") {
    const std::vector<mirakana::DisplayInfo> displays{
        mirakana::DisplayInfo{.id = 2,
                              .name = "Primary",
                              .bounds = mirakana::DisplayRect{.x = -1920, .y = 0, .width = 1920, .height = 1080},
                              .usable_bounds =
                                  mirakana::DisplayRect{.x = -1920, .y = 40, .width = 1920, .height = 1040},
                              .content_scale = 1.0F,
                              .primary = true},
    };

    auto placement =
        mirakana::plan_window_placement(displays, mirakana::WindowPlacementRequest{
                                                      .policy = mirakana::WindowPlacementPolicy::centered,
                                                      .display =
                                                          mirakana::DisplaySelectionRequest{
                                                              .policy = mirakana::DisplaySelectionPolicy::specific,
                                                              .display_id = 2,
                                                          },
                                                      .extent = mirakana::WindowExtent{.width = 800, .height = 600},
                                                      .position = mirakana::WindowPosition{},
                                                  });
    MK_REQUIRE(placement.has_value());
    MK_REQUIRE(placement->display_id == 2);
    MK_REQUIRE(placement->position.x == -1360);
    MK_REQUIRE(placement->position.y == 260);
    MK_REQUIRE(placement->extent.width == 800);
    MK_REQUIRE(placement->extent.height == 600);

    placement = mirakana::plan_window_placement(
        displays, mirakana::WindowPlacementRequest{
                      .policy = mirakana::WindowPlacementPolicy::top_left,
                      .display = mirakana::DisplaySelectionRequest{.policy = mirakana::DisplaySelectionPolicy::primary},
                      .extent = mirakana::WindowExtent{.width = 320, .height = 240},
                      .position = mirakana::WindowPosition{},
                  });
    MK_REQUIRE(placement.has_value());
    MK_REQUIRE(placement->position.x == -1920);
    MK_REQUIRE(placement->position.y == 40);
}

MK_TEST("window placement policy supports absolute placement and rejects invalid extents") {
    const std::vector<mirakana::DisplayInfo> displays{
        mirakana::DisplayInfo{.id = 7,
                              .name = "Display",
                              .bounds = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1920, .height = 1080},
                              .usable_bounds = mirakana::DisplayRect{.x = 0, .y = 0, .width = 1920, .height = 1080},
                              .content_scale = 1.0F,
                              .primary = true},
    };

    auto placement = mirakana::plan_window_placement(
        displays, mirakana::WindowPlacementRequest{
                      .policy = mirakana::WindowPlacementPolicy::absolute,
                      .display = mirakana::DisplaySelectionRequest{.policy = mirakana::DisplaySelectionPolicy::primary},
                      .extent = mirakana::WindowExtent{.width = 1024, .height = 768},
                      .position = mirakana::WindowPosition{.x = 100, .y = 200},
                  });
    MK_REQUIRE(placement.has_value());
    MK_REQUIRE(placement->display_id == 7);
    MK_REQUIRE(placement->position.x == 100);
    MK_REQUIRE(placement->position.y == 200);

    MK_REQUIRE(!mirakana::plan_window_placement(
                    displays,
                    mirakana::WindowPlacementRequest{
                        mirakana::WindowPlacementPolicy::centered,
                        mirakana::DisplaySelectionRequest{mirakana::DisplaySelectionPolicy::primary},
                        mirakana::WindowExtent{0, 768},
                        mirakana::WindowPosition{},
                    })
                    .has_value());
}

MK_TEST("headless window tracks position and applies placement") {
    mirakana::HeadlessWindow window(mirakana::WindowDesc{.title = "Editor",
                                                         .extent = mirakana::WindowExtent{.width = 320, .height = 240},
                                                         .position = mirakana::WindowPosition{.x = 10, .y = 20}});

    MK_REQUIRE(window.position().x == 10);
    MK_REQUIRE(window.position().y == 20);

    window.move(mirakana::WindowPosition{.x = -20, .y = 30});
    MK_REQUIRE(window.position().x == -20);
    MK_REQUIRE(window.position().y == 30);

    window.apply_placement(mirakana::WindowPlacement{.position = mirakana::WindowPosition{.x = 100, .y = 200},
                                                     .extent = mirakana::WindowExtent{.width = 800, .height = 600},
                                                     .display_id = 3});
    MK_REQUIRE(window.position().x == 100);
    MK_REQUIRE(window.position().y == 200);
    MK_REQUIRE(window.extent().width == 800);
    MK_REQUIRE(window.extent().height == 600);
}

MK_TEST("file dialog requests validate filters and save semantics") {
    MK_REQUIRE(mirakana::is_valid_file_dialog_filter(mirakana::FileDialogFilter{"Scenes", "scene;prefab"}));
    MK_REQUIRE(mirakana::is_valid_file_dialog_filter(mirakana::FileDialogFilter{"All", "*"}));
    MK_REQUIRE(!mirakana::is_valid_file_dialog_filter(mirakana::FileDialogFilter{"", "png"}));
    MK_REQUIRE(!mirakana::is_valid_file_dialog_filter(mirakana::FileDialogFilter{"Bad", "png jpg"}));
    MK_REQUIRE(!mirakana::is_valid_file_dialog_filter(mirakana::FileDialogFilter{"Bad", ""}));

    mirakana::FileDialogRequest request{
        .kind = mirakana::FileDialogKind::open_file,
        .title = "Open Asset",
        .filters = {mirakana::FileDialogFilter{.name = "Images", .pattern = "png;jpg"}},
        .default_location = "assets",
        .allow_many = true,
        .accept_label = "Open",
        .cancel_label = "Cancel",
    };
    MK_REQUIRE(mirakana::validate_file_dialog_request(request).empty());

    request.kind = mirakana::FileDialogKind::save_file;
    request.allow_many = true;
    MK_REQUIRE(mirakana::validate_file_dialog_request(request) == "save file dialogs cannot allow multiple selections");
}

MK_TEST("memory file dialog service completes scripted async results") {
    mirakana::MemoryFileDialogService dialogs;
    dialogs.enqueue_response(mirakana::MemoryFileDialogResponse{
        .status = mirakana::FileDialogStatus::accepted,
        .paths = {"assets/player.png", "assets/enemy.png"},
        .selected_filter = 0,
        .error = {},
    });

    const auto id = dialogs.show(mirakana::FileDialogRequest{
        .kind = mirakana::FileDialogKind::open_file,
        .title = "Open Textures",
        .filters = {mirakana::FileDialogFilter{.name = "Images", .pattern = "png;jpg"}},
        .default_location = "assets",
        .allow_many = true,
        .accept_label = "Open",
        .cancel_label = "Cancel",
    });

    MK_REQUIRE(dialogs.last_request().has_value());
    MK_REQUIRE(dialogs.last_request()->title == "Open Textures");

    auto result = dialogs.poll_result(id);
    MK_REQUIRE(result.has_value());
    MK_REQUIRE(result->id == id);
    MK_REQUIRE(result->status == mirakana::FileDialogStatus::accepted);
    MK_REQUIRE(result->paths.size() == 2);
    MK_REQUIRE(result->paths[1] == "assets/enemy.png");
    MK_REQUIRE(result->selected_filter == 0);
    MK_REQUIRE(!dialogs.poll_result(id).has_value());
}

MK_TEST("file dialog result queue preserves pending canceled and failed states") {
    mirakana::FileDialogResultQueue queue;
    MK_REQUIRE(!queue.poll(7).has_value());

    queue.push(mirakana::FileDialogResult{
        .id = 7, .status = mirakana::FileDialogStatus::canceled, .paths = {}, .selected_filter = -1, .error = {}});
    queue.push(mirakana::FileDialogResult{.id = 8,
                                          .status = mirakana::FileDialogStatus::failed,
                                          .paths = {},
                                          .selected_filter = -1,
                                          .error = "native dialog failed"});

    auto canceled = queue.poll(7);
    MK_REQUIRE(canceled.has_value());
    MK_REQUIRE(canceled->status == mirakana::FileDialogStatus::canceled);
    MK_REQUIRE(canceled->paths.empty());

    auto failed = queue.poll(8);
    MK_REQUIRE(failed.has_value());
    MK_REQUIRE(failed->status == mirakana::FileDialogStatus::failed);
    MK_REQUIRE(failed->error == "native dialog failed");
}

MK_TEST("memory clipboard stores text and clears deterministically") {
    mirakana::MemoryClipboard storage;
    mirakana::IClipboard& clipboard = storage;

    MK_REQUIRE(!clipboard.has_text());
    MK_REQUIRE(clipboard.text().empty());

    clipboard.set_text("hello clipboard");
    MK_REQUIRE(clipboard.has_text());
    MK_REQUIRE(clipboard.text() == "hello clipboard");

    clipboard.set_text("");
    MK_REQUIRE(!clipboard.has_text());
    MK_REQUIRE(clipboard.text().empty());

    clipboard.set_text("second value");
    clipboard.clear();
    MK_REQUIRE(!clipboard.has_text());
    MK_REQUIRE(clipboard.text().empty());
}

MK_TEST("memory cursor tracks normal hidden confined and relative modes") {
    mirakana::MemoryCursor cursor;

    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::normal);
    MK_REQUIRE(cursor.state().visible);
    MK_REQUIRE(!cursor.state().grabbed);
    MK_REQUIRE(!cursor.state().relative);

    cursor.set_mode(mirakana::CursorMode::hidden);
    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::hidden);
    MK_REQUIRE(!cursor.state().visible);
    MK_REQUIRE(!cursor.state().grabbed);
    MK_REQUIRE(!cursor.state().relative);

    cursor.set_mode(mirakana::CursorMode::confined);
    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::confined);
    MK_REQUIRE(cursor.state().visible);
    MK_REQUIRE(cursor.state().grabbed);
    MK_REQUIRE(!cursor.state().relative);

    cursor.set_mode(mirakana::CursorMode::relative);
    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::relative);
    MK_REQUIRE(!cursor.state().visible);
    MK_REQUIRE(cursor.state().grabbed);
    MK_REQUIRE(cursor.state().relative);

    cursor.set_mode(mirakana::CursorMode::normal);
    MK_REQUIRE(cursor.state().mode == mirakana::CursorMode::normal);
    MK_REQUIRE(cursor.state().visible);
    MK_REQUIRE(!cursor.state().grabbed);
    MK_REQUIRE(!cursor.state().relative);
}

MK_TEST("virtual lifecycle records low memory background and foreground transitions") {
    mirakana::VirtualLifecycle lifecycle;

    MK_REQUIRE(lifecycle.state().interactive);
    MK_REQUIRE(!lifecycle.state().backgrounded);
    MK_REQUIRE(lifecycle.events().empty());

    lifecycle.push(mirakana::LifecycleEventKind::low_memory);
    lifecycle.push(mirakana::LifecycleEventKind::will_enter_background);
    lifecycle.push(mirakana::LifecycleEventKind::did_enter_background);

    MK_REQUIRE(lifecycle.events().size() == 3);
    MK_REQUIRE(lifecycle.events()[0].sequence == 1);
    MK_REQUIRE(lifecycle.events()[0].kind == mirakana::LifecycleEventKind::low_memory);
    MK_REQUIRE(lifecycle.state().low_memory);
    MK_REQUIRE(lifecycle.state().backgrounded);
    MK_REQUIRE(!lifecycle.state().interactive);

    lifecycle.begin_frame();
    MK_REQUIRE(lifecycle.events().empty());
    MK_REQUIRE(!lifecycle.state().low_memory);
    MK_REQUIRE(lifecycle.state().backgrounded);
    MK_REQUIRE(!lifecycle.state().interactive);

    lifecycle.push(mirakana::LifecycleEventKind::will_enter_foreground);
    lifecycle.push(mirakana::LifecycleEventKind::did_enter_foreground);

    MK_REQUIRE(!lifecycle.state().backgrounded);
    MK_REQUIRE(lifecycle.state().interactive);
    MK_REQUIRE(lifecycle.events()[0].sequence == 4);
    MK_REQUIRE(lifecycle.events()[1].sequence == 5);
}

MK_TEST("virtual lifecycle keeps quit and termination requests until reset") {
    mirakana::VirtualLifecycle lifecycle;

    lifecycle.push(mirakana::LifecycleEventKind::quit_requested);
    lifecycle.push(mirakana::LifecycleEventKind::terminating);
    lifecycle.begin_frame();

    MK_REQUIRE(lifecycle.state().quit_requested);
    MK_REQUIRE(lifecycle.state().terminating);

    lifecycle.reset();
    MK_REQUIRE(!lifecycle.state().quit_requested);
    MK_REQUIRE(!lifecycle.state().terminating);
    MK_REQUIRE(lifecycle.state().interactive);
    MK_REQUIRE(lifecycle.events().empty());
}

MK_TEST("memory filesystem stores reads and lists text files") {
    mirakana::MemoryFileSystem fs;

    fs.write_text("assets/player.meta", "kind=texture");
    fs.write_text("assets/enemy.meta", "kind=texture");

    MK_REQUIRE(fs.exists("assets/player.meta"));
    MK_REQUIRE(fs.is_directory("assets"));
    MK_REQUIRE(fs.read_text("assets/player.meta") == "kind=texture");

    const auto files = fs.list_files("assets");
    MK_REQUIRE(files.size() == 2);

    fs.remove("assets/player.meta");
    MK_REQUIRE(!fs.exists("assets/player.meta"));
}

MK_TEST("file watcher backend selection prefers native and falls back to polling") {
    const auto native_choice = mirakana::choose_file_watch_backend(
        mirakana::FileWatchBackendKind::automatic,
        mirakana::FileWatchBackendAvailability{
            .polling = true,
            .native = true,
            .native_backend = mirakana::FileWatchNativeBackendKind::windows_read_directory_changes,
        });
    MK_REQUIRE(native_choice.available);
    MK_REQUIRE(!native_choice.fallback);
    MK_REQUIRE(native_choice.selected == mirakana::FileWatchBackendKind::native);
    MK_REQUIRE(native_choice.native_backend == mirakana::FileWatchNativeBackendKind::windows_read_directory_changes);

    const auto polling_choice = mirakana::choose_file_watch_backend(mirakana::FileWatchBackendKind::automatic,
                                                                    mirakana::FileWatchBackendAvailability{
                                                                        .polling = true,
                                                                        .native = false,
                                                                    });
    MK_REQUIRE(polling_choice.available);
    MK_REQUIRE(!polling_choice.fallback);
    MK_REQUIRE(polling_choice.selected == mirakana::FileWatchBackendKind::polling);

    const auto fallback_choice = mirakana::choose_file_watch_backend(mirakana::FileWatchBackendKind::native,
                                                                     mirakana::FileWatchBackendAvailability{
                                                                         .polling = true,
                                                                         .native = false,
                                                                     });
    MK_REQUIRE(fallback_choice.available);
    MK_REQUIRE(fallback_choice.fallback);
    MK_REQUIRE(fallback_choice.selected == mirakana::FileWatchBackendKind::polling);
    MK_REQUIRE(!fallback_choice.diagnostic.empty());
}

MK_TEST("file watcher backend selection records platform native backend families") {
    const auto linux_choice =
        mirakana::choose_file_watch_backend(mirakana::FileWatchBackendKind::native,
                                            mirakana::FileWatchBackendAvailability{
                                                .polling = true,
                                                .native = true,
                                                .native_backend = mirakana::FileWatchNativeBackendKind::linux_inotify,
                                            });
    MK_REQUIRE(linux_choice.available);
    MK_REQUIRE(linux_choice.selected == mirakana::FileWatchBackendKind::native);
    MK_REQUIRE(linux_choice.native_backend == mirakana::FileWatchNativeBackendKind::linux_inotify);
    MK_REQUIRE(std::string(mirakana::file_watch_native_backend_name(linux_choice.native_backend)) == "linux_inotify");

    const auto macos_choice =
        mirakana::choose_file_watch_backend(mirakana::FileWatchBackendKind::automatic,
                                            mirakana::FileWatchBackendAvailability{
                                                .polling = true,
                                                .native = true,
                                                .native_backend = mirakana::FileWatchNativeBackendKind::macos_fsevents,
                                            });
    MK_REQUIRE(macos_choice.available);
    MK_REQUIRE(macos_choice.selected == mirakana::FileWatchBackendKind::native);
    MK_REQUIRE(macos_choice.native_backend == mirakana::FileWatchNativeBackendKind::macos_fsevents);
    MK_REQUIRE(std::string(mirakana::file_watch_native_backend_name(macos_choice.native_backend)) == "macos_fsevents");

    const auto fallback_choice =
        mirakana::choose_file_watch_backend(mirakana::FileWatchBackendKind::native,
                                            mirakana::FileWatchBackendAvailability{
                                                .polling = true,
                                                .native = false,
                                                .native_backend = mirakana::FileWatchNativeBackendKind::linux_inotify,
                                            });
    MK_REQUIRE(fallback_choice.available);
    MK_REQUIRE(fallback_choice.fallback);
    MK_REQUIRE(fallback_choice.selected == mirakana::FileWatchBackendKind::polling);
    MK_REQUIRE(fallback_choice.native_backend == mirakana::FileWatchNativeBackendKind::unavailable);
}

#if !defined(__linux__)
MK_TEST("linux file watcher reports unavailable outside linux hosts") {
    mirakana::LinuxFileWatcher watcher(mirakana::LinuxFileWatcherDesc{
        .directory = std::filesystem::current_path(),
        .path_prefix = "assets",
        .recursive = true,
    });

    const auto result = watcher.poll();
    MK_REQUIRE(!watcher.active());
    MK_REQUIRE(!result.active);
    MK_REQUIRE(!result.diagnostic.empty());
    MK_REQUIRE(watcher.backend_kind() == mirakana::FileWatchBackendKind::native);
    MK_REQUIRE(watcher.native_backend_kind() == mirakana::FileWatchNativeBackendKind::linux_inotify);
}
#endif

#if !defined(__APPLE__)
MK_TEST("macos file watcher reports unavailable outside macos hosts") {
    mirakana::MacOSFileWatcher watcher(mirakana::MacOSFileWatcherDesc{
        .directory = std::filesystem::current_path(),
        .path_prefix = "assets",
        .recursive = true,
    });

    const auto result = watcher.poll();
    MK_REQUIRE(!watcher.active());
    MK_REQUIRE(!result.active);
    MK_REQUIRE(!result.diagnostic.empty());
    MK_REQUIRE(watcher.backend_kind() == mirakana::FileWatchBackendKind::native);
    MK_REQUIRE(watcher.native_backend_kind() == mirakana::FileWatchNativeBackendKind::macos_fsevents);
}
#endif

MK_TEST("polling file watcher reports added modified and removed files deterministically") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("assets/player.texture", "albedo=red");
    fs.write_text("assets2/ignored.texture", "not under root");

    mirakana::PollingFileWatcher watcher(mirakana::PollingFileWatcherDesc{
        .filesystem = &fs,
        .root = "assets",
    });
    MK_REQUIRE(watcher.backend_kind() == mirakana::FileWatchBackendKind::polling);

    const auto first = watcher.poll();
    MK_REQUIRE(first.events.size() == 1);
    MK_REQUIRE(first.events[0].kind == mirakana::FileWatchEventKind::added);
    MK_REQUIRE(first.events[0].path == "assets/player.texture");
    MK_REQUIRE(first.events[0].previous_revision == 0);
    MK_REQUIRE(first.events[0].current_revision != 0);
    MK_REQUIRE(first.events[0].current_size_bytes == 10);
    MK_REQUIRE(watcher.watched_count() == 1);

    const auto first_revision = first.events[0].current_revision;
    fs.write_text("assets/player.texture", "albedo=blue");
    fs.write_text("assets/materials/player.material", "base_color=blue");

    const auto second = watcher.poll();
    MK_REQUIRE(second.events.size() == 2);
    MK_REQUIRE(second.events[0].kind == mirakana::FileWatchEventKind::added);
    MK_REQUIRE(second.events[0].path == "assets/materials/player.material");
    MK_REQUIRE(second.events[1].kind == mirakana::FileWatchEventKind::modified);
    MK_REQUIRE(second.events[1].path == "assets/player.texture");
    MK_REQUIRE(second.events[1].previous_revision == first_revision);
    MK_REQUIRE(second.events[1].current_revision != first_revision);

    fs.remove("assets/player.texture");
    const auto third = watcher.poll();
    MK_REQUIRE(third.events.size() == 1);
    MK_REQUIRE(third.events[0].kind == mirakana::FileWatchEventKind::removed);
    MK_REQUIRE(third.events[0].path == "assets/player.texture");
    MK_REQUIRE(third.events[0].previous_revision != 0);
    MK_REQUIRE(third.events[0].current_revision == 0);
    MK_REQUIRE(watcher.watched_count() == 1);

    MK_REQUIRE(watcher.poll().events.empty());
}

MK_TEST("rooted filesystem stores reads and lists text files under a root") {
    const auto root = std::filesystem::current_path() / "ge-rooted-filesystem-test";
    std::filesystem::remove_all(root);

    mirakana::RootedFileSystem fs(root);
    fs.write_text("assets/player.meta", "kind=texture");
    fs.write_text("assets/nested/enemy.meta", "kind=mesh");

    MK_REQUIRE(fs.exists("assets/player.meta"));
    MK_REQUIRE(fs.is_directory("assets"));
    MK_REQUIRE(fs.is_directory("assets/nested"));
    MK_REQUIRE(fs.read_text("assets/player.meta") == "kind=texture");

    const auto files = fs.list_files("assets");
    MK_REQUIRE(files.size() == 2);
    MK_REQUIRE(files[0] == "assets/nested/enemy.meta");
    MK_REQUIRE(files[1] == "assets/player.meta");

    std::filesystem::remove_all(root);
}

MK_TEST("rooted filesystem removes files and empty directories under a root") {
    const auto root = std::filesystem::current_path() / "ge-rooted-filesystem-remove-test";
    std::filesystem::remove_all(root);

    mirakana::RootedFileSystem fs(root);
    fs.write_text("assets/nested/player.meta", "kind=texture");

    MK_REQUIRE(fs.exists("assets/nested/player.meta"));
    MK_REQUIRE(fs.is_directory("assets"));
    MK_REQUIRE(fs.is_directory("assets/nested"));

    fs.remove("assets/nested/player.meta");
    MK_REQUIRE(!fs.exists("assets/nested/player.meta"));
    MK_REQUIRE(fs.is_directory("assets/nested"));

    fs.remove_empty_directory("assets/nested");
    MK_REQUIRE(!fs.is_directory("assets/nested"));
    MK_REQUIRE(fs.is_directory("assets"));

    fs.remove_empty_directory("assets");
    MK_REQUIRE(!fs.is_directory("assets"));

    fs.remove_empty_directory("missing");

    std::filesystem::remove_all(root);
}

MK_TEST("rooted filesystem remove empty directory never deletes regular files") {
    const auto root = std::filesystem::current_path() / "ge-rooted-filesystem-remove-file-policy-test";
    std::filesystem::remove_all(root);

    mirakana::RootedFileSystem fs(root);
    fs.write_text("assets", "not a directory");

    bool rejected_regular_file = false;
    try {
        fs.remove_empty_directory("assets");
    } catch (const std::runtime_error&) {
        rejected_regular_file = true;
    }

    MK_REQUIRE(rejected_regular_file);
    MK_REQUIRE(fs.exists("assets"));
    MK_REQUIRE(fs.read_text("assets") == "not a directory");

    std::filesystem::remove_all(root);
}

MK_TEST("rooted filesystem rejects absolute and parent relative paths") {
    const auto root = std::filesystem::current_path() / "ge-rooted-filesystem-policy-test";
    std::filesystem::remove_all(root);
    mirakana::RootedFileSystem fs(root);

    bool rejected_parent = false;
    try {
        fs.write_text("../outside.txt", "nope");
    } catch (const std::invalid_argument&) {
        rejected_parent = true;
    }

    bool rejected_absolute = false;
    try {
        (void)fs.exists((root / "outside.txt").string());
    } catch (const std::invalid_argument&) {
        rejected_absolute = true;
    }

    MK_REQUIRE(rejected_parent);
    MK_REQUIRE(rejected_absolute);
    MK_REQUIRE(!std::filesystem::exists(root / "outside.txt"));

    bool rejected_remove = false;
    try {
        fs.remove("../outside.txt");
    } catch (const std::invalid_argument&) {
        rejected_remove = true;
    }

    bool rejected_remove_directory = false;
    try {
        fs.remove_empty_directory("../outside");
    } catch (const std::invalid_argument&) {
        rejected_remove_directory = true;
    }

    MK_REQUIRE(rejected_remove);
    MK_REQUIRE(rejected_remove_directory);

    std::filesystem::remove_all(root);
}

MK_TEST("recording process runner records shell free commands") {
    mirakana::RecordingProcessRunner runner;
    const mirakana::ProcessCommand command{
        .executable = "shader-tool",
        .arguments = {"--input", "assets/shaders/fullscreen.hlsl", "--output", "out/shaders/fullscreen.dxil"},
        .working_directory = "games/sample",
    };

    const auto result = mirakana::run_process_command(runner, command);

    MK_REQUIRE(result.launched);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.exit_code == 0);
    MK_REQUIRE(result.stdout_text.empty());
    MK_REQUIRE(result.stderr_text.empty());
    MK_REQUIRE(runner.commands().size() == 1);
    MK_REQUIRE(runner.commands()[0].arguments.size() == 4);
}

MK_TEST("process runner rejects unsafe shell commands") {
    mirakana::RecordingProcessRunner runner;
    MK_REQUIRE(!mirakana::is_safe_process_command(mirakana::ProcessCommand{"", {}, {}}));
    MK_REQUIRE(!mirakana::is_safe_process_command(mirakana::ProcessCommand{"cmd.exe", {"/c", "echo unsafe"}, {}}));
    MK_REQUIRE(!mirakana::is_safe_process_command(mirakana::ProcessCommand{"powershell", {"-NoProfile"}, {}}));
    MK_REQUIRE(!mirakana::is_safe_process_command(mirakana::ProcessCommand{"tool", {"bad\narg"}, {}}));

    bool rejected = false;
    try {
        (void)mirakana::run_process_command(runner, mirakana::ProcessCommand{.executable = "cmd.exe",
                                                                             .arguments = {"/c", "echo unsafe"},
                                                                             .working_directory = {}});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
    MK_REQUIRE(runner.commands().empty());
}

MK_TEST("process contract accepts reviewed pwsh validation recipe invocations") {
    mirakana::ProcessCommand good{
        .executable = "pwsh",
        .arguments = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/run-validation-recipe.ps1", "-Mode",
                      "Execute", "-Recipe", "agent-contract"},
        .working_directory = "C:/repo",
    };
    MK_REQUIRE(mirakana::is_safe_reviewed_validation_recipe_invocation(good));
    MK_REQUIRE(!mirakana::is_safe_process_command(good));
    MK_REQUIRE(mirakana::is_allowed_process_command(good));

    mirakana::ProcessCommand wrong_script{
        .executable = "pwsh",
        .arguments = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/other.ps1", "-Mode", "Execute",
                      "-Recipe", "x"},
        .working_directory = {},
    };
    MK_REQUIRE(!mirakana::is_safe_reviewed_validation_recipe_invocation(wrong_script));
}

MK_TEST("process contract accepts reviewed pwsh pix host helper invocations") {
    mirakana::ProcessCommand skip_launch{.executable = "pwsh",
                                         .arguments = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File",
                                                       "tools/launch-pix-host-helper.ps1", "-SkipLaunch"},
                                         .working_directory = "C:/repo"};
    MK_REQUIRE(mirakana::is_safe_reviewed_pix_host_helper_invocation(skip_launch));
    MK_REQUIRE(!mirakana::is_safe_process_command(skip_launch));
    MK_REQUIRE(mirakana::is_allowed_process_command(skip_launch));

    mirakana::ProcessCommand launch_pix{
        .executable = "pwsh",
        .arguments = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/launch-pix-host-helper.ps1"},
        .working_directory = "C:/repo"};
    MK_REQUIRE(mirakana::is_safe_reviewed_pix_host_helper_invocation(launch_pix));
    MK_REQUIRE(!mirakana::is_safe_process_command(launch_pix));
    MK_REQUIRE(mirakana::is_allowed_process_command(launch_pix));

    mirakana::ProcessCommand missing_working_directory{
        .executable = "pwsh",
        .arguments = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "tools/launch-pix-host-helper.ps1",
                      "-SkipLaunch"},
        .working_directory = {},
    };
    MK_REQUIRE(!mirakana::is_safe_reviewed_pix_host_helper_invocation(missing_working_directory));

    mirakana::RecordingProcessRunner runner;
    const auto result = mirakana::run_process_command(runner, skip_launch);
    MK_REQUIRE(result.launched);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(runner.commands().size() == 1);
}

MK_TEST("asset registry stores stable logical asset records") {
    mirakana::AssetRegistry assets;
    const auto id = mirakana::AssetId::from_name("player.sprite");

    assets.add(mirakana::AssetRecord{.id = id, .kind = mirakana::AssetKind::texture, .path = "assets/player.png"});

    const auto* record = assets.find(id);
    MK_REQUIRE(record != nullptr);
    MK_REQUIRE(record->id == id);
    MK_REQUIRE(record->kind == mirakana::AssetKind::texture);
    MK_REQUIRE(record->path == "assets/player.png");
}

MK_TEST("asset registry rejects duplicate asset ids") {
    mirakana::AssetRegistry assets;
    const auto id = mirakana::AssetId::from_name("player.sprite");

    assets.add(mirakana::AssetRecord{.id = id, .kind = mirakana::AssetKind::texture, .path = "assets/player.png"});
    MK_REQUIRE(!assets.try_add(mirakana::AssetRecord{id, mirakana::AssetKind::texture, "assets/player-alt.png"}));
    MK_REQUIRE(assets.count() == 1);
}

MK_TEST("shader metadata registry tracks generated artifacts") {
    mirakana::ShaderMetadataRegistry shaders;
    const auto shader_id = mirakana::AssetId::from_name("shaders/editor-grid.hlsl");

    shaders.add(mirakana::ShaderSourceMetadata{
        .id = shader_id,
        .source_path = "assets/shaders/editor-grid.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::fragment,
        .entry_point = "ps_main",
        .defines = {},
        .artifacts = {},
        .reflection = {},
    });

    MK_REQUIRE(shaders.try_add_artifact(shader_id, mirakana::ShaderGeneratedArtifact{
                                                       .path = "build/shaders/editor-grid.ps.dxil",
                                                       .format = mirakana::ShaderArtifactFormat::dxil,
                                                       .profile = "ps_6_7",
                                                       .entry_point = "ps_main",
                                                   }));

    const auto* metadata = shaders.find(shader_id);
    MK_REQUIRE(metadata != nullptr);
    MK_REQUIRE(metadata->source_path == "assets/shaders/editor-grid.hlsl");
    MK_REQUIRE(metadata->language == mirakana::ShaderSourceLanguage::hlsl);
    MK_REQUIRE(metadata->stage == mirakana::ShaderSourceStage::fragment);
    MK_REQUIRE(metadata->artifacts.size() == 1);
    MK_REQUIRE(metadata->artifacts[0].format == mirakana::ShaderArtifactFormat::dxil);
}

MK_TEST("shader metadata registry rejects invalid records and duplicate artifacts") {
    mirakana::ShaderMetadataRegistry shaders;
    const auto shader_id = mirakana::AssetId::from_name("shaders/triangle.hlsl");

    MK_REQUIRE(!shaders.try_add(mirakana::ShaderSourceMetadata{
        .id = shader_id,
        .source_path = "",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::vertex,
        .entry_point = "vs_main",
        .defines = {},
        .artifacts = {},
        .reflection = {},
    }));

    shaders.add(mirakana::ShaderSourceMetadata{
        .id = shader_id,
        .source_path = "assets/shaders/triangle.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::vertex,
        .entry_point = "vs_main",
        .defines = {},
        .artifacts = {},
        .reflection = {},
    });

    const mirakana::ShaderGeneratedArtifact artifact{
        .path = "build/shaders/triangle.vs.dxil",
        .format = mirakana::ShaderArtifactFormat::dxil,
        .profile = "vs_6_7",
        .entry_point = "vs_main",
    };
    MK_REQUIRE(shaders.try_add_artifact(shader_id, artifact));
    MK_REQUIRE(!shaders.try_add_artifact(shader_id, artifact));
    MK_REQUIRE(!shaders.try_add_artifact(shader_id, mirakana::ShaderGeneratedArtifact{
                                                        .path = "",
                                                        .format = mirakana::ShaderArtifactFormat::dxil,
                                                        .profile = "vs_6_7",
                                                        .entry_point = "vs_main",
                                                    }));
}

MK_TEST("asset import metadata registry tracks texture mesh and material inputs") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto audio_id = mirakana::AssetId::from_name("audio/hit");
    const auto scene_id = mirakana::AssetId::from_name("scenes/level");

    MK_REQUIRE(imports.try_add_texture(mirakana::TextureImportMetadata{
        texture_id,
        "source/textures/player_albedo.png",
        "assets/textures/player_albedo.texture",
        mirakana::TextureColorSpace::srgb,
        true,
        mirakana::TextureCompression::bc7,
    }));
    MK_REQUIRE(imports.try_add_mesh(mirakana::MeshImportMetadata{
        mesh_id,
        "source/meshes/player.fbx",
        "assets/meshes/player.mesh",
        1.0F,
        true,
        true,
    }));
    MK_REQUIRE(imports.try_add_material(mirakana::MaterialImportMetadata{
        material_id,
        "source/materials/player.material.json",
        "assets/materials/player.material",
        {texture_id},
    }));
    MK_REQUIRE(imports.try_add_audio(mirakana::AudioImportMetadata{
        audio_id,
        "source/audio/hit.audio_source",
        "assets/audio/hit.audio",
        false,
    }));
    MK_REQUIRE(imports.try_add_scene(mirakana::SceneImportMetadata{
        scene_id,
        "source/scenes/level.scene",
        "assets/scenes/level.scene",
        {mesh_id},
        {material_id},
        {texture_id},
    }));

    MK_REQUIRE(imports.texture_count() == 1);
    MK_REQUIRE(imports.mesh_count() == 1);
    MK_REQUIRE(imports.material_count() == 1);
    MK_REQUIRE(imports.audio_count() == 1);
    MK_REQUIRE(imports.scene_count() == 1);
    MK_REQUIRE(imports.find_texture(texture_id)->compression == mirakana::TextureCompression::bc7);
    MK_REQUIRE(imports.find_mesh(mesh_id)->generate_collision);
    MK_REQUIRE(imports.find_material(material_id)->texture_dependencies[0] == texture_id);
    MK_REQUIRE(!imports.find_audio(audio_id)->streaming);
    MK_REQUIRE(imports.find_scene(scene_id)->mesh_dependencies[0] == mesh_id);
    MK_REQUIRE(imports.find_scene(scene_id)->material_dependencies[0] == material_id);
    MK_REQUIRE(imports.find_scene(scene_id)->sprite_dependencies[0] == texture_id);
}

MK_TEST("asset import metadata registry rejects invalid records and duplicates") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto texture_id = mirakana::AssetId::from_name("textures/sky");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/sky");
    const auto material_id = mirakana::AssetId::from_name("materials/sky");
    const auto scene_id = mirakana::AssetId::from_name("scenes/sky");

    MK_REQUIRE(!imports.try_add_texture(mirakana::TextureImportMetadata{
        texture_id,
        "",
        "assets/textures/sky.texture",
        mirakana::TextureColorSpace::srgb,
        true,
        mirakana::TextureCompression::bc7,
    }));
    MK_REQUIRE(!imports.try_add_mesh(mirakana::MeshImportMetadata{
        mirakana::AssetId::from_name("meshes/broken"),
        "source/meshes/broken.fbx",
        "assets/meshes/broken.mesh",
        0.0F,
        false,
        false,
    }));
    MK_REQUIRE(!imports.try_add_material(mirakana::MaterialImportMetadata{
        material_id,
        "source/materials/sky.json",
        "assets/materials/sky.material",
        {texture_id, texture_id},
    }));
    MK_REQUIRE(!imports.try_add_audio(mirakana::AudioImportMetadata{
        mirakana::AssetId::from_name("audio/broken"),
        "",
        "assets/audio/broken.audio",
        false,
    }));
    MK_REQUIRE(!imports.try_add_scene(mirakana::SceneImportMetadata{
        scene_id,
        "",
        "assets/scenes/sky.scene",
        {},
        {},
        {},
    }));
    MK_REQUIRE(!imports.try_add_scene(mirakana::SceneImportMetadata{
        scene_id,
        "source/scenes/sky.scene",
        "assets/scenes/sky.scene",
        {mesh_id, mesh_id},
        {},
        {},
    }));
    MK_REQUIRE(!imports.try_add_scene(mirakana::SceneImportMetadata{
        scene_id,
        "source/scenes/sky.scene",
        "assets/scenes/sky.scene",
        {},
        {material_id, material_id},
        {},
    }));
    MK_REQUIRE(!imports.try_add_scene(mirakana::SceneImportMetadata{
        scene_id,
        "source/scenes/sky.scene",
        "assets/scenes/sky.scene",
        {},
        {},
        {texture_id, texture_id},
    }));

    MK_REQUIRE(imports.try_add_texture(mirakana::TextureImportMetadata{
        texture_id,
        "source/textures/sky.hdr",
        "assets/textures/sky.texture",
        mirakana::TextureColorSpace::linear,
        false,
        mirakana::TextureCompression::none,
    }));
    MK_REQUIRE(!imports.try_add_texture(mirakana::TextureImportMetadata{
        texture_id,
        "source/textures/sky-alt.hdr",
        "assets/textures/sky-alt.texture",
        mirakana::TextureColorSpace::linear,
        false,
        mirakana::TextureCompression::none,
    }));
    MK_REQUIRE(imports.try_add_scene(mirakana::SceneImportMetadata{
        scene_id,
        "source/scenes/sky.scene",
        "assets/scenes/sky.scene",
        {},
        {},
        {texture_id},
    }));
    MK_REQUIRE(!imports.try_add_scene(mirakana::SceneImportMetadata{
        scene_id,
        "source/scenes/sky-alt.scene",
        "assets/scenes/sky-alt.scene",
        {},
        {},
        {texture_id},
    }));
}

MK_TEST("asset import pipeline builds audio import actions") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto audio_id = mirakana::AssetId::from_name("audio/hit");
    imports.add_audio(mirakana::AudioImportMetadata{
        .id = audio_id,
        .source_path = "source/audio/hit.audio_source",
        .imported_path = "assets/audio/hit.audio",
        .streaming = true,
    });

    const auto plan = mirakana::build_asset_import_plan(imports);

    MK_REQUIRE(plan.actions.size() == 1);
    MK_REQUIRE(plan.actions[0].id == audio_id);
    MK_REQUIRE(plan.actions[0].kind == mirakana::AssetImportActionKind::audio);
    MK_REQUIRE(plan.actions[0].source_path == "source/audio/hit.audio_source");
    MK_REQUIRE(plan.actions[0].output_path == "assets/audio/hit.audio");
}

MK_TEST("asset import pipeline builds animation float clip import actions") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto clip_id = mirakana::AssetId::from_name("animation/demo-float-clip");
    imports.add_animation_float_clip(mirakana::AnimationFloatClipImportMetadata{
        .id = clip_id,
        .source_path = "source/animation/demo.animation_float_clip_source",
        .imported_path = "runtime/assets/animation/demo.animation_float_clip",
    });

    const auto plan = mirakana::build_asset_import_plan(imports);

    MK_REQUIRE(plan.actions.size() == 1);
    MK_REQUIRE(plan.actions[0].id == clip_id);
    MK_REQUIRE(plan.actions[0].kind == mirakana::AssetImportActionKind::animation_float_clip);
    MK_REQUIRE(plan.actions[0].source_path == "source/animation/demo.animation_float_clip_source");
    MK_REQUIRE(plan.actions[0].output_path == "runtime/assets/animation/demo.animation_float_clip");
}

MK_TEST("asset import pipeline builds deterministic actions and dependency edges") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto scene_id = mirakana::AssetId::from_name("scenes/level");

    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });
    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player_albedo.png",
        .imported_path = "assets/textures/player_albedo.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::bc7,
    });
    imports.add_mesh(mirakana::MeshImportMetadata{
        .id = mesh_id,
        .source_path = "source/meshes/player.gltf",
        .imported_path = "assets/meshes/player.mesh",
        .scale = 1.0F,
        .generate_lods = true,
        .generate_collision = true,
    });
    imports.add_scene(mirakana::SceneImportMetadata{
        .id = scene_id,
        .source_path = "source/scenes/level.scene",
        .imported_path = "assets/scenes/level.scene",
        .mesh_dependencies = {mesh_id},
        .material_dependencies = {material_id},
        .sprite_dependencies = {texture_id},
    });

    const auto plan = mirakana::build_asset_import_plan(imports);

    MK_REQUIRE(plan.actions.size() == 4);
    MK_REQUIRE(plan.actions[0].kind == mirakana::AssetImportActionKind::material);
    MK_REQUIRE(plan.actions[0].output_path == "assets/materials/player.material");
    MK_REQUIRE(plan.actions[0].dependencies.size() == 1);
    MK_REQUIRE(plan.actions[1].kind == mirakana::AssetImportActionKind::mesh);
    MK_REQUIRE(plan.actions[2].kind == mirakana::AssetImportActionKind::scene);
    MK_REQUIRE(plan.actions[2].output_path == "assets/scenes/level.scene");
    MK_REQUIRE(plan.actions[2].dependencies.size() == 3);
    MK_REQUIRE(plan.actions[3].kind == mirakana::AssetImportActionKind::texture);

    auto has_edge = [&plan](mirakana::AssetId asset, mirakana::AssetId dependency, mirakana::AssetDependencyKind kind,
                            std::string_view path) {
        return std::ranges::any_of(plan.dependencies, [&](const auto& edge) {
            return edge.asset == asset && edge.dependency == dependency && edge.kind == kind && edge.path == path;
        });
    };

    MK_REQUIRE(plan.dependencies.size() == 4);
    MK_REQUIRE(has_edge(material_id, texture_id, mirakana::AssetDependencyKind::material_texture,
                        "assets/textures/player_albedo.texture"));
    MK_REQUIRE(has_edge(scene_id, mesh_id, mirakana::AssetDependencyKind::scene_mesh, "assets/meshes/player.mesh"));
    MK_REQUIRE(has_edge(scene_id, material_id, mirakana::AssetDependencyKind::scene_material,
                        "assets/materials/player.material"));
    MK_REQUIRE(has_edge(scene_id, texture_id, mirakana::AssetDependencyKind::scene_sprite,
                        "assets/textures/player_albedo.texture"));
}

MK_TEST("asset import pipeline rejects materials with missing texture dependencies") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto missing_texture_id = mirakana::AssetId::from_name("textures/missing");
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = mirakana::AssetId::from_name("materials/broken"),
        .source_path = "source/materials/broken.material",
        .imported_path = "assets/materials/broken.material",
        .texture_dependencies = {missing_texture_id},
    });

    bool rejected = false;
    try {
        (void)mirakana::build_asset_import_plan(imports);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    MK_REQUIRE(rejected);
}

MK_TEST("asset import pipeline rejects scenes with missing referenced assets") {
    const auto missing_texture_id = mirakana::AssetId::from_name("textures/missing");
    const auto missing_mesh_id = mirakana::AssetId::from_name("meshes/missing");
    const auto missing_material_id = mirakana::AssetId::from_name("materials/missing");

    auto rejected_scene = [](mirakana::SceneImportMetadata scene) {
        mirakana::AssetImportMetadataRegistry imports;
        imports.add_scene(std::move(scene));
        bool rejected = false;
        try {
            (void)mirakana::build_asset_import_plan(imports);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        return rejected;
    };

    MK_REQUIRE(rejected_scene(mirakana::SceneImportMetadata{
        mirakana::AssetId::from_name("scenes/missing-mesh"),
        "source/scenes/missing-mesh.scene",
        "assets/scenes/missing-mesh.scene",
        {missing_mesh_id},
        {},
        {},
    }));
    MK_REQUIRE(rejected_scene(mirakana::SceneImportMetadata{
        mirakana::AssetId::from_name("scenes/missing-material"),
        "source/scenes/missing-material.scene",
        "assets/scenes/missing-material.scene",
        {},
        {missing_material_id},
        {},
    }));
    MK_REQUIRE(rejected_scene(mirakana::SceneImportMetadata{
        mirakana::AssetId::from_name("scenes/missing-sprite"),
        "source/scenes/missing-sprite.scene",
        "assets/scenes/missing-sprite.scene",
        {},
        {},
        {missing_texture_id},
    }));
}

MK_TEST("asset import pipeline builds deterministic recook plans from hot reload requests") {
    mirakana::AssetImportMetadataRegistry imports;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto material_id = mirakana::AssetId::from_name("materials/player");

    imports.add_texture(mirakana::TextureImportMetadata{
        .id = texture_id,
        .source_path = "source/textures/player_albedo.texture_source",
        .imported_path = "assets/textures/player_albedo.texture",
        .color_space = mirakana::TextureColorSpace::srgb,
        .generate_mips = true,
        .compression = mirakana::TextureCompression::bc7,
    });
    imports.add_material(mirakana::MaterialImportMetadata{
        .id = material_id,
        .source_path = "source/materials/player.material",
        .imported_path = "assets/materials/player.material",
        .texture_dependencies = {texture_id},
    });

    const auto import_plan = mirakana::build_asset_import_plan(imports);
    const auto recook_plan = mirakana::build_asset_recook_plan(
        import_plan, {
                         mirakana::AssetHotReloadRecookRequest{
                             .asset = texture_id,
                             .source_asset = texture_id,
                             .trigger_path = "assets/textures/player_albedo.texture",
                             .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
                             .reason = mirakana::AssetHotReloadRecookReason::source_modified,
                             .previous_revision = 1,
                             .current_revision = 2,
                             .ready_tick = 8,
                         },
                         mirakana::AssetHotReloadRecookRequest{
                             .asset = material_id,
                             .source_asset = texture_id,
                             .trigger_path = "assets/textures/player_albedo.texture",
                             .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
                             .reason = mirakana::AssetHotReloadRecookReason::dependency_invalidated,
                             .previous_revision = 1,
                             .current_revision = 2,
                             .ready_tick = 8,
                         },
                         mirakana::AssetHotReloadRecookRequest{
                             .asset = texture_id,
                             .source_asset = texture_id,
                             .trigger_path = "assets/textures/player_albedo.texture",
                             .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
                             .reason = mirakana::AssetHotReloadRecookReason::source_modified,
                             .previous_revision = 1,
                             .current_revision = 2,
                             .ready_tick = 9,
                         },
                     });

    MK_REQUIRE(recook_plan.actions.size() == 2);
    MK_REQUIRE(recook_plan.actions[0].id == material_id);
    MK_REQUIRE(recook_plan.actions[1].id == texture_id);
    MK_REQUIRE(recook_plan.dependencies.size() == 1);
    MK_REQUIRE(recook_plan.dependencies[0].asset == material_id);
    MK_REQUIRE(recook_plan.dependencies[0].dependency == texture_id);
}

MK_TEST("cooked package index records content hashes and dependency edges deterministically") {
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto scene_id = mirakana::AssetId::from_name("scenes/level");

    const std::vector<mirakana::AssetCookedArtifact> artifacts{
        mirakana::AssetCookedArtifact{
            .asset = material_id,
            .kind = mirakana::AssetKind::material,
            .path = "assets/materials/player.material",
            .content = "material-v1",
            .source_revision = 30,
            .dependencies = {texture_id},
        },
        mirakana::AssetCookedArtifact{
            .asset = scene_id,
            .kind = mirakana::AssetKind::scene,
            .path = "assets/scenes/level.scene",
            .content = "scene-v1",
            .source_revision = 40,
            .dependencies = {material_id},
        },
        mirakana::AssetCookedArtifact{
            .asset = texture_id,
            .kind = mirakana::AssetKind::texture,
            .path = "assets/textures/player.texture",
            .content = "texture-v1",
            .source_revision = 20,
            .dependencies = {},
        },
    };
    const std::vector<mirakana::AssetDependencyEdge> edges{
        mirakana::AssetDependencyEdge{
            .asset = material_id,
            .dependency = texture_id,
            .kind = mirakana::AssetDependencyKind::material_texture,
            .path = "assets/textures/player.texture",
        },
        mirakana::AssetDependencyEdge{
            .asset = scene_id,
            .dependency = material_id,
            .kind = mirakana::AssetDependencyKind::scene_material,
            .path = "assets/materials/player.material",
        },
    };

    const auto index = mirakana::build_asset_cooked_package_index(artifacts, edges);

    MK_REQUIRE(index.entries.size() == 3);
    MK_REQUIRE(index.entries[0].asset == material_id);
    MK_REQUIRE(index.entries[0].dependencies.size() == 1);
    MK_REQUIRE(index.entries[0].dependencies[0] == texture_id);
    MK_REQUIRE(index.entries[0].content_hash == mirakana::hash_asset_cooked_content("material-v1"));
    MK_REQUIRE(index.entries[1].asset == scene_id);
    MK_REQUIRE(index.entries[1].kind == mirakana::AssetKind::scene);
    MK_REQUIRE(index.entries[1].dependencies.size() == 1);
    MK_REQUIRE(index.entries[1].dependencies[0] == material_id);
    MK_REQUIRE(index.entries[2].asset == texture_id);
    MK_REQUIRE(index.dependencies.size() == 2);
    MK_REQUIRE(index.dependencies[0].asset == scene_id);
    MK_REQUIRE(index.dependencies[0].kind == mirakana::AssetDependencyKind::scene_material);
    MK_REQUIRE(index.dependencies[1].asset == material_id);
    MK_REQUIRE(index.dependencies[1].kind == mirakana::AssetDependencyKind::material_texture);

    const auto serialized = mirakana::serialize_asset_cooked_package_index(index);
    MK_REQUIRE(serialized.starts_with("format=GameEngine.CookedPackageIndex.v1\n"));
    MK_REQUIRE(serialized.contains("dependency.0.kind=scene_material\n"));
    const auto roundtrip = mirakana::deserialize_asset_cooked_package_index(serialized);
    MK_REQUIRE(roundtrip.entries.size() == 3);
    MK_REQUIRE(roundtrip.entries[0].asset == material_id);
    MK_REQUIRE(roundtrip.entries[0].content_hash == index.entries[0].content_hash);
    MK_REQUIRE(roundtrip.entries[1].asset == scene_id);
    MK_REQUIRE(roundtrip.dependencies.size() == 2);
    MK_REQUIRE(roundtrip.dependencies[0].kind == mirakana::AssetDependencyKind::scene_material);
    MK_REQUIRE(roundtrip.dependencies[1].dependency == texture_id);
}

MK_TEST("cooked package index round trips physics collision scene assets") {
    const auto collision_scene = mirakana::AssetId::from_name("physics/collision/main");
    const std::string payload = "format=GameEngine.PhysicsCollisionScene3D.v1\n"
                                "asset.id=" +
                                std::to_string(collision_scene.value) +
                                "\n"
                                "asset.kind=physics_collision_scene\n";

    const auto index = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{
                .asset = collision_scene,
                .kind = mirakana::AssetKind::physics_collision_scene,
                .path = "runtime/assets/physics/main.collision3d",
                .content = payload,
                .source_revision = 1,
                .dependencies = {},
            },
        },
        {});

    const auto serialized = mirakana::serialize_asset_cooked_package_index(index);
    MK_REQUIRE(serialized.contains("entry.0.kind=physics_collision_scene\n"));

    const auto roundtrip = mirakana::deserialize_asset_cooked_package_index(serialized);
    MK_REQUIRE(roundtrip.entries.size() == 1);
    MK_REQUIRE(roundtrip.entries[0].asset == collision_scene);
    MK_REQUIRE(roundtrip.entries[0].kind == mirakana::AssetKind::physics_collision_scene);
    MK_REQUIRE(roundtrip.entries[0].path == "runtime/assets/physics/main.collision3d");
    MK_REQUIRE(roundtrip.entries[0].content_hash == mirakana::hash_asset_cooked_content(payload));
}

MK_TEST("cooked package index rejects dependency edges outside package entries") {
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto missing_texture_id = mirakana::AssetId::from_name("textures/missing");

    bool rejected_unknown_dependency = false;
    try {
        (void)mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = material_id,
                                              .kind = mirakana::AssetKind::material,
                                              .path = "assets/materials/player.material",
                                              .content = "material-v1",
                                              .source_revision = 30,
                                              .dependencies = {missing_texture_id}},
            },
            {
                mirakana::AssetDependencyEdge{
                    .asset = material_id,
                    .dependency = missing_texture_id,
                    .kind = mirakana::AssetDependencyKind::material_texture,
                    .path = "assets/textures/missing.texture",
                },
            });
    } catch (const std::invalid_argument&) {
        rejected_unknown_dependency = true;
    }
    MK_REQUIRE(rejected_unknown_dependency);

    bool rejected_undeclared_dependency = false;
    try {
        (void)mirakana::build_asset_cooked_package_index(
            {
                mirakana::AssetCookedArtifact{.asset = material_id,
                                              .kind = mirakana::AssetKind::material,
                                              .path = "assets/materials/player.material",
                                              .content = "material-v1",
                                              .source_revision = 30,
                                              .dependencies = {}},
                mirakana::AssetCookedArtifact{.asset = texture_id,
                                              .kind = mirakana::AssetKind::texture,
                                              .path = "assets/textures/player.texture",
                                              .content = "texture-v1",
                                              .source_revision = 20,
                                              .dependencies = {}},
            },
            {
                mirakana::AssetDependencyEdge{
                    .asset = material_id,
                    .dependency = texture_id,
                    .kind = mirakana::AssetDependencyKind::material_texture,
                    .path = "assets/textures/player.texture",
                },
            });
    } catch (const std::invalid_argument&) {
        rejected_undeclared_dependency = true;
    }
    MK_REQUIRE(rejected_undeclared_dependency);
}

MK_TEST("cooked package recook decisions are deterministic for missing stale and dependency changes") {
    const auto audio_id = mirakana::AssetId::from_name("audio/hit");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");
    const auto shader_id = mirakana::AssetId::from_name("shaders/fullscreen");
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");

    const auto previous = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = audio_id,
                                          .kind = mirakana::AssetKind::audio,
                                          .path = "assets/audio/hit.audio",
                                          .content = "audio-v1",
                                          .source_revision = 5,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{
                .asset = material_id,
                .kind = mirakana::AssetKind::material,
                .path = "assets/materials/player.material",
                .content = "material-v1",
                .source_revision = 30,
                .dependencies = {texture_id},
            },
            mirakana::AssetCookedArtifact{.asset = mesh_id,
                                          .kind = mirakana::AssetKind::mesh,
                                          .path = "assets/meshes/player.mesh",
                                          .content = "mesh-v1",
                                          .source_revision = 7,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{
                .asset = texture_id,
                .kind = mirakana::AssetKind::texture,
                .path = "assets/textures/player.texture",
                .content = "texture-v1",
                .source_revision = 20,
                .dependencies = {},
            },
        },
        {
            mirakana::AssetDependencyEdge{
                .asset = material_id,
                .dependency = texture_id,
                .kind = mirakana::AssetDependencyKind::material_texture,
                .path = "assets/textures/player.texture",
            },
        });

    const auto current = mirakana::build_asset_cooked_package_index(
        {
            mirakana::AssetCookedArtifact{.asset = audio_id,
                                          .kind = mirakana::AssetKind::audio,
                                          .path = "assets/audio/hit.audio",
                                          .content = "audio-v1",
                                          .source_revision = 6,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{
                .asset = material_id,
                .kind = mirakana::AssetKind::material,
                .path = "assets/materials/player.material",
                .content = "material-v1",
                .source_revision = 30,
                .dependencies = {},
            },
            mirakana::AssetCookedArtifact{.asset = mesh_id,
                                          .kind = mirakana::AssetKind::mesh,
                                          .path = "assets/meshes/player.mesh",
                                          .content = "mesh-v2",
                                          .source_revision = 7,
                                          .dependencies = {}},
            mirakana::AssetCookedArtifact{
                .asset = shader_id,
                .kind = mirakana::AssetKind::shader,
                .path = "assets/shaders/fullscreen.shader",
                .content = "shader-v1",
                .source_revision = 1,
                .dependencies = {},
            },
            mirakana::AssetCookedArtifact{
                .asset = texture_id,
                .kind = mirakana::AssetKind::texture,
                .path = "assets/textures/player.texture",
                .content = "texture-v1",
                .source_revision = 20,
                .dependencies = {},
            },
        },
        {});

    const auto decisions = mirakana::build_asset_cooked_package_recook_decisions(previous, current);

    MK_REQUIRE(decisions.size() == 5);
    MK_REQUIRE(decisions[0].asset == audio_id);
    MK_REQUIRE(decisions[0].kind == mirakana::AssetCookedPackageRecookDecisionKind::source_revision_changed);
    MK_REQUIRE(decisions[1].asset == material_id);
    MK_REQUIRE(decisions[1].kind == mirakana::AssetCookedPackageRecookDecisionKind::dependencies_changed);
    MK_REQUIRE(decisions[2].asset == mesh_id);
    MK_REQUIRE(decisions[2].kind == mirakana::AssetCookedPackageRecookDecisionKind::content_hash_changed);
    MK_REQUIRE(decisions[3].asset == shader_id);
    MK_REQUIRE(decisions[3].kind == mirakana::AssetCookedPackageRecookDecisionKind::missing_from_previous_index);
    MK_REQUIRE(decisions[4].asset == texture_id);
    MK_REQUIRE(decisions[4].kind == mirakana::AssetCookedPackageRecookDecisionKind::up_to_date);
}

MK_TEST("texture source document parses deterministic metadata") {
    const auto texture = mirakana::deserialize_texture_source_document("format=GameEngine.TextureSource.v1\n"
                                                                       "texture.width=4\n"
                                                                       "texture.height=2\n"
                                                                       "texture.pixel_format=rgba8_unorm\n");

    MK_REQUIRE(texture.width == 4);
    MK_REQUIRE(texture.height == 2);
    MK_REQUIRE(texture.pixel_format == mirakana::TextureSourcePixelFormat::rgba8_unorm);
    MK_REQUIRE(mirakana::texture_source_uncompressed_bytes(texture) == 32);

    const auto text = mirakana::serialize_texture_source_document(texture);
    MK_REQUIRE(text.starts_with("format=GameEngine.TextureSource.v1\n"));
    MK_REQUIRE(text.contains("texture.pixel_format=rgba8_unorm\n"));
}

MK_TEST("mesh source document parses deterministic metadata") {
    const auto mesh = mirakana::deserialize_mesh_source_document("format=GameEngine.MeshSource.v2\n"
                                                                 "mesh.vertex_count=24\n"
                                                                 "mesh.index_count=36\n"
                                                                 "mesh.has_normals=true\n"
                                                                 "mesh.has_uvs=true\n"
                                                                 "mesh.has_tangent_frame=true\n");

    MK_REQUIRE(mesh.vertex_count == 24);
    MK_REQUIRE(mesh.index_count == 36);
    MK_REQUIRE(mesh.has_normals);
    MK_REQUIRE(mesh.has_uvs);
    MK_REQUIRE(mesh.has_tangent_frame);

    const auto text = mirakana::serialize_mesh_source_document(mesh);
    MK_REQUIRE(text.starts_with("format=GameEngine.MeshSource.v2\n"));
    MK_REQUIRE(text.contains("mesh.index_count=36\n"));
}

MK_TEST("morph mesh CPU source document serializes round-trip") {
    mirakana::MorphMeshCpuSourceDocument document;
    document.vertex_count = 3;
    document.bind_position_bytes.assign(36U, 0U);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    target.position_delta_bytes.assign(36U, 0U);
    document.targets.push_back(target);
    document.target_weight_bytes.resize(4U);
    const float weight = 0.25F;
    std::uint32_t bits = 0U;
    std::memcpy(&bits, &weight, sizeof(float));
    document.target_weight_bytes.clear();
    document.target_weight_bytes.push_back(static_cast<std::uint8_t>(bits & 0xFFU));
    document.target_weight_bytes.push_back(static_cast<std::uint8_t>((bits >> 8U) & 0xFFU));
    document.target_weight_bytes.push_back(static_cast<std::uint8_t>((bits >> 16U) & 0xFFU));
    document.target_weight_bytes.push_back(static_cast<std::uint8_t>((bits >> 24U) & 0xFFU));
    MK_REQUIRE(mirakana::is_valid_morph_mesh_cpu_source_document(document));
    const auto text = mirakana::serialize_morph_mesh_cpu_source_document(document);
    const auto round_trip = mirakana::deserialize_morph_mesh_cpu_source_document(text);
    MK_REQUIRE(round_trip.vertex_count == 3);
    MK_REQUIRE(round_trip.targets.size() == 1);
    MK_REQUIRE(round_trip.bind_position_bytes == document.bind_position_bytes);
    MK_REQUIRE(round_trip.targets[0].position_delta_bytes == document.targets[0].position_delta_bytes);
    MK_REQUIRE(round_trip.target_weight_bytes == document.target_weight_bytes);
}

MK_TEST("animation float clip source document serializes round-trip") {
    mirakana::AnimationFloatClipSourceDocument document;
    mirakana::AnimationFloatClipTrackSourceDocument track;
    track.target = "weights/0";
    std::vector<std::uint8_t> times(8U);
    std::vector<std::uint8_t> values(8U);
    const float t0 = 0.0F;
    const float t1 = 1.0F;
    const float v0 = 0.25F;
    const float v1 = 0.75F;
    std::memcpy(times.data(), &t0, sizeof(float));
    std::memcpy(&times[4U], &t1, sizeof(float));
    std::memcpy(values.data(), &v0, sizeof(float));
    std::memcpy(&values[4U], &v1, sizeof(float));
    track.time_seconds_bytes = times;
    track.value_bytes = values;
    document.tracks.push_back(std::move(track));
    MK_REQUIRE(mirakana::is_valid_animation_float_clip_source_document(document));
    const auto text = mirakana::serialize_animation_float_clip_source_document(document);
    const auto round_trip = mirakana::deserialize_animation_float_clip_source_document(text);
    MK_REQUIRE(round_trip.tracks.size() == 1);
    MK_REQUIRE(round_trip.tracks[0].target == "weights/0");
    MK_REQUIRE(round_trip.tracks[0].time_seconds_bytes.size() == 8U);
    MK_REQUIRE(round_trip.tracks[0].value_bytes.size() == 8U);
}

MK_TEST("animation transform binding source document serializes round-trip") {
    mirakana::AnimationTransformBindingSourceDocument document;
    document.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "gltf/node/1/translation/x",
        .node_name = "animated_node",
        .component = mirakana::AnimationTransformBindingComponent::translation_x,
    });
    document.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "gltf/node/1/rotation_z",
        .node_name = "animated_node",
        .component = mirakana::AnimationTransformBindingComponent::rotation_z,
    });

    MK_REQUIRE(mirakana::is_valid_animation_transform_binding_source_document(document));
    const auto text = mirakana::serialize_animation_transform_binding_source_document(document);
    MK_REQUIRE(text.starts_with("format=GameEngine.AnimationTransformBindingSource.v1\n"));
    MK_REQUIRE(text.contains("binding.count=2\n"));
    MK_REQUIRE(text.contains("binding.0.component=translation_x\n"));
    MK_REQUIRE(text.contains("binding.1.component=rotation_z\n"));

    const auto round_trip = mirakana::deserialize_animation_transform_binding_source_document(text);
    MK_REQUIRE(round_trip.bindings.size() == 2);
    MK_REQUIRE(round_trip.bindings[0].target == "gltf/node/1/translation/x");
    MK_REQUIRE(round_trip.bindings[0].node_name == "animated_node");
    MK_REQUIRE(round_trip.bindings[0].component == mirakana::AnimationTransformBindingComponent::translation_x);
    MK_REQUIRE(round_trip.bindings[1].target == "gltf/node/1/rotation_z");
    MK_REQUIRE(round_trip.bindings[1].component == mirakana::AnimationTransformBindingComponent::rotation_z);
}

MK_TEST("animation transform binding source document rejects duplicate and invalid rows") {
    mirakana::AnimationTransformBindingSourceDocument duplicate_target;
    duplicate_target.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "gltf/node/1/translation/x",
        .node_name = "animated_node",
        .component = mirakana::AnimationTransformBindingComponent::translation_x,
    });
    duplicate_target.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "gltf/node/1/translation/x",
        .node_name = "other_node",
        .component = mirakana::AnimationTransformBindingComponent::translation_x,
    });
    MK_REQUIRE(!mirakana::is_valid_animation_transform_binding_source_document(duplicate_target));

    mirakana::AnimationTransformBindingSourceDocument duplicate_component;
    duplicate_component.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "curve/a",
        .node_name = "animated_node",
        .component = mirakana::AnimationTransformBindingComponent::rotation_z,
    });
    duplicate_component.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "curve/b",
        .node_name = "animated_node",
        .component = mirakana::AnimationTransformBindingComponent::rotation_z,
    });
    MK_REQUIRE(!mirakana::is_valid_animation_transform_binding_source_document(duplicate_component));

    mirakana::AnimationTransformBindingSourceDocument invalid_token;
    invalid_token.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "bad\ntarget",
        .node_name = "animated_node",
        .component = mirakana::AnimationTransformBindingComponent::translation_x,
    });
    MK_REQUIRE(!mirakana::is_valid_animation_transform_binding_source_document(invalid_token));

    mirakana::AnimationTransformBindingSourceDocument invalid_component;
    invalid_component.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "curve/a",
        .node_name = "animated_node",
        .component = mirakana::AnimationTransformBindingComponent::unknown,
    });
    MK_REQUIRE(!mirakana::is_valid_animation_transform_binding_source_document(invalid_component));

    bool threw = false;
    try {
        (void)mirakana::deserialize_animation_transform_binding_source_document(
            "format=GameEngine.AnimationTransformBindingSource.v1\n"
            "binding.count=1\n"
            "binding.0.target=curve/a\n"
            "binding.0.node_name=animated_node\n"
            "binding.0.component=bad_component\n");
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    MK_REQUIRE(threw);
}

MK_TEST("audio source document parses deterministic metadata") {
    const auto audio = mirakana::deserialize_audio_source_document("format=GameEngine.AudioSource.v1\n"
                                                                   "audio.sample_rate=48000\n"
                                                                   "audio.channel_count=2\n"
                                                                   "audio.frame_count=24000\n"
                                                                   "audio.sample_format=pcm16\n");

    MK_REQUIRE(audio.sample_rate == 48000);
    MK_REQUIRE(audio.channel_count == 2);
    MK_REQUIRE(audio.frame_count == 24000);
    MK_REQUIRE(audio.sample_format == mirakana::AudioSourceSampleFormat::pcm16);
    MK_REQUIRE(mirakana::audio_source_uncompressed_bytes(audio) == 96000);

    const auto text = mirakana::serialize_audio_source_document(audio);
    MK_REQUIRE(text.starts_with("format=GameEngine.AudioSource.v1\n"));
    MK_REQUIRE(text.contains("audio.sample_format=pcm16\n"));
}

MK_TEST("asset source documents parse deterministic byte payloads") {
    const auto texture = mirakana::deserialize_texture_source_document("format=GameEngine.TextureSource.v1\n"
                                                                       "texture.width=2\n"
                                                                       "texture.height=1\n"
                                                                       "texture.pixel_format=rgba8_unorm\n"
                                                                       "texture.data_hex=0001020304050607\n");

    MK_REQUIRE(texture.bytes.size() == 8);
    MK_REQUIRE(texture.bytes[0] == std::uint8_t{0x00});
    MK_REQUIRE(texture.bytes[7] == std::uint8_t{0x07});
    MK_REQUIRE(mirakana::serialize_texture_source_document(texture).contains("texture.data_hex=0001020304050607\n"));

    const auto mesh = mirakana::deserialize_mesh_source_document(
        "format=GameEngine.MeshSource.v2\n"
        "mesh.vertex_count=3\n"
        "mesh.index_count=3\n"
        "mesh.has_normals=false\n"
        "mesh.has_uvs=false\n"
        "mesh.has_tangent_frame=false\n"
        "mesh.vertex_data_hex="
        "000102000000000000000000000000000000000000000000000000000000000000000000\n"
        "mesh.index_data_hex=000000000100000002000000\n");

    MK_REQUIRE(mesh.vertex_bytes.size() == 36);
    MK_REQUIRE(mesh.index_bytes.size() == 12);
    MK_REQUIRE(mirakana::serialize_mesh_source_document(mesh).contains(
        "mesh.vertex_data_hex=000102000000000000000000000000000000000000000000000000000000000000000000\n"));
    MK_REQUIRE(
        mirakana::serialize_mesh_source_document(mesh).contains("mesh.index_data_hex=000000000100000002000000\n"));

    const auto audio = mirakana::deserialize_audio_source_document("format=GameEngine.AudioSource.v1\n"
                                                                   "audio.sample_rate=48000\n"
                                                                   "audio.channel_count=2\n"
                                                                   "audio.frame_count=1\n"
                                                                   "audio.sample_format=pcm16\n"
                                                                   "audio.data_hex=00010203\n");

    MK_REQUIRE(audio.samples.size() == 4);
    MK_REQUIRE(audio.samples[3] == std::uint8_t{0x03});
    MK_REQUIRE(mirakana::serialize_audio_source_document(audio).contains("audio.data_hex=00010203\n"));
}

MK_TEST("asset source documents reject invalid dimensions and duplicate keys") {
    bool rejected_texture = false;
    try {
        (void)mirakana::deserialize_texture_source_document("format=GameEngine.TextureSource.v1\n"
                                                            "texture.width=0\n"
                                                            "texture.height=2\n"
                                                            "texture.pixel_format=rgba8_unorm\n");
    } catch (const std::invalid_argument&) {
        rejected_texture = true;
    }

    bool rejected_mesh = false;
    try {
        (void)mirakana::deserialize_mesh_source_document("format=GameEngine.MeshSource.v2\n"
                                                         "mesh.vertex_count=24\n"
                                                         "mesh.vertex_count=24\n"
                                                         "mesh.index_count=36\n"
                                                         "mesh.has_normals=true\n"
                                                         "mesh.has_uvs=true\n"
                                                         "mesh.has_tangent_frame=true\n");
    } catch (const std::invalid_argument&) {
        rejected_mesh = true;
    }

    MK_REQUIRE(rejected_texture);
    MK_REQUIRE(rejected_mesh);

    bool rejected_audio = false;
    try {
        (void)mirakana::deserialize_audio_source_document("format=GameEngine.AudioSource.v1\n"
                                                          "audio.sample_rate=0\n"
                                                          "audio.channel_count=2\n"
                                                          "audio.frame_count=24000\n"
                                                          "audio.sample_format=pcm16\n");
    } catch (const std::invalid_argument&) {
        rejected_audio = true;
    }
    MK_REQUIRE(rejected_audio);
}

MK_TEST("asset source documents reject malformed byte payloads") {
    bool rejected_odd_texture_hex = false;
    try {
        (void)mirakana::deserialize_texture_source_document("format=GameEngine.TextureSource.v1\n"
                                                            "texture.width=2\n"
                                                            "texture.height=1\n"
                                                            "texture.pixel_format=rgba8_unorm\n"
                                                            "texture.data_hex=000\n");
    } catch (const std::invalid_argument&) {
        rejected_odd_texture_hex = true;
    }
    MK_REQUIRE(rejected_odd_texture_hex);

    bool rejected_bad_mesh_hex = false;
    try {
        (void)mirakana::deserialize_mesh_source_document("format=GameEngine.MeshSource.v2\n"
                                                         "mesh.vertex_count=3\n"
                                                         "mesh.index_count=3\n"
                                                         "mesh.has_normals=false\n"
                                                         "mesh.has_uvs=false\n"
                                                         "mesh.has_tangent_frame=false\n"
                                                         "mesh.vertex_data_hex=zz\n");
    } catch (const std::invalid_argument&) {
        rejected_bad_mesh_hex = true;
    }
    MK_REQUIRE(rejected_bad_mesh_hex);

    bool rejected_texture_size = false;
    try {
        (void)mirakana::deserialize_texture_source_document("format=GameEngine.TextureSource.v1\n"
                                                            "texture.width=2\n"
                                                            "texture.height=1\n"
                                                            "texture.pixel_format=rgba8_unorm\n"
                                                            "texture.data_hex=00010203\n");
    } catch (const std::invalid_argument&) {
        rejected_texture_size = true;
    }
    MK_REQUIRE(rejected_texture_size);

    bool rejected_audio_size = false;
    try {
        (void)mirakana::deserialize_audio_source_document("format=GameEngine.AudioSource.v1\n"
                                                          "audio.sample_rate=48000\n"
                                                          "audio.channel_count=2\n"
                                                          "audio.frame_count=1\n"
                                                          "audio.sample_format=pcm16\n"
                                                          "audio.data_hex=0001\n");
    } catch (const std::invalid_argument&) {
        rejected_audio_size = true;
    }
    MK_REQUIRE(rejected_audio_size);
}

MK_TEST("asset hot reload tracker emits added modified and removed events") {
    mirakana::AssetHotReloadTracker tracker;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto mesh_id = mirakana::AssetId::from_name("meshes/player");

    const auto first = tracker.update({
        mirakana::AssetFileSnapshot{
            .asset = texture_id, .path = "assets/textures/player.texture", .revision = 10, .size_bytes = 100},
        mirakana::AssetFileSnapshot{
            .asset = mesh_id, .path = "assets/meshes/player.mesh", .revision = 20, .size_bytes = 200},
    });
    MK_REQUIRE(first.size() == 2);
    MK_REQUIRE(first[0].kind == mirakana::AssetHotReloadEventKind::added);
    MK_REQUIRE(first[0].asset == mesh_id);
    MK_REQUIRE(first[1].kind == mirakana::AssetHotReloadEventKind::added);
    MK_REQUIRE(first[1].asset == texture_id);

    const auto second = tracker.update({
        mirakana::AssetFileSnapshot{
            .asset = texture_id, .path = "assets/textures/player.texture", .revision = 11, .size_bytes = 100},
    });
    MK_REQUIRE(second.size() == 2);
    MK_REQUIRE(second[0].kind == mirakana::AssetHotReloadEventKind::removed);
    MK_REQUIRE(second[0].asset == mesh_id);
    MK_REQUIRE(second[1].kind == mirakana::AssetHotReloadEventKind::modified);
    MK_REQUIRE(second[1].asset == texture_id);
    MK_REQUIRE(second[1].previous_revision == 10);
    MK_REQUIRE(second[1].current_revision == 11);
}

MK_TEST("asset hot reload tracker rejects duplicate watched paths") {
    mirakana::AssetHotReloadTracker tracker;
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");

    bool rejected = false;
    try {
        (void)tracker.update({
            mirakana::AssetFileSnapshot{
                .asset = texture_id, .path = "assets/textures/player.texture", .revision = 10, .size_bytes = 100},
            mirakana::AssetFileSnapshot{
                .asset = texture_id, .path = "assets/textures/player.texture", .revision = 11, .size_bytes = 100},
        });
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    MK_REQUIRE(rejected);
}

MK_TEST("asset hot reload recook scheduler debounces source and dependent assets") {
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto material_id = mirakana::AssetId::from_name("materials/player");

    mirakana::AssetDependencyGraph dependencies;
    dependencies.add(mirakana::AssetDependencyEdge{
        .asset = material_id,
        .dependency = texture_id,
        .kind = mirakana::AssetDependencyKind::material_texture,
        .path = "assets/textures/player.texture",
    });

    mirakana::AssetHotReloadRecookScheduler scheduler(mirakana::AssetHotReloadRecookSchedulerDesc{
        3,
    });
    scheduler.enqueue(
        {
            mirakana::AssetHotReloadEvent{
                .kind = mirakana::AssetHotReloadEventKind::modified,
                .asset = texture_id,
                .path = "source/textures/player.texture",
                .previous_revision = 10,
                .current_revision = 11,
                .previous_size_bytes = 100,
                .current_size_bytes = 104,
            },
        },
        dependencies, 100);

    MK_REQUIRE(scheduler.pending_count() == 2);
    MK_REQUIRE(scheduler.ready(102).empty());

    scheduler.enqueue(
        {
            mirakana::AssetHotReloadEvent{
                .kind = mirakana::AssetHotReloadEventKind::modified,
                .asset = texture_id,
                .path = "source/textures/player.texture",
                .previous_revision = 11,
                .current_revision = 12,
                .previous_size_bytes = 104,
                .current_size_bytes = 108,
            },
        },
        dependencies, 102);
    MK_REQUIRE(scheduler.ready(104).empty());

    const auto ready = scheduler.ready(105);
    MK_REQUIRE(ready.size() == 2);
    MK_REQUIRE(ready[0].asset == material_id);
    MK_REQUIRE(ready[0].source_asset == texture_id);
    MK_REQUIRE(ready[0].reason == mirakana::AssetHotReloadRecookReason::dependency_invalidated);
    MK_REQUIRE(ready[0].ready_tick == 105);
    MK_REQUIRE(ready[1].asset == texture_id);
    MK_REQUIRE(ready[1].source_asset == texture_id);
    MK_REQUIRE(ready[1].reason == mirakana::AssetHotReloadRecookReason::source_modified);
    MK_REQUIRE(ready[1].current_revision == 12);
    MK_REQUIRE(scheduler.pending_count() == 0);
}

MK_TEST("asset hot reload apply state keeps previous asset on failed recook") {
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    mirakana::AssetHotReloadApplyState state;
    state.seed({
        mirakana::AssetFileSnapshot{
            .asset = texture_id, .path = "source/textures/player.texture", .revision = 10, .size_bytes = 100},
    });

    const mirakana::AssetHotReloadRecookRequest request{
        .asset = texture_id,
        .source_asset = texture_id,
        .trigger_path = "source/textures/player.texture",
        .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
        .reason = mirakana::AssetHotReloadRecookReason::source_modified,
        .previous_revision = 10,
        .current_revision = 11,
        .ready_tick = 103,
    };

    const auto failed = state.mark_failed(request, "decode failed");
    MK_REQUIRE(failed.kind == mirakana::AssetHotReloadApplyResultKind::failed_rolled_back);
    MK_REQUIRE(failed.asset == texture_id);
    MK_REQUIRE(failed.requested_revision == 11);
    MK_REQUIRE(failed.active_revision == 10);
    MK_REQUIRE(failed.diagnostic == "decode failed");
    const auto* active_after_failure = state.find(texture_id);
    MK_REQUIRE(active_after_failure != nullptr);
    MK_REQUIRE(active_after_failure->revision == 10);

    const auto applied = state.mark_applied(request, 11);
    MK_REQUIRE(applied.kind == mirakana::AssetHotReloadApplyResultKind::applied);
    MK_REQUIRE(applied.active_revision == 11);
    const auto* active_after_success = state.find(texture_id);
    MK_REQUIRE(active_after_success != nullptr);
    MK_REQUIRE(active_after_success->revision == 11);
}

MK_TEST("runtime asset replacement stages recooked assets until a safe point") {
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    mirakana::AssetRuntimeReplacementState state;
    state.seed({
        mirakana::AssetFileSnapshot{
            .asset = texture_id, .path = "assets/textures/player.texture", .revision = 10, .size_bytes = 100},
    });

    const mirakana::AssetHotReloadRecookRequest request{
        .asset = texture_id,
        .source_asset = texture_id,
        .trigger_path = "source/textures/player.texture",
        .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
        .reason = mirakana::AssetHotReloadRecookReason::source_modified,
        .previous_revision = 10,
        .current_revision = 11,
        .ready_tick = 103,
    };

    const auto staged = state.stage(request, "assets/textures/player.texture", 11);
    MK_REQUIRE(staged.kind == mirakana::AssetHotReloadApplyResultKind::staged);
    MK_REQUIRE(staged.requested_revision == 11);
    MK_REQUIRE(staged.active_revision == 10);
    MK_REQUIRE(state.pending_count() == 1);

    const auto* active_before_safe_point = state.find_active(texture_id);
    MK_REQUIRE(active_before_safe_point != nullptr);
    MK_REQUIRE(active_before_safe_point->revision == 10);

    const auto results = state.commit_safe_point();
    MK_REQUIRE(results.size() == 1);
    MK_REQUIRE(results[0].kind == mirakana::AssetHotReloadApplyResultKind::applied);
    MK_REQUIRE(results[0].asset == texture_id);
    MK_REQUIRE(results[0].requested_revision == 11);
    MK_REQUIRE(results[0].active_revision == 11);
    MK_REQUIRE(state.pending_count() == 0);

    const auto* active_after_safe_point = state.find_active(texture_id);
    MK_REQUIRE(active_after_safe_point != nullptr);
    MK_REQUIRE(active_after_safe_point->revision == 11);
}

MK_TEST("runtime asset replacement can commit only selected staged assets") {
    const auto texture_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    mirakana::AssetRuntimeReplacementState state;
    state.seed({
        mirakana::AssetFileSnapshot{
            .asset = texture_id, .path = "assets/textures/player.texture", .revision = 10, .size_bytes = 100},
        mirakana::AssetFileSnapshot{
            .asset = material_id, .path = "assets/materials/player.material", .revision = 5, .size_bytes = 64},
    });

    const mirakana::AssetHotReloadRecookRequest texture_request{
        .asset = texture_id,
        .source_asset = texture_id,
        .trigger_path = "source/textures/player.texture",
        .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
        .reason = mirakana::AssetHotReloadRecookReason::source_modified,
        .previous_revision = 10,
        .current_revision = 11,
        .ready_tick = 103,
    };
    const mirakana::AssetHotReloadRecookRequest material_request{
        .asset = material_id,
        .source_asset = material_id,
        .trigger_path = "source/materials/player.material",
        .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
        .reason = mirakana::AssetHotReloadRecookReason::source_modified,
        .previous_revision = 5,
        .current_revision = 6,
        .ready_tick = 104,
    };

    MK_REQUIRE(state.stage(texture_request, "assets/textures/player.texture", 11).kind ==
               mirakana::AssetHotReloadApplyResultKind::staged);
    MK_REQUIRE(state.stage(material_request, "assets/materials/player.material", 6).kind ==
               mirakana::AssetHotReloadApplyResultKind::staged);
    MK_REQUIRE(state.pending_count() == 2);

    const std::array selected_assets{texture_id};
    const auto selected_results = state.commit_safe_point(selected_assets);
    MK_REQUIRE(selected_results.size() == 1);
    MK_REQUIRE(selected_results[0].asset == texture_id);
    MK_REQUIRE(selected_results[0].active_revision == 11);
    MK_REQUIRE(state.pending_count() == 1);
    MK_REQUIRE(state.find_pending(texture_id) == nullptr);
    MK_REQUIRE(state.find_pending(material_id) != nullptr);
    MK_REQUIRE(state.find_active(texture_id)->revision == 11);
    MK_REQUIRE(state.find_active(material_id)->revision == 5);

    const auto remaining_results = state.commit_safe_point();
    MK_REQUIRE(remaining_results.size() == 1);
    MK_REQUIRE(remaining_results[0].asset == material_id);
    MK_REQUIRE(remaining_results[0].active_revision == 6);
    MK_REQUIRE(state.pending_count() == 0);
    MK_REQUIRE(state.find_active(material_id)->revision == 6);
}

MK_TEST("runtime asset replacement rolls back failed recooks without losing the active asset") {
    const auto material_id = mirakana::AssetId::from_name("materials/player");
    mirakana::AssetRuntimeReplacementState state;
    state.seed({
        mirakana::AssetFileSnapshot{
            .asset = material_id, .path = "assets/materials/player.material", .revision = 5, .size_bytes = 64},
    });

    const mirakana::AssetHotReloadRecookRequest request{
        .asset = material_id,
        .source_asset = material_id,
        .trigger_path = "source/materials/player.material",
        .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
        .reason = mirakana::AssetHotReloadRecookReason::source_modified,
        .previous_revision = 5,
        .current_revision = 6,
        .ready_tick = 103,
    };

    const auto staged = state.stage(request, "assets/materials/player.material", 6);
    MK_REQUIRE(staged.kind == mirakana::AssetHotReloadApplyResultKind::staged);
    MK_REQUIRE(state.pending_count() == 1);

    const auto failed = state.mark_failed(request, "material decode failed");
    MK_REQUIRE(failed.kind == mirakana::AssetHotReloadApplyResultKind::failed_rolled_back);
    MK_REQUIRE(failed.requested_revision == 6);
    MK_REQUIRE(failed.active_revision == 5);
    MK_REQUIRE(failed.diagnostic == "material decode failed");
    MK_REQUIRE(state.pending_count() == 0);

    const auto* active = state.find_active(material_id);
    MK_REQUIRE(active != nullptr);
    MK_REQUIRE(active->revision == 5);
}

MK_TEST("material definition validates factors and texture slots") {
    const mirakana::AssetId material_id = mirakana::AssetId::from_name("materials/player");
    const mirakana::AssetId albedo_id = mirakana::AssetId::from_name("textures/player.albedo");
    const mirakana::AssetId normal_id = mirakana::AssetId::from_name("textures/player.normal");
    mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {1.0F, 0.8F, 0.6F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.1F,
                .roughness = 0.7F,
            },
        .texture_bindings =
            {
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                 .texture = albedo_id},
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::normal, .texture = normal_id},
            },
        .double_sided = true,
    };

    MK_REQUIRE(mirakana::is_valid_material_definition(material));

    material.texture_bindings.push_back(
        mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color, .texture = normal_id});
    MK_REQUIRE(!mirakana::is_valid_material_definition(material));
}

MK_TEST("material definition serializes deterministic text") {
    const mirakana::AssetId material_id = mirakana::AssetId::from_name("materials/player");
    const mirakana::AssetId albedo_id = mirakana::AssetId::from_name("textures/player.albedo");
    const mirakana::AssetId normal_id = mirakana::AssetId::from_name("textures/player.normal");
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::masked,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {1.0F, 0.8F, 0.6F, 1.0F},
                .emissive = {0.2F, 0.1F, 0.0F},
                .metallic = 0.25F,
                .roughness = 0.75F,
            },
        .texture_bindings =
            {
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::normal, .texture = normal_id},
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                 .texture = albedo_id},
            },
        .double_sided = true,
    };

    const auto text = mirakana::serialize_material_definition(material);
    MK_REQUIRE(text.starts_with("format=GameEngine.Material.v1\n"));
    MK_REQUIRE(text.contains("material.name=Player\n"));
    MK_REQUIRE(text.contains("texture.1.slot=base_color\n"));
    MK_REQUIRE(text.contains("texture.2.slot=normal\n"));

    const auto restored = mirakana::deserialize_material_definition(text);
    MK_REQUIRE(restored.id == material_id);
    MK_REQUIRE(restored.name == "Player");
    MK_REQUIRE(restored.shading_model == mirakana::MaterialShadingModel::lit);
    MK_REQUIRE(restored.surface_mode == mirakana::MaterialSurfaceMode::masked);
    MK_REQUIRE(restored.double_sided);
    MK_REQUIRE(restored.factors.metallic == 0.25F);
    MK_REQUIRE(restored.texture_bindings.size() == 2);
    MK_REQUIRE(restored.texture_bindings[0].slot == mirakana::MaterialTextureSlot::base_color);
    MK_REQUIRE(restored.texture_bindings[0].texture == albedo_id);
}

MK_TEST("material definition rejects malformed float text") {
    const mirakana::AssetId material_id = mirakana::AssetId::from_name("materials/player");
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::masked,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {1.0F, 0.8F, 0.6F, 1.0F},
                .emissive = {0.2F, 0.1F, 0.0F},
                .metallic = 0.25F,
                .roughness = 0.75F,
            },
        .texture_bindings = {},
        .double_sided = true,
    };

    auto text = mirakana::serialize_material_definition(material);
    const std::string valid_metallic = "factor.metallic=0.25\n";
    const auto metallic = text.find(valid_metallic);
    MK_REQUIRE(metallic != std::string::npos);
    text.replace(metallic, valid_metallic.size(), "factor.metallic=0.25x\n");

    bool rejected = false;
    try {
        (void)mirakana::deserialize_material_definition(text);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    MK_REQUIRE(rejected);
}

MK_TEST("material binding metadata derives stable descriptor semantics from texture slots") {
    const mirakana::AssetId material_id = mirakana::AssetId::from_name("materials/player");
    const mirakana::AssetId albedo_id = mirakana::AssetId::from_name("textures/player.albedo");
    const mirakana::AssetId normal_id = mirakana::AssetId::from_name("textures/player.normal");
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::transparent,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings =
            {
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::normal, .texture = normal_id},
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                 .texture = albedo_id},
            },
        .double_sided = true,
    };

    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);

    MK_REQUIRE(metadata.material == material_id);
    MK_REQUIRE(metadata.surface_mode == mirakana::MaterialSurfaceMode::transparent);
    MK_REQUIRE(metadata.double_sided);
    MK_REQUIRE(metadata.requires_alpha_blending);
    MK_REQUIRE(!metadata.requires_alpha_test);
    MK_REQUIRE(metadata.bindings.size() == 6);
    MK_REQUIRE(metadata.bindings[0].semantic == "material_factors");
    MK_REQUIRE(metadata.bindings[0].resource_kind == mirakana::MaterialBindingResourceKind::uniform_buffer);
    MK_REQUIRE(metadata.bindings[0].set == 0);
    MK_REQUIRE(metadata.bindings[0].binding == 0);
    MK_REQUIRE(metadata.bindings[0].stages == mirakana::MaterialShaderStageMask::fragment);
    MK_REQUIRE(metadata.bindings[1].semantic == "scene_pbr_frame");
    MK_REQUIRE(metadata.bindings[1].resource_kind == mirakana::MaterialBindingResourceKind::uniform_buffer);
    MK_REQUIRE(metadata.bindings[1].binding == 6);
    MK_REQUIRE(metadata.bindings[1].stages ==
               (mirakana::MaterialShaderStageMask::vertex | mirakana::MaterialShaderStageMask::fragment));
    MK_REQUIRE(metadata.bindings[2].texture_slot == mirakana::MaterialTextureSlot::base_color);
    MK_REQUIRE(metadata.bindings[2].semantic == "texture.base_color");
    MK_REQUIRE(metadata.bindings[2].binding == 1);
    MK_REQUIRE(metadata.bindings[3].texture_slot == mirakana::MaterialTextureSlot::base_color);
    MK_REQUIRE(metadata.bindings[3].resource_kind == mirakana::MaterialBindingResourceKind::sampler);
    MK_REQUIRE(metadata.bindings[3].semantic == "sampler.base_color");
    MK_REQUIRE(metadata.bindings[3].binding == 16);
    MK_REQUIRE(metadata.bindings[4].texture_slot == mirakana::MaterialTextureSlot::normal);
    MK_REQUIRE(metadata.bindings[4].binding == 2);
    MK_REQUIRE(metadata.bindings[5].resource_kind == mirakana::MaterialBindingResourceKind::sampler);
    MK_REQUIRE(metadata.bindings[5].texture_slot == mirakana::MaterialTextureSlot::normal);
    MK_REQUIRE(metadata.bindings[5].binding == 17);
}

MK_TEST("material binding metadata maps masked materials to alpha test pipeline state") {
    const mirakana::AssetId material_id = mirakana::AssetId::from_name("materials/leaves");
    const mirakana::MaterialDefinition material{
        .id = material_id,
        .name = "Leaves",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::masked,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {},
        .double_sided = false,
    };

    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);

    MK_REQUIRE(metadata.bindings.size() == 2);
    MK_REQUIRE(metadata.requires_alpha_test);
    MK_REQUIRE(!metadata.requires_alpha_blending);
}

MK_TEST("material instance overrides parent factors and texture slots") {
    const auto parent_id = mirakana::AssetId::from_name("materials/player");
    const auto instance_id = mirakana::AssetId::from_name("materials/player.team-red");
    const auto albedo_id = mirakana::AssetId::from_name("textures/player.albedo");
    const auto red_albedo_id = mirakana::AssetId::from_name("textures/player.red-albedo");
    const auto normal_id = mirakana::AssetId::from_name("textures/player.normal");

    const mirakana::MaterialDefinition parent{
        .id = parent_id,
        .name = "Player",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors =
            mirakana::MaterialFactors{
                .base_color = {0.7F, 0.7F, 0.7F, 1.0F},
                .emissive = {0.0F, 0.0F, 0.0F},
                .metallic = 0.0F,
                .roughness = 0.65F,
            },
        .texture_bindings =
            {
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                 .texture = albedo_id},
                mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::normal, .texture = normal_id},
            },
        .double_sided = false,
    };

    mirakana::MaterialInstanceDefinition instance;
    instance.id = instance_id;
    instance.name = "Player Team Red";
    instance.parent = parent_id;
    instance.factor_overrides = mirakana::MaterialFactors{
        .base_color = {1.0F, 0.05F, 0.02F, 1.0F},
        .emissive = {0.1F, 0.0F, 0.0F},
        .metallic = 0.0F,
        .roughness = 0.5F,
    };
    instance.texture_overrides.push_back(
        mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color, .texture = red_albedo_id});

    MK_REQUIRE(mirakana::is_valid_material_instance_definition(instance));

    const auto composed = mirakana::compose_material_instance(parent, instance);

    MK_REQUIRE(composed.id == instance_id);
    MK_REQUIRE(composed.name == "Player Team Red");
    MK_REQUIRE(composed.shading_model == mirakana::MaterialShadingModel::lit);
    MK_REQUIRE(composed.factors.base_color[0] == 1.0F);
    MK_REQUIRE(composed.factors.base_color[1] == 0.05F);
    MK_REQUIRE(composed.factors.roughness == 0.5F);
    MK_REQUIRE(composed.texture_bindings.size() == 2);
    MK_REQUIRE(composed.texture_bindings[0].slot == mirakana::MaterialTextureSlot::base_color);
    MK_REQUIRE(composed.texture_bindings[0].texture == red_albedo_id);
    MK_REQUIRE(composed.texture_bindings[1].slot == mirakana::MaterialTextureSlot::normal);
    MK_REQUIRE(composed.texture_bindings[1].texture == normal_id);
}

MK_TEST("material instance serializes deterministic text") {
    const auto instance_id = mirakana::AssetId::from_name("materials/player.team-blue");
    const auto parent_id = mirakana::AssetId::from_name("materials/player");
    const auto albedo_id = mirakana::AssetId::from_name("textures/player.blue-albedo");

    mirakana::MaterialInstanceDefinition instance;
    instance.id = instance_id;
    instance.name = "Player Team Blue";
    instance.parent = parent_id;
    instance.factor_overrides = mirakana::MaterialFactors{
        .base_color = {0.05F, 0.2F, 1.0F, 1.0F},
        .emissive = {0.0F, 0.0F, 0.05F},
        .metallic = 0.1F,
        .roughness = 0.45F,
    };
    instance.texture_overrides.push_back(
        mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color, .texture = albedo_id});

    const auto text = mirakana::serialize_material_instance_definition(instance);

    MK_REQUIRE(text.starts_with("format=GameEngine.MaterialInstance.v1\n"));
    MK_REQUIRE(text.contains("instance.name=Player Team Blue\n"));
    MK_REQUIRE(text.contains("factor.override=true\n"));
    MK_REQUIRE(text.contains("texture.1.slot=base_color\n"));

    auto restored = mirakana::deserialize_material_instance_definition(text);
    MK_REQUIRE(restored.id == instance_id);
    MK_REQUIRE(restored.parent == parent_id);
    MK_REQUIRE(restored.factor_overrides.has_value());
    MK_REQUIRE(restored.factor_overrides->base_color[2] == 1.0F);
    MK_REQUIRE(restored.texture_overrides.size() == 1);
    MK_REQUIRE(restored.texture_overrides[0].texture == albedo_id);

    restored.texture_overrides.push_back(
        mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color, .texture = albedo_id});
    MK_REQUIRE(!mirakana::is_valid_material_instance_definition(restored));
}

MK_TEST("shader pipeline builds dxc dxil compile commands") {
    const mirakana::ShaderSourceMetadata source{
        .id = mirakana::AssetId::from_name("shaders/editor-grid.hlsl"),
        .source_path = "assets/shaders/editor-grid.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::fragment,
        .entry_point = "ps_main",
        .defines = {"EDITOR_GRID=1"},
        .artifacts = {},
        .reflection = {},
    };

    const auto command = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = source,
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/editor-grid.ps.dxil",
        .profile = "ps_6_7",
        .include_paths = {"assets/shaders/include"},
        .debug_symbols = true,
        .optimize = false,
    });

    MK_REQUIRE(command.executable == "dxc");
    MK_REQUIRE(command.arguments.size() == 13);
    MK_REQUIRE(command.arguments[0] == "-T");
    MK_REQUIRE(command.arguments[1] == "ps_6_7");
    MK_REQUIRE(command.arguments[2] == "-E");
    MK_REQUIRE(command.arguments[3] == "ps_main");
    MK_REQUIRE(command.arguments[4] == "-Fo");
    MK_REQUIRE(command.arguments[5] == "out/shaders/editor-grid.ps.dxil");
    MK_REQUIRE(command.arguments[6] == "-I");
    MK_REQUIRE(command.arguments[7] == "assets/shaders/include");
    MK_REQUIRE(command.arguments[8] == "-D");
    MK_REQUIRE(command.arguments[9] == "EDITOR_GRID=1");
    MK_REQUIRE(command.arguments[10] == "-Zi");
    MK_REQUIRE(command.arguments[11] == "-Od");
    MK_REQUIRE(command.arguments[12] == "assets/shaders/editor-grid.hlsl");
    MK_REQUIRE(command.artifact.format == mirakana::ShaderArtifactFormat::dxil);
    MK_REQUIRE(command.artifact.profile == "ps_6_7");
}

MK_TEST("shader pipeline builds spirv and metal compile commands") {
    const mirakana::ShaderSourceMetadata hlsl_source{
        .id = mirakana::AssetId::from_name("shaders/fullscreen.hlsl"),
        .source_path = "assets/shaders/fullscreen.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::vertex,
        .entry_point = "vs_main",
        .defines = {},
        .artifacts = {},
        .reflection = {},
    };
    const auto spirv = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = hlsl_source,
        .target = mirakana::ShaderCompileTarget::vulkan_spirv,
        .output_path = "out/shaders/fullscreen.vs.spv",
        .profile = "vs_6_7",
        .include_paths = {},
        .debug_symbols = true,
        .optimize = true,
    });

    MK_REQUIRE(spirv.executable == "dxc");
    MK_REQUIRE(spirv.arguments[0] == "-spirv");
    MK_REQUIRE(spirv.arguments[1] == "-fspv-target-env=vulkan1.3");
    MK_REQUIRE(spirv.artifact.format == mirakana::ShaderArtifactFormat::spirv);

    const mirakana::ShaderSourceMetadata metal_source{
        .id = mirakana::AssetId::from_name("shaders/fullscreen.metal"),
        .source_path = "assets/shaders/fullscreen.metal",
        .language = mirakana::ShaderSourceLanguage::msl,
        .stage = mirakana::ShaderSourceStage::fragment,
        .entry_point = "fragment_main",
        .defines = {},
        .artifacts = {},
        .reflection = {},
    };
    const auto metal = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = metal_source,
        .target = mirakana::ShaderCompileTarget::metal_ir,
        .output_path = "out/shaders/fullscreen.air",
        .profile = "metal3.2",
        .include_paths = {},
        .debug_symbols = true,
        .optimize = true,
    });

    MK_REQUIRE(metal.executable == "metal");
    MK_REQUIRE(metal.arguments.size() == 5);
    MK_REQUIRE(metal.arguments[0] == "-c");
    MK_REQUIRE(metal.arguments[1] == "assets/shaders/fullscreen.metal");
    MK_REQUIRE(metal.arguments[2] == "-o");
    MK_REQUIRE(metal.arguments[3] == "out/shaders/fullscreen.air");
    MK_REQUIRE(metal.arguments[4] == "-gline-tables-only");
    MK_REQUIRE(metal.artifact.format == mirakana::ShaderArtifactFormat::metal_ir);
}

MK_TEST("shader artifact manifests round trip deterministic records") {
    std::vector<mirakana::ShaderSourceMetadata> shaders;
    shaders.push_back(mirakana::ShaderSourceMetadata{
        .id = mirakana::AssetId::from_name("shaders/z.hlsl"),
        .source_path = "assets/shaders/z.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::vertex,
        .entry_point = "vs_main",
        .defines = {},
        .artifacts = {mirakana::ShaderGeneratedArtifact{.path = "out/shaders/z.vs.dxil",
                                                        .format = mirakana::ShaderArtifactFormat::dxil,
                                                        .profile = "vs_6_7",
                                                        .entry_point = "vs_main"}},
        .reflection = {},
    });
    shaders.push_back(mirakana::ShaderSourceMetadata{
        .id = mirakana::AssetId::from_name("shaders/a.hlsl"),
        .source_path = "assets/shaders/a.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::fragment,
        .entry_point = "ps_main",
        .defines = {"USE_FOG=1"},
        .artifacts = {mirakana::ShaderGeneratedArtifact{.path = "out/shaders/a.ps.spv",
                                                        .format = mirakana::ShaderArtifactFormat::spirv,
                                                        .profile = "ps_6_7",
                                                        .entry_point = "ps_main"}},
        .reflection = {},
    });

    const auto text = mirakana::serialize_shader_artifact_manifest(shaders);
    MK_REQUIRE(text.starts_with("format=GameEngine.ShaderArtifacts.v1\n"));
    MK_REQUIRE(text.contains("shader.1.source=assets/shaders/a.hlsl"));
    MK_REQUIRE(text.contains("shader.2.source=assets/shaders/z.hlsl"));

    const auto restored = mirakana::deserialize_shader_artifact_manifest(text);
    MK_REQUIRE(restored.size() == 2);
    MK_REQUIRE(restored[0].source_path == "assets/shaders/a.hlsl");
    MK_REQUIRE(restored[0].defines.size() == 1);
    MK_REQUIRE(restored[0].artifacts.size() == 1);
    MK_REQUIRE(restored[0].artifacts[0].format == mirakana::ShaderArtifactFormat::spirv);
    MK_REQUIRE(restored[1].source_path == "assets/shaders/z.hlsl");
}

MK_TEST("shader tool runner records safe shell free commands") {
    const mirakana::ShaderSourceMetadata source{
        .id = mirakana::AssetId::from_name("shaders/fullscreen.hlsl"),
        .source_path = "assets/shaders/fullscreen.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::vertex,
        .entry_point = "vs_main",
        .defines = {},
        .artifacts = {},
        .reflection = {},
    };
    const auto command = mirakana::make_shader_compile_command(mirakana::ShaderCompileRequest{
        .source = source,
        .target = mirakana::ShaderCompileTarget::d3d12_dxil,
        .output_path = "out/shaders/fullscreen.vs.dxil",
        .profile = "vs_6_7",
        .include_paths = {},
        .debug_symbols = true,
        .optimize = true,
    });

    mirakana::RecordingShaderToolRunner runner;
    const auto result = mirakana::run_shader_tool_command(runner, command);

    MK_REQUIRE(result.exit_code == 0);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.stdout_text.empty());
    MK_REQUIRE(result.stderr_text.empty());
    MK_REQUIRE(result.artifact.path == "out/shaders/fullscreen.vs.dxil");
    MK_REQUIRE(runner.commands().size() == 1);
    MK_REQUIRE(runner.commands()[0].executable == "dxc");
}

MK_TEST("shader tool run result captures compiler output") {
    const mirakana::ShaderToolRunResult result{
        .exit_code = 1,
        .diagnostic = "compiler failed",
        .stdout_text = "compiled 0 shaders",
        .stderr_text = "error: missing entry point",
        .artifact = mirakana::ShaderGeneratedArtifact{.path = "out/shaders/fullscreen.vs.dxil",
                                                      .format = mirakana::ShaderArtifactFormat::dxil,
                                                      .profile = "vs_6_7",
                                                      .entry_point = "vs_main"},
    };

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.stdout_text == "compiled 0 shaders");
    MK_REQUIRE(result.stderr_text == "error: missing entry point");
}

MK_TEST("shader tool runner rejects unsafe commands") {
    mirakana::ShaderCompileCommand unsafe_executable{
        .executable = "powershell",
        .arguments = {"-NoProfile"},
        .artifact = mirakana::ShaderGeneratedArtifact{.path = "out/shaders/fullscreen.vs.dxil",
                                                      .format = mirakana::ShaderArtifactFormat::dxil,
                                                      .profile = "vs_6_7",
                                                      .entry_point = "vs_main"},
    };
    mirakana::RecordingShaderToolRunner runner;

    bool rejected_executable = false;
    try {
        (void)mirakana::run_shader_tool_command(runner, unsafe_executable);
    } catch (const std::invalid_argument&) {
        rejected_executable = true;
    }

    mirakana::ShaderCompileCommand unsafe_output{
        .executable = "dxc",
        .arguments = {"-T", "vs_6_7", "-E", "vs_main", "-Fo", "../fullscreen.dxil", "assets/shaders/fullscreen.hlsl"},
        .artifact = mirakana::ShaderGeneratedArtifact{.path = "../fullscreen.dxil",
                                                      .format = mirakana::ShaderArtifactFormat::dxil,
                                                      .profile = "vs_6_7",
                                                      .entry_point = "vs_main"},
    };
    bool rejected_output = false;
    try {
        (void)mirakana::run_shader_tool_command(runner, unsafe_output);
    } catch (const std::invalid_argument&) {
        rejected_output = true;
    }

    MK_REQUIRE(rejected_executable);
    MK_REQUIRE(rejected_output);
    MK_REQUIRE(runner.commands().empty());
}

MK_TEST("shader pipeline discovers include dependencies") {
    const auto dependencies = mirakana::discover_shader_source_dependencies("// #include \"ignored.hlsl\"\n"
                                                                            "#include \"common.hlsl\"\n"
                                                                            "   #include <engine/lighting.hlsl>\n"
                                                                            "/* #include \"also_ignored.hlsl\" */\n"
                                                                            "#include \"common.hlsl\"\n");

    MK_REQUIRE(dependencies.size() == 2);
    MK_REQUIRE(dependencies[0].path == "common.hlsl");
    MK_REQUIRE(dependencies[0].kind == mirakana::ShaderIncludeKind::quoted);
    MK_REQUIRE(dependencies[1].path == "engine/lighting.hlsl");
    MK_REQUIRE(dependencies[1].kind == mirakana::ShaderIncludeKind::system);
}

MK_TEST("asset dependency graph tracks deterministic shader include edges") {
    const mirakana::ShaderSourceMetadata source{
        .id = mirakana::AssetId::from_name("shaders/fullscreen.hlsl"),
        .source_path = "assets/shaders/fullscreen.hlsl",
        .language = mirakana::ShaderSourceLanguage::hlsl,
        .stage = mirakana::ShaderSourceStage::vertex,
        .entry_point = "vs_main",
        .defines = {},
        .artifacts = {},
        .reflection = {},
    };

    const auto edges = mirakana::make_shader_include_dependency_edges(source, "#include \"common.hlsl\"\n"
                                                                              "#include <engine/lighting.hlsl>\n");
    MK_REQUIRE(edges.size() == 2);

    mirakana::AssetDependencyGraph graph;
    MK_REQUIRE(graph.try_add(edges[1]));
    MK_REQUIRE(graph.try_add(edges[0]));

    const auto dependencies = graph.dependencies_of(source.id);
    MK_REQUIRE(dependencies.size() == 2);
    MK_REQUIRE(dependencies[0].path == "assets/shaders/common.hlsl");
    MK_REQUIRE(dependencies[0].dependency == mirakana::AssetId::from_name("assets/shaders/common.hlsl"));
    MK_REQUIRE(dependencies[0].kind == mirakana::AssetDependencyKind::shader_include);
    MK_REQUIRE(dependencies[1].path == "engine/lighting.hlsl");
    MK_REQUIRE(dependencies[1].dependency == mirakana::AssetId::from_name("engine/lighting.hlsl"));

    const auto dependents = graph.dependents_of(mirakana::AssetId::from_name("assets/shaders/common.hlsl"));
    MK_REQUIRE(dependents.size() == 1);
    MK_REQUIRE(dependents[0].asset == source.id);
}

MK_TEST("asset dependency graph rejects invalid and duplicate edges") {
    const auto shader = mirakana::AssetId::from_name("shaders/fullscreen.hlsl");
    const auto include = mirakana::AssetId::from_name("assets/shaders/common.hlsl");
    mirakana::AssetDependencyGraph graph;

    const mirakana::AssetDependencyEdge edge{
        .asset = shader,
        .dependency = include,
        .kind = mirakana::AssetDependencyKind::shader_include,
        .path = "assets/shaders/common.hlsl",
    };

    MK_REQUIRE(graph.try_add(edge));
    MK_REQUIRE(!graph.try_add(edge));
    MK_REQUIRE(!graph.try_add(mirakana::AssetDependencyEdge{
        shader,
        shader,
        mirakana::AssetDependencyKind::shader_include,
        "assets/shaders/fullscreen.hlsl",
    }));
    MK_REQUIRE(!graph.try_add(mirakana::AssetDependencyEdge{
        shader,
        include,
        mirakana::AssetDependencyKind::unknown,
        "assets/shaders/common.hlsl",
    }));
    MK_REQUIRE(graph.edge_count() == 1);
}

MK_TEST("scene creates named nodes with transforms") {
    mirakana::Scene scene("level-01");

    const auto player = scene.create_node("Player");
    auto* node = scene.find_node(player);
    MK_REQUIRE(node != nullptr);
    MK_REQUIRE(node->name == "Player");

    node->transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    MK_REQUIRE(scene.find_node(player)->transform.position == (mirakana::Vec3{1.0F, 2.0F, 3.0F}));
}

MK_TEST("scene parents child nodes") {
    mirakana::Scene scene("level-01");

    const auto parent = scene.create_node("Parent");
    const auto child = scene.create_node("Child");

    scene.set_parent(child, parent);

    const auto* child_node = scene.find_node(child);
    const auto* parent_node = scene.find_node(parent);
    MK_REQUIRE(child_node != nullptr);
    MK_REQUIRE(parent_node != nullptr);
    MK_REQUIRE(child_node->parent == parent);
    MK_REQUIRE(parent_node->children.size() == 1);
    MK_REQUIRE(parent_node->children[0] == child);
}

MK_TEST("scene serializes and restores hierarchy and transforms") {
    mirakana::Scene scene("level-01");

    const auto parent = scene.create_node("Player");
    const auto child = scene.create_node("Weapon");
    scene.set_parent(child, parent);
    scene.find_node(parent)->transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    scene.find_node(parent)->transform.scale = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F};
    scene.find_node(parent)->transform.rotation_radians = mirakana::Vec3{.x = 0.1F, .y = 0.2F, .z = 0.3F};

    const auto serialized = mirakana::serialize_scene(scene);
    MK_REQUIRE(serialized.contains("format=GameEngine.Scene.v1"));
    MK_REQUIRE(serialized.contains("scene.name=level-01"));
    MK_REQUIRE(serialized.contains("node.1.name=Player"));
    MK_REQUIRE(serialized.contains("node.2.parent=1"));
    MK_REQUIRE(serialized.contains("node.1.position=1,2,3"));
    MK_REQUIRE(serialized.contains("node.1.rotation=0.1,0.2,0.3"));

    const auto restored = mirakana::deserialize_scene(serialized);
    MK_REQUIRE(restored.name() == "level-01");
    MK_REQUIRE(restored.nodes().size() == 2);
    MK_REQUIRE(restored.find_node(mirakana::SceneNodeId{1})->name == "Player");
    MK_REQUIRE(restored.find_node(mirakana::SceneNodeId{2})->parent == mirakana::SceneNodeId{1});
    MK_REQUIRE(restored.find_node(mirakana::SceneNodeId{1})->children.size() == 1);
    MK_REQUIRE(restored.find_node(mirakana::SceneNodeId{1})->transform.position == (mirakana::Vec3{1.0F, 2.0F, 3.0F}));
    MK_REQUIRE(restored.find_node(mirakana::SceneNodeId{1})->transform.scale == (mirakana::Vec3{2.0F, 2.0F, 2.0F}));
    MK_REQUIRE(restored.find_node(mirakana::SceneNodeId{1})->transform.rotation_radians ==
               (mirakana::Vec3{0.1F, 0.2F, 0.3F}));
}

MK_TEST("scene components validate camera lighting and renderer references") {
    mirakana::CameraComponent camera;
    camera.projection = mirakana::CameraProjectionMode::perspective;
    camera.vertical_fov_radians = 1.0F;
    camera.near_plane = 0.1F;
    camera.far_plane = 1000.0F;
    camera.primary = true;
    MK_REQUIRE(mirakana::is_valid_camera_component(camera));

    camera.far_plane = 0.05F;
    MK_REQUIRE(!mirakana::is_valid_camera_component(camera));

    mirakana::LightComponent light;
    light.type = mirakana::LightType::spot;
    light.color = mirakana::Vec3{.x = 1.0F, .y = 0.95F, .z = 0.8F};
    light.intensity = 4.0F;
    light.range = 25.0F;
    light.inner_cone_radians = 0.25F;
    light.outer_cone_radians = 0.75F;
    MK_REQUIRE(mirakana::is_valid_light_component(light));

    light.inner_cone_radians = 1.0F;
    MK_REQUIRE(!mirakana::is_valid_light_component(light));

    const mirakana::MeshRendererComponent renderer{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    MK_REQUIRE(mirakana::is_valid_mesh_renderer_component(renderer));

    mirakana::SpriteRendererComponent sprite{
        .sprite = mirakana::AssetId::from_name("sprites/player"),
        .material = mirakana::AssetId::from_name("materials/sprite"),
        .size = mirakana::Vec2{.x = 2.0F, .y = 3.0F},
        .tint = {1.0F, 0.5F, 0.25F, 1.0F},
        .visible = true,
    };
    MK_REQUIRE(mirakana::is_valid_sprite_renderer_component(sprite));
    sprite.size.x = 0.0F;
    MK_REQUIRE(!mirakana::is_valid_sprite_renderer_component(sprite));
}

MK_TEST("scene serializes and restores camera light mesh and sprite components") {
    mirakana::Scene scene("level-01");
    const auto camera_node = scene.create_node("MainCamera");
    const auto mesh_node = scene.create_node("Player");
    const auto sprite_node = scene.create_node("HudIcon");

    mirakana::SceneNodeComponents camera_components;
    camera_components.camera = mirakana::CameraComponent{
        .projection = mirakana::CameraProjectionMode::perspective,
        .vertical_fov_radians = 1.0F,
        .orthographic_height = 10.0F,
        .near_plane = 0.1F,
        .far_plane = 500.0F,
        .primary = true,
    };
    camera_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::directional,
        .color = mirakana::Vec3{.x = 1.0F, .y = 0.95F, .z = 0.8F},
        .intensity = 3.0F,
        .range = 100.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(camera_node, camera_components);

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    scene.set_components(mesh_node, mesh_components);

    mirakana::SceneNodeComponents sprite_components;
    sprite_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("sprites/hud-icon"),
        .material = mirakana::AssetId::from_name("materials/sprite"),
        .size = mirakana::Vec2{.x = 2.0F, .y = 3.0F},
        .tint = {0.25F, 0.5F, 0.75F, 1.0F},
        .visible = true,
    };
    scene.set_components(sprite_node, sprite_components);

    const auto serialized = mirakana::serialize_scene(scene);
    MK_REQUIRE(serialized.contains("node.1.camera.projection=perspective"));
    MK_REQUIRE(serialized.contains("node.1.light.type=directional"));
    MK_REQUIRE(serialized.contains("node.2.mesh_renderer.visible=true"));
    MK_REQUIRE(serialized.contains("node.3.sprite_renderer.sprite="));
    MK_REQUIRE(serialized.contains("node.3.sprite_renderer.size=2,3"));

    const auto restored = mirakana::deserialize_scene(serialized);
    const auto* restored_camera = restored.find_node(camera_node);
    const auto* restored_mesh = restored.find_node(mesh_node);
    const auto* restored_sprite = restored.find_node(sprite_node);
    MK_REQUIRE(restored_camera != nullptr);
    MK_REQUIRE(restored_mesh != nullptr);
    MK_REQUIRE(restored_sprite != nullptr);
    MK_REQUIRE(restored_camera->components.camera.has_value());
    MK_REQUIRE(restored_camera->components.camera->primary);
    MK_REQUIRE(restored_camera->components.light.has_value());
    MK_REQUIRE(restored_camera->components.light->intensity == 3.0F);
    MK_REQUIRE(restored_mesh->components.mesh_renderer.has_value());
    MK_REQUIRE(restored_mesh->components.mesh_renderer->mesh == mirakana::AssetId::from_name("meshes/player"));
    MK_REQUIRE(restored_sprite->components.sprite_renderer.has_value());
    MK_REQUIRE(restored_sprite->components.sprite_renderer->sprite == mirakana::AssetId::from_name("sprites/hud-icon"));
    MK_REQUIRE(restored_sprite->components.sprite_renderer->size == (mirakana::Vec2{2.0F, 3.0F}));
}

MK_TEST("scene render packet extracts renderable components with world transforms") {
    mirakana::Scene scene("level-01");
    const auto root = scene.create_node("Root");
    const auto camera_node = scene.create_node("MainCamera");
    const auto light_node = scene.create_node("KeyLight");
    const auto mesh_node = scene.create_node("Player");
    const auto hidden_mesh_node = scene.create_node("HiddenDebug");
    const auto sprite_node = scene.create_node("Nameplate");
    const auto hidden_sprite_node = scene.create_node("HiddenSprite");

    scene.find_node(root)->transform.position = mirakana::Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F};
    scene.find_node(root)->transform.scale = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F};
    scene.find_node(root)->transform.rotation_radians = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.57079637F};
    scene.find_node(camera_node)->transform.position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    scene.find_node(mesh_node)->transform.position = mirakana::Vec3{.x = 0.0F, .y = 3.0F, .z = 0.0F};
    scene.find_node(sprite_node)->transform.position = mirakana::Vec3{.x = 2.0F, .y = 1.0F, .z = 0.0F};
    scene.set_parent(camera_node, root);
    scene.set_parent(mesh_node, root);
    scene.set_parent(sprite_node, root);

    mirakana::SceneNodeComponents camera_components;
    camera_components.camera = mirakana::CameraComponent{
        .projection = mirakana::CameraProjectionMode::perspective,
        .vertical_fov_radians = 1.0F,
        .orthographic_height = 10.0F,
        .near_plane = 0.1F,
        .far_plane = 500.0F,
        .primary = true,
    };
    scene.set_components(camera_node, camera_components);

    mirakana::SceneNodeComponents light_components;
    light_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::directional,
        .color = mirakana::Vec3{.x = 1.0F, .y = 0.9F, .z = 0.7F},
        .intensity = 2.0F,
        .range = 100.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(light_node, light_components);

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    scene.set_components(mesh_node, mesh_components);

    mirakana::SceneNodeComponents hidden_mesh_components;
    hidden_mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/debug"),
        .material = mirakana::AssetId::from_name("materials/debug"),
        .visible = false,
    };
    scene.set_components(hidden_mesh_node, hidden_mesh_components);

    mirakana::SceneNodeComponents sprite_components;
    sprite_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("sprites/nameplate"),
        .material = mirakana::AssetId::from_name("materials/sprite"),
        .size = mirakana::Vec2{.x = 3.0F, .y = 0.5F},
        .tint = {0.2F, 0.4F, 0.6F, 1.0F},
        .visible = true,
    };
    scene.set_components(sprite_node, sprite_components);

    mirakana::SceneNodeComponents hidden_sprite_components;
    hidden_sprite_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("sprites/hidden"),
        .material = mirakana::AssetId::from_name("materials/sprite"),
        .size = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .tint = {1.0F, 1.0F, 1.0F, 1.0F},
        .visible = false,
    };
    scene.set_components(hidden_sprite_node, hidden_sprite_components);

    const auto packet = mirakana::build_scene_render_packet(scene);

    MK_REQUIRE(packet.cameras.size() == 1);
    MK_REQUIRE(packet.lights.size() == 1);
    MK_REQUIRE(packet.meshes.size() == 1);
    MK_REQUIRE(packet.sprites.size() == 1);
    MK_REQUIRE(packet.primary_camera() != nullptr);
    MK_REQUIRE(packet.primary_camera()->node == camera_node);
    MK_REQUIRE(packet.meshes[0].node == mesh_node);
    MK_REQUIRE(packet.meshes[0].renderer.mesh == mirakana::AssetId::from_name("meshes/player"));
    MK_REQUIRE(packet.meshes[0].renderer.material == mirakana::AssetId::from_name("materials/player"));
    MK_REQUIRE(packet.sprites[0].node == sprite_node);
    MK_REQUIRE(packet.sprites[0].renderer.sprite == mirakana::AssetId::from_name("sprites/nameplate"));
    const auto camera_world_origin =
        mirakana::transform_point(packet.cameras[0].world_from_node, mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
    const auto mesh_world_origin =
        mirakana::transform_point(packet.meshes[0].world_from_node, mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
    const auto sprite_world_origin =
        mirakana::transform_point(packet.sprites[0].world_from_node, mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
    MK_REQUIRE(std::abs(camera_world_origin.x - 10.0F) < 0.0001F);
    MK_REQUIRE(std::abs(camera_world_origin.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(mesh_world_origin.x - 4.0F) < 0.0001F);
    MK_REQUIRE(std::abs(mesh_world_origin.y) < 0.0001F);
    MK_REQUIRE(std::abs(sprite_world_origin.x - 8.0F) < 0.0001F);
    MK_REQUIRE(std::abs(sprite_world_origin.y - 4.0F) < 0.0001F);
}

MK_TEST("prefab instantiates hierarchy with components deterministically") {
    mirakana::Scene scene("level-01");
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";

    mirakana::PrefabNodeTemplate root;
    root.name = "PlayerRoot";
    root.transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    root.components.camera = mirakana::CameraComponent{
        .projection = mirakana::CameraProjectionMode::orthographic,
        .vertical_fov_radians = 1.0F,
        .orthographic_height = 12.0F,
        .near_plane = 0.1F,
        .far_plane = 250.0F,
        .primary = true,
    };
    prefab.nodes.push_back(root);

    mirakana::PrefabNodeTemplate child;
    child.name = "Body";
    child.parent_index = 1;
    child.components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    prefab.nodes.push_back(child);

    MK_REQUIRE(mirakana::is_valid_prefab_definition(prefab));

    const auto instance = mirakana::instantiate_prefab(scene, prefab);

    MK_REQUIRE(instance.nodes.size() == 2);
    MK_REQUIRE(scene.nodes().size() == 2);
    MK_REQUIRE(scene.find_node(instance.nodes[0])->name == "PlayerRoot");
    MK_REQUIRE(scene.find_node(instance.nodes[0])->transform.position == (mirakana::Vec3{1.0F, 2.0F, 3.0F}));
    MK_REQUIRE(scene.find_node(instance.nodes[1])->parent == instance.nodes[0]);
    MK_REQUIRE(scene.find_node(instance.nodes[1])->components.mesh_renderer.has_value());
}

MK_TEST("prefab instantiation records durable source links") {
    mirakana::Scene scene("level-01");
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";

    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    prefab.nodes.push_back(root);

    mirakana::PrefabNodeTemplate child;
    child.name = "Weapon";
    child.parent_index = 1;
    prefab.nodes.push_back(child);

    const auto instance = mirakana::instantiate_prefab(
        scene, mirakana::PrefabInstantiateDesc{.prefab = prefab, .source_path = "assets/prefabs/player.prefab"});

    MK_REQUIRE(instance.nodes.size() == 2);
    const auto* player = scene.find_node(instance.nodes[0]);
    const auto* weapon = scene.find_node(instance.nodes[1]);
    MK_REQUIRE(player != nullptr);
    MK_REQUIRE(weapon != nullptr);
    MK_REQUIRE(player->prefab_source.has_value());
    MK_REQUIRE(weapon->prefab_source.has_value());
    MK_REQUIRE(player->prefab_source->prefab_name == "player.prefab");
    MK_REQUIRE(player->prefab_source->prefab_path == "assets/prefabs/player.prefab");
    MK_REQUIRE(player->prefab_source->source_node_index == 1);
    MK_REQUIRE(player->prefab_source->source_node_name == "Player");
    MK_REQUIRE(weapon->prefab_source->source_node_index == 2);
    MK_REQUIRE(weapon->prefab_source->source_node_name == "Weapon");
}

MK_TEST("prefab builds from scene subtree with deterministic parent indices") {
    mirakana::Scene scene("Prefab Source");
    const auto root = scene.create_node("Player");
    const auto child = scene.create_node("Weapon");
    scene.find_node(root)->transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    scene.find_node(child)->transform.rotation_radians = mirakana::Vec3{.x = 0.1F, .y = 0.2F, .z = 0.3F};
    scene.set_parent(child, root);

    mirakana::SceneNodeComponents root_components;
    root_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    scene.set_components(root, root_components);

    const auto prefab = mirakana::build_prefab_from_scene_subtree(scene, root, "player.prefab");

    MK_REQUIRE(prefab.has_value());
    MK_REQUIRE(prefab->name == "player.prefab");
    MK_REQUIRE(prefab->nodes.size() == 2);
    MK_REQUIRE(prefab->nodes[0].name == "Player");
    MK_REQUIRE(prefab->nodes[0].parent_index == 0);
    MK_REQUIRE(prefab->nodes[0].transform.position == (mirakana::Vec3{1.0F, 2.0F, 3.0F}));
    MK_REQUIRE(prefab->nodes[0].components.mesh_renderer.has_value());
    MK_REQUIRE(prefab->nodes[1].name == "Weapon");
    MK_REQUIRE(prefab->nodes[1].parent_index == 1);
    MK_REQUIRE(prefab->nodes[1].transform.rotation_radians == (mirakana::Vec3{0.1F, 0.2F, 0.3F}));
}

MK_TEST("scene serializes restores and validates prefab source links") {
    mirakana::Scene scene("level-01");
    const auto player = scene.create_node("Player");
    auto* player_node = scene.find_node(player);
    player_node->prefab_source = mirakana::ScenePrefabSourceLink{
        .prefab_name = "player.prefab",
        .prefab_path = "assets/prefabs/player.prefab",
        .source_node_index = 1,
        .source_node_name = "Player",
    };

    const auto serialized = mirakana::serialize_scene(scene);
    MK_REQUIRE(serialized.contains("node.1.prefab_source.prefab_name=player.prefab"));
    MK_REQUIRE(serialized.contains("node.1.prefab_source.prefab_path=assets/prefabs/player.prefab"));
    MK_REQUIRE(serialized.contains("node.1.prefab_source.source_node_index=1"));
    MK_REQUIRE(serialized.contains("node.1.prefab_source.source_node_name=Player"));

    const auto restored = mirakana::deserialize_scene(serialized);
    const auto* restored_player = restored.find_node(player);
    MK_REQUIRE(restored_player != nullptr);
    MK_REQUIRE(restored_player->prefab_source.has_value());
    MK_REQUIRE(restored_player->prefab_source->prefab_path == "assets/prefabs/player.prefab");
    MK_REQUIRE(restored_player->prefab_source->source_node_name == "Player");

    bool rejected = false;
    try {
        (void)mirakana::deserialize_scene("format=GameEngine.Scene.v1\n"
                                          "scene.name=broken\n"
                                          "node.count=1\n"
                                          "node.1.name=Player\n"
                                          "node.1.parent=0\n"
                                          "node.1.position=0,0,0\n"
                                          "node.1.scale=1,1,1\n"
                                          "node.1.rotation=0,0,0\n"
                                          "node.1.prefab_source.prefab_name=player.prefab\n"
                                          "node.1.prefab_source.source_node_name=Player\n");
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("prefab build from scene subtree rejects missing root invalid name or child cycles") {
    mirakana::Scene scene("Prefab Source");
    const auto root = scene.create_node("Player");
    const auto child = scene.create_node("Weapon");
    scene.set_parent(child, root);

    MK_REQUIRE(!mirakana::build_prefab_from_scene_subtree(scene, root, "").has_value());
    MK_REQUIRE(
        !mirakana::build_prefab_from_scene_subtree(scene, mirakana::SceneNodeId{99}, "missing.prefab").has_value());

    scene.find_node(child)->children.push_back(root);
    MK_REQUIRE(!mirakana::build_prefab_from_scene_subtree(scene, root, "cycle.prefab").has_value());
}

MK_TEST("prefab definition serializes deterministically and rejects malformed payloads") {
    mirakana::PrefabDefinition prefab;
    prefab.name = "player.prefab";

    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    root.transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    root.components.camera = mirakana::CameraComponent{};
    prefab.nodes.push_back(root);

    mirakana::PrefabNodeTemplate child;
    child.name = "Body";
    child.parent_index = 1;
    child.components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    prefab.nodes.push_back(child);

    mirakana::PrefabNodeTemplate grandchild;
    grandchild.name = "Weapon";
    grandchild.parent_index = 2;
    prefab.nodes.push_back(grandchild);

    const auto serialized = mirakana::serialize_prefab_definition(prefab);
    MK_REQUIRE(serialized.starts_with("format=GameEngine.Prefab.v1\n"));
    MK_REQUIRE(serialized.contains("prefab.name=player.prefab\n"));
    MK_REQUIRE(serialized.contains("node.2.parent=1\n"));
    MK_REQUIRE(serialized.contains("node.3.parent=2\n"));
    MK_REQUIRE(serialized.contains("node.2.mesh_renderer.mesh="));

    const auto restored = mirakana::deserialize_prefab_definition(serialized);
    MK_REQUIRE(restored.name == "player.prefab");
    MK_REQUIRE(restored.nodes.size() == 3);
    MK_REQUIRE(restored.nodes[0].name == "Player");
    MK_REQUIRE(restored.nodes[0].components.camera.has_value());
    MK_REQUIRE(restored.nodes[1].parent_index == 1);
    MK_REQUIRE(restored.nodes[1].components.mesh_renderer.has_value());
    MK_REQUIRE(restored.nodes[2].parent_index == 2);

    bool rejected_format = false;
    try {
        (void)mirakana::deserialize_prefab_definition("format=GameEngine.Prefab.v0\n"
                                                      "prefab.name=broken.prefab\n"
                                                      "node.count=0\n");
    } catch (const std::invalid_argument&) {
        rejected_format = true;
    }
    MK_REQUIRE(rejected_format);

    bool rejected_missing_field = false;
    try {
        (void)mirakana::deserialize_prefab_definition("format=GameEngine.Prefab.v1\n"
                                                      "prefab.name=broken.prefab\n"
                                                      "node.count=1\n"
                                                      "node.1.name=Child\n"
                                                      "node.1.parent=0\n"
                                                      "node.1.position=0,0,0\n"
                                                      "node.1.scale=1,1,1\n");
    } catch (const std::invalid_argument&) {
        rejected_missing_field = true;
    }
    MK_REQUIRE(rejected_missing_field);

    bool rejected_invalid_component = false;
    try {
        (void)mirakana::deserialize_prefab_definition("format=GameEngine.Prefab.v1\n"
                                                      "prefab.name=broken.prefab\n"
                                                      "node.count=1\n"
                                                      "node.1.name=Child\n"
                                                      "node.1.parent=0\n"
                                                      "node.1.position=0,0,0\n"
                                                      "node.1.scale=1,1,1\n"
                                                      "node.1.rotation=0,0,0\n"
                                                      "node.1.mesh_renderer.mesh=0\n"
                                                      "node.1.mesh_renderer.material=1\n"
                                                      "node.1.mesh_renderer.visible=true\n");
    } catch (const std::invalid_argument&) {
        rejected_invalid_component = true;
    }
    MK_REQUIRE(rejected_invalid_component);

    bool rejected_parent = false;
    try {
        (void)mirakana::deserialize_prefab_definition("format=GameEngine.Prefab.v1\n"
                                                      "prefab.name=broken.prefab\n"
                                                      "node.count=1\n"
                                                      "node.1.name=Child\n"
                                                      "node.1.parent=2\n"
                                                      "node.1.position=0,0,0\n"
                                                      "node.1.scale=1,1,1\n"
                                                      "node.1.rotation=0,0,0\n");
    } catch (const std::invalid_argument&) {
        rejected_parent = true;
    }
    MK_REQUIRE(rejected_parent);
}

MK_TEST("prefab variant composes node name transform and component overrides") {
    mirakana::PrefabDefinition base;
    base.name = "player.prefab";

    mirakana::PrefabNodeTemplate root;
    root.name = "Player";
    root.transform.position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F};
    base.nodes.push_back(root);

    mirakana::PrefabNodeTemplate child;
    child.name = "Body";
    child.parent_index = 1;
    child.transform.scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    child.components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/player"),
        .material = mirakana::AssetId::from_name("materials/player"),
        .visible = true,
    };
    base.nodes.push_back(child);

    mirakana::PrefabVariantDefinition variant;
    variant.name = "elite-player.prefabvariant";
    variant.base_prefab = base;

    mirakana::PrefabNodeOverride root_name;
    root_name.node_index = 1;
    root_name.kind = mirakana::PrefabOverrideKind::name;
    root_name.name = "ElitePlayer";
    variant.overrides.push_back(root_name);

    mirakana::PrefabNodeOverride child_transform;
    child_transform.node_index = 2;
    child_transform.kind = mirakana::PrefabOverrideKind::transform;
    child_transform.transform.position = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F};
    child_transform.transform.scale = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F};
    child_transform.transform.rotation_radians = mirakana::Vec3{.x = 0.0F, .y = 0.25F, .z = 0.5F};
    variant.overrides.push_back(child_transform);

    mirakana::SceneNodeComponents upgraded_components;
    upgraded_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId::from_name("meshes/elite-player"),
        .material = mirakana::AssetId::from_name("materials/elite-player"),
        .visible = false,
    };
    mirakana::PrefabNodeOverride child_components;
    child_components.node_index = 2;
    child_components.kind = mirakana::PrefabOverrideKind::components;
    child_components.components = upgraded_components;
    variant.overrides.push_back(child_components);

    const auto result = mirakana::compose_prefab_variant(variant);

    MK_REQUIRE(result.success);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.prefab.name == "elite-player.prefabvariant");
    MK_REQUIRE(result.prefab.nodes.size() == 2);
    MK_REQUIRE(result.prefab.nodes[0].name == "ElitePlayer");
    MK_REQUIRE(result.prefab.nodes[0].parent_index == 0);
    MK_REQUIRE(result.prefab.nodes[1].parent_index == 1);
    MK_REQUIRE(result.prefab.nodes[1].transform.position == (mirakana::Vec3{4.0F, 5.0F, 6.0F}));
    MK_REQUIRE(result.prefab.nodes[1].transform.scale == (mirakana::Vec3{2.0F, 2.0F, 2.0F}));
    MK_REQUIRE(result.prefab.nodes[1].transform.rotation_radians == (mirakana::Vec3{0.0F, 0.25F, 0.5F}));
    MK_REQUIRE(result.prefab.nodes[1].components.mesh_renderer.has_value());
    MK_REQUIRE(result.prefab.nodes[1].components.mesh_renderer->mesh ==
               mirakana::AssetId::from_name("meshes/elite-player"));
    MK_REQUIRE(!result.prefab.nodes[1].components.mesh_renderer->visible);
    MK_REQUIRE(mirakana::is_valid_prefab_definition(result.prefab));
}

MK_TEST("prefab variant validation reports invalid overrides deterministically") {
    mirakana::PrefabVariantDefinition invalid_base;
    invalid_base.name = "broken.prefabvariant";
    invalid_base.base_prefab.name = "empty.prefab";

    const auto invalid_base_diagnostics = mirakana::validate_prefab_variant_definition(invalid_base);
    MK_REQUIRE(!invalid_base_diagnostics.empty());
    MK_REQUIRE(invalid_base_diagnostics[0].kind == mirakana::PrefabVariantDiagnosticKind::invalid_base_prefab);

    mirakana::PrefabDefinition base;
    base.name = "base.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Root";
    base.nodes.push_back(root);

    mirakana::PrefabVariantDefinition variant;
    variant.name = "variant.prefabvariant";
    variant.base_prefab = base;

    mirakana::PrefabNodeOverride missing_node;
    missing_node.node_index = 2;
    missing_node.kind = mirakana::PrefabOverrideKind::name;
    missing_node.name = "Missing";
    variant.overrides.push_back(missing_node);

    mirakana::PrefabNodeOverride duplicate_a;
    duplicate_a.node_index = 1;
    duplicate_a.kind = mirakana::PrefabOverrideKind::name;
    duplicate_a.name = "A";
    variant.overrides.push_back(duplicate_a);

    mirakana::PrefabNodeOverride duplicate_b;
    duplicate_b.node_index = 1;
    duplicate_b.kind = mirakana::PrefabOverrideKind::name;
    duplicate_b.name = "B";
    variant.overrides.push_back(duplicate_b);

    mirakana::PrefabNodeOverride invalid_name;
    invalid_name.node_index = 1;
    invalid_name.kind = mirakana::PrefabOverrideKind::name;
    invalid_name.name = "Bad=Name";
    variant.overrides.push_back(invalid_name);

    mirakana::SceneNodeComponents invalid_components;
    invalid_components.mesh_renderer = mirakana::MeshRendererComponent{
        .mesh = mirakana::AssetId{},
        .material = mirakana::AssetId::from_name("materials/variant"),
        .visible = true,
    };
    mirakana::PrefabNodeOverride bad_components;
    bad_components.node_index = 1;
    bad_components.kind = mirakana::PrefabOverrideKind::components;
    bad_components.components = invalid_components;
    variant.overrides.push_back(bad_components);

    const auto diagnostics = mirakana::validate_prefab_variant_definition(variant);
    MK_REQUIRE(diagnostics.size() == 4);
    MK_REQUIRE(diagnostics[0].kind == mirakana::PrefabVariantDiagnosticKind::invalid_node_index);
    MK_REQUIRE(diagnostics[1].kind == mirakana::PrefabVariantDiagnosticKind::duplicate_override);
    MK_REQUIRE(diagnostics[2].kind == mirakana::PrefabVariantDiagnosticKind::invalid_node_name);
    MK_REQUIRE(diagnostics[3].kind == mirakana::PrefabVariantDiagnosticKind::invalid_components);
    MK_REQUIRE(!mirakana::compose_prefab_variant(variant).success);
}

MK_TEST("prefab variant validation rejects unsupported kinds and malformed transforms") {
    mirakana::PrefabDefinition base;
    base.name = "base.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Root";
    base.nodes.push_back(root);

    mirakana::PrefabVariantDefinition variant;
    variant.name = "variant.prefabvariant";
    variant.base_prefab = base;

    mirakana::PrefabNodeOverride invalid_kind;
    invalid_kind.node_index = 1;
    invalid_kind.kind = static_cast<mirakana::PrefabOverrideKind>(99);
    variant.overrides.push_back(invalid_kind);

    mirakana::PrefabNodeOverride invalid_transform;
    invalid_transform.node_index = 1;
    invalid_transform.kind = mirakana::PrefabOverrideKind::transform;
    invalid_transform.transform.position =
        mirakana::Vec3{.x = std::numeric_limits<float>::infinity(), .y = 0.0F, .z = 0.0F};
    invalid_transform.transform.scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    invalid_transform.transform.rotation_radians = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    variant.overrides.push_back(invalid_transform);

    const auto diagnostics = mirakana::validate_prefab_variant_definition(variant);
    MK_REQUIRE(diagnostics.size() == 2);
    MK_REQUIRE(diagnostics[0].kind == mirakana::PrefabVariantDiagnosticKind::invalid_override_kind);
    MK_REQUIRE(mirakana::prefab_override_kind_label(diagnostics[0].override_kind) == "unknown");
    MK_REQUIRE(diagnostics[1].kind == mirakana::PrefabVariantDiagnosticKind::invalid_transform);
    MK_REQUIRE(!mirakana::compose_prefab_variant(variant).success);

    bool rejected_serialization = false;
    try {
        (void)mirakana::serialize_prefab_variant_definition(variant);
    } catch (const std::invalid_argument&) {
        rejected_serialization = true;
    }
    MK_REQUIRE(rejected_serialization);
}

MK_TEST("prefab variant serializes deterministically and round trips") {
    mirakana::PrefabDefinition base;
    base.name = "base.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Root";
    base.nodes.push_back(root);
    mirakana::PrefabNodeTemplate child;
    child.name = "Sprite";
    child.parent_index = 1;
    base.nodes.push_back(child);

    mirakana::PrefabVariantDefinition variant;
    variant.name = "sprite-variant.prefabvariant";
    variant.base_prefab = base;

    mirakana::PrefabNodeOverride rename;
    rename.node_index = 1;
    rename.kind = mirakana::PrefabOverrideKind::name;
    rename.name = "RenamedRoot";
    variant.overrides.push_back(rename);

    mirakana::SceneNodeComponents sprite_components;
    sprite_components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = mirakana::AssetId::from_name("textures/sprite"),
        .material = mirakana::AssetId::from_name("materials/sprite"),
        .size = mirakana::Vec2{.x = 3.0F, .y = 4.0F},
        .tint = {0.25F, 0.5F, 0.75F, 1.0F},
        .visible = true,
    };
    mirakana::PrefabNodeOverride sprite_override;
    sprite_override.node_index = 2;
    sprite_override.kind = mirakana::PrefabOverrideKind::components;
    sprite_override.components = sprite_components;
    variant.overrides.push_back(sprite_override);

    const auto serialized = mirakana::serialize_prefab_variant_definition(variant);
    MK_REQUIRE(serialized.starts_with("format=GameEngine.PrefabVariant.v1\n"));
    MK_REQUIRE(serialized.contains("variant.name=sprite-variant.prefabvariant\n"));
    MK_REQUIRE(serialized.contains("base.format=GameEngine.Prefab.v1\n"));
    MK_REQUIRE(serialized.contains("override.count=2\n"));
    MK_REQUIRE(serialized.contains("override.2.sprite_renderer.sprite="));

    const auto restored = mirakana::deserialize_prefab_variant_definition(serialized);
    MK_REQUIRE(restored.name == variant.name);
    MK_REQUIRE(restored.base_prefab.name == "base.prefab");
    MK_REQUIRE(restored.overrides.size() == 2);
    MK_REQUIRE(restored.overrides[0].kind == mirakana::PrefabOverrideKind::name);
    MK_REQUIRE(restored.overrides[0].name == "RenamedRoot");
    MK_REQUIRE(restored.overrides[1].kind == mirakana::PrefabOverrideKind::components);
    MK_REQUIRE(restored.overrides[1].components.sprite_renderer.has_value());

    const auto composed = mirakana::compose_prefab_variant(restored);
    MK_REQUIRE(composed.success);
    MK_REQUIRE(composed.prefab.nodes[0].name == "RenamedRoot");
    MK_REQUIRE(composed.prefab.nodes[1].components.sprite_renderer.has_value());
    MK_REQUIRE(composed.prefab.nodes[1].components.sprite_renderer->size == (mirakana::Vec2{3.0F, 4.0F}));
}

MK_TEST("prefab variant preserves source node name hints") {
    const std::string text = "format=GameEngine.PrefabVariant.v1\n"
                             "variant.name=camera-variant.prefabvariant\n"
                             "base.format=GameEngine.Prefab.v1\n"
                             "base.prefab.name=camera.prefab\n"
                             "base.node.count=2\n"
                             "base.node.1.name=Root\n"
                             "base.node.1.parent=0\n"
                             "base.node.1.position=0,0,0\n"
                             "base.node.1.scale=1,1,1\n"
                             "base.node.1.rotation=0,0,0\n"
                             "base.node.2.name=Camera\n"
                             "base.node.2.parent=1\n"
                             "base.node.2.position=0,0,0\n"
                             "base.node.2.scale=1,1,1\n"
                             "base.node.2.rotation=0,0,0\n"
                             "override.count=1\n"
                             "override.1.node=1\n"
                             "override.1.kind=transform\n"
                             "override.1.source_node_name=Camera\n"
                             "override.1.position=1,2,3\n"
                             "override.1.scale=1,1,1\n"
                             "override.1.rotation=0,0.5,0\n";

    const auto variant = mirakana::deserialize_prefab_variant_definition(text);
    const auto serialized = mirakana::serialize_prefab_variant_definition(variant);

    MK_REQUIRE(serialized.contains("override.1.source_node_name=Camera\n"));
    MK_REQUIRE(serialized.contains("override.1.position=1,2,3\n"));

    const auto round_trip = mirakana::deserialize_prefab_variant_definition_for_review(serialized);
    MK_REQUIRE(mirakana::serialize_prefab_variant_definition(round_trip) == serialized);
}

MK_TEST("prefab variant deserialization rejects fields outside the declared override kind") {
    mirakana::PrefabDefinition base;
    base.name = "base.prefab";
    mirakana::PrefabNodeTemplate root;
    root.name = "Root";
    base.nodes.push_back(root);

    mirakana::PrefabVariantDefinition variant;
    variant.name = "variant.prefabvariant";
    variant.base_prefab = base;

    mirakana::PrefabNodeOverride rename;
    rename.node_index = 1;
    rename.kind = mirakana::PrefabOverrideKind::name;
    rename.name = "RenamedRoot";
    variant.overrides.push_back(rename);

    const auto serialized = mirakana::serialize_prefab_variant_definition(variant);
    const auto malformed = serialized + "override.1.position=0,0,0\n";

    bool rejected = false;
    try {
        (void)mirakana::deserialize_prefab_variant_definition(malformed);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("scene parenting rejects cycles") {
    mirakana::Scene scene("Cycle Guard");
    const auto root = scene.create_node("Root");
    const auto child = scene.create_node("Child");
    const auto grandchild = scene.create_node("Grandchild");
    const mirakana::SceneNodeId child_node = child;
    const mirakana::SceneNodeId parent_root = root;
    // NOLINTNEXTLINE(bugprone-swapped-arguments)
    scene.set_parent(child_node, parent_root);
    const mirakana::SceneNodeId grandchild_node = grandchild;
    const mirakana::SceneNodeId parent_child = child;
    // NOLINTNEXTLINE
    scene.set_parent(grandchild_node, parent_child);

    bool rejected_cycle = false;
    try {
        const mirakana::SceneNodeId cycle_child = root;
        const mirakana::SceneNodeId cycle_parent = grandchild;
        // NOLINTNEXTLINE(bugprone-swapped-arguments)
        scene.set_parent(cycle_child, cycle_parent);
    } catch (const std::invalid_argument&) {
        rejected_cycle = true;
    }

    MK_REQUIRE(rejected_cycle);
    MK_REQUIRE(scene.find_node(root)->parent == mirakana::null_scene_node);
    MK_REQUIRE(scene.find_node(grandchild)->children.empty());
}

MK_TEST("physics world integrates dynamic bodies deterministically") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = -10.0F}});
    const auto body = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 2.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });

    world.apply_force(body, mirakana::Vec2{.x = 4.0F, .y = 0.0F});
    world.step(0.5F);

    const auto* state = world.find_body(body);
    MK_REQUIRE(state != nullptr);
    MK_REQUIRE(std::abs(state->velocity.x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(state->velocity.y - -5.0F) < 0.0001F);
    MK_REQUIRE(std::abs(state->position.x - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(state->position.y - -2.5F) < 0.0001F);
}

MK_TEST("2d physics broadphase filters pairs by overlap and collision masks") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    constexpr std::uint32_t player_layer = 1U << 0U;
    constexpr std::uint32_t terrain_layer = 1U << 1U;
    constexpr std::uint32_t trigger_layer = 1U << 2U;

    const auto player = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = player_layer,
        .collision_mask = terrain_layer,
    });
    const auto terrain = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 1.5F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = terrain_layer,
        .collision_mask = player_layer,
    });
    const auto trigger = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.25F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = player_layer,
    });
    MK_REQUIRE(trigger != mirakana::null_physics_body_2d);

    const auto pairs = world.broadphase_pairs();

    MK_REQUIRE(pairs.size() == 1);
    MK_REQUIRE(pairs[0].first == player);
    MK_REQUIRE(pairs[0].second == terrain);
}

MK_TEST("2d physics raycast hits nearest collision bounds and reports surface data") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto far = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 5.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
    });
    const auto near = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 2.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
    });
    MK_REQUIRE(far != mirakana::null_physics_body_2d);

    const auto hit = world.raycast(mirakana::PhysicsRaycast2DDesc{
        .origin = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .direction = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .max_distance = 10.0F,
    });

    MK_REQUIRE(hit.has_value());
    MK_REQUIRE(hit->body == near);
    MK_REQUIRE(std::abs(hit->distance - 1.5F) < 0.0001F);
    MK_REQUIRE(hit->point == (mirakana::Vec2{1.5F, 0.0F}));
    MK_REQUIRE(hit->normal == (mirakana::Vec2{-1.0F, 0.0F}));
}

MK_TEST("2d physics raycast respects collision masks and disabled bodies") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    constexpr std::uint32_t terrain_layer = 1U << 1U;
    constexpr std::uint32_t sensor_layer = 1U << 2U;

    const auto disabled = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = false,
    });
    const auto sensor = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 2.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = sensor_layer,
    });
    const auto terrain = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 3.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = terrain_layer,
    });
    MK_REQUIRE(disabled != mirakana::null_physics_body_2d);
    MK_REQUIRE(sensor != mirakana::null_physics_body_2d);

    const auto hit = world.raycast(mirakana::PhysicsRaycast2DDesc{
        .origin = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .direction = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .max_distance = 10.0F,
        .collision_mask = terrain_layer,
    });

    MK_REQUIRE(hit.has_value());
    MK_REQUIRE(hit->body == terrain);
}

MK_TEST("2d physics reports trigger overlaps without contact resolution") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    constexpr std::uint32_t player_layer = 1U << 0U;
    constexpr std::uint32_t trigger_layer = 1U << 1U;
    constexpr std::uint32_t terrain_layer = 1U << 2U;

    const auto player = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = player_layer,
        .collision_mask = trigger_layer,
    });
    const auto trigger = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.25F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = player_layer,
        .trigger = true,
    });
    const auto filtered = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.25F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = terrain_layer,
        .trigger = true,
    });
    const auto disabled = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.25F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = false,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = player_layer,
        .trigger = true,
    });
    MK_REQUIRE(filtered != mirakana::null_physics_body_2d);
    MK_REQUIRE(disabled != mirakana::null_physics_body_2d);

    const auto overlaps = world.trigger_overlaps();
    MK_REQUIRE(overlaps.size() == 1);
    MK_REQUIRE(overlaps[0].first == player);
    MK_REQUIRE(overlaps[0].second == trigger);
    MK_REQUIRE(world.contacts().empty());

    world.resolve_contacts(mirakana::PhysicsContactSolver2DConfig{.restitution = 0.0F, .iterations = 4U});
    MK_REQUIRE(world.find_body(player)->position == (mirakana::Vec2{0.0F, 0.0F}));
    MK_REQUIRE(world.find_body(trigger)->position == (mirakana::Vec2{0.25F, 0.0F}));

    const auto replay = world.trigger_overlaps();
    MK_REQUIRE(replay.size() == overlaps.size());
    MK_REQUIRE(replay[0].first == overlaps[0].first);
    MK_REQUIRE(replay[0].second == overlaps[0].second);

    mirakana::PhysicsWorld2D trigger_first_world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto early_trigger = trigger_first_world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = player_layer | trigger_layer,
        .trigger = true,
    });
    const auto late_body = trigger_first_world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.25F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = player_layer,
        .collision_mask = trigger_layer,
    });
    const auto trigger_first_pairs = trigger_first_world.trigger_overlaps();
    MK_REQUIRE(trigger_first_pairs.size() == 1);
    MK_REQUIRE(trigger_first_pairs[0].first == early_trigger);
    MK_REQUIRE(trigger_first_pairs[0].second == late_body);

    mirakana::PhysicsWorld2D trigger_pair_world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto first_trigger = trigger_pair_world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = trigger_layer,
        .trigger = true,
    });
    const auto second_trigger = trigger_pair_world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.25F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = trigger_layer,
        .trigger = true,
    });
    const auto trigger_trigger_pairs = trigger_pair_world.trigger_overlaps();
    MK_REQUIRE(trigger_trigger_pairs.size() == 1);
    MK_REQUIRE(trigger_trigger_pairs[0].first == first_trigger);
    MK_REQUIRE(trigger_trigger_pairs[0].second == second_trigger);
    MK_REQUIRE(trigger_pair_world.contacts().empty());
}

MK_TEST("2d physics shape sweep reports nearest initial and filtered hits") {
    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    constexpr std::uint32_t terrain_layer = 1U << 1U;
    constexpr std::uint32_t sensor_layer = 1U << 2U;

    const auto sensor = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 2.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = sensor_layer,
        .collision_mask = 0xFFFF'FFFFU,
        .trigger = true,
    });
    const auto near = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 4.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = terrain_layer,
        .collision_mask = 0xFFFF'FFFFU,
    });
    const auto far = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 6.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .radius = 0.5F,
        .collision_layer = terrain_layer,
        .collision_mask = 0xFFFF'FFFFU,
    });

    const auto hit = world.shape_sweep(mirakana::PhysicsShapeSweep2DDesc{
        .origin = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .direction = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .radius = 0.5F,
        .collision_mask = terrain_layer | sensor_layer,
        .ignored_body = mirakana::null_physics_body_2d,
        .include_triggers = false,
    });

    MK_REQUIRE(sensor != mirakana::null_physics_body_2d);
    MK_REQUIRE(hit.has_value());
    MK_REQUIRE(hit->body == near);
    MK_REQUIRE(!hit->initial_overlap);
    MK_REQUIRE(std::abs(hit->distance - 3.0F) < 0.0001F);
    MK_REQUIRE(hit->position == (mirakana::Vec2{3.0F, 0.0F}));
    MK_REQUIRE(hit->normal == (mirakana::Vec2{-1.0F, 0.0F}));

    const auto with_triggers = world.shape_sweep(mirakana::PhysicsShapeSweep2DDesc{
        .origin = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .direction = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .radius = 0.5F,
        .collision_mask = terrain_layer | sensor_layer,
    });
    MK_REQUIRE(with_triggers.has_value());
    MK_REQUIRE(with_triggers->body == sensor);
    MK_REQUIRE(std::abs(with_triggers->distance - 1.0F) < 0.0001F);

    const auto initial = world.shape_sweep(mirakana::PhysicsShapeSweep2DDesc{
        .origin = mirakana::Vec2{.x = 3.75F, .y = 0.0F},
        .direction = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .radius = 0.5F,
        .collision_mask = terrain_layer,
        .ignored_body = mirakana::null_physics_body_2d,
        .include_triggers = false,
    });
    MK_REQUIRE(initial.has_value());
    MK_REQUIRE(initial->body == near);
    MK_REQUIRE(initial->initial_overlap);
    MK_REQUIRE(initial->distance == 0.0F);
    MK_REQUIRE(initial->position == (mirakana::Vec2{3.75F, 0.0F}));

    const auto zero_direction_initial = world.shape_sweep(mirakana::PhysicsShapeSweep2DDesc{
        .origin = mirakana::Vec2{.x = 3.75F, .y = 0.0F},
        .direction = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .max_distance = 0.0F,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .radius = 0.0F,
        .collision_mask = terrain_layer,
        .ignored_body = mirakana::null_physics_body_2d,
        .include_triggers = false,
    });
    MK_REQUIRE(zero_direction_initial.has_value());
    MK_REQUIRE(zero_direction_initial->body == near);
    MK_REQUIRE(zero_direction_initial->initial_overlap);

    const auto circle_sweep = world.shape_sweep(mirakana::PhysicsShapeSweep2DDesc{
        .origin = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .direction = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape2DKind::circle,
        .half_extents = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .radius = 0.5F,
        .collision_mask = terrain_layer,
        .ignored_body = mirakana::null_physics_body_2d,
        .include_triggers = false,
    });
    MK_REQUIRE(circle_sweep.has_value());
    MK_REQUIRE(circle_sweep->body == near);

    const auto ignored = world.shape_sweep(mirakana::PhysicsShapeSweep2DDesc{
        .origin = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .direction = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape2DKind::aabb,
        .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
        .radius = 0.5F,
        .collision_mask = terrain_layer,
        .ignored_body = near,
        .include_triggers = false,
    });
    MK_REQUIRE(ignored.has_value());
    MK_REQUIRE(ignored->body == far);

    MK_REQUIRE(!world
                    .shape_sweep(mirakana::PhysicsShapeSweep2DDesc{
                        mirakana::Vec2{0.0F, 0.0F},
                        mirakana::Vec2{0.0F, 0.0F},
                        10.0F,
                        mirakana::PhysicsShape2DKind::aabb,
                        mirakana::Vec2{0.5F, 0.5F},
                        0.5F,
                        terrain_layer,
                    })
                    .has_value());
    MK_REQUIRE(!world
                    .shape_sweep(mirakana::PhysicsShapeSweep2DDesc{
                        mirakana::Vec2{0.0F, 0.0F},
                        mirakana::Vec2{1.0F, 0.0F},
                        10.0F,
                        mirakana::PhysicsShape2DKind::aabb,
                        mirakana::Vec2{0.0F, 0.5F},
                        0.5F,
                        terrain_layer,
                    })
                    .has_value());
}

MK_TEST("2d physics reports circle contacts and resolves dynamic body against static body") {
    MK_REQUIRE(!mirakana::is_valid_physics_body_desc(mirakana::PhysicsBody2DDesc{
        mirakana::Vec2{0.0F, 0.0F},
        mirakana::Vec2{0.0F, 0.0F},
        1.0F,
        0.0F,
        true,
        mirakana::Vec2{1.0F, 1.0F},
        true,
        mirakana::PhysicsShape2DKind::circle,
        0.0F,
        1U,
        1U,
    }));

    mirakana::PhysicsWorld2D world(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}});
    const auto first = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::circle,
        .radius = 1.0F,
        .collision_layer = 1U,
        .collision_mask = 1U,
    });
    const auto second = world.create_body(mirakana::PhysicsBody2DDesc{
        .position = mirakana::Vec2{.x = 1.5F, .y = 0.0F},
        .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec2{.x = 1.0F, .y = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape2DKind::circle,
        .radius = 1.0F,
        .collision_layer = 1U,
        .collision_mask = 1U,
    });

    const auto contacts = world.contacts();
    MK_REQUIRE(contacts.size() == 1);
    MK_REQUIRE(contacts[0].first == first);
    MK_REQUIRE(contacts[0].second == second);
    MK_REQUIRE(contacts[0].normal == (mirakana::Vec2{1.0F, 0.0F}));
    MK_REQUIRE(std::abs(contacts[0].penetration_depth - 0.5F) < 0.0001F);

    world.resolve_contacts(mirakana::PhysicsContactSolver2DConfig{.restitution = 0.0F, .iterations = 2U});
    MK_REQUIRE(std::abs(world.find_body(first)->position.x - -0.5F) < 0.0001F);
    MK_REQUIRE(world.find_body(second)->position == (mirakana::Vec2{1.5F, 0.0F}));
}

MK_TEST("3d physics world integrates dynamic bodies deterministically") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -10.0F, .z = 0.0F}});
    const auto body = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F},
        .mass = 2.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });

    world.apply_force(body, mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 2.0F});
    world.step(0.5F);

    const auto* state = world.find_body(body);
    MK_REQUIRE(state != nullptr);
    MK_REQUIRE(std::abs(state->velocity.x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(state->velocity.y - -5.0F) < 0.0001F);
    MK_REQUIRE(std::abs(state->velocity.z - 1.5F) < 0.0001F);
    MK_REQUIRE(std::abs(state->position.x - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(state->position.y - -2.5F) < 0.0001F);
    MK_REQUIRE(std::abs(state->position.z - 0.75F) < 0.0001F);
}

MK_TEST("3d physics continuous step reports fast body hit rows") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto wall = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.05F, .y = 2.0F, .z = 2.0F},
    });
    const auto bullet = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 20.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.1F,
    });

    const auto result = world.step_continuous(0.5F);

    MK_REQUIRE(result.status == mirakana::PhysicsContinuousStep3DStatus::stepped);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsContinuousStep3DDiagnostic::none);
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.rows[0].body == bullet);
    MK_REQUIRE(result.rows[0].hit_body == wall);
    MK_REQUIRE(result.rows[0].ccd_applied);
    MK_REQUIRE(result.rows[0].hit.has_value());
    MK_REQUIRE(result.rows[0].hit->body == wall);
    MK_REQUIRE(result.rows[0].hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(result.rows[0].previous_position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(result.rows[0].attempted_displacement == (mirakana::Vec3{10.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(result.rows[0].hit->distance - 4.85F) < 0.0001F);
    MK_REQUIRE(std::abs(result.rows[0].applied_displacement.x - 4.849F) < 0.0001F);
    MK_REQUIRE(result.rows[0].applied_displacement.y == 0.0F);
    MK_REQUIRE(result.rows[0].applied_displacement.z == 0.0F);
    MK_REQUIRE(std::abs(result.rows[0].remaining_displacement.x - 5.151F) < 0.0001F);
    MK_REQUIRE(world.find_body(bullet)->position == result.rows[0].applied_displacement);
    MK_REQUIRE(std::abs(world.find_body(bullet)->velocity.x) < 0.0001F);
}

MK_TEST("3d physics continuous step matches discrete step when no hit") {
    auto create_world = [] {
        mirakana::PhysicsWorld3D world(
            mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -2.0F, .z = 0.0F}});
        const auto body = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
            .mass = 2.0F,
            .linear_damping = 0.25F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.25F, .z = 0.25F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::sphere,
            .radius = 0.25F,
        });
        world.apply_force(body, mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F});
        return std::pair{world, body};
    };

    auto [discrete, discrete_body] = create_world();
    auto [continuous, continuous_body] = create_world();

    discrete.step(0.25F);
    const auto result = continuous.step_continuous(0.25F);

    const auto* discrete_state = discrete.find_body(discrete_body);
    const auto* continuous_state = continuous.find_body(continuous_body);
    MK_REQUIRE(discrete_state != nullptr);
    MK_REQUIRE(continuous_state != nullptr);
    MK_REQUIRE(result.status == mirakana::PhysicsContinuousStep3DStatus::stepped);
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(!result.rows[0].ccd_applied);
    MK_REQUIRE(!result.rows[0].hit.has_value());
    MK_REQUIRE(result.rows[0].hit_body == mirakana::null_physics_body_3d);
    MK_REQUIRE(result.rows[0].applied_displacement == result.rows[0].attempted_displacement);
    MK_REQUIRE(result.rows[0].remaining_displacement == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(continuous_state->position == discrete_state->position);
    MK_REQUIRE(continuous_state->velocity == discrete_state->velocity);
    MK_REQUIRE(continuous_state->accumulated_force == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
}

MK_TEST("3d physics continuous step honors filters and keeps dynamic targets out of CCD scope") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    constexpr std::uint32_t mover_layer = 1U << 0U;
    constexpr std::uint32_t wall_layer = 1U << 1U;
    constexpr std::uint32_t sensor_layer = 1U << 2U;
    const auto disabled = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = false,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = wall_layer,
    });
    const auto trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = sensor_layer,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto target_mask_rejects_mover = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 3.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = wall_layer,
        .collision_mask = sensor_layer,
    });
    const auto dynamic_target = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = wall_layer,
        .collision_mask = 0xFFFF'FFFFU,
    });
    const auto solid = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 6.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = wall_layer,
        .collision_mask = mover_layer,
    });
    const auto bullet = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 20.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.1F,
        .collision_layer = mover_layer,
        .collision_mask = wall_layer | sensor_layer,
    });

    const auto result = world.step_continuous(0.5F);

    MK_REQUIRE(disabled != mirakana::null_physics_body_3d);
    MK_REQUIRE(trigger != mirakana::null_physics_body_3d);
    MK_REQUIRE(target_mask_rejects_mover != mirakana::null_physics_body_3d);
    MK_REQUIRE(dynamic_target != mirakana::null_physics_body_3d);
    MK_REQUIRE(result.rows.size() == 2);
    MK_REQUIRE(result.rows[0].body == dynamic_target);
    MK_REQUIRE(!result.rows[0].ccd_applied);
    MK_REQUIRE(result.rows[1].body == bullet);
    MK_REQUIRE(result.rows[1].hit_body == solid);
    MK_REQUIRE(result.rows[1].ccd_applied);
    MK_REQUIRE(result.rows[1].hit.has_value());
    MK_REQUIRE(result.rows[1].hit->body == solid);
    MK_REQUIRE(world.find_body(bullet)->position.x > 5.0F);
    MK_REQUIRE(world.find_body(bullet)->position.x < 6.0F);
}

MK_TEST("3d physics continuous step rejects invalid requests without mutation") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F}});
    const auto body = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
        .velocity = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F},
        .mass = 2.0F,
        .linear_damping = 0.1F,
        .dynamic = true,
    });
    world.apply_force(body, mirakana::Vec3{.x = 7.0F, .y = 8.0F, .z = 9.0F});
    const auto before = *world.find_body(body);

    const auto invalid_delta = world.step_continuous(-0.01F);
    MK_REQUIRE(invalid_delta.status == mirakana::PhysicsContinuousStep3DStatus::invalid_request);
    MK_REQUIRE(invalid_delta.diagnostic == mirakana::PhysicsContinuousStep3DDiagnostic::invalid_delta_seconds);
    MK_REQUIRE(invalid_delta.rows.empty());
    MK_REQUIRE(world.find_body(body)->position == before.position);
    MK_REQUIRE(world.find_body(body)->velocity == before.velocity);
    MK_REQUIRE(world.find_body(body)->accumulated_force == before.accumulated_force);

    const auto invalid_skin =
        world.step_continuous(0.016F, mirakana::PhysicsContinuousStep3DConfig{.skin_width = -1.0F});
    MK_REQUIRE(invalid_skin.status == mirakana::PhysicsContinuousStep3DStatus::invalid_request);
    MK_REQUIRE(invalid_skin.diagnostic == mirakana::PhysicsContinuousStep3DDiagnostic::invalid_config);
    MK_REQUIRE(invalid_skin.rows.empty());
    MK_REQUIRE(world.find_body(body)->position == before.position);
    MK_REQUIRE(world.find_body(body)->velocity == before.velocity);
    MK_REQUIRE(world.find_body(body)->accumulated_force == before.accumulated_force);
}

MK_TEST("3d physics continuous step handles zero movement and preserves tangent velocity") {
    mirakana::PhysicsWorld3D zero_world(
        mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto idle = zero_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto zero = zero_world.step_continuous(0.0F);
    MK_REQUIRE(zero.rows.size() == 1);
    MK_REQUIRE(zero.rows[0].body == idle);
    MK_REQUIRE(!zero.rows[0].ccd_applied);
    MK_REQUIRE(zero.rows[0].attempted_displacement == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(zero_world.find_body(idle)->position == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));

    mirakana::PhysicsWorld3D tangent_world(
        mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto wall = tangent_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.05F, .y = 2.0F, .z = 2.0F},
    });
    const auto glancing = tangent_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 20.0F, .y = 3.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.1F,
    });

    const auto tangent = tangent_world.step_continuous(0.5F);

    MK_REQUIRE(tangent.rows.size() == 1);
    MK_REQUIRE(tangent.rows[0].hit_body == wall);
    MK_REQUIRE(std::abs(tangent_world.find_body(glancing)->velocity.x) < 0.0001F);
    MK_REQUIRE(std::abs(tangent_world.find_body(glancing)->velocity.y - 3.0F) < 0.0001F);
}

MK_TEST("3d physics continuous step supports moving aabb sphere and capsule bodies") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto sphere_target = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.25F, .z = 0.25F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.25F,
    });
    const auto capsule_target = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 2.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.75F, .z = 0.25F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .radius = 0.25F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
    });
    const auto moving_box = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 20.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.2F, .y = 0.2F, .z = 0.2F},
    });
    const auto moving_capsule = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 20.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.2F, .y = 0.6F, .z = 0.2F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .radius = 0.2F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.4F,
    });

    const auto result = world.step_continuous(0.5F);

    MK_REQUIRE(result.rows.size() == 2);
    MK_REQUIRE(result.rows[0].body == moving_box);
    MK_REQUIRE(result.rows[0].hit_body == sphere_target);
    MK_REQUIRE(result.rows[0].ccd_applied);
    MK_REQUIRE(result.rows[1].body == moving_capsule);
    MK_REQUIRE(result.rows[1].hit_body == capsule_target);
    MK_REQUIRE(result.rows[1].ccd_applied);
}

MK_TEST("3d physics continuous step deterministic rows and default step remains discrete") {
    auto create_world = [] {
        mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
        const auto wall = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.05F, .y = 3.0F, .z = 3.0F},
        });
        const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 20.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::sphere,
            .radius = 0.1F,
        });
        const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 20.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::sphere,
            .radius = 0.1F,
        });
        return std::tuple{world, wall, first, second};
    };

    auto [first_world, first_wall, first_body, second_body] = create_world();
    auto [second_world, second_wall, first_body_b, second_body_b] = create_world();

    const auto first = first_world.step_continuous(0.5F);
    const auto second = second_world.step_continuous(0.5F);
    MK_REQUIRE(first_wall == second_wall);
    MK_REQUIRE(first_body == first_body_b);
    MK_REQUIRE(second_body == second_body_b);
    MK_REQUIRE(first.rows.size() == second.rows.size());
    for (std::size_t index = 0; index < first.rows.size(); ++index) {
        MK_REQUIRE(first.rows[index].body == second.rows[index].body);
        MK_REQUIRE(first.rows[index].hit_body == second.rows[index].hit_body);
        MK_REQUIRE(first.rows[index].ccd_applied == second.rows[index].ccd_applied);
        MK_REQUIRE(first.rows[index].applied_displacement == second.rows[index].applied_displacement);
    }

    mirakana::PhysicsWorld3D discrete_world(
        mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto wall = discrete_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.05F, .y = 2.0F, .z = 2.0F},
    });
    const auto bullet = discrete_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 20.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.1F,
    });
    discrete_world.step(0.5F);
    MK_REQUIRE(wall != mirakana::null_physics_body_3d);
    MK_REQUIRE(discrete_world.find_body(bullet)->position.x > 5.0F);
}

MK_TEST("3d physics distance joints solve deterministic body pairs") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = -3.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });

    const auto result = mirakana::solve_physics_joints_3d(
        world, mirakana::PhysicsJointSolve3DDesc{
                   .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                   .distance_joints =
                       std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                           mirakana::PhysicsDistanceJoint3DDesc{
                               .first = first,
                               .second = second,
                               .local_anchor_first = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .local_anchor_second = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .rest_distance = 2.0F,
                           },
                       },
               });

    MK_REQUIRE(result.status == mirakana::PhysicsJoint3DStatus::solved);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsJoint3DDiagnostic::none);
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.rows[0].source_index == 0U);
    MK_REQUIRE(result.rows[0].first == first);
    MK_REQUIRE(result.rows[0].second == second);
    MK_REQUIRE(std::abs(result.rows[0].previous_distance - 4.0F) < 0.0001F);
    MK_REQUIRE(std::abs(result.rows[0].target_distance - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(result.rows[0].residual_distance) < 0.0001F);
    MK_REQUIRE(result.rows[0].first_correction == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(result.rows[0].second_correction == (mirakana::Vec3{-1.0F, -0.0F, -0.0F}));
    MK_REQUIRE(world.find_body(first)->position == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(world.find_body(second)->position == (mirakana::Vec3{3.0F, 0.0F, 0.0F}));
    MK_REQUIRE(world.find_body(first)->velocity == (mirakana::Vec3{2.0F, 0.0F, 0.0F}));
    MK_REQUIRE(world.find_body(second)->velocity == (mirakana::Vec3{-3.0F, 0.0F, 0.0F}));

    mirakana::PhysicsWorld3D anchored_world(
        mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto anchored_first = anchored_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto anchored_second = anchored_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto anchored_result = mirakana::solve_physics_joints_3d(
        anchored_world, mirakana::PhysicsJointSolve3DDesc{
                            .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                            .distance_joints =
                                std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                                    mirakana::PhysicsDistanceJoint3DDesc{
                                        .first = anchored_first,
                                        .second = anchored_second,
                                        .local_anchor_first = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                        .local_anchor_second = mirakana::Vec3{.x = -1.0F, .y = 0.0F, .z = 0.0F},
                                        .rest_distance = 1.0F,
                                    },
                                },
                        });

    MK_REQUIRE(anchored_result.status == mirakana::PhysicsJoint3DStatus::solved);
    MK_REQUIRE(anchored_result.rows.size() == 1);
    MK_REQUIRE(std::abs(anchored_result.rows[0].previous_distance - 3.0F) < 0.0001F);
    MK_REQUIRE(std::abs(anchored_result.rows[0].target_distance - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(anchored_result.rows[0].residual_distance) < 0.0001F);
    MK_REQUIRE(anchored_result.rows[0].first_correction == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(anchored_result.rows[0].second_correction == (mirakana::Vec3{-1.0F, -0.0F, -0.0F}));
    MK_REQUIRE(anchored_world.find_body(anchored_first)->position == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(anchored_world.find_body(anchored_second)->position == (mirakana::Vec3{4.0F, 0.0F, 0.0F}));
}

MK_TEST("3d physics distance joints cover static disabled and replay rows") {
    mirakana::PhysicsWorld3D static_world(
        mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto anchor = static_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
    });
    const auto dynamic = static_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 7.0F, .z = 0.0F},
        .mass = 2.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });

    const auto static_result = mirakana::solve_physics_joints_3d(
        static_world, mirakana::PhysicsJointSolve3DDesc{
                          .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 2U, .tolerance = 0.0001F},
                          .distance_joints =
                              std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                                  mirakana::PhysicsDistanceJoint3DDesc{
                                      .first = anchor,
                                      .second = dynamic,
                                      .local_anchor_first = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                      .local_anchor_second = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                      .rest_distance = 2.0F,
                                  },
                              },
                      });

    MK_REQUIRE(static_result.status == mirakana::PhysicsJoint3DStatus::solved);
    MK_REQUIRE(static_result.rows.size() == 1);
    MK_REQUIRE(static_result.rows[0].first_correction == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(static_result.rows[0].second_correction == (mirakana::Vec3{-2.0F, -0.0F, -0.0F}));
    MK_REQUIRE(static_world.find_body(anchor)->position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(static_world.find_body(dynamic)->position == (mirakana::Vec3{2.0F, 0.0F, 0.0F}));
    MK_REQUIRE(static_world.find_body(dynamic)->velocity == (mirakana::Vec3{0.0F, 7.0F, 0.0F}));

    mirakana::PhysicsWorld3D disabled_world(
        mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto disabled_first = disabled_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto disabled_second = disabled_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto disabled = mirakana::solve_physics_joints_3d(
        disabled_world, mirakana::PhysicsJointSolve3DDesc{
                            .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                            .distance_joints =
                                std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                                    mirakana::PhysicsDistanceJoint3DDesc{
                                        .first = disabled_first,
                                        .second = disabled_second,
                                        .local_anchor_first = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                        .local_anchor_second = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                        .rest_distance = 2.0F,
                                        .enabled = false,
                                    },
                                },
                        });
    MK_REQUIRE(disabled.status == mirakana::PhysicsJoint3DStatus::solved);
    MK_REQUIRE(disabled.rows.size() == 1);
    MK_REQUIRE(disabled.rows[0].diagnostic == mirakana::PhysicsJoint3DDiagnostic::disabled_joint);
    MK_REQUIRE(disabled_world.find_body(disabled_first)->position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(disabled_world.find_body(disabled_second)->position == (mirakana::Vec3{4.0F, 0.0F, 0.0F}));

    auto create_replay_world = [] {
        mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
        const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
        });
        const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
        });
        const auto third = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 8.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
        });
        return std::tuple{world, first, second, third};
    };

    auto [first_world, first_a, first_b, first_c] = create_replay_world();
    auto [second_world, second_a, second_b, second_c] = create_replay_world();
    const auto replay_desc = mirakana::PhysicsJointSolve3DDesc{
        .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 2U, .tolerance = 0.0001F},
        .distance_joints =
            std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                mirakana::PhysicsDistanceJoint3DDesc{.first = first_a,
                                                     .second = first_b,
                                                     .local_anchor_first = {},
                                                     .local_anchor_second = {},
                                                     .rest_distance = 2.0F},
                mirakana::PhysicsDistanceJoint3DDesc{.first = first_b,
                                                     .second = first_c,
                                                     .local_anchor_first = {},
                                                     .local_anchor_second = {},
                                                     .rest_distance = 2.0F},
            },
    };
    const auto replay_first = mirakana::solve_physics_joints_3d(first_world, replay_desc);
    const auto replay_second = mirakana::solve_physics_joints_3d(
        second_world, mirakana::PhysicsJointSolve3DDesc{
                          .config = replay_desc.config,
                          .distance_joints =
                              std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                                  mirakana::PhysicsDistanceJoint3DDesc{.first = second_a,
                                                                       .second = second_b,
                                                                       .local_anchor_first = {},
                                                                       .local_anchor_second = {},
                                                                       .rest_distance = 2.0F},
                                  mirakana::PhysicsDistanceJoint3DDesc{.first = second_b,
                                                                       .second = second_c,
                                                                       .local_anchor_first = {},
                                                                       .local_anchor_second = {},
                                                                       .rest_distance = 2.0F},
                              },
                      });

    MK_REQUIRE(replay_first.status == replay_second.status);
    MK_REQUIRE(replay_first.rows.size() == replay_second.rows.size());
    for (std::size_t index = 0; index < replay_first.rows.size(); ++index) {
        MK_REQUIRE(replay_first.rows[index].source_index == replay_second.rows[index].source_index);
        MK_REQUIRE(replay_first.rows[index].previous_distance == replay_second.rows[index].previous_distance);
        MK_REQUIRE(replay_first.rows[index].target_distance == replay_second.rows[index].target_distance);
        MK_REQUIRE(replay_first.rows[index].residual_distance == replay_second.rows[index].residual_distance);
        MK_REQUIRE(replay_first.rows[index].first_correction == replay_second.rows[index].first_correction);
        MK_REQUIRE(replay_first.rows[index].second_correction == replay_second.rows[index].second_correction);
    }
    MK_REQUIRE(first_world.bodies().size() == second_world.bodies().size());
    for (std::size_t index = 0; index < first_world.bodies().size(); ++index) {
        MK_REQUIRE(first_world.bodies()[index].position == second_world.bodies()[index].position);
    }
}

MK_TEST("3d physics joints reject invalid requests without mutation") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto body = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
        .velocity = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto other = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 3.0F, .y = 2.0F, .z = 3.0F},
        .velocity = mirakana::Vec3{.x = 7.0F, .y = 8.0F, .z = 9.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto before = *world.find_body(body);
    const auto other_before = *world.find_body(other);

    const auto missing_body = mirakana::solve_physics_joints_3d(
        world, mirakana::PhysicsJointSolve3DDesc{
                   .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                   .distance_joints =
                       std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                           mirakana::PhysicsDistanceJoint3DDesc{
                               .first = body,
                               .second = mirakana::PhysicsBody3DId{999U},
                               .local_anchor_first = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .local_anchor_second = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .rest_distance = 1.0F,
                           },
                       },
               });

    MK_REQUIRE(missing_body.status == mirakana::PhysicsJoint3DStatus::invalid_request);
    MK_REQUIRE(missing_body.diagnostic == mirakana::PhysicsJoint3DDiagnostic::missing_body);
    MK_REQUIRE(missing_body.rows.size() == 1);
    MK_REQUIRE(missing_body.rows[0].diagnostic == mirakana::PhysicsJoint3DDiagnostic::missing_body);
    MK_REQUIRE(world.find_body(body)->position == before.position);
    MK_REQUIRE(world.find_body(body)->velocity == before.velocity);
    MK_REQUIRE(world.find_body(body)->accumulated_force == before.accumulated_force);
    MK_REQUIRE(world.find_body(other)->position == other_before.position);

    const auto invalid_config = mirakana::solve_physics_joints_3d(
        world, mirakana::PhysicsJointSolve3DDesc{
                   .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 0U, .tolerance = 0.0001F},
                   .distance_joints =
                       std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                           mirakana::PhysicsDistanceJoint3DDesc{
                               .first = body,
                               .second = body,
                               .local_anchor_first = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .local_anchor_second = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .rest_distance = 1.0F,
                           },
                       },
               });
    MK_REQUIRE(invalid_config.status == mirakana::PhysicsJoint3DStatus::invalid_request);
    MK_REQUIRE(invalid_config.diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_config);
    MK_REQUIRE(invalid_config.rows.empty());
    MK_REQUIRE(world.find_body(body)->position == before.position);

    const auto invalid_tolerance = mirakana::solve_physics_joints_3d(
        world, mirakana::PhysicsJointSolve3DDesc{
                   .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U,
                                                                 .tolerance = std::numeric_limits<float>::infinity()},
                   .distance_joints =
                       std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                           mirakana::PhysicsDistanceJoint3DDesc{.first = body,
                                                                .second = other,
                                                                .local_anchor_first = {},
                                                                .local_anchor_second = {},
                                                                .rest_distance = 1.0F},
                       },
               });
    MK_REQUIRE(invalid_tolerance.status == mirakana::PhysicsJoint3DStatus::invalid_request);
    MK_REQUIRE(invalid_tolerance.diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_config);
    MK_REQUIRE(invalid_tolerance.rows.empty());
    MK_REQUIRE(world.find_body(body)->position == before.position);
    MK_REQUIRE(world.find_body(other)->position == other_before.position);

    const auto self_joint = mirakana::solve_physics_joints_3d(
        world, mirakana::PhysicsJointSolve3DDesc{
                   .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                   .distance_joints =
                       std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                           mirakana::PhysicsDistanceJoint3DDesc{.first = body,
                                                                .second = body,
                                                                .local_anchor_first = {},
                                                                .local_anchor_second = {},
                                                                .rest_distance = 0.0F},
                       },
               });
    MK_REQUIRE(self_joint.status == mirakana::PhysicsJoint3DStatus::invalid_request);
    MK_REQUIRE(self_joint.diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_joint);
    MK_REQUIRE(self_joint.rows.size() == 1);
    MK_REQUIRE(self_joint.rows[0].diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_joint);
    MK_REQUIRE(world.find_body(body)->position == before.position);

    const auto invalid_joint = mirakana::solve_physics_joints_3d(
        world, mirakana::PhysicsJointSolve3DDesc{
                   .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                   .distance_joints =
                       std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                           mirakana::PhysicsDistanceJoint3DDesc{
                               .first = body,
                               .second = body,
                               .local_anchor_first = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .local_anchor_second =
                                   mirakana::Vec3{.x = 0.0F, .y = std::numeric_limits<float>::infinity(), .z = 0.0F},
                               .rest_distance = 1.0F,
                           },
                       },
               });
    MK_REQUIRE(invalid_joint.status == mirakana::PhysicsJoint3DStatus::invalid_request);
    MK_REQUIRE(invalid_joint.diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_joint);
    MK_REQUIRE(invalid_joint.rows.size() == 1);
    MK_REQUIRE(invalid_joint.rows[0].diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_joint);
    MK_REQUIRE(world.find_body(body)->position == before.position);

    const auto negative_rest_distance = mirakana::solve_physics_joints_3d(
        world, mirakana::PhysicsJointSolve3DDesc{
                   .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                   .distance_joints =
                       std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                           mirakana::PhysicsDistanceJoint3DDesc{.first = body,
                                                                .second = other,
                                                                .local_anchor_first = {},
                                                                .local_anchor_second = {},
                                                                .rest_distance = -1.0F},
                       },
               });
    MK_REQUIRE(negative_rest_distance.status == mirakana::PhysicsJoint3DStatus::invalid_request);
    MK_REQUIRE(negative_rest_distance.diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_joint);
    MK_REQUIRE(negative_rest_distance.rows.size() == 1);
    MK_REQUIRE(negative_rest_distance.rows[0].diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_joint);
    MK_REQUIRE(world.find_body(body)->position == before.position);
    MK_REQUIRE(world.find_body(other)->position == other_before.position);

    const auto non_finite_rest_distance = mirakana::solve_physics_joints_3d(
        world, mirakana::PhysicsJointSolve3DDesc{
                   .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                   .distance_joints =
                       std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                           mirakana::PhysicsDistanceJoint3DDesc{
                               .first = body,
                               .second = other,
                               .local_anchor_first = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .local_anchor_second = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                               .rest_distance = std::numeric_limits<float>::infinity(),
                           },
                       },
               });
    MK_REQUIRE(non_finite_rest_distance.status == mirakana::PhysicsJoint3DStatus::invalid_request);
    MK_REQUIRE(non_finite_rest_distance.diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_joint);
    MK_REQUIRE(non_finite_rest_distance.rows.size() == 1);
    MK_REQUIRE(non_finite_rest_distance.rows[0].diagnostic == mirakana::PhysicsJoint3DDiagnostic::invalid_joint);
    MK_REQUIRE(world.find_body(body)->position == before.position);
    MK_REQUIRE(world.find_body(other)->position == other_before.position);

    mirakana::PhysicsWorld3D static_pair_world(
        mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto static_first = static_pair_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
    });
    const auto static_second = static_pair_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
    });
    const auto static_before_first = *static_pair_world.find_body(static_first);
    const auto static_before_second = *static_pair_world.find_body(static_second);
    const auto both_static = mirakana::solve_physics_joints_3d(
        static_pair_world, mirakana::PhysicsJointSolve3DDesc{
                               .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 1U, .tolerance = 0.0001F},
                               .distance_joints =
                                   std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                                       mirakana::PhysicsDistanceJoint3DDesc{
                                           .first = static_first,
                                           .second = static_second,
                                           .local_anchor_first = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                           .local_anchor_second = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                           .rest_distance = 2.0F,
                                       },
                                   },
                           });
    MK_REQUIRE(both_static.status == mirakana::PhysicsJoint3DStatus::invalid_request);
    MK_REQUIRE(both_static.diagnostic == mirakana::PhysicsJoint3DDiagnostic::static_pair);
    MK_REQUIRE(both_static.rows.size() == 1);
    MK_REQUIRE(both_static.rows[0].diagnostic == mirakana::PhysicsJoint3DDiagnostic::static_pair);
    MK_REQUIRE(static_pair_world.find_body(static_first)->position == static_before_first.position);
    MK_REQUIRE(static_pair_world.find_body(static_second)->position == static_before_second.position);
}

MK_TEST("3d physics joints keep default step unchanged when no joint solve is called") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });

    world.step(1.0F);

    MK_REQUIRE(world.find_body(first)->position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(world.find_body(second)->position == (mirakana::Vec3{4.0F, 0.0F, 0.0F}));
}

MK_TEST("3d physics determinism gate reports stable query and solver budgets") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.75F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
        .trigger = true,
    });
    MK_REQUIRE(first != mirakana::null_physics_body_3d);
    MK_REQUIRE(second != mirakana::null_physics_body_3d);
    MK_REQUIRE(trigger != mirakana::null_physics_body_3d);

    const auto pairs = world.broadphase_pairs();
    const auto contacts = world.contacts();
    const auto manifolds = world.contact_manifolds();
    const auto overlaps = world.trigger_overlaps();
    std::size_t contact_points = 0;
    for (const auto& manifold : manifolds) {
        contact_points += manifold.points.size();
    }

    const auto result =
        mirakana::evaluate_physics_determinism_gate_3d(world, mirakana::PhysicsDeterminismGate3DConfig{
                                                                  .max_bodies = world.bodies().size(),
                                                                  .max_broadphase_pairs = pairs.size(),
                                                                  .max_contacts = contacts.size(),
                                                                  .max_contact_manifolds = manifolds.size(),
                                                                  .max_trigger_overlaps = overlaps.size(),
                                                                  .max_contact_points = contact_points,
                                                              });
    const auto replay = mirakana::make_physics_replay_signature_3d(world);

    MK_REQUIRE(result.status == mirakana::PhysicsDeterminismGate3DStatus::passed);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsDeterminismGate3DDiagnostic::none);
    MK_REQUIRE(result.counts.bodies == 3U);
    MK_REQUIRE(result.counts.broadphase_pairs == pairs.size());
    MK_REQUIRE(result.counts.contacts == contacts.size());
    MK_REQUIRE(result.counts.contact_manifolds == manifolds.size());
    MK_REQUIRE(result.counts.trigger_overlaps == overlaps.size());
    MK_REQUIRE(result.counts.contact_points == contact_points);
    MK_REQUIRE(result.replay_signature.value == replay.value);
    MK_REQUIRE(result.replay_signature.body_count == replay.body_count);
}

MK_TEST("3d physics determinism gate rejects budget regressions without mutation") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F}});
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.75F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = -1.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    MK_REQUIRE(second != mirakana::null_physics_body_3d);
    world.apply_force(first, mirakana::Vec3{.x = 0.25F, .y = 0.5F, .z = 0.75F});

    const auto before_signature = mirakana::make_physics_replay_signature_3d(world);
    const auto before_bodies = world.bodies();
    auto config = mirakana::PhysicsDeterminismGate3DConfig{};
    config.max_bodies = 1U;

    const auto result = mirakana::evaluate_physics_determinism_gate_3d(world, config);
    const auto after_signature = mirakana::make_physics_replay_signature_3d(world);

    MK_REQUIRE(result.status == mirakana::PhysicsDeterminismGate3DStatus::budget_exceeded);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsDeterminismGate3DDiagnostic::bodies_exceeded);
    MK_REQUIRE(result.counts.bodies == 2U);
    MK_REQUIRE(result.replay_signature.value == before_signature.value);
    MK_REQUIRE(after_signature.value == before_signature.value);
    MK_REQUIRE(world.bodies().size() == before_bodies.size());
    for (std::size_t index = 0; index < before_bodies.size(); ++index) {
        MK_REQUIRE(world.bodies()[index].position == before_bodies[index].position);
        MK_REQUIRE(world.bodies()[index].velocity == before_bodies[index].velocity);
        MK_REQUIRE(world.bodies()[index].accumulated_force == before_bodies[index].accumulated_force);
        MK_REQUIRE(world.bodies()[index].collision_enabled == before_bodies[index].collision_enabled);
    }
}

MK_TEST("3d physics determinism gate reports each budget diagnostic") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.75F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
    });
    const auto trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
        .trigger = true,
    });
    MK_REQUIRE(first != mirakana::null_physics_body_3d);
    MK_REQUIRE(second != mirakana::null_physics_body_3d);
    MK_REQUIRE(trigger != mirakana::null_physics_body_3d);

    const auto baseline = mirakana::evaluate_physics_determinism_gate_3d(world);
    MK_REQUIRE(baseline.status == mirakana::PhysicsDeterminismGate3DStatus::passed);
    MK_REQUIRE(baseline.counts.bodies > 0U);
    MK_REQUIRE(baseline.counts.broadphase_pairs > 0U);
    MK_REQUIRE(baseline.counts.contacts > 0U);
    MK_REQUIRE(baseline.counts.contact_manifolds > 0U);
    MK_REQUIRE(baseline.counts.trigger_overlaps > 0U);
    MK_REQUIRE(baseline.counts.contact_points > 0U);

    auto expect_budget = [&world](mirakana::PhysicsDeterminismGate3DConfig config,
                                  mirakana::PhysicsDeterminismGate3DDiagnostic diagnostic) {
        const auto result = mirakana::evaluate_physics_determinism_gate_3d(world, config);
        MK_REQUIRE(result.status == mirakana::PhysicsDeterminismGate3DStatus::budget_exceeded);
        MK_REQUIRE(result.diagnostic == diagnostic);
    };

    auto config = mirakana::PhysicsDeterminismGate3DConfig{};
    config.max_bodies = baseline.counts.bodies - 1U;
    expect_budget(config, mirakana::PhysicsDeterminismGate3DDiagnostic::bodies_exceeded);

    config = mirakana::PhysicsDeterminismGate3DConfig{};
    config.max_broadphase_pairs = baseline.counts.broadphase_pairs - 1U;
    expect_budget(config, mirakana::PhysicsDeterminismGate3DDiagnostic::broadphase_pairs_exceeded);

    config = mirakana::PhysicsDeterminismGate3DConfig{};
    config.max_contacts = baseline.counts.contacts - 1U;
    expect_budget(config, mirakana::PhysicsDeterminismGate3DDiagnostic::contacts_exceeded);

    config = mirakana::PhysicsDeterminismGate3DConfig{};
    config.max_contact_manifolds = baseline.counts.contact_manifolds - 1U;
    expect_budget(config, mirakana::PhysicsDeterminismGate3DDiagnostic::contact_manifolds_exceeded);

    config = mirakana::PhysicsDeterminismGate3DConfig{};
    config.max_trigger_overlaps = baseline.counts.trigger_overlaps - 1U;
    expect_budget(config, mirakana::PhysicsDeterminismGate3DDiagnostic::trigger_overlaps_exceeded);

    config = mirakana::PhysicsDeterminismGate3DConfig{};
    config.max_contact_points = baseline.counts.contact_points - 1U;
    expect_budget(config, mirakana::PhysicsDeterminismGate3DDiagnostic::contact_points_exceeded);
}

MK_TEST("3d physics determinism gate treats defaults as unlimited and all zero as invalid") {
    mirakana::PhysicsWorld3D empty_world;
    auto zero_contacts_config = mirakana::PhysicsDeterminismGate3DConfig{};
    zero_contacts_config.max_contacts = 0U;

    const auto zero_contacts = mirakana::evaluate_physics_determinism_gate_3d(empty_world, zero_contacts_config);
    MK_REQUIRE(zero_contacts.status == mirakana::PhysicsDeterminismGate3DStatus::passed);
    MK_REQUIRE(zero_contacts.counts.contacts == 0U);

    const auto unlimited = mirakana::evaluate_physics_determinism_gate_3d(empty_world);
    MK_REQUIRE(unlimited.status == mirakana::PhysicsDeterminismGate3DStatus::passed);
    MK_REQUIRE(unlimited.diagnostic == mirakana::PhysicsDeterminismGate3DDiagnostic::none);

    const auto invalid = mirakana::evaluate_physics_determinism_gate_3d(
        empty_world, mirakana::PhysicsDeterminismGate3DConfig{.max_bodies = 0U,
                                                              .max_broadphase_pairs = 0U,
                                                              .max_contacts = 0U,
                                                              .max_contact_manifolds = 0U,
                                                              .max_trigger_overlaps = 0U,
                                                              .max_contact_points = 0U});
    MK_REQUIRE(invalid.status == mirakana::PhysicsDeterminismGate3DStatus::invalid_request);
    MK_REQUIRE(invalid.diagnostic == mirakana::PhysicsDeterminismGate3DDiagnostic::invalid_config);
}

MK_TEST("3d physics replay signature is stable across duplicated worlds") {
    auto make_world = [] {
        mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
        const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
        });
        const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
        });
        const auto trigger = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::sphere,
            .radius = 0.5F,
            .collision_layer = 1U,
            .collision_mask = 0xFFFF'FFFFU,
            .half_height = 0.5F,
            .trigger = true,
        });
        return std::tuple<mirakana::PhysicsWorld3D, mirakana::PhysicsBody3DId, mirakana::PhysicsBody3DId,
                          mirakana::PhysicsBody3DId>{
            std::move(world),
            first,
            second,
            trigger,
        };
    };

    auto [first_world, first_a, first_b, first_trigger] = make_world();
    auto [second_world, second_a, second_b, second_trigger] = make_world();

    const auto first_initial = mirakana::make_physics_replay_signature_3d(first_world);
    const auto second_initial = mirakana::make_physics_replay_signature_3d(second_world);
    MK_REQUIRE(first_a == second_a);
    MK_REQUIRE(first_b == second_b);
    MK_REQUIRE(first_trigger == second_trigger);
    MK_REQUIRE(first_initial.value == second_initial.value);
    MK_REQUIRE(first_initial.body_count == 3U);

    const auto first_joint = mirakana::solve_physics_joints_3d(
        first_world, mirakana::PhysicsJointSolve3DDesc{
                         .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 2U, .tolerance = 0.0001F},
                         .distance_joints =
                             std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                                 mirakana::PhysicsDistanceJoint3DDesc{.first = first_a,
                                                                      .second = first_b,
                                                                      .local_anchor_first = {},
                                                                      .local_anchor_second = {},
                                                                      .rest_distance = 2.0F},
                             },
                     });
    const auto second_joint = mirakana::solve_physics_joints_3d(
        second_world, mirakana::PhysicsJointSolve3DDesc{
                          .config = mirakana::PhysicsJointSolve3DConfig{.iterations = 2U, .tolerance = 0.0001F},
                          .distance_joints =
                              std::vector<mirakana::PhysicsDistanceJoint3DDesc>{
                                  mirakana::PhysicsDistanceJoint3DDesc{.first = second_a,
                                                                       .second = second_b,
                                                                       .local_anchor_first = {},
                                                                       .local_anchor_second = {},
                                                                       .rest_distance = 2.0F},
                              },
                      });
    const auto first_after = mirakana::make_physics_replay_signature_3d(first_world);
    const auto second_after = mirakana::make_physics_replay_signature_3d(second_world);

    MK_REQUIRE(first_joint.status == mirakana::PhysicsJoint3DStatus::solved);
    MK_REQUIRE(second_joint.status == first_joint.status);
    MK_REQUIRE(first_after.value == second_after.value);
    MK_REQUIRE(first_after.value != first_initial.value);
}

MK_TEST("3d physics replay signature covers next-step integration inputs") {
    auto make_world = [](mirakana::PhysicsWorld3DConfig config, float linear_damping) {
        mirakana::PhysicsWorld3D world(config);
        const auto body = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
            .velocity = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F},
            .mass = 2.0F,
            .linear_damping = linear_damping,
            .dynamic = true,
        });
        return std::pair<mirakana::PhysicsWorld3D, mirakana::PhysicsBody3DId>{std::move(world), body};
    };

    auto [baseline_world, baseline_body] =
        make_world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -9.80665F, .z = 0.0F}}, 0.0F);
    auto [forced_world, forced_body] =
        make_world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -9.80665F, .z = 0.0F}}, 0.0F);
    auto [damped_world, damped_body] =
        make_world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -9.80665F, .z = 0.0F}}, 0.25F);
    auto [low_gravity_world, low_gravity_body] =
        make_world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F}}, 0.0F);

    forced_world.apply_force(forced_body, mirakana::Vec3{.x = 7.0F, .y = 8.0F, .z = 9.0F});

    const auto baseline = mirakana::make_physics_replay_signature_3d(baseline_world);
    const auto forced = mirakana::make_physics_replay_signature_3d(forced_world);
    const auto damped = mirakana::make_physics_replay_signature_3d(damped_world);
    const auto low_gravity = mirakana::make_physics_replay_signature_3d(low_gravity_world);

    MK_REQUIRE(baseline_body == forced_body);
    MK_REQUIRE(baseline_body == damped_body);
    MK_REQUIRE(baseline_body == low_gravity_body);
    MK_REQUIRE(baseline.body_count == 1U);
    MK_REQUIRE(forced.body_count == baseline.body_count);
    MK_REQUIRE(damped.body_count == baseline.body_count);
    MK_REQUIRE(low_gravity.body_count == baseline.body_count);
    MK_REQUIRE(forced.value != baseline.value);
    MK_REQUIRE(damped.value != baseline.value);
    MK_REQUIRE(low_gravity.value != baseline.value);
}

MK_TEST("3d physics world keeps static bodies fixed and clears force") {
    mirakana::PhysicsWorld3D world;
    const auto body = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
        .velocity = mirakana::Vec3{.x = 4.0F, .y = 5.0F, .z = 6.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
    });

    world.apply_force(body, mirakana::Vec3{.x = 100.0F, .y = 0.0F, .z = 0.0F});
    world.step(1.0F);

    const auto* state = world.find_body(body);
    MK_REQUIRE(state != nullptr);
    MK_REQUIRE(state->position == (mirakana::Vec3{1.0F, 2.0F, 3.0F}));
    MK_REQUIRE(state->velocity == (mirakana::Vec3{4.0F, 5.0F, 6.0F}));
    MK_REQUIRE(state->accumulated_force == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
}

MK_TEST("3d physics world rejects invalid bodies and deltas") {
    MK_REQUIRE(!mirakana::is_valid_physics_body_desc(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        0.0F,
        0.0F,
        true,
        mirakana::Vec3{0.5F, 0.5F, 0.5F},
    }));
    MK_REQUIRE(!mirakana::is_valid_physics_body_desc(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        1.0F,
        0.0F,
        true,
        mirakana::Vec3{0.0F, 0.5F, 0.5F},
    }));

    mirakana::PhysicsWorld3D world;
    bool rejected_delta = false;
    try {
        world.step(-0.01F);
    } catch (const std::invalid_argument&) {
        rejected_delta = true;
    }
    MK_REQUIRE(rejected_delta);
}

MK_TEST("3d physics world reports deterministic aabb contacts") {
    mirakana::PhysicsWorld3D world;
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.5F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
    });
    const auto distant = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
    });
    MK_REQUIRE(distant != mirakana::null_physics_body_3d);

    const auto contacts = world.contacts();

    MK_REQUIRE(contacts.size() == 1);
    MK_REQUIRE(contacts[0].first == first);
    MK_REQUIRE(contacts[0].second == second);
    MK_REQUIRE(contacts[0].normal == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(contacts[0].penetration_depth - 0.5F) < 0.0001F);
}

MK_TEST("3d physics contact manifolds expose deterministic solver rows") {
    mirakana::PhysicsWorld3D world;
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.5F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
    });

    const auto manifolds = world.contact_manifolds();

    MK_REQUIRE(manifolds.size() == 1);
    MK_REQUIRE(manifolds[0].first == first);
    MK_REQUIRE(manifolds[0].second == second);
    MK_REQUIRE(manifolds[0].normal == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(manifolds[0].points.size() == 1);
    MK_REQUIRE(std::abs(manifolds[0].points[0].penetration_depth - 0.5F) < 0.0001F);
    MK_REQUIRE(manifolds[0].points[0].warm_start_eligible);
    MK_REQUIRE(manifolds[0].points[0].feature_id != 0U);
}

MK_TEST("3d physics contact manifold points use shape pair closest features") {
    mirakana::PhysicsWorld3D world;
    const auto sphere = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 1U,
    });
    const auto box = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 1U,
    });

    const auto manifolds = world.contact_manifolds();

    MK_REQUIRE(manifolds.size() == 1);
    MK_REQUIRE(manifolds[0].first == sphere);
    MK_REQUIRE(manifolds[0].second == box);
    MK_REQUIRE(manifolds[0].points.size() == 1);
    MK_REQUIRE(manifolds[0].points[0].position == (mirakana::Vec3{0.25F, 0.0F, 0.0F}));
    MK_REQUIRE(manifolds[0].points[0].warm_start_eligible);
}

MK_TEST("3d physics contact manifold feature ids persist under tangent movement") {
    auto create_world = [](float tangent_shift) {
        mirakana::PhysicsWorld3D world;
        const auto first_box = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = tangent_shift, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        });
        const auto second_box = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 1.5F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        });
        const auto capsule = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 5.0F, .y = tangent_shift, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::capsule,
            .radius = 0.25F,
            .collision_layer = 1U,
            .collision_mask = 1U,
            .half_height = 0.75F,
        });
        const auto sphere = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 5.0F, .y = 1.0F + tangent_shift, .z = 0.4F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::sphere,
            .radius = 0.35F,
            .collision_layer = 1U,
            .collision_mask = 1U,
        });
        MK_REQUIRE(first_box != mirakana::null_physics_body_3d);
        MK_REQUIRE(second_box != mirakana::null_physics_body_3d);
        MK_REQUIRE(capsule != mirakana::null_physics_body_3d);
        MK_REQUIRE(sphere != mirakana::null_physics_body_3d);
        return world;
    };

    auto before_world = create_world(0.0F);
    auto after_world = create_world(0.01F);
    const auto before = before_world.contact_manifolds();
    const auto after = after_world.contact_manifolds();

    MK_REQUIRE(before.size() == 2);
    MK_REQUIRE(after.size() == before.size());
    for (std::size_t index = 0; index < before.size(); ++index) {
        MK_REQUIRE(before[index].points.size() == 1);
        MK_REQUIRE(after[index].points.size() == 1);
        MK_REQUIRE(before[index].points[0].feature_id == after[index].points[0].feature_id);
        MK_REQUIRE(before[index].points[0].warm_start_eligible);
        MK_REQUIRE(after[index].points[0].warm_start_eligible);
    }
}

MK_TEST("3d physics contact manifolds exclude trigger and disabled overlaps") {
    mirakana::PhysicsWorld3D world;
    const auto dynamic_body = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
    });
    const auto trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 1U,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto disabled = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = -0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = false,
    });

    MK_REQUIRE(dynamic_body != mirakana::null_physics_body_3d);
    MK_REQUIRE(trigger != mirakana::null_physics_body_3d);
    MK_REQUIRE(disabled != mirakana::null_physics_body_3d);
    MK_REQUIRE(world.contact_manifolds().empty());
    MK_REQUIRE(world.contacts().empty());
}

MK_TEST("3d physics world ignores disabled collision bodies") {
    mirakana::PhysicsWorld3D world;
    const auto disabled = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = false,
    });
    const auto enabled = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.5F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
    });
    MK_REQUIRE(disabled != mirakana::null_physics_body_3d);
    MK_REQUIRE(enabled != mirakana::null_physics_body_3d);

    MK_REQUIRE(world.contacts().empty());
}

MK_TEST("3d physics broadphase filters deterministic pairs by overlap and collision masks") {
    mirakana::PhysicsWorld3D world;
    constexpr std::uint32_t player_layer = 1U << 0U;
    constexpr std::uint32_t terrain_layer = 1U << 1U;
    constexpr std::uint32_t trigger_layer = 1U << 2U;

    const auto player = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = player_layer,
        .collision_mask = terrain_layer,
    });
    const auto terrain = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.5F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = terrain_layer,
        .collision_mask = player_layer,
    });
    const auto trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = player_layer,
    });
    MK_REQUIRE(trigger != mirakana::null_physics_body_3d);

    const auto pairs = world.broadphase_pairs();

    MK_REQUIRE(pairs.size() == 1);
    MK_REQUIRE(pairs[0].first == player);
    MK_REQUIRE(pairs[0].second == terrain);
}

MK_TEST("3d physics raycast hits nearest collision bounds and reports surface data") {
    mirakana::PhysicsWorld3D world;
    const auto far = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });
    const auto near = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });
    MK_REQUIRE(far != mirakana::null_physics_body_3d);

    const auto hit = world.raycast(mirakana::PhysicsRaycast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
    });

    MK_REQUIRE(hit.has_value());
    MK_REQUIRE(hit->body == near);
    MK_REQUIRE(std::abs(hit->distance - 1.5F) < 0.0001F);
    MK_REQUIRE(hit->point == (mirakana::Vec3{1.5F, 0.0F, 0.0F}));
    MK_REQUIRE(hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));
}

MK_TEST("3d physics reports trigger overlaps without contact resolution") {
    mirakana::PhysicsWorld3D world;
    constexpr std::uint32_t player_layer = 1U << 0U;
    constexpr std::uint32_t trigger_layer = 1U << 1U;
    constexpr std::uint32_t terrain_layer = 1U << 2U;

    const auto player = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = player_layer,
        .collision_mask = trigger_layer,
        .half_height = 0.5F,
    });
    const auto trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = player_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto filtered = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = terrain_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto disabled = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = false,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = player_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    MK_REQUIRE(filtered != mirakana::null_physics_body_3d);
    MK_REQUIRE(disabled != mirakana::null_physics_body_3d);

    const auto overlaps = world.trigger_overlaps();
    MK_REQUIRE(overlaps.size() == 1);
    MK_REQUIRE(overlaps[0].first == player);
    MK_REQUIRE(overlaps[0].second == trigger);
    MK_REQUIRE(world.contacts().empty());

    world.resolve_contacts(mirakana::PhysicsContactSolver3DConfig{.restitution = 0.0F, .iterations = 4U});
    MK_REQUIRE(world.find_body(player)->position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(world.find_body(trigger)->position == (mirakana::Vec3{0.25F, 0.0F, 0.0F}));

    const auto replay = world.trigger_overlaps();
    MK_REQUIRE(replay.size() == overlaps.size());
    MK_REQUIRE(replay[0].first == overlaps[0].first);
    MK_REQUIRE(replay[0].second == overlaps[0].second);

    mirakana::PhysicsWorld3D trigger_first_world;
    const auto early_trigger = trigger_first_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = player_layer | trigger_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto late_body = trigger_first_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = player_layer,
        .collision_mask = trigger_layer,
    });
    const auto trigger_first_pairs = trigger_first_world.trigger_overlaps();
    MK_REQUIRE(trigger_first_pairs.size() == 1);
    MK_REQUIRE(trigger_first_pairs[0].first == early_trigger);
    MK_REQUIRE(trigger_first_pairs[0].second == late_body);

    mirakana::PhysicsWorld3D trigger_pair_world;
    const auto first_trigger = trigger_pair_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = trigger_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto second_trigger = trigger_pair_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = trigger_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto trigger_trigger_pairs = trigger_pair_world.trigger_overlaps();
    MK_REQUIRE(trigger_trigger_pairs.size() == 1);
    MK_REQUIRE(trigger_trigger_pairs[0].first == first_trigger);
    MK_REQUIRE(trigger_trigger_pairs[0].second == second_trigger);
    MK_REQUIRE(trigger_pair_world.contacts().empty());
}

MK_TEST("3d physics shape sweep reports nearest initial and filtered hits") {
    mirakana::PhysicsWorld3D world;
    constexpr std::uint32_t terrain_layer = 1U << 1U;
    constexpr std::uint32_t sensor_layer = 1U << 2U;

    const auto sensor = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = sensor_layer,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto near = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = terrain_layer,
        .collision_mask = 0xFFFF'FFFFU,
    });
    const auto far = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 6.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = terrain_layer,
        .collision_mask = 0xFFFF'FFFFU,
    });

    const auto hit = world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .radius = 0.5F,
        .half_height = 0.5F,
        .collision_mask = terrain_layer | sensor_layer,
        .ignored_body = mirakana::null_physics_body_3d,
        .include_triggers = false,
    });

    MK_REQUIRE(sensor != mirakana::null_physics_body_3d);
    MK_REQUIRE(hit.has_value());
    MK_REQUIRE(hit->body == near);
    MK_REQUIRE(!hit->initial_overlap);
    MK_REQUIRE(std::abs(hit->distance - 3.0F) < 0.0001F);
    MK_REQUIRE(hit->position == (mirakana::Vec3{3.0F, 0.0F, 0.0F}));
    MK_REQUIRE(hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));

    const auto with_triggers = world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .radius = 0.5F,
        .half_height = 0.5F,
        .collision_mask = terrain_layer | sensor_layer,
    });
    MK_REQUIRE(with_triggers.has_value());
    MK_REQUIRE(with_triggers->body == sensor);
    MK_REQUIRE(std::abs(with_triggers->distance - 1.0F) < 0.0001F);

    const auto initial = world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 3.75F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .radius = 0.5F,
        .half_height = 0.5F,
        .collision_mask = terrain_layer,
        .ignored_body = mirakana::null_physics_body_3d,
        .include_triggers = false,
    });
    MK_REQUIRE(initial.has_value());
    MK_REQUIRE(initial->body == near);
    MK_REQUIRE(initial->initial_overlap);
    MK_REQUIRE(initial->distance == 0.0F);
    MK_REQUIRE(initial->position == (mirakana::Vec3{3.75F, 0.0F, 0.0F}));

    const auto zero_direction_initial = world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 3.75F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 0.0F,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .radius = 0.0F,
        .half_height = 0.0F,
        .collision_mask = terrain_layer,
        .ignored_body = mirakana::null_physics_body_3d,
        .include_triggers = false,
    });
    MK_REQUIRE(zero_direction_initial.has_value());
    MK_REQUIRE(zero_direction_initial->body == near);
    MK_REQUIRE(zero_direction_initial->initial_overlap);

    const auto sphere_sweep = world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .half_extents = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .radius = 0.5F,
        .half_height = 0.0F,
        .collision_mask = terrain_layer,
        .ignored_body = mirakana::null_physics_body_3d,
        .include_triggers = false,
    });
    MK_REQUIRE(sphere_sweep.has_value());
    MK_REQUIRE(sphere_sweep->body == near);

    const auto ignored = world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .radius = 0.5F,
        .half_height = 0.5F,
        .collision_mask = terrain_layer,
        .ignored_body = near,
        .include_triggers = false,
    });
    MK_REQUIRE(ignored.has_value());
    MK_REQUIRE(ignored->body == far);

    MK_REQUIRE(!world
                    .shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
                        mirakana::Vec3{0.0F, 0.0F, 0.0F},
                        mirakana::Vec3{0.0F, 0.0F, 0.0F},
                        10.0F,
                        mirakana::PhysicsShape3DKind::aabb,
                        mirakana::Vec3{0.5F, 0.5F, 0.5F},
                        0.5F,
                        0.5F,
                        terrain_layer,
                    })
                    .has_value());
    MK_REQUIRE(!world
                    .shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
                        mirakana::Vec3{0.0F, 0.0F, 0.0F},
                        mirakana::Vec3{1.0F, 0.0F, 0.0F},
                        10.0F,
                        mirakana::PhysicsShape3DKind::capsule,
                        mirakana::Vec3{0.5F, 0.5F, 0.5F},
                        0.5F,
                        0.0F,
                        terrain_layer,
                    })
                    .has_value());
}

MK_TEST("3d physics exact sphere cast hits sphere surface without bounds inflation") {
    mirakana::PhysicsWorld3D world;
    const auto target = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 1.0F,
    });

    const auto result = world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
    });

    MK_REQUIRE(result.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsExactSphereCast3DDiagnostic::none);
    MK_REQUIRE(result.hit.has_value());
    MK_REQUIRE(result.hit->body == target);
    MK_REQUIRE(!result.hit->initial_overlap);
    MK_REQUIRE(std::abs(result.hit->distance - 3.5F) < 0.0001F);
    MK_REQUIRE(result.hit->position == (mirakana::Vec3{3.5F, 0.0F, 0.0F}));
    MK_REQUIRE(result.hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));
}

MK_TEST("3d physics exact shape sweep sphere matches exact sphere cast") {
    mirakana::PhysicsWorld3D world;
    const auto target = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });

    const auto sphere = world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 8.0F,
        .radius = 0.5F,
    });
    const auto generic = world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 8.0F,
        .shape = mirakana::PhysicsShape3DDesc::sphere(0.5F),
    });

    MK_REQUIRE(sphere.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(generic.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(generic.diagnostic == mirakana::PhysicsExactShapeSweep3DDiagnostic::none);
    MK_REQUIRE(generic.hit.has_value());
    MK_REQUIRE(generic.hit->body == target);
    MK_REQUIRE(std::abs(generic.hit->distance - sphere.hit->distance) < 0.0001F);
    MK_REQUIRE(generic.hit->normal == sphere.hit->normal);
}

MK_TEST("3d physics exact shape sweep supports moving aabb queries") {
    mirakana::PhysicsWorld3D box_world;
    const auto box = box_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });

    const auto box_hit = box_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F}),
    });
    MK_REQUIRE(box_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(box_hit.hit.has_value());
    MK_REQUIRE(box_hit.hit->body == box);
    MK_REQUIRE(std::abs(box_hit.hit->distance - 3.0F) < 0.0001F);
    MK_REQUIRE(box_hit.hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));

    mirakana::PhysicsWorld3D sphere_world;
    const auto sphere = sphere_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 0.9F, .z = 0.9F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });

    const auto conservative = sphere_world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });
    MK_REQUIRE(conservative.has_value());
    MK_REQUIRE(conservative->body == sphere);

    const auto exact_miss = sphere_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F}),
    });
    MK_REQUIRE(exact_miss.status == mirakana::PhysicsExactShapeSweep3DStatus::no_hit);
    MK_REQUIRE(!exact_miss.hit.has_value());

    mirakana::PhysicsWorld3D capsule_world;
    const auto capsule = capsule_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 1.0F,
    });
    const auto capsule_hit = capsule_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F}),
    });
    MK_REQUIRE(capsule_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(capsule_hit.hit.has_value());
    MK_REQUIRE(capsule_hit.hit->body == capsule);
    MK_REQUIRE(std::abs(capsule_hit.hit->distance - 3.0F) < 0.0001F);
    MK_REQUIRE(capsule_hit.hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));

    const auto invalid = box_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.0F, .y = 0.5F, .z = 0.5F}),
    });
    MK_REQUIRE(invalid.status == mirakana::PhysicsExactShapeSweep3DStatus::invalid_request);
    MK_REQUIRE(invalid.diagnostic == mirakana::PhysicsExactShapeSweep3DDiagnostic::invalid_request);
    MK_REQUIRE(!invalid.hit.has_value());
}

MK_TEST("3d physics exact shape sweep handles aabb rounded corners diagonal paths and nearest hits") {
    mirakana::PhysicsWorld3D corner_world;
    const auto rounded_corner_sphere = corner_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.8F, .z = 0.8F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });
    const auto corner_hit = corner_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F}),
    });
    MK_REQUIRE(corner_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(corner_hit.hit.has_value());
    MK_REQUIRE(corner_hit.hit->body == rounded_corner_sphere);
    MK_REQUIRE(std::abs(corner_hit.hit->distance - 3.2354249F) < 0.0001F);

    mirakana::PhysicsWorld3D diagonal_world;
    const auto diagonal_box = diagonal_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 4.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });
    const auto diagonal_hit = diagonal_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F}),
    });
    MK_REQUIRE(diagonal_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(diagonal_hit.hit.has_value());
    MK_REQUIRE(diagonal_hit.hit->body == diagonal_box);
    MK_REQUIRE(std::abs(diagonal_hit.hit->distance - 4.2426405F) < 0.0001F);

    mirakana::PhysicsWorld3D nearest_world;
    const auto disabled = nearest_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = false,
    });
    const auto far = nearest_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 6.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });
    const auto near = nearest_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });
    const auto nearest_hit = nearest_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F}),
    });
    MK_REQUIRE(disabled.value > 0U);
    MK_REQUIRE(far.value > 0U);
    MK_REQUIRE(nearest_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(nearest_hit.hit.has_value());
    MK_REQUIRE(nearest_hit.hit->body == near);
}

MK_TEST("3d physics exact shape sweep supports moving capsule queries") {
    mirakana::PhysicsWorld3D box_world;
    const auto box = box_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });

    const auto box_hit = box_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 1.0F),
    });
    MK_REQUIRE(box_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(box_hit.hit.has_value());
    MK_REQUIRE(box_hit.hit->body == box);
    MK_REQUIRE(std::abs(box_hit.hit->distance - 3.0F) < 0.0001F);
    MK_REQUIRE(box_hit.hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));

    mirakana::PhysicsWorld3D sphere_world;
    const auto sphere = sphere_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });
    const auto sphere_hit = sphere_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 1.0F),
    });
    MK_REQUIRE(sphere_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(sphere_hit.hit.has_value());
    MK_REQUIRE(sphere_hit.hit->body == sphere);
    MK_REQUIRE(std::abs(sphere_hit.hit->distance - 3.0F) < 0.0001F);

    mirakana::PhysicsWorld3D capsule_world;
    const auto capsule = capsule_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
    });
    const auto capsule_hit = capsule_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 1.0F),
    });
    MK_REQUIRE(capsule_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(capsule_hit.hit.has_value());
    MK_REQUIRE(capsule_hit.hit->body == capsule);
    MK_REQUIRE(std::abs(capsule_hit.hit->distance - 3.0F) < 0.0001F);

    mirakana::PhysicsWorld3D miss_world;
    const auto offset_sphere = miss_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 1.4F, .z = 1.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });
    const auto conservative = miss_world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.5F, .z = 0.5F},
        .radius = 0.5F,
        .half_height = 1.0F,
    });
    MK_REQUIRE(conservative.has_value());
    MK_REQUIRE(conservative->body == offset_sphere);

    const auto exact_miss = miss_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 1.0F),
    });
    MK_REQUIRE(exact_miss.status == mirakana::PhysicsExactShapeSweep3DStatus::no_hit);
    MK_REQUIRE(!exact_miss.hit.has_value());

    const auto invalid = capsule_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 0.0F),
    });
    MK_REQUIRE(invalid.status == mirakana::PhysicsExactShapeSweep3DStatus::invalid_request);
    MK_REQUIRE(invalid.diagnostic == mirakana::PhysicsExactShapeSweep3DDiagnostic::invalid_request);
}

MK_TEST("3d physics exact shape sweep handles capsule cap hits and invalid diagnostics") {
    mirakana::PhysicsWorld3D cap_world;
    const auto capsule = cap_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.5F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
    });
    const auto cap_hit = cap_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 0.5F),
    });
    MK_REQUIRE(cap_hit.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(cap_hit.hit.has_value());
    MK_REQUIRE(cap_hit.hit->body == capsule);
    MK_REQUIRE(std::abs(cap_hit.hit->distance - 3.1339746F) < 0.0001F);

    const auto invalid_distance = cap_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = -1.0F,
        .shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 0.5F),
    });
    MK_REQUIRE(invalid_distance.status == mirakana::PhysicsExactShapeSweep3DStatus::invalid_request);
    MK_REQUIRE(invalid_distance.diagnostic == mirakana::PhysicsExactShapeSweep3DDiagnostic::invalid_request);

    const auto invalid_origin = cap_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 0.5F),
    });
    MK_REQUIRE(invalid_origin.status == mirakana::PhysicsExactShapeSweep3DStatus::invalid_request);
    MK_REQUIRE(invalid_origin.diagnostic == mirakana::PhysicsExactShapeSweep3DDiagnostic::invalid_request);

    const auto invalid_direction = cap_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::sphere(0.5F),
    });
    MK_REQUIRE(invalid_direction.status == mirakana::PhysicsExactShapeSweep3DStatus::invalid_request);
    MK_REQUIRE(invalid_direction.diagnostic == mirakana::PhysicsExactShapeSweep3DDiagnostic::invalid_request);

    const auto non_finite_direction = cap_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = std::numeric_limits<float>::infinity(), .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::sphere(0.5F),
    });
    MK_REQUIRE(non_finite_direction.status == mirakana::PhysicsExactShapeSweep3DStatus::invalid_request);
    MK_REQUIRE(non_finite_direction.diagnostic == mirakana::PhysicsExactShapeSweep3DDiagnostic::invalid_request);

    const auto non_finite_distance = cap_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = std::numeric_limits<float>::infinity(),
        .shape = mirakana::PhysicsShape3DDesc::sphere(0.5F),
    });
    MK_REQUIRE(non_finite_distance.status == mirakana::PhysicsExactShapeSweep3DStatus::invalid_request);
    MK_REQUIRE(non_finite_distance.diagnostic == mirakana::PhysicsExactShapeSweep3DDiagnostic::invalid_request);
}

MK_TEST("3d physics exact shape sweep reports initial overlaps filters and deterministic ties") {
    mirakana::PhysicsWorld3D initial_world;
    const auto overlapping = initial_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });
    const auto initial = initial_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F}),
    });
    MK_REQUIRE(initial.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(initial.hit.has_value());
    MK_REQUIRE(initial.hit->body == overlapping);
    MK_REQUIRE(initial.hit->initial_overlap);
    MK_REQUIRE(initial.hit->distance == 0.0F);

    mirakana::PhysicsWorld3D filter_world;
    constexpr std::uint32_t terrain_layer = 1U << 1U;
    constexpr std::uint32_t sensor_layer = 1U << 2U;
    const auto sensor = filter_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
        .collision_layer = sensor_layer,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto solid = filter_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
        .collision_layer = terrain_layer,
    });
    const auto skip_triggers = filter_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::sphere(0.5F),
        .filter = mirakana::PhysicsQueryFilter3D{.collision_mask = terrain_layer | sensor_layer,
                                                 .ignored_body = mirakana::null_physics_body_3d,
                                                 .include_triggers = false},
    });
    MK_REQUIRE(sensor.value > 0U);
    MK_REQUIRE(skip_triggers.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(skip_triggers.hit.has_value());
    MK_REQUIRE(skip_triggers.hit->body == solid);

    const auto ignored = filter_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::sphere(0.5F),
        .filter = mirakana::PhysicsQueryFilter3D{.collision_mask = terrain_layer,
                                                 .ignored_body = solid,
                                                 .include_triggers = true},
    });
    MK_REQUIRE(ignored.status == mirakana::PhysicsExactShapeSweep3DStatus::no_hit);

    mirakana::PhysicsWorld3D tie_world;
    const auto first = tie_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });
    const auto second = tie_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = -1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });
    const auto tie = tie_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::sphere(0.5F),
    });
    MK_REQUIRE(tie.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(tie.hit.has_value());
    MK_REQUIRE(first.value < second.value);
    MK_REQUIRE(tie.hit->body == first);

    mirakana::PhysicsWorld3D epsilon_tie_world;
    const auto lower_id = epsilon_tie_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });
    const auto microscopically_closer = epsilon_tie_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 3.9999995F, .y = -1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });
    const auto epsilon_tie = epsilon_tie_world.exact_shape_sweep(mirakana::PhysicsExactShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DDesc::sphere(0.5F),
    });
    MK_REQUIRE(epsilon_tie.status == mirakana::PhysicsExactShapeSweep3DStatus::hit);
    MK_REQUIRE(epsilon_tie.hit.has_value());
    MK_REQUIRE(lower_id.value < microscopically_closer.value);
    MK_REQUIRE(epsilon_tie.hit->body == lower_id);
}

MK_TEST("3d physics exact sphere cast rejects conservative bounds false positive") {
    mirakana::PhysicsWorld3D world;
    const auto target = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 5.0F, .y = 1.1F, .z = 1.1F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 1.0F,
    });

    const auto conservative = world.shape_sweep(mirakana::PhysicsShapeSweep3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .half_extents = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .radius = 0.5F,
    });
    MK_REQUIRE(conservative.has_value());
    MK_REQUIRE(conservative->body == target);

    const auto exact = world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
    });

    MK_REQUIRE(exact.status == mirakana::PhysicsExactSphereCast3DStatus::no_hit);
    MK_REQUIRE(exact.diagnostic == mirakana::PhysicsExactSphereCast3DDiagnostic::none);
    MK_REQUIRE(!exact.hit.has_value());
}

MK_TEST("3d physics exact sphere cast reports initial overlaps and honors filters") {
    mirakana::PhysicsWorld3D overlap_world;
    const auto overlapping = overlap_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 1.0F,
    });

    const auto initial = overlap_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
    });
    MK_REQUIRE(initial.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(initial.hit.has_value());
    MK_REQUIRE(initial.hit->body == overlapping);
    MK_REQUIRE(initial.hit->initial_overlap);
    MK_REQUIRE(initial.hit->distance == 0.0F);
    MK_REQUIRE(initial.hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));

    mirakana::PhysicsWorld3D filter_world;
    constexpr std::uint32_t terrain_layer = 1U << 1U;
    constexpr std::uint32_t sensor_layer = 1U << 2U;
    const auto sensor = filter_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 1.0F,
        .collision_layer = sensor_layer,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto solid = filter_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 1.0F,
        .collision_layer = terrain_layer,
    });
    const auto disabled = filter_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = false,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 1.0F,
        .collision_layer = terrain_layer,
    });
    MK_REQUIRE(disabled.value > 0U);

    const auto skip_triggers = filter_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
        .collision_mask = terrain_layer | sensor_layer,
        .ignored_body = mirakana::null_physics_body_3d,
        .include_triggers = false,
    });
    MK_REQUIRE(skip_triggers.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(skip_triggers.hit.has_value());
    MK_REQUIRE(skip_triggers.hit->body == solid);

    const auto include_triggers = filter_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
        .collision_mask = terrain_layer | sensor_layer,
    });
    MK_REQUIRE(include_triggers.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(include_triggers.hit.has_value());
    MK_REQUIRE(include_triggers.hit->body == sensor);

    const auto ignored = filter_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
        .collision_mask = terrain_layer,
        .ignored_body = solid,
    });
    MK_REQUIRE(ignored.status == mirakana::PhysicsExactSphereCast3DStatus::no_hit);
    MK_REQUIRE(!ignored.hit.has_value());
}

MK_TEST("3d physics exact sphere cast supports aabb and capsule targets") {
    mirakana::PhysicsWorld3D aabb_world;
    const auto box = aabb_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 1.0F},
    });

    const auto box_hit = aabb_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
    });
    MK_REQUIRE(box_hit.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(box_hit.hit.has_value());
    MK_REQUIRE(box_hit.hit->body == box);
    MK_REQUIRE(std::abs(box_hit.hit->distance - 3.0F) < 0.0001F);
    MK_REQUIRE(box_hit.hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));

    mirakana::PhysicsWorld3D capsule_world;
    const auto capsule = capsule_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 1.0F,
    });

    const auto capsule_hit = capsule_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
    });
    MK_REQUIRE(capsule_hit.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(capsule_hit.hit.has_value());
    MK_REQUIRE(capsule_hit.hit->body == capsule);
    MK_REQUIRE(std::abs(capsule_hit.hit->distance - 3.0F) < 0.0001F);
    MK_REQUIRE(capsule_hit.hit->normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));
}

MK_TEST("3d physics exact sphere cast handles rounded edges caps and deterministic ties") {
    mirakana::PhysicsWorld3D corner_world;
    const auto corner_box = corner_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 1.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });

    const auto corner_hit = corner_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 1.0F,
    });
    MK_REQUIRE(corner_hit.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(corner_hit.hit.has_value());
    MK_REQUIRE(corner_hit.hit->body == corner_box);
    MK_REQUIRE(std::abs(corner_hit.hit->distance - 2.7928932F) < 0.0001F);

    mirakana::PhysicsWorld3D cap_world;
    const auto capsule = cap_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .radius = 0.5F,
        .collision_layer = 1U,
        .collision_mask = 0xFFFF'FFFFU,
        .half_height = 0.5F,
    });

    const auto cap_hit = cap_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
    });
    MK_REQUIRE(cap_hit.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(cap_hit.hit.has_value());
    MK_REQUIRE(cap_hit.hit->body == capsule);
    MK_REQUIRE(std::abs(cap_hit.hit->distance - 3.1339746F) < 0.0001F);

    mirakana::PhysicsWorld3D tie_world;
    const auto first = tie_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });
    const auto second = tie_world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = -1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.5F,
    });

    const auto tie_hit = tie_world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
    });
    MK_REQUIRE(tie_hit.status == mirakana::PhysicsExactSphereCast3DStatus::hit);
    MK_REQUIRE(tie_hit.hit.has_value());
    MK_REQUIRE(first.value < second.value);
    MK_REQUIRE(tie_hit.hit->body == first);
}

MK_TEST("3d physics exact sphere cast rejects invalid requests") {
    mirakana::PhysicsWorld3D world;
    const auto invalid_radius = world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.0F,
    });
    MK_REQUIRE(invalid_radius.status == mirakana::PhysicsExactSphereCast3DStatus::invalid_request);
    MK_REQUIRE(invalid_radius.diagnostic == mirakana::PhysicsExactSphereCast3DDiagnostic::invalid_request);
    MK_REQUIRE(!invalid_radius.hit.has_value());

    const auto invalid_direction = world.exact_sphere_cast(mirakana::PhysicsExactSphereCast3DDesc{
        .origin = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .direction = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .max_distance = 10.0F,
        .radius = 0.5F,
    });
    MK_REQUIRE(invalid_direction.status == mirakana::PhysicsExactSphereCast3DStatus::invalid_request);
    MK_REQUIRE(invalid_direction.diagnostic == mirakana::PhysicsExactSphereCast3DDiagnostic::invalid_request);
    MK_REQUIRE(!invalid_direction.hit.has_value());
}

MK_TEST("3d physics character controller constrains capsule against wall") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto wall = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 2.0F, .z = 2.0F},
    });

    mirakana::PhysicsCharacterController3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F};
    request.radius = 0.5F;
    request.half_height = 0.5F;
    request.skin_width = 0.05F;

    const auto result = mirakana::move_physics_character_controller_3d(world, request);

    MK_REQUIRE(result.status == mirakana::PhysicsCharacterController3DStatus::constrained);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsCharacterController3DDiagnostic::none);
    MK_REQUIRE(!result.grounded);
    MK_REQUIRE(result.contacts.size() == 1);
    MK_REQUIRE(result.contacts[0].body == wall);
    MK_REQUIRE(result.contacts[0].normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(!result.contacts[0].initial_overlap);
    MK_REQUIRE(std::abs(result.position.x - 2.95F) < 0.0001F);
    MK_REQUIRE(std::abs(result.applied_displacement.x - 2.95F) < 0.0001F);
}

MK_TEST("3d physics character controller reports grounded downward sweep") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto floor = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 5.0F, .y = 0.5F, .z = 5.0F},
    });

    mirakana::PhysicsCharacterController3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 0.0F, .y = -3.0F, .z = 0.0F};
    request.radius = 0.5F;
    request.half_height = 0.5F;
    request.skin_width = 0.05F;

    const auto result = mirakana::move_physics_character_controller_3d(world, request);

    MK_REQUIRE(result.status == mirakana::PhysicsCharacterController3DStatus::constrained);
    MK_REQUIRE(result.grounded);
    MK_REQUIRE(result.contacts.size() == 1);
    MK_REQUIRE(result.contacts[0].body == floor);
    MK_REQUIRE(result.contacts[0].grounded);
    MK_REQUIRE(result.contacts[0].normal == (mirakana::Vec3{0.0F, 1.0F, 0.0F}));
    MK_REQUIRE(std::abs(result.position.y - 1.55F) < 0.0001F);
}

MK_TEST("3d physics character controller rejects initial overlap") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto block = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });

    mirakana::PhysicsCharacterController3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    request.radius = 0.5F;
    request.half_height = 0.5F;

    const auto result = mirakana::move_physics_character_controller_3d(world, request);

    MK_REQUIRE(result.status == mirakana::PhysicsCharacterController3DStatus::initial_overlap);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsCharacterController3DDiagnostic::initial_overlap);
    MK_REQUIRE(result.position == request.position);
    MK_REQUIRE(result.contacts.size() == 1);
    MK_REQUIRE(result.contacts[0].body == block);
    MK_REQUIRE(result.contacts[0].initial_overlap);
}

MK_TEST("3d physics character dynamic policy reports solid trigger and push rows") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    constexpr std::uint32_t character_layer = 1U << 0U;
    constexpr std::uint32_t solid_layer = 1U << 1U;
    constexpr std::uint32_t trigger_layer = 1U << 2U;
    constexpr std::uint32_t dynamic_layer = 1U << 3U;

    const auto trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.5F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.1F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = trigger_layer,
        .collision_mask = character_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto crate = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.5F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = dynamic_layer,
        .collision_mask = character_layer,
    });
    const auto wall = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = solid_layer,
        .collision_mask = character_layer,
    });

    mirakana::PhysicsCharacterDynamicPolicy3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F};
    request.radius = 0.5F;
    request.half_height = 0.5F;
    request.character_layer = character_layer;
    request.collision_mask = solid_layer | trigger_layer | dynamic_layer;
    request.include_triggers = true;
    request.skin_width = 0.05F;
    request.dynamic_push_distance = 0.25F;

    const auto before_crate = *world.find_body(crate);
    const auto result = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);

    MK_REQUIRE(result.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::none);
    MK_REQUIRE(!result.stepped);
    MK_REQUIRE(!result.grounded);
    MK_REQUIRE(std::abs(result.position.x - 3.20F) < 0.0001F);
    MK_REQUIRE(std::abs(result.applied_displacement.x - 3.20F) < 0.0001F);
    MK_REQUIRE(std::abs(result.remaining_displacement.x - 1.80F) < 0.0001F);
    MK_REQUIRE(result.rows.size() == 3);
    MK_REQUIRE(result.rows[0].kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::trigger_overlap);
    MK_REQUIRE(result.rows[0].body == trigger);
    MK_REQUIRE(result.rows[1].kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::dynamic_push);
    MK_REQUIRE(result.rows[1].body == crate);
    MK_REQUIRE(result.rows[1].suggested_displacement.x > 0.0F);
    MK_REQUIRE(result.rows[1].suggested_displacement.x <= request.dynamic_push_distance);
    MK_REQUIRE(result.rows[2].kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::solid_contact);
    MK_REQUIRE(result.rows[2].body == wall);
    MK_REQUIRE(result.rows[2].normal == (mirakana::Vec3{-1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(world.find_body(crate)->position == before_crate.position);
    MK_REQUIRE(world.find_body(crate)->velocity == before_crate.velocity);
}

MK_TEST("3d physics character dynamic policy steps over low obstacles and probes ground") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto floor = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = -0.5F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 8.0F, .y = 0.5F, .z = 8.0F},
    });
    const auto low_step = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 0.25F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.25F, .z = 1.0F},
    });

    mirakana::PhysicsCharacterDynamicPolicy3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 1.05F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 3.0F, .y = 0.0F, .z = 0.0F};
    request.radius = 0.5F;
    request.half_height = 0.5F;
    request.step_height = 0.65F;
    request.ground_probe_distance = 0.2F;
    request.skin_width = 0.02F;
    request.grounded_normal_y = 0.70F;
    request.max_slope_normal_y = 0.70F;

    const auto result = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);

    MK_REQUIRE(result.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::stepped);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::none);
    MK_REQUIRE(result.stepped);
    MK_REQUIRE(result.grounded);
    MK_REQUIRE(std::abs(result.position.x - 3.0F) < 0.0001F);
    MK_REQUIRE(std::abs(result.position.y - 1.05F) < 0.0001F);
    MK_REQUIRE(result.rows.size() == 3);
    MK_REQUIRE(result.rows[0].kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::solid_contact);
    MK_REQUIRE(result.rows[0].body == low_step);
    MK_REQUIRE(!result.rows[0].walkable_slope);
    MK_REQUIRE(!result.rows[0].grounded);
    MK_REQUIRE(result.rows[1].kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::step_up);
    MK_REQUIRE(result.rows[1].body == low_step);
    MK_REQUIRE(result.rows[1].suggested_displacement == (mirakana::Vec3{0.0F, request.step_height, 0.0F}));
    MK_REQUIRE(result.rows[2].kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::ground_probe);
    MK_REQUIRE(result.rows[2].body == floor);
    MK_REQUIRE(result.rows[2].grounded);
    MK_REQUIRE(result.rows[2].walkable_slope);
}

MK_TEST("3d physics character dynamic policy blocks high steps and requires trigger opt in") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    constexpr std::uint32_t character_layer = 1U << 0U;
    constexpr std::uint32_t accepted_layer = 1U << 1U;
    constexpr std::uint32_t rejected_layer = 1U << 2U;

    const auto ignored_trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.8F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.1F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = accepted_layer,
        .collision_mask = character_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto reciprocal_mask_reject = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.2F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.1F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = accepted_layer,
        .collision_mask = rejected_layer,
    });
    const auto high_step = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 0.75F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.75F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = accepted_layer,
        .collision_mask = character_layer,
    });

    mirakana::PhysicsCharacterDynamicPolicy3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 1.05F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 3.0F, .y = 0.0F, .z = 0.0F};
    request.character_layer = character_layer;
    request.collision_mask = accepted_layer;
    request.include_triggers = false;
    request.step_height = 0.4F;
    request.skin_width = 0.02F;

    const auto result = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);

    MK_REQUIRE(ignored_trigger != mirakana::null_physics_body_3d);
    MK_REQUIRE(reciprocal_mask_reject != mirakana::null_physics_body_3d);
    MK_REQUIRE(result.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::step_blocked);
    MK_REQUIRE(!result.stepped);
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.rows[0].body == high_step);
    MK_REQUIRE(result.rows[0].kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::solid_contact);
    MK_REQUIRE(std::abs(result.position.x - 1.23F) < 0.0001F);
}

MK_TEST("3d physics character dynamic policy rejects steps that land inside blockers") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto low_step = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 0.25F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.25F, .z = 1.0F},
    });

    mirakana::PhysicsCharacterDynamicPolicy3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 1.05F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F};
    request.step_height = 0.65F;
    request.skin_width = 0.02F;

    const auto result = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);

    MK_REQUIRE(result.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained);
    MK_REQUIRE(result.diagnostic == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::step_blocked);
    MK_REQUIRE(!result.stepped);
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.rows[0].body == low_step);
    MK_REQUIRE(result.position.x < 2.0F);
}

MK_TEST("3d physics character dynamic policy propagates grounded blocking contacts without probes") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto floor = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = -0.5F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 8.0F, .y = 0.5F, .z = 8.0F},
    });

    mirakana::PhysicsCharacterDynamicPolicy3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 1.5F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    request.ground_probe_distance = 0.0F;
    request.skin_width = 0.02F;

    const auto result = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);

    MK_REQUIRE(result.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained);
    MK_REQUIRE(result.grounded);
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.rows[0].body == floor);
    MK_REQUIRE(result.rows[0].grounded);
    MK_REQUIRE(result.rows[0].walkable_slope);
}

MK_TEST("3d physics character dynamic policy honors filters and rejects invalid requests") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    constexpr std::uint32_t character_layer = 1U << 0U;
    constexpr std::uint32_t accepted_layer = 1U << 1U;
    constexpr std::uint32_t rejected_layer = 1U << 2U;

    const auto rejected_trigger = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.1F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = rejected_layer,
        .collision_mask = character_layer,
        .half_height = 0.5F,
        .trigger = true,
    });
    const auto rejected_dynamic = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 2.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.5F, .z = 0.5F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = rejected_layer,
        .collision_mask = character_layer,
    });
    const auto accepted_wall = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::aabb,
        .radius = 0.5F,
        .collision_layer = accepted_layer,
        .collision_mask = character_layer,
    });

    mirakana::PhysicsCharacterDynamicPolicy3DDesc request;
    request.position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F};
    request.displacement = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F};
    request.character_layer = character_layer;
    request.collision_mask = accepted_layer;
    request.include_triggers = true;
    request.skin_width = 0.05F;

    const auto result = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);

    MK_REQUIRE(rejected_trigger != mirakana::null_physics_body_3d);
    MK_REQUIRE(rejected_dynamic != mirakana::null_physics_body_3d);
    MK_REQUIRE(result.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained);
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.rows[0].body == accepted_wall);
    MK_REQUIRE(result.rows[0].kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::solid_contact);

    const auto before = *world.find_body(accepted_wall);
    request.radius = 0.0F;
    const auto invalid = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);
    MK_REQUIRE(invalid.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::invalid_request);
    MK_REQUIRE(invalid.diagnostic == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::invalid_request);
    MK_REQUIRE(invalid.rows.empty());
    MK_REQUIRE(world.find_body(accepted_wall)->position == before.position);
}

MK_TEST("3d physics authored collision scene builds deterministic world") {
    mirakana::PhysicsAuthoredCollisionScene3DDesc scene;
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "floor",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                .mass = 0.0F,
                .linear_damping = 0.0F,
                .dynamic = false,
                .half_extents = mirakana::Vec3{.x = 5.0F, .y = 0.5F, .z = 5.0F},
            },
    });
    scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
        .name = "wall",
        .body =
            mirakana::PhysicsBody3DDesc{
                .position = mirakana::Vec3{.x = 3.0F, .y = 1.0F, .z = 0.0F},
                .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                .mass = 0.0F,
                .linear_damping = 0.0F,
                .dynamic = false,
                .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 2.0F},
            },
    });

    const auto built = mirakana::build_physics_world_3d_from_authored_collision_scene(scene);

    MK_REQUIRE(built.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::success);
    MK_REQUIRE(built.diagnostic == mirakana::PhysicsAuthoredCollision3DDiagnostic::none);
    MK_REQUIRE(built.bodies.size() == 2);
    MK_REQUIRE(built.bodies[0].name == "floor");
    MK_REQUIRE(built.bodies[0].source_index == 0U);
    MK_REQUIRE(built.bodies[1].name == "wall");
    MK_REQUIRE(built.world.bodies().size() == 2);
    MK_REQUIRE(built.world.find_body(built.bodies[0].body) != nullptr);
    MK_REQUIRE(!built.world.find_body(built.bodies[0].body)->dynamic);
}

MK_TEST("3d physics authored collision scene rejects duplicates and native backend requests") {
    mirakana::PhysicsAuthoredCollisionScene3DDesc duplicate;
    duplicate.bodies.push_back(
        mirakana::PhysicsAuthoredCollisionBody3DDesc{.name = "floor", .body = mirakana::PhysicsBody3DDesc{}});
    duplicate.bodies.push_back(
        mirakana::PhysicsAuthoredCollisionBody3DDesc{.name = "floor", .body = mirakana::PhysicsBody3DDesc{}});

    const auto duplicate_result = mirakana::build_physics_world_3d_from_authored_collision_scene(duplicate);
    MK_REQUIRE(duplicate_result.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::duplicate_name);
    MK_REQUIRE(duplicate_result.diagnostic == mirakana::PhysicsAuthoredCollision3DDiagnostic::duplicate_body_name);
    MK_REQUIRE(duplicate_result.body_index == 1U);
    MK_REQUIRE(duplicate_result.world.bodies().empty());

    mirakana::PhysicsAuthoredCollisionScene3DDesc native_request;
    native_request.require_native_backend = true;
    native_request.bodies.push_back(
        mirakana::PhysicsAuthoredCollisionBody3DDesc{.name = "floor", .body = mirakana::PhysicsBody3DDesc{}});

    const auto native_result = mirakana::build_physics_world_3d_from_authored_collision_scene(native_request);
    MK_REQUIRE(native_result.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::unsupported_native_backend);
    MK_REQUIRE(native_result.diagnostic == mirakana::PhysicsAuthoredCollision3DDiagnostic::native_backend_unsupported);
    MK_REQUIRE(native_result.body_index == 0U);
    MK_REQUIRE(native_result.world.bodies().empty());
}

MK_TEST("3d physics world reports sphere contacts and rejects invalid collision filters") {
    MK_REQUIRE(!mirakana::is_valid_physics_body_desc(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        1.0F,
        0.0F,
        true,
        mirakana::Vec3{1.0F, 1.0F, 1.0F},
        true,
        mirakana::PhysicsShape3DKind::sphere,
        0.0F,
        1U,
        1U,
    }));
    MK_REQUIRE(!mirakana::is_valid_physics_body_desc(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        1.0F,
        0.0F,
        true,
        mirakana::Vec3{1.0F, 1.0F, 1.0F},
        true,
        mirakana::PhysicsShape3DKind::aabb,
        0.5F,
        0U,
        1U,
    }));

    mirakana::PhysicsWorld3D world;
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 1.0F,
        .collision_layer = 1U,
        .collision_mask = 1U,
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.5F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 1.0F,
        .collision_layer = 1U,
        .collision_mask = 1U,
    });

    const auto contacts = world.contacts();

    MK_REQUIRE(contacts.size() == 1);
    MK_REQUIRE(contacts[0].first == first);
    MK_REQUIRE(contacts[0].second == second);
    MK_REQUIRE(contacts[0].normal == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(contacts[0].penetration_depth - 0.5F) < 0.0001F);
}

MK_TEST("3d physics world reports capsule sphere contacts") {
    MK_REQUIRE(!mirakana::is_valid_physics_body_desc(mirakana::PhysicsBody3DDesc{
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        mirakana::Vec3{0.0F, 0.0F, 0.0F},
        1.0F,
        0.0F,
        true,
        mirakana::Vec3{1.0F, 1.0F, 1.0F},
        true,
        mirakana::PhysicsShape3DKind::capsule,
        0.25F,
        1U,
        1U,
        0.0F,
    }));

    mirakana::PhysicsWorld3D world;
    const auto capsule = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::capsule,
        .radius = 0.25F,
        .collision_layer = 1U,
        .collision_mask = 1U,
        .half_height = 0.75F,
    });
    const auto sphere = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.4F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
        .collision_enabled = true,
        .shape = mirakana::PhysicsShape3DKind::sphere,
        .radius = 0.35F,
        .collision_layer = 1U,
        .collision_mask = 1U,
    });

    const auto contacts = world.contacts();

    MK_REQUIRE(contacts.size() == 1);
    MK_REQUIRE(contacts[0].first == capsule);
    MK_REQUIRE(contacts[0].second == sphere);
    MK_REQUIRE(contacts[0].normal.y > 0.4F);
    MK_REQUIRE(contacts[0].normal.z > 0.7F);
    MK_REQUIRE(contacts[0].penetration_depth > 0.12F);
}

MK_TEST("3d physics world resolves dynamic body against static body") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto floor = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 5.0F, .y = 1.0F, .z = 5.0F},
    });
    const auto box = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 1.5F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = -2.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 0.5F},
    });

    world.resolve_contacts();

    const auto* floor_state = world.find_body(floor);
    const auto* box_state = world.find_body(box);
    MK_REQUIRE(floor_state != nullptr);
    MK_REQUIRE(box_state != nullptr);
    MK_REQUIRE(floor_state->position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(box_state->position.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(box_state->velocity.y) < 0.0001F);
}

MK_TEST("3d physics world splits contact correction by inverse mass") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto first = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
    });
    const auto second = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 1.5F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
    });

    world.resolve_contacts();

    MK_REQUIRE(std::abs(world.find_body(first)->position.x - -0.25F) < 0.0001F);
    MK_REQUIRE(std::abs(world.find_body(second)->position.x - 1.75F) < 0.0001F);
}

MK_TEST("3d physics contact solver preserves tangent velocity and rejects invalid config") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto floor = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 5.0F, .y = 1.0F, .z = 5.0F},
    });
    const auto box = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 1.5F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 2.0F, .y = -2.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 0.5F},
    });

    bool rejected_iterations = false;
    try {
        world.resolve_contacts(mirakana::PhysicsContactSolver3DConfig{
            .restitution = 0.0F, .iterations = 0U, .position_correction_percent = 1.0F, .penetration_slop = 0.0F});
    } catch (const std::invalid_argument&) {
        rejected_iterations = true;
    }
    MK_REQUIRE(rejected_iterations);

    bool rejected_non_finite_restitution = false;
    try {
        world.resolve_contacts(
            mirakana::PhysicsContactSolver3DConfig{.restitution = std::numeric_limits<float>::infinity(),
                                                   .iterations = 1U,
                                                   .position_correction_percent = 1.0F,
                                                   .penetration_slop = 0.0F});
    } catch (const std::invalid_argument&) {
        rejected_non_finite_restitution = true;
    }
    MK_REQUIRE(rejected_non_finite_restitution);

    world.resolve_contacts(mirakana::PhysicsContactSolver3DConfig{
        .restitution = 0.0F, .iterations = 8U, .position_correction_percent = 0.85F, .penetration_slop = 0.001F});

    MK_REQUIRE(floor != mirakana::null_physics_body_3d);
    const auto* box_state = world.find_body(box);
    MK_REQUIRE(box_state != nullptr);
    MK_REQUIRE(std::abs(box_state->velocity.x - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(box_state->velocity.y) < 0.0001F);
}

MK_TEST("3d physics iterative solver produces deterministic replay state") {
    auto create_world = [] {
        mirakana::PhysicsWorld3D world(
            mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F}});
        const auto floor = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 3.0F, .y = 0.5F, .z = 3.0F},
        });
        const auto box = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 0.75F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = -0.5F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
        });
        MK_REQUIRE(floor != mirakana::null_physics_body_3d);
        MK_REQUIRE(box != mirakana::null_physics_body_3d);
        return world;
    };

    auto first = create_world();
    auto second = create_world();
    const mirakana::PhysicsContactSolver3DConfig solver{.restitution = 0.0F, .iterations = 4U};
    auto require_matching_manifolds = [](const mirakana::PhysicsWorld3D& lhs, const mirakana::PhysicsWorld3D& rhs) {
        const auto first_manifolds = lhs.contact_manifolds();
        const auto second_manifolds = rhs.contact_manifolds();
        MK_REQUIRE(first_manifolds.size() == second_manifolds.size());
        for (std::size_t manifold_index = 0; manifold_index < first_manifolds.size(); ++manifold_index) {
            MK_REQUIRE(first_manifolds[manifold_index].first == second_manifolds[manifold_index].first);
            MK_REQUIRE(first_manifolds[manifold_index].second == second_manifolds[manifold_index].second);
            MK_REQUIRE(first_manifolds[manifold_index].normal == second_manifolds[manifold_index].normal);
            MK_REQUIRE(first_manifolds[manifold_index].points.size() == second_manifolds[manifold_index].points.size());
            for (std::size_t point_index = 0; point_index < first_manifolds[manifold_index].points.size();
                 ++point_index) {
                MK_REQUIRE(first_manifolds[manifold_index].points[point_index].feature_id ==
                           second_manifolds[manifold_index].points[point_index].feature_id);
                MK_REQUIRE(first_manifolds[manifold_index].points[point_index].warm_start_eligible ==
                           second_manifolds[manifold_index].points[point_index].warm_start_eligible);
            }
        }
    };

    for (int step = 0; step < 3; ++step) {
        require_matching_manifolds(first, second);
        first.resolve_contacts(solver);
        second.resolve_contacts(solver);
        require_matching_manifolds(first, second);
        first.step(1.0F / 60.0F);
        second.step(1.0F / 60.0F);
    }

    MK_REQUIRE(first.broadphase_pairs().size() == second.broadphase_pairs().size());
    MK_REQUIRE(first.contacts().size() == second.contacts().size());
    require_matching_manifolds(first, second);
    MK_REQUIRE(first.bodies().size() == second.bodies().size());
    for (std::size_t index = 0; index < first.bodies().size(); ++index) {
        MK_REQUIRE(first.bodies()[index].position == second.bodies()[index].position);
        MK_REQUIRE(first.bodies()[index].velocity == second.bodies()[index].velocity);
    }
}

MK_TEST("3d physics iterative solver stabilizes simple stacked bodies") {
    mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
    const auto floor = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = -0.5F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        .mass = 0.0F,
        .linear_damping = 0.0F,
        .dynamic = false,
        .half_extents = mirakana::Vec3{.x = 3.0F, .y = 0.5F, .z = 3.0F},
    });
    const auto lower = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 0.35F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });
    const auto upper = world.create_body(mirakana::PhysicsBody3DDesc{
        .position = mirakana::Vec3{.x = 0.0F, .y = 1.15F, .z = 0.0F},
        .velocity = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
        .mass = 1.0F,
        .linear_damping = 0.0F,
        .dynamic = true,
        .half_extents = mirakana::Vec3{.x = 0.5F, .y = 0.5F, .z = 0.5F},
    });

    world.resolve_contacts(mirakana::PhysicsContactSolver3DConfig{
        .restitution = 0.0F, .iterations = 16U, .position_correction_percent = 0.85F, .penetration_slop = 0.001F});

    MK_REQUIRE(floor != mirakana::null_physics_body_3d);
    MK_REQUIRE(world.find_body(lower)->position.y > 0.49F);
    MK_REQUIRE(world.find_body(upper)->position.y > 1.48F);
    MK_REQUIRE(std::abs(world.find_body(lower)->velocity.y) < 0.0001F);
    MK_REQUIRE(std::abs(world.find_body(upper)->velocity.y) < 0.0001F);
}

MK_TEST("audio mixer builds deterministic gain plan") {
    mirakana::AudioMixer mixer;
    mixer.add_bus(mirakana::AudioBusDesc{.name = "sfx", .gain = 0.5F, .muted = false});
    const auto clip = mirakana::AssetId::from_name("audio/jump.wav");

    MK_REQUIRE(mixer.play(mirakana::AudioVoiceDesc{clip, "sfx", 0.8F, false}).value == 1);

    const auto plan = mixer.mix_plan();
    MK_REQUIRE(plan.size() == 1);
    MK_REQUIRE(plan[0].clip == clip);
    MK_REQUIRE(plan[0].bus == "sfx");
    MK_REQUIRE(std::abs(plan[0].gain - 0.4F) < 0.0001F);

    mixer.set_bus_muted("sfx", true);
    MK_REQUIRE(mixer.mix_plan()[0].gain == 0.0F);
}

MK_TEST("audio mixer builds deterministic spatial voice plan") {
    mirakana::AudioMixer mixer;
    mixer.add_bus(mirakana::AudioBusDesc{.name = "sfx", .gain = 0.5F, .muted = false});
    const auto clip = mirakana::AssetId::from_name("audio/step");
    const auto voice = mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "sfx", .gain = 0.8F, .looping = false});

    const std::vector<mirakana::AudioSpatialVoiceDesc> spatial_sources{
        mirakana::AudioSpatialVoiceDesc{.voice = voice,
                                        .position = mirakana::AudioPoint3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
                                        .min_distance = 1.0F,
                                        .max_distance = 9.0F,
                                        .spatialized = true},
    };
    const auto plan = mixer.spatial_mix_plan(
        mirakana::AudioSpatialListenerDesc{.position = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                           .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
        spatial_sources);

    MK_REQUIRE(plan.size() == 1);
    MK_REQUIRE(plan[0].voice == voice);
    MK_REQUIRE(plan[0].clip == clip);
    MK_REQUIRE(plan[0].spatialized);
    MK_REQUIRE(std::abs(plan[0].distance - 5.0F) < 0.0001F);
    MK_REQUIRE(std::abs(plan[0].attenuation - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(plan[0].pan - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(plan[0].center_gain - 0.2F) < 0.0001F);
    MK_REQUIRE(std::abs(plan[0].left_gain - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(plan[0].right_gain - 0.2F) < 0.0001F);

    const std::vector<mirakana::AudioSpatialVoiceDesc> disabled_sources{
        mirakana::AudioSpatialVoiceDesc{.voice = voice,
                                        .position = mirakana::AudioPoint3{.x = 5.0F, .y = 0.0F, .z = 0.0F},
                                        .min_distance = 1.0F,
                                        .max_distance = 9.0F,
                                        .spatialized = false},
    };
    const auto disabled_plan = mixer.spatial_mix_plan(
        mirakana::AudioSpatialListenerDesc{.position = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                           .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
        disabled_sources);
    MK_REQUIRE(disabled_plan.size() == 1);
    MK_REQUIRE(!disabled_plan[0].spatialized);
    MK_REQUIRE(std::abs(disabled_plan[0].center_gain - 0.4F) < 0.0001F);
    MK_REQUIRE(std::abs(disabled_plan[0].left_gain - 0.4F) < 0.0001F);
    MK_REQUIRE(std::abs(disabled_plan[0].right_gain - 0.4F) < 0.0001F);

    const auto nan = std::numeric_limits<float>::quiet_NaN();
    const std::vector<mirakana::AudioSpatialVoiceDesc> disabled_invalid_sources{
        mirakana::AudioSpatialVoiceDesc{.voice = voice,
                                        .position = mirakana::AudioPoint3{.x = nan, .y = 0.0F, .z = 0.0F},
                                        .min_distance = 4.0F,
                                        .max_distance = 2.0F,
                                        .spatialized = false},
    };
    MK_REQUIRE(mirakana::is_valid_audio_spatial_voice_desc(disabled_invalid_sources[0]));
    const auto disabled_invalid_plan = mixer.spatial_mix_plan(
        mirakana::AudioSpatialListenerDesc{.position = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                           .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
        disabled_invalid_sources);
    MK_REQUIRE(disabled_invalid_plan.size() == 1);
    MK_REQUIRE(!disabled_invalid_plan[0].spatialized);
    MK_REQUIRE(std::abs(disabled_invalid_plan[0].center_gain - 0.4F) < 0.0001F);

    MK_REQUIRE(!mirakana::is_valid_audio_spatial_listener_desc(mirakana::AudioSpatialListenerDesc{
        mirakana::AudioPoint3{0.0F, 0.0F, 0.0F}, mirakana::AudioPoint3{0.0F, 0.0F, 0.0F}}));
    MK_REQUIRE(!mirakana::is_valid_audio_spatial_listener_desc(mirakana::AudioSpatialListenerDesc{
        mirakana::AudioPoint3{0.0F, 0.0F, 0.0F},
        mirakana::AudioPoint3{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
                              std::numeric_limits<float>::max()}}));
    MK_REQUIRE(!mirakana::is_valid_audio_spatial_voice_desc(
        mirakana::AudioSpatialVoiceDesc{voice, mirakana::AudioPoint3{0.0F, 0.0F, 0.0F}, 4.0F, 2.0F, true}));

    bool rejected_listener = false;
    try {
        (void)mixer.spatial_mix_plan(
            mirakana::AudioSpatialListenerDesc{.position = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                               .right = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
            spatial_sources);
    } catch (const std::invalid_argument&) {
        rejected_listener = true;
    }
    MK_REQUIRE(rejected_listener);

    bool rejected_duplicate_voice = false;
    try {
        const std::vector<mirakana::AudioSpatialVoiceDesc> duplicates{
            mirakana::AudioSpatialVoiceDesc{.voice = voice,
                                            .position = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                            .min_distance = 1.0F,
                                            .max_distance = 9.0F,
                                            .spatialized = true},
            mirakana::AudioSpatialVoiceDesc{.voice = voice,
                                            .position = mirakana::AudioPoint3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
                                            .min_distance = 1.0F,
                                            .max_distance = 9.0F,
                                            .spatialized = true},
        };
        (void)mixer.spatial_mix_plan(
            mirakana::AudioSpatialListenerDesc{.position = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                               .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
            duplicates);
    } catch (const std::invalid_argument&) {
        rejected_duplicate_voice = true;
    }
    MK_REQUIRE(rejected_duplicate_voice);

    bool rejected_nonfinite_distance = false;
    try {
        const std::vector<mirakana::AudioSpatialVoiceDesc> distant_sources{
            mirakana::AudioSpatialVoiceDesc{.voice = voice,
                                            .position = mirakana::AudioPoint3{.x = std::numeric_limits<float>::max(),
                                                                              .y = std::numeric_limits<float>::max(),
                                                                              .z = std::numeric_limits<float>::max()},
                                            .min_distance = 1.0F,
                                            .max_distance = 9.0F,
                                            .spatialized = true},
        };
        (void)mixer.spatial_mix_plan(
            mirakana::AudioSpatialListenerDesc{.position = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                               .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
            distant_sources);
    } catch (const std::invalid_argument&) {
        rejected_nonfinite_distance = true;
    }
    MK_REQUIRE(rejected_nonfinite_distance);
}

MK_TEST("audio mixer spatial plan covers edge cases and replay") {
    mirakana::AudioMixer mixer;
    const auto centered = mixer.play(mirakana::AudioVoiceDesc{
        .clip = mirakana::AssetId::from_name("audio/center"), .bus = "master", .gain = 1.0F, .looping = false});
    const auto left = mixer.play(mirakana::AudioVoiceDesc{
        .clip = mirakana::AssetId::from_name("audio/left"), .bus = "master", .gain = 1.0F, .looping = false});
    const auto min = mixer.play(mirakana::AudioVoiceDesc{
        .clip = mirakana::AssetId::from_name("audio/min"), .bus = "master", .gain = 1.0F, .looping = false});
    const auto max = mixer.play(mirakana::AudioVoiceDesc{
        .clip = mirakana::AssetId::from_name("audio/max"), .bus = "master", .gain = 1.0F, .looping = false});
    const auto beyond = mixer.play(mirakana::AudioVoiceDesc{
        .clip = mirakana::AssetId::from_name("audio/beyond"), .bus = "master", .gain = 1.0F, .looping = false});
    const auto missing = mixer.play(mirakana::AudioVoiceDesc{
        .clip = mirakana::AssetId::from_name("audio/missing"), .bus = "master", .gain = 1.0F, .looping = false});

    const std::vector<mirakana::AudioSpatialVoiceDesc> spatial_sources{
        mirakana::AudioSpatialVoiceDesc{.voice = centered,
                                        .position = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                        .min_distance = 1.0F,
                                        .max_distance = 9.0F,
                                        .spatialized = true},
        mirakana::AudioSpatialVoiceDesc{.voice = left,
                                        .position = mirakana::AudioPoint3{.x = -5.0F, .y = 0.0F, .z = 0.0F},
                                        .min_distance = 1.0F,
                                        .max_distance = 9.0F,
                                        .spatialized = true},
        mirakana::AudioSpatialVoiceDesc{.voice = min,
                                        .position = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                        .min_distance = 1.0F,
                                        .max_distance = 9.0F,
                                        .spatialized = true},
        mirakana::AudioSpatialVoiceDesc{.voice = max,
                                        .position = mirakana::AudioPoint3{.x = 9.0F, .y = 0.0F, .z = 0.0F},
                                        .min_distance = 1.0F,
                                        .max_distance = 9.0F,
                                        .spatialized = true},
        mirakana::AudioSpatialVoiceDesc{.voice = beyond,
                                        .position = mirakana::AudioPoint3{.x = 10.0F, .y = 0.0F, .z = 0.0F},
                                        .min_distance = 1.0F,
                                        .max_distance = 9.0F,
                                        .spatialized = true},
    };
    const auto listener =
        mirakana::AudioSpatialListenerDesc{.position = mirakana::AudioPoint3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                           .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F}};
    const auto plan = mixer.spatial_mix_plan(listener, spatial_sources);
    const auto replay = mixer.spatial_mix_plan(listener, spatial_sources);

    MK_REQUIRE(plan.size() == 6);
    MK_REQUIRE(replay.size() == plan.size());
    for (std::size_t index = 0; index < plan.size(); ++index) {
        MK_REQUIRE(replay[index].voice == plan[index].voice);
        MK_REQUIRE(replay[index].spatialized == plan[index].spatialized);
        MK_REQUIRE(std::abs(replay[index].distance - plan[index].distance) < 0.0001F);
        MK_REQUIRE(std::abs(replay[index].attenuation - plan[index].attenuation) < 0.0001F);
        MK_REQUIRE(std::abs(replay[index].pan - plan[index].pan) < 0.0001F);
        MK_REQUIRE(std::abs(replay[index].left_gain - plan[index].left_gain) < 0.0001F);
        MK_REQUIRE(std::abs(replay[index].right_gain - plan[index].right_gain) < 0.0001F);
    }

    MK_REQUIRE(plan[0].voice == centered);
    MK_REQUIRE(plan[0].spatialized);
    MK_REQUIRE(plan[0].distance == 0.0F);
    MK_REQUIRE(plan[0].attenuation == 1.0F);
    MK_REQUIRE(plan[0].pan == 0.0F);
    MK_REQUIRE(plan[0].left_gain == 1.0F);
    MK_REQUIRE(plan[0].right_gain == 1.0F);

    MK_REQUIRE(plan[1].voice == left);
    MK_REQUIRE(std::abs(plan[1].attenuation - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(plan[1].pan + 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(plan[1].left_gain - 0.5F) < 0.0001F);
    MK_REQUIRE(plan[1].right_gain == 0.0F);

    MK_REQUIRE(plan[2].voice == min);
    MK_REQUIRE(plan[2].attenuation == 1.0F);
    MK_REQUIRE(plan[2].pan == 1.0F);
    MK_REQUIRE(plan[2].left_gain == 0.0F);
    MK_REQUIRE(plan[2].right_gain == 1.0F);

    MK_REQUIRE(plan[3].voice == max);
    MK_REQUIRE(plan[3].attenuation == 0.0F);
    MK_REQUIRE(plan[3].left_gain == 0.0F);
    MK_REQUIRE(plan[3].right_gain == 0.0F);

    MK_REQUIRE(plan[4].voice == beyond);
    MK_REQUIRE(plan[4].attenuation == 0.0F);
    MK_REQUIRE(plan[4].left_gain == 0.0F);
    MK_REQUIRE(plan[4].right_gain == 0.0F);

    MK_REQUIRE(plan[5].voice == missing);
    MK_REQUIRE(!plan[5].spatialized);
    MK_REQUIRE(plan[5].center_gain == 1.0F);
    MK_REQUIRE(plan[5].left_gain == 1.0F);
    MK_REQUIRE(plan[5].right_gain == 1.0F);
}

MK_TEST("audio mixer render plan schedules voices and reports streaming underruns") {
    mirakana::AudioMixer mixer;
    mixer.add_bus(mirakana::AudioBusDesc{.name = "music", .gain = 0.5F, .muted = false});
    const auto clip = mirakana::AssetId::from_name("audio/theme");

    MK_REQUIRE(mixer.register_clip(mirakana::AudioClipDesc{
        clip,
        48000,
        2,
        1024,
        mirakana::AudioSampleFormat::float32,
        true,
        192,
    }));

    const auto voice = mixer.play(
        mirakana::AudioVoiceDesc{.clip = clip, .bus = "music", .gain = 0.75F, .looping = false, .start_frame = 128});
    const auto plan = mixer.render_plan(mirakana::AudioRenderRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .frame_count = 512,
        .device_frame = 0,
        .underrun_warning_threshold_frames = 64,
    });

    MK_REQUIRE(plan.commands.size() == 1);
    MK_REQUIRE(plan.commands[0].voice == voice);
    MK_REQUIRE(plan.commands[0].output_offset_frames == 128);
    MK_REQUIRE(plan.commands[0].output_frame_count == 384);
    MK_REQUIRE(plan.commands[0].source_start_frame == 0);
    MK_REQUIRE(plan.commands[0].source_frame_count == 384);
    MK_REQUIRE(std::abs(plan.commands[0].gain - 0.375F) < 0.0001F);
    MK_REQUIRE(plan.commands[0].conversion == mirakana::AudioFormatConversion::none);

    MK_REQUIRE(plan.underruns.size() == 1);
    MK_REQUIRE(plan.underruns[0].voice == voice);
    MK_REQUIRE(plan.underruns[0].requested_source_frames == 384);
    MK_REQUIRE(plan.underruns[0].buffered_source_frames == 192);
    MK_REQUIRE(plan.underruns[0].missing_source_frames == 192);
}

MK_TEST("audio mixer render plan saturates streaming underrun threshold near frame limit") {
    mirakana::AudioMixer mixer;
    const auto clip = mirakana::AssetId::from_name("audio/long-stream");
    const auto max_frame = std::numeric_limits<std::uint64_t>::max();

    mixer.add_clip(mirakana::AudioClipDesc{
        .clip = clip,
        .sample_rate = 48000,
        .channel_count = 2,
        .frame_count = max_frame,
        .sample_format = mirakana::AudioSampleFormat::float32,
        .streaming = true,
        .buffered_frame_count = max_frame - 1U,
    });
    const auto voice =
        mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});

    const auto plan = mixer.render_plan(mirakana::AudioRenderRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .frame_count = 16,
        .device_frame = max_frame - 16U,
        .underrun_warning_threshold_frames = 1,
    });

    MK_REQUIRE(plan.commands.size() == 1);
    MK_REQUIRE(plan.commands[0].voice == voice);
    MK_REQUIRE(plan.commands[0].source_start_frame == max_frame - 16U);
    MK_REQUIRE(plan.commands[0].source_frame_count == 16);
    MK_REQUIRE(plan.underruns.size() == 1);
    MK_REQUIRE(plan.underruns[0].requested_source_frames == 16);
    MK_REQUIRE(plan.underruns[0].buffered_source_frames == 15);
    MK_REQUIRE(plan.underruns[0].missing_source_frames == 1);
}

MK_TEST("audio streaming queue consumes decoded chunks and reports starvation") {
    const auto clip = mirakana::AssetId::from_name("audio/dialog");
    const auto make_queue = [clip]() {
        return mirakana::AudioStreamingQueue(mirakana::AudioClipDesc{
            .clip = clip,
            .sample_rate = 48000,
            .channel_count = 2,
            .frame_count = 1024,
            .sample_format = mirakana::AudioSampleFormat::float32,
            .streaming = true,
            .buffered_frame_count = 0,
        });
    };
    const auto same_consume = [](const mirakana::AudioStreamingConsumeResult& lhs,
                                 const mirakana::AudioStreamingConsumeResult& rhs) {
        return lhs.requested_frames == rhs.requested_frames && lhs.consumed_frames == rhs.consumed_frames &&
               lhs.missing_frames == rhs.missing_frames && lhs.next_read_cursor_frame == rhs.next_read_cursor_frame &&
               lhs.starved == rhs.starved;
    };

    bool rejected_non_streaming = false;
    try {
        mirakana::AudioStreamingQueue rejected(mirakana::AudioClipDesc{
            .clip = clip,
            .sample_rate = 48000,
            .channel_count = 2,
            .frame_count = 1024,
            .sample_format = mirakana::AudioSampleFormat::float32,
            .streaming = false,
            .buffered_frame_count = 0,
        });
        (void)rejected;
    } catch (const std::invalid_argument&) {
        rejected_non_streaming = true;
    }
    MK_REQUIRE(rejected_non_streaming);

    bool rejected_prebuffered = false;
    try {
        mirakana::AudioStreamingQueue rejected(mirakana::AudioClipDesc{
            .clip = clip,
            .sample_rate = 48000,
            .channel_count = 2,
            .frame_count = 1024,
            .sample_format = mirakana::AudioSampleFormat::float32,
            .streaming = true,
            .buffered_frame_count = 1,
        });
        (void)rejected;
    } catch (const std::invalid_argument&) {
        rejected_prebuffered = true;
    }
    MK_REQUIRE(rejected_prebuffered);

    auto queue = make_queue();

    MK_REQUIRE(queue.read_cursor_frame() == 0);
    MK_REQUIRE(queue.buffered_frame_count() == 0);
    MK_REQUIRE(queue.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        0,
        256,
    }));
    MK_REQUIRE(queue.buffered_frame_count() == 256);

    const auto first = queue.consume(128);
    MK_REQUIRE(first.requested_frames == 128);
    MK_REQUIRE(first.consumed_frames == 128);
    MK_REQUIRE(first.missing_frames == 0);
    MK_REQUIRE(!first.starved);
    MK_REQUIRE(first.next_read_cursor_frame == 128);
    MK_REQUIRE(queue.buffered_frame_count() == 128);

    MK_REQUIRE(!queue.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        512,
        128,
    }));
    MK_REQUIRE(queue.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        256,
        256,
    }));
    MK_REQUIRE(queue.buffered_frame_count() == 384);

    const auto second = queue.consume(512);
    MK_REQUIRE(second.requested_frames == 512);
    MK_REQUIRE(second.consumed_frames == 384);
    MK_REQUIRE(second.missing_frames == 128);
    MK_REQUIRE(second.starved);
    MK_REQUIRE(second.next_read_cursor_frame == 512);
    MK_REQUIRE(queue.buffered_frame_count() == 0);

    MK_REQUIRE(!queue.append(mirakana::AudioStreamingChunkDesc{
        mirakana::AssetId::from_name("audio/other"),
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        512,
        128,
    }));
    MK_REQUIRE(!queue.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{44100, 2, mirakana::AudioSampleFormat::float32},
        512,
        128,
    }));
    MK_REQUIRE(!queue.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        900,
        200,
    }));
    MK_REQUIRE(queue.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        512,
        512,
    }));
    const auto third = queue.consume(512);
    MK_REQUIRE(third.requested_frames == 512);
    MK_REQUIRE(third.consumed_frames == 512);
    MK_REQUIRE(third.missing_frames == 0);
    MK_REQUIRE(!third.starved);
    MK_REQUIRE(third.next_read_cursor_frame == 1024);
    MK_REQUIRE(queue.buffered_frame_count() == 0);
    MK_REQUIRE(!queue.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        1024,
        1,
    }));

    auto replay = make_queue();
    MK_REQUIRE(replay.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        0,
        256,
    }));
    const auto replay_first = replay.consume(128);
    MK_REQUIRE(replay.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        256,
        256,
    }));
    const auto replay_second = replay.consume(512);
    MK_REQUIRE(replay.append(mirakana::AudioStreamingChunkDesc{
        clip,
        mirakana::AudioDeviceFormat{48000, 2, mirakana::AudioSampleFormat::float32},
        512,
        512,
    }));
    const auto replay_third = replay.consume(512);
    MK_REQUIRE(same_consume(first, replay_first));
    MK_REQUIRE(same_consume(second, replay_second));
    MK_REQUIRE(same_consume(third, replay_third));
    MK_REQUIRE(replay.read_cursor_frame() == queue.read_cursor_frame());
    MK_REQUIRE(replay.buffered_end_frame() == queue.buffered_end_frame());
    MK_REQUIRE(replay.buffered_frame_count() == queue.buffered_frame_count());
}

MK_TEST("audio mixer render plan marks deterministic device format conversion") {
    mirakana::AudioMixer mixer;
    const auto clip = mirakana::AssetId::from_name("audio/mono-hit");

    mixer.add_clip(mirakana::AudioClipDesc{
        .clip = clip,
        .sample_rate = 44100,
        .channel_count = 1,
        .frame_count = 441,
        .sample_format = mirakana::AudioSampleFormat::pcm16,
        .streaming = false,
        .buffered_frame_count = 441,
    });
    const auto voice =
        mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});

    const auto plan = mixer.render_plan(mirakana::AudioRenderRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .frame_count = 960,
        .device_frame = 0,
        .underrun_warning_threshold_frames = 0,
    });

    MK_REQUIRE(plan.commands.size() == 1);
    MK_REQUIRE(plan.commands[0].voice == voice);
    MK_REQUIRE(plan.commands[0].output_frame_count == 480);
    MK_REQUIRE(plan.commands[0].source_frame_count == 441);
    MK_REQUIRE((plan.commands[0].conversion & mirakana::AudioFormatConversion::resample) ==
               mirakana::AudioFormatConversion::resample);
    MK_REQUIRE((plan.commands[0].conversion & mirakana::AudioFormatConversion::channel_convert) ==
               mirakana::AudioFormatConversion::channel_convert);
    MK_REQUIRE((plan.commands[0].conversion & mirakana::AudioFormatConversion::sample_format_convert) ==
               mirakana::AudioFormatConversion::sample_format_convert);
    MK_REQUIRE(plan.underruns.empty());
}

MK_TEST("audio mixer renders deterministic interleaved float samples") {
    mirakana::AudioMixer mixer;
    mixer.add_bus(mirakana::AudioBusDesc{.name = "sfx", .gain = 0.5F, .muted = false});
    const auto clip = mirakana::AssetId::from_name("audio/jump");
    mixer.add_clip(mirakana::AudioClipDesc{.clip = clip,
                                           .sample_rate = 48000,
                                           .channel_count = 1,
                                           .frame_count = 4,
                                           .sample_format = mirakana::AudioSampleFormat::float32,
                                           .streaming = false,
                                           .buffered_frame_count = 4});
    MK_REQUIRE(mixer.play(mirakana::AudioVoiceDesc{clip, "sfx", 0.8F, false}).value == 1);

    const std::vector<mirakana::AudioClipSampleData> samples{
        mirakana::AudioClipSampleData{
            .clip = clip,
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 4,
            .interleaved_float_samples = {1.0F, 0.5F, -0.5F, -1.0F},
        },
    };
    const auto output = mixer.render_interleaved_float(
        mirakana::AudioRenderRequest{
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 2,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 4,
            .device_frame = 0,
            .underrun_warning_threshold_frames = 0,
        },
        samples);

    MK_REQUIRE(output.plan.commands.size() == 1);
    MK_REQUIRE(output.format.channel_count == 2);
    MK_REQUIRE(output.frame_count == 4);
    MK_REQUIRE(output.interleaved_float_samples.size() == 8);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[0] - 0.4F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[1] - 0.4F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[2] - 0.2F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[3] - 0.2F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[4] + 0.2F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[5] + 0.2F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[6] + 0.4F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[7] + 0.4F) < 0.0001F);
}

MK_TEST("audio mixer renders linear resampled float samples") {
    mirakana::AudioMixer mixer;
    const auto clip = mirakana::AssetId::from_name("audio/linear-resample");
    mixer.add_clip(mirakana::AudioClipDesc{.clip = clip,
                                           .sample_rate = 24000,
                                           .channel_count = 1,
                                           .frame_count = 3,
                                           .sample_format = mirakana::AudioSampleFormat::float32,
                                           .streaming = false,
                                           .buffered_frame_count = 3});
    (void)mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});

    const std::vector<mirakana::AudioClipSampleData> samples{
        mirakana::AudioClipSampleData{
            .clip = clip,
            .format = mirakana::AudioDeviceFormat{.sample_rate = 24000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 3,
            .interleaved_float_samples = {0.0F, 1.0F, 0.0F},
        },
    };
    const auto output = mixer.render_interleaved_float(
        mirakana::AudioRenderRequest{
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 5,
            .device_frame = 0,
            .underrun_warning_threshold_frames = 0,
            .resampling_quality = mirakana::AudioResamplingQuality::linear,
        },
        samples);

    MK_REQUIRE(output.plan.commands.size() == 1);
    MK_REQUIRE(output.plan.commands[0].resampling_quality == mirakana::AudioResamplingQuality::linear);
    MK_REQUIRE(output.interleaved_float_samples.size() == 5);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[0] - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[1] - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[2] - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[3] - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[4] - 0.0F) < 0.0001F);
}

MK_TEST("audio mixer can request nearest resampling for deterministic low cost output") {
    mirakana::AudioMixer mixer;
    const auto clip = mirakana::AssetId::from_name("audio/nearest-resample");
    mixer.add_clip(mirakana::AudioClipDesc{.clip = clip,
                                           .sample_rate = 24000,
                                           .channel_count = 1,
                                           .frame_count = 3,
                                           .sample_format = mirakana::AudioSampleFormat::float32,
                                           .streaming = false,
                                           .buffered_frame_count = 3});
    (void)mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});

    const std::vector<mirakana::AudioClipSampleData> samples{
        mirakana::AudioClipSampleData{
            .clip = clip,
            .format = mirakana::AudioDeviceFormat{.sample_rate = 24000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 3,
            .interleaved_float_samples = {0.0F, 1.0F, 0.0F},
        },
    };
    const auto output = mixer.render_interleaved_float(
        mirakana::AudioRenderRequest{
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 5,
            .device_frame = 0,
            .underrun_warning_threshold_frames = 0,
            .resampling_quality = mirakana::AudioResamplingQuality::nearest,
        },
        samples);

    MK_REQUIRE(output.plan.commands.size() == 1);
    MK_REQUIRE(output.plan.commands[0].resampling_quality == mirakana::AudioResamplingQuality::nearest);
    MK_REQUIRE(output.interleaved_float_samples.size() == 5);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[0] - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[1] - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[2] - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[3] - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(output.interleaved_float_samples[4] - 0.0F) < 0.0001F);
}

MK_TEST("audio mixer clamps summed rendered float samples") {
    mirakana::AudioMixer mixer;
    const auto clip = mirakana::AssetId::from_name("audio/loud");
    mixer.add_clip(mirakana::AudioClipDesc{.clip = clip,
                                           .sample_rate = 48000,
                                           .channel_count = 1,
                                           .frame_count = 2,
                                           .sample_format = mirakana::AudioSampleFormat::float32,
                                           .streaming = false,
                                           .buffered_frame_count = 2});
    (void)mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});
    (void)mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});

    const std::vector<mirakana::AudioClipSampleData> samples{
        mirakana::AudioClipSampleData{
            .clip = clip,
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 2,
            .interleaved_float_samples = {0.75F, -0.75F},
        },
    };
    const auto output = mixer.render_interleaved_float(
        mirakana::AudioRenderRequest{
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 2,
            .device_frame = 0,
            .underrun_warning_threshold_frames = 0,
        },
        samples);

    MK_REQUIRE(output.interleaved_float_samples.size() == 2);
    MK_REQUIRE(output.interleaved_float_samples[0] == 1.0F);
    MK_REQUIRE(output.interleaved_float_samples[1] == -1.0F);
}

MK_TEST("audio device stream planner fills device queues in bounded chunks") {
    const auto plan = mirakana::plan_audio_device_stream(mirakana::AudioDeviceStreamRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .device_frame = 1000,
        .queued_frames = 128,
        .target_queued_frames = 512,
        .max_render_frames = 256,
        .underrun_warning_threshold_frames = 32,
        .resampling_quality = mirakana::AudioResamplingQuality::nearest,
    });

    MK_REQUIRE(plan.status == mirakana::AudioDeviceStreamStatus::ready);
    MK_REQUIRE(plan.diagnostic == mirakana::AudioDeviceStreamDiagnostic::none);
    MK_REQUIRE(plan.queued_frames_before == 128);
    MK_REQUIRE(plan.target_queued_frames == 512);
    MK_REQUIRE(plan.frames_to_render == 256);
    MK_REQUIRE(plan.queued_frames_after == 384);
    MK_REQUIRE(plan.render_request.format.sample_rate == 48000);
    MK_REQUIRE(plan.render_request.format.channel_count == 2);
    MK_REQUIRE(plan.render_request.frame_count == 256);
    MK_REQUIRE(plan.render_request.device_frame == 1128);
    MK_REQUIRE(plan.render_request.underrun_warning_threshold_frames == 32);
    MK_REQUIRE(plan.render_request.resampling_quality == mirakana::AudioResamplingQuality::nearest);
}

MK_TEST("audio device stream planner reports no work and invalid requests") {
    const auto no_work = mirakana::plan_audio_device_stream(mirakana::AudioDeviceStreamRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .device_frame = 4,
        .queued_frames = 512,
        .target_queued_frames = 256,
        .max_render_frames = 128,
    });
    MK_REQUIRE(no_work.status == mirakana::AudioDeviceStreamStatus::no_work);
    MK_REQUIRE(no_work.diagnostic == mirakana::AudioDeviceStreamDiagnostic::none);
    MK_REQUIRE(no_work.frames_to_render == 0);
    MK_REQUIRE(no_work.queued_frames_after == 512);

    const auto invalid_format = mirakana::plan_audio_device_stream(mirakana::AudioDeviceStreamRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 0,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .device_frame = 0,
        .queued_frames = 0,
        .target_queued_frames = 256,
        .max_render_frames = 128,
    });
    MK_REQUIRE(invalid_format.status == mirakana::AudioDeviceStreamStatus::invalid_request);
    MK_REQUIRE(invalid_format.diagnostic == mirakana::AudioDeviceStreamDiagnostic::invalid_format);

    const auto invalid_target = mirakana::plan_audio_device_stream(mirakana::AudioDeviceStreamRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .device_frame = 0,
        .queued_frames = 0,
        .target_queued_frames = 0,
        .max_render_frames = 128,
    });
    MK_REQUIRE(invalid_target.status == mirakana::AudioDeviceStreamStatus::invalid_request);
    MK_REQUIRE(invalid_target.diagnostic == mirakana::AudioDeviceStreamDiagnostic::invalid_queue_target);

    const auto invalid_budget = mirakana::plan_audio_device_stream(mirakana::AudioDeviceStreamRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .device_frame = 0,
        .queued_frames = 0,
        .target_queued_frames = 256,
        .max_render_frames = 0,
    });
    MK_REQUIRE(invalid_budget.status == mirakana::AudioDeviceStreamStatus::invalid_request);
    MK_REQUIRE(invalid_budget.diagnostic == mirakana::AudioDeviceStreamDiagnostic::invalid_render_budget);

    const auto overflow = mirakana::plan_audio_device_stream(mirakana::AudioDeviceStreamRequest{
        .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                              .channel_count = 2,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .device_frame = std::numeric_limits<std::uint64_t>::max() - 4,
        .queued_frames = 8,
        .target_queued_frames = 16,
        .max_render_frames = 8,
    });
    MK_REQUIRE(overflow.status == mirakana::AudioDeviceStreamStatus::invalid_request);
    MK_REQUIRE(overflow.diagnostic == mirakana::AudioDeviceStreamDiagnostic::device_frame_overflow);
}

MK_TEST("audio device stream render starts after already queued frames") {
    mirakana::AudioMixer mixer;
    const auto clip = mirakana::AssetId::from_name("audio/stream-pump");
    mixer.add_clip(mirakana::AudioClipDesc{.clip = clip,
                                           .sample_rate = 48000,
                                           .channel_count = 1,
                                           .frame_count = 4,
                                           .sample_format = mirakana::AudioSampleFormat::float32,
                                           .streaming = false,
                                           .buffered_frame_count = 4});
    (void)mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});

    const std::vector<mirakana::AudioClipSampleData> samples{
        mirakana::AudioClipSampleData{
            .clip = clip,
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 4,
            .interleaved_float_samples = {0.1F, 0.2F, 0.3F, 0.4F},
        },
    };

    const auto output = mirakana::render_audio_device_stream_interleaved_float(
        mixer,
        mirakana::AudioDeviceStreamRequest{
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .device_frame = 0,
            .queued_frames = 2,
            .target_queued_frames = 4,
            .max_render_frames = 2,
        },
        samples);

    MK_REQUIRE(output.plan.status == mirakana::AudioDeviceStreamStatus::ready);
    MK_REQUIRE(output.plan.render_request.device_frame == 2);
    MK_REQUIRE(output.buffer.frame_count == 2);
    MK_REQUIRE(output.buffer.plan.commands.size() == 1);
    MK_REQUIRE(output.buffer.plan.commands[0].source_start_frame == 2);
    MK_REQUIRE(output.buffer.interleaved_float_samples.size() == 2);
    MK_REQUIRE(std::abs(output.buffer.interleaved_float_samples[0] - 0.3F) < 0.0001F);
    MK_REQUIRE(std::abs(output.buffer.interleaved_float_samples[1] - 0.4F) < 0.0001F);
}

MK_TEST("animation keyframes sample with clamped linear interpolation") {
    const std::vector<mirakana::Vec3Keyframe> keys{
        mirakana::Vec3Keyframe{.time_seconds = 0.0F, .value = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
        mirakana::Vec3Keyframe{.time_seconds = 1.0F, .value = mirakana::Vec3{.x = 10.0F, .y = 20.0F, .z = 30.0F}},
    };

    MK_REQUIRE(mirakana::sample_vec3_keyframes(keys, -1.0F) == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(mirakana::sample_vec3_keyframes(keys, 2.0F) == (mirakana::Vec3{10.0F, 20.0F, 30.0F}));
    MK_REQUIRE(mirakana::sample_vec3_keyframes(keys, 0.25F) == (mirakana::Vec3{2.5F, 5.0F, 7.5F}));
}

MK_TEST("animation quaternion keyframes sample with normalized shortest path interpolation") {
    const auto near_vec = [](mirakana::Vec3 lhs, mirakana::Vec3 rhs) { return mirakana::length(lhs - rhs) < 0.001F; };
    const auto z_quarter_turn =
        mirakana::Quat::from_axis_angle(mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}, 1.57079637F);
    const mirakana::Quat negated_z_quarter_turn{
        .x = -z_quarter_turn.x, .y = -z_quarter_turn.y, .z = -z_quarter_turn.z, .w = -z_quarter_turn.w};
    const std::vector<mirakana::QuatKeyframe> keys{
        mirakana::QuatKeyframe{.time_seconds = 0.0F, .value = mirakana::Quat::identity()},
        mirakana::QuatKeyframe{.time_seconds = 1.0F, .value = negated_z_quarter_turn},
    };

    MK_REQUIRE(mirakana::is_valid_quat_keyframes(keys));
    MK_REQUIRE(mirakana::is_valid_quat_animation_track(mirakana::QuatAnimationTrack{"joint/root/rotation", keys}));
    MK_REQUIRE(
        near_vec(mirakana::rotate(mirakana::sample_quat_keyframes(keys, -1.0F), mirakana::Vec3{1.0F, 0.0F, 0.0F}),
                 mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(near_vec(mirakana::rotate(mirakana::sample_quat_keyframes(keys, 0.5F), mirakana::Vec3{1.0F, 0.0F, 0.0F}),
                        mirakana::Vec3{0.7071F, 0.7071F, 0.0F}));
    MK_REQUIRE(mirakana::is_normalized_quat(mirakana::sample_quat_keyframes(keys, 0.5F)));

    MK_REQUIRE(!mirakana::is_valid_quat_keyframes({}));
    MK_REQUIRE(
        !mirakana::is_valid_quat_keyframes({mirakana::QuatKeyframe{0.0F, mirakana::Quat{0.0F, 0.0F, 0.0F, 2.0F}}}));
    MK_REQUIRE(!mirakana::is_valid_quat_keyframes(
        {mirakana::QuatKeyframe{1.0F, mirakana::Quat::identity()}, mirakana::QuatKeyframe{1.0F, z_quarter_turn}}));

    bool threw = false;
    try {
        (void)mirakana::sample_quat_keyframes(
            {mirakana::QuatKeyframe{
                .time_seconds = 0.0F,
                .value =
                    mirakana::Quat{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .z = 0.0F, .w = 1.0F}}},
            0.0F);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    MK_REQUIRE(threw);
}

MK_TEST("animation float curve samples apply to bound 3d transforms") {
    std::vector<mirakana::Transform3D> transforms(2);
    transforms[1].position = mirakana::Vec3{.x = -1.0F, .y = -2.0F, .z = -3.0F};
    transforms[1].rotation_radians = mirakana::Vec3{.x = 0.25F, .y = 0.5F, .z = 0.75F};
    transforms[1].scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F};

    const std::vector<mirakana::FloatAnimationCurveSample> samples{
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/1/translation/x", .value = 1.0F},
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/1/translation/y", .value = 2.0F},
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/1/translation/z", .value = 3.0F},
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/1/rotation_z", .value = 0.7853982F},
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/1/scale/x", .value = 1.5F},
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/1/scale/y", .value = 2.0F},
        mirakana::FloatAnimationCurveSample{.target = "gltf/node/1/scale/z", .value = 2.5F},
    };
    const std::vector<mirakana::AnimationTransformCurveBinding> bindings{
        mirakana::AnimationTransformCurveBinding{.target = "gltf/node/1/translation/x",
                                                 .transform_index = 1,
                                                 .component = mirakana::AnimationTransformComponent::translation_x},
        mirakana::AnimationTransformCurveBinding{.target = "gltf/node/1/translation/y",
                                                 .transform_index = 1,
                                                 .component = mirakana::AnimationTransformComponent::translation_y},
        mirakana::AnimationTransformCurveBinding{.target = "gltf/node/1/translation/z",
                                                 .transform_index = 1,
                                                 .component = mirakana::AnimationTransformComponent::translation_z},
        mirakana::AnimationTransformCurveBinding{.target = "gltf/node/1/rotation_z",
                                                 .transform_index = 1,
                                                 .component = mirakana::AnimationTransformComponent::rotation_z},
        mirakana::AnimationTransformCurveBinding{.target = "gltf/node/1/scale/x",
                                                 .transform_index = 1,
                                                 .component = mirakana::AnimationTransformComponent::scale_x},
        mirakana::AnimationTransformCurveBinding{.target = "gltf/node/1/scale/y",
                                                 .transform_index = 1,
                                                 .component = mirakana::AnimationTransformComponent::scale_y},
        mirakana::AnimationTransformCurveBinding{.target = "gltf/node/1/scale/z",
                                                 .transform_index = 1,
                                                 .component = mirakana::AnimationTransformComponent::scale_z},
    };

    const auto result = mirakana::apply_float_animation_samples_to_transform3d(samples, bindings, transforms);

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.diagnostic.empty());
    MK_REQUIRE(result.applied_sample_count == 7);
    MK_REQUIRE(transforms[0].position == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(transforms[1].position == (mirakana::Vec3{1.0F, 2.0F, 3.0F}));
    MK_REQUIRE(std::abs(transforms[1].rotation_radians.x - 0.25F) < 0.0001F);
    MK_REQUIRE(std::abs(transforms[1].rotation_radians.y - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(transforms[1].rotation_radians.z - 0.7853982F) < 0.0001F);
    MK_REQUIRE(transforms[1].scale == (mirakana::Vec3{1.5F, 2.0F, 2.5F}));
}

MK_TEST("animation float curve transform application rejects invalid inputs deterministically") {
    std::vector<mirakana::Transform3D> transforms(1);
    const std::vector<mirakana::AnimationTransformCurveBinding> bindings{
        mirakana::AnimationTransformCurveBinding{.target = "node/scale/x",
                                                 .transform_index = 0,
                                                 .component = mirakana::AnimationTransformComponent::scale_x},
    };

    const std::vector<mirakana::FloatAnimationCurveSample> bad_scale_samples{
        mirakana::FloatAnimationCurveSample{.target = "node/scale/x", .value = 0.0F},
    };
    const auto bad_scale =
        mirakana::apply_float_animation_samples_to_transform3d(bad_scale_samples, bindings, transforms);
    MK_REQUIRE(!bad_scale.succeeded);
    MK_REQUIRE(bad_scale.diagnostic.contains("scale"));

    const std::vector<mirakana::FloatAnimationCurveSample> duplicate_samples{
        mirakana::FloatAnimationCurveSample{.target = "node/scale/x", .value = 1.0F},
        mirakana::FloatAnimationCurveSample{.target = "node/scale/x", .value = 2.0F},
    };
    const auto duplicate_sample =
        mirakana::apply_float_animation_samples_to_transform3d(duplicate_samples, bindings, transforms);
    MK_REQUIRE(!duplicate_sample.succeeded);
    MK_REQUIRE(duplicate_sample.diagnostic.contains("duplicate sample target"));

    const std::vector<mirakana::AnimationTransformCurveBinding> out_of_range_binding{
        mirakana::AnimationTransformCurveBinding{.target = "node/translation/x",
                                                 .transform_index = 1,
                                                 .component = mirakana::AnimationTransformComponent::translation_x},
    };
    const std::vector<mirakana::FloatAnimationCurveSample> translation_samples{
        mirakana::FloatAnimationCurveSample{.target = "node/translation/x", .value = 1.0F},
    };
    const auto out_of_range =
        mirakana::apply_float_animation_samples_to_transform3d(translation_samples, out_of_range_binding, transforms);
    MK_REQUIRE(!out_of_range.succeeded);
    MK_REQUIRE(out_of_range.diagnostic.contains("transform index"));

    const std::vector<mirakana::AnimationTransformCurveBinding> missing_sample_binding{
        mirakana::AnimationTransformCurveBinding{.target = "node/missing",
                                                 .transform_index = 0,
                                                 .component = mirakana::AnimationTransformComponent::translation_x},
    };
    const auto missing_sample =
        mirakana::apply_float_animation_samples_to_transform3d(translation_samples, missing_sample_binding, transforms);
    MK_REQUIRE(!missing_sample.succeeded);
    MK_REQUIRE(missing_sample.diagnostic.contains("missing sample"));
}

MK_TEST("animation skeleton validates hierarchy names and rest pose") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "hips",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "spine",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.5F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "head",
                .parent_index = 1,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.75F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };

    MK_REQUIRE(mirakana::is_valid_animation_skeleton_desc(skeleton));
    MK_REQUIRE(mirakana::validate_animation_skeleton_desc(skeleton).empty());

    const auto rest_pose = mirakana::make_animation_rest_pose(skeleton);
    MK_REQUIRE(rest_pose.joints.size() == 3);
    MK_REQUIRE(rest_pose.joints[0].translation == (mirakana::Vec3{0.0F, 1.0F, 0.0F}));
    MK_REQUIRE(rest_pose.joints[2].translation == (mirakana::Vec3{0.0F, 0.75F, 0.0F}));

    auto invalid = skeleton;
    invalid.joints[1].parent_index = 2;
    auto diagnostics = mirakana::validate_animation_skeleton_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::parent_not_before_child);

    invalid = skeleton;
    invalid.joints[2].name = "spine";
    diagnostics = mirakana::validate_animation_skeleton_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::duplicate_joint_name);

    invalid = skeleton;
    invalid.joints[0].rest.scale = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 1.0F};
    diagnostics = mirakana::validate_animation_skeleton_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_rest_transform);

    diagnostics = mirakana::validate_animation_skeleton_desc(mirakana::AnimationSkeletonDesc{});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::empty_skeleton);

    invalid = skeleton;
    invalid.joints[1].name = "bad=joint";
    diagnostics = mirakana::validate_animation_skeleton_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_joint_name);

    invalid = skeleton;
    invalid.joints[1].name.clear();
    diagnostics = mirakana::validate_animation_skeleton_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_joint_name);

    invalid = skeleton;
    invalid.joints[1].parent_index = 99;
    diagnostics = mirakana::validate_animation_skeleton_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_parent_index);

    bool rejected_rest_pose = false;
    try {
        (void)mirakana::make_animation_rest_pose(invalid);
    } catch (const std::invalid_argument&) {
        rejected_rest_pose = true;
    }
    MK_REQUIRE(rejected_rest_pose);
}

MK_TEST("animation 3D skeleton validates quaternion rest pose and builds model pose") {
    const auto near = [](float lhs, float rhs) { return std::abs(lhs - rhs) < 0.0001F; };
    const auto near_vec = [&](mirakana::Vec3 lhs, mirakana::Vec3 rhs) {
        return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
    };

    const mirakana::AnimationSkeleton3dDesc skeleton{
        {
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{
                        .translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                        .rotation = mirakana::Quat::from_axis_angle(mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                                    1.57079637F),
                        .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
                    },
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "child",
                .parent_index = 0,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{
                        .translation = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                        .rotation = mirakana::Quat::identity(),
                        .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
                    },
            },
        },
    };

    MK_REQUIRE(mirakana::is_valid_animation_skeleton_3d_desc(skeleton));
    MK_REQUIRE(mirakana::validate_animation_skeleton_3d_desc(skeleton).empty());

    const auto rest_pose = mirakana::make_animation_rest_pose_3d(skeleton);
    MK_REQUIRE(rest_pose.joints.size() == 2);
    MK_REQUIRE(mirakana::validate_animation_pose_3d(skeleton, rest_pose).empty());

    const auto model_pose = mirakana::build_animation_model_pose_3d(skeleton, rest_pose);
    MK_REQUIRE(model_pose.joint_matrices.size() == 2);
    MK_REQUIRE(near_vec(mirakana::transform_direction(model_pose.joint_matrices[0], mirakana::Vec3{0.0F, 1.0F, 0.0F}),
                        mirakana::Vec3{0.0F, 0.0F, 1.0F}));
    MK_REQUIRE(near_vec(mirakana::transform_point(model_pose.joint_matrices[1], mirakana::Vec3{0.0F, 0.0F, 0.0F}),
                        mirakana::Vec3{0.0F, 0.0F, 1.0F}));
}

MK_TEST("animation 3D skeleton rejects invalid quaternion pose inputs") {
    mirakana::AnimationSkeleton3dDesc skeleton{
        {
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "child",
                .parent_index = 0,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };

    auto invalid = skeleton;
    invalid.joints[0].rest.rotation = mirakana::Quat{.x = 0.0F, .y = 0.0F, .z = 0.0F, .w = 2.0F};
    auto diagnostics = mirakana::validate_animation_skeleton_3d_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_rest_transform);
    MK_REQUIRE(diagnostics[0].message.contains("rotation"));

    invalid = skeleton;
    invalid.joints[1].parent_index = 1;
    diagnostics = mirakana::validate_animation_skeleton_3d_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::parent_not_before_child);

    auto pose = mirakana::make_animation_rest_pose_3d(skeleton);
    pose.joints[1].rotation =
        mirakana::Quat{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .z = 0.0F, .w = 1.0F};
    diagnostics = mirakana::validate_animation_pose_3d(skeleton, pose);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_pose_transform);

    pose = mirakana::make_animation_rest_pose_3d(skeleton);
    pose.joints[0].scale = mirakana::Vec3{.x = 1.0F, .y = -1.0F, .z = 1.0F};
    diagnostics = mirakana::validate_animation_pose_3d(skeleton, pose);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_pose_transform);
}

MK_TEST("animation 3D local pose samples quaternion joint tracks") {
    const auto near = [](float lhs, float rhs) { return std::abs(lhs - rhs) < 0.001F; };
    const auto near_vec = [&](mirakana::Vec3 lhs, mirakana::Vec3 rhs) {
        return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
    };

    const mirakana::AnimationSkeleton3dDesc skeleton{
        {
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "child",
                .parent_index = 0,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const std::vector<mirakana::AnimationJointTrack3dDesc> tracks{
        mirakana::AnimationJointTrack3dDesc{
            .joint_index = 0,
            .translation_keyframes = {},
            .rotation_keyframes = {mirakana::QuatKeyframe{.time_seconds = 0.0F, .value = mirakana::Quat::identity()},
                                   mirakana::QuatKeyframe{
                                       .time_seconds = 1.0F,
                                       .value = mirakana::Quat::from_axis_angle(
                                           mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}, 1.57079637F)}},
            .scale_keyframes = {},
        },
        mirakana::AnimationJointTrack3dDesc{
            .joint_index = 1,
            .translation_keyframes = {mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                                             .value = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
                                      mirakana::Vec3Keyframe{.time_seconds = 1.0F,
                                                             .value = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}}},
            .rotation_keyframes = {},
            .scale_keyframes = {mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                                       .value = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                                mirakana::Vec3Keyframe{.time_seconds = 1.0F,
                                                       .value = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F}}},
        },
    };

    MK_REQUIRE(mirakana::is_valid_animation_joint_tracks_3d(skeleton, tracks));
    const auto pose = mirakana::sample_animation_local_pose_3d(skeleton, tracks, 0.5F);
    MK_REQUIRE(pose.joints.size() == 2);
    MK_REQUIRE(near_vec(mirakana::rotate(pose.joints[0].rotation, mirakana::Vec3{1.0F, 0.0F, 0.0F}),
                        mirakana::Vec3{0.7071F, 0.7071F, 0.0F}));
    MK_REQUIRE(mirakana::is_normalized_quat(pose.joints[0].rotation));
    MK_REQUIRE(near_vec(pose.joints[1].translation, mirakana::Vec3{0.5F, 0.5F, 0.0F}));
    MK_REQUIRE(near_vec(pose.joints[1].scale, mirakana::Vec3{1.5F, 1.5F, 1.5F}));

    const auto rest_from_empty_tracks = mirakana::sample_animation_local_pose_3d(skeleton, {}, 0.5F);
    MK_REQUIRE(near_vec(rest_from_empty_tracks.joints[1].translation, mirakana::Vec3{1.0F, 0.0F, 0.0F}));

    const auto model_pose = mirakana::build_animation_model_pose_3d(skeleton, pose);
    MK_REQUIRE(near_vec(mirakana::transform_point(model_pose.joint_matrices[1], mirakana::Vec3{0.0F, 0.0F, 0.0F}),
                        mirakana::Vec3{0.0F, 0.7071F, 0.0F}));
}

MK_TEST("animation 3D local pose rejects invalid quaternion joint tracks") {
    const mirakana::AnimationSkeleton3dDesc skeleton{
        {
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const std::vector<mirakana::AnimationJointTrack3dDesc> valid_tracks{
        mirakana::AnimationJointTrack3dDesc{
            .joint_index = 0,
            .translation_keyframes = {},
            .rotation_keyframes = {mirakana::QuatKeyframe{.time_seconds = 0.0F, .value = mirakana::Quat::identity()},
                                   mirakana::QuatKeyframe{
                                       .time_seconds = 1.0F,
                                       .value = mirakana::Quat::from_axis_angle(
                                           mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}, 1.57079637F)}},
            .scale_keyframes = {},
        },
    };

    MK_REQUIRE(mirakana::validate_animation_joint_tracks_3d(skeleton, valid_tracks).empty());

    auto invalid_tracks = valid_tracks;
    invalid_tracks.push_back(valid_tracks.front());
    MK_REQUIRE(!mirakana::is_valid_animation_joint_tracks_3d(skeleton, invalid_tracks));
    auto diagnostics = mirakana::validate_animation_joint_tracks_3d(skeleton, invalid_tracks);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::duplicate_track_binding);

    invalid_tracks = valid_tracks;
    invalid_tracks[0].joint_index = 1;
    MK_REQUIRE(!mirakana::is_valid_animation_joint_tracks_3d(skeleton, invalid_tracks));

    invalid_tracks = valid_tracks;
    invalid_tracks[0].rotation_keyframes[0].value = mirakana::Quat{.x = 0.0F, .y = 0.0F, .z = 0.0F, .w = 2.0F};
    MK_REQUIRE(!mirakana::is_valid_animation_joint_tracks_3d(skeleton, invalid_tracks));

    invalid_tracks = valid_tracks;
    invalid_tracks[0].scale_keyframes = {
        mirakana::Vec3Keyframe{.time_seconds = 0.0F, .value = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 1.0F}}};
    MK_REQUIRE(!mirakana::is_valid_animation_joint_tracks_3d(skeleton, invalid_tracks));

    bool threw = false;
    try {
        (void)mirakana::sample_animation_local_pose_3d(skeleton, valid_tracks, std::numeric_limits<float>::quiet_NaN());
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    MK_REQUIRE(threw);
}

MK_TEST("animation 3D local rotation limits validate and clamp swing twist") {
    const auto near = [](float lhs, float rhs) { return std::abs(lhs - rhs) < 0.001F; };
    const auto near_vec = [&](mirakana::Vec3 lhs, mirakana::Vec3 rhs) {
        return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
    };

    const mirakana::AnimationSkeleton3dDesc skeleton{
        {
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const std::vector<mirakana::AnimationIkLocalRotationLimit3d> limits{
        mirakana::AnimationIkLocalRotationLimit3d{
            .joint_index = 0U,
            .twist_axis = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
            .max_swing_radians = 0.25F,
            .min_twist_radians = -0.5F,
            .max_twist_radians = 0.5F,
        },
    };

    MK_REQUIRE(mirakana::validate_animation_ik_local_rotation_limits_3d(skeleton, limits).empty());
    MK_REQUIRE(mirakana::is_valid_animation_ik_local_rotation_limits_3d(skeleton, limits));

    auto invalid = limits;
    invalid[0].twist_axis = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    auto diagnostics = mirakana::validate_animation_ik_local_rotation_limits_3d(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_twist_axis);

    invalid = limits;
    invalid[0].joint_index = 1U;
    diagnostics = mirakana::validate_animation_ik_local_rotation_limits_3d(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_joint_index);

    invalid = limits;
    invalid.push_back(limits.front());
    diagnostics = mirakana::validate_animation_ik_local_rotation_limits_3d(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationIkLocalRotationLimit3dDiagnosticCode::duplicate_joint);

    invalid = limits;
    invalid[0].max_swing_radians = -0.01F;
    diagnostics = mirakana::validate_animation_ik_local_rotation_limits_3d(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_swing_limit);

    invalid = limits;
    invalid[0].min_twist_radians = 0.75F;
    invalid[0].max_twist_radians = -0.75F;
    diagnostics = mirakana::validate_animation_ik_local_rotation_limits_3d(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationIkLocalRotationLimit3dDiagnosticCode::invalid_twist_range);

    auto pose = mirakana::make_animation_rest_pose_3d(skeleton);
    pose.joints[0].rotation = mirakana::Quat::from_axis_angle(mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}, 1.0F);
    auto result = mirakana::apply_animation_local_rotation_limits_3d(skeleton, limits, pose);
    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.constrained);
    MK_REQUIRE(result.constrained_joint_count == 1U);
    MK_REQUIRE(result.diagnostic.empty());
    MK_REQUIRE(mirakana::is_normalized_quat(pose.joints[0].rotation));
    MK_REQUIRE(near_vec(mirakana::rotate(pose.joints[0].rotation, mirakana::Vec3{0.0F, 1.0F, 0.0F}),
                        mirakana::Vec3{0.0F, std::cos(0.5F), std::sin(0.5F)}));

    pose = mirakana::make_animation_rest_pose_3d(skeleton);
    pose.joints[0].rotation = mirakana::Quat::from_axis_angle(mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}, 1.0F);
    result = mirakana::apply_animation_local_rotation_limits_3d(skeleton, limits, pose);
    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.constrained);
    MK_REQUIRE(result.constrained_joint_count == 1U);
    MK_REQUIRE(mirakana::is_normalized_quat(pose.joints[0].rotation));
    MK_REQUIRE(near_vec(mirakana::rotate(pose.joints[0].rotation, mirakana::Vec3{1.0F, 0.0F, 0.0F}),
                        mirakana::Vec3{std::cos(0.25F), std::sin(0.25F), 0.0F}));
}

MK_TEST("animation skeleton samples deterministic local poses from joint tracks") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "hips",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand_l",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = -0.5F, .y = 0.5F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };

    const std::vector<mirakana::AnimationJointTrackDesc> tracks{
        mirakana::AnimationJointTrackDesc{
            .joint_index = 1,
            .translation_keyframes =
                {
                    mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                           .value = mirakana::Vec3{.x = -0.5F, .y = 0.5F, .z = 0.0F}},
                    mirakana::Vec3Keyframe{.time_seconds = 1.0F,
                                           .value = mirakana::Vec3{.x = -1.0F, .y = 0.75F, .z = 0.0F}},
                },
            .rotation_z_keyframes =
                {
                    mirakana::FloatKeyframe{.time_seconds = 0.0F, .value = 0.0F},
                    mirakana::FloatKeyframe{.time_seconds = 1.0F, .value = 1.0F},
                },
            .scale_keyframes =
                {
                    mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                           .value = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
                    mirakana::Vec3Keyframe{.time_seconds = 1.0F,
                                           .value = mirakana::Vec3{.x = 2.0F, .y = 2.0F, .z = 2.0F}},
                },
        },
    };

    MK_REQUIRE(mirakana::is_valid_animation_joint_tracks(skeleton, tracks));

    const auto pose = mirakana::sample_animation_local_pose(skeleton, tracks, 0.25F);
    MK_REQUIRE(pose.joints.size() == 2);
    MK_REQUIRE(pose.joints[0].translation == (mirakana::Vec3{0.0F, 1.0F, 0.0F}));
    MK_REQUIRE(pose.joints[1].translation == (mirakana::Vec3{-0.625F, 0.5625F, 0.0F}));
    MK_REQUIRE(std::abs(pose.joints[1].rotation_z_radians - 0.25F) < 0.0001F);
    MK_REQUIRE(pose.joints[1].scale == (mirakana::Vec3{1.25F, 1.25F, 1.25F}));

    const auto clamped = mirakana::sample_animation_local_pose(skeleton, tracks, 5.0F);
    MK_REQUIRE(clamped.joints[1].translation == (mirakana::Vec3{-1.0F, 0.75F, 0.0F}));
    MK_REQUIRE(clamped.joints[1].scale == (mirakana::Vec3{2.0F, 2.0F, 2.0F}));

    const auto rest_from_empty_tracks = mirakana::sample_animation_local_pose(skeleton, {}, 0.25F);
    MK_REQUIRE(rest_from_empty_tracks.joints[1].translation == (mirakana::Vec3{-0.5F, 0.5F, 0.0F}));
    MK_REQUIRE(rest_from_empty_tracks.joints[1].scale == (mirakana::Vec3{1.0F, 1.0F, 1.0F}));

    const std::vector<mirakana::AnimationJointTrackDesc> out_of_order_tracks{
        mirakana::AnimationJointTrackDesc{
            .joint_index = 1,
            .translation_keyframes = {},
            .rotation_z_keyframes = {mirakana::FloatKeyframe{.time_seconds = 0.0F, .value = 0.0F},
                                     mirakana::FloatKeyframe{.time_seconds = 1.0F, .value = 1.0F}},
            .scale_keyframes = {},
        },
        mirakana::AnimationJointTrackDesc{
            .joint_index = 0,
            .translation_keyframes = {mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                                             .value = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}},
                                      mirakana::Vec3Keyframe{.time_seconds = 1.0F,
                                                             .value = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F}}},
            .rotation_z_keyframes = {},
            .scale_keyframes = {},
        },
    };
    const auto out_of_order_pose = mirakana::sample_animation_local_pose(skeleton, out_of_order_tracks, 0.5F);
    MK_REQUIRE(out_of_order_pose.joints[0].translation == (mirakana::Vec3{0.0F, 1.5F, 0.0F}));
    MK_REQUIRE(std::abs(out_of_order_pose.joints[1].rotation_z_radians - 0.5F) < 0.0001F);

    auto invalid_tracks = tracks;
    invalid_tracks.push_back(tracks.front());
    const auto diagnostics = mirakana::validate_animation_joint_tracks(skeleton, invalid_tracks);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::duplicate_track_binding);

    invalid_tracks = tracks;
    invalid_tracks[0].joint_index = 9;
    const auto out_of_range_track = mirakana::validate_animation_joint_tracks(skeleton, invalid_tracks);
    MK_REQUIRE(!out_of_range_track.empty());
    MK_REQUIRE(out_of_range_track[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_track_binding);

    invalid_tracks = tracks;
    invalid_tracks[0].scale_keyframes[1].value = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 1.0F};
    const auto invalid_scale_track = mirakana::validate_animation_joint_tracks(skeleton, invalid_tracks);
    MK_REQUIRE(!invalid_scale_track.empty());
    MK_REQUIRE(invalid_scale_track[0].code == mirakana::AnimationSkeletonDiagnosticCode::invalid_track_binding);

    bool rejected_tracks = false;
    try {
        (void)mirakana::sample_animation_local_pose(skeleton, invalid_tracks, 0.25F);
    } catch (const std::invalid_argument&) {
        rejected_tracks = true;
    }
    MK_REQUIRE(rejected_tracks);

    mirakana::AnimationPose wrong_pose;
    wrong_pose.joints.push_back(mirakana::JointLocalTransform{});
    const auto pose_diagnostics = mirakana::validate_animation_pose(skeleton, wrong_pose);
    MK_REQUIRE(!pose_diagnostics.empty());
    MK_REQUIRE(pose_diagnostics[0].code == mirakana::AnimationSkeletonDiagnosticCode::pose_joint_count_mismatch);

    bool rejected_time = false;
    try {
        (void)mirakana::sample_animation_local_pose(skeleton, tracks, std::numeric_limits<float>::quiet_NaN());
    } catch (const std::invalid_argument&) {
        rejected_time = true;
    }
    MK_REQUIRE(rejected_time);
}

MK_TEST("animation root motion samples translation and z rotation deltas") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.25F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const std::vector<mirakana::AnimationJointTrackDesc> tracks{
        mirakana::AnimationJointTrackDesc{
            .joint_index = 0,
            .translation_keyframes =
                {
                    mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                           .value = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
                    mirakana::Vec3Keyframe{.time_seconds = 1.0F,
                                           .value = mirakana::Vec3{.x = 4.0F, .y = 2.0F, .z = 0.0F}},
                },
            .rotation_z_keyframes =
                {
                    mirakana::FloatKeyframe{.time_seconds = 0.0F, .value = 0.5F},
                    mirakana::FloatKeyframe{.time_seconds = 1.0F, .value = 1.5F},
                },
            .scale_keyframes = {},
        },
    };

    const auto sample = mirakana::sample_animation_root_motion(
        skeleton, tracks,
        mirakana::AnimationRootMotionSampleDesc{
            .root_joint_index = 0, .from_time_seconds = 0.25F, .to_time_seconds = 0.75F});
    MK_REQUIRE(sample.start_translation == (mirakana::Vec3{1.0F, 0.5F, 0.0F}));
    MK_REQUIRE(sample.end_translation == (mirakana::Vec3{3.0F, 1.5F, 0.0F}));
    MK_REQUIRE(sample.delta_translation == (mirakana::Vec3{2.0F, 1.0F, 0.0F}));
    MK_REQUIRE(std::abs(sample.start_rotation_z_radians - 0.75F) < 0.0001F);
    MK_REQUIRE(std::abs(sample.end_rotation_z_radians - 1.25F) < 0.0001F);
    MK_REQUIRE(std::abs(sample.delta_rotation_z_radians - 0.5F) < 0.0001F);

    const auto replay = mirakana::sample_animation_root_motion(
        skeleton, tracks,
        mirakana::AnimationRootMotionSampleDesc{
            .root_joint_index = 0, .from_time_seconds = 0.25F, .to_time_seconds = 0.75F});
    MK_REQUIRE(replay.delta_translation == sample.delta_translation);
    MK_REQUIRE(std::abs(replay.delta_rotation_z_radians - sample.delta_rotation_z_radians) < 0.0001F);

    const auto clamped = mirakana::sample_animation_root_motion(
        skeleton, tracks,
        mirakana::AnimationRootMotionSampleDesc{
            .root_joint_index = 0, .from_time_seconds = -1.0F, .to_time_seconds = 5.0F});
    MK_REQUIRE(clamped.start_translation == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(clamped.end_translation == (mirakana::Vec3{4.0F, 2.0F, 0.0F}));
    MK_REQUIRE(clamped.delta_translation == (mirakana::Vec3{4.0F, 2.0F, 0.0F}));
    MK_REQUIRE(std::abs(clamped.start_rotation_z_radians - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(clamped.end_rotation_z_radians - 1.5F) < 0.0001F);
    MK_REQUIRE(std::abs(clamped.delta_rotation_z_radians - 1.0F) < 0.0001F);

    const auto rest_rotation = mirakana::sample_animation_root_motion(
        skeleton, {},
        mirakana::AnimationRootMotionSampleDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 1.0F});
    MK_REQUIRE(rest_rotation.start_translation == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(rest_rotation.end_translation == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(rest_rotation.delta_translation == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(rest_rotation.start_rotation_z_radians - 0.25F) < 0.0001F);
    MK_REQUIRE(std::abs(rest_rotation.end_rotation_z_radians - 0.25F) < 0.0001F);
    MK_REQUIRE(std::abs(rest_rotation.delta_rotation_z_radians) < 0.0001F);
}

MK_TEST("animation root motion reports deterministic diagnostics") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const std::vector<mirakana::AnimationJointTrackDesc> tracks{
        mirakana::AnimationJointTrackDesc{
            .joint_index = 0,
            .translation_keyframes = {mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                                             .value = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
                                      mirakana::Vec3Keyframe{.time_seconds = 1.0F,
                                                             .value = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}}},
            .rotation_z_keyframes = {mirakana::FloatKeyframe{.time_seconds = 0.0F, .value = 0.0F},
                                     mirakana::FloatKeyframe{.time_seconds = 1.0F, .value = 1.0F}},
            .scale_keyframes = {},
        },
    };

    MK_REQUIRE(mirakana::is_valid_animation_root_motion_sample(
        skeleton, tracks,
        mirakana::AnimationRootMotionSampleDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 1.0F}));
    MK_REQUIRE(mirakana::validate_animation_root_motion_sample(
                   skeleton, tracks,
                   mirakana::AnimationRootMotionSampleDesc{
                       .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 1.0F})
                   .empty());

    auto diagnostics = mirakana::validate_animation_root_motion_sample(
        skeleton, tracks,
        mirakana::AnimationRootMotionSampleDesc{
            .root_joint_index = 9, .from_time_seconds = 0.0F, .to_time_seconds = 1.0F});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationRootMotionDiagnosticCode::invalid_root_joint_index);
    MK_REQUIRE(!mirakana::is_valid_animation_root_motion_sample(
        skeleton, tracks,
        mirakana::AnimationRootMotionSampleDesc{
            .root_joint_index = 9, .from_time_seconds = 0.0F, .to_time_seconds = 1.0F}));

    diagnostics = mirakana::validate_animation_root_motion_sample(
        skeleton, tracks,
        mirakana::AnimationRootMotionSampleDesc{.root_joint_index = 0,
                                                .from_time_seconds = std::numeric_limits<float>::quiet_NaN(),
                                                .to_time_seconds = 1.0F});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationRootMotionDiagnosticCode::invalid_time);

    auto invalid_tracks = tracks;
    invalid_tracks[0].rotation_z_keyframes[1].value = std::numeric_limits<float>::quiet_NaN();
    diagnostics = mirakana::validate_animation_root_motion_sample(
        skeleton, invalid_tracks,
        mirakana::AnimationRootMotionSampleDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 1.0F});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationRootMotionDiagnosticCode::invalid_joint_tracks);

    bool rejected = false;
    try {
        (void)mirakana::sample_animation_root_motion(skeleton, invalid_tracks,
                                                     mirakana::AnimationRootMotionSampleDesc{.root_joint_index = 0,
                                                                                             .from_time_seconds = 0.0F,
                                                                                             .to_time_seconds = 1.0F});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("animation root motion accumulates forward loops deterministically") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const std::vector<mirakana::AnimationJointTrackDesc> tracks{
        mirakana::AnimationJointTrackDesc{
            .joint_index = 0,
            .translation_keyframes =
                {
                    mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                           .value = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
                    mirakana::Vec3Keyframe{.time_seconds = 2.0F,
                                           .value = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F}},
                },
            .rotation_z_keyframes =
                {
                    mirakana::FloatKeyframe{.time_seconds = 0.0F, .value = 0.0F},
                    mirakana::FloatKeyframe{.time_seconds = 2.0F, .value = 1.0F},
                },
            .scale_keyframes = {},
        },
    };

    const auto accumulated = mirakana::accumulate_animation_root_motion(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 0.5F, .to_time_seconds = 4.5F, .clip_duration_seconds = 2.0F});
    MK_REQUIRE(accumulated.delta_translation == (mirakana::Vec3{4.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(accumulated.delta_rotation_z_radians - 2.0F) < 0.0001F);

    const auto replay = mirakana::accumulate_animation_root_motion(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 0.5F, .to_time_seconds = 4.5F, .clip_duration_seconds = 2.0F});
    MK_REQUIRE(replay.delta_translation == accumulated.delta_translation);
    MK_REQUIRE(std::abs(replay.delta_rotation_z_radians - accumulated.delta_rotation_z_radians) < 0.0001F);

    const auto exact_boundary = mirakana::accumulate_animation_root_motion(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 4.0F, .clip_duration_seconds = 2.0F});
    MK_REQUIRE(exact_boundary.delta_translation == (mirakana::Vec3{4.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(exact_boundary.delta_rotation_z_radians - 2.0F) < 0.0001F);

    const auto same_cycle = mirakana::accumulate_animation_root_motion(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{.root_joint_index = 0,
                                                      .from_time_seconds = 0.25F,
                                                      .to_time_seconds = 0.75F,
                                                      .clip_duration_seconds = 2.0F});
    MK_REQUIRE(same_cycle.delta_translation == (mirakana::Vec3{0.5F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(same_cycle.delta_rotation_z_radians - 0.25F) < 0.0001F);

    const auto zero_interval = mirakana::accumulate_animation_root_motion(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 1.0F, .to_time_seconds = 1.0F, .clip_duration_seconds = 2.0F});
    MK_REQUIRE(zero_interval.delta_translation == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(zero_interval.delta_rotation_z_radians) < 0.0001F);

    const auto rest_fallback = mirakana::accumulate_animation_root_motion(
        skeleton, {},
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 4.0F, .clip_duration_seconds = 2.0F});
    MK_REQUIRE(rest_fallback.delta_translation == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(rest_fallback.delta_rotation_z_radians) < 0.0001F);
}

MK_TEST("animation root motion accumulation reports deterministic diagnostics") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const std::vector<mirakana::AnimationJointTrackDesc> tracks{
        mirakana::AnimationJointTrackDesc{
            .joint_index = 0,
            .translation_keyframes = {mirakana::Vec3Keyframe{.time_seconds = 0.0F,
                                                             .value = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
                                      mirakana::Vec3Keyframe{.time_seconds = 2.0F,
                                                             .value = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F}}},
            .rotation_z_keyframes = {mirakana::FloatKeyframe{.time_seconds = 0.0F, .value = 0.0F},
                                     mirakana::FloatKeyframe{.time_seconds = 2.0F, .value = 1.0F}},
            .scale_keyframes = {},
        },
    };

    MK_REQUIRE(mirakana::is_valid_animation_root_motion_accumulation(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 2.0F, .clip_duration_seconds = 2.0F}));
    MK_REQUIRE(mirakana::validate_animation_root_motion_accumulation(
                   skeleton, tracks,
                   mirakana::AnimationRootMotionAccumulationDesc{.root_joint_index = 0,
                                                                 .from_time_seconds = 0.0F,
                                                                 .to_time_seconds = 2.0F,
                                                                 .clip_duration_seconds = 2.0F})
                   .empty());

    auto diagnostics = mirakana::validate_animation_root_motion_accumulation(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 2.0F, .clip_duration_seconds = 0.0F});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationRootMotionDiagnosticCode::invalid_clip_duration);
    MK_REQUIRE(!mirakana::is_valid_animation_root_motion_accumulation(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 2.0F, .clip_duration_seconds = 0.0F}));

    diagnostics = mirakana::validate_animation_root_motion_accumulation(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{.root_joint_index = 0,
                                                      .from_time_seconds = -0.25F,
                                                      .to_time_seconds = 2.0F,
                                                      .clip_duration_seconds = 2.0F});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationRootMotionDiagnosticCode::invalid_time);

    diagnostics = mirakana::validate_animation_root_motion_accumulation(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 2.0F, .to_time_seconds = 1.0F, .clip_duration_seconds = 2.0F});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationRootMotionDiagnosticCode::invalid_time);

    diagnostics = mirakana::validate_animation_root_motion_accumulation(
        skeleton, tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 9, .from_time_seconds = 0.0F, .to_time_seconds = 2.0F, .clip_duration_seconds = 2.0F});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationRootMotionDiagnosticCode::invalid_root_joint_index);

    auto invalid_tracks = tracks;
    invalid_tracks[0].rotation_z_keyframes[1].value = std::numeric_limits<float>::quiet_NaN();
    diagnostics = mirakana::validate_animation_root_motion_accumulation(
        skeleton, invalid_tracks,
        mirakana::AnimationRootMotionAccumulationDesc{
            .root_joint_index = 0, .from_time_seconds = 0.0F, .to_time_seconds = 2.0F, .clip_duration_seconds = 2.0F});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationRootMotionDiagnosticCode::invalid_joint_tracks);

    bool rejected = false;
    try {
        (void)mirakana::accumulate_animation_root_motion(
            skeleton, invalid_tracks,
            mirakana::AnimationRootMotionAccumulationDesc{.root_joint_index = 0,
                                                          .from_time_seconds = 0.0F,
                                                          .to_time_seconds = 2.0F,
                                                          .clip_duration_seconds = 2.0F});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("animation morph mesh cpu applies additive POSITION targets with glTF-style weights") {
    mirakana::AnimationMorphMeshCpuDesc desc;
    desc.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    desc.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}},
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}},
    };
    desc.target_weights = {0.5F, 0.25F};
    MK_REQUIRE(mirakana::is_valid_animation_morph_mesh_cpu_desc(desc));
    const auto morphed = mirakana::apply_animation_morph_targets_positions_cpu(desc);
    MK_REQUIRE(morphed.size() == 1);
    MK_REQUIRE(std::abs(morphed[0].x - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(morphed[0].y - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(morphed[0].z) < 0.0001F);

    desc.targets.clear();
    desc.target_weights.clear();
    const auto passthrough = mirakana::apply_animation_morph_targets_positions_cpu(desc);
    MK_REQUIRE(passthrough == desc.bind_positions);
}

MK_TEST("animation morph mesh cpu applies additive NORMAL morphs with unit-length output") {
    mirakana::AnimationMorphMeshCpuDesc desc;
    desc.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    desc.bind_normals = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}};
    desc.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {},
                                              .normal_deltas = {mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}},
                                              .tangent_deltas = {}},
    };
    desc.target_weights = {1.0F};
    MK_REQUIRE(mirakana::is_valid_animation_morph_mesh_cpu_desc(desc));
    const auto positions = mirakana::apply_animation_morph_targets_positions_cpu(desc);
    MK_REQUIRE(positions == desc.bind_positions);
    const auto normals = mirakana::apply_animation_morph_targets_normals_cpu(desc);
    MK_REQUIRE(normals.size() == 1);
    const auto len = mirakana::length(normals[0]);
    MK_REQUIRE(std::abs(len - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(normals[0].x) < 0.0001F);
    MK_REQUIRE(std::abs(normals[0].y - 0.70710677F) < 0.0001F);
    MK_REQUIRE(std::abs(normals[0].z - 0.70710677F) < 0.0001F);
}

MK_TEST("animation morph mesh cpu mixes POSITION and NORMAL streams per glTF morph target") {
    mirakana::AnimationMorphMeshCpuDesc desc;
    desc.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                           mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}};
    desc.bind_normals = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F},
                         mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}};
    desc.targets = {
        mirakana::AnimationMorphTargetCpuDesc{
            .position_deltas = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
            .normal_deltas = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                              mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
            .tangent_deltas = {},
        },
    };
    desc.target_weights = {0.5F};
    const auto positions = mirakana::apply_animation_morph_targets_positions_cpu(desc);
    MK_REQUIRE(positions[0].x == 0.5F);
    MK_REQUIRE(positions[1].x == 1.0F);
    const auto normals = mirakana::apply_animation_morph_targets_normals_cpu(desc);
    MK_REQUIRE(normals[0] == desc.bind_normals[0]);
    MK_REQUIRE(normals[1] == desc.bind_normals[1]);
}

MK_TEST("animation morph mesh cpu applies additive TANGENT morphs with unit-length output") {
    mirakana::AnimationMorphMeshCpuDesc desc;
    desc.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    desc.bind_tangents = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}};
    desc.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {},
                                              .normal_deltas = {},
                                              .tangent_deltas = {mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}}},
    };
    desc.target_weights = {1.0F};
    MK_REQUIRE(mirakana::is_valid_animation_morph_mesh_cpu_desc(desc));
    const auto positions = mirakana::apply_animation_morph_targets_positions_cpu(desc);
    MK_REQUIRE(positions == desc.bind_positions);
    const auto tangents = mirakana::apply_animation_morph_targets_tangents_cpu(desc);
    MK_REQUIRE(tangents.size() == 1);
    const auto inv_sqrt2 = 0.70710677F;
    MK_REQUIRE(std::abs(mirakana::length(tangents[0]) - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(tangents[0].x - inv_sqrt2) < 0.0001F);
    MK_REQUIRE(std::abs(tangents[0].y - inv_sqrt2) < 0.0001F);
    MK_REQUIRE(std::abs(tangents[0].z) < 0.0001F);
}

MK_TEST("animation morph mesh cpu reports deterministic diagnostics") {
    mirakana::AnimationMorphMeshCpuDesc empty_bind;
    MK_REQUIRE(!mirakana::is_valid_animation_morph_mesh_cpu_desc(empty_bind));
    auto diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(empty_bind);
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::empty_bind_positions);

    mirakana::AnimationMorphMeshCpuDesc count_mismatch;
    count_mismatch.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    count_mismatch.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}}};
    count_mismatch.target_weights = {0.5F, 0.5F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(count_mismatch);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::target_weight_count_mismatch);

    mirakana::AnimationMorphMeshCpuDesc bad_delta_count;
    bad_delta_count.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                      mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}};
    bad_delta_count.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}}};
    bad_delta_count.target_weights = {1.0F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(bad_delta_count);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::morph_vertex_count_mismatch);

    mirakana::AnimationMorphMeshCpuDesc bad_weight;
    bad_weight.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    bad_weight.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}}};
    bad_weight.target_weights = {1.5F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(bad_weight);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::invalid_target_weight);

    mirakana::AnimationMorphMeshCpuDesc nan_bind;
    nan_bind.bind_positions = {mirakana::Vec3{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .z = 0.0F}};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(nan_bind);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::invalid_bind_position);

    mirakana::AnimationMorphMeshCpuDesc valid_then_throw;
    valid_then_throw.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    valid_then_throw.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}}};
    valid_then_throw.target_weights = {1.0F};
    valid_then_throw.targets[0].position_deltas[0] =
        mirakana::Vec3{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .z = 0.0F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(valid_then_throw);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::invalid_position_delta);

    bool threw = false;
    try {
        (void)mirakana::apply_animation_morph_targets_positions_cpu(valid_then_throw);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    MK_REQUIRE(threw);

    mirakana::AnimationMorphMeshCpuDesc empty_streams;
    empty_streams.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    empty_streams.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {}, .normal_deltas = {}, .tangent_deltas = {}}};
    empty_streams.target_weights = {0.5F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(empty_streams);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::morph_target_empty_streams);

    mirakana::AnimationMorphMeshCpuDesc normal_without_bind;
    normal_without_bind.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    normal_without_bind.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {},
                                              .normal_deltas = {mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}},
                                              .tangent_deltas = {}}};
    normal_without_bind.target_weights = {1.0F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(normal_without_bind);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code ==
               mirakana::AnimationMorphMeshDiagnosticCode::missing_bind_normals_for_normal_morph);

    mirakana::AnimationMorphMeshCpuDesc unused_bind_normals;
    unused_bind_normals.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    unused_bind_normals.bind_normals = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}};
    unused_bind_normals.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}}};
    unused_bind_normals.target_weights = {1.0F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(unused_bind_normals);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::bind_normals_without_normal_morph);

    mirakana::AnimationMorphMeshCpuDesc position_only_valid;
    position_only_valid.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    position_only_valid.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}}};
    position_only_valid.target_weights = {1.0F};
    MK_REQUIRE(mirakana::is_valid_animation_morph_mesh_cpu_desc(position_only_valid));
    bool normals_threw = false;
    try {
        (void)mirakana::apply_animation_morph_targets_normals_cpu(position_only_valid);
    } catch (const std::invalid_argument&) {
        normals_threw = true;
    }
    MK_REQUIRE(normals_threw);

    mirakana::AnimationMorphMeshCpuDesc tangent_without_bind;
    tangent_without_bind.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    tangent_without_bind.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {},
                                              .normal_deltas = {},
                                              .tangent_deltas = {mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}}}};
    tangent_without_bind.target_weights = {1.0F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(tangent_without_bind);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code ==
               mirakana::AnimationMorphMeshDiagnosticCode::missing_bind_tangents_for_tangent_morph);

    mirakana::AnimationMorphMeshCpuDesc unused_bind_tangents;
    unused_bind_tangents.bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    unused_bind_tangents.bind_tangents = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}};
    unused_bind_tangents.targets = {
        mirakana::AnimationMorphTargetCpuDesc{.position_deltas = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
                                              .normal_deltas = {},
                                              .tangent_deltas = {}}};
    unused_bind_tangents.target_weights = {1.0F};
    diagnostics = mirakana::validate_animation_morph_mesh_cpu_desc(unused_bind_tangents);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationMorphMeshDiagnosticCode::bind_tangents_without_tangent_morph);

    bool tangents_threw = false;
    try {
        (void)mirakana::apply_animation_morph_targets_tangents_cpu(position_only_valid);
    } catch (const std::invalid_argument&) {
        tangents_threw = true;
    }
    MK_REQUIRE(tangents_threw);
}

MK_TEST("animation morph weights track samples with clamped linear interpolation") {
    mirakana::AnimationMorphWeightsTrackDesc track;
    track.morph_target_count = 2;
    track.keyframes = {
        mirakana::AnimationMorphWeightsKeyframeDesc{.time_seconds = 0.0F, .weights = {0.0F, 0.0F}},
        mirakana::AnimationMorphWeightsKeyframeDesc{.time_seconds = 1.0F, .weights = {1.0F, 0.5F}},
    };
    MK_REQUIRE(mirakana::is_valid_animation_morph_weights_track_desc(track));
    const auto mid = mirakana::sample_animation_morph_weights_at_time(track, 0.5F);
    MK_REQUIRE(mid.size() == 2);
    MK_REQUIRE(std::abs(mid[0] - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(mid[1] - 0.25F) < 0.0001F);
    const auto clamped = mirakana::sample_animation_morph_weights_at_time(track, 10.0F);
    MK_REQUIRE(std::abs(clamped[0] - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(clamped[1] - 0.5F) < 0.0001F);
}

MK_TEST("animation two bone IK reaches planar targets within reachable annulus") {
    mirakana::AnimationTwoBoneIkXyDesc desc{.parent_bone_length = 1.0F,
                                            .child_bone_length = 1.0F,
                                            .target_xy = {.x = 1.0F, .y = 0.0F},
                                            .bend_side = mirakana::AnimationTwoBoneIkXyBendSide::negative,
                                            .pole_vector_xy = {}};
    mirakana::AnimationTwoBoneIkXySolution sol{};
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_two_bone_ik_xy_plane(desc, sol, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    const float c0 = std::cos(sol.parent_rotation_z_radians);
    const float s0 = std::sin(sol.parent_rotation_z_radians);
    const float sum_angle = sol.parent_rotation_z_radians + sol.child_rotation_z_radians;
    const float c1 = std::cos(sum_angle);
    const float s1 = std::sin(sum_angle);
    const float tip_x = (desc.parent_bone_length * c0) + (desc.child_bone_length * c1);
    const float tip_y = (desc.parent_bone_length * s0) + (desc.child_bone_length * s1);
    MK_REQUIRE(std::abs(tip_x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(tip_y) < 0.0001F);

    mirakana::AnimationTwoBoneIkXyDesc far_desc = desc;
    far_desc.target_xy = {.x = 10.0F, .y = 0.0F};
    MK_REQUIRE(!mirakana::solve_animation_two_bone_ik_xy_plane(far_desc, sol, diagnostic));
    MK_REQUIRE(!diagnostic.empty());
}

MK_TEST("animation two bone IK selects explicit planar bend side") {
    const auto solve_elbow = [](mirakana::AnimationTwoBoneIkXyBendSide bend_side) {
        mirakana::AnimationTwoBoneIkXyDesc desc{.parent_bone_length = 1.0F,
                                                .child_bone_length = 1.0F,
                                                .target_xy = {.x = 1.0F, .y = 0.0F},
                                                .bend_side = bend_side,
                                                .pole_vector_xy = {}};
        mirakana::AnimationTwoBoneIkXySolution sol{};
        std::string diagnostic;
        MK_REQUIRE(mirakana::solve_animation_two_bone_ik_xy_plane(desc, sol, diagnostic));
        MK_REQUIRE(diagnostic.empty());

        const mirakana::Vec2 elbow{.x = desc.parent_bone_length * std::cos(sol.parent_rotation_z_radians),
                                   .y = desc.parent_bone_length * std::sin(sol.parent_rotation_z_radians)};
        const auto tip_angle = sol.parent_rotation_z_radians + sol.child_rotation_z_radians;
        const mirakana::Vec2 tip{.x = elbow.x + (desc.child_bone_length * std::cos(tip_angle)),
                                 .y = elbow.y + (desc.child_bone_length * std::sin(tip_angle))};
        MK_REQUIRE(std::abs(tip.x - desc.target_xy.x) < 0.0001F);
        MK_REQUIRE(std::abs(tip.y - desc.target_xy.y) < 0.0001F);
        return elbow;
    };

    const auto positive_side_elbow = solve_elbow(mirakana::AnimationTwoBoneIkXyBendSide::positive);
    MK_REQUIRE(positive_side_elbow.y > 0.0F);

    const auto negative_side_elbow = solve_elbow(mirakana::AnimationTwoBoneIkXyBendSide::negative);
    MK_REQUIRE(negative_side_elbow.y < 0.0F);
}

MK_TEST("animation two bone IK pole vector selects bend side and rejects invalid poles") {
    mirakana::AnimationTwoBoneIkXyDesc desc{.parent_bone_length = 1.0F,
                                            .child_bone_length = 1.0F,
                                            .target_xy = {.x = 1.0F, .y = 0.0F},
                                            .bend_side = mirakana::AnimationTwoBoneIkXyBendSide::negative,
                                            .pole_vector_xy = mirakana::Vec2{.x = 0.0F, .y = 1.0F}};
    mirakana::AnimationTwoBoneIkXySolution sol{};
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_two_bone_ik_xy_plane(desc, sol, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(std::sin(sol.parent_rotation_z_radians) > 0.0F);

    desc.bend_side = mirakana::AnimationTwoBoneIkXyBendSide::positive;
    desc.pole_vector_xy = mirakana::Vec2{.x = 0.0F, .y = -1.0F};
    MK_REQUIRE(mirakana::solve_animation_two_bone_ik_xy_plane(desc, sol, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(std::sin(sol.parent_rotation_z_radians) < 0.0F);

    desc.pole_vector_xy = mirakana::Vec2{.x = 2.0F, .y = 0.0F};
    MK_REQUIRE(!mirakana::solve_animation_two_bone_ik_xy_plane(desc, sol, diagnostic));
    MK_REQUIRE(diagnostic.contains("pole"));

    desc.pole_vector_xy = mirakana::Vec2{.x = std::numeric_limits<float>::quiet_NaN(), .y = 1.0F};
    MK_REQUIRE(!mirakana::solve_animation_two_bone_ik_xy_plane(desc, sol, diagnostic));
    MK_REQUIRE(diagnostic.contains("pole"));
}

MK_TEST("animation two bone IK 3D orientation reaches target with orthonormal frames") {
    const auto near = [](float lhs, float rhs) { return std::abs(lhs - rhs) < 0.0001F; };
    const auto near_vec = [](mirakana::Vec3 lhs, mirakana::Vec3 rhs) { return mirakana::length(lhs - rhs) < 0.0001F; };
    const auto assert_frame = [&](const mirakana::AnimationTwoBoneIk3dJointFrame& frame) {
        MK_REQUIRE(near(mirakana::length(frame.x_axis), 1.0F));
        MK_REQUIRE(near(mirakana::length(frame.y_axis), 1.0F));
        MK_REQUIRE(near(mirakana::length(frame.z_axis), 1.0F));
        MK_REQUIRE(near(mirakana::dot(frame.x_axis, frame.y_axis), 0.0F));
        MK_REQUIRE(near(mirakana::dot(frame.x_axis, frame.z_axis), 0.0F));
        MK_REQUIRE(near(mirakana::dot(frame.y_axis, frame.z_axis), 0.0F));
    };

    mirakana::AnimationTwoBoneIk3dDesc desc{.parent_bone_length = 1.0F,
                                            .child_bone_length = 1.0F,
                                            .target = {.x = 1.0F, .y = 1.0F, .z = 1.0F},
                                            .pole_vector = {.x = 0.0F, .y = 0.0F, .z = 1.0F}};
    mirakana::AnimationTwoBoneIk3dSolution solution{};
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_two_bone_ik_3d_orientation(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(near_vec(solution.root_position, mirakana::Vec3{}));
    MK_REQUIRE(near_vec(solution.tip_position, desc.target));
    MK_REQUIRE(near_vec(solution.elbow_position, solution.parent_frame.x_axis * desc.parent_bone_length));
    MK_REQUIRE(near_vec(solution.tip_position - solution.elbow_position,
                        solution.child_frame.x_axis * desc.child_bone_length));
    assert_frame(solution.parent_frame);
    assert_frame(solution.child_frame);
    MK_REQUIRE(mirakana::dot(solution.parent_frame.y_axis, desc.pole_vector) > 0.0F);
}

MK_TEST("animation two bone IK 3D orientation rejects unreachable targets and collinear poles") {
    mirakana::AnimationTwoBoneIk3dDesc desc{.parent_bone_length = 1.0F,
                                            .child_bone_length = 1.0F,
                                            .target = {.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                            .pole_vector = {.x = 0.0F, .y = 1.0F, .z = 0.0F}};
    mirakana::AnimationTwoBoneIk3dSolution solution{};
    solution.tip_position = {.x = 10.0F, .y = 10.0F, .z = 10.0F};
    std::string diagnostic;

    desc.target = {.x = 10.0F, .y = 0.0F, .z = 0.0F};
    MK_REQUIRE(!mirakana::solve_animation_two_bone_ik_3d_orientation(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("reachable"));
    MK_REQUIRE(solution.tip_position == mirakana::Vec3{});

    desc.target = {.x = 1.0F, .y = 0.0F, .z = 0.0F};
    desc.pole_vector = {.x = 2.0F, .y = 0.0F, .z = 0.0F};
    MK_REQUIRE(!mirakana::solve_animation_two_bone_ik_3d_orientation(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("pole"));

    desc.pole_vector = {.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .z = 1.0F};
    MK_REQUIRE(!mirakana::solve_animation_two_bone_ik_3d_orientation(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("finite"));
}

MK_TEST("animation FABRIK 3D chain IK solves reachable targets") {
    const auto near = [](float lhs, float rhs) { return std::abs(lhs - rhs) < 0.0005F; };
    const auto near_vec = [](mirakana::Vec3 lhs, mirakana::Vec3 rhs) { return mirakana::length(lhs - rhs) < 0.0005F; };

    mirakana::AnimationFabrikIk3dDesc desc;
    desc.segment_lengths = {1.0F, 1.0F, 1.0F};
    desc.root_position = {.x = 0.0F, .y = 0.0F, .z = 0.0F};
    desc.target_position = {.x = 1.5F, .y = 1.0F, .z = 0.5F};
    desc.initial_joint_positions = {
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.5F, .y = 0.75F, .z = 0.25F},
        mirakana::Vec3{.x = 2.2F, .y = 1.1F, .z = 0.6F},
    };
    desc.max_iterations = 64;
    desc.tolerance = 0.0005F;

    mirakana::AnimationFabrikIk3dSolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_3d_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(solution.reached);
    MK_REQUIRE(!solution.target_was_unreachable);
    MK_REQUIRE(solution.joint_positions.size() == 4);
    MK_REQUIRE(near_vec(solution.joint_positions.front(), desc.root_position));
    MK_REQUIRE(near_vec(solution.joint_positions.back(), desc.target_position));
    MK_REQUIRE(solution.end_effector_error <= desc.tolerance);
    MK_REQUIRE(solution.iterations <= desc.max_iterations);
    for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
        const auto delta = solution.joint_positions[segment + 1U] - solution.joint_positions[segment];
        MK_REQUIRE(near(mirakana::length(delta), desc.segment_lengths[segment]));
    }
}

MK_TEST("animation FABRIK 3D chain IK clamps unreachable targets deterministically") {
    const auto near_vec = [](mirakana::Vec3 lhs, mirakana::Vec3 rhs) { return mirakana::length(lhs - rhs) < 0.0001F; };

    mirakana::AnimationFabrikIk3dDesc desc;
    desc.segment_lengths = {1.0F, 1.0F};
    desc.root_position = {.x = 0.0F, .y = 0.0F, .z = 0.0F};
    desc.target_position = {.x = 5.0F, .y = 0.0F, .z = 0.0F};

    mirakana::AnimationFabrikIk3dSolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_3d_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(!solution.reached);
    MK_REQUIRE(solution.target_was_unreachable);
    MK_REQUIRE(std::abs(solution.end_effector_error - 3.0F) < 0.0001F);
    MK_REQUIRE(solution.iterations == 0U);
    MK_REQUIRE(solution.joint_positions.size() == 3);
    MK_REQUIRE(near_vec(solution.joint_positions[0], mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(near_vec(solution.joint_positions[1], mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(near_vec(solution.joint_positions[2], mirakana::Vec3{2.0F, 0.0F, 0.0F}));

    desc.segment_lengths = {3.0F, 1.0F, 1.0F};
    desc.target_position = {.x = 0.25F, .y = 0.0F, .z = 0.0F};
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_3d_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(!solution.reached);
    MK_REQUIRE(solution.target_was_unreachable);
    MK_REQUIRE(std::abs(solution.end_effector_error - 0.75F) < 0.0001F);
    MK_REQUIRE(solution.iterations == 0U);
    MK_REQUIRE(solution.joint_positions.size() == 4);
    MK_REQUIRE(near_vec(solution.joint_positions[0], mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(near_vec(solution.joint_positions[1], mirakana::Vec3{3.0F, 0.0F, 0.0F}));
    MK_REQUIRE(near_vec(solution.joint_positions[2], mirakana::Vec3{2.0F, 0.0F, 0.0F}));
    MK_REQUIRE(near_vec(solution.joint_positions[3], mirakana::Vec3{1.0F, 0.0F, 0.0F}));
}

MK_TEST("animation FABRIK 3D chain IK reports invalid descriptions") {
    mirakana::AnimationFabrikIk3dDesc desc;
    desc.segment_lengths = {1.0F, -1.0F};
    desc.root_position = {.x = 0.0F, .y = 0.0F, .z = 0.0F};
    desc.target_position = {.x = 1.0F, .y = 0.0F, .z = 0.0F};

    mirakana::AnimationFabrikIk3dSolution solution;
    solution.joint_positions = {mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F}};
    std::string diagnostic;
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_3d_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("segment lengths"));
    MK_REQUIRE(solution.joint_positions.empty());

    desc.segment_lengths = {1.0F, 1.0F};
    desc.initial_joint_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_3d_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("initial_joint_positions"));

    desc.initial_joint_positions = {
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
    };
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_3d_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("zero-length"));

    desc.initial_joint_positions.clear();
    desc.root_position = {.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .z = 0.0F};
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_3d_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("finite"));
}

MK_TEST("animation FABRIK 3D IK solution applies to quaternion skeleton pose rotations") {
    const auto near_vec = [](mirakana::Vec3 lhs, mirakana::Vec3 rhs) { return mirakana::length(lhs - rhs) < 0.001F; };
    const auto near_quat = [](mirakana::Quat lhs, mirakana::Quat rhs) {
        return std::abs(lhs.x - rhs.x) < 0.0001F && std::abs(lhs.y - rhs.y) < 0.0001F &&
               std::abs(lhs.z - rhs.z) < 0.0001F && std::abs(lhs.w - rhs.w) < 0.0001F;
    };

    const mirakana::AnimationSkeleton3dDesc skeleton{
        {
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{
                        .translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                        .rotation = mirakana::Quat::identity(),
                        .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
                    },
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "mid",
                .parent_index = 0,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{
                        .translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                        .rotation = mirakana::Quat::identity(),
                        .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
                    },
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "tip",
                .parent_index = 1,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{
                        .translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                        .rotation = mirakana::Quat::identity(),
                        .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F},
                    },
            },
        },
    };
    mirakana::AnimationFabrikIk3dSolution solution;
    solution.joint_positions = {
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
        mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 1.0F},
    };

    auto pose = mirakana::make_animation_rest_pose_3d(skeleton);
    const std::vector<std::size_t> joint_chain{0, 1, 2};
    std::string diagnostic;
    MK_REQUIRE(
        mirakana::apply_animation_fabrik_ik_3d_solution_to_pose(skeleton, solution, joint_chain, pose, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(mirakana::validate_animation_pose_3d(skeleton, pose).empty());
    MK_REQUIRE(mirakana::is_normalized_quat(pose.joints[0].rotation));
    MK_REQUIRE(mirakana::is_normalized_quat(pose.joints[1].rotation));
    MK_REQUIRE(near_quat(pose.joints[2].rotation, mirakana::Quat::identity()));
    MK_REQUIRE(pose.joints[1].translation == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(pose.joints[1].scale == (mirakana::Vec3{1.0F, 1.0F, 1.0F}));

    const auto model_pose = mirakana::build_animation_model_pose_3d(skeleton, pose);
    for (std::size_t index = 0; index < joint_chain.size(); ++index) {
        const auto joint_origin = mirakana::transform_point(model_pose.joint_matrices[joint_chain[index]],
                                                            mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        MK_REQUIRE(near_vec(joint_origin, solution.joint_positions[index]));
    }
}

MK_TEST("animation FABRIK 3D IK pose application rejects invalid inputs without mutating pose") {
    const auto near_quat = [](mirakana::Quat lhs, mirakana::Quat rhs) {
        return std::abs(lhs.x - rhs.x) < 0.0001F && std::abs(lhs.y - rhs.y) < 0.0001F &&
               std::abs(lhs.z - rhs.z) < 0.0001F && std::abs(lhs.w - rhs.w) < 0.0001F;
    };

    const mirakana::AnimationSkeleton3dDesc skeleton{
        {
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "mid",
                .parent_index = 0,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "tip",
                .parent_index = 1,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    mirakana::AnimationFabrikIk3dSolution solution;
    solution.joint_positions = {
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
        mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 1.0F},
    };
    auto pose = mirakana::make_animation_rest_pose_3d(skeleton);
    const auto& original_pose = pose;
    std::string diagnostic;
    const std::vector<std::size_t> joint_chain{0, 1, 2};
    const auto expect_rejected_without_mutation = [&](std::span<const std::size_t> candidate_chain,
                                                      const mirakana::AnimationFabrikIk3dSolution& candidate_solution,
                                                      mirakana::AnimationPose3d candidate_pose,
                                                      std::string_view expected_diagnostic) {
        const auto before = candidate_pose;
        MK_REQUIRE(!mirakana::apply_animation_fabrik_ik_3d_solution_to_pose(
            skeleton, candidate_solution, candidate_chain, candidate_pose, diagnostic));
        MK_REQUIRE(diagnostic.contains(expected_diagnostic));
        for (std::size_t index = 0; index < candidate_pose.joints.size(); ++index) {
            MK_REQUIRE(candidate_pose.joints[index].translation == before.joints[index].translation);
            MK_REQUIRE(near_quat(candidate_pose.joints[index].rotation, before.joints[index].rotation));
            MK_REQUIRE(candidate_pose.joints[index].scale == before.joints[index].scale);
        }
    };

    const std::vector<std::size_t> invalid_order_chain{0, 2, 1};
    const std::vector<std::size_t> duplicate_chain{0, 1, 1};
    const std::vector<std::size_t> out_of_range_chain{0, 1, 3};
    expect_rejected_without_mutation(invalid_order_chain, solution, original_pose, "parent");
    expect_rejected_without_mutation(duplicate_chain, solution, original_pose, "duplicate");
    expect_rejected_without_mutation(out_of_range_chain, solution, original_pose, "range");

    auto bad_solution = solution;
    bad_solution.joint_positions.pop_back();
    expect_rejected_without_mutation(joint_chain, bad_solution, original_pose, "shape");

    bad_solution = solution;
    bad_solution.joint_positions[1] =
        mirakana::Vec3{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .z = 0.0F};
    expect_rejected_without_mutation(joint_chain, bad_solution, original_pose, "finite");

    bad_solution = solution;
    bad_solution.joint_positions[1] = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F};
    expect_rejected_without_mutation(joint_chain, bad_solution, original_pose, "length");

    auto invalid_pose = original_pose;
    invalid_pose.joints[0].rotation = mirakana::Quat{.x = 0.0F, .y = 0.0F, .z = 0.0F, .w = 2.0F};
    expect_rejected_without_mutation(joint_chain, solution, invalid_pose, "pose");

    invalid_pose = original_pose;
    invalid_pose.joints[1].translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    expect_rejected_without_mutation(joint_chain, solution, invalid_pose, "zero-length");
}

MK_TEST("animation FABRIK 3D IK pose application accepts local rotation limits") {
    const auto near_vec = [](mirakana::Vec3 lhs, mirakana::Vec3 rhs) { return mirakana::length(lhs - rhs) < 0.001F; };
    const auto near_quat = [](mirakana::Quat lhs, mirakana::Quat rhs) {
        return std::abs(lhs.x - rhs.x) < 0.0001F && std::abs(lhs.y - rhs.y) < 0.0001F &&
               std::abs(lhs.z - rhs.z) < 0.0001F && std::abs(lhs.w - rhs.w) < 0.0001F;
    };

    const mirakana::AnimationSkeleton3dDesc skeleton{
        {
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "mid",
                .parent_index = 0,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeleton3dJointDesc{
                .name = "tip",
                .parent_index = 1,
                .rest =
                    mirakana::AnimationJointLocalTransform3d{.translation =
                                                                 mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                             .rotation = mirakana::Quat::identity(),
                                                             .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    mirakana::AnimationFabrikIk3dSolution solution;
    solution.joint_positions = {
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
        mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 1.0F},
    };
    const std::vector<std::size_t> joint_chain{0, 1, 2};
    const std::vector<mirakana::AnimationIkLocalRotationLimit3d> permissive_limits{
        mirakana::AnimationIkLocalRotationLimit3d{
            .joint_index = 0U,
            .twist_axis = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
            .max_swing_radians = std::numbers::pi_v<float>,
            .min_twist_radians = -std::numbers::pi_v<float>,
            .max_twist_radians = std::numbers::pi_v<float>,
        },
        mirakana::AnimationIkLocalRotationLimit3d{
            .joint_index = 1U,
            .twist_axis = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
            .max_swing_radians = std::numbers::pi_v<float>,
            .min_twist_radians = -std::numbers::pi_v<float>,
            .max_twist_radians = std::numbers::pi_v<float>,
        },
    };

    auto pose = mirakana::make_animation_rest_pose_3d(skeleton);
    std::string diagnostic;
    MK_REQUIRE(mirakana::apply_animation_fabrik_ik_3d_solution_to_pose(skeleton, solution, joint_chain,
                                                                       permissive_limits, pose, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    const auto model_pose = mirakana::build_animation_model_pose_3d(skeleton, pose);
    for (std::size_t index = 0; index < joint_chain.size(); ++index) {
        const auto joint_origin = mirakana::transform_point(model_pose.joint_matrices[joint_chain[index]],
                                                            mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        MK_REQUIRE(near_vec(joint_origin, solution.joint_positions[index]));
    }

    const std::vector<mirakana::AnimationIkLocalRotationLimit3d> restrictive_limits{
        mirakana::AnimationIkLocalRotationLimit3d{
            .joint_index = 0U,
            .twist_axis = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
            .max_swing_radians = 0.0F,
            .min_twist_radians = -std::numbers::pi_v<float>,
            .max_twist_radians = std::numbers::pi_v<float>,
        },
    };
    pose = mirakana::make_animation_rest_pose_3d(skeleton);
    const auto before = pose;
    MK_REQUIRE(!mirakana::apply_animation_fabrik_ik_3d_solution_to_pose(skeleton, solution, joint_chain,
                                                                        restrictive_limits, pose, diagnostic));
    MK_REQUIRE(diagnostic.contains("local rotation limits"));
    for (std::size_t index = 0; index < pose.joints.size(); ++index) {
        MK_REQUIRE(pose.joints[index].translation == before.joints[index].translation);
        MK_REQUIRE(near_quat(pose.joints[index].rotation, before.joints[index].rotation));
        MK_REQUIRE(pose.joints[index].scale == before.joints[index].scale);
    }
}

MK_TEST("animation FABRIK chain IK solves reachable planar targets") {
    mirakana::AnimationFabrikIkXyDesc desc;
    desc.segment_lengths = {1.0F, 1.0F, 1.0F};
    desc.root_xy = {.x = 0.0F, .y = 0.0F};
    desc.target_xy = {.x = 2.0F, .y = 1.0F};
    desc.initial_joint_positions = {
        mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        mirakana::Vec2{.x = 2.0F, .y = 0.0F},
        mirakana::Vec2{.x = 3.0F, .y = 0.0F},
    };
    desc.max_iterations = 32;
    desc.tolerance = 0.0001F;

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(solution.reached);
    MK_REQUIRE(!solution.target_was_unreachable);
    MK_REQUIRE(solution.joint_positions.size() == 4);
    MK_REQUIRE(solution.segment_rotation_z_radians.size() == 3);
    MK_REQUIRE(solution.iterations <= desc.max_iterations);
    MK_REQUIRE(solution.end_effector_error <= desc.tolerance);

    const auto end_delta = solution.joint_positions.back() - desc.target_xy;
    MK_REQUIRE(mirakana::length(end_delta) <= desc.tolerance);
    for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
        const auto delta = solution.joint_positions[segment + 1U] - solution.joint_positions[segment];
        MK_REQUIRE(std::abs(mirakana::length(delta) - desc.segment_lengths[segment]) < 0.0001F);
        MK_REQUIRE(std::isfinite(solution.segment_rotation_z_radians[segment]));
    }
}

MK_TEST("animation FABRIK chain IK clamps unreachable targets deterministically") {
    mirakana::AnimationFabrikIkXyDesc desc;
    desc.segment_lengths = {1.0F, 1.0F};
    desc.root_xy = {.x = 0.0F, .y = 0.0F};
    desc.target_xy = {.x = 5.0F, .y = 0.0F};

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(!solution.reached);
    MK_REQUIRE(solution.target_was_unreachable);
    MK_REQUIRE(std::abs(solution.end_effector_error - 3.0F) < 0.0001F);
    MK_REQUIRE(solution.joint_positions.size() == 3);
    MK_REQUIRE(std::abs(solution.joint_positions.back().x - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(solution.joint_positions.back().y) < 0.0001F);
    MK_REQUIRE(std::abs(solution.segment_rotation_z_radians[0]) < 0.0001F);
    MK_REQUIRE(std::abs(solution.segment_rotation_z_radians[1]) < 0.0001F);
}

MK_TEST("animation FABRIK chain IK clamps targets inside minimum reach deterministically") {
    mirakana::AnimationFabrikIkXyDesc desc;
    desc.segment_lengths = {3.0F, 1.0F, 1.0F};
    desc.root_xy = {.x = 0.0F, .y = 0.0F};
    desc.target_xy = {.x = 0.25F, .y = 0.0F};

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(!solution.reached);
    MK_REQUIRE(solution.target_was_unreachable);
    MK_REQUIRE(solution.iterations == 0U);
    MK_REQUIRE(solution.joint_positions.size() == 4);
    MK_REQUIRE(solution.segment_rotation_z_radians.size() == 3);
    MK_REQUIRE(std::abs(solution.end_effector_error - 0.75F) < 0.0001F);
    MK_REQUIRE(std::abs(solution.joint_positions.back().x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(solution.joint_positions.back().y) < 0.0001F);
    for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
        const auto delta = solution.joint_positions[segment + 1U] - solution.joint_positions[segment];
        MK_REQUIRE(std::abs(mirakana::length(delta) - desc.segment_lengths[segment]) < 0.0001F);
        MK_REQUIRE(std::isfinite(solution.segment_rotation_z_radians[segment]));
    }
}

MK_TEST("animation FABRIK chain IK reports invalid descriptions") {
    mirakana::AnimationFabrikIkXyDesc desc;
    desc.segment_lengths = {1.0F, -1.0F};
    desc.root_xy = {.x = 0.0F, .y = 0.0F};
    desc.target_xy = {.x = 1.0F, .y = 0.0F};

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("segment lengths"));
}

MK_TEST("animation FABRIK chain IK applies integrated local rotation limits during solve") {
    mirakana::AnimationFabrikIkXyDesc desc;
    desc.segment_lengths = {1.0F, 1.0F};
    desc.root_xy = {.x = 0.0F, .y = 0.0F};
    desc.target_xy = {.x = 0.0F, .y = 2.0F};
    desc.initial_joint_positions = {
        mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        mirakana::Vec2{.x = 2.0F, .y = 0.0F},
    };
    desc.max_iterations = 32;
    desc.tolerance = 0.0001F;
    desc.local_rotation_limits = {
        mirakana::AnimationFabrikIkXySegmentRotationLimit{
            .segment_index = 0U, .min_rotation_z_radians = -0.1F, .max_rotation_z_radians = 0.1F},
        mirakana::AnimationFabrikIkXySegmentRotationLimit{
            .segment_index = 1U, .min_rotation_z_radians = -0.1F, .max_rotation_z_radians = 0.1F},
    };

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(!solution.reached);
    MK_REQUIRE(!solution.target_was_unreachable);
    MK_REQUIRE(solution.joint_positions.size() == 3);
    MK_REQUIRE(solution.segment_rotation_z_radians.size() == 2);
    MK_REQUIRE(std::abs(solution.segment_rotation_z_radians[0]) <= 0.1001F);
    const auto child_local_rotation = solution.segment_rotation_z_radians[1] - solution.segment_rotation_z_radians[0];
    MK_REQUIRE(std::abs(child_local_rotation) <= 0.1001F);
    MK_REQUIRE(solution.joint_positions.back().x > 1.95F);
    MK_REQUIRE(solution.joint_positions.back().y < 0.35F);
    MK_REQUIRE(solution.end_effector_error > 2.0F);
    for (std::size_t segment = 0; segment < desc.segment_lengths.size(); ++segment) {
        const auto delta = solution.joint_positions[segment + 1U] - solution.joint_positions[segment];
        MK_REQUIRE(std::abs(mirakana::length(delta) - desc.segment_lengths[segment]) < 0.0001F);
    }
}

MK_TEST("animation FABRIK chain IK rejects invalid integrated local rotation limits") {
    mirakana::AnimationFabrikIkXyDesc desc;
    desc.segment_lengths = {1.0F, 1.0F};
    desc.root_xy = {.x = 0.0F, .y = 0.0F};
    desc.target_xy = {.x = 1.0F, .y = 1.0F};
    desc.local_rotation_limits = {
        mirakana::AnimationFabrikIkXySegmentRotationLimit{
            .segment_index = 2U, .min_rotation_z_radians = -0.1F, .max_rotation_z_radians = 0.1F},
    };

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("rotation limit"));

    desc.local_rotation_limits = {
        mirakana::AnimationFabrikIkXySegmentRotationLimit{
            .segment_index = 0U, .min_rotation_z_radians = 0.5F, .max_rotation_z_radians = -0.5F},
    };
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("rotation limit"));

    desc.local_rotation_limits = {
        mirakana::AnimationFabrikIkXySegmentRotationLimit{
            .segment_index = 1U, .min_rotation_z_radians = -0.5F, .max_rotation_z_radians = 0.5F},
        mirakana::AnimationFabrikIkXySegmentRotationLimit{
            .segment_index = 1U, .min_rotation_z_radians = -0.25F, .max_rotation_z_radians = 0.25F},
    };
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("duplicate"));
}

MK_TEST("animation FABRIK chain IK normalizes integrated local rotation limits across pi boundary") {
    constexpr float parent_rotation = 3.10F;
    constexpr float child_rotation = -3.10F;
    const mirakana::Vec2 first_joint{.x = std::cos(parent_rotation), .y = std::sin(parent_rotation)};
    const mirakana::Vec2 target{
        .x = first_joint.x + std::cos(child_rotation),
        .y = first_joint.y + std::sin(child_rotation),
    };

    mirakana::AnimationFabrikIkXyDesc desc;
    desc.segment_lengths = {1.0F, 1.0F};
    desc.root_xy = {.x = 0.0F, .y = 0.0F};
    desc.target_xy = target;
    desc.initial_joint_positions = {
        mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        first_joint,
        mirakana::Vec2{.x = first_joint.x + std::cos(-3.0F), .y = first_joint.y + std::sin(-3.0F)},
    };
    desc.max_iterations = 32;
    desc.tolerance = 0.0001F;
    desc.local_rotation_limits = {
        mirakana::AnimationFabrikIkXySegmentRotationLimit{
            .segment_index = 0U, .min_rotation_z_radians = 3.05F, .max_rotation_z_radians = 3.15F},
        mirakana::AnimationFabrikIkXySegmentRotationLimit{
            .segment_index = 1U, .min_rotation_z_radians = -0.10F, .max_rotation_z_radians = 0.10F},
    };

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(solution.reached);
    MK_REQUIRE(solution.end_effector_error <= desc.tolerance);
    MK_REQUIRE(solution.joint_positions.size() == 3);
    MK_REQUIRE(solution.segment_rotation_z_radians.size() == 2);
    MK_REQUIRE(std::abs(solution.segment_rotation_z_radians[0] - parent_rotation) < 0.001F);
    MK_REQUIRE(std::abs(solution.segment_rotation_z_radians[1] - child_rotation) < 0.001F);
}

MK_TEST("animation FABRIK chain IK selects explicit planar bend side") {
    const auto make_desc = [] {
        mirakana::AnimationFabrikIkXyDesc desc;
        desc.segment_lengths = {1.0F, 1.0F, 1.0F};
        desc.root_xy = {.x = 0.0F, .y = 0.0F};
        desc.target_xy = {.x = 2.0F, .y = 0.0F};
        desc.initial_joint_positions = {
            mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            mirakana::Vec2{.x = 0.7F, .y = 0.7F},
            mirakana::Vec2{.x = 1.4F, .y = 0.7F},
            mirakana::Vec2{.x = 2.0F, .y = 0.0F},
        };
        desc.max_iterations = 32;
        desc.tolerance = 0.0001F;
        return desc;
    };
    const auto first_internal_side = [](const mirakana::AnimationFabrikIkXyDesc& desc,
                                        const mirakana::AnimationFabrikIkXySolution& solution) {
        const auto axis = desc.target_xy - desc.root_xy;
        for (std::size_t joint = 1; joint + 1U < solution.joint_positions.size(); ++joint) {
            const auto offset = solution.joint_positions[joint] - desc.root_xy;
            const auto side = (axis.x * offset.y) - (axis.y * offset.x);
            if (std::abs(side) > 0.0001F) {
                return side;
            }
        }
        return 0.0F;
    };

    auto positive_desc = make_desc();
    positive_desc.bend_side = mirakana::AnimationFabrikIkXyBendSide::positive;
    mirakana::AnimationFabrikIkXySolution positive_solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(positive_desc, positive_solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(positive_solution.reached);
    MK_REQUIRE(first_internal_side(positive_desc, positive_solution) > 0.0F);

    auto negative_desc = make_desc();
    negative_desc.bend_side = mirakana::AnimationFabrikIkXyBendSide::negative;
    mirakana::AnimationFabrikIkXySolution negative_solution;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(negative_desc, negative_solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(negative_solution.reached);
    MK_REQUIRE(first_internal_side(negative_desc, negative_solution) < 0.0F);
    MK_REQUIRE(std::abs(positive_solution.joint_positions[1].x - negative_solution.joint_positions[1].x) < 0.0001F);
    MK_REQUIRE(std::abs(positive_solution.joint_positions[1].y + negative_solution.joint_positions[1].y) < 0.0001F);
}

MK_TEST("animation FABRIK chain IK pole vector selects bend side and rejects invalid poles") {
    mirakana::AnimationFabrikIkXyDesc desc;
    desc.segment_lengths = {1.0F, 1.0F, 1.0F};
    desc.root_xy = {.x = 0.0F, .y = 0.0F};
    desc.target_xy = {.x = 2.0F, .y = 0.0F};
    desc.initial_joint_positions = {
        mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        mirakana::Vec2{.x = 0.7F, .y = 0.7F},
        mirakana::Vec2{.x = 1.4F, .y = 0.7F},
        mirakana::Vec2{.x = 2.0F, .y = 0.0F},
    };
    desc.max_iterations = 32;
    desc.tolerance = 0.0001F;
    desc.bend_side = mirakana::AnimationFabrikIkXyBendSide::negative;
    desc.pole_vector_xy = mirakana::Vec2{.x = 0.0F, .y = 1.0F};

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(solution.reached);
    MK_REQUIRE(solution.joint_positions[1].y > 0.0F);

    desc.bend_side = mirakana::AnimationFabrikIkXyBendSide::positive;
    desc.pole_vector_xy = mirakana::Vec2{.x = 0.0F, .y = -1.0F};
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(solution.reached);
    MK_REQUIRE(solution.joint_positions[1].y < 0.0F);

    desc.pole_vector_xy = mirakana::Vec2{.x = 2.0F, .y = 0.0F};
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("pole"));

    desc.pole_vector_xy = mirakana::Vec2{.x = 0.0F, .y = 0.0F};
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("pole"));

    desc.pole_vector_xy = mirakana::Vec2{.x = std::numeric_limits<float>::quiet_NaN(), .y = 1.0F};
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("pole"));

    desc.pole_vector_xy = mirakana::Vec2{.x = 0.0F, .y = 1.0F};
    desc.target_xy = desc.root_xy;
    MK_REQUIRE(!mirakana::solve_animation_fabrik_ik_xy_chain(desc, solution, diagnostic));
    MK_REQUIRE(diagnostic.contains("pole"));
}

MK_TEST("animation FABRIK IK solution applies to skeleton pose rotations") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "elbow",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 1,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };

    mirakana::AnimationFabrikIkXyDesc ik_desc;
    ik_desc.segment_lengths = {1.0F, 1.0F};
    ik_desc.root_xy = {.x = 0.0F, .y = 0.0F};
    ik_desc.target_xy = {.x = 1.0F, .y = 1.0F};
    ik_desc.initial_joint_positions = {
        mirakana::Vec2{.x = 0.0F, .y = 0.0F},
        mirakana::Vec2{.x = 1.0F, .y = 0.0F},
        mirakana::Vec2{.x = 2.0F, .y = 0.0F},
    };
    ik_desc.max_iterations = 32;
    ik_desc.tolerance = 0.0001F;

    mirakana::AnimationFabrikIkXySolution solution;
    std::string diagnostic;
    MK_REQUIRE(mirakana::solve_animation_fabrik_ik_xy_chain(ik_desc, solution, diagnostic));
    MK_REQUIRE(solution.reached);

    auto pose = mirakana::make_animation_rest_pose(skeleton);
    const std::vector<std::size_t> joint_chain{0, 1, 2};
    MK_REQUIRE(
        mirakana::apply_animation_fabrik_ik_xy_solution_to_pose(skeleton, solution, joint_chain, pose, diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(mirakana::validate_animation_pose(skeleton, pose).empty());
    MK_REQUIRE(std::abs(pose.joints[2].rotation_z_radians) < 0.0001F);

    const auto model_pose = mirakana::build_animation_model_pose(skeleton, pose);
    for (std::size_t index = 0; index < joint_chain.size(); ++index) {
        const auto joint_origin = mirakana::transform_point(model_pose.joint_matrices[joint_chain[index]],
                                                            mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
        MK_REQUIRE(std::abs(joint_origin.x - solution.joint_positions[index].x) < 0.001F);
        MK_REQUIRE(std::abs(joint_origin.y - solution.joint_positions[index].y) < 0.001F);
        MK_REQUIRE(std::abs(joint_origin.z) < 0.0001F);
    }
}

MK_TEST("animation FABRIK IK pose application rejects invalid chains without mutating pose") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.25F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "elbow",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.5F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 1,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.75F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    auto pose = mirakana::make_animation_rest_pose(skeleton);
    const auto original_pose = pose;

    mirakana::AnimationFabrikIkXySolution solution;
    solution.joint_positions = {mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 1.0F, .y = 0.0F},
                                mirakana::Vec2{.x = 2.0F, .y = 0.0F}};
    solution.segment_rotation_z_radians = {0.0F, 0.0F};

    std::string diagnostic;
    const std::vector<std::size_t> invalid_joint_chain{0, 2, 1};
    MK_REQUIRE(!mirakana::apply_animation_fabrik_ik_xy_solution_to_pose(skeleton, solution, invalid_joint_chain, pose,
                                                                        diagnostic));
    MK_REQUIRE(diagnostic.contains("parent"));
    MK_REQUIRE(pose.joints.size() == original_pose.joints.size());
    for (std::size_t index = 0; index < pose.joints.size(); ++index) {
        MK_REQUIRE(pose.joints[index].translation == original_pose.joints[index].translation);
        MK_REQUIRE(std::abs(pose.joints[index].rotation_z_radians - original_pose.joints[index].rotation_z_radians) <
                   0.0001F);
        MK_REQUIRE(pose.joints[index].scale == original_pose.joints[index].scale);
    }
}

MK_TEST("animation FABRIK IK pose application clamps local rotation limits") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "elbow",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 1,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.75F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    mirakana::AnimationFabrikIkXySolution solution;
    solution.joint_positions = {mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 0.54F, .y = 0.84F},
                                mirakana::Vec2{.x = 0.86F, .y = 1.78F}};
    solution.segment_rotation_z_radians = {1.0F, 1.25F};

    auto pose = mirakana::make_animation_rest_pose(skeleton);
    const std::vector<std::size_t> joint_chain{0, 1, 2};
    const std::vector<mirakana::AnimationIkLocalRotationLimit> limits{
        mirakana::AnimationIkLocalRotationLimit{
            .joint_index = 0, .min_rotation_z_radians = -0.25F, .max_rotation_z_radians = 0.25F},
        mirakana::AnimationIkLocalRotationLimit{
            .joint_index = 1, .min_rotation_z_radians = -0.5F, .max_rotation_z_radians = 0.5F},
    };

    std::string diagnostic;
    MK_REQUIRE(mirakana::apply_animation_fabrik_ik_xy_solution_to_pose(skeleton, solution, joint_chain, limits, pose,
                                                                       diagnostic));
    MK_REQUIRE(diagnostic.empty());
    MK_REQUIRE(std::abs(pose.joints[0].rotation_z_radians - 0.25F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[1].rotation_z_radians - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[2].rotation_z_radians - 0.75F) < 0.0001F);
    MK_REQUIRE(pose.joints[1].translation == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(pose.joints[1].scale == (mirakana::Vec3{1.0F, 1.0F, 1.0F}));
}

MK_TEST("animation FABRIK IK pose application normalizes local rotation limits across pi boundary") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "elbow",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 1,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    mirakana::AnimationFabrikIkXySolution solution;
    solution.joint_positions = {mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = -1.0F, .y = 0.04F},
                                mirakana::Vec2{.x = -2.0F, .y = 0.0F}};
    solution.segment_rotation_z_radians = {3.10F, -3.10F};

    auto pose = mirakana::make_animation_rest_pose(skeleton);
    const std::vector<std::size_t> joint_chain{0, 1, 2};
    const std::vector<mirakana::AnimationIkLocalRotationLimit> limits{
        mirakana::AnimationIkLocalRotationLimit{
            .joint_index = 1, .min_rotation_z_radians = -0.20F, .max_rotation_z_radians = 0.20F},
    };

    std::string diagnostic;
    MK_REQUIRE(mirakana::apply_animation_fabrik_ik_xy_solution_to_pose(skeleton, solution, joint_chain, limits, pose,
                                                                       diagnostic));
    MK_REQUIRE(diagnostic.empty());
    constexpr float expected_child_local_rotation = (2.0F * std::numbers::pi_v<float>)-6.20F;
    MK_REQUIRE(std::abs(pose.joints[0].rotation_z_radians - 3.10F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[1].rotation_z_radians - expected_child_local_rotation) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[2].rotation_z_radians) < 0.0001F);
}

MK_TEST("animation FABRIK IK pose application rejects invalid local rotation limits") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.25F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "elbow",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.5F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 1,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.75F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    auto pose = mirakana::make_animation_rest_pose(skeleton);
    const auto original_pose = pose;

    mirakana::AnimationFabrikIkXySolution solution;
    solution.joint_positions = {mirakana::Vec2{.x = 0.0F, .y = 0.0F}, mirakana::Vec2{.x = 1.0F, .y = 0.0F},
                                mirakana::Vec2{.x = 2.0F, .y = 0.0F}};
    solution.segment_rotation_z_radians = {0.0F, 0.0F};

    std::string diagnostic;
    const std::vector<std::size_t> joint_chain{0, 1, 2};
    const std::vector<mirakana::AnimationIkLocalRotationLimit> invalid_limits{
        mirakana::AnimationIkLocalRotationLimit{
            .joint_index = 1, .min_rotation_z_radians = 0.5F, .max_rotation_z_radians = -0.5F},
    };
    MK_REQUIRE(!mirakana::apply_animation_fabrik_ik_xy_solution_to_pose(skeleton, solution, joint_chain, invalid_limits,
                                                                        pose, diagnostic));
    MK_REQUIRE(diagnostic.contains("limit"));
    for (std::size_t index = 0; index < pose.joints.size(); ++index) {
        MK_REQUIRE(pose.joints[index].translation == original_pose.joints[index].translation);
        MK_REQUIRE(std::abs(pose.joints[index].rotation_z_radians - original_pose.joints[index].rotation_z_radians) <
                   0.0001F);
        MK_REQUIRE(pose.joints[index].scale == original_pose.joints[index].scale);
    }
}

MK_TEST("animation skin payload normalizes and builds deterministic palettes") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    mirakana::AnimationPose pose;
    pose.joints = {
        mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                      .rotation_z_radians = 0.0F,
                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
        mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 3.0F, .z = 0.0F},
                                      .rotation_z_radians = 0.0F,
                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
    };
    const mirakana::AnimationSkinPayloadDesc skin{
        .vertex_count = 2,
        .joints =
            {
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                 .inverse_bind_matrix = mirakana::Mat4::translation(
                                                     mirakana::Vec3{.x = -1.0F, .y = 0.0F, .z = 0.0F})},
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 1,
                                                 .inverse_bind_matrix = mirakana::Mat4::translation(
                                                     mirakana::Vec3{.x = -1.0F, .y = -2.0F, .z = 0.0F})},
            },
        .vertices =
            {
                mirakana::AnimationSkinVertexWeights{
                    {
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 2.0F},
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 2.0F},
                    },
                },
                mirakana::AnimationSkinVertexWeights{
                    {
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 3.0F},
                    },
                },
            },
    };

    MK_REQUIRE(mirakana::is_valid_animation_skin_payload(skeleton, skin));
    MK_REQUIRE(mirakana::validate_animation_skin_payload(skeleton, skin).empty());

    const auto normalized = mirakana::normalize_animation_skin_payload(skeleton, skin);
    const auto replay_normalized = mirakana::normalize_animation_skin_payload(skeleton, skin);
    MK_REQUIRE(normalized.vertex_count == 2);
    MK_REQUIRE(normalized.vertices.size() == 2);
    MK_REQUIRE(normalized.vertices[0].influences.size() == 2);
    MK_REQUIRE(std::abs(normalized.vertices[0].influences[0].weight - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(normalized.vertices[0].influences[1].weight - 0.5F) < 0.0001F);
    MK_REQUIRE(std::abs(normalized.vertices[1].influences[0].weight - 1.0F) < 0.0001F);
    MK_REQUIRE(replay_normalized.vertices[0].influences[0].skin_joint_index ==
               normalized.vertices[0].influences[0].skin_joint_index);
    MK_REQUIRE(std::abs(replay_normalized.vertices[0].influences[0].weight -
                        normalized.vertices[0].influences[0].weight) < 0.0001F);

    const auto model_pose = mirakana::build_animation_model_pose(skeleton, pose);
    const auto replay_model_pose = mirakana::build_animation_model_pose(skeleton, pose);
    MK_REQUIRE(model_pose.joint_matrices.size() == 2);
    MK_REQUIRE(mirakana::transform_point(model_pose.joint_matrices[0], mirakana::Vec3{0.0F, 0.0F, 0.0F}) ==
               (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(mirakana::transform_point(model_pose.joint_matrices[1], mirakana::Vec3{0.0F, 0.0F, 0.0F}) ==
               (mirakana::Vec3{1.0F, 3.0F, 0.0F}));
    MK_REQUIRE(replay_model_pose.joint_matrices[1].at(1, 3) == model_pose.joint_matrices[1].at(1, 3));

    const auto palette = mirakana::build_animation_skinning_palette(skeleton, skin, pose);
    const auto replay_palette = mirakana::build_animation_skinning_palette(skeleton, skin, pose);
    MK_REQUIRE(palette.matrices.size() == 2);
    MK_REQUIRE(mirakana::transform_point(palette.matrices[0], mirakana::Vec3{0.0F, 0.0F, 0.0F}) ==
               (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(mirakana::transform_point(palette.matrices[1], mirakana::Vec3{0.0F, 0.0F, 0.0F}) ==
               (mirakana::Vec3{0.0F, 1.0F, 0.0F}));
    MK_REQUIRE(replay_palette.matrices[1].at(1, 3) == palette.matrices[1].at(1, 3));

    const mirakana::AnimationSkeletonDesc rotated_scaled_skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 1.57079637F,
                                                      .scale = mirakana::Vec3{.x = 2.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "child",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const auto rotated_scaled_pose = mirakana::build_animation_model_pose(
        rotated_scaled_skeleton, mirakana::make_animation_rest_pose(rotated_scaled_skeleton));
    const auto transformed_child = mirakana::transform_point(rotated_scaled_pose.joint_matrices[1],
                                                             mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F});
    MK_REQUIRE(std::abs(transformed_child.x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(transformed_child.y - 4.0F) < 0.0001F);
}

MK_TEST("animation skin payload reports deterministic diagnostics") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    const mirakana::AnimationSkinPayloadDesc valid{
        .vertex_count = 1,
        .joints =
            {
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                 .inverse_bind_matrix = mirakana::Mat4::identity()},
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 1,
                                                 .inverse_bind_matrix = mirakana::Mat4::translation(
                                                     mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F})},
            },
        .vertices =
            {
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
            },
    };

    auto diagnostics = mirakana::validate_animation_skin_payload(skeleton, mirakana::AnimationSkinPayloadDesc{});
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::empty_skin);

    auto invalid = valid;
    invalid.joints[1].skeleton_joint_index = 0;
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::duplicate_skin_joint);

    invalid = valid;
    invalid.joints[0].skeleton_joint_index = 9;
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::invalid_skin_joint);

    invalid = valid;
    invalid.joints[0].inverse_bind_matrix.at(0, 0) = std::numeric_limits<float>::quiet_NaN();
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::invalid_inverse_bind_matrix);

    invalid = valid;
    invalid.joints[1].inverse_bind_matrix = mirakana::Mat4::identity();
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::bind_pose_mismatch);

    invalid = valid;
    invalid.vertex_count = 2;
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::vertex_count_mismatch);

    invalid = valid;
    invalid.vertices[0].influences.clear();
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::invalid_vertex_influence_count);

    invalid = valid;
    invalid.vertices[0].influences = {
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
    };
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::invalid_vertex_influence_count);

    invalid = valid;
    invalid.vertices[0].influences[0].skin_joint_index = 5;
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::invalid_vertex_joint_index);
    MK_REQUIRE(diagnostics[0].influence_index == 0);

    invalid = valid;
    invalid.vertices[0].influences = {
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = -1.0F},
    };
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::invalid_vertex_weight);
    MK_REQUIRE(diagnostics[0].influence_index == 1);

    invalid = valid;
    invalid.vertices[0].influences[0].weight = std::numeric_limits<float>::infinity();
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::invalid_vertex_weight);

    invalid = valid;
    invalid.vertices[0].influences = {
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = std::numeric_limits<float>::max()},
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = std::numeric_limits<float>::max()},
    };
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::invalid_vertex_weight);

    invalid = valid;
    invalid.vertices[0].influences[0].weight = 0.0F;
    diagnostics = mirakana::validate_animation_skin_payload(skeleton, invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationSkinDiagnosticCode::zero_vertex_weight);

    bool rejected_normalize = false;
    try {
        (void)mirakana::normalize_animation_skin_payload(skeleton, invalid);
    } catch (const std::invalid_argument&) {
        rejected_normalize = true;
    }
    MK_REQUIRE(rejected_normalize);

    mirakana::AnimationPose invalid_pose;
    invalid_pose.joints.push_back(mirakana::JointLocalTransform{});
    bool rejected_model_pose = false;
    try {
        (void)mirakana::build_animation_model_pose(skeleton, invalid_pose);
    } catch (const std::invalid_argument&) {
        rejected_model_pose = true;
    }
    MK_REQUIRE(rejected_model_pose);

    bool rejected_palette = false;
    try {
        (void)mirakana::build_animation_skinning_palette(skeleton, invalid,
                                                         mirakana::make_animation_rest_pose(skeleton));
    } catch (const std::invalid_argument&) {
        rejected_palette = true;
    }
    MK_REQUIRE(rejected_palette);
}

MK_TEST("animation cpu skinning deforms bind positions deterministically") {
    const mirakana::AnimationSkeletonDesc skeleton{
        {
            mirakana::AnimationSkeletonJointDesc{
                .name = "root",
                .parent_index = mirakana::animation_no_parent,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
            mirakana::AnimationSkeletonJointDesc{
                .name = "hand",
                .parent_index = 0,
                .rest = mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F},
                                                      .rotation_z_radians = 0.0F,
                                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
            },
        },
    };
    mirakana::AnimationPose pose;
    pose.joints = {
        mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                      .rotation_z_radians = 0.0F,
                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
        mirakana::JointLocalTransform{.translation = mirakana::Vec3{.x = 0.0F, .y = 3.0F, .z = 0.0F},
                                      .rotation_z_radians = 0.0F,
                                      .scale = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 1.0F}},
    };

    const mirakana::AnimationSkinPayloadDesc skin{
        .vertex_count = 3,
        .joints =
            {
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                 .inverse_bind_matrix = mirakana::Mat4::translation(
                                                     mirakana::Vec3{.x = -1.0F, .y = 0.0F, .z = 0.0F})},
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 1,
                                                 .inverse_bind_matrix = mirakana::Mat4::translation(
                                                     mirakana::Vec3{.x = -1.0F, .y = -2.0F, .z = 0.0F})},
            },
        .vertices =
            {
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 1.0F}}},
                mirakana::AnimationSkinVertexWeights{
                    {
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 3.0F},
                    },
                },
            },
    };
    const auto palette = mirakana::build_animation_skinning_palette(skeleton, skin, pose);
    const mirakana::AnimationCpuSkinningDesc desc{
        .bind_positions =
            {
                mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 0.0F},
                mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
            },
        .skin = skin,
        .palette = palette,
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };

    const auto diagnostics = mirakana::validate_animation_cpu_skinning_desc(desc);
    MK_REQUIRE(diagnostics.empty());
    MK_REQUIRE(mirakana::is_valid_animation_cpu_skinning_desc(desc));

    const auto skinned = mirakana::skin_animation_vertices_cpu(desc);
    const auto replay = mirakana::skin_animation_vertices_cpu(desc);
    MK_REQUIRE(skinned.positions.size() == 3);
    MK_REQUIRE(skinned.positions[0] == (mirakana::Vec3{0.0F, 0.0F, 0.0F}));
    MK_REQUIRE(skinned.positions[1] == (mirakana::Vec3{1.0F, 3.0F, 0.0F}));
    MK_REQUIRE(std::abs(skinned.positions[2].x - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.positions[2].y - 0.75F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.positions[2].z - 0.0F) < 0.0001F);
    MK_REQUIRE(replay.positions[2] == skinned.positions[2]);
}

MK_TEST("animation cpu skinning applies direct palette rotation and scale") {
    const mirakana::AnimationSkinPayloadDesc skin{
        .vertex_count = 1,
        .joints =
            {
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                 .inverse_bind_matrix = mirakana::Mat4::identity()},
            },
        .vertices =
            {
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
            },
    };
    const mirakana::AnimationCpuSkinningDesc desc{
        .bind_positions = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}},
        .skin = skin,
        .palette = mirakana::AnimationSkinningPalette{{mirakana::Mat4::rotation_z(1.57079637F) *
                                                       mirakana::Mat4::scale(
                                                           mirakana::Vec3{.x = 2.0F, .y = 1.0F, .z = 1.0F})}},
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };

    const auto skinned = mirakana::skin_animation_vertices_cpu(desc);
    MK_REQUIRE(skinned.positions.size() == 1);
    MK_REQUIRE(std::abs(skinned.positions[0].x - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.positions[0].y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.positions[0].z - 0.0F) < 0.0001F);
}

MK_TEST("animation cpu skinning deforms optional bind normals deterministically") {
    const mirakana::AnimationSkinPayloadDesc skin{
        .vertex_count = 3,
        .joints =
            {
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                 .inverse_bind_matrix = mirakana::Mat4::identity()},
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 1,
                                                 .inverse_bind_matrix = mirakana::Mat4::identity()},
            },
        .vertices =
            {
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 1.0F}}},
                mirakana::AnimationSkinVertexWeights{
                    {
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 3.0F},
                    },
                },
            },
    };
    mirakana::AnimationCpuSkinningDesc desc{
        .bind_positions =
            {
                mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
            },
        .skin = skin,
        .palette = mirakana::AnimationSkinningPalette{{
            mirakana::Mat4::translation(mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F}),
            mirakana::Mat4::translation(mirakana::Vec3{.x = -2.0F, .y = 4.0F, .z = 0.0F}) *
                mirakana::Mat4::rotation_z(1.57079637F),
        }},
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };
    desc.bind_normals = {
        mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
    };

    const auto diagnostics = mirakana::validate_animation_cpu_skinning_desc(desc);
    MK_REQUIRE(diagnostics.empty());

    const auto skinned = mirakana::skin_animation_vertices_cpu(desc);
    const auto replay = mirakana::skin_animation_vertices_cpu(desc);
    MK_REQUIRE(skinned.positions.size() == 3);
    MK_REQUIRE(skinned.normals.size() == 3);
    MK_REQUIRE(skinned.normals[0] == (mirakana::Vec3{0.0F, 1.0F, 0.0F}));
    MK_REQUIRE(std::abs(skinned.normals[1].x - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.normals[1].y - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.normals[1].z - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.normals[2].x - 0.31622776F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.normals[2].y - 0.94868332F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.normals[2].z - 0.0F) < 0.0001F);
    MK_REQUIRE(replay.normals[2] == skinned.normals[2]);
}

MK_TEST("animation cpu skinning reports normal stream diagnostics") {
    mirakana::AnimationCpuSkinningDesc valid{
        .bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
        .skin =
            mirakana::AnimationSkinPayloadDesc{
                .vertex_count = 1,
                .joints =
                    {
                        mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                         .inverse_bind_matrix = mirakana::Mat4::identity()},
                    },
                .vertices =
                    {
                        mirakana::AnimationSkinVertexWeights{
                            {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
                    },
            },
        .palette = mirakana::AnimationSkinningPalette{{mirakana::Mat4::identity()}},
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };
    valid.bind_normals = {mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}};

    auto invalid = valid;
    invalid.bind_normals.push_back(mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F});
    auto diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::normal_count_mismatch);

    invalid = valid;
    invalid.bind_normals[0].x = std::numeric_limits<float>::quiet_NaN();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_bind_normal);

    bool rejected = false;
    try {
        (void)mirakana::skin_animation_vertices_cpu(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("animation cpu skinning deforms optional bind tangents deterministically") {
    const mirakana::AnimationSkinPayloadDesc skin{
        .vertex_count = 3,
        .joints =
            {
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                 .inverse_bind_matrix = mirakana::Mat4::identity()},
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 1,
                                                 .inverse_bind_matrix = mirakana::Mat4::identity()},
            },
        .vertices =
            {
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 1.0F}}},
                mirakana::AnimationSkinVertexWeights{
                    {
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 3.0F},
                    },
                },
            },
    };
    mirakana::AnimationCpuSkinningDesc desc{
        .bind_positions =
            {
                mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
            },
        .skin = skin,
        .palette = mirakana::AnimationSkinningPalette{{
            mirakana::Mat4::translation(mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F}),
            mirakana::Mat4::translation(mirakana::Vec3{.x = -2.0F, .y = 4.0F, .z = 0.0F}) *
                mirakana::Mat4::rotation_z(1.57079637F),
        }},
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };
    desc.bind_normals = {
        mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
    };
    desc.bind_tangents = {
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
    };

    const auto diagnostics = mirakana::validate_animation_cpu_skinning_desc(desc);
    MK_REQUIRE(diagnostics.empty());

    const auto skinned = mirakana::skin_animation_vertices_cpu(desc);
    const auto replay = mirakana::skin_animation_vertices_cpu(desc);
    MK_REQUIRE(skinned.positions.size() == 3);
    MK_REQUIRE(skinned.normals.size() == 3);
    MK_REQUIRE(skinned.tangents.size() == 3);
    MK_REQUIRE(skinned.tangents[0] == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(std::abs(skinned.tangents[1].x - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.tangents[1].y - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.tangents[1].z - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.tangents[2].x - 0.31622776F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.tangents[2].y - 0.94868332F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.tangents[2].z - 0.0F) < 0.0001F);
    MK_REQUIRE(skinned.bitangents.empty());
    MK_REQUIRE(replay.tangents[2] == skinned.tangents[2]);
}

MK_TEST("animation cpu skinning reports tangent stream diagnostics") {
    mirakana::AnimationCpuSkinningDesc valid{
        .bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
        .skin =
            mirakana::AnimationSkinPayloadDesc{
                .vertex_count = 1,
                .joints =
                    {
                        mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                         .inverse_bind_matrix = mirakana::Mat4::identity()},
                    },
                .vertices =
                    {
                        mirakana::AnimationSkinVertexWeights{
                            {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
                    },
            },
        .palette = mirakana::AnimationSkinningPalette{{mirakana::Mat4::identity()}},
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };
    valid.bind_tangents = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}};

    auto invalid = valid;
    invalid.bind_tangents.push_back(mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F});
    auto diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::tangent_count_mismatch);

    invalid = valid;
    invalid.bind_tangents[0].x = std::numeric_limits<float>::quiet_NaN();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_bind_tangent);

    bool rejected = false;
    try {
        (void)mirakana::skin_animation_vertices_cpu(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("animation cpu skinning reconstructs optional bitangents from tangent handedness") {
    const mirakana::AnimationSkinPayloadDesc skin{
        .vertex_count = 3,
        .joints =
            {
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                 .inverse_bind_matrix = mirakana::Mat4::identity()},
                mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 1,
                                                 .inverse_bind_matrix = mirakana::Mat4::identity()},
            },
        .vertices =
            {
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
                mirakana::AnimationSkinVertexWeights{
                    {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 1.0F}}},
                mirakana::AnimationSkinVertexWeights{
                    {
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F},
                        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 1, .weight = 3.0F},
                    },
                },
            },
    };
    mirakana::AnimationCpuSkinningDesc desc{
        .bind_positions =
            {
                mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
            },
        .skin = skin,
        .palette = mirakana::AnimationSkinningPalette{{
            mirakana::Mat4::translation(mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F}),
            mirakana::Mat4::translation(mirakana::Vec3{.x = -2.0F, .y = 4.0F, .z = 0.0F}) *
                mirakana::Mat4::rotation_z(1.57079637F),
        }},
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };
    desc.bind_normals = {
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F},
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F},
        mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F},
    };
    desc.bind_tangents = {
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
        mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
    };
    desc.bind_tangent_handedness = {1.0F, -1.0F, 1.0F};

    const auto diagnostics = mirakana::validate_animation_cpu_skinning_desc(desc);
    MK_REQUIRE(diagnostics.empty());

    const auto skinned = mirakana::skin_animation_vertices_cpu(desc);
    const auto replay = mirakana::skin_animation_vertices_cpu(desc);
    MK_REQUIRE(skinned.normals.size() == 3);
    MK_REQUIRE(skinned.tangents.size() == 3);
    MK_REQUIRE(skinned.bitangents.size() == 3);
    MK_REQUIRE(std::abs(skinned.bitangents[0].x - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.bitangents[0].y - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.bitangents[0].z - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.bitangents[1].x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.bitangents[1].y - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.bitangents[1].z - 0.0F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.bitangents[2].x + 0.94868332F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.bitangents[2].y - 0.31622776F) < 0.0001F);
    MK_REQUIRE(std::abs(skinned.bitangents[2].z - 0.0F) < 0.0001F);
    MK_REQUIRE(replay.bitangents[2] == skinned.bitangents[2]);
}

MK_TEST("animation cpu skinning reports tangent handedness diagnostics") {
    mirakana::AnimationCpuSkinningDesc valid{
        .bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
        .skin =
            mirakana::AnimationSkinPayloadDesc{
                .vertex_count = 1,
                .joints =
                    {
                        mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                         .inverse_bind_matrix = mirakana::Mat4::identity()},
                    },
                .vertices =
                    {
                        mirakana::AnimationSkinVertexWeights{
                            {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
                    },
            },
        .palette = mirakana::AnimationSkinningPalette{{mirakana::Mat4::identity()}},
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };
    valid.bind_normals = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F}};
    valid.bind_tangents = {mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}};
    valid.bind_tangent_handedness = {1.0F};

    auto invalid = valid;
    invalid.bind_normals.clear();
    auto diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::tangent_handedness_missing_streams);

    invalid = valid;
    invalid.bind_tangents.clear();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::tangent_handedness_missing_streams);

    invalid = valid;
    invalid.bind_tangent_handedness.push_back(-1.0F);
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::tangent_handedness_count_mismatch);

    invalid = valid;
    invalid.bind_tangent_handedness[0] = 0.0F;
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_tangent_handedness);

    invalid = valid;
    invalid.bind_tangent_handedness[0] = std::numeric_limits<float>::quiet_NaN();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_tangent_handedness);

    bool rejected = false;
    try {
        (void)mirakana::skin_animation_vertices_cpu(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("animation cpu skinning reports deterministic diagnostics") {
    const mirakana::AnimationCpuSkinningDesc valid{
        .bind_positions = {mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}},
        .skin =
            mirakana::AnimationSkinPayloadDesc{
                .vertex_count = 1,
                .joints =
                    {
                        mirakana::AnimationSkinJointDesc{.skeleton_joint_index = 0,
                                                         .inverse_bind_matrix = mirakana::Mat4::identity()},
                    },
                .vertices =
                    {
                        mirakana::AnimationSkinVertexWeights{
                            {mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = 1.0F}}},
                    },
            },
        .palette = mirakana::AnimationSkinningPalette{{mirakana::Mat4::identity()}},
        .bind_normals = {},
        .bind_tangents = {},
        .bind_tangent_handedness = {},
    };

    auto invalid = valid;
    invalid.bind_positions.clear();
    auto diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::empty_bind_positions);

    invalid = valid;
    invalid.skin.vertex_count = 2;
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::vertex_count_mismatch);

    invalid = valid;
    invalid.palette.matrices.clear();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::palette_count_mismatch);

    invalid = valid;
    invalid.bind_positions[0].x = std::numeric_limits<float>::quiet_NaN();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_bind_position);

    invalid = valid;
    invalid.palette.matrices[0].at(0, 0) = std::numeric_limits<float>::quiet_NaN();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_palette_matrix);

    invalid = valid;
    invalid.skin.vertices[0].influences.clear();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_vertex_influence_count);

    invalid = valid;
    invalid.skin.vertices[0].influences[0].skin_joint_index = 8;
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_vertex_joint_index);
    MK_REQUIRE(diagnostics[0].vertex_index == 0);
    MK_REQUIRE(diagnostics[0].influence_index == 0);

    invalid = valid;
    invalid.skin.vertices[0].influences[0].weight = -1.0F;
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_vertex_weight);

    invalid = valid;
    invalid.skin.vertices[0].influences[0].weight = std::numeric_limits<float>::infinity();
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_vertex_weight);

    invalid = valid;
    invalid.skin.vertices[0].influences = {
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = std::numeric_limits<float>::max()},
        mirakana::AnimationSkinVertexInfluence{.skin_joint_index = 0, .weight = std::numeric_limits<float>::max()},
    };
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::invalid_vertex_weight);

    invalid = valid;
    invalid.skin.vertices[0].influences[0].weight = 0.0F;
    diagnostics = mirakana::validate_animation_cpu_skinning_desc(invalid);
    MK_REQUIRE(!diagnostics.empty());
    MK_REQUIRE(diagnostics[0].code == mirakana::AnimationCpuSkinningDiagnosticCode::zero_vertex_weight);

    bool rejected = false;
    try {
        (void)mirakana::skin_animation_vertices_cpu(invalid);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("animation state machine advances looping and clamped clips") {
    mirakana::AnimationStateMachine machine(mirakana::AnimationStateMachineDesc{
        .states =
            {
                mirakana::AnimationStateDesc{
                    .name = "idle",
                    .clip = mirakana::AnimationClipDesc{.name = "idle", .duration_seconds = 1.0F, .looping = false}},
                mirakana::AnimationStateDesc{
                    .name = "walk",
                    .clip = mirakana::AnimationClipDesc{.name = "walk", .duration_seconds = 2.0F, .looping = true}},
            },
        .initial_state = "idle",
        .transitions =
            {
                mirakana::AnimationTransitionDesc{
                    .from_state = "idle", .to_state = "walk", .trigger = "move", .blend_seconds = 0.0F},
            },
    });

    machine.update(2.0F);
    MK_REQUIRE(machine.sample().to_state == "idle");
    MK_REQUIRE(std::abs(machine.sample().to_time_seconds - 1.0F) < 0.0001F);

    MK_REQUIRE(machine.trigger("move"));
    machine.update(2.5F);

    const auto sample = machine.sample();
    MK_REQUIRE(sample.to_state == "walk");
    MK_REQUIRE(!sample.blending);
    MK_REQUIRE(std::abs(sample.to_time_seconds - 0.5F) < 0.0001F);
}

MK_TEST("animation state machine reports deterministic blend samples") {
    mirakana::AnimationStateMachine machine(mirakana::AnimationStateMachineDesc{
        .states =
            {
                mirakana::AnimationStateDesc{
                    .name = "idle",
                    .clip = mirakana::AnimationClipDesc{.name = "idle", .duration_seconds = 1.0F, .looping = true}},
                mirakana::AnimationStateDesc{
                    .name = "run",
                    .clip = mirakana::AnimationClipDesc{.name = "run", .duration_seconds = 0.5F, .looping = true}},
            },
        .initial_state = "idle",
        .transitions =
            {
                mirakana::AnimationTransitionDesc{
                    .from_state = "idle", .to_state = "run", .trigger = "run", .blend_seconds = 0.25F},
            },
    });

    machine.update(0.5F);
    MK_REQUIRE(machine.trigger("run"));
    auto sample = machine.sample();
    MK_REQUIRE(sample.blending);
    MK_REQUIRE(sample.from_state == "idle");
    MK_REQUIRE(sample.to_state == "run");
    MK_REQUIRE(std::abs(sample.blend_alpha) < 0.0001F);
    MK_REQUIRE(std::abs(sample.from_time_seconds - 0.5F) < 0.0001F);

    machine.update(0.125F);
    sample = machine.sample();
    MK_REQUIRE(sample.blending);
    MK_REQUIRE(std::abs(sample.blend_alpha - 0.5F) < 0.0001F);

    machine.update(0.125F);
    sample = machine.sample();
    MK_REQUIRE(!sample.blending);
    MK_REQUIRE(sample.to_state == "run");
    MK_REQUIRE(std::abs(sample.to_time_seconds - 0.25F) < 0.0001F);
}

MK_TEST("animation state machine rejects invalid graph descriptions") {
    MK_REQUIRE(!mirakana::is_valid_animation_state_machine_desc(mirakana::AnimationStateMachineDesc{
        {
            mirakana::AnimationStateDesc{"idle", mirakana::AnimationClipDesc{"idle", 0.0F, false}},
        },
        "idle",
        {},
    }));
    MK_REQUIRE(!mirakana::is_valid_animation_state_machine_desc(mirakana::AnimationStateMachineDesc{
        {
            mirakana::AnimationStateDesc{"idle", mirakana::AnimationClipDesc{"idle", 1.0F, false}},
        },
        "missing",
        {},
    }));
    MK_REQUIRE(!mirakana::is_valid_animation_state_machine_desc(mirakana::AnimationStateMachineDesc{
        {
            mirakana::AnimationStateDesc{"idle", mirakana::AnimationClipDesc{"idle", 1.0F, false}},
        },
        "idle",
        {
            mirakana::AnimationTransitionDesc{"idle", "missing", "go", 0.0F},
        },
    }));
}

MK_TEST("animation blend tree normalizes weighted clip samples deterministically") {
    const mirakana::AnimationBlendTreeDesc tree{
        {
            mirakana::AnimationBlendTreeChildDesc{
                .name = "idle_pose",
                .clip = mirakana::AnimationClipDesc{.name = "idle", .duration_seconds = 1.0F, .looping = false},
                .weight = 1.0F},
            mirakana::AnimationBlendTreeChildDesc{
                .name = "run_pose",
                .clip = mirakana::AnimationClipDesc{.name = "run", .duration_seconds = 2.0F, .looping = true},
                .weight = 3.0F},
        },
    };

    const auto samples = mirakana::sample_animation_blend_tree(tree, 2.5F);

    MK_REQUIRE(samples.size() == 2);
    MK_REQUIRE(samples[0].name == "idle_pose");
    MK_REQUIRE(samples[0].clip_name == "idle");
    MK_REQUIRE(samples[0].time_seconds == 1.0F);
    MK_REQUIRE(std::abs(samples[0].normalized_weight - 0.25F) < 0.0001F);
    MK_REQUIRE(samples[1].name == "run_pose");
    MK_REQUIRE(samples[1].time_seconds == 0.5F);
    MK_REQUIRE(std::abs(samples[1].normalized_weight - 0.75F) < 0.0001F);

    MK_REQUIRE(!mirakana::is_valid_animation_blend_tree_desc(mirakana::AnimationBlendTreeDesc{
        {mirakana::AnimationBlendTreeChildDesc{"bad", mirakana::AnimationClipDesc{"bad", 1.0F, false}, 0.0F}},
    }));
}

MK_TEST("animation blend graph samples nearest one dimensional clips deterministically") {
    const mirakana::AnimationBlend1DDesc blend{
        .parameter_name = "speed",
        .children =
            {
                mirakana::AnimationBlend1DChildDesc{
                    .name = "idle_pose",
                    .clip = mirakana::AnimationClipDesc{.name = "idle", .duration_seconds = 1.0F, .looping = false},
                    .threshold = 0.0F},
                mirakana::AnimationBlend1DChildDesc{
                    .name = "walk_pose",
                    .clip = mirakana::AnimationClipDesc{.name = "walk", .duration_seconds = 2.0F, .looping = false},
                    .threshold = 1.0F},
                mirakana::AnimationBlend1DChildDesc{
                    .name = "run_pose",
                    .clip = mirakana::AnimationClipDesc{.name = "run", .duration_seconds = 1.0F, .looping = true},
                    .threshold = 4.0F},
            },
    };

    const auto samples = mirakana::sample_animation_blend_1d(blend, 2.5F, 5.0F);

    MK_REQUIRE(samples.size() == 2);
    MK_REQUIRE(samples[0].name == "walk_pose");
    MK_REQUIRE(samples[0].clip_name == "walk");
    MK_REQUIRE(samples[0].time_seconds == 2.0F);
    MK_REQUIRE(std::abs(samples[0].normalized_weight - 0.5F) < 0.0001F);
    MK_REQUIRE(samples[1].name == "run_pose");
    MK_REQUIRE(samples[1].time_seconds == 0.0F);
    MK_REQUIRE(std::abs(samples[1].normalized_weight - 0.5F) < 0.0001F);

    const auto clamped = mirakana::sample_animation_blend_1d(blend, -10.0F, 0.5F);
    MK_REQUIRE(clamped.size() == 1);
    MK_REQUIRE(clamped[0].name == "idle_pose");

    MK_REQUIRE(!mirakana::is_valid_animation_blend_1d_desc(mirakana::AnimationBlend1DDesc{
        "speed",
        {
            mirakana::AnimationBlend1DChildDesc{"run", mirakana::AnimationClipDesc{"run", 1.0F, true}, 4.0F},
            mirakana::AnimationBlend1DChildDesc{"walk", mirakana::AnimationClipDesc{"walk", 1.0F, true}, 1.0F},
        },
    }));
}

MK_TEST("animation layered samples normalize layer weights and preserve additive flags") {
    const std::vector<mirakana::AnimationLayerDesc> layers{
        mirakana::AnimationLayerDesc{
            .name = "base",
            .blend_tree =
                mirakana::AnimationBlendTreeDesc{
                    {mirakana::AnimationBlendTreeChildDesc{
                        .name = "walk",
                        .clip = mirakana::AnimationClipDesc{.name = "walk", .duration_seconds = 1.0F, .looping = true},
                        .weight = 1.0F}},
                },
            .weight = 3.0F,
            .additive = false,
        },
        mirakana::AnimationLayerDesc{
            .name = "upper",
            .blend_tree =
                mirakana::AnimationBlendTreeDesc{
                    {mirakana::AnimationBlendTreeChildDesc{
                        .name = "aim",
                        .clip = mirakana::AnimationClipDesc{.name = "aim", .duration_seconds = 2.0F, .looping = false},
                        .weight = 1.0F}},
                },
            .weight = 1.0F,
            .additive = true,
        },
    };

    const auto samples = mirakana::sample_animation_layers(layers, 1.25F);

    MK_REQUIRE(samples.size() == 2);
    MK_REQUIRE(samples[0].name == "base");
    MK_REQUIRE(!samples[0].additive);
    MK_REQUIRE(std::abs(samples[0].normalized_weight - 0.75F) < 0.0001F);
    MK_REQUIRE(samples[0].clips[0].time_seconds == 0.25F);
    MK_REQUIRE(samples[1].name == "upper");
    MK_REQUIRE(samples[1].additive);
    MK_REQUIRE(std::abs(samples[1].normalized_weight - 0.25F) < 0.0001F);
    MK_REQUIRE(samples[1].clips[0].time_seconds == 1.25F);
}

MK_TEST("animation float curve bindings sample sorted targets") {
    const std::vector<mirakana::FloatAnimationTrack> tracks{
        mirakana::FloatAnimationTrack{.target = "weapon.recoil",
                                      .keyframes = {mirakana::FloatKeyframe{.time_seconds = 0.0F, .value = 0.0F},
                                                    mirakana::FloatKeyframe{.time_seconds = 1.0F, .value = 4.0F}}},
        mirakana::FloatAnimationTrack{.target = "camera.fov",
                                      .keyframes = {mirakana::FloatKeyframe{.time_seconds = 0.0F, .value = 60.0F},
                                                    mirakana::FloatKeyframe{.time_seconds = 1.0F, .value = 90.0F}}},
    };

    const auto samples = mirakana::sample_float_animation_tracks(tracks, 0.5F);

    MK_REQUIRE(samples.size() == 2);
    MK_REQUIRE(samples[0].target == "camera.fov");
    MK_REQUIRE(std::abs(samples[0].value - 75.0F) < 0.0001F);
    MK_REQUIRE(samples[1].target == "weapon.recoil");
    MK_REQUIRE(std::abs(samples[1].value - 2.0F) < 0.0001F);
}

MK_TEST("animation float clip byte sources convert to sampled tracks") {
    const auto append_f32 = [](std::vector<std::uint8_t>& bytes, float value) {
        std::uint32_t bits = 0U;
        std::memcpy(&bits, &value, sizeof(float));
        bytes.push_back(static_cast<std::uint8_t>(bits & 0xFFU));
        bytes.push_back(static_cast<std::uint8_t>((bits >> 8U) & 0xFFU));
        bytes.push_back(static_cast<std::uint8_t>((bits >> 16U) & 0xFFU));
        bytes.push_back(static_cast<std::uint8_t>((bits >> 24U) & 0xFFU));
    };

    std::vector<std::uint8_t> times(8U);
    std::vector<std::uint8_t> recoil_values(8U);
    std::vector<std::uint8_t> fov_values(8U);
    const float t0 = 0.0F;
    const float t1 = 1.0F;
    const float recoil0 = 0.0F;
    const float recoil1 = 4.0F;
    const float fov0 = 60.0F;
    const float fov1 = 90.0F;
    times.clear();
    recoil_values.clear();
    fov_values.clear();
    append_f32(times, t0);
    append_f32(times, t1);
    append_f32(recoil_values, recoil0);
    append_f32(recoil_values, recoil1);
    append_f32(fov_values, fov0);
    append_f32(fov_values, fov1);

    const std::vector<mirakana::FloatAnimationTrackByteSource> sources{
        mirakana::FloatAnimationTrackByteSource{
            .target = "weapon.recoil", .time_seconds_bytes = times, .value_bytes = recoil_values},
        mirakana::FloatAnimationTrackByteSource{
            .target = "camera=fov", .time_seconds_bytes = times, .value_bytes = fov_values},
    };

    const auto tracks = mirakana::make_float_animation_tracks_from_f32_bytes(sources);
    const auto samples = mirakana::sample_float_animation_tracks(tracks, 0.5F);

    MK_REQUIRE(samples.size() == 2);
    MK_REQUIRE(samples[0].target == "camera=fov");
    MK_REQUIRE(std::abs(samples[0].value - 75.0F) < 0.0001F);
    MK_REQUIRE(samples[1].target == "weapon.recoil");
    MK_REQUIRE(std::abs(samples[1].value - 2.0F) < 0.0001F);
}

MK_TEST("animation quaternion clip byte sources convert to sampled 3D joint tracks") {
    auto append_f32 = [](std::vector<std::uint8_t>& bytes, float value) {
        std::uint32_t bits = 0U;
        std::memcpy(&bits, &value, sizeof(float));
        bytes.push_back(static_cast<std::uint8_t>(bits & 0xFFU));
        bytes.push_back(static_cast<std::uint8_t>((bits >> 8U) & 0xFFU));
        bytes.push_back(static_cast<std::uint8_t>((bits >> 16U) & 0xFFU));
        bytes.push_back(static_cast<std::uint8_t>((bits >> 24U) & 0xFFU));
    };

    std::vector<std::uint8_t> times;
    append_f32(times, 0.0F);
    append_f32(times, 1.0F);

    std::vector<std::uint8_t> translations;
    append_f32(translations, 0.0F);
    append_f32(translations, 0.0F);
    append_f32(translations, 0.0F);
    append_f32(translations, 2.0F);
    append_f32(translations, 4.0F);
    append_f32(translations, 6.0F);

    std::vector<std::uint8_t> rotations;
    append_f32(rotations, 0.0F);
    append_f32(rotations, 0.0F);
    append_f32(rotations, 0.0F);
    append_f32(rotations, 1.0F);
    append_f32(rotations, 0.0F);
    append_f32(rotations, 0.0F);
    append_f32(rotations, 0.70710677F);
    append_f32(rotations, 0.70710677F);

    std::vector<std::uint8_t> scales;
    append_f32(scales, 1.0F);
    append_f32(scales, 1.0F);
    append_f32(scales, 1.0F);
    append_f32(scales, 2.0F);
    append_f32(scales, 3.0F);
    append_f32(scales, 4.0F);

    const std::vector<mirakana::AnimationJointTrack3dByteSource> sources{
        mirakana::AnimationJointTrack3dByteSource{
            .joint_index = 0U,
            .target = "joint/root",
            .translation_time_seconds_bytes = times,
            .translation_xyz_bytes = translations,
            .rotation_time_seconds_bytes = times,
            .rotation_xyzw_bytes = rotations,
            .scale_time_seconds_bytes = times,
            .scale_xyz_bytes = scales,
        },
    };

    const auto tracks = mirakana::make_animation_joint_tracks_3d_from_f32_bytes(sources);
    const mirakana::AnimationSkeleton3dDesc skeleton{
        {mirakana::AnimationSkeleton3dJointDesc{
            .name = "root", .parent_index = mirakana::animation_no_parent, .rest = {}}},
    };
    const auto pose = mirakana::sample_animation_local_pose_3d(skeleton, tracks, 0.5F);

    MK_REQUIRE(pose.joints.size() == 1U);
    MK_REQUIRE(std::abs(pose.joints[0].translation.x - 1.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].translation.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].translation.z - 3.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].rotation.z - 0.38268343F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].rotation.w - 0.9238795F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].scale.x - 1.5F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].scale.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(pose.joints[0].scale.z - 2.5F) < 0.0001F);
}

MK_TEST("animation retarget policy evaluates deterministic binding scales") {
    const mirakana::AnimationRetargetPolicyDesc policy{
        {
            mirakana::AnimationRetargetBindingDesc{
                .source = "hips",
                .target = "pelvis",
                .mode = mirakana::AnimationRetargetMode::scale_to_target,
                .source_reference_length = 1.0F,
                .target_reference_length = 2.0F,
            },
            mirakana::AnimationRetargetBindingDesc{
                .source = "hand_l",
                .target = "left_hand",
                .mode = mirakana::AnimationRetargetMode::preserve_source,
                .source_reference_length = 1.0F,
                .target_reference_length = 3.0F,
            },
        },
    };

    const auto samples = mirakana::evaluate_animation_retarget_policy(policy);

    MK_REQUIRE(samples.size() == 2);
    MK_REQUIRE(samples[0].source == "hips");
    MK_REQUIRE(samples[0].target == "pelvis");
    MK_REQUIRE(samples[0].mode == mirakana::AnimationRetargetMode::scale_to_target);
    MK_REQUIRE(std::abs(samples[0].scale - 2.0F) < 0.0001F);
    MK_REQUIRE(samples[1].source == "hand_l");
    MK_REQUIRE(samples[1].target == "left_hand");
    MK_REQUIRE(samples[1].scale == 1.0F);

    MK_REQUIRE(!mirakana::is_valid_animation_retarget_policy_desc(mirakana::AnimationRetargetPolicyDesc{
        {mirakana::AnimationRetargetBindingDesc{"hips", "pelvis", mirakana::AnimationRetargetMode::scale_to_target,
                                                0.0F, 2.0F}},
    }));
}

MK_TEST("animation timeline fires non looping events once and clamps time") {
    mirakana::AnimationTimelinePlayback timeline(mirakana::AnimationTimelineDesc{
        .duration_seconds = 1.0F,
        .looping = false,
        .events =
            {
                mirakana::AnimationTimelineEventDesc{.time_seconds = 0.25F, .name = "footstep", .payload = "left"},
                mirakana::AnimationTimelineEventDesc{.time_seconds = 0.75F, .name = "impact", .payload = "ground"},
            },
    });

    auto events = timeline.update(0.5F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].name == "footstep");
    MK_REQUIRE(events[0].payload == "left");
    MK_REQUIRE(events[0].loop_index == 0);
    MK_REQUIRE(std::abs(timeline.time_seconds() - 0.5F) < 0.0001F);
    MK_REQUIRE(!timeline.finished());

    events = timeline.update(1.0F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].name == "impact");
    MK_REQUIRE(events[0].payload == "ground");
    MK_REQUIRE(std::abs(timeline.time_seconds() - 1.0F) < 0.0001F);
    MK_REQUIRE(timeline.finished());

    MK_REQUIRE(timeline.update(0.25F).empty());
}

MK_TEST("animation timeline fires looping events across wrap") {
    mirakana::AnimationTimelinePlayback timeline(mirakana::AnimationTimelineDesc{
        .duration_seconds = 1.0F,
        .looping = true,
        .events =
            {
                mirakana::AnimationTimelineEventDesc{.time_seconds = 0.25F, .name = "step", .payload = "left"},
                mirakana::AnimationTimelineEventDesc{.time_seconds = 0.75F, .name = "step", .payload = "right"},
            },
    });

    auto events = timeline.update(0.8F);
    MK_REQUIRE(events.size() == 2);
    MK_REQUIRE(events[0].payload == "left");
    MK_REQUIRE(events[1].payload == "right");
    MK_REQUIRE(events[0].loop_index == 0);
    MK_REQUIRE(events[1].loop_index == 0);

    events = timeline.update(0.5F);
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0].payload == "left");
    MK_REQUIRE(events[0].loop_index == 1);
    MK_REQUIRE(std::abs(timeline.time_seconds() - 0.3F) < 0.0001F);
    MK_REQUIRE(timeline.loop_index() == 1);
    MK_REQUIRE(!timeline.finished());
}

MK_TEST("animation authored event tracks flatten deterministic timeline events") {
    const auto timeline = mirakana::build_animation_timeline_from_tracks(mirakana::AnimationAuthoredTimelineDesc{
        .duration_seconds = 1.0F,
        .looping = false,
        .tracks =
            {
                mirakana::AnimationTimelineEventTrackDesc{
                    .name = "gameplay",
                    .events = {mirakana::AnimationTimelineEventDesc{
                        .time_seconds = 0.25F, .name = "spawn", .payload = "enemy"}},
                },
                mirakana::AnimationTimelineEventTrackDesc{
                    .name = "audio",
                    .events = {mirakana::AnimationTimelineEventDesc{
                        .time_seconds = 0.25F, .name = "play", .payload = "step"}},
                },
            },
    });

    MK_REQUIRE(timeline.events.size() == 2);
    MK_REQUIRE(timeline.events[0].track == "audio");
    MK_REQUIRE(timeline.events[0].name == "play");
    MK_REQUIRE(timeline.events[1].track == "gameplay");
    MK_REQUIRE(timeline.events[1].name == "spawn");

    mirakana::AnimationTimelinePlayback playback(timeline);
    const auto events = playback.update(0.5F);
    MK_REQUIRE(events.size() == 2);
    MK_REQUIRE(events[0].track == "audio");
    MK_REQUIRE(events[1].track == "gameplay");

    MK_REQUIRE(!mirakana::is_valid_animation_authored_timeline_desc(mirakana::AnimationAuthoredTimelineDesc{
        1.0F,
        false,
        {mirakana::AnimationTimelineEventTrackDesc{"bad=track",
                                                   {mirakana::AnimationTimelineEventDesc{0.25F, "spawn", ""}}}},
    }));
}

MK_TEST("animation timeline rejects invalid descriptions and deltas") {
    MK_REQUIRE(!mirakana::is_valid_animation_timeline_desc(mirakana::AnimationTimelineDesc{
        0.0F,
        false,
        {},
    }));
    MK_REQUIRE(!mirakana::is_valid_animation_timeline_desc(mirakana::AnimationTimelineDesc{
        1.0F,
        false,
        {
            mirakana::AnimationTimelineEventDesc{0.8F, "late", ""},
            mirakana::AnimationTimelineEventDesc{0.2F, "early", ""},
        },
    }));
    MK_REQUIRE(!mirakana::is_valid_animation_timeline_desc(mirakana::AnimationTimelineDesc{
        1.0F,
        false,
        {
            mirakana::AnimationTimelineEventDesc{1.5F, "outside", ""},
        },
    }));

    mirakana::AnimationTimelinePlayback timeline(
        mirakana::AnimationTimelineDesc{.duration_seconds = 1.0F, .looping = false, .events = {}});
    bool rejected_delta = false;
    try {
        (void)timeline.update(-0.01F);
    } catch (const std::invalid_argument&) {
        rejected_delta = true;
    }
    MK_REQUIRE(rejected_delta);
}

MK_TEST("scene deserialization rejects unsupported or incomplete text") {
    bool rejected_format = false;
    try {
        (void)mirakana::deserialize_scene("format=GameEngine.Scene.v0\nscene.name=old\nnode.count=0\n");
    } catch (const std::invalid_argument&) {
        rejected_format = true;
    }

    bool rejected_parent = false;
    try {
        (void)mirakana::deserialize_scene("format=GameEngine.Scene.v1\n"
                                          "scene.name=broken\n"
                                          "node.count=1\n"
                                          "node.1.name=Child\n"
                                          "node.1.parent=42\n"
                                          "node.1.position=0,0,0\n"
                                          "node.1.scale=1,1,1\n");
    } catch (const std::invalid_argument&) {
        rejected_parent = true;
    }

    MK_REQUIRE(rejected_format);
    MK_REQUIRE(rejected_parent);
}

int main() {
    return mirakana::test::run_all();
}
