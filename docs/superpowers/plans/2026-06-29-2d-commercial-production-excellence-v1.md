# 2D Commercial Production Excellence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `gameengine-feature` before implementation. Use `gameengine-rendering` before renderer/RHI edits, `gameengine-editor` before editor-core or visible editor edits, `gameengine-performance-optimization` before performance-budget or benchmark edits, `gameengine-license-audit` before dependency, asset, notice, legal, clean-room, or distribution changes, and `gameengine-cmake-build-system` before CMake or validation-script edits. Refresh Context7 and official documentation for each phase on the implementation date.

**Plan ID:** `2d-commercial-production-excellence-v1`
**Date:** `2026-06-29`
**Status:** Active.

**Goal:** Define a clean-break, first-party, legally safe, official-recommendation-aligned path from the selected ready 2D desktop runtime package proof to a highest-level commercial 2D game-production engine surface. The plan must keep the current ready claims honest, close the remaining 2D authoring/runtime/renderer/UI/package/performance gaps with exact evidence, and avoid copying or implying compatibility with Unity, Unreal Engine, Godot, or any other engine.

**Architecture:** Implement future phases as independent first-party value contracts, safe-point execution paths, retained evidence artifacts, package-visible counters, and fail-closed validators. `MK_assets` and `MK_tools` own cooking, provenance, and package conditioning. `MK_runtime` owns cooked-only ingestion and safe resident-package replacement. `MK_renderer` and backend-local RHI layers own backend-specific synchronization, residency, profiling, and presentation evidence without public native handles. `MK_editor_core` and first-party visible editor surfaces own authoring review models and UI state. `MK_platform` owns host adapters behind opaque first-party handles. No external engine project reader, API clone, editor layout clone, compatibility layer, or marketplace asset ingestion is introduced.

**Tech Stack:** C++23, CMake `dev` preset, PowerShell 7 repository scripts, existing `MK_runtime`, `MK_assets`, `MK_tools`, `MK_renderer`, `MK_rhi`, `MK_platform`, `MK_audio`, `MK_physics`, `MK_ui`, `MK_editor_core`, `game.agent.json`, `engine/agent/manifest.fragments/`, repository legal/dependency policy, official Khronos Vulkan, Microsoft Direct3D 12, Apple Metal, Windows platform, GitHub Actions, and repository validation lanes.

---

## Selection Policy

This is the active milestone after `Diagnostics Backend Adapter Handoff v1` returned the live pointer to the production-completion selection gate.

- Keep `currentActivePlan` and `recommendedNextPlan.id` aligned with this plan while it is active. Do not change `unsupportedProductionGaps`, renderer readiness counters, or package readiness counters merely by selecting or extending this plan.
- Do not reopen completed 2D plans. Treat `2d-production-engine-capability-gap-cluster-v1`, `original-2d-commercial-authoring-live-iteration-v1`, selected package/playtest productization, and completed Source Pulse evidence as dependencies.
- Do not implement all phases in one branch. Each phase must be a reviewable slice with a behavior/API boundary, focused tests, docs/manifest/static-check synchronization, and validation evidence.
- Return `currentActivePlan` to the production-completion master plan only when this milestone is completed, blocked with evidence, or explicitly superseded.

## Current Repository Baseline

Known ready evidence as of this planning pass:

- `tools/check-production-readiness-audit.ps1` reports zero unsupported gaps and ready audited host-gated rows for the selected current scope.
- `tools/validate-2d-production-workloads.ps1 -RequireReady` reports the selected 2D production workload matrix ready.
- `tools/validate-2d-package-playtest-productization.ps1 -RequireReady` reports the selected 2D package/playtest productization lane ready.
- Existing 2D surfaces cover selected gameplay loop, sprite/tile/sandbox/package playtest foundations, safe package replacement, source-change review, and selected package counters.
- `docs/current-capabilities.md` still keeps broad all-platform/commercial claims separate where exact evidence is missing.

Known non-ready or host-gated areas that this plan is meant to organize:

- Broad source asset cooking, background package streaming, cooked/runtime residency, and production atlas packing beyond selected proof rows.
- Runtime source image/audio decoding in shipping builds. The desired commercial path is cooked-only runtime ingest unless a separate dependency/license gate explicitly chooses otherwise.
- Full tilemap/editor canvas workflow, production atlas editor UX, large-project asset-browser/import UX, and hot iteration across complex packages.
- Broad runtime UI production parity, including real text shaping, font fallback, glyph atlas upload, IME, accessibility publication, localization, and platform parity.
- Broad renderer/RHI commercial quality, cross-backend synchronization/residency/profiling/frame pacing/pipeline-cache evidence, and Metal memory/profiling host evidence.
- Broad audio, network, scripting/modding, physics/navigation, and multiplayer device/profile depth beyond selected optional-adapter or value-contract rows.
- Broad CPU/GPU/memory optimization, long-running package streaming, p95/p99 frame pacing, input-to-present latency, allocator/residency high-water limits, and release PGO/LTO lanes.

## Official Source Ledger

Refresh this table before any implementation slice. Context7 is used for API/toolchain documentation when available; official site fallbacks are required where Context7 lacks the authoritative source.

| Source class | Source | Planning conclusion |
| --- | --- | --- |
| Context7 Vulkan docs | `/khronosgroup/vulkan-docs`; official fallback `https://docs.vulkan.org/` | Use Vulkan 1.3 / `VK_KHR_synchronization2` style barriers and timestamp-query evidence. Do not ship full-pipeline/full-access barriers except as explicit debug diagnostics. Backend-local validators must prove stage/access masks, timestamp validity, validation-layer cleanliness, and no cross-backend inference. |
| Context7 Direct3D 12 docs | `/websites/learn_microsoft_en-us_windows_win32_direct3d12`; official fallback `https://learn.microsoft.com/en-us/windows/win32/direct3d12/` | Use official D3D12 debug layer, descriptor heap, PSO reuse, command allocator/list reset, fence, resource barrier, and present-state patterns. Validators must prove allocator reset only after GPU completion and retained PIX/WPA or timestamp evidence where a performance claim is made. |
| Context7 Metal Shading Language docs | Context7 MSL specification mirror; official fallback `https://developer.apple.com/metal/` and Apple MSL specification | Keep Metal shaders inside MSL language limits, use explicit address spaces and entry-point attributes, and validate Apple-host shader/package/memory evidence on actual Apple hardware when readiness depends on it. Context7 mirror evidence is not enough for final Apple readiness without official Apple fallback and host proof. |
| Official Windows platform docs | Microsoft Learn for Win32, DirectWrite, TSF, UI Automation, WASAPI, WPR/WPA, and DirectStorage where selected | Platform features stay behind first-party adapters. Public APIs expose value contracts and opaque handles only. |
| Unity legal/trademark docs | `https://unity.com/legal/branding-trademarks`, `https://unity.com/legal/terms-of-service`, Unity official manual pages for category research only | Unity material is not an implementation source. Do not copy Unity APIs, editor layouts, asset names, serialized formats, screenshots, package names, documentation prose, examples, or branding. |
| Unreal Engine legal/trademark docs | `https://www.unrealengine.com/eula/unreal`, `https://dev.epicgames.com/docs/dev-portal/unreal-engine/ue-trademark-license` | Unreal Engine code, Examples, Starter Content, Blueprints, Marketplace/Fab content, shaders, editor UI, and project formats are not implementation sources. Do not claim compatibility, equivalence, replacement, or endorsement. |
| Godot official license/compliance docs | `https://godotengine.org/license/`, `https://docs.godotengine.org/en/stable/about/complying_with_licenses.html` | Godot is permissively licensed with notice obligations if code is redistributed, but this plan copies no Godot code/assets/docs prose and does not adopt Godot node/resource/scene names as public `mirakana` surfaces. |
| U.S. copyright/trademark public guidance | `https://www.copyright.gov/help/faq/faq-protect.html`, `https://www.copyright.gov/what-is-copyright/`, `https://www.uspto.gov/trademarks/basics/what-trademark` | Common functional ideas may be independently implemented, but protected expression and source-identifying marks must not be copied. Keep external engine names out of public APIs, row ids, UI labels, and samples except in legal/source-audit records. |
| Repository legal/dependency policy | `docs/legal-and-licensing.md`, `docs/dependencies.md`, `THIRD_PARTY_NOTICES.md`, `vcpkg.json`, `gameengine-license-audit` | License-less material is unusable. Any third-party dependency or distributable asset requires dependency records, notices, validation guards, and explicit bootstrap/update handling before distribution. |

