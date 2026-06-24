# Original 2D Commercial Authoring And Live Iteration v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party, original, commercial-grade 2D authoring and live-iteration loop for MIRAIKANAI that connects reviewed source asset changes, recook, runtime package replacement, editor review rows, package smoke counters, and legal-clean-room evidence without copying Unity, Unreal Engine, Godot, or their assets, UI, source, samples, names, schemas, editor layouts, or trade dress.

**Architecture:** Keep the feature as a reviewed value-model and safe-point execution stack. `MK_platform` owns file-watcher backends and native handles. `MK_assets` owns source asset snapshots, dependency invalidation, and recook scheduling. `MK_tools` owns the first-party `Source Pulse` review/execution API that bridges watcher events into the existing `execute_asset_runtime_package_hot_reload_registered_asset_watch_tick_safe_point`. `MK_editor_core` owns retained UI models and evidence rows. Runtime package mutation occurs only through existing reviewed resident package replacement safe points after runtime scene/package validation evidence succeeds. No engine compatibility loader, external scene importer, external engine API wrapper, or external engine project/schema reader is introduced.

**Tech Stack:** C++23, CMake `dev` preset, existing `MK_platform`, `MK_assets`, `MK_tools`, `MK_runtime`, `MK_editor_core`, `MK_ui`, repository PowerShell 7 scripts, existing desktop runtime package validation lanes, official OS SDK documentation, repository legal/dependency policy, and first-party test fixtures.

---

## Non-Negotiable Legal And Originality Boundaries

This section is an engineering compliance plan, not legal advice. Final commercial legal clearance requires qualified counsel review before public marketing, trademark use, or distribution terms are finalized.

Allowed:

- Use public official documentation only to identify common functional categories, official OS APIs, and license boundaries.
- Use the repository's existing first-party code, tests, docs, and generated sample assets.
- Use official platform APIs already allowed by repository policy, such as Windows `ReadDirectoryChangesW`, DirectWrite, TSF, and UI Automation, behind first-party adapters.
- Use Godot source only in a future separate licensed-dependency task that records MIT notice obligations. This plan does not use or distribute Godot code.

Forbidden:

- Do not copy Unity, Unreal Engine, or Godot source code, sample code, project files, templates, editor layouts, icons, fonts, default themes, documentation examples, screenshots, sample scenes, marketplace assets, or binary artifacts.
- Do not create public API names, file formats, serialized schemas, UI labels, command names, row ids, sample output, or product-facing claims that imply Unity, Unreal Engine, Godot, Nanite, Blueprint, Prefab, UXML, USS, SceneTree, TileMap compatibility, equivalence, certification, or endorsement.
- Do not parse Unity `.unity`, `.prefab`, `.asset`, `.meta`, Unreal `.uasset`, `.umap`, `.uproject`, Blueprint, Godot `.tscn`, `.tres`, `.godot`, or equivalent external engine project files.
- Do not use Unity, Unreal Engine, or Godot trademarks in `mirakana::` public identifiers, package-visible counters, product UI, samples, docs examples, or marketing copy except in internal legal/source-audit discussion.
- Do not vendor third-party UI middleware, game-engine code, imported assets, or marketplace content for this feature.

Legal-source conclusions to preserve in implementation evidence:

- The U.S. Copyright Office distinguishes unprotected ideas, systems, and methods from protected expression. This allows first-party implementation of common concepts while prohibiting copied expression.
- USPTO guidance treats product names/logos used to identify business offerings as trademark-sensitive, so external engine names stay out of public product/API surfaces.
- Unity and Unreal Engine materials are licensed technology/content under their own terms. This plan uses their legal/docs pages only as boundary/category references, never as implementation source.
- Godot is MIT/Expat licensed and commercially usable with notice obligations when redistributed, but this plan avoids copying Godot code/assets so it introduces no new Godot notice.

## Current Repository Baseline

