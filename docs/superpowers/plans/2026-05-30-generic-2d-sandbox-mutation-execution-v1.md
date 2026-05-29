# Generic 2D Sandbox Mutation Execution v1 - 2026-05-30

## Goal

Close the next reusable 2D sandbox runtime slice by adding first-party, host-independent mutation execution for already-reviewed sandbox placement and destruction rows.

## Context

`Generic 2D Sandbox Runtime Foundation v1` added deterministic in-memory chunk/cell world build, sample, and snapshot APIs. This slice adds the next public `MK_runtime` layer: applying accepted placement/destruction rows to a copied runtime world and returning deterministic dirty region evidence that later persistence, package streaming, renderer upload, and platform host work can consume behind separate gates.

## Constraints

- Keep the API first-party, backend-neutral, and dependency-free inside `MK_runtime`.
- Do not reintroduce SDL3, desktop middleware shims, native handles, renderer/RHI handles, editor APIs, threads, package IO, persistence IO, or platform calls.
- Preserve the input world on invalid execution plans.
- Emit explicit side-effect flags so future agents cannot infer hidden persistence, package, renderer, platform, or threading work.
- Keep game-specific biome, block-art, balance, and content rules outside the engine contract.

## Done When

- `RuntimeSandboxWorldMutationExecutionStatus`, `RuntimeSandboxWorldDirtyRegion`, `RuntimeSandboxWorldMutationExecutionResult`, and `apply_runtime_sandbox_world_mutations` are public in `sandbox_world_runtime.hpp`.
- Accepted placement/destruction rows apply to a copied runtime world and produce deterministic dirty regions with chunk ids, intent ids, coordinate bounds, layer masks, previous/new block ids, chunk-dirty flags, and replay hashes.
- Rejected or blocked mutation rows are counted but do not mutate the copied world.
- Invalid execution plans return `rejected_plan` and preserve the input snapshot.
- Tests, docs, manifest fragments, composed manifest, and static AI integration checks lock the public contract.

## Validation Evidence

| Check | Evidence |
| --- | --- |
| Focused build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_runtime_tests` |
| Focused tests | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev --output-on-failure -R MK_runtime_sandbox_world_runtime_tests` |
| Agent/static drift | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-ai-integration.ps1` |
| Manifest contracts | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1` |
| Final gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1` |
