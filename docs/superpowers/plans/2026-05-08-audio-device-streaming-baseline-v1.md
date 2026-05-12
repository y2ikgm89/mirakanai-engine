# Audio Device Streaming Baseline v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `audio-device-streaming-baseline-v1`  
**Status:** Completed.  
**Goal:** Add a first-party audio device stream planning and render pump contract that keeps output queues filled with deterministic PCM without exposing SDL3, AAudio, codec, or OS handles to game code.

**Architecture:** Keep the new contract in `mirakana_audio` beside `AudioMixer` because it is device-independent queue-fill policy over existing clip/render metadata. Optional device adapters such as `mirakana_audio_sdl3` continue to own actual OS streams and only consume the generated interleaved float buffer.

**Tech Stack:** C++23, `mirakana_audio`, optional `mirakana_audio_sdl3` tests, existing CMake `dev` preset.

---

## Context

- Master plan row: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md` lists `audio-device-streaming-baseline-v1` under runtime-system minimums.
- Current `mirakana_audio` already validates buses, voices, clip metadata, streaming queue append/consume state, render requests, underrun diagnostics, nearest/linear resampling, channel conversion, and deterministic interleaved float PCM.
- Current `mirakana_audio_sdl3` already owns SDL3 runtime/device lifetime, queued-frame reporting, silence queuing, float PCM queuing, pause/resume, and clear.

## Constraints

- Do not add codec integration, HRTF, DSP graphs, device hotplug/selection, mixer authoring, streaming decode threads, callbacks, SDL3/AAudio handles, native handles, or new dependencies.
- Do not move OS audio device ownership into `mirakana_audio`.
- Keep public API names in `mirakana::`, use value types, and preserve deterministic behavior.
- Use TDD before production code.

## Done When

- `mirakana_audio` exposes `AudioDeviceStreamRequest`, `AudioDeviceStreamPlan`, status/diagnostic enums, `plan_audio_device_stream`, and `render_audio_device_stream_interleaved_float`.
- The planner validates device format, queue target, render budget, resampling quality, and device-frame overflow; reports `no_work` when the existing queue already satisfies the target; and schedules render frames from `device_frame + queued_frames`.
- The render helper uses the planner plus `AudioMixer::render_interleaved_float` to return deterministic PCM only for ready plans.
- Unit tests prove bounded queue fill, no-work/invalid diagnostics, render start-frame alignment after already queued frames, and optional SDL3 dummy-device queuing through the new helper.
- Docs, plan registry, master plan, manifest, AI game-development guidance, and static checks describe the new boundary and keep codec/HRTF/DSP/hotplug/mixer-authoring claims unsupported.
- Focused audio tests, API boundary check, format check, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before committing.

## Files

- Modify: `engine/audio/include/mirakana/audio/audio_mixer.hpp`
- Modify: `engine/audio/src/audio_mixer.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_audio_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/architecture.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

## Tasks

### Task 1: Red Tests

- [x] Add `core_tests.cpp` tests for bounded queue fill, no-work diagnostics, invalid diagnostics, and render start-frame alignment from `device_frame + queued_frames`.
- [x] Update `sdl3_audio_tests.cpp` to queue rendered dummy-device PCM through `render_audio_device_stream_interleaved_float`.
- [x] Run focused audio test build and record expected compile failure before the new APIs exist.

### Task 2: Audio Device Stream Contract

- [x] Add public device stream request/result/status/diagnostic rows in `audio_mixer.hpp`.
- [x] Implement `plan_audio_device_stream` with finite queue-fill validation, bounded render-frame selection, no-work result, and overflow rejection.
- [x] Implement `render_audio_device_stream_interleaved_float` by calling the planner and `AudioMixer::render_interleaved_float` only for ready plans.

### Task 3: Documentation And Contract Checks

- [x] Update current-truth docs, AI guidance, game-development skills, manifest, master plan, and registry.
- [x] Update static checks to assert the new audio stream API names and unsupported boundaries.

### Task 4: Validation And Commit

- [x] Run focused audio build and CTest.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add audio device streaming baseline`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests mirakana_sdl3_audio_tests` | RED as expected | `mirakana_core_tests` failed to compile before the new public audio device stream APIs existed; the command did not reach SDL3 target generation. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests` | Passed | Focused `mirakana_audio` / core test target built after implementation. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R mirakana_core_tests` | Passed | `mirakana_core_tests` passed with the new audio device stream planner/render helper coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` | Passed | Validated the optional SDL3 dummy-device path using the new helper; `mirakana_sdl3_audio_tests` passed in the desktop-runtime preset. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest JSON remained schema-valid. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | AI integration checks accepted the audio stream contract and unsupported boundaries after `docs/testing.md` was tightened to include the new API names. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Unsupported production gap audit remained consistent with 11 non-ready rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public header boundary accepted the new `mirakana_audio` API. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting check passed after formatting `tests/unit/sdl3_audio_tests.cpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Default repository validation passed, including all 29 `dev` CTest tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Build gate passed before commit. |

## Status

**Status:** Completed.
