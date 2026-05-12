# Validation Recipe Runner Tooling v1 Implementation Plan (2026-05-01)

> **For agentic workers:** Use this as the next focused C-phase slice after `scene-package-apply-tooling-v1`. Do not append this work to completed package, scene, material, UI atlas, desktop runtime, or validation infrastructure plans.

**Goal:** Add a reviewed validation-recipe runner surface that lets AI agents dry-run and execute only manifest-declared validation recipes with host-gate diagnostics and no free-form shell input.

**Architecture:** Keep recipe definitions in `engine/agent/manifest.json` and add a narrow repository tool wrapper that resolves recipe ids to reviewed command plans. The runner must use explicit allowlisted command builders instead of evaluating arbitrary manifest command strings, and it must keep validation execution separate from editor productization, package mutation, runtime source import, renderer/RHI residency, package streaming, native handles, and broad build orchestration.

**Tech Stack:** PowerShell 7 validation entrypoints under `tools/`, `engine/agent/manifest.json`, schema/static checks, existing package/runtime validation scripts, optional focused C++ tests only if a C++ helper is introduced.

---

## Goal

Make validation execution AI-operable through a narrow reviewed path:

- dry-run a validation recipe by id and report the exact command plan that would run
- execute only known manifest-declared validation recipes through reviewed command builders
- support recipe-specific arguments only where already documented, such as selected desktop runtime game targets and strict Vulkan sample package smoke args
- reject unknown recipe ids, stale command strings, unsafe game targets, unsupported free-form arguments, and missing host-gate acknowledgements
- keep command stdout/stderr/exit-code reporting deterministic enough for agents to decide the next debugging step
- keep `run-validation-recipe` honest: it runs validation, not arbitrary shell, package mutation design, shader generation, renderer residency, or editor productization

## Context

- `engine/agent/manifest.json.aiOperableProductionLoop.commandSurfaces.run-validation-recipe` is still planned/blocked.
- `engine/agent/manifest.json.validationRecipes` already lists default, agent/schema, API boundary, desktop runtime, selected package, shader toolchain, dependency, asset importer, release package, installed SDK, mobile, and iOS smoke recipes.
- Current agents can run repository commands directly, but the command surface contract is descriptor-only and does not provide a reviewed recipe resolver/result shape.
- Existing scripts already own the risky work:
  - `tools/test.ps1`
  - `tools/check-ai-integration.ps1`
  - `tools/check-json-contracts.ps1`
  - `tools/check-public-api-boundaries.ps1`
  - `tools/check-shader-toolchain.ps1`
  - `tools/validate-desktop-game-runtime.ps1`
  - `tools/package-desktop-runtime.ps1`
  - `tools/validate-installed-sdk.ps1`
  - `tools/mobile-check.ps1`
- Package validation may hit the known sandbox/vcpkg `CreateFileW stdin failed with 5` blocker; the runner must report that as a host/sandbox blocker rather than hiding it.

## Constraints

- Do not add third-party dependencies in this slice.
- Do not evaluate arbitrary shell text from JSON or user input.
- Do not expose a general process runner to gameplay-facing C++ APIs.
- Do not make `mirakana_scene`, `mirakana_assets`, or `mirakana_ui` depend on renderer, RHI, platform, SDL3, Dear ImGui, or backend APIs.
- Do not create a broad build dashboard, CI service, retry daemon, or editor UX.
- Do not change validation semantics owned by existing scripts unless a failing RED check proves the current contract is wrong.
- Keep Metal, Apple packaging, Android signing/device smoke, and tidy compile database availability as honest host gates.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A reviewed `tools/run-validation-recipe.ps1` dry-run/execute path exists for an explicit initial allowlist of recipe ids.
- `engine/agent/manifest.json` promotes only the validated `run-validation-recipe` surface and lists unsupported free-form shell/build orchestration claims.
- Static checks reject stale recipe ids, non-allowlisted runner mappings, free-form command execution, and docs that imply broad validation automation.
- Existing scene/material/UI package apply surfaces and desktop package proofs still pass.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers change, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, focused runner tests, selected package validation where relevant, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory And Runner Contract

**Files:**
- Read: `engine/agent/manifest.json`
- Read: `tools/agent-context.ps1`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `tools/common.ps1`
- Read: `tools/package-desktop-runtime.ps1`
- Read: `tools/validate-desktop-game-runtime.ps1`
- Read: `package.json`
- Read: `docs/testing.md`
- Read: `games/sample_desktop_runtime_game/game.agent.json`

- [x] Decide the initial allowlist for the first ready runner surface:
  - `agent-contract`
  - `default`
  - `public-api-boundary`
  - `shader-toolchain`
  - `desktop-game-runtime`
  - `desktop-runtime-sample-game-scene-gpu-package`
  - `desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package`
