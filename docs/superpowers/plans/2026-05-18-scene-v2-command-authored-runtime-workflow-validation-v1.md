# Scene v2 Command-Authored Runtime Workflow Validation v1

**Status:** Completed.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Narrow `scene-component-prefab-schema-v2` by proving reviewed Scene/Prefab v2 authoring commands can feed source asset registration, registered cook, Scene v2 runtime migration, and non-mutating runtime scene package validation in one host-independent workflow.

**Context:** The prior registered-asset workflow proof validated already-authored Scene v2 input through migration and runtime scene instantiation. The remaining gap still needed command-authored Scene/Prefab v2 evidence that starts from `plan_scene_prefab_authoring` / `apply_scene_prefab_authoring`, without widening into broad package cooking, renderer/RHI residency, editor productization, or public native handles.

**Constraints:**
- Keep this as a host-independent `MK_tools_tests` proof over reviewed helpers.
- Do not execute external importers, broad/dependent package cooking, renderer/RHI residency, package streaming, editor productization, or validation recipes.
- Preserve the `implemented-contract-only` status for `scene-component-prefab-schema-v2`.
- Keep `spot` lights unsupported in this migration slice; only map already-supported light fields into Scene v1.

**Done When:**
- A RED test fails because command-authored light cone fields cannot migrate into the runtime package.
- The migration bridge maps supported light cone fields and the new command-authored workflow test passes.
- Docs, manifest fragments, composed manifest, and static guards describe the validated workflow through `validate-runtime-scene-package`.
- Focused tests and full `tools/validate.ps1` pass for the coherent C++/tooling/public-contract slice.

---

### Task 1: RED Workflow Test

**Files:**
- Modify: `tests/unit/tools_tests.cpp`

- [x] Add a `MK_tools_tests` case that creates Scene/Prefab v2 source documents through `apply_scene_prefab_authoring`, registers selected texture/mesh/material sources, cooks selected registry rows, migrates Scene v2 to a runtime Scene v1 package, and validates it through `execute_runtime_scene_package_validation`.
- [x] Include a perspective camera, directional light with `inner_cone_radians` / `outer_cone_radians`, mesh renderer, and prefab instantiation so the test proves command-authored component rows feed the runtime package.
- [x] Verify RED with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_tools_tests$"` failing on the migration result.

### Task 2: Migration Support

**Files:**
- Modify: `engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp`
- Modify: `tests/unit/tools_tests.cpp`

- [x] Add supported light property allowlist coverage for `inner_cone_radians` and `outer_cone_radians`.
- [x] Map those values into the Scene v1 light payload.
- [x] Keep unsupported claim sentinels and `spot` light behavior fail-closed.
- [x] Verify GREEN with focused `MK_tools_tests`.

### Task 3: Docs, Manifest, And Static Guards

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-070-production-ledger.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`

- [x] Update durable guidance to name the validated workflow as `register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> validate-runtime-scene-package`.
- [x] Compose the manifest from fragments.
- [x] Run focused static checks and record evidence below.

---

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_tools_tests$"` failed as expected on the new migration result before light cone fields were mapped.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_tools_tests$"` passed.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after formatting.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/tools/scene/scene_v2_runtime_package_migration_tool.cpp,tests/unit/tools_tests.cpp` passed.
- Gate: full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed.