- `tools/check-production-readiness-audit.ps1` reports `unsupported_gaps=0`, `host_gates_ready=12`, and `production-readiness-audit-check: ok`.
- `tools/validate-2d-production-workloads.ps1 -RequireReady` reports the selected 2D production workload matrix ready.
- `tools/validate-2d-package-playtest-productization.ps1 -RequireReady` reports `2d_package_playtest_productization_status=ready` and selected package/playtest counters ready.
- `engine/platform/include/mirakana/platform/file_watcher.hpp` already exposes polling and native backend choice rows.
- `engine/platform/include/mirakana/platform/windows_file_watcher.hpp`, `engine/platform/src/windows_file_watcher.cpp`, `tests/unit/platform_process_tests.cpp`, and `tests/unit/platform_native_file_watcher_tests.cpp` already prove native file change reporting without public native-handle exposure.
- `engine/tools/include/mirakana/tools/asset_runtime_package_hot_reload_tool.hpp` already exposes replacement and registered-watch safe-point helpers that recook and replace runtime resident packages without native file watching, package scripts, renderer/RHI ownership, or native-handle integration.
- `editor/core/include/mirakana/editor/playtest_package_review.hpp` already exposes playtest, runtime scene validation, evidence import, remediation, and package authoring review models.
- `engine/tools/include/mirakana/tools/production_authoring_workflows.hpp` already exposes value-only production authoring workflow review rows and fail-closed diagnostics.

## What This Plan Adds

The feature name is `Source Pulse`. It is a first-party MIRAIKANAI concept: a reviewed stream of source asset changes that can be promoted into a runtime package safe point after validation evidence.

New externally meaningful guarantees:

- A first-party originality/provenance review for 2D authoring sources and feature references.
- A deterministic `Source Pulse` bridge from existing `FileWatchEvent` rows into registered asset watch tick state.
- An editor-core 2D live-iteration review model that reports readiness, host gates, package validation evidence, source-pulse status, and unsupported claims.
- A selected desktop runtime package smoke counter proving reviewed active-session package replacement from a 2D source change.
- Static guards, manifest rows, and docs that keep legal boundaries and non-claims durable.

Non-goals:

- No Unity/Unreal/Godot compatibility.
- No import of external engine projects or scenes.
- No arbitrary shell execution, package script execution, raw manifest command evaluation, or editor-core mutation.
- No public native handles.
- No renderer/RHI ownership changes.
- No broad all-platform 2D editor readiness claim.
- No legal conclusion beyond engineering evidence and counsel-ready records.

## Phase 1: Clean-Room Source And Originality Gate

Goal: create a first-party evidence model that proves this feature is implemented from approved source categories and rejects copied/external-engine implementation inputs.

- [x] Add `engine/tools/include/mirakana/tools/2d_originality_review.hpp`.
- [x] Add `engine/tools/asset/2d_originality_review.cpp`.
- [x] Register the source in `engine/tools/asset/CMakeLists.txt`.
- [x] Add `tests/unit/tools_2d_originality_review_tests.cpp`.
- [x] Register `MK_tools_2d_originality_review_tests` in root `CMakeLists.txt`.

Public API shape:

```cpp
namespace mirakana {

enum class TwoDOriginalitySourceKind : std::uint8_t {
    first_party_design,
    official_documentation_category,
    official_platform_sdk,
    permissive_dependency_notice_record,
    prohibited_external_engine_code,
    prohibited_external_engine_asset,
    prohibited_external_engine_schema,
    prohibited_trademark_surface,
    unknown,
};

struct TwoDOriginalitySourceRow {
    std::string id;
    TwoDOriginalitySourceKind kind{TwoDOriginalitySourceKind::unknown};
    std::string source_uri;
    std::string usage_scope;
    bool copied_code{false};
    bool copied_asset{false};
    bool copied_documentation_text{false};
    bool public_surface_uses_external_engine_mark{false};
};

struct TwoDOriginalityReviewResult {
    std::vector<TwoDOriginalitySourceRow> accepted_rows;
    std::vector<TwoDOriginalitySourceRow> rejected_rows;
    std::vector<std::string> diagnostics;
    std::size_t official_documentation_category_rows{0};
    std::size_t official_platform_sdk_rows{0};
    std::size_t first_party_design_rows{0};
    std::size_t permissive_notice_rows{0};
    bool clean_room_ready{false};
    bool requires_legal_counsel_review{true};
};

[[nodiscard]] TwoDOriginalityReviewResult
review_2d_originality_sources(std::span<const TwoDOriginalitySourceRow> rows);

} // namespace mirakana
```

