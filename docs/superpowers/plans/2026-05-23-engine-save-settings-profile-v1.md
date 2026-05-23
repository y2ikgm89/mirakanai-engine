# Engine Save Settings Profile v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the existing runtime save/settings/profile foundation with backend-neutral save-slot, progression, and package-resume evidence, then prove it through runtime tests and 2D/3D package smoke counters.

**Plan ID:** `engine-save-settings-profile-v1`

**Status:** Local validation complete; PR/hosted checks pending.

**Gap:** `engine-save-settings-profile-v1`

**Architecture:** Reuse the existing `RuntimeSaveData`, `RuntimeSettings`, `RuntimeInputRebindingProfile`, `RuntimeSessionProfilePathPlan`, and `RuntimeSessionProfileDocumentLoadResult` contracts. Add a first-party value planner that reviews already-loaded profile documents for required save slot, progression checkpoint, package id, and profile id coherence. Keep platform user-directory resolution, cloud saves, binary saves, filesystem mutation, and migrations outside the public API; unsupported/corrupt document rows remain fail-closed migration diagnostics.

**Tech Stack:** C++23, CMake `MK_runtime`, repository `tools/*.ps1` validation entrypoints, `game.agent.json` package smoke recipes, composed engine agent manifest fragments.

---

## Classification

Large: this changes public C++ runtime API, generated/sample package smoke behavior, package-visible counters, docs, manifest fragments, and static checks. It requires subagent design/final review, TDD red/green, focused C++ build/test/static checks, and full `tools/validate.ps1`.

## Goal / Context / Constraints / Done When

**Goal:** Let generated games prove a loaded profile can resume a selected package with deterministic save-slot, progression, settings, and input-profile evidence before claiming broad generated-game save support.

**Context:** `MK_runtime` already has versioned save/settings/input-profile documents, project-relative profile path policy, and bundle load/write diagnostics. The remaining gap is package-visible resume evidence tying loaded save state to a selected package and progression checkpoint.

**Constraints:**
- Clean breaking greenfield implementation; no compatibility shim, deprecated alias, migration layer, or duplicate old API.
- Public API stays first-party and backend-neutral.
- `engine/agent/manifest.json` is never edited directly; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/*.ps1`; no `bun run`.
- Keep task-owned changes only in this worktree.

**Done when:**
- Runtime tests prove ready resume rows, missing/mismatched save-slot/progression/package/profile diagnostics, and unsupported/corrupt document blocking.
- 2D and 3D package samples report resume counters under a selected requirement flag.
- Docs, manifest fragments, and static checks name the new contract and counters while preserving non-goals.
- Focused build/test/static lanes and final `tools/validate.ps1` pass.
- PR hosted checks pass, PR merges, main fast-forwards, and the worktree is removed with `tools/remove-merged-worktree.ps1`.

## Task 1: Runtime API Red Test

**Files:**
- Modify: `tests/unit/runtime_tests.cpp`

- [x] **Step 1: Write failing tests**

Add tests for `plan_runtime_session_profile_resume` that assert:
- ready resume evidence from loaded profile documents exposes save slot, progression checkpoint, package id, schema versions, row counts, and no diagnostics;
- missing or mismatched save-slot/progression/package/profile evidence fails closed;
- corrupt or unsupported-version document rows from `load_runtime_session_profile_documents` block resume before package claims.

- [x] **Step 2: Verify red**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests
```

Expected: build fails because the new resume API symbols are missing.

## Task 2: Runtime API Green

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/session_services.hpp`
- Modify: `engine/runtime/src/session_services.cpp`

- [x] **Step 1: Add public value types**

Add `RuntimeSessionProfileResumeRequest`, `RuntimeSessionProfileResumePlan`, `RuntimeSessionProfileResumeDiagnostic`, and supporting enums.

- [x] **Step 2: Add planner**

Implement `plan_runtime_session_profile_resume` over an existing `RuntimeSessionProfileDocumentLoadResult` without filesystem mutation. It should fail closed for blocking document rows, missing required keys, package mismatch, and profile mismatch.

- [x] **Step 3: Verify green**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests
```

Expected: target builds and runtime tests pass.

## Task 3: Package Resume Counters

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: sample package READMEs and manifests as needed

- [x] **Step 1: Add selected package smoke flag**

Add a requirement flag that writes/loads in-memory runtime profile documents, calls the new planner, and emits deterministic `runtime_profile_resume_*` counters.

- [x] **Step 2: Focused sample verification**

Run targeted builds and source-tree smoke commands for the 2D and 3D package sample targets with the new flag.

## Task 4: Agent Surfaces And Closeout

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/004-modules.json` and/or `014-gameCodeGuidance.json` if durable guidance changes
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`

- [x] **Step 1: Sync durable guidance**

Record the new runtime resume contract, package counters, clean non-goals, and validation evidence.

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
| Runtime red | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests` | Passed preflight/configure; target build failed as expected before implementation because `RuntimeSessionProfileResume*` and `plan_runtime_session_profile_resume` were missing. |
| Runtime green | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests` | Passed. |
| Package samples | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package`; source-tree package smokes with `--require-runtime-profile-resume` | Passed with `runtime_profile_resume_ready=1`, `runtime_profile_resume_loaded_documents=3`, `runtime_profile_resume_defaulted_documents=0`, `runtime_profile_resume_save_schema_version=3`, and `runtime_profile_resume_settings_schema_version=2` for both samples. |
| Review regression red | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Failed as expected before review fixes: defaulted documents were incorrectly resumable, and sample code did not require exact resume counter values. |
| Review regression green | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; 2D/3D source-tree package smokes with `--require-runtime-profile-resume` | Passed after making `defaulted_missing` rows block package resume and requiring `loaded_documents=3`, `defaulted_documents=0`, `save_schema_version=3`, and `settings_schema_version=2` in selected package smokes. |
| Public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| Format/tidy | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files 'engine/runtime/src/session_services.cpp,games/sample_2d_desktop_runtime_package/main.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp,tests/unit/runtime_tests.cpp'` | Passed after applying `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| Agent surfaces | `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1` | Passed. |
| Full gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed after review fixes; `validate: ok`, including 74/74 CTest tests. |
