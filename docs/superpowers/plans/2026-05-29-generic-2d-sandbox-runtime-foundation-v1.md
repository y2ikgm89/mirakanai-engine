# Generic 2D Sandbox Runtime Foundation v1 - 2026-05-29

**Status:** Completed
**Scope:** Developer-owned post-1.0 capability slice under the production-completion master plan.
**Branch:** `codex/generic-2d-sandbox-runtime-v1`
**Tech Stack:** C++23, `MK_runtime`, CMake/CTest through repository PowerShell wrappers, and Context7-checked CMake target/test wiring.

## Goal

Promote the existing value-only sandbox mutation review into a reusable first-party in-memory runtime world foundation that generic 2D sandbox/survival/building games can use before game-owned simulation code runs.

## Context

- `genre-sandbox-world-pack-v1` already proves deterministic chunk/cell, placement/destruction, construction-cost, persistence-review, unsafe-mutation, and package-counter evidence through `plan_runtime_sandbox_world_mutation`, including selected `sandbox_world_ready=1` package smokes.
- The remaining generic-game need is a small runtime value surface that turns already-reviewed chunk/cell rows into a deterministic sampled world snapshot without pulling in packages, renderer/RHI, platform, editor, persistence, threads, or native handles.
- Context7 official CMake documentation was checked for target-scoped source/test wiring: use target-local sources and named CTest tests rather than ad hoc build integration.

## Constraints

- Keep the design clean-break and first-party; do not add backward-compatibility aliases or migration shims.
- Do not add or reintroduce SDL3.
- Keep public game-facing types in `mirakana::runtime` and `MK_runtime`.
- Do not mutate worlds, files, packages, renderer state, platform state, or editor state.
- Keep package streaming, biome/block-art rules, procedural generation quality, persistence IO, and game-specific content policy out of this slice.
- Update docs, manifest fragments, static guards, and generated manifest in the same candidate so agent surfaces cannot drift.

## Implementation

- [x] Add RED unit coverage for a missing `mirakana/runtime/sandbox_world_runtime.hpp` contract.
- [x] Add `RuntimeSandboxWorldDesc`, `RuntimeSandboxWorldBuildResult`, `RuntimeSandboxWorld`, `RuntimeSandboxWorldSnapshot`, `RuntimeSandboxCellSample`, and typed runtime diagnostics.
- [x] Add `build_runtime_sandbox_world` for deterministic chunk/cell validation, sorting, snapshot hash generation, and fail-closed diagnostics.
- [x] Add `sample_runtime_sandbox_cell` for occupied, empty, and missing-chunk cell sampling.
- [x] Add `snapshot_runtime_sandbox_world` for replay-stable summary evidence.
- [x] Recompute snapshots from mutable public world vectors and reject overlapping chunk extents before sampling can become ambiguous.
- [x] Reject backend/native/renderer/RHI/GPU/SDL3/ImGui tokens in the runtime world surface.
- [x] Keep `invoked_persistence_io`, `invoked_package_io`, and `invoked_renderer_upload` false in the built world evidence.
- [x] Register `MK_runtime_sandbox_world_runtime_tests` through the repository CMake/CTest surface.
- [x] Sync current capabilities, roadmap, production backlog/projections, manifest fragments, generated manifest, and the sandbox-world static integration guard.

## Done When

- The focused runtime sandbox tests pass.
- Adjacent sandbox mutation and world-region streaming runtime tests still pass.
- Agent-surface drift checks pass.
- Full `tools/validate.ps1` passes before the candidate is committed and published.
- The manifest returns to the production-completion master plan selection gate with `unsupportedProductionGaps = []`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed | Configured the fresh worktree before RED/GREEN loops. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_runtime_tests` before implementation | Failed as expected | RED failed on missing `mirakana/runtime/sandbox_world_runtime.hpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_runtime_tests` | Passed | GREEN built the new runtime sandbox world tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_sandbox_world_runtime_tests` | Passed | New focused tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_genre_sandbox_world_tests MK_runtime_world_region_streaming_tests` | Passed | Adjacent runtime tests rebuilt. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_sandbox_world_runtime_tests\|MK_runtime_genre_sandbox_world_tests\|MK_runtime_world_region_streaming_tests"` | Passed | 3/3 adjacent runtime tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_runtime_tests; pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_sandbox_world_runtime_tests` after review tests | RED then GREEN | RED failed on overlapping chunk acceptance and stale mutable-world snapshot counts; GREEN passed after overlap rejection and snapshot recomputation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full repository validation gate for this C++/public-runtime candidate. |

## Non-Claims

This slice does not claim world mutation execution, persistence IO, package loading, package streaming, broad background streaming, renderer/RHI residency, biome or block-art ownership, procedural terrain generation, game-specific content rules, editor tooling, platform APIs, native handles, SDL3, or commercial sandbox-game content completeness.