Tests:

- [x] `accepts_first_party_design_official_category_docs_and_platform_sdk_rows`.
- [x] `rejects_unity_unreal_godot_code_assets_schema_or_copied_docs`.
- [x] `rejects_external_engine_trademarks_in_public_surfaces`.
- [x] `requires_legal_counsel_review_even_when_engineering_gate_is_ready`.
- [x] `keeps_godot_mit_notice_as_future_notice_only_when_no_code_is_copied`.

Required counters for future package smoke:

- `2d_originality_review_status=ready`
- `2d_originality_review_ready=1`
- `2d_originality_review_first_party_design_rows>=1`
- `2d_originality_review_official_category_rows>=3`
- `2d_originality_review_platform_sdk_rows>=1`
- `2d_originality_review_rejected_external_engine_rows>=4`
- `2d_originality_review_copied_code_rows=0`
- `2d_originality_review_copied_asset_rows=0`
- `2d_originality_review_trademark_surface_rows=0`
- `2d_originality_review_requires_counsel_review=1`

Phase 1 validation evidence:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_2d_originality_review_tests` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_2d_originality_review_tests` PASS.
- No third-party code, assets, dependencies, license obligations, or legal notice files were added in Phase 1.

## Phase 2: Source Pulse Watch Bridge

Goal: connect existing platform file watcher evidence to existing asset recook/runtime replacement safe points without exposing native handles or adding autonomous unsafe mutation.

- [x] Add `engine/tools/include/mirakana/tools/2d_source_pulse.hpp`.
- [x] Add `engine/tools/asset/2d_source_pulse.cpp`.
- [x] Register the source in `engine/tools/asset/CMakeLists.txt`.
- [x] Add `tests/unit/tools_2d_source_pulse_tests.cpp`.
- [x] Register `MK_tools_2d_source_pulse_tests` in root `CMakeLists.txt`.

Public API shape:

```cpp
namespace mirakana {

enum class TwoDSourcePulseStatus : std::uint8_t {
    blocked,
    primed,
    no_ready_changes,
    recook_pending,
    committed,
    failed,
};

struct TwoDSourcePulseEventRow {
    AssetId asset;
    std::string path;
    FileWatchEventKind watch_event_kind{FileWatchEventKind::unknown};
    AssetHotReloadEventKind hot_reload_event_kind{AssetHotReloadEventKind::unknown};
    std::uint64_t previous_revision{0};
    std::uint64_t current_revision{0};
};

struct TwoDSourcePulseDesc {
    std::vector<FileWatchEvent> file_watch_events;
    bool native_file_watch_invoked{false};
    bool native_handle_exposed{false};
    bool request_autonomous_background_commit{false};
    bool request_package_script_execution{false};
    bool request_renderer_rhi_handles{false};
};

struct TwoDSourcePulsePlan {
    TwoDSourcePulseStatus status{TwoDSourcePulseStatus::blocked};
    std::vector<TwoDSourcePulseEventRow> event_rows;
    std::vector<std::string> diagnostics;
    bool native_file_watch_invoked{false};
    bool native_handle_exposed{false};
    bool safe_point_required{true};
};

[[nodiscard]] TwoDSourcePulsePlan plan_2d_source_pulse_events(const TwoDSourcePulseDesc& desc);

} // namespace mirakana
```

Implementation notes:

- Map only existing `FileWatchEventKind::{added, modified, removed}` into `AssetHotReloadEventKind`.
- Treat unknown, absolute, parent-segment, empty, or control-character paths as diagnostics.
- Keep native watcher objects in `MK_platform`; `MK_tools` receives copied value rows only.
- Set `native_file_watch_invoked=true` only when the caller supplies host-collected native watcher events; do not create or own native watchers in `MK_tools`.
- Reject `native_handle_exposed`, `request_autonomous_background_commit`, `request_package_script_execution`, and `request_renderer_rhi_handles`.

