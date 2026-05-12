# Runtime Host Observability Bridge Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add host-owned frame instrumentation to `MK_runtime_host` so desktop game runs can emit `MK_core` profile samples and renderer counters without exposing SDL, OS, GPU, RHI backend, Dear ImGui, or editor APIs to game code.

**Architecture:** Keep the shared recorder and clock contracts in `MK_core`. `DesktopHostServices` gains optional first-party observability pointers, and `DesktopGameRunner` records a `runtime_host.frame` profile zone plus cumulative `IRenderer::stats()` counters when a recorder is supplied. Rendering remains behind `IRenderer`; this slice does not add GPU markers, trace export, telemetry, crash reports, allocator hooks, or editor profiler UI.

**Tech Stack:** C++23, existing `MK_core`, `MK_renderer`, `MK_runtime_host`, `MK_runtime_host_tests`, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Context

- `MK_core` already owns `DiagnosticsRecorder`, `CounterSample`, `ProfileSample`, `ManualProfileClock`, `SteadyProfileClock`, and `ScopedProfileZone`.
- `MK_renderer` already exposes backend-neutral `RendererStats` through `IRenderer::stats()`.
- `DesktopGameRunner` owns the desktop frame loop and already receives `IWindow`, virtual input/lifecycle, and optional `IRenderer` through first-party contracts.
- At the start of this slice, the production gap analysis listed runtime-host frame instrumentation and renderer counter bridges as the next observability adapter work.

## Constraints

- Keep public API names in `mirakana::`.
- Do not add third-party dependencies.
- Do not expose SDL3, OS window handles, D3D12/Vulkan/Metal handles, GPU handles, RHI backend internals, Dear ImGui, or editor APIs through game public APIs.
- Keep headless samples and existing desktop runtime behavior compatible.
- Add tests before implementation and verify the focused runtime-host test.

## Done When

- [x] `DesktopHostServices` can optionally accept a `DiagnosticsRecorder` and deterministic `IProfileClock` override.
- [x] `DesktopGameRunner` records one `runtime_host.frame` profile sample per loop iteration when diagnostics are supplied.
- [x] `DesktopGameRunner` records renderer counters from `IRenderer::stats()` after each updated frame when diagnostics and a renderer are supplied.
- [x] Runtime-host tests cover deterministic frame duration, frame index, and renderer counter labels/values.
- [x] Plan registry, roadmap, gap analysis, manifest, skills, and Codex/Claude subagent guidance describe the new capability honestly and keep GPU markers/trace/export/editor-profiler work as follow-up.
- [x] Focused validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host/toolchain blockers are recorded.

---

### Task 1: Runtime Host Observability Tests

**Files:**
- Modify: `tests/unit/runtime_host_tests.cpp`

- [x] **Step 1: Add failing runtime-host instrumentation test**

Add a test named `desktop game runner records frame diagnostics and renderer counters`. Use `DiagnosticsRecorder`, `ManualProfileClock`, a `NullRenderer`, and a `GameApp` that submits one sprite and one mesh during `on_update`. Expect a single `runtime_host.frame` profile sample with frame index `0`, deterministic duration, and four renderer counters:

```cpp
MK_REQUIRE(counter_named(capture.counters, "renderer.frames_started")->value == 1.0);
MK_REQUIRE(counter_named(capture.counters, "renderer.frames_finished")->value == 1.0);
MK_REQUIRE(counter_named(capture.counters, "renderer.sprites_submitted")->value == 1.0);
MK_REQUIRE(counter_named(capture.counters, "renderer.meshes_submitted")->value == 1.0);
```

- [x] **Step 2: Verify red**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_host_tests
ctest --preset dev --output-on-failure -R MK_runtime_host_tests
```

Expected: FAIL before implementation because `DesktopHostServices` has no diagnostics recorder/profile clock fields and the runner emits no samples.

Initial RED evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed while compiling `MK_runtime_host_tests` with MSVC `C7559` because `diagnostics_recorder` and `profile_clock` were not members of `mirakana::DesktopHostServices`.

### Task 2: Runtime Host Implementation

**Files:**
- Modify: `engine/runtime_host/include/mirakana/runtime_host/desktop_runner.hpp`
- Modify: `engine/runtime_host/src/desktop_runner.cpp`

- [x] **Step 1: Add optional observability services**

Add `#include "mirakana/core/diagnostics.hpp"` to the public host header and extend `DesktopHostServices` with:

```cpp
DiagnosticsRecorder* diagnostics_recorder{nullptr};
const IProfileClock* profile_clock{nullptr};
```

- [x] **Step 2: Record frame profile zones and renderer counters**

In `DesktopGameRunner::run`, create a local `SteadyProfileClock` fallback and wrap each loop iteration in `ScopedProfileZone` named `runtime_host.frame` when `diagnostics_recorder` is non-null. After `app.on_update`, record renderer stats counters for the current frame index when both `diagnostics_recorder` and `renderer` are non-null.

- [x] **Step 3: Verify green**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_host_tests
ctest --preset dev --output-on-failure -R MK_runtime_host_tests
```

Expected: PASS.

Green evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after implementation, including `MK_runtime_host_tests`.

### Task 3: Documentation And Agent Guidance

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.agents/skills/gameengine-agent-integration/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `.codex/agents/engine-architect.toml`
- Modify: `.claude/agents/engine-architect.md`

- [x] **Step 1: Synchronize capability claims**

Document that runtime-host frame instrumentation and renderer counter bridge are implemented through `DesktopHostServices` plus `MK_core` observability. Keep GPU markers, trace export, telemetry, crash reporting, allocator diagnostics, and recorder-backed editor profiler panels listed as follow-up work.

- [x] **Step 2: Verify docs/contracts**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: PASS.

### Task 4: Final Validation

**Files:**
- No additional implementation files.

- [x] **Step 1: Run focused and boundary checks**

Run:

```powershell
ctest --preset dev --output-on-failure -R MK_runtime_host_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: PASS.

- [x] **Step 2: Run final validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: PASS. Host-gated Metal/Apple/mobile diagnostics remain diagnostic-only where applicable.

## Validation Evidence

- `ctest --test-dir out\build\dev -C Debug --output-on-failure -R MK_runtime_host_tests`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: PASS, 18/18 CTest tests passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS on escalated rerun, 7/7 CTest tests passed. The first sandboxed run hit the known vcpkg 7zip extraction blocker: `CreateFileW stdin failed with 5 (Access is denied.)`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS.
