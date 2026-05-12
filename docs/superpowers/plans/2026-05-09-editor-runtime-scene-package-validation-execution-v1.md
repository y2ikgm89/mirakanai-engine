# Editor Runtime Scene Package Validation Execution Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let the editor package playtest loop execute the existing non-mutating `validate-runtime-scene-package` command surface and retain deterministic validation evidence before host-gated desktop smoke.

**Architecture:** Keep package cooking, package scripts, broad validation recipes, and host-gated desktop smoke outside this slice. `mirakana_editor_core` will convert the selected `EditorPlaytestPackageReviewModel` runtime scene validation target into a reviewed `RuntimeScenePackageValidationRequest`, execute only the first-party non-mutating validation helper through caller-supplied `IFileSystem`, and expose retained rows under `playtest_package_review.runtime_scene_validation`. The optional `mirakana_editor` shell may render and trigger that reviewed validation using its rooted project filesystem, while host-gated desktop smoke remains separate.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_tools`, `mirakana_editor`, first-party `mirakana_ui` retained models, CMake/CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Plan ID:** `editor-runtime-scene-package-validation-execution-v1`

**Status:** Completed.

---

## Context

- Active master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Selected gap: `editor-productization`.
- Previous slice: `docs/superpowers/plans/2026-05-09-editor-in-process-runtime-host-review-v1.md`.
- Existing editor package review orders `review-editor-package-candidates -> apply-reviewed-runtime-package-files -> select-runtime-scene-validation-target -> validate-runtime-scene-package -> run-host-gated-desktop-smoke`, but the visible editor does not yet produce first-party runtime scene package validation execution evidence directly from that review step.
- This slice narrows editor productization by adding the reviewed, non-mutating validation execution evidence needed before host-gated desktop smoke. It does not make editor productization ready by itself.

## Files

- Modify: `editor/core/include/mirakana/editor/playtest_package_review.hpp`
  - Add runtime scene package validation execution status, descriptor, model, result, label, execution helper, and retained UI model declarations.
- Modify: `editor/core/src/playtest_package_review.cpp`
  - Build a deterministic validation request from the selected package review target.
  - Execute `mirakana::execute_runtime_scene_package_validation` only when the review model is ready.
  - Retain status, summary, diagnostics, unsupported claims, and selected target rows under stable `playtest_package_review.runtime_scene_validation` ids.
- Modify: `tests/unit/editor_core_tests.cpp`
  - Add failing coverage for ready execution, retained UI rows, blocked pending package registration, validation failure evidence, and unsupported claim rejection.
- Modify: `editor/src/main.cpp`
  - Add visible `Validate Runtime Scene Package` evidence controls in the AI/package review area using the editor rooted project filesystem.
- Modify: `docs/editor.md`, `docs/current-capabilities.md`, `docs/testing.md`, `docs/roadmap.md`
  - Document the reviewed non-mutating validation execution and unsupported boundaries.
- Modify: `.agents/skills/editor-change/SKILL.md`, `.claude/skills/gameengine-editor/SKILL.md`
  - Keep Codex and Claude editor guidance aligned.
- Modify: `engine/agent/manifest.json`
  - Update `currentActivePlan`, `recommendedNextPlan`, and editor-productization capability text.
- Modify: `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`
  - Add sentinel checks for the reviewed validation execution contract.
- Modify: `docs/superpowers/plans/README.md`
  - Track this plan as the active slice, then latest completed evidence at closeout.
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update current verdict and selected-gap evidence after validation.

## Done When

- `mirakana_editor_core` exposes `EditorRuntimeScenePackageValidationExecutionDesc`, `EditorRuntimeScenePackageValidationExecutionModel`, `EditorRuntimeScenePackageValidationExecutionResult`, `make_editor_runtime_scene_package_validation_execution_model`, `execute_editor_runtime_scene_package_validation`, and `make_editor_runtime_scene_package_validation_execution_ui_model`.
- The model is executable only when `EditorPlaytestPackageReviewModel::ready_for_runtime_scene_validation` is true and the selected `RuntimeSceneValidationTargetRow` is present.
- The execution helper calls only `mirakana::execute_runtime_scene_package_validation` with a caller-supplied `IFileSystem` and records pass/fail/blocker evidence without mutating files.
- Unsupported claims for package cooking, runtime source parsing, external importer execution, renderer/RHI residency, package streaming, material graphs, shader graphs, live shader generation, broad editor productization, Metal readiness, public native/RHI handles, broad renderer quality, arbitrary shell, and free-form edits are rejected deterministically.
- The visible editor exposes `Validate Runtime Scene Package` only through the reviewed model and does not run package scripts, broad validation recipes, host-gated desktop smoke, arbitrary shell, or raw manifest commands through this path.
- Focused `mirakana_editor_core_tests` pass.
- GUI build passes because `editor/src/main.cpp` changes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `git diff --check -- <touched files>`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

### Task 1: Add failing editor-core coverage

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add a test named `editor runtime scene package validation execution records reviewed evidence`.
- [x] Build a ready `EditorPlaytestPackageReviewModel` with registered runtime package files and a selected runtime scene validation target.
- [x] Assert the execution model builds a safe non-mutating `RuntimeScenePackageValidationRequest`.
- [x] Assert successful execution over `MemoryFileSystem` records passed status, scene/package summary rows, and retained `playtest_package_review.runtime_scene_validation` UI rows.
- [x] Assert pending package registration blocks execution before filesystem reads.
- [x] Assert malformed or missing package data records failed evidence instead of claiming readiness.
- [x] Assert unsupported claims are rejected before execution.
- [x] Build `mirakana_editor_core_tests` and verify the test fails before implementation.

Run:

```powershell
cmake --build --preset dev --target mirakana_editor_core_tests
```

Expected before implementation: compile failure for the new runtime scene package validation execution types and functions.

### Task 2: Implement reviewed execution model

**Files:**
- Modify: `editor/core/include/mirakana/editor/playtest_package_review.hpp`
- Modify: `editor/core/src/playtest_package_review.cpp`

- [x] Add `EditorRuntimeScenePackageValidationExecutionStatus`.
- [x] Add `EditorRuntimeScenePackageValidationExecutionDesc` with the package review model and unsupported-claim flags.
- [x] Add `EditorRuntimeScenePackageValidationExecutionModel` with status, status label, selected target, request, summary counters, blocker rows, unsupported claims, and diagnostics.
- [x] Add `EditorRuntimeScenePackageValidationExecutionResult` with status, status label, model, validation result, evidence flags, and diagnostics.
- [x] Implement `make_editor_runtime_scene_package_validation_execution_model` so it converts only the selected `RuntimeSceneValidationTargetRow` into a `RuntimeScenePackageValidationRequest`.
- [x] Keep validation execution blocked when package registration is pending, no target is selected, target fields are invalid, or unsupported claims are requested.

### Task 3: Execute and retain validation evidence

**Files:**
- Modify: `editor/core/src/playtest_package_review.cpp`

- [x] Implement `execute_editor_runtime_scene_package_validation` over caller-supplied `IFileSystem`.
- [x] Call only `mirakana::execute_runtime_scene_package_validation`.
- [x] Mark successful validation as `passed`, validation diagnostics as `failed`, and preflight or filesystem exceptions as `blocked`.
- [x] Add `make_editor_runtime_scene_package_validation_execution_ui_model` with retained `playtest_package_review.runtime_scene_validation` ids for status, target, package index, scene asset key, package record count, scene node count, reference count, diagnostics, and unsupported claims.

### Task 4: Wire visible editor controls

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Add transient runtime scene package validation evidence state.
- [x] Render `Runtime Scene Package Validation` status near the AI/package review controls.
- [x] Expose `Validate Runtime Scene Package` only when the reviewed execution model is executable.
- [x] Execute through `tool_filesystem_` and show pass/fail/blocker summaries without running package scripts or desktop smoke.

### Task 5: Synchronize docs, manifest, skills, and checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [x] Document this slice as reviewed non-mutating runtime scene package validation execution evidence.
- [x] Keep host-gated desktop smoke, broad package scripts, arbitrary shell, raw manifest commands, package cooking, renderer/RHI residency, package streaming, Metal readiness, and broad editor productization unsupported.
- [x] Add sentinels for the new type names, retained row id, docs, skills, visible control text, and manifest wording.

### Task 6: Validate and close the slice

**Files:**
- Modify: this plan file.

- [x] Run focused editor-core build and tests.
- [x] Run GUI build with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run relevant static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact validation evidence in this plan.
- [x] Set this plan status to `Completed`, move `currentActivePlan` back to the master plan, and update the plan registry latest completed slice.

## Validation Evidence

- RED: `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation because the new test referenced missing runtime scene package validation execution model/result types and helpers.
- GREEN: `cmake --build --preset dev --target mirakana_editor_core_tests` passed after adding the reviewed execution model and non-mutating execution helper.
- GREEN: `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed, including the `desktop-gui` test lane.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after formatting the C++ edits.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding manifest/contentRoot drift guards.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after synchronizing docs, manifest, skills, and static sentinels.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It included `production-readiness-audit-check: ok` with `unsupported_gaps=11`, `editor-productization status=partly-ready`, full `ctest --preset dev` at 29/29 passing, `public-api-boundary-check: ok`, `tidy-check: ok (1 files)`, and diagnostic-only host gates for Metal/Apple tooling on this Windows host.

Discovered clean contract fix: current sample `.geindex` entries already use runtime-relative paths, so `contentRoot: "runtime"` would duplicate the runtime prefix during actual validation. This slice removes the duplicated manifest/template `contentRoot` values and adds static guards so future generated manifests omit `contentRoot` when cooked package entries already include the runtime-relative directory.