Tests:

- [x] `maps_native_or_polling_file_events_to_source_pulse_rows`.
- [x] `rejects_invalid_paths_and_unknown_event_kinds`.
- [x] `rejects_native_handle_exposure`.
- [x] `rejects_autonomous_background_commit`.
- [x] `rejects_package_script_and_renderer_rhi_requests`.
- [x] `does_not_construct_or_own_platform_watchers`.

Phase 2 validation evidence:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_2d_source_pulse_tests` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_2d_source_pulse_tests` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS.
- No third-party code, assets, dependencies, license obligations, or legal notice files were added in Phase 2.

## Phase 3: Reviewed Active-Session Package Replacement

Goal: turn a source pulse into a safe-point package replacement using existing asset recook and runtime resident package replacement helpers.

- [x] Extend `engine/tools/include/mirakana/tools/asset_runtime_package_hot_reload_tool.hpp` with a new value-only descriptor that accepts `TwoDSourcePulsePlan`.
- [x] Extend `engine/tools/asset/asset_runtime_package_hot_reload_tool.cpp`.
- [x] Extend `tests/unit/tools_runtime_hot_reload_package_tests.cpp`.

Public API shape:

```cpp
namespace mirakana {

struct TwoDSourcePulseRuntimeReplacementDesc {
    TwoDSourcePulsePlan source_pulse;
    AssetRuntimePackageHotReloadRegisteredAssetWatchTickDesc watch_tick;
    bool runtime_scene_validation_succeeded{false};
    bool operator_reviewed_safe_point{false};
    bool request_editor_core_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_active_session_without_safe_point{false};
};

struct TwoDSourcePulseRuntimeReplacementResult {
    TwoDSourcePulseStatus status{TwoDSourcePulseStatus::blocked};
    AssetRuntimePackageHotReloadRegisteredAssetWatchTickResult watch_tick;
    std::vector<std::string> diagnostics;
    bool invoked_runtime_replacement{false};
    bool committed{false};
    bool active_session_hot_reload{false};
    bool native_handle_exposed{false};
};

[[nodiscard]] TwoDSourcePulseRuntimeReplacementResult
execute_2d_source_pulse_runtime_replacement_safe_point(
    IFileSystem& filesystem,
    const AssetRegistry& assets,
    const AssetDependencyGraph& dependencies,
    AssetRuntimePackageHotReloadRegisteredAssetWatchTickState& tick_state,
    AssetRuntimeReplacementState& replacements,
    runtime::RuntimeResidentPackageMountSetV2& mount_set,
    runtime::RuntimeResidentCatalogCacheV2& catalog_cache,
    const TwoDSourcePulseRuntimeReplacementDesc& desc);

} // namespace mirakana
```

Rules:

- Require `runtime_scene_validation_succeeded=true`.
- Require `operator_reviewed_safe_point=true`.
- Require `source_pulse.safe_point_required=true`.
- Reject editor-core execution and arbitrary shell execution.
- Reject active-session replacement without safe point.
- Keep `active_session_hot_reload=true` only for a committed reviewed package replacement during an already-running runtime host lane; do not infer it from unit tests.

Tests:

- [x] `commits_source_pulse_after_scene_validation_and_operator_review`.
- [x] `blocks_without_runtime_scene_validation`.
- [x] `blocks_without_operator_reviewed_safe_point`.
- [x] `blocks_active_session_without_safe_point`.
- [x] `does_not_execute_editor_core_or_arbitrary_shell`.
- [x] `preserves_existing_recook_failure_and_runtime_replacement_failure_diagnostics`.

Phase 3 validation evidence:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_runtime_hot_reload_package_tests` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_runtime_hot_reload_package_tests` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS.
- No third-party code, assets, dependencies, license obligations, or legal notice files were added in Phase 3.

## Phase 4: Editor-Core 2D Live Iteration Review Model

Goal: expose the feature to the first-party editor core as retained review/evidence rows without executing commands or mutating files from editor core.