## Non-Negotiable Legal And Originality Boundaries

This section is an engineering compliance plan, not legal advice. Final commercial release, marketing, trademark, and distribution decisions require qualified counsel review.

Allowed:

- Use official public documentation to identify general functional categories, platform API requirements, host-validation expectations, and license/trademark constraints.
- Use existing first-party MIRAIKANAI code, tests, generated fixtures, package artifacts, and project-specific names.
- Use first-party APIs, first-party UI layouts, first-party schemas, first-party package formats, and first-party sample content.
- Use permissive dependencies only through an explicit repository dependency decision, license audit, `vcpkg.json` or bootstrap integration, notice update, and validation guard.

Forbidden:

- Do not copy, translate, port, derive from, or closely mimic Unity, Unreal Engine, Godot, GameMaker, Defold, SDL, Dear ImGui, middleware, marketplace, sample, blog, Stack Overflow, or repository snippets without an explicit license record and architectural approval.
- Do not parse Unity `.unity`, `.prefab`, `.asset`, `.meta`; Unreal `.uasset`, `.umap`, `.uproject`, Blueprints; Godot `.tscn`, `.tres`, `.godot`; or any external engine project formats.
- Do not use external engine trademarks, feature names, product names, project-file names, editor labels, serialized schema names, default theme/layout expression, icons, fonts, sample screenshots, sample scenes, starter content, shaders, or asset packs in public `mirakana` API, package-visible counters, product UI, tests, or examples.
- Do not claim compatibility, parity, equivalence, certification, replacement, or endorsement relative to Unity, Unreal Engine, Godot, or any third-party engine.
- Do not rely on external engine documentation examples as pseudocode. Category research must be translated into first-party requirements and then implemented from official platform/SDK docs or first-party design.

## Highest-Level Implementation Standard

Every phase selected from this plan must meet this bar before a ready claim:

1. Re-run targeted repository context: `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard`, plus relevant current docs and manifest fragments.
2. Refresh Context7 and official documentation for every library, SDK, graphics API, platform API, toolchain, dependency, or cloud service used by the slice.
3. Start with tests that lock the smallest externally meaningful public behavior, validator output, package counter, or API contract.
4. Use clean-break C++23 API shapes. Do not add compatibility shims, deprecated aliases, migration layers, or external-engine naming bridges.
5. Keep runtime shipping paths cooked-only by default. Source decoding, source compilers, live importers, scripts, and external processes stay in tools/editor lanes unless a separate safety plan promotes them.
6. Emit deterministic fail-closed diagnostics for missing host evidence, missing dependency records, unsupported broad claims, native-handle access, invalid package rows, over-budget metrics, stale artifacts, and legal-clean-room blockers.
7. Make performance claims only from retained artifacts with exact workload IDs, backend IDs, host class, metric names, p50/p95/p99 or high-water thresholds, and before/after evidence where optimization is claimed.
8. Keep docs, schemas, manifest fragments, validation recipes, static checks, skills/rules, and `.clangd` synchronized only when durable behavior, public API, workflow, validation, packaging, or agent contracts change.
9. Run focused build/CTest/static loops first, then `tools/validate.ps1` once for C++/runtime/build/packaging/public-contract slice closeout.
10. Preserve all non-claims until exact evidence exists. Ready counters must never be promoted from adjacent backend evidence, fixtures, dry runs, or manual assertions.

## Capability Phases

### Phase 0: Selection And Fresh Audit

Goal: select this milestone only when appropriate and prove the starting point is still accurate.

- [x] Confirm the current active plan can be changed, or keep this plan as a candidate.
- [x] Re-run `tools/agent-context.ps1 -ContextProfile Minimal` and read only the manifest fragments needed for 2D/runtime/package/renderer/UI truth.
- [x] Re-run baseline validators:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-production-workloads.ps1 -RequireReady`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-package-playtest-productization.ps1 -RequireReady`
- [ ] Reconcile non-ready rows in `docs/current-capabilities.md`, `docs/roadmap.md`, `games/*/game.agent.json`, and relevant validation scripts.
- [ ] If selected, update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, compose `engine/agent/manifest.json`, and update the plan registry without changing readiness counters.

Done when: the selected starting state is evidence-backed and no broad 2D, renderer, UI, editor, network, audio, scripting, or optimization claim is made from plan selection alone.

Phase 0 validation evidence:

- 2026-06-29: planning audit confirmed `Renderer Commercial Readiness Evidence Promotion v1` remains active and this plan must stay candidate-only.
- 2026-06-29: `tools/agent-context.ps1 -ContextProfile Minimal`, `tools/check-production-readiness-audit.ps1`, `tools/validate-2d-production-workloads.ps1 -RequireReady`, and `tools/validate-2d-package-playtest-productization.ps1 -RequireReady` ran successfully for this worktree.

### Phase 1: Official-Source And Clean-Room Evidence Gate

Goal: make legal/originality constraints machine-checkable before deeper feature work.

