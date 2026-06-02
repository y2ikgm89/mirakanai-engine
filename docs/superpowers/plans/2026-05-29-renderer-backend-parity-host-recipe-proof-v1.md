# 2026-05-29 Renderer Backend Parity Host Recipe Proof v1

**Plan ID:** `renderer-backend-parity-host-recipe-proof-v1`

**Status:** Completed

**Owner:** `MK_renderer`

## Goal

Make renderer backend parity evidence unambiguous for Metal by requiring every Metal host-gated or Apple-host-validated proof row to name the validation recipe that can close it.

## Context

The production-completion manifest has `unsupportedProductionGaps = []`, so this slice is a post-1.0 evidence hardening pass, not a reopened Engine 1.0 blocker. `Renderer Backend Parity Metal Apple Evidence v1` already keeps `metal-apple` host-gated behind Apple/macOS/Xcode evidence, but `BackendRendererParityProofRow` did not carry a machine-readable recipe id for the host-gate row itself.

Official and Context7 checks used for this slice:

- Apple Metal developer workflows ([developer.apple.com/documentation/xcode/metal-developer-workflows](https://developer.apple.com/documentation/xcode/metal-developer-workflows/)): Metal API validation, shader validation, and Metal debugger evidence belong to Apple/Xcode workflows.
- Apple Xcode command-line tools ([developer.apple.com/documentation/xcode/xcode-command-line-tool-reference](https://developer.apple.com/documentation/xcode/xcode-command-line-tool-reference)): `xcodebuild` and `xcrun` require Xcode/Apple developer directory availability.
- Apple Metal feature set tables ([developer.apple.com/metal/capabilities](https://developer.apple.com/metal/capabilities/)): feature availability is GPU family/platform dependent and must be checked on the relevant Apple host.
- Context7 `/websites/developer_apple`: Apple documentation summary confirms Metal validation and Apple silicon/Apple GPU verification are host/tool workflows.

## Constraints

- Keep public gameplay contracts first-party and backend-neutral.
- Do not expose native D3D12, Vulkan, Metal, RHI, or platform handles.
- Do not infer Metal readiness from D3D12/Vulkan proof.
- Do not introduce SDL3 or compatibility shims.
- Keep `unsupportedProductionGaps = []` and return the manifest to `recommendedNextPlan.id = next-production-gap-selection`.

## Implementation

- Added `BackendRendererParityProofRow::host_validation_recipe_id`.
- Added `BackendRendererParityDiagnosticCode::missing_host_validation_recipe`; the later allowlist hardening also adds `BackendRendererParityDiagnosticCode::unreviewed_host_validation_recipe`.
- Updated `plan_backend_renderer_parity_policy` so Metal rows require a non-empty, valid, native-token-free recipe id, and so replay hashes include the recipe id.
- Documented reviewed recipe ids such as `shader-toolchain`, `mobile-packaging`, `renderer-metal-apple-host-evidence`, and `ios-simulator-smoke` as the explicit Metal host-validation anchors.
- Updated renderer tests to cover missing Metal recipe ids.
- Updated current capabilities, generated-game guidance, validation scenarios, roadmap, plan registry, production-completion projections, manifest fragments, and rendering skills.
- Added static integration checks for the new public field, diagnostic, docs, manifest guidance, and test coverage.

## Done When

- Focused renderer test covers the missing Metal host recipe diagnostic.
- Docs, skills, manifest fragments, composed manifest, and static checks describe the same contract.
- `metal-apple` remains host-gated without Apple-host proof.
- Full repository validation passes or a concrete host/tool blocker is recorded.

## Validation Evidence

| Check | Result |
| --- | --- |
| RED `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_renderer_tests` | Failed before implementation because `host_validation_recipe_id` and `missing_host_validation_recipe` did not exist. |
| GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_renderer_tests` | Passed. |
| GREEN `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests` | Passed, 1/1 tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1` | Passed after manifest recomposition. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-ai-integration.ps1` | Passed after adding the backend-parity host recipe drift guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-public-api-boundaries.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-format.ps1` | Passed. |
| `git diff --check` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-shader-toolchain.ps1` | Passed as diagnostic-only: D3D12/Vulkan ready, Metal compiler/library tooling missing on this host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-apple-host-evidence.ps1` | Passed as host-gated on Windows with macOS/Xcode/Simulator blockers recorded. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-toolchain.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1` | Passed, including 19 static checks, build, tidy smoke, and 80/80 CTest tests. |
