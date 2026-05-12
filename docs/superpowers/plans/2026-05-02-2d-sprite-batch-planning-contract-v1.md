# 2D Sprite Batch Planning Contract v1 (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent, renderer-neutral 2D sprite batch planning contract that lets engine/AI workflows reason about stable sprite draw batches without claiming production sprite batching or exposing native handles.

**Architecture:** The slice adds a small `mirakana_renderer` value API over existing `mirakana::SpriteCommand` rows. The planner preserves submit order, coalesces only adjacent compatible sprite runs, reports deterministic diagnostics for invalid texture metadata, and remains independent from RHI devices, backends, runtime packages, atlas packing, and image decoding.

**Tech Stack:** C++23, `mirakana_renderer`, existing unit-test executable `mirakana_renderer_tests`, docs/static checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Goal

Introduce `2d-sprite-batch-planning-contract` as a focused, host-independent planning surface for 2D sprite submissions. The ready claim is limited to deterministic batch planning and diagnostics; native GPU execution, texture atlas creation, image decoding, renderer quality, and production batching execution remain non-ready.

## Post-Generated Review Findings

- Latest completed slice: `docs/superpowers/plans/2026-05-02-generated-2d-native-rhi-sprite-package-scaffold-v1.md`.
- Validation evidence from that slice is recorded as PASS for `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`; the latest `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` evidence reports `validate: ok` and 28/28 CTest entries passed.
- Host gates remain explicit: Apple/iOS/Metal requires macOS/Xcode/metal/metallib; strict clang-tidy requires a compile database for the active generator/toolchain. Vulkan package claims stay strict host/runtime/toolchain-gated outside the verified Windows host.
- Non-goals from the prior slice remain non-ready: production sprite batching, source image decoding, production atlas packing, tilemap editor UX, package streaming execution, renderer/RHI teardown execution, public native/RHI handles, Metal/iOS readiness, broad renderer quality, material/shader graphs, live shader generation, and production text/font/IME/accessibility.
- `engine/agent/manifest.json.aiOperableProductionLoop.recommendedNextPlan` requested this post-review before selecting the next focused gap. This plan selects the next gap as a planning contract only, not a production batching ready claim.

## Context

- Existing `mirakana::SpriteCommand` already carries transform, color, and optional texture region metadata.
- `RhiNativeUiOverlay` can submit a narrow host-owned native overlay path for selected package smokes, but it is not a production sprite batching system.
- Current docs explicitly list production sprite batching as non-ready. A batch planning contract is a small prerequisite that can be validated through normal Windows source-tree tests without broadening renderer claims.

## Constraints

- Keep `engine/core` independent from OS, GPU, asset formats, editor, SDL3, Dear ImGui, and native handles.
- Keep gameplay public APIs free of native OS/GPU/RHI handles.
- Do not introduce third-party dependencies.
- Preserve sprite submit order; do not sort by texture because transparent 2D ordering must remain caller-owned.
- Do not claim production sprite batching, atlas packing, runtime/source image decoding, package streaming execution, Metal/iOS readiness, material/shader graphs, live shader generation, production text/font/IME/accessibility, public native/RHI handles, or broad renderer quality.
- Keep the planner host-independent and testable through `mirakana_renderer_tests`.

## Done When

- [x] RED unit tests were added before implementation; the raw RED build attempt was blocked by a host MSBuild `Path`/`PATH` environment issue before the compiler reached the expected missing-contract failure, and this is recorded below.
- [x] `mirakana_renderer` exposes a deterministic sprite batch planning value API over `std::span<const SpriteCommand>`.
- [x] The planner coalesces only adjacent compatible sprite rows while preserving submission order.
- [x] The planner reports invalid texture metadata diagnostics and treats invalid textured sprites as untextured fallback rows for planning.
- [x] `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` are synchronized with a limited planning-contract ready claim.
- [x] Static checks enforce the manifest/docs contract and continue to reject production sprite batching, atlas packing, image decoding, Metal, public native/RHI handle, and renderer quality claims.
- [x] Required validation has PASS evidence or concrete environment blockers recorded here.

## Implementation Tasks

- [x] Add RED tests to `tests/unit/renderer_rhi_tests.cpp` for stable adjacent-run coalescing and invalid texture metadata diagnostics.
- [x] Add `engine/renderer/include/mirakana/renderer/sprite_batch.hpp` with `SpriteBatchKind`, `SpriteBatchRange`, `SpriteBatchDiagnostic`, `SpriteBatchPlan`, and `plan_sprite_batches`.
- [x] Add `engine/renderer/src/sprite_batch.cpp` and register it in `engine/renderer/CMakeLists.txt`.
- [x] Run the focused renderer test to verify GREEN.
- [x] Update docs, registry, manifest, and static checks with the limited `2d-sprite-batch-planning-contract` capability.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Re-read the registry and manifest after completion and set the next recommended focused step or stop on host-gated/broad-only work.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_renderer_tests` | BLOCKED RED attempt | RED tests were added before implementation, but the raw build stopped before compiler diagnostics with MSBuild `CL.exe` environment error: duplicate `Path` / `PATH` key. This did not prove the intended missing-contract failure, so GREEN verification used the repository wrapper that normalizes the process environment. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -Command '. ''tools/common.ps1''; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_renderer_tests; Invoke-CheckedCommand $tools.CTest --test-dir out/build/dev -C Debug --output-on-failure -R mirakana_renderer_tests'` | PASS | `mirakana_renderer_tests` built and CTest reported `100% tests passed, 0 tests failed out of 1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Initially failed until docs/static contract text included `2D Sprite Batch Planning Contract v1`; after updates, `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Initially failed until the manifest recipe id was defined; after updates, `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Final re-run after the Next-Step Decision update reported `validate: ok`; 28/28 dev CTest entries passed. Diagnostic-only host gates remain: Metal shader/library tools missing on this Windows host, Apple packaging blocked by missing macOS/Xcode/xcodebuild/xcrun, and strict clang-tidy compile database missing for the active Visual Studio generator. D3D12 DXIL and Vulkan SPIR-V shader toolchain readiness report ready. |

## Non-Goals

- Production sprite batching execution or renderer quality claims.
- Sprite sorting, material sorting, z-order policy, transparency policy, or render-queue ownership.
- Source/runtime image decoding.
- Production atlas packing or tilemap editor UX.
- Native GPU/RHI upload, residency, or package streaming execution.
- Public gameplay access to native OS, GPU, or RHI handles.
- Metal/iOS readiness.
- Material/shader graphs, live shader generation, or production text/font/IME/accessibility.

## Next-Step Decision

After completion, `docs/superpowers/plans/README.md` and `engine/agent/manifest.json` were re-read. The current active plan points at this completed slice and `recommendedNextPlan` is `post-2d-sprite-batch-planning-contract-review`.

No additional concrete focused production slice is selected by the registry/manifest. Remaining manifest gaps are broad or host-gated: `3d-playable-vertical-slice`, `editor-productization`, and `production-ui-importer-platform-adapters` are still broad planned areas; Apple/iOS/Metal remains host-gated by macOS/Xcode/metal/metallib. Stop here until a post-review chooses a new focused slice with explicit non-goals and validation evidence.
