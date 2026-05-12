# Editor Material Preview Vulkan Strict Validation Recipe v1 (2026-05-11)

**Plan ID:** `editor-material-preview-vulkan-strict-validation-recipe-v1`  
**Gap:** `editor-productization` (material preview display parity stream — toolchain evidence)  
**Parent stream:** [2026-05-11-editor-productization-material-preview-display-parity-stream-v1.md](2026-05-11-editor-productization-material-preview-display-parity-stream-v1.md)  
**Status:** Completed (see validation table)

## Goal

Extend the reviewed **`run-validation-recipe`** allowlist with a **strict Vulkan** validation recipe for the generated **material/shader desktop runtime scaffold** (`sample_generated_desktop_runtime_material_shader_package`), mirroring the depth of the existing per-game `desktop-runtime-release-target-vulkan-toolchain-gated` row and the D3D12 `desktop-runtime-generated-material-shader-scaffold-package` recipe, so operators and AI workflows can dry-run or execute the same argv plan behind **`vulkan-strict`** acknowledgement.

## Context

- Global manifest validation recipes live in [engine/agent/manifest.fragments/009-validationRecipes.json](engine/agent/manifest.fragments/009-validationRecipes.json); the runner allowlist is enforced in [tools/run-validation-recipe-plans.ps1](tools/run-validation-recipe-plans.ps1) and [tools/check-validation-recipe-runner.ps1](tools/check-validation-recipe-runner.ps1).
- The sample game already declares the equivalent packaged command on `game.agent.json`; this slice promotes the same contract to the **repository-wide** recipe table and `vulkan-strict` host gate.

## Constraints

- No widening of `editor-productization` ready claims or clearing `requiredBeforeReadyClaim` for full Vulkan/Metal **editor** material-preview display parity.
- Execution remains **host-gated**: `-HostGateAcknowledgements vulkan-strict` required for `Execute` mode, same pattern as other strict Vulkan package recipes.
- No new gameplay or editor-shell automation; no public RHI/native handle exposure.

## Done when

- New recipe id `desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict` appears in `009-validationRecipes.json`, `010` `run-validation-recipe` command `validationRecipes`, `vulkan-strict` `hostGates[].validationRecipes`, `material-shader-authoring-review-loop.hostGatedSmokeRecipes`, and `desktop-runtime-material-shader-package` / `aiDrivenGameWorkflow` recipe validation recipe lists where applicable.
- `Get-ValidationRecipeCommandPlan` returns the reviewed `package-desktop-runtime.ps1` argv with `-RequireVulkanShaders` and the deterministic smoke arg tail.
- `check-validation-recipe-runner`, `check-json-contracts`, and `check-ai-integration` allowlist counts and `recommendedNextPlan.completedContext` needles are updated.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the integration host.

## Validation evidence

| Check | Command / artifact | Result |
| --- | --- | --- |
| Recipe dry-run | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict -HostGateAcknowledgements vulkan-strict` | **Passed** |
| Runner contract | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | **Passed** |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | **Passed** |