- [x] Add or extend a first-party `TwoDCommercialProductionSourceReview` value contract under `MK_tools` or reuse the existing originality review if it already covers this exact scope.
- [x] Record Context7 verification rows plus official-source rows for Vulkan/D3D12/MSL checks, official platform docs, repository legal policy, and legal/trademark source URLs.
- [x] Add static guards that reject public engine headers, game/sample code and manifests, editor-core public headers, selected public schemas, and package-visible manifest rows using prohibited external-engine marks or compatibility claims.
- [x] Add fail-closed diagnostics for explicit retained markers and product-facing scanned surfaces that indicate copied code/assets/docs prose, external engine schema names, trademark-surface use, missing notice records, and unapproved dependency sources.
- [x] Generate counsel-ready clean-room input records without drawing legal conclusions.

Likely surfaces: `engine/tools/include/mirakana/tools/`, `engine/tools/asset/`, `tests/unit/`, `tools/check-ai-integration.ps1`, `tools/check-dependency-policy.ps1`, `docs/legal-and-licensing.md`, `docs/current-capabilities.md`, `THIRD_PARTY_NOTICES.md`.

Slice 1 validation evidence:

- 2026-06-29 scope: extended the existing `2D Originality Review v1` API with `review_2d_commercial_production_sources`, `official_source_ledger_ready`, `commercial_production_source_gate_ready`, and external-engine compatibility/equivalence/parity claim rejection. The broader static forbidden-token scan, dependency notice gate, and counsel-ready artifact generation remain open Phase 1 work.
- 2026-06-29 RED: `tools/cmake.ps1 --build --preset dev --target MK_tools_2d_originality_review_tests` failed because `review_2d_commercial_production_sources` and external-engine claim fields did not exist.
- 2026-06-29 GREEN: `tools/cmake.ps1 --build --preset dev --target MK_tools_2d_originality_review_tests` succeeded and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_2d_originality_review_tests` passed.
- 2026-06-29 review fix: C++ review found commercial diagnostics could leave `clean_room_ready` stale; `review_2d_commercial_production_sources` now recomputes it after commercial diagnostics and public result fields document which gate they represent.
- 2026-06-29 closeout: `tools/check-tidy.ps1 -Files 'engine/tools/asset/2d_originality_review.cpp,tests/unit/tools_2d_originality_review_tests.cpp'`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, `tools/check-dependency-policy.ps1`, `tools/check-format.ps1`, and full `tools/validate.ps1` passed after the review fix.
- 2026-06-30 scope: added `docs/specs/2026-06-30-2d-commercial-clean-room-source-ledger-v1.md`, `tools/check-2d-commercial-clean-room.ps1`, `tools/check-2d-commercial-clean-room-contract.ps1`, `tools/generate-2d-commercial-clean-room-review-input.ps1`, `schemas/2d-commercial-clean-room-review-input.schema.json`, and `schemas/2d-commercial-official-source-summary.schema.json` for official-source row retention, public/product-facing forbidden-token rejection, and counsel-ready review input generation.
- 2026-06-30 diagnostics: added `tools/check-2d-commercial-source-diagnostics.ps1` and `tools/check-2d-commercial-source-diagnostics-contract.ps1` for explicit copied-material, external-schema, trademark, missing-notice, unapproved-dependency, and external-engine claim diagnostics over retained markers and product-facing scanned surfaces. Successful counters are scoped to `retained_markers_and_public_surface_tokens` and do not claim legal clearance, plagiarism detection, or dependency approval.

Done when: implementation phases can prove `2d_commercial_source_diagnostics_scope=retained_markers_and_public_surface_tokens`, `external_code_copied_marker_rows=0`, `external_assets_copied_marker_rows=0`, `copied_documentation_text_marker_rows=0`, `external_engine_schema_surface_rows=0`, `third_party_trademark_public_surface_rows=0`, `missing_notice_marker_rows=0`, `unapproved_dependency_source_marker_rows=0`, `external_engine_compatibility_claim_rows=0`, `external_engine_equivalence_claim_rows=0`, `external_engine_parity_claim_rows=0`, `official_source_ledger_ready=1`, `commercial_production_source_gate_ready=1`, `2d_commercial_clean_room_public_docs_only=1`, and `requires_legal_counsel_review=1` without implying legal clearance.

### Phase 2: First-Party 2D Asset Conditioning And Cooking

Goal: move from selected package proof to a production-quality 2D asset conditioning path while keeping shipping runtime cooked-only.

- [x] Define first-party source asset manifests for sprites, sprite sheets, atlas pages, tile chunks, animation clips, audio cues, localization text, UI glyph inputs, and package provenance.
- [x] Build deterministic cook outputs with content hashes, dependency graph rows, incremental recook invalidation, source provenance, license status, package budget rows, and replay hashes.
- [x] Promote production atlas packing only after padding/bleed control, rotation policy, power-of-two policy where selected, multiple pages, deterministic placement, texture-format targets, mip policy, and package inclusion are validated.
- [x] Keep runtime source PNG/JPEG/audio/font parsing out of shipping runtime unless a separate dependency/license/security plan explicitly promotes it.
- [x] Add large-project fixtures that prove stable output under many assets, duplicate names, missing dependencies, invalid metadata, and asset deletion/rename churn.

Phase 2 validation evidence:

- 2026-06-30 source manifest contract: added `aiWorkflow.twoDCommercialSourceAssetManifests` / `2d-commercial-source-asset-manifests-v1` to `schemas/game-agent.schema.json`, `games/sample_2d_desktop_runtime_package/game.agent.json`, and `tools/check-json-contracts-076-2d-commercial-source-asset-manifests.ps1`. The descriptor records first-party source asset manifests for sprite sheets, atlas pages, tile chunks, animation clips, audio cues, localization text, UI glyph inputs, and package provenance. It keeps `runtime source parsing remains unsupported`, source decoding in reviewed tools/editor lanes, package mutation behind reviewed handoff, and broad commercial 2D readiness unclaimed.
- 2026-06-30 deterministic cook output contract: added `aiWorkflow.twoDCommercialDeterministicCookOutputs` / `2d-commercial-deterministic-cook-outputs-v1` to `schemas/game-agent.schema.json`, `games/sample_2d_desktop_runtime_package/game.agent.json`, and `tools/check-json-contracts-077-2d-commercial-deterministic-cook-outputs.ps1`. The descriptor records deterministic cook outputs, content hashes, dependency graph rows, incremental recook invalidation, source provenance, license status, package budget rows, and replay hashes for selected reviewed 2D runtime package files. It keeps `runtime package mutation remains unsupported`, runtime source parsing unsupported, nondeterministic cook output unsupported, and broad commercial 2D readiness unclaimed.
- 2026-06-30 production atlas packing contract: added `aiWorkflow.twoDCommercialProductionAtlasPacking` / `2d-commercial-production-atlas-packing-v1` to `schemas/game-agent.schema.json`, `games/sample_2d_desktop_runtime_package/game.agent.json`, and `tools/check-json-contracts-078-2d-commercial-production-atlas-packing.ps1`, with `ProductionSpriteAtlasPackingPolicy` plus `pack_production_sprite_atlas_rgba8` in `engine/assets`. The selected first-party production atlas packing proof records deterministic multi-page atlas placement, padding and bleed control, rotation policy disabled, power-of-two policy recorded where selected, `rgba8_unorm` texture format target, `base-level-only` mip policy, and package inclusion rows. `runtime source image decoding remains unsupported`, renderer/RHI residency remains unsupported, broad production atlas packing remains unsupported, broad commercial 2D readiness remains unclaimed, and Unity/Unreal Engine/Godot compatibility, equivalence, parity, replacement, endorsement, and legal approval remain unclaimed.
- 2026-06-30 runtime source parser exclusion contract: added `aiWorkflow.twoDCommercialRuntimeSourceParserExclusion` / `2d-commercial-runtime-source-parser-exclusion-v1` to `schemas/game-agent.schema.json`, `games/sample_2d_desktop_runtime_package/game.agent.json`, `engine/agent/manifest.fragments/014-gameCodeGuidance.json`, and `tools/check-json-contracts-079-2d-commercial-runtime-source-parser-exclusion.ps1`. The descriptor proves the shipping runtime consumes only reviewed cooked payloads by covering every `runtimePackageFiles` row, linking runtime files to deterministic cook outputs and production atlas package inclusion, rejecting `source/` paths plus PNG/JPEG/source audio/project font/source registry extensions in runtime package files, and requiring a separate dependency/license/security plan before any source parser is promoted. No third-party dependency or external engine material was added; Unity/Unreal Engine/Godot compatibility, equivalence, parity, replacement, endorsement, and legal approval remain unclaimed.
- 2026-06-30 large-project fixture contract: added `aiWorkflow.twoDCommercialLargeProjectFixtures` / `2d-commercial-large-project-fixtures-v1` to `schemas/game-agent.schema.json`, `games/sample_2d_desktop_runtime_package/game.agent.json`, `engine/agent/manifest.fragments/014-gameCodeGuidance.json`, and `tools/check-json-contracts-080-2d-commercial-large-project-fixtures.ps1`. The descriptor links source manifests, deterministic cook outputs, production atlas packing, runtime source parser exclusion, and runtime package files while recording a 512-asset first-party fixture, duplicate-name probes, missing-dependency probes, invalid-metadata probes, deletion/rename churn cycles, equal baseline/post-churn replay hashes, and fail-closed diagnostics. Runtime source parsing, external asset ingestion, package scripts, renderer/RHI residency, broad large-project readiness, broad commercial 2D readiness, external engine compatibility/equivalence/parity/replacement/endorsement, and legal approval remain unclaimed.

Likely surfaces: `engine/assets/include/mirakana/assets/`, `engine/tools/include/mirakana/tools/`, `engine/tools/asset/`, `engine/runtime/include/mirakana/runtime/`, `games/sample_2d_desktop_runtime_package/`, `schemas/game-agent.schema.json`, package tools, validation scripts.

Done when: selected 2D package cook output can be rebuilt deterministically from first-party source records, and the runtime consumes only reviewed cooked payloads with no source decoder or external-engine project import claim.

### Phase 3: Atlas, Tilemap, And Level Authoring UX

Goal: provide production-grade authoring models without copying third-party editor expression.

- [x] Add or extend first-party retained editor models for atlas inspection, tile-set review, tilemap chunk editing, collision overlay review, animation preview, package-diff review, and validation diagnostics.
- [x] Keep editor-core state value-only; visible shell widgets must use first-party `mirakana_ui` / `mirakana::ui` and project styling.
- [x] Provide undo/redo revision safety, safe package mutation previews, selected package smoke review, and rejection diagnostics before applying any package update.
- [x] Add large-scene navigation and filtering primitives for asset browser/import flows if missing.
- [x] Prove authoring workflows using first-party sample content and generated fixtures only.

Likely surfaces: `editor/core/include/mirakana/editor/`, `editor/core/`, `editor/tests/`, `engine/ui/`, `games/sample_2d_desktop_runtime_package/`, editor validation scripts.

Phase 3 validation evidence:

- 2026-06-30 retained authoring review model: added `Editor2DCommercialAuthoringReviewStatus`, `Editor2DCommercialAuthoringReviewSurface`, `Editor2DCommercialAuthoringReviewRowInput`, `Editor2DCommercialAuthoringReviewRow`, `Editor2DCommercialAuthoringReviewDesc`, `Editor2DCommercialAuthoringReviewModel`, `make_editor_2d_commercial_authoring_review_model`, and `make_editor_2d_commercial_authoring_review_ui_model` in `MK_editor_core`. The value-only retained model aggregates atlas inspection, tile-set review, tilemap chunk editing, collision overlay review, animation preview, package-diff review, and validation diagnostics rows from existing sprite/tilemap/preflight diagnostics plus explicit first-party review inputs. It rejects missing retained rows, duplicate retained ids, input mutation/execution/native-handle claims, package mutation, validation execution, runtime source parsing, renderer/RHI handle exposure, native handle exposure, external engine project import, and external engine API parity claims. It does not apply package updates, execute validation, expose native handles, promote runtime source parsing, add visible editor widgets, or claim broad commercial 2D readiness.
- 2026-06-30 RED: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` failed on missing `mirakana/editor/two_d_commercial_authoring_review.hpp`.
- 2026-06-30 GREEN: `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests`, and `tools/ctest.ps1 --preset dev -R MK_editor_core_tests --output-on-failure` passed for the retained authoring review model.
- 2026-06-30 closeout: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files 'editor/core/src/two_d_commercial_authoring_review.cpp,tests/unit/editor_core_tests.cpp'`, `git diff --check`, and full `tools/validate.ps1` passed.
- 2026-06-30 package update review model: added `Editor2DCommercialPackageUpdateReviewStatus`, `Editor2DCommercialPackageSmokeReviewStatus`, `Editor2DCommercialPackageUpdateSmokeReviewInput`, `Editor2DCommercialPackageUpdateRejectionDiagnosticInput`, `Editor2DCommercialPackageUpdateReviewDesc`, `Editor2DCommercialPackageUpdatePreviewRow`, `Editor2DCommercialPackageUpdateSmokeReviewRow`, `Editor2DCommercialPackageUpdateRejectionDiagnosticRow`, `Editor2DCommercialPackageUpdateReviewModel`, `make_editor_2d_commercial_package_update_review_model`, `make_editor_2d_commercial_package_update_review_ui_model`, and `make_editor_2d_commercial_package_update_apply_action` in `MK_editor_core`. The value-only review model requires a matching manifest revision, safe reviewed `runtimePackageFiles` mutation preview rows, selected package smoke evidence, complete rejection diagnostics, and no package-script/validation/runtime-source/native-handle/external-engine/parity claims before creating an undoable apply action. The action reuses `ScenePackageRegistrationApplyPlan` and `apply_scene_package_registration_to_manifest`, captures before/after manifest text, and is executable through `UndoStack` for redo/undo. It does not execute validation recipes or package scripts, parse runtime sources, expose native handles, import external engine projects, add visible editor widgets, or claim broad commercial 2D readiness/legal approval.
- 2026-06-30 RED: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` failed on missing `mirakana/editor/two_d_commercial_package_update_review.hpp`.
- 2026-06-30 GREEN: `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests`, and `tools/ctest.ps1 --preset dev -R MK_editor_core_tests --output-on-failure` passed for the package update review model.
- 2026-06-30 closeout: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files 'editor/core/src/two_d_commercial_package_update_review.cpp,tests/unit/editor_core_tests.cpp'`, `git diff --check`, and full `tools/validate.ps1` passed.
- 2026-06-30 large-scene asset navigation primitive: added `ContentBrowserNavigationAction`, `ContentBrowserNavigationRequest`, `ContentBrowserNavigationRow`, `ContentBrowserNavigationModel`, `kContentBrowserNavigationMaxPageSize`, and `plan_content_browser_navigation` in `MK_editor_core`. The value-only planner pages existing filtered `ContentBrowserState` rows, clamps unsafe page sizes and offsets, finds the selected visible asset, exposes first/previous/next/last/selected-page navigation state, and reports diagnostics without mutating browser state or executing import tools. It does not add visible shell productization, import execution, package mutation, runtime package streaming, native handles, external engine import, or broad commercial 2D readiness/legal approval.
- 2026-06-30 RED: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` failed on missing `plan_content_browser_navigation` / `ContentBrowserNavigationRequest` / `ContentBrowserNavigationAction`.
- 2026-06-30 GREEN: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` and `tools/ctest.ps1 --preset dev -R MK_editor_core_tests --output-on-failure` passed for the large-scene asset navigation primitive.
- 2026-06-30 overflow RED/GREEN: an added `std::numeric_limits<std::size_t>::max()` next-page request first failed `tools/ctest.ps1 --preset dev -R MK_editor_core_tests --output-on-failure`, then passed after saturating large page-offset addition before final-page clamping.
- 2026-06-30 closeout: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files 'editor/core/src/content_browser.cpp,tests/unit/editor_core_tests.cpp'`, `git diff --check`, and full `tools/validate.ps1` passed.
- 2026-06-30 authoring workflow proof: added `Editor2DCommercialAuthoringWorkflowProofStatus`, `Editor2DCommercialAuthoringWorkflowProofSourceOrigin`, `Editor2DCommercialAuthoringWorkflowProofSourceRowInput`, `Editor2DCommercialAuthoringWorkflowProofSourceRow`, `Editor2DCommercialAuthoringWorkflowProofDesc`, `Editor2DCommercialAuthoringWorkflowProofModel`, `make_editor_2d_commercial_authoring_workflow_proof_model`, and `make_editor_2d_commercial_authoring_workflow_proof_ui_model` in `MK_editor_core`. The value-only proof aggregates 2D commercial authoring review, package update review, large-scene asset navigation, and `review_production_authoring_workflow` evidence while requiring first-party sample or generated fixture source rows. It rejects external assets, external-engine project/schema rows, copied material, command/file/package IO execution, runtime source parsing, native handles, external engine API parity, and legal-approval claims.
- 2026-06-30 RED: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` failed on missing `mirakana/editor/two_d_commercial_authoring_workflow_proof.hpp`.
- 2026-06-30 GREEN: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` and `tools/ctest.ps1 --preset dev -R MK_editor_core_tests --output-on-failure` passed for the authoring workflow proof model.
- 2026-06-30 closeout: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files 'editor/core/src/two_d_commercial_authoring_workflow_proof.cpp,tests/unit/editor_core_tests.cpp'`, `git diff --check`, and full `tools/validate.ps1` passed.

Done when: a reviewer can inspect, edit, validate, and package a non-trivial 2D atlas/tilemap scene through first-party editor models and retained evidence without external engine UI/layout/schema influence.

### Phase 4: Cooked Package Streaming And Residency

Goal: turn descriptor-only or selected safe-point package proof into measured long-running cooked package residency behavior.

- [x] Add package cache high-water limits, LRU/recency policy, eviction blockers, resident payload descriptors, and safe-point replacement rules.
- [x] Implement background read scheduling only through reviewed queue boundaries with deterministic safe-point application on the main/runtime-owned boundary.
- [x] Add package pop-in, upload-byte, IO-byte, decompression-time, CPU-time, GPU-upload, memory high-water, and asset-miss counters.
- [x] Keep package scripts, arbitrary shell execution, network fetches, encryption/auth, and external process launches out of this phase.
- [x] Run long-running package playtest workloads with retained artifacts and fail-closed over-budget rows.

Likely surfaces: `engine/runtime/`, `engine/tools/asset/`, `engine/platform/`, package validators, performance artifacts under ignored/retained artifact roots as appropriate.

Phase 4 validation evidence:

- 2026-06-30 package residency policy slice: added `RuntimePackageResidencyPolicyStatus`, `RuntimePackageResidencyTelemetryRow`, `RuntimePackageResidencyPolicyDesc`, `RuntimePackageResidencyPolicyPlan`, and `plan_runtime_package_residency_policy` in `MK_runtime`. The helper is value-only over all already-mounted cooked packages, including overlay-overridden resident payloads: it reports resident byte/record/package counters, caller-supplied telemetry counters, LRU candidate order from reviewed `last_touched_frame` rows, protected eviction blockers, and the minimum recommended mount ids needed to return within high-water limits. It fail-closes missing/duplicate/unknown telemetry and unsupported execution requests while keeping package file reads, background IO, package scripts, external processes, runtime source parsing, renderer/RHI residency, native handles, broad background streaming, long-run performance readiness, commercial 2D readiness, external-engine compatibility/equivalence/parity/replacement, and legal approval unclaimed.
- 2026-06-30 focused validation: RED `tools/cmake.ps1 --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests` failed before the API existed; GREEN `tools/cmake.ps1 --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_package_streaming_resident_mount_tests` passed.
- 2026-06-30 package background read queue slice: added `RuntimePackageBackgroundReadQueueDesc`, `RuntimePackageBackgroundLoadedRow`, `RuntimePackageBackgroundReadQueueResult`, `RuntimePackageBackgroundReadServiceState`, `RuntimePackageBackgroundReadServiceTickDesc`, `RuntimePackageBackgroundReadServiceTickResult`, `dispatch_runtime_package_background_reads`, and `tick_runtime_package_background_read_service` in `MK_runtime`. The queue validates reviewed `RuntimePackageStreamingExecutionDesc` rows before worker dispatch, performs only `load_runtime_package_candidate_v2` package candidate file IO through caller-owned `JobExecutionPool` tasks, returns deterministic input-order loaded rows, and leaves later mount/replace/unmount application to existing main/runtime-owned safe-point APIs. The service retains pending descriptors across ticks, coalesces duplicate target/package rows by replacing stale descriptor evidence, and dispatches bounded rows without claiming autonomous scheduling. It keeps live resident mount sets, catalogs, package scripts, external processes, runtime source parsing, renderer/RHI/native handles, DirectStorage, GPU memory pressure, broad async-overlap/performance proof, broad background package streaming, commercial 2D readiness, external-engine compatibility/equivalence/parity/replacement, and legal approval unclaimed.
- 2026-06-30 package background read queue validation: RED `tools/cmake.ps1 --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests` failed on missing `dispatch_runtime_package_background_reads`, `RuntimePackageBackgroundReadQueueDesc`, `RuntimePackageBackgroundReadServiceState`, and `tick_runtime_package_background_read_service`; GREEN `tools/cmake.ps1 --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_package_streaming_resident_mount_tests` passed.
- 2026-06-30 package playtest long-run evidence slice: extended `Runtime2DPackagePlaytestRecipeRow`, `Runtime2DPackagePlaytestEvidenceRow`, `Runtime2DPackagePlaytestResult`, and `Runtime2DPackagePlaytestFailureClassification` so the selected package playtest productization gate imports long-run frame count, over-budget frame count, memory high-water, and retained profile artifact hash evidence. The planner now fails closed on missing playtest recipes, below-required long-run frames, over-budget long-run frames, missing memory high-water evidence, and missing retained profile artifact hash evidence while preserving non-claims for broad long-running readiness, active-session hot reload, package scripts, editor-core execution, arbitrary shell execution, native handles, external-engine compatibility/equivalence/parity/replacement, and legal approval.
- 2026-06-30 package playtest long-run evidence validation: RED `tools/cmake.ps1 --build --preset dev --target MK_runtime_package_hot_reload_candidate_review_tests` failed before the new long-run fields/classifications existed; GREEN `tools/cmake.ps1 --build --preset dev --target MK_runtime_package_hot_reload_candidate_review_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_package_hot_reload_candidate_review_tests`, and `tools/validate-2d-package-playtest-productization.ps1 -RequireReady` passed. The installed package smoke required `--require-runtime-ui-renderer-atlas-handoff`, `--require-sandbox-package-budgets`, `--require-performance-baseline`, and `--require-long-run-performance-readiness` before accepting the productization evidence.
- 2026-06-30 package background read queue closeout: `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files 'engine/runtime/src/package_streaming.cpp,tests/unit/runtime_package_streaming_resident_mount_tests.cpp'`, `git diff --check`, and full `tools/validate.ps1` passed. Full validation included all 160 CTest tests passing.
- 2026-06-30 package residency telemetry budget slice: extended `RuntimePackageResidencyTelemetryRow`, `RuntimePackageResidencyPolicyDesc`, and `RuntimePackageResidencyPolicyPlan` with explicit decompression-time, memory high-water, aggregate telemetry budgets, and `RuntimePackageResidencyPolicyStatus::telemetry_budget_exceeded`. `plan_runtime_package_residency_policy` now saturates unsigned telemetry aggregates at numeric limits, records memory high-water as a maximum, and fail-closes over-budget IO, decompressed bytes, decompression time, CPU time, GPU upload bytes, memory high-water, asset misses, and pop-in rows before any ready residency result. Focused RED/GREEN evidence used `MK_runtime_package_streaming_resident_mount_tests`, including over-budget and numeric-limit saturation coverage.
- 2026-06-30 package residency telemetry budget closeout: `tools/cmake.ps1 --build --preset dev --target MK_runtime_package_streaming_resident_mount_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_package_streaming_resident_mount_tests`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, `tools/check-tidy.ps1 -Files 'engine/runtime/src/package_streaming.cpp,tests/unit/runtime_package_streaming_resident_mount_tests.cpp'`, `git diff --check`, and full `tools/validate.ps1` passed. Full validation included all 160 CTest tests passing.

Done when: selected 2D packages can stream cooked payloads under measured CPU/memory/IO/GPU-upload budgets with deterministic eviction and no runtime source mutation or package-script execution.

### Phase 5: Runtime UI, Text, IME, Accessibility, And Localization Closeout

Goal: close the 2D game UI production gap through official platform adapters and first-party UI contracts.

- [x] Promote real text shaping/font fallback/glyph rasterization/glyph atlas upload through audited dependency or official platform adapter decisions.
- [x] Add IME, keyboard layout, text input, caret/selection, focus traversal, controller navigation, screen-reader/accessibility publication, high contrast, and localization evidence where selected.
- [x] Keep first-party widget contracts and UI styling; do not vendor UI middleware or copy external engine inspector/game UI patterns.
- [x] Add package-visible UI smoke scenes with multiple languages, glyph fallback, long labels, controller-only operation, and accessibility tree review.
- [x] Separate Windows-ready proof from Linux/macOS/mobile proof unless each platform host lane supplies evidence.

Likely surfaces: `engine/ui/`, `engine/platform/`, `editor/core/`, runtime package samples, UI/text validators, docs/legal/dependency records if a shaping/raster dependency is selected.

Done when: selected 2D package UI is usable under real text/input/accessibility constraints on the claimed host class, and all other platforms remain host-gated or unclaimed.

Phase 5 validation evidence:

- 2026-06-30 Runtime UI Package Smoke Scenes v1 slice: added `RuntimeUiPackageSmokeSceneKind`, `RuntimeUiPackageSmokeSceneRow`, `RuntimeUiPackageSmokeSceneReview`, `RuntimeUiPackageSmokeSceneDiagnosticCode`, `runtime_ui_package_smoke_scene_kind_name`, and `review_runtime_ui_package_smoke_scenes` in `MK_ui`. The value-only review requires selected ready multilingual glyph fallback, long-label wrapping, controller-only navigation, accessibility tree review, and supporting evidence rows; rejects native handles, UI middleware claims, external-engine compatibility claims, and row-budget overflow; and does not add dependencies or claim broad runtime UI parity.
- 2026-06-30 selected package integration: `sample_2d_desktop_runtime_package` aggregates explicitly requested first-party standard widget, widget vocabulary, binding/input-routing, workbench, platform production, Windows DirectWrite, Windows TSF, Windows UIA, D3D12 atlas upload, and renderer atlas handoff evidence into four selected package-visible scene rows with zero unsafe claim rows. `runtime_ui_controller_glyph_refs` is derived from `RuntimeUiBindingPlan` instead of a hard-coded sample counter. Non-Windows UI readiness, native candidate UI, full screen-reader parity, UI middleware, native handles, and Unity/Unreal/Godot compatibility/parity remain unclaimed.
- 2026-06-30 closeout validation: `tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_package_smoke_scene_tests MK_runtime_ui_binding_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_(package_smoke_scene|binding)_tests"`, `tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package`, source-tree `sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex` plus explicit `--require-runtime-ui-standard-widgets`, `--require-runtime-ui-widgets`, `--require-runtime-ui-binding`, `--require-runtime-ui-workbench`, `--require-runtime-ui-production-stack`, `--require-runtime-ui-platform-package`, `--require-runtime-ui-font-rasterization`, `--require-runtime-ui-tsf-session`, `--require-runtime-ui-uia-publication`, `--require-runtime-ui-atlas-upload`, `--require-runtime-ui-renderer-atlas-handoff`, and `--require-runtime-ui-package-smoke-scenes`, `tools/check-tidy.ps1 -Files 'engine/ui/src/runtime_ui_binding.cpp,engine/ui/src/runtime_ui_package_smoke_scene.cpp,tests/unit/runtime_ui_binding_tests.cpp,tests/unit/runtime_ui_package_smoke_scene_tests.cpp'`, `tools/check-tidy.ps1 -Preset desktop-runtime -Files 'games/sample_2d_desktop_runtime_package/main.cpp'`, `tools/validate-2d-production-workloads.ps1 -RequireReady`, and full `tools/validate.ps1` passed. The installed 2D workload gate reported `runtime_ui_package_smoke_scene_ready=1`, four selected ready rows, `runtime_ui_package_smoke_scene_supporting_evidence_rows=10`, zero unsafe claim rows, and `runtime_ui_renderer_atlas_handoff_ready=1`, with explicit prerequisite flags guarded by `tools/check-ai-integration.ps1`.
- 2026-07-01 Phase 5 closeout: added `TwoDCommercialRuntimeUiOfficialSourceKind`, `TwoDCommercialRuntimeUiOfficialSourceRow`, `TwoDCommercialRuntimeUiCloseoutDesc`, `TwoDCommercialRuntimeUiCloseoutResult`, and `evaluate_2d_commercial_runtime_ui_closeout` in `MK_ui`. The value contract aggregates the selected `RuntimeUiPlatformProductionResult`, selected package-visible `RuntimeUiPackageSmokeSceneReview`, refreshed Context7/official-source rows for DirectWrite, Win32 TSF/UIA, D3D12, and repository legal policy, and the exact runtime UI adapter gate rows. It can become ready only for the selected Windows/D3D12 proof with DirectWrite text/font/rasterization, TSF IME/text input, UIA accessibility publication, D3D12 glyph/image atlas upload, first-party widget/package smoke, and clean-room non-claims; it rejects public native handles, UI middleware, external-engine compatibility, cross-platform parity, legal-approval claims, non-Windows ready claims, and any promoted Linux/macOS/iOS/Android/Vulkan/Metal adapter gate.
- 2026-07-01 RED/GREEN: `tools/cmake.ps1 --build --preset dev --target MK_two_d_commercial_runtime_ui_closeout_tests` first failed on missing `mirakana/ui/two_d_commercial_runtime_ui_closeout.hpp`, then passed after adding the value contract, source, CMake source entry, and focused tests. `tools/ctest.ps1 --preset dev --output-on-failure -R MK_two_d_commercial_runtime_ui_closeout_tests` passed.