- [x] Extend `editor/core/include/mirakana/editor/playtest_package_review.hpp`.
- [x] Extend `editor/core/src/playtest_package_review.cpp`.
- [x] Extend `tests/unit/editor_core_tests.cpp`.

Public API shape:

```cpp
namespace mirakana::editor {

enum class Editor2DLiveIterationStageStatus : std::uint8_t {
    ready,
    blocked,
    host_gated,
    evidence_required,
};

struct Editor2DLiveIterationEvidenceRow {
    std::string id;
    Editor2DLiveIterationStageStatus status{Editor2DLiveIterationStageStatus::blocked};
    std::string source_model;
    std::vector<std::string> validation_recipe_ids;
    std::vector<std::string> host_gates;
    std::string diagnostic;
};

struct Editor2DLiveIterationReviewDesc {
    EditorPlaytestPackageReviewModel playtest_review;
    EditorRuntimeScenePackageValidationExecutionResult runtime_scene_validation;
    std::vector<Editor2DLiveIterationEvidenceRow> source_pulse_rows;
    bool request_mutation{false};
    bool request_validation_execution{false};
    bool request_arbitrary_shell_execution{false};
    bool request_package_script_execution{false};
    bool request_native_handle_exposure{false};
};

struct Editor2DLiveIterationReviewModel {
    std::vector<Editor2DLiveIterationEvidenceRow> rows;
    Editor2DLiveIterationStageStatus status{Editor2DLiveIterationStageStatus::blocked};
    bool ready_for_source_pulse_safe_point{false};
    bool mutates{false};
    bool executes{false};
    bool exposes_native_handles{false};
    std::vector<std::string> unsupported_claims;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] Editor2DLiveIterationReviewModel
make_editor_2d_live_iteration_review_model(const Editor2DLiveIterationReviewDesc& desc);

[[nodiscard]] mirakana::ui::UiDocument
make_editor_2d_live_iteration_review_ui_model(const Editor2DLiveIterationReviewModel& model);

} // namespace mirakana::editor
```

Retained UI ids:

- `2d_live_iteration.review`
- `2d_live_iteration.review.originality`
- `2d_live_iteration.review.runtime_scene_validation`
- `2d_live_iteration.review.source_pulse`
- `2d_live_iteration.review.package_replacement_safe_point`
- `2d_live_iteration.review.external_evidence`

Tests:

- [x] `reports_ready_when_originality_validation_source_pulse_and_safe_point_rows_are_ready`.
- [x] `keeps_editor_core_non_mutating_and_non_executing`.
- [x] `renders_retained_ui_rows`.
- [x] `rejects_package_script_arbitrary_shell_and_native_handle_claims`.
- [x] `keeps_host_gated_rows_host_gated_until_external_evidence_is_supplied`.

Phase 4 validation evidence (2026-06-24):

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` failed before implementation because `Editor2DLiveIterationEvidenceRow`, `Editor2DLiveIterationStageStatus`, `Editor2DLiveIterationReviewDesc`, and `make_editor_2d_live_iteration_review_model` were undefined.
- GREEN focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` PASS.
- GREEN focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` PASS.
- GREEN focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests` PASS.
- GREEN slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS.
- No third-party code, assets, dependencies, license obligations, external engine schemas, or legal notice files were added in Phase 4.

## Phase 5: Package And Installed Smoke Evidence

Goal: make the feature package-visible and installed-smoke visible for one selected 2D desktop runtime package.

- [x] Extend the selected 2D package sample under `games/sample_2d_desktop_runtime_package/` only if the package smoke currently owns the relevant counters there. If the current package smoke emits counters from tools scripts instead, update those scripts instead of game code.
- [x] Extend `tools/validate-2d-package-playtest-productization.ps1`.
- [x] Extend `tools/validate-2d-production-workloads.ps1` only if it owns the aggregate selected 2D workload readiness row.
- [x] Extend installed desktop runtime validators that already check `production_authoring_workflow_*` and `2d_package_playtest_productization_*` counters.
- [x] Add synthetic ignored evidence under `out/` only during validation; do not track generated artifacts.

Required counters:

