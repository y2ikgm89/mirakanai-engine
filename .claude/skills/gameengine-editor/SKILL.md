---
name: gameengine-editor
description: Guides editor/core, native visible editor shell boundaries, project IO, asset UI, viewport tools, and editor tests. Use when editing files under editor/ or editor-related tests.
paths:
  - "editor/**"
---

# Editor Change

## Scope

Use this skill for editor/core models, native visible editor shell boundaries, project IO, asset UI, viewport tools, and editor tests.

## Context Budget Rules

- Start with targeted file reads, targeted manifest fragments, and `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard` whenever possible.
- Do not load `references/full-guidance.md` by default. Load it only when the current task needs exact API names, validation counters, retained ids, package lanes, or backend/editor details not present here.
- Keep implementation slices small, clean-break, and evidence-backed. Do not add compatibility shims, stale aliases, broad ready claims, or unsupported host assumptions.
- Prefer focused build/test/static loops while iterating, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the coherent slice gate.

## Required Discipline

- Keep `editor/core` GUI-independent and route persistent behavior through editor-core models before shell wiring.
- Prefer first-party editor documents, stable `mirakana::ui` ids, semantic roles, and private shell adapters; do not adapt editor work to Dear ImGui or UI middleware.
- AI operation status stays in first-party value rows: `EditorAiOperationSnapshot.status_rows` uses exact ids
  `editor.ai.dock.selected_panel`, `editor.ai.rich_text.documents`, `editor.ai.text_input.focused_target`, `editor.ai.adapter.text_font`,
  `editor.ai.ime.session`, `editor.ai.accessibility.uia_provider`, `editor.ai.viewport.display`, and `editor.ai.material_preview.display`;
  reviewed rich-text commands use `<rich_text_document_id>.copy_plain_text` and `<rich_text_document_id>.copy_selection_plain_text` only,
  reject stale revisions/native handles/shell execution/validation-recipe execution/screen coordinates, and do not mutate state.
- Runtime UI authoring lives in GUI-independent `MK_editor_core` as `EditorRuntimeUiDocumentModel`,
  `EditorRuntimeUiThemeModel`, `make_editor_runtime_ui_authoring_model`, `plan_editor_runtime_ui_authoring_command`, and
  `make_editor_runtime_ui_authoring_command_action`; reviewed command ids are `runtime_ui.element.add`,
  `runtime_ui.element.remove`, `runtime_ui.element.reorder`, `runtime_ui.element.select`,
  `runtime_ui.property.edit_text`, `runtime_ui.style.set_token`, and `runtime_ui.preview.refresh`.
  Mutating commands require the relevant expected document/theme revision, validate duplicate ids and hierarchy cycles,
  keep reorder sibling/subtree-safe, route undoable changes through `UndoStack`, and must not execute renderer work,
  package scripts, validation recipes, expose native handles, import external-engine schemas, copy external visual themes,
  or claim visible runtime UI editor, Windows UIA runtime publication, D3D12 UI atlas upload, cross-platform adapter parity,
  external engine compatibility, or broad platform UI readiness.
- Dock command planning lives in `MK_editor_core` as `EditorDockCommandKind`, `EditorDockCommandRequest`, `EditorDockCommandPlan`,
  `plan_editor_dock_command`, and `apply_editor_dock_command`; AI operation overloads expose reviewed dock layout rows, rich-text
  `rich_text_rows`, and show/hide/activate/move/split/reset command rows, workspace v2 persists core dock layouts, and the native shell
  renders single-window dock tab headers/gutters, active/focused panel state, hidden-tab disabled commands, keyboard focus traversal over
  dock tabs, Console diagnostics, AI Commands status/command/evidence rows, and Inspector property rows as read-only `EditorRichText*`
  spans, docking smoke counters, private Windows DirectWrite text-layout/glyph-raster adapter validation, selected editor text atlas handoff
  evidence, private Windows TSF text-input/IME session selection through existing `MK_ui` platform text-input, IME composition, and
  committed-text contracts with `editor_shell_ime_status=win32_tsf_selected`, private Windows UIA provider publication with
  `editor_shell_accessibility_status=uia_provider_ready`, screen-space bounds, hosted-root null runtime ids, and child `UiaAppendRuntimeId`
  rows, native viewport/material preview lifecycle gates, private `native_texture_display_adapter.*` RHI evidence, and private
  `native_editor_visible_texture_compositor.*` presentation for requested private D3D12 texture display, offscreen targets, descriptor
  updates, resource-barrier, fence readiness, viewport resize-safe teardown, visible-compositor consumption, positive visible texture
  composite smoke counters, and material-preview host-private frame preparation without native handle exposure. Multi-window drag/tear-off,
  help rich text, broad editable rich text, Direct2D GPU text rendering/upload, broad shaping/bidi/fallback, full app-owned `ITextStoreACP`
  callback coverage, native IME candidate UI, reconversion, full UIA control pattern/event parity, Vulkan/Metal editor texture-display
  parity, broader material-preview GPU parity, cross-platform accessibility parity, and cross-platform font adapters remain future phases.