### Phase 6: Renderer/RHI 2D Commercial Quality

Goal: prove high-quality 2D rendering through backend-local official practices and retained artifacts.

- [ ] For D3D12, prove debug-layer-clean command list/allocator/fence/resource-barrier/descriptor-heap/PSO patterns with package-visible evidence.
- [ ] For Vulkan, prove `VK_KHR_synchronization2` or Vulkan 1.3 synchronization, validation-layer cleanliness, timestamp-query validity, descriptor/resource lifetime, and package/readback evidence.
- [ ] For Metal, prove Apple-host MSL compilation/package execution, resource/pipeline/capture evidence, memory/residency evidence where selected, and no cross-backend inference.
- [ ] Add sprite batching, material variants, atlas residency, overdraw diagnostics, texture upload scheduling, shader/pipeline cache, frame pacing, and present-latency evidence for 2D workloads.
- [ ] Keep public native handles hidden and backend readiness rows independent.

Likely surfaces: `engine/renderer/`, `engine/rhi/`, `engine/runtime_scene_rhi/`, shaders, renderer validation scripts, GitHub Actions host lanes, `games/sample_2d_desktop_runtime_package/`.

Done when: each claimed backend has its own official-doc-aligned host proof, retained metrics, package-visible evidence, and no broad renderer/commercial readiness promotion from missing or adjacent evidence.

