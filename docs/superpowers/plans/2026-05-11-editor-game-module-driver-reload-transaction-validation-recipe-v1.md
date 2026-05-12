# Editor Game Module Driver Reload Transaction Validation Recipe v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-reload-transaction-validation-recipe-v1`  
**Gap:** `editor-productization` (hot reload + stable ABI stream — stopped-state reload evidence)  
**Parent stream:** [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md)  
**Status:** Completed (see validation table)

## Goal

Promote the existing Windows-only `MK_editor_game_module_driver_load_tests` CTest lane to the reviewed **`run-validation-recipe`** allowlist so operators and AI workflows can **dry-run** or **execute** the same `cmake --preset dev` + targeted build + `ctest -R MK_editor_game_module_driver_load_tests` plan behind an explicit **`windows-msvc-dev-editor-game-module-driver-ctest`** host-gate acknowledgement, aligning the hot-reload stream “validation recipes that prove safe reload” direction without claiming active-session hot reload, `mirakana_editor` automation, or stable third-party ABI.

## Context

- Load tests and DLL barrier/session policy UI already land in `MK_editor_core_tests` and `MK_editor_game_module_driver_load_tests`; this slice wires **repository-wide** recipe metadata and the runner argv builder only.
- Global manifest validation recipes live in [engine/agent/manifest.fragments/009-validationRecipes.json](../../engine/agent/manifest.fragments/009-validationRecipes.json); runner mapping lives in [tools/run-validation-recipe-plans.ps1](../../tools/run-validation-recipe-plans.ps1) with enforcement in [tools/check-validation-recipe-runner.ps1](../../tools/check-validation-recipe-runner.ps1), [tools/check-json-contracts.ps1](../../tools/check-json-contracts.ps1), and [tools/check-ai-integration.ps1](../../tools/check-ai-integration.ps1).

## Constraints

- **Windows-only:** `MK_editor_game_module_driver_probe` / `MK_editor_game_module_driver_load_tests` are `WIN32` CMake registrations; non-Windows hosts must not treat execution as supported.
- No widening of active-session hot reload, stable third-party ABI, or `mirakana_editor` productization ready claims.
- Execution remains **host-gated:** `-HostGateAcknowledgements windows-msvc-dev-editor-game-module-driver-ctest` required for `Execute` mode.

## Done when

- New recipe id `dev-windows-editor-game-module-driver-load-tests` appears in `009-validationRecipes.json`, `010` `run-validation-recipe` command `validationRecipes`, `hostGates` on that command surface, global `hostGates` with id `windows-msvc-dev-editor-game-module-driver-ctest`, `tools/run-editor-game-module-driver-load-tests.ps1`, `Get-ValidationRecipeCommandPlan` branch, `check-validation-recipe-runner` dry-run + missing-ack execute rejection, allowlist bumps in `check-json-contracts` / `check-ai-integration`, and `recommendedNextPlan.completedContext` needle.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the integration host.

## Validation evidence

| Check | Command / artifact | Result |
| --- | --- | --- |
| Recipe dry-run | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe dev-windows-editor-game-module-driver-load-tests -HostGateAcknowledgements windows-msvc-dev-editor-game-module-driver-ctest` | **Passed** |
| Recipe execute | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe dev-windows-editor-game-module-driver-load-tests -HostGateAcknowledgements windows-msvc-dev-editor-game-module-driver-ctest` | **Passed** (Windows MSVC dev host) |
| Runner contract | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | **Passed** |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | **Passed** |
