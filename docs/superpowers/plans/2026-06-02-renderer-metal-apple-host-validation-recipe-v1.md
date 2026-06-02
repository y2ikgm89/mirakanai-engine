# 2026-06-02 Renderer Metal Apple Host Validation Recipe v1

**Plan ID:** `renderer-metal-apple-host-validation-recipe-v1`
**Status:** Completed
**Owner:** `tools` / `MK_renderer` / agent-surface governance
**Parent:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a reviewed AI-operable validation recipe that can collect Apple-host renderer Metal evidence through the existing macOS CMake/CTest lane without promoting Metal readiness from non-Apple evidence.

## Context

The current production-completion manifest keeps `unsupportedProductionGaps = []` and `recommendedNextPlan.id = next-production-gap-selection`. Remaining renderer rows are host-gated because Windows/D3D12 and strict Vulkan evidence must not imply Apple/Metal readiness. The repository already has `check-apple-host-evidence.ps1` for macOS/Xcode/Metal tool diagnostics and `ci-macos-appleclang` for hosted macOS configure/build/CTest coverage, but `run-validation-recipe` does not expose a focused renderer Metal host-evidence recipe.

Official and current-doc checks used for this slice:

- Apple Xcode command-line tool reference: full Xcode provides tools such as `xcodebuild` and `simctl`, and Xcode must be active before these commands are available.
- Apple command-line tools installation docs: some commands, including `xcodebuild`, ship only with Xcode, not just the Command Line Tools package.
- Apple Metal command structure docs: Metal GPU work is encoded through command buffers/encoders and committed to the GPU, so renderer host evidence must run on an Apple host rather than be inferred from another backend.
- Apple Metal feature set tables: Metal capability support is platform/GPU-family specific.
- Context7 `/kitware/cmake`: CMake configure/build/test presets are the supported way to name reproducible configure, build, and CTest flows; `ctest --preset` derives the binary directory through the test preset's `configurePreset`.

## Constraints

- Do not mark `metal-apple` ready.
- Do not change `renderer_quality_matrix_general_renderer_quality_ready=0`.
- Do not expose public native Metal, RHI, platform, or backend handles.
- Do not add SDL3, compatibility shims, or raw arbitrary shell evaluation.
- Keep `run-validation-recipe` allowlist-only and require explicit `metal-apple` acknowledgement before execute.
- Keep manifest changes fragment-owned and recompose `engine/agent/manifest.json`.

## Tasks

- [x] Add `tools/validate-renderer-metal-apple.ps1` as the reviewed host-evidence entrypoint.
- [x] Add `renderer-metal-apple-host-evidence` to `tools/run-validation-recipe-plans.ps1` with required `metal-apple` acknowledgement.
- [x] Extend `tools/check-validation-recipe-runner.ps1` dry-run and missing-host-gate coverage.
- [x] Update validation recipe manifest fragments and `metal-apple` host-gate recipe references.
- [x] Compose `engine/agent/manifest.json`.
- [x] Update registry/current docs/static checks and keep host-gated wording exact.
- [x] Run focused validation and record evidence.
## Done When

- `run-validation-recipe -Mode DryRun -Recipe renderer-metal-apple-host-evidence` returns a reviewed command plan.
- `run-validation-recipe -Mode Execute -Recipe renderer-metal-apple-host-evidence` is rejected unless `HostGateAcknowledgements` includes `metal-apple`.
- The execute command runs `check-apple-host-evidence.ps1 -RequireReady`, then the `ci-macos-appleclang` configure/build/CTest path for `MK_backend_scaffold_tests` and `MK_renderer_quality_matrix_tests`.
- Root validation recipes, production-loop command surface, host gate recipe rows, composed manifest, and static checks agree.
- Local Windows validation records Apple/Metal as host-gated and does not claim broad renderer quality.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe renderer-metal-apple-host-evidence` | Passed | Returned a reviewed `validate-renderer-metal-apple.ps1` command plan with `metal-apple` host gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe renderer-metal-apple-host-evidence` | Rejected as expected | Failed closed with `missing-host-gate-acknowledgement` until `metal-apple` is explicitly acknowledged. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe renderer-metal-apple-host-evidence -HostGateAcknowledgements metal-apple -TimeoutSeconds 30` | Failed as expected on Windows | Reported `apple-host-evidence-check: host-gated` with `not_macos`, missing Xcode, missing `xcrun`, and missing Metal tool blockers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | Passed | Dry-run and execute-gate runner contracts include `renderer-metal-apple-host-evidence`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest compose and JSON contracts agree with the new recipe. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent surfaces agree with the allowlist and host-gated contract. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `unsupported_gaps=0`; no readiness promotion was introduced. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Script/agent surface hygiene stayed green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Text and source formatting stayed green. |
| `git diff --check` | Passed | No whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full local validation passed: 19 static checks, build, tidy smoke, and 85/85 CTest tests. |