Phase 6 validation evidence:

- 2026-07-01 2D Commercial Renderer/RHI Quality Gate v1 slice: added `TwoDCommercialRendererRhiQualityOfficialSourceKind`, `TwoDCommercialRendererRhiQualityOfficialSourceRow`, `TwoDCommercialRendererRhiQualityEvidenceRow`, `TwoDCommercialRendererRhiQualityDesc`, `TwoDCommercialRendererRhiQualityResult`, and `evaluate_2d_commercial_renderer_rhi_quality` in `MK_renderer`. The value-only gate can report ready only for the selected D3D12 scope after refreshed Microsoft D3D12, Khronos Vulkan, Apple Metal, and repository legal policy source rows, selected D3D12 command allocator/list/fence, descriptor heap, PSO reuse, resource barrier, debug validation, timestamp/PIX, package readback, sprite throughput, atlas residency/upload scheduling, frame pacing, claim-control, and clean-room rows are all ready. It rejects stale official-source rows, public native handles, cross-backend Vulkan/Metal inference, broad backend/renderer readiness claims, external Unity/Unreal Engine/Godot code/sample/asset/trademark/compatibility rows, and legal-approval claims. This does not execute GPU commands, native captures, package scripts, shell commands, or promote Vulkan/Metal/broad renderer commercial readiness.
- 2026-07-01 RED/GREEN: `tools/cmake.ps1 --build --preset dev --target MK_2d_renderer_rhi_quality_tests` first failed on missing `mirakana/renderer/two_d_commercial_renderer_rhi_quality.hpp`, then passed after adding the value contract, source, renderer CMake source entry, root CMake test target, and focused tests. `tools/ctest.ps1 --preset dev --output-on-failure -R MK_two_d_commercial_renderer_rhi_quality_tests` passed.