- [x] Define the runner request fields: `mode`, `validationRecipe`, optional `gameTarget`, optional `strictBackend`, optional `hostGateAcknowledgements`, optional `timeoutSeconds`, and rejected remaining/free-form arguments.
- [x] Define dry-run result fields: `recipe`, `status`, `command`, `argv`, `hostGates`, `diagnostics`, and `blockedBy`.
- [x] Define execute result fields: `recipe`, `status`, `exitCode`, `durationSeconds`, `stdoutSummary`, `stderrSummary`, `hostGates`, and `diagnostics`.
- [x] Record explicit non-goals: arbitrary shell execution, arbitrary manifest command eval, editor UX, CI service, broad package cooking, shader graph/live shader generation, renderer/RHI residency, package streaming, native handle exposure, and Metal ready claims.

### Task 2: RED Checks And Tests

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Create or modify: focused PowerShell tests under the existing validation script test pattern
- Modify: `docs/superpowers/plans/2026-05-01-validation-recipe-runner-tooling-v1.md`

- [x] Add failing static checks requiring a ready `run-validation-recipe` command surface with dry-run and execute modes.
- [x] Add failing static checks requiring the runner script path and helper names in manifest notes.
- [x] Add failing static checks rejecting docs or manifest notes that imply arbitrary shell execution or free-form validation commands.
- [x] Add failing focused tests for dry-run output for `agent-contract`, `default`, `shader-toolchain`, and both selected sample package recipes.
- [x] Add failing focused tests for unknown recipe id rejection.
- [x] Add failing focused tests for unsafe `gameTarget` values and unsupported free-form argument rejection.
- [x] Add failing focused tests for missing host-gate acknowledgement where a selected recipe requires D3D12, strict Vulkan, Metal, Android, iOS, or tidy availability.
- [x] Record RED evidence in this plan before production implementation.

### Task 3: Reviewed Runner Implementation

**Files:**
- Create: `tools/run-validation-recipe.ps1`
- Modify: `tools/common.ps1` only for small shared helpers if duplication becomes concrete
- Modify: `package.json` only if adding a repository entrypoint is necessary

- [x] Implement a typed recipe allowlist in `tools/run-validation-recipe.ps1`; do not evaluate raw manifest `command` strings.
- [x] Implement `-Mode DryRun` to return the exact argv plan without launching commands.
- [x] Implement `-Mode Execute` to run only the selected reviewed command builder.
- [x] Implement strict recipe-specific package command builders for `sample_desktop_runtime_game` D3D12 and strict Vulkan UI atlas metadata package validation.
- [x] Validate `gameTarget` against manifest registered game targets or the known selected package target before building argv.
- [x] Reject unknown recipes, unsupported `gameTarget`, unsafe path-like target strings, extra free-form args, and missing host-gate acknowledgement.
- [x] Capture exit code and bounded stdout/stderr summaries without hiding the full underlying command output from normal script execution.
- [x] Preserve existing validation scripts as the source of truth for actual build/test/package behavior.

### Task 4: Manifest, Docs, And Agent Context Sync

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` only if top-level output needs a new runner field
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the reviewed `run-validation-recipe` subset to ready.
- [x] Keep validation recipes outside the allowlist planned or host-gated.
- [x] Keep the manifest command surface explicit that this is not arbitrary shell execution, CI orchestration, package mutation, shader graph/live shader generation, renderer/RHI residency, package streaming, native handle exposure, or Metal readiness.
- [x] Update docs so agents use the runner for known recipes and still use direct repository commands only when a recipe is not yet runner-owned.
- [x] Keep `currentActivePlan` and `recommendedNextPlan` synchronized with the plan registry.

### Task 5: Validation

**Files:**
- Modify: this plan's Validation Evidence section
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/roadmap.md`

- [x] Run focused runner dry-run tests.
- [x] Run focused runner execute tests for a cheap recipe such as `agent-contract`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers changed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run the D3D12 selected package command if the runner allowlist includes it.
- [x] Run the strict Vulkan selected package command if the runner allowlist includes it and local Vulkan/SPIR-V gates are ready.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected after adding static checks. Failure: `run-validation-recipe` result shape is still missing `executeFields`.
- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after adding static checks. Failure: `run-validation-recipe` result shape is still missing `executeFields`.
- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` failed as expected. Failure: `Missing validation recipe runner: tools/run-validation-recipe.ps1`.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` passed. Covered dry-run command plans for `agent-contract`, `default`, `shader-toolchain`, `desktop-runtime-sample-game-scene-gpu-package`, and `desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package`; execute mode for `agent-contract`; unknown recipe, unsafe `gameTarget`, unsupported free-form argument, and missing host-gate acknowledgement rejection.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after manifest/docs/static-check synchronization.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed. No public C++ headers were changed in this slice.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only with `d3d12_dxil=ready`, `vulkan_spirv=ready`, `dxc_spirv_codegen=ready`, and Metal tools still missing as an explicit host gate.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed with 14/14 CTest tests.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed with installed SDK validation, selected D3D12 package smoke, and CPack ZIP generation.
- GREEN, 2026-05-01: strict Vulkan selected package command passed with `renderer=vulkan`, `scene_gpu_status=ready`, `postprocess_depth_input_ready=1`, `directional_shadow_ready=1`, `ui_overlay_status=ready`, `ui_atlas_metadata_status=ready`, and `ui_texture_overlay_status=ready`.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 28/28 default CTest tests. Known diagnostic-only host gates remain Metal tools, Apple packaging/Xcode, Android signing/device smoke, and tidy compile database availability.
