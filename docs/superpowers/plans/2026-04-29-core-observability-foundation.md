# Core Observability Foundation Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free `MK_core` diagnostics/profiling recorder that engine modules can share without introducing runtime, renderer, platform, editor, SDL3, or native API dependencies.

**Architecture:** Keep the shared observability contract in `MK_core`. The first slice records value-based diagnostic events, counter samples, and profile zones with bounded storage, manual clocks for deterministic tests, and a steady-clock adapter for host/editor callers. Renderer, runtime host, GPU markers, trace export, and `MK_core` recorder-backed editor profiler panels remain follow-up adapters.

**Tech Stack:** C++23, standard library containers and `std::chrono::steady_clock`, existing `MK_core` target, existing unit test executable `MK_core_tests`, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Context

- `MK_core` currently owns `ILogger`, app lifecycle, fixed timestep, registry, and version.
- `MK_runtime` has package/session-specific `RuntimeDiagnosticReport`, but `MK_renderer` and `MK_runtime_host` should not depend on `MK_runtime`.
- `MK_renderer` already exposes `RendererStats`; later slices can bridge those stats into counters.
- `DesktopGameRunner` is a natural later integration point for frame/update/event-pump profile zones.

## Constraints

- Keep public API in `mirakana::`.
- Keep `MK_core` standard-library-only.
- Do not add global mutable state.
- Do not expose platform clocks, OS handles, GPU handles, renderer handles, SDL3, Dear ImGui, or editor APIs.
- Do not add third-party dependencies.
- Keep this slice deterministic and unit-testable.

## Done When

- [x] `DiagnosticsRecorder` records bounded diagnostic events, counters, and profile samples.
- [x] `ManualProfileClock` supports deterministic tests and `SteadyProfileClock` exposes a production timestamp adapter without platform APIs.
- [x] `ScopedProfileZone` records nested scope duration through RAII.
- [x] `DiagnosticSummary` aggregates event/counter/profile counts, warning/error counts, and min/max/total profile duration.
- [x] Docs, roadmap, manifest, skills, and subagent guidance describe the new observability foundation honestly.
- [x] Focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or host/toolchain blockers are recorded.

---

### Task 1: Core Observability Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] **Step 1: Add failing bounded capture and summary test**

Add `#include "mirakana/core/diagnostics.hpp"` and a test that records events, counters, and profile samples through a bounded recorder. The landed test records three events, counters, and profile samples so oldest-entry eviction is verified for all three streams.

```cpp
MK_TEST("diagnostics recorder keeps bounded events counters and profile samples") {
    mirakana::DiagnosticsRecorder recorder(2);

    recorder.record_event(mirakana::DiagnosticEvent{
        mirakana::DiagnosticSeverity::info,
        "runtime",
        "startup",
        1,
    });
    recorder.record_event(mirakana::DiagnosticEvent{
        mirakana::DiagnosticSeverity::warning,
        "runtime",
        "slow frame",
        2,
    });
    recorder.record_event(mirakana::DiagnosticEvent{
        mirakana::DiagnosticSeverity::error,
        "runtime",
        "missing asset",
        3,
    });
    recorder.record_counter(mirakana::CounterSample{"entities.active", 42.0, 3});
    recorder.record_profile_sample(mirakana::ProfileSample{"game.update", 3, 100, 50, 0});

    const auto capture = recorder.snapshot();
    MK_REQUIRE(capture.events.size() == 2);
    MK_REQUIRE(capture.events[0].message == "slow frame");
    MK_REQUIRE(capture.events[1].message == "missing asset");
    MK_REQUIRE(capture.counters.size() == 1);
    MK_REQUIRE(capture.counters[0].value == 42.0);
    MK_REQUIRE(capture.profiles.size() == 1);
    MK_REQUIRE(capture.profiles[0].duration_ns == 50);

    const auto summary = mirakana::summarize_diagnostics(capture);
    MK_REQUIRE(summary.event_count == 2);
    MK_REQUIRE(summary.warning_count == 1);
    MK_REQUIRE(summary.error_count == 1);
    MK_REQUIRE(summary.counter_count == 1);
    MK_REQUIRE(summary.profile_count == 1);
    MK_REQUIRE(summary.total_profile_time_ns == 50);
    MK_REQUIRE(summary.min_profile_time_ns == 50);
    MK_REQUIRE(summary.max_profile_time_ns == 50);
}
```

- [x] **Step 2: Add failing manual clock and RAII scope test**

Add a test that uses a manual clock and `ScopedProfileZone` to record nested profile depths:

```cpp
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
```

- [x] **Step 3: Verify red**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1
```

Initial RED evidence: failed with MSVC `C1083` because `mirakana/core/diagnostics.hpp` did not exist before implementation.

- [x] **Step 4: Add invalid recorder input coverage**

Review hardening added coverage for invalid labels and non-finite counters. The recorder rejects invalid user records by recording bounded warning diagnostic events in the `diagnostics` category.

---

### Task 2: Core Observability Implementation

**Files:**
- Create: `engine/core/include/mirakana/core/diagnostics.hpp`
- Create: `engine/core/src/diagnostics.cpp`
- Modify: `engine/core/CMakeLists.txt`

- [x] **Step 1: Add public header**

Define severity labels, event/counter/profile/capture/summary value types, `ManualProfileClock`, `SteadyProfileClock`, `DiagnosticsRecorder`, and `ScopedProfileZone`.

- [x] **Step 2: Add implementation**

Implement bounded oldest-entry eviction, invalid-name filtering, summary aggregation, manual clock advancement, steady-clock timestamp conversion, nested depth tracking, and RAII scope closing.

- [x] **Step 3: Register source file**

Add `src/diagnostics.cpp` to `MK_core` in `engine/core/CMakeLists.txt`.

- [x] **Step 4: Verify green**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1
```

Expected: PASS.

---

### Task 3: Documentation And Agent Guidance

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.agents/skills/gameengine-agent-integration/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Synchronize docs and AI contracts**

Document that `MK_core` owns dependency-free diagnostics/counter/profile capture. Make clear that runtime-host frame instrumentation, `InstrumentedRenderer`, GPU markers, trace export, allocator hooks, telemetry, crash reports, and `MK_core` recorder-backed editor profiler panels remain follow-up adapters. The existing editor Profiler panel is a lightweight status/debug counter panel, not this recorder-backed adapter.

- [x] **Step 2: Verify docs/contracts**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: PASS.

---

### Task 4: Final Validation

**Files:**
- No new implementation files.

- [x] **Step 1: Run focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: PASS.

- [x] **Step 2: Run final validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: PASS. Host-gated Metal/Apple/mobile diagnostics remain diagnostic-only where applicable.

---

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: PASS, 18/18 CTest tests passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS; analysis blocked by missing `out/build/dev/compile_commands.json` for the active Visual Studio generator.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS.