### Phase 7: Gameplay Runtime Depth

Goal: raise 2D gameplay capability beyond package proof without creating broad false claims.

- [ ] For physics, add exact first-party 2D execution extensions only where missing: TOI/CCD where selected, joints/constraints, trigger/area evidence, kinematic contact resolution, deterministic replay, and package counters.
- [ ] For input, prove actions, remapping data, device profiles, controller/mouse/keyboard/touch class handling, symbolic glyph keys, multiplayer device assignment where selected, and accessibility-friendly navigation.
- [ ] For audio, prove real playback/mixing/streaming/latency/device hotplug/spatialization rows only when host evidence exists.
- [ ] For networking, keep optional transport adapters separate from production online/matchmaking/NAT/security claims; add rollback/prediction only through a dedicated deterministic simulation plan.
- [ ] For scripting/modding, keep the default runtime sandbox denied for filesystem/network/process/native-plugin access; any Lua/WASM/JIT or user-script execution requires a separate security/dependency/license plan.

Likely surfaces: `engine/runtime/`, `engine/input/`, `engine/audio/`, `engine/physics/`, `engine/network/`, `engine/scripting/`, sample game manifests, validators.

Done when: selected 2D gameplay depth is proven by deterministic tests and package-visible counters, while broad online/audio/scripting/physics parity remains unclaimed unless exact evidence exists.

