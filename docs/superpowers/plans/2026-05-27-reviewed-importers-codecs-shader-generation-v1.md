# Reviewed Importers Codecs And Shader Generation v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move selected asset import, codec, and shader generation lanes from value-only review into reviewed, dependency-gated execution without arbitrary importer plugins, live shader generation, runtime source parsing, external downloads, or public native handles.

**Architecture:** Keep gameplay/runtime package consumption on first-party cooked formats. Importer, codec, and shader compiler integrations live in `MK_tools` behind reviewed command/value contracts, optional vcpkg features, deterministic diagnostics, package-visible counters, legal records, and host/toolchain gates.

**Tech Stack:** C++23, `MK_assets`, `MK_tools`, `MK_runtime`, `MK_renderer`, optional `asset-importers` vcpkg feature, libspng, fastgltf, miniaudio, KTX2/Basis/KTX Software, DXC, SPIR-V Tools `spirv-val`, CMake/CTest, and PowerShell validation wrappers.

---

**Plan ID:** `reviewed-importers-codecs-shader-generation-v1`

**Status:** Active.

Selected child plan of `clean-break-broad-production-readiness-master-plan-v1` after `runtime-ui-text-platform-stack-v1` completed through PR #264, PR #265, PR #266, PR #267, and PR #268.

**Date:** 2026-05-27

## Context

Current import and cook evidence is intentionally split:

- `broad-reviewed-asset-import-production-review-v1` gives `MK_assets` a value-only promotion gate through `review_asset_import_production_readiness`.
- The optional `asset-importers` lane already carries audited libspng, fastgltf, and miniaudio dependencies behind `tools/bootstrap-deps.ps1`, `tools/build-asset-importers.ps1`, and `MK_ENABLE_ASSET_IMPORTERS`.
- `MK_tools` has first-party source registration, package update, glTF inspection/import helpers, material graph shader export, shader compile-request planning, and shader cache metadata reconciliation.
- Runtime/game code must consume cooked packages and must not parse PNG, glTF, KTX2, audio, shader, or material source files directly.

This child plan promotes only selected reviewed execution lanes. It must not make broad codec readiness, arbitrary importer execution, live shader compiler execution, renderer/RHI residency, package streaming, or runtime source parsing ready by inference.

## Official Practice Check

Official documentation rechecked for this plan selection:

- Microsoft vcpkg manifest and feature guidance through Context7 `/microsoft/vcpkg`: optional dependencies stay in manifest features, the registry stays pinned by `builtin-baseline`, and CMake consumes already bootstrapped packages instead of installing during configure.
- Khronos glTF 2.0 specification: <https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html> and Khronos glTF overview <https://www.khronos.org/gltf>.
- Existing fastgltf dependency docs: <https://github.com/spnda/fastgltf>.
- KTX Software / KTX2 / Basis Universal through Context7 `/khronosgroup/ktx-software`: Basis-compressed KTX2 may require explicit transcode target selection before GPU upload; compression/transcoding stays offline/reviewed until a host/toolchain lane implements it.
- Existing libspng docs: <https://libspng.org/>.
- Existing miniaudio docs: <https://miniaud.io/docs/manual/index.html>.
- Microsoft DirectXShaderCompiler / DXC docs: <https://github.com/microsoft/DirectXShaderCompiler>.
- Khronos SPIR-V Tools / `spirv-val`: <https://github.com/KhronosGroup/SPIRV-Tools>.
- cgltf Context7 `/jkuhlmann/cgltf` was checked as a cross-reference for glTF parser boundaries: parse/load/validate/free lifetimes stay importer-private and do not leak parser types into public engine APIs. This plan keeps the current fastgltf dependency unless a later legal/dependency slice intentionally changes it.

## Constraints

