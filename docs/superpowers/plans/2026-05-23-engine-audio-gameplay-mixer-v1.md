# Engine Audio Gameplay Mixer v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the existing first-party audio gameplay mix planner into selected package-visible generated-game audio evidence over cooked package audio payloads with deterministic bus, cue, fade, pause, loop, trigger, spatial metadata, render, and sample/payload diagnostic counters.

**Plan ID:** `engine-audio-gameplay-mixer-v1`

**Status:** Local validation complete; PR hosted checks and merge pending.

**Gap:** `engine-audio-gameplay-mixer-v1`

**Architecture:** Keep gameplay audio as a first-party value contract in `MK_audio`. Package samples may plan mixer-ready `AudioVoiceDesc` rows and render deterministic device-independent PCM evidence, but native device ownership, codec decoding breadth, HRTF, DSP graphs, middleware, SDL3 device types, and platform handles stay outside the public game-facing API.

**Tech Stack:** C++23, `MK_audio`, selected 2D/3D sample package smokes, `game.agent.json` validation recipes, repository `tools/*.ps1` validation, composed engine agent manifest fragments.

---

## Classification

Large: this changes C++ runtime package sample behavior, validation recipes, docs, manifest fragments, and static checks for a production capability. It requires TDD/static red, focused sample builds and smokes, final diff review, and full `tools/validate.ps1`.

## Goal / Context / Constraints / Done When

**Goal:** Let generated-game package recipes require deterministic gameplay audio mix evidence before claiming the selected audio gameplay mixer foundation.

**Context:** `MK_audio` already exposes `AudioGameplayMixRequest`, bus/cue/trigger descriptors, `AudioGameplayMixPlan`, `plan_gameplay_audio_mix`, `AudioMixer`, and device-stream planning. Current docs show source-tree and gameplay-systems audio evidence, but the developer-owned backlog still asks for selected package counters, device-independent render smoke, and cooked package audio sample diagnostics before marking `engine-audio-gameplay-mixer-v1` implemented.

**Constraints:**
- Clean breaking greenfield implementation; no compatibility shim, deprecated alias, duplicate API, or migration layer.
- Public game-facing contract stays first-party and backend-neutral; no native device handles, SDL3, renderer/RHI, or middleware types leak into public audio APIs.
- Use existing cooked package data and deterministic in-memory sample rows; do not introduce dependencies or broad codec/device readiness claims.
- `engine/agent/manifest.json` is never edited directly; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/*.ps1`; no `bun run`.
- Keep task-owned changes only in this worktree.

**Done when:**
- Static guards fail first for missing dedicated package audio gameplay mixer flags, counters, validation recipes, and docs.
- 2D and 3D package samples expose `--require-audio-gameplay-mixer` with deterministic `audio_gameplay_mixer_*` counters.
- The selected 2D and 3D samples derive audio gameplay mixer evidence from cooked package audio records; both samples prove mixer-ready commands and device-independent render output without native device claims.
- Docs, master backlog, manifest fragments, sample manifests, and static checks record supported evidence and unsupported boundaries.
- Focused build/test/static lanes and final `tools/validate.ps1` pass.
- PR hosted checks pass, PR merges, main fast-forwards, and the worktree is removed with `tools/remove-merged-worktree.ps1`.

## Task 1: Static And Test Red

**Files:**
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify as needed: `tests/unit/core_tests.cpp`

- [x] **Step 1: Add failing static guards**

Require both selected package samples to expose `--require-audio-gameplay-mixer`, `audio_gameplay_mixer_ready`, deterministic bus/cue/trigger/command/render/sample diagnostics counters, cooked package audio payload evidence, and dedicated validation recipe ids.

- [x] **Step 2: Verify red**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

## Task 2: Package Audio Gameplay Mixer Evidence

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`

- [x] **Step 1: Add selected package smoke flag**

Add `--require-audio-gameplay-mixer` and package-visible counters for planning success, diagnostics, bus rows, cue rows, trigger rows, command rows, paused buses, faded buses, looping commands, spatial commands, render commands, render frames, rendered samples, sample absolute sum, and sample/payload diagnostics.

- [x] **Step 2: Focused sample verification**

Build the selected package sample targets and run source-tree smokes with `--require-audio-gameplay-mixer`.

## Task 3: Agent Surfaces And Closeout

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: selected sample `README.md` / `game.agent.json`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`

- [x] **Step 1: Sync durable guidance**

Record the supported package counters, validation recipes, clean-breaking decision, and unsupported native device/codec/middleware boundaries.

- [x] **Step 2: Compose and validate agent surfaces**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

## Validation Evidence

| Check | Command | Result |
| --- | --- | --- |
| Static red | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Failed as expected on missing `--require-audio-gameplay-mixer` package evidence before implementation. |
| Package samples | Targeted 2D/3D builds and source-tree smokes with `--require-audio-gameplay-mixer` | Passed for `sample_2d_desktop_runtime_package` and `sample_generated_desktop_runtime_3d_package`: `audio_gameplay_mixer_ready=1`, diagnostics/payload diagnostics `0`, bus/cue/trigger/command rows `2`, paused/faded/looping/spatial counters `1`, render frames/samples `2`; 3D package audio also reported `gameplay_systems_audio_first_sample=0.3`, `gameplay_systems_audio_second_sample=0.4`, and `gameplay_systems_audio_abs_sum=0.7` from the cooked payload. |
| Public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| Agent surfaces | `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1` | Passed. |
| Format/tidy | `tools/check-format.ps1`; targeted `tools/check-tidy.ps1 -Files ...` | Passed. |
| Full gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed: static checks passed, build succeeded, CTest reported 74/74 passed. Apple/Metal checks remained host-gated diagnostic-only on Windows. |