### Phase 8: Performance, Optimization, And Regression Gates

Goal: make "optimized" claims evidence-based, repeatable, and backend/host-specific.

- [ ] Define fixed 2D workload scenes: dense sprites, large tilemaps, UI-heavy scene, animation-heavy scene, streaming stress, physics stress, audio/input stress, and long-running playtest.
- [ ] Track CPU frame time, GPU frame time, input-to-present latency, present pacing, IO/decompression/upload overlap, memory high-water, GPU residency pressure, allocator churn, job queue depth, cache misses where available, and package miss/pop-in counters.
- [ ] Add p50/p95/p99 thresholds and fail-closed over-budget diagnostics. Keep thresholds host-class-specific.
- [ ] Add official profiler artifact collectors: Windows WPR/WPA/PIX where selected, Vulkan timestamp/debug-utils evidence, Apple `xctrace`/Metal capture evidence, and Linux perf where selected.
- [ ] Keep PGO/LTO and release optimization lanes separate from debug/dev validation, with reproducible compiler flags and artifact metadata.

Likely surfaces: performance specs, `tools/validate-*performance*.ps1`, CI workflows, artifacts, `engine/performance/`, `docs/specs/`, `docs/current-capabilities.md`.

Done when: any optimization claim has retained before/after or budget evidence tied to exact workload/backend/host IDs, and regressions fail in a targeted validator.