- Do not add new third-party dependencies without `vcpkg.json`, `tools/bootstrap-deps.ps1`, docs/dependency records, legal records, notices, CMake package config, manifest feature rows, and validation checks in the same slice.
- Do not run importer plugins, external downloads, live shader generation, shell commands from game/editor rows, or shader compiler execution unless a reviewed host/toolchain command row explicitly authorizes the exact invocation.
- Do not expose fastgltf, libspng, miniaudio, KTX, DXC, SPIR-V Tools, D3D12, Vulkan, Metal, OS, filesystem, or native process handles through gameplay/runtime public APIs.
- Do not let generated games or editor-core rows mutate packages or execute tools directly. They may display reviewed rows, import external evidence, or call reviewed command surfaces after engine-owned implementation lands.
- Keep source formats out of runtime packages unless a package manifest explicitly marks them as authoring/source inputs outside `runtimePackageFiles`.
- Keep `unsupportedProductionGaps = []`; this is post-1.0 broad production work, not a reopened Engine 1.0 blocker.

## Candidate Evidence

| Candidate | Scope | Current evidence | Remaining gap |
| --- | --- | --- | --- |
| `reviewed-importer-execution-baseline-v1` | Audit current optional importer/tooling surfaces and fail-closed broad claims. | `asset-importers` feature, libspng/fastgltf/miniaudio notices, source registration, package update helpers, and value-only production review gate exist. | Need an execution-readiness matrix that distinguishes reviewed execution, host-gated execution, dependency-gated execution, package mutation, and unsupported arbitrary importer claims. |
| `ktx2-basis-texture-review-v1` | Reviewed KTX2/Basis texture import and offline transcode planning. | Current asset review can record host-gated KTX2/Basis evidence but no KTX dependency or adapter is selected. | Need dependency/legal decision, schema/diagnostics, package handoff rows, explicit transcode target policy, and package counters before any KTX2/Basis claim is ready. |
| `gltf-scene-import-execution-v1` | Promote selected fastgltf-backed mesh/material/animation import helpers into reviewed package handoff lanes. | Existing glTF helper tests cover multiple mesh/skin/animation/morph import rows and cooked package artifacts. | Need production promotion rows, source-root restrictions, deterministic hash/provenance rows, package handoff counters, and generated-game guidance without parser type leakage. |
| `source-image-audio-codec-execution-v1` | Promote selected libspng/miniaudio decode lanes into reviewed package source/cooked handoff. | PNG and audio source adapters exist behind `asset-importers`; runtime consumes cooked assets. | Need package-visible execution evidence, codec diagnostics, sample/channel/pixel format rows, dependency/legal confirmation, and explicit denial of broad codec readiness. |
| `reviewed-shader-generation-execution-v1` | Move shader compile-request planning toward reviewed offline compiler execution and cache artifacts. | `build_shader_pipeline_cache_plan`, compile request rows, D3D12/Vulkan package shader artifact smokes, and `spirv-val` toolchain checks exist. | Need exact reviewed command rows, DXC/SPIR-V validation evidence, no live runtime generation, package cache metadata, and host-gated Metal non-claims. |

## Files