- `2d_source_pulse_status=ready`
- `2d_source_pulse_ready=1`
- `2d_source_pulse_event_rows>=3`
- `2d_source_pulse_native_backend_rows>=1`
- `2d_source_pulse_polling_fallback_rows>=1`
- `2d_source_pulse_runtime_replacement_committed_rows=1`
- `2d_source_pulse_runtime_scene_validation_required=1`
- `2d_source_pulse_operator_safe_point_required=1`
- `2d_source_pulse_editor_core_execution=0`
- `2d_source_pulse_arbitrary_shell_execution=0`
- `2d_source_pulse_package_script_execution=0`
- `2d_source_pulse_native_handle_exposure=0`
- `2d_source_pulse_external_engine_schema_import=0`
- `2d_source_pulse_external_engine_asset_use=0`
- `2d_source_pulse_external_engine_code_use=0`

Validation expectations:

- Existing `2d_package_playtest_productization_status=ready` remains ready.
- Existing package smoke counters stay unchanged unless the added Source Pulse rows are explicitly selected.
- `active_session_hot_reload=0` remains unchanged in existing validators until this phase adds an exact selected active-session Source Pulse counter. If promoted, the old broad counter must be replaced with a narrower `2d_source_pulse_active_session_safe_point_replacement=1` and all broad hot-reload claims must remain `0`.

Phase 5 validation evidence (2026-06-24):

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-package-playtest-productization.ps1` failed before manifest/source updates because `validationRecipes` was missing `installed-2d-source-pulse-smoke`.
- GREEN focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-package-playtest-productization.ps1` PASS.
- GREEN package/installed smoke: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-package-playtest-productization.ps1 -RequireReady` PASS after the runtime recook path used the runtime package path while the package index kept the package-relative texture path.
- GREEN aggregate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-production-workloads.ps1` PASS.
- Source Pulse remains selected package evidence only: no tracked generated artifacts, third-party code, third-party assets, new dependencies, external engine schemas, package scripts, arbitrary shell execution, editor-core mutation, or native-handle exposure were added.

## Phase 6: Docs, Manifest, Static Guards, And Legal Records

Goal: make the new behavior durable for agents and repository validators without broadening claims.

- [x] Update `docs/current-capabilities.md` with exact Source Pulse capability and non-claims.
- [x] Update `docs/roadmap.md` only if this feature changes roadmap status.
- [x] Update `docs/legal-and-licensing.md` only if a new dependency, license obligation, or legal process rule is added. If no third-party code/assets enter the repo, record "no dependency/legal notice changes" in the plan closeout evidence instead.
- [x] Update `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md` only if a dependency is added. This plan should add none.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and `engine/agent/manifest.fragments/014-gameCodeGuidance.json` with exact Source Pulse support and non-claims.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [x] Extend `tools/check-ai-integration-020-engine-manifest.ps1`, `tools/check-ai-integration-040-agent-surfaces.ps1`, `tools/check-ai-integration-050-game-generation.ps1`, `tools/check-ai-integration-060-editor-workflows.ps1`, and `tools/check-ai-integration-070-production-ledger.ps1` only for exact durable literals introduced by this feature.
- [x] Extend `tools/check-json-contracts-010-engine-manifest.ps1` and `tools/check-json-contracts-040-agent-surfaces.ps1` if manifest or retained row ids require schema/static checks.
- [x] Update `.agents/skills/gameengine-feature/SKILL.md`, `.claude/skills/gameengine-feature/SKILL.md`, and `.cursor/skills/gameengine-feature/SKILL.md` only if reusable workflow behavior changes. Do not touch skills for simple capability evidence.

Manifest wording must include:

- Source Pulse is a first-party MIRAIKANAI 2D authoring/live-iteration workflow.
- Source Pulse can consume copied file-watch value rows and execute reviewed package replacement safe points only after validation and operator review.
- Source Pulse does not import Unity, Unreal Engine, or Godot projects, schemas, code, samples, assets, or UI.
- Source Pulse does not expose native handles, execute arbitrary shell commands, execute package scripts, mutate files from editor core, or claim broad 2D editor/all-platform readiness.
- Legal readiness is an engineering evidence gate and still requires counsel for public commercial claims.