### Phase 9: Packaging, Distribution, And Legal Release Gate

Goal: prepare a counsel-ready and distribution-ready 2D package path without overstating legal conclusions.

- [ ] Validate package contents, notices, dependency manifests, source/provenance summaries, static clean-room guards, trademark-surface guards, and distribution artifact inventory.
- [ ] Add package signing/notarization/store-certification planning only through platform-specific official docs and separate host gates.
- [ ] Generate retained legal review input records from official-source tables, dependency records, third-party notice records, and clean-room static-check summaries.
- [ ] Keep legal output phrased as "engineering review input" and "requires counsel review"; do not claim legal approval.
- [ ] Add release blockers for missing notices, unapproved dependencies, external engine marks, copied assets, unknown license rows, and unreviewed generated assets.

Likely surfaces: packaging scripts, `docs/legal-and-licensing.md`, `docs/dependencies.md`, `THIRD_PARTY_NOTICES.md`, `tools/check-dependency-policy.ps1`, release validators, package manifests.

Done when: the selected 2D release package has a complete engineering legal/dependency/provenance handoff and blocks release on unresolved license or originality risks.

### Phase 10: Closeout And Promotion

Goal: promote only exact, evidence-backed claims and leave every other claim fail-closed.

- [ ] Update `docs/current-capabilities.md`, `docs/roadmap.md`, plan registry, game manifests, validation recipes, schemas, static checks, and manifest fragments only for completed and validated surfaces.
- [ ] Compose `engine/agent/manifest.json` if fragments change.
- [ ] Run targeted public API, package, JSON, agent-surface, legal/dependency, and static guards.
- [ ] Run `tools/validate.ps1` for runtime/build/public-contract changes.
- [ ] Record remaining host-gated rows, dependency-gated rows, unsupported broad claims, and recommended next plan.

Done when: selected ready counters match retained evidence, no stale plan/manifest/docs drift exists, and broad 2D/commercial/all-platform claims remain unclaimed unless all exact gates are satisfied.

## Validation Matrix

Use focused validators during each phase, then close with the full gate when C++/runtime/build/packaging/public-contract behavior changes.

| Area | Required command or evidence |
| --- | --- |
| Toolchain | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` |
| Configure/build/test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev`; targeted `tools/ctest.ps1 --preset dev --output-on-failure` |
| Current 2D readiness baseline | `tools/check-production-readiness-audit.ps1`; `tools/validate-2d-production-workloads.ps1 -RequireReady`; `tools/validate-2d-package-playtest-productization.ps1 -RequireReady` |
| Legal/dependency | `tools/check-dependency-policy.ps1`; `tools/check-ai-integration.ps1`; clean-room/static guards added by the selected phase |
| Manifest/schema | `tools/compose-agent-manifest.ps1 -Write` when fragments change; `tools/check-json-contracts.ps1` |
| Public API/contracts | `tools/check-public-api-boundaries.ps1` where applicable; targeted C++ unit tests |
| Formatting/static | `tools/check-format.ps1`; `tools/check-tidy.ps1 -Files <changed C++ files>` where applicable |
| Package/runtime | Existing or new `tools/validate-2d-*.ps1`, package smoke validators, installed package validators, retained package artifacts |
| Renderer/RHI | Backend-specific D3D12, strict Vulkan, Apple Metal, Metal memory/profiling, and renderer commercial readiness validators as selected |
| Performance | Workload-specific retained profiler/trace artifacts, budget validators, p95/p99/high-water gates |
| Slice closeout | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` for C++/runtime/build/packaging/public-contract slices |

## Done When

This candidate milestone is complete only when:

- Every selected phase has tests, implementation, docs, manifest/schema/static-check updates, and retained validation evidence.
- All public APIs and package-visible row ids are first-party `mirakana` concepts with no external-engine compatibility, equivalence, or trademark surface.
- Runtime shipping paths consume cooked reviewed package data by default.
- Renderer, UI, audio, network, scripting, physics, platform, and performance claims are promoted only for exact host/backend/dependency evidence.
- Legal/dependency records are complete for every third-party dependency or distributable asset, and legal review output remains counsel-ready engineering evidence rather than legal advice.
- `docs/current-capabilities.md`, `docs/roadmap.md`, plan registry, `engine/agent/manifest.fragments/`, composed `engine/agent/manifest.json`, schemas, validation recipes, and game manifests agree.
- Full validation or an explicit host/dependency blocker is recorded for every touched surface.

## Explicit Non-Goals

- No Unity, Unreal Engine, Godot, GameMaker, Defold, SDL, Dear ImGui, or middleware compatibility layer.
- No external engine project import, conversion, schema reader, package reader, shader reader, Blueprint/visual-script reader, or scene/resource reader.
- No copied source, documentation prose, shaders, samples, UI layouts, icons, fonts, themes, starter content, marketplace assets, or screenshots.
- No public native handles.
- No broad all-platform readiness from one platform.
- No broad renderer/RHI quality from one backend.
- No broad production UI/text/accessibility from value-only rows.
- No online/matchmaking/NAT/rollback/modding/JIT/scripting production claim without a separate security and host-evidence plan.
- No legal approval claim without qualified counsel.
