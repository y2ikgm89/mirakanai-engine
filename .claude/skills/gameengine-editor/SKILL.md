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
- Environment authoring readiness rows such as `environment.readiness.physical_sky.package_status`, `environment.readiness.backend.metal_status`, and `environment.readiness.unsupported.environment_ready` are non-editable Inspector diagnostics from `make_environment_authoring_inspector_model`; do not treat them as backend/package execution, public handle exposure, or broad `environment_ready`.
- Read `references/full-guidance.md` only when detailed retained row ids, panel contracts, visible-shell boundary rules, or detailed validation lanes are needed.
- Prefer focused `MK_editor_core_tests` or `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate.
- When adding retained UI ids or CMake target literals enforced by `check-ai-integration.ps1`, update Needles and Codex/Claude skill twins together.
- Generated Game Studio v1 uses `EditorAiGeneratedGameStudioV1Model`, `make_editor_ai_generated_game_studio_v1_model`, `make_editor_ai_generated_game_studio_v1_ui_model`, and retained `generated_game_studio` rows over existing AI playtest/operator workflow models; keep it read-only, GUI-independent in `editor/core`, and free of validation execution, manifest mutation, engine-internal edits, native handles, renderer/RHI residency, Metal readiness, or broad editor productization.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and detailed validation evidence. Load only the sections needed for the current task.
