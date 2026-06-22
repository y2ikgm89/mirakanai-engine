# 2D Production Workload Matrix v1

## Status

Active implementation evidence for `2d-production-engine-capability-gap-cluster-v1` Phase 6.

## Goal

Define the exact selected 2D production workload matrix that can be validated from `games/sample_2d_desktop_runtime_package/game.agent.json` and package-visible smoke counters without claiming broad CPU/GPU/memory optimization, backend parity, Metal readiness, native handles, or renderer/RHI residency.

## Workload Rows

| Workload ID | Required evidence | Ready boundary |
| --- | --- | --- |
| `2d-dense-arena` | `installed-2d-sprite-throughput-smoke`, `2d_sprite_throughput_*` counters, and retained timing hash evidence. | Ready only for selected dense 512, dense 4096, and projectile storm sprite throughput rows. |
| `2d-sandbox-tilemap` | `installed-2d-sandbox-package-budget-smoke`, `installed-2d-tilemap-runtime-ux-smoke`, `sandbox_package_budget_*`, `tilemap_*`, and `tile_chunk_renderer_*` counters. | Ready only for selected sandbox/tile package, tile sampling, tile chunk planning, and safe-point streaming budget counters. |
| `2d-ui-overlay` | `installed-2d-runtime-ui-renderer-atlas-handoff-smoke` and `runtime_ui_renderer_atlas_handoff_*` counters. | Ready only for cooked UI atlas metadata, renderer-neutral submission rows, atlas budget rows, and zero native/renderer upload side effects. |
| `2d-hot-reload-package` | External playtest loop evidence rooted at `games/sample_2d_desktop_runtime_package/reports/playtest/latest` plus existing hot-reload review contracts. | Host-gated until an external operator supplies recook-to-resident replacement evidence. It must stay non-executing in this matrix. |
| `2d-long-run-selected` | `installed-2d-long-run-readiness-smoke`, optional `host-2d-long-run-readiness-soak`, and `long_run_readiness_*` counters. | Ready for the selected short deterministic long-run package lane; the 108000-frame soak remains opt-in host evidence. |

## Required Manifest Contract

`games/sample_2d_desktop_runtime_package/game.agent.json.performanceBudgets` must include:

- `workloadRows` with exactly the five workload IDs above.
- `budgetRows` that name package-visible metrics for frame, memory, draw, upload, package, streaming, UI overlay, and hot-reload evidence review.
- `evidenceRows` that point at same-manifest `validationRecipes` and distinguish `ready` package-smoke evidence from `host-gated` manual or trace evidence.
- `unsupportedClaims` on the budget set and each workload row, including `broad-optimized-game`, `cross-vendor-performance-parity`, `cross-backend-performance-parity`, and `native-handles`.

## Validator Contract

`tools/validate-2d-production-workloads.ps1` is the owning gate.

Default mode:

- Confirms this spec exists.
- Validates `performanceBudgets.workloadRows`, recipe references, budget row references, evidence row references, artifact-path safety, and unsupported-claim sentinels.

`-RequireReady` mode:

- Runs the selected installed package smoke through `tools/package-desktop-runtime.ps1`.
- Requires the selected package-visible counters for sprite throughput, sandbox/tilemap, UI overlay, performance baseline, long-run readiness, and input-device side-effect non-claims.
- Requires positive retained hash or high-water counters where the selected ready rows claim measured or retained evidence.
- Allows `2d-hot-reload-package` and optional `host-2d-long-run-readiness-soak` to remain `host-gated`; they must not promote broad optimization readiness.

## Non-Claims

This matrix does not prove broad optimized-game readiness, all-core CPU scheduling, cross-vendor or cross-backend performance parity, renderer/RHI residency, automatic package streaming, in-process hot reload, package-script execution, native handles, Metal readiness, CUDA/HIP/SYCL, allocator/GPU budget enforcement, or general production renderer quality.