- Modify: `engine/assets/include/mirakana/assets/asset_import_production_review.hpp`
- Modify: `engine/assets/src/asset_import_production_review.cpp`
- Modify: `engine/tools/include/mirakana/tools/asset_import_tool.hpp`
- Modify: `engine/tools/include/mirakana/tools/asset_import_adapters.hpp`
- Modify: `engine/tools/include/mirakana/tools/gltf_*`
- Modify: `engine/tools/include/mirakana/tools/source_image_decode.hpp`
- Modify: `engine/tools/include/mirakana/tools/shader_compile_action.hpp`
- Modify: `engine/tools/include/mirakana/tools/material_graph_shader_pipeline.hpp`
- Modify: `engine/tools/asset/*.cpp`
- Modify: `engine/tools/gltf/*.cpp`
- Modify: `engine/tools/shader/*.cpp`
- Modify: `tests/unit/asset_import_production_review_tests.cpp`
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/*/game.agent.json`
- Modify: `vcpkg.json`
- Modify: `CMakePresets.json`
- Modify: `tools/bootstrap-deps.ps1`
- Modify: `tools/build-asset-importers.ps1`
- Modify: `tools/check-dependency-policy.ps1`
- Modify: `tools/check-ai-integration*.ps1`
- Modify: `tools/check-json-contracts*.ps1`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/*.json`
- Generate: `engine/agent/manifest.json`

## Task 1 - Baseline Import/Codec/Shader Execution Audit

- [x] Read current `MK_assets`, `MK_tools`, optional importer adapter, shader toolchain, sample package, dependency, and legal records for import/codec/shader execution.
- [x] Add an evidence table to this plan for ready rows, dependency-gated rows, host-gated rows, and unsupported broad claims.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
```

Expected: dependency/toolchain gates either pass or record exact host/tool blockers before implementation.

Task 1 baseline evidence:

| Surface | Evidence row | Result | Boundary |
| --- | --- | --- | --- |
| `MK_assets` production review | Ready rows | `AssetImportProductionExecutionReadiness::reviewed_execution`, `ready_row_count`, and `broad_asset_import_ready` require declared source roots, importer ids, package rows, license/provenance/hash rows, validator/dependency/legal rows, reviewed command rows, and host validation. | Value-only review; no importer execution, shader compiler execution, package mutation, or runtime source parsing. |
| `MK_assets` production review | Dependency-gated rows | `AssetImportProductionStatus::dependency_evidence_required`, `dependency_gated_row_count`, and execution-readiness rows now distinguish missing dependency/legal evidence from invalid broad readiness without adding diagnostics when the row explicitly declares a dependency gate. | KTX2/Basis and other optional dependency lanes stay non-ready until selected dependency/legal evidence exists. |
| `MK_assets` production review | Host-gated rows | Existing host-gated rows remain `AssetImportProductionStatus::host_evidence_required` with `host_gated_row_count` and no broad readiness. | Windows evidence does not promote Apple/Metal or unavailable host/toolchain rows. |
| `MK_assets` production review | Package mutation rows | `request_package_mutation`, `unsupported_package_mutation`, `package_mutation_request_count`, and `package_mutation_required` readiness reject package mutation from importer review rows separately from broad unsupported importer claims. | Generated games and editor rows do not mutate packages through this review API. |
| `MK_assets` production review | Unsupported broad claims | Arbitrary importer plugin, external download, live shader generation, source mutation outside roots, native handle, unreviewed compiler execution, runtime source parsing, and broad codec requests remain `unsupported_claim` rows with diagnostics. | Broad importer/codec/shader claims stay fail-closed until a later reviewed execution lane implements them. |
| Host/toolchain | `tools/check-toolchain.ps1` | Passed. | Local Windows toolchain was available for this slice. |
| Dependency policy | `tools/check-dependency-policy.ps1` | Passed. | No dependency manifest/legal change in this baseline slice. |
| Shader toolchain | `tools/check-shader-toolchain.ps1` | Exited 0 as diagnostic-only: D3D12 DXIL and Vulkan SPIR-V tools ready, Metal library tools missing. | Metal remains host/toolchain-gated. |
| Optional importer lane | `tools/build-asset-importers.ps1` | Passed; installed consumer validation reported `installed-sdk-validation: ok`. | Optional libspng/fastgltf/miniaudio lane builds without making broad importer execution ready. |

## Task 2 - RED Tests For Reviewed Execution Promotion

- [x] Add tests that fail until import production rows distinguish value-only review, reviewed execution, dependency-gated execution, host-gated execution, package handoff, and unsupported arbitrary importer claims.
- [ ] Add tests that reject KTX2/Basis, broad image/audio codec, glTF scene import, or shader compiler readiness without dependency/legal records, source-root validation, deterministic hashes, package rows, and host/toolchain validation.
- [ ] Add tests that reject parser/compiler/native handle leakage in public gameplay/runtime rows.
- [x] Run focused tests and record expected RED failures in this plan.

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_production_review_tests MK_tools_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_asset_import_production_review_tests|MK_tools_tests"
```

Task 2 baseline RED/GREEN evidence:

| Step | Command | Result |
| --- | --- | --- |
| RED | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_production_review_tests` | Failed as expected before implementation on missing `AssetImportProductionExecutionReadiness`, dependency-gate fields/counters, package-mutation fields/counters, and `unsupported_package_mutation`. |
| GREEN build | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_production_review_tests` | Passed after the baseline execution-readiness matrix implementation. |
| GREEN test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_asset_import_production_review_tests` | Passed: 1/1 test, 0 failures. |

## Task 3 - KTX2/Basis Texture Review Lane

- [ ] Decide whether to select KTX Software as an optional dependency. If selected, update `vcpkg.json`, bootstrap, dependency docs, legal notices, CMake config, and dependency checks in the same commit.
- [ ] Add first-party KTX2/Basis review rows for container validation, supercompression/transcode policy, GPU target compatibility, source provenance, package output rows, and host/tool gates.
- [ ] Keep compression/transcoding as offline reviewed cook planning unless a later slice implements exact tool execution with host evidence.
- [ ] Add selected package counters only for implemented rows; unsupported broad texture codec readiness remains explicit.
- [ ] Run focused dependency, asset review, tools, package, and static checks.

## Task 4 - glTF Scene Import Execution Lane

- [ ] Promote selected fastgltf-backed mesh/material/animation import helpers through reviewed source-root, hash/provenance, package handoff, and validator evidence rows.
- [ ] Reject arbitrary glTF extensions, external network fetches, runtime source parsing, parser type leakage, native handles, and broad scene import readiness without explicit rows.
- [ ] Add generated 3D package counters only for implemented import rows.
- [ ] Run focused `MK_tools_tests`, selected generated 3D package smokes, package validation, and agent/static checks.

## Task 5 - Source Image And Audio Codec Execution Lane

- [ ] Promote selected libspng PNG decode and miniaudio source decode rows through reviewed package handoff, pixel/sample format diagnostics, deterministic hashes, and dependency/legal evidence.
- [ ] Keep SVG/vector, broad image codec, broad audio codec, background decode streaming, HRTF/middleware, and runtime source parsing unready unless separately implemented.
- [ ] Add selected 2D/3D package counters for implemented decode/cook rows.
- [ ] Run focused importer, audio, package, dependency, and static checks.

## Task 6 - Reviewed Shader Generation And Cache Execution Lane

- [ ] Add reviewed offline compiler execution rows for exact DXC and `spirv-val` commands, inputs, outputs, target profiles/environments, cache keys, provenance, and diagnostics.
- [ ] Keep live shader generation, runtime compiler execution, native PSO/Vulkan/Metal cache handles, renderer/RHI residency, and Metal library generation host-gated or unsupported.
- [ ] Add package-visible shader generation/cache counters only for implemented D3D12/Vulkan lanes.
- [ ] Run shader toolchain, generated material/shader package, Vulkan strict package, dependency/static, and full validation gates.

## Task 7 - Docs, Manifest, Static Checks, And Closeout

- [ ] Reconcile current capabilities, roadmap, AI game guidance, generated-game scenarios, dependency/legal records, manifest fragments, schemas, static checks, and validation recipes with only the implemented rows.
- [ ] Compose the manifest.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

Expected: all checks pass, or exact host/dependency blockers are recorded.

## Done When

- Importer, codec, and shader-generation claims are backed by reviewed execution rows, dependency/legal records, package counters, and validation evidence.
- Broad importer plugins, runtime source parsing, external downloads, live shader generation, public native handles, and broad codec readiness remain unclaimed unless separately implemented with proof.
- Docs, manifest fragments, schemas/static checks, generated-game guidance, dependency/legal records, and validation recipes match the implemented scope.
- A validated commit and reviewable PR exist for each independent candidate before moving to the next child plan.
