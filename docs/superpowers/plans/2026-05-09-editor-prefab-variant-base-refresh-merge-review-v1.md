# Editor Prefab Variant Base Refresh Merge Review Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a GUI-independent reviewed base-prefab refresh surface for prefab variants so editor authors can retarget existing overrides by stable `source_node_name` hints before accepting a refreshed embedded base prefab.

**Architecture:** Extend `mirakana_editor_core` prefab variant authoring with a planning/result/action/UI model that never executes tools, mutates files, loads dynamic game modules, or depends on Dear ImGui, SDL3, renderer, RHI, package scripts, or runtime host APIs. Treat base refresh as an explicit reviewed merge step: preserve/retarget only deterministic source-name matches, block ambiguous or unsafe mappings, and leave full nested prefab propagation outside the ready claim.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_scene` prefab variants, `mirakana_ui` retained rows, `mirakana_editor_core_tests`, docs/manifest/static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

**Plan ID:** `editor-prefab-variant-base-refresh-merge-review-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Gap:** `editor-productization` focused child slice.  
**Previous Slice:** [2026-05-09-physics-1-0-collision-system-closeout-v1.md](2026-05-09-physics-1-0-collision-system-closeout-v1.md)

## Context

- The active production-completion loop has selected `editor-productization` after closing the first-party Physics 1.0 gap.
- Existing prefab variant work covers conflict review, missing-node cleanup, unique source-node retarget, source-node mismatch retarget, accept-current hint repair, reviewed batch resolution, and native `.prefabvariant` dialogs.
- The manifest still lists nested prefab propagation/merge resolution UX as unsupported beyond those reviewed rows. A refreshed embedded base-prefab review is the next host-independent, testable editor-core slice that narrows that gap without claiming full nested prefab instances, fuzzy matching, automatic propagation, or broad Unity/UE-like UX.

## Constraints

- Keep the implementation in `editor/core`; do not add GUI, renderer/RHI, SDL3, native handle, package, process, runtime-host, or filesystem execution.
- Require explicit review before mutation. Planning and UI model construction must be read-only; only the apply result and undo action may mutate a copied/document variant.
- Use deterministic source-node names only. Missing, ambiguous, invalid, source-less, or duplicate-target mappings must block refresh instead of guessing.
- Keep strict `mirakana_scene` composition index-based and update indices only as part of the reviewed base-refresh apply.
- Do not mark full editor productization or nested prefab propagation ready in this slice.

## Done When

- `mirakana_editor_core` exposes `PrefabVariantBaseRefreshPlan`, typed base-refresh status/row/result APIs, a retained `prefab_variant_base_refresh` UI model, and an undoable document action.
- Tests prove source-name retargeting, read-only UI rows, blocked missing hints, ambiguous/missing/source-less/duplicate target mappings, invalid refreshed base handling, explicit apply, and undo.
- `docs/editor.md`, current capability/roadmap docs, Codex/Claude editor skills, `engine/agent/manifest.json`, and static checks describe the new narrow reviewed surface and remaining unsupported limits.
- Focused editor-core build/test and relevant static checks pass, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete host/tool blocker.

## Task 1: RED Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add a helper to find `PrefabVariantBaseRefreshRow` rows by id.
- [x] Add a failing test for retargeting name/transform overrides when a refreshed base prefab inserts nodes and source names uniquely map to new indices.
- [x] Add failing tests for blocked ambiguous, missing, source-less, invalid-base, and duplicate-target mappings.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and confirm the tests fail because the new API is missing.

## Task 2: Editor-Core API And Implementation

**Files:**
- Modify: `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp`
- Modify: `editor/core/src/prefab_variant_authoring.cpp`

- [x] Add base-refresh status, row kind, row, plan, and result structs.
- [x] Add label helpers, `plan_prefab_variant_base_refresh`, `apply_prefab_variant_base_refresh`, `make_prefab_variant_base_refresh_action`, and `make_prefab_variant_base_refresh_ui_model`.
- [x] Reuse existing prefab variant validation, source-node lookup, duplicate override key, snapshot undo, retained UI helper, and sanitization patterns.
- [x] Keep planning side-effect-free and retained UI execution-free; report the available reviewed apply surface as mutating but non-executing with `mutates=true`, `executes=false`.

## Task 3: Documentation, Manifest, And Static Checks

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`

- [x] Record the new reviewed base-refresh merge surface and retained ids.
- [x] Keep `editor-productization` partly-ready with full nested prefab propagation/merge UX, dynamic module loading, Vulkan/Metal preview display parity, resource execution, automatic host-gated AI execution, and Unity/UE-like UX still unsupported.
- [x] Update static checks so manifest/docs/skills cannot drift from the new contract.

## Task 4: Validation And Closeout

- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `git diff --check --` for touched files.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence, mark this plan completed, and move `currentActivePlan` back to the master plan for the next `editor-productization` slice.

## Self-Review

- The feature is a reviewed base-refresh merge helper, not automatic nested prefab propagation.
- Every unsafe mapping is blocked rather than guessed.
- The API stays editor-core and GUI-independent.
- Ready claims remain scoped to evidence from tests, docs, manifest, and static checks.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_editor_core_tests` | Expected RED before implementation; PASS after implementation | RED failed on the missing `PrefabVariantBaseRefresh*` API during test-first setup; final focused build completed `mirakana_editor_core_tests.exe`. |
| `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` | PASS | `mirakana_editor_core_tests` passed, 1/1 test, 0 failures. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after manifest, plan registry, docs, and static contract updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok`. |
| `git diff --check -- ...` | PASS | Targeted touched-file whitespace check exited 0; Git reported only line-ending normalization warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; production-readiness audit reports 11 remaining unsupported gaps with `editor-productization` still `partly-ready`; full CTest passed 29/29, 0 failures. |