Phase 6 validation evidence (2026-06-24):

- `docs/current-capabilities.md` now separates Source Pulse API non-claims from the selected package-visible `2d_source_pulse_*` smoke lane.
- `docs/roadmap.md` was not changed because this slice adds selected package evidence without changing roadmap status.
- `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md` were not changed because this slice adds no third-party code, third-party assets, dependency, license obligation, or legal notice record.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` records retained Original 2D Commercial Authoring Live Iteration evidence without reopening `unsupportedProductionGaps`.
- `engine/agent/manifest.fragments/014-gameCodeGuidance.json` records the exact selected `installed-2d-source-pulse-smoke` package guidance and non-claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` PASS and regenerated `engine/agent/manifest.json`.
- `tools/check-ai-integration-136-2d-source-pulse-package-smoke.ps1` and `tools/check-json-contracts-073-2d-source-pulse-package-smoke.ps1` cover the durable Source Pulse package smoke literals instead of broadening unrelated chapters.

## Phase 7: Final Validation And Publication Gate

Run focused validation after each phase:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_2d_originality_review_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_2d_originality_review_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_2d_source_pulse_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_2d_source_pulse_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_runtime_hot_reload_package_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_runtime_hot_reload_package_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests
```

Run package and static validation before closeout:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-production-workloads.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-package-playtest-productization.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Before staging, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Phase 7 validation evidence (2026-06-24):

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-package-playtest-productization.ps1 -RequireReady` PASS and emitted `2d_source_pulse_status=ready`, `2d_source_pulse_ready=1`, `2d_source_pulse_event_rows=3`, `2d_source_pulse_native_backend_rows=1`, `2d_source_pulse_polling_fallback_rows=1`, `2d_source_pulse_runtime_replacement_committed_rows=1`, and all unsafe/external Source Pulse counters at `0`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-2d-production-workloads.ps1 -RequireReady` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS after the new Source Pulse package-smoke JSON contract was aligned to the actual manifest path and legal-clean-room wording.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS, including 27 static checks, configured/build dev, and 154/154 CTest tests.
- The host reported Metal/Apple diagnostics as expected host-gated checks inside validation; they are diagnostic/host-gated and did not fail the slice.

## Done When

- `TwoDOriginalityReview*`, `review_2d_originality_sources`, `TwoDSourcePulse*`, `plan_2d_source_pulse_events`, and `execute_2d_source_pulse_runtime_replacement_safe_point` exist with focused tests.
- `Editor2DLiveIterationReview*`, `make_editor_2d_live_iteration_review_model`, and `make_editor_2d_live_iteration_review_ui_model` exist with retained UI row tests.
- Package validators emit exact `2d_originality_review_*` and `2d_source_pulse_*` counters.
- Legal/source evidence rejects external engine code, assets, schemas, copied docs text, and public trademark surfaces.
- No Unity, Unreal Engine, Godot, or third-party engine code/assets/templates/schemas are added.
- `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md` are either unchanged because no dependency was added, or updated with exact obligations if a future change adds one.
- Manifest fragments and composed `engine/agent/manifest.json` truthfully describe the capability and non-claims.
- Static guards cover new retained row ids and package-visible counters.
- Full `tools/validate.ps1` passes after all code, docs, manifest, static checks, and package validators settle.

## Reference Sources

- U.S. Copyright Office FAQ: <https://www.copyright.gov/help/faq/faq-protect.html>
- USPTO Trademark basics: <https://www.uspto.gov/trademarks/basics>
- Unity Terms of Service: <https://unity.com/legal/terms-of-service>
- Unity Package Distribution License: <https://unity.com/legal/licenses/unity-package-distribution-license>
- Unreal Engine EULA: <https://www.unrealengine.com/eula/unreal>
- Godot Engine license: <https://godotengine.org/license/>
- Microsoft ReadDirectoryChangesW: <https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw>
- Microsoft DirectWrite: <https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal>
- Microsoft Text Services Framework: <https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework>
- Microsoft UI Automation: <https://learn.microsoft.com/en-us/windows/win32/winauto/entry-uiauto-win32>
