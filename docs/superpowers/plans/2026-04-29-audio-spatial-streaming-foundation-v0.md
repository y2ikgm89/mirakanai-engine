# Audio Spatial/Streaming Foundation v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend `mirakana_audio` with deterministic first-party spatial voice planning and streaming-queue contracts so gameplay can model positional audio and streamed asset delivery without depending on platform audio APIs, codecs, SDL3, Android AAudio, editor code, or third-party middleware.

**Architecture:** Keep this slice in `mirakana_audio` as C++23 value-type contracts and deterministic mixer planning. Reuse existing clip, voice, bus, resampling, streaming buffer availability, underrun diagnostic, and interleaved float rendering concepts. This slice adds host-independent spatial attenuation/pan planning and queue-state validation only; codec decode, device hotplug, HRTF, DSP graphs, mixer authoring UI, platform device selection, and middleware remain follow-up work.

**Tech Stack:** C++23, `mirakana_audio`, existing unit tests, no new third-party dependencies.

---

## Context

- `mirakana_audio` already has deterministic mixer planning, clip metadata, sample-frame scheduling, streaming buffer availability intent, underrun diagnostics, selectable nearest/linear resampling, and interleaved float PCM rendering.
- Production audio still lacks streaming decode architecture, spatial audio model, DSP graph, mixer authoring, device hotplug/selection, and richer backend QA.
- A host-independent spatial/streaming contract is feasible on the current Windows host without codec or device dependencies.

## Constraints

- Do not add third-party dependencies, codecs, or middleware.
- Do not add platform, SDL3, Android AAudio, editor, renderer, RHI, OS, or native-handle dependencies to `mirakana_audio`.
- Do not implement actual streaming decode, HRTF, occlusion, reverb, DSP graph execution, device hotplug, device selection, mixer UI, or platform output in this slice.
- Keep public API under `mirakana::`.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_audio` exposes deterministic spatial voice/listener value types and mixer planning output for distance attenuation and stereo pan metadata.
- [x] `mirakana_audio` exposes deterministic streaming queue/request state contracts that validate queued decoded chunks, cursor advancement, starvation/underrun state, and stable diagnostics without decoding source formats.
- [x] Tests cover spatial attenuation/pan edge cases, disabled spatial behavior, invalid listener/source values, streaming queue append/consume/starvation behavior, deterministic replay, and existing mixer output compatibility.
- [x] Docs, roadmap, gap analysis, manifest, and Codex/Claude guidance describe this as Audio Spatial/Streaming Foundation v0, not codecs, HRTF, DSP graphs, platform device management, or mixer authoring.
- [ ] Focused audio tests, API boundary, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Audio Spatial/Streaming Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp` or the existing audio test file if one exists.
- Inspect: `engine/audio/include/mirakana/audio/`
- Inspect: `engine/audio/src/`

- [x] Add tests for deterministic spatial voice planning from listener/source data.
- [x] Add tests for invalid spatial values and non-spatial fallback behavior.
- [x] Add tests for streaming queue append/consume/starvation contracts without codecs.
- [x] Verify the new tests fail for missing APIs before implementation.

### Task 2: Spatial Audio Planning Contracts

**Files:**
- Modify under `engine/audio/include/mirakana/audio/`
- Modify under `engine/audio/src/`

- [x] Add value-type listener/source/spatial planning structs.
- [x] Implement finite-input validation, distance attenuation, stereo pan metadata, and deterministic ordering.
- [x] Keep behavior channel-count/device independent and mixer-owned.

### Task 3: Streaming Queue Contracts

**Files:**
- Modify under `engine/audio/include/mirakana/audio/`
- Modify under `engine/audio/src/`

- [x] Add decoded-chunk queue descriptors/results that do not decode source formats.
- [x] Implement append/consume cursor behavior, starvation/underrun reporting, and stable diagnostics.
- [x] Preserve deterministic replay and compatibility with existing mixer render planning.

### Task 4: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/architecture.md` if subsystem capability text changes.
- Modify: `docs/testing.md` if test coverage text changes.
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] Mark spatial/streaming contracts honestly as implemented only after tests and validation pass.
- [x] Keep codec decode, HRTF, DSP graph, device hotplug/selection, and mixer authoring as follow-up work.

### Task 5: Verification

- [x] Run focused audio tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED test evidence: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` failed before implementation because the new spatial and streaming queue APIs did not exist.
- Focused audio validation: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "mirakana_core_tests"` passed after implementation and reviewer fixes.
- cpp-reviewer findings addressed: disabled spatial descriptors now ignore unused spatial fields, finite validated inputs cannot produce non-finite spatial commands without rejection, streaming underrun warning thresholds use saturating addition, and spatial/streaming edge plus replay tests were added.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: initially failed after code edits; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format; retry PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS, strict analysis diagnostic-only because `out/build/dev/compile_commands.json` is missing for the Visual Studio generator.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Diagnostic-only host gates remain Metal `metal` / `metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database missing.