- When selecting a future editor text/accessibility milestone, start with first-party editable-rich-text core and AI-operable text commands before GPU upload, custom IME candidate UI, or full UIA parity. Own the editor document, command, semantic, and adapter contracts in `MK_editor_core`; keep Unicode shaping, bidi, font fallback/rasterization, TSF/IME protocol, accessibility bridges, and platform rendering details behind official SDK or audited-dependency adapters.
- Cross-platform editor adapter work is future-gated with `editor.cross_platform.adapter.*` rows: macOS Core Text/InputMethodKit/NSAccessibility, Linux AT-SPI/IBus/Fcitx, Android InputMethodService/accessibility, iOS UITextInput/UIAccessibility, and HarfBuzz/FreeType/ICU-class adapters must not be claimed from Windows DirectWrite/TSF/UIA evidence; generated games and runtime UI stay on public `mirakana::ui`, and new text/font/image dependencies require `license-audit`, `vcpkg.json`, `docs/dependencies.md`, and `THIRD_PARTY_NOTICES.md`.
- Environment authoring uses clean-break `GameEngine.EnvironmentProfile.v2` editor documents. Retained rows include
  `environment.profile_v2.volume_count`, `environment.profile_v2.weather_keyframes`, `environment.volume.<index>.id`,
  `environment.volume.<index>.shape`, `environment.volume.<index>.priority`, `environment.volume.<index>.blend_weight`,
  `environment.volume.<index>.fade_distance_m`, `environment.weather_keyframe.<index>.time_of_day_hours`,
  `environment.weather_keyframe.<index>.weather`, `environment.weather_keyframe.<index>.precipitation`,
  `environment.weather_keyframe.<index>.quality_preset`, `environment.quality.tier`, `environment.capture.cubemap.request_status`,
  `environment.readiness.physical_sky.package_status`, `environment.readiness.backend.metal_status`, and
  `environment.readiness.unsupported.environment_ready` from `make_environment_authoring_inspector_model`. Environment preset library
  browsing uses `EnvironmentPresetLibraryDesc`, `make_environment_preset_library_model`,
  `make_environment_preset_library_ui_model`, and `make_environment_preset_library_package_candidate_rows`; retained rows include
  `environment.preset_library.pack.id`, `environment.preset_library.pack.provenance_id`,
  `environment.preset_library.pack.license_id`, `environment.preset_library.pack.art_direction`,
  `environment.preset_library.pack.quality_tier`, `environment.preset_library.pack.preset_count`,
  `environment.preset_library.package.index_registered`, `environment.preset_library.package.runtime_path`,
  `environment.preset_library.sample.consumption_evidence`, `environment.preset_library.preset.<index>.id`,
  `environment.preset_library.preset.<index>.profile_asset_path`, `environment.preset_library.preset.<index>.quality_tier`,
  `environment.preset_library.preset.<index>.validation_recipe_id`, and
  `environment.readiness.unsupported.environment_aaa_preset_library_ready`; reviewed command ids
  `environment.command.volume.add`, `environment.command.volume.remove`, `environment.command.volume.reorder`,
  `environment.command.weather_keyframe.edit`, `environment.command.quality_preset.select`, and
  `environment.command.capture.cubemap.request` are editor-core value rows only. Do not treat them as backend/package/validation
  execution, public handle exposure, broad `environment_ready`, or broad AAA preset-library readiness.
- Asset import regression workflow rows live in GUI-independent `MK_editor_core` as
  `EditorAssetImportRegressionWorkflowDesc`, `EditorAssetImportRegressionWorkflowModel`,
  `make_editor_asset_import_regression_workflow_model`, and
  `make_editor_asset_import_regression_workflow_retained_ui_desc`; retained rows use
  `asset_browser.import_workflow.*`, and reviewed command ids are `asset_browser.importer_corpus.run`,
  `asset_browser.importer_corpus.open_report`, `asset_browser.import.batch_reimport`,
  `asset_browser.import.preset_diff`, and `asset_browser.import.axis_unit_preview`. The native shell may read a safe
  `--asset-import-regression-report <project-relative-report>` path, sanitize retained report rows, and emit
  `editor_asset_import_regression_*` smoke counters. Keep these value-only: no editor-core importer execution,
  package scripts, validation recipes, package mutation, native handles, arbitrary importer plugins, absolute host path
  leakage, external downloads, or Unity/Unreal/Godot compatibility claims.
- Read `references/full-guidance.md` only when detailed retained row ids, panel contracts, visible-shell boundary rules, or detailed validation lanes are needed.
- Prefer focused `MK_editor_core_tests` or `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate.
- When adding retained UI ids or CMake target literals enforced by `check-ai-integration.ps1`, update Needles and Codex/Claude skill twins together.
- Generated Game Studio v1 uses `EditorAiGeneratedGameStudioV1Model`, `make_editor_ai_generated_game_studio_v1_model`, `make_editor_ai_generated_game_studio_v1_ui_model`, and retained `generated_game_studio` rows over existing AI playtest/operator workflow models; keep it read-only, GUI-independent in `editor/core`, and free of validation execution, manifest mutation, engine-internal edits, native handles, renderer/RHI residency, Metal readiness, or broad editor productization.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and detailed validation evidence. Load only the sections needed for the current task.
