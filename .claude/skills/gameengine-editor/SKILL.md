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
  `editor.ai.ime.session`, `editor.ai.ime.parity`, `editor.ai.ime.candidate_selection`, `editor.ai.ime.reconversion`,
  `editor.ai.ime.platform_host_gates`, `editor.ai.accessibility.uia_provider`, `editor.ai.accessibility.parity`,
  `editor.ai.viewport.display`, and
  `editor.ai.material_preview.display`;
  reviewed rich-text commands always include read-only copy rows `<rich_text_document_id>.copy_plain_text` and
  `<rich_text_document_id>.copy_selection_plain_text`; editable documents may additionally expose
  `<rich_text_document_id>.insert_text`, `.delete_selection`, `.replace_selection`, `.toggle_bold`, `.toggle_italic`,
  `.copy_rich_text`, `.cut_selection`, `.paste_plain_text`, and `.paste_rich_text`. These rows reject stale revisions,
  native handles, shell execution, validation-recipe execution, screen coordinates, unsupported markup, unsafe tokens,
  invalid UTF-8, and unknown or disabled commands.
- Dock command planning lives in `MK_editor_core` as `EditorDockCommandKind`, `EditorDockCommandRequest`, `EditorDockCommandPlan`,
  `plan_editor_dock_command`, and `apply_editor_dock_command`; AI operation overloads expose reviewed dock layout rows, rich-text
  `rich_text_rows`, and show/hide/activate/move/split/reset command rows, workspace v2 persists core dock layouts, and the native shell
  renders single-window dock tab headers/gutters, active/focused panel state, hidden-tab disabled commands, keyboard focus traversal over
  dock tabs, Console diagnostics, AI Commands status/command/evidence rows as read-only `EditorRichText*` spans, and Inspector property
  rows with visible edit command controls only for editable `EditorRichText*` documents, docking smoke counters, private Windows DirectWrite text-layout/glyph-raster adapter validation, selected editor text atlas handoff
  evidence rows including `editor_text_shaping_status=ready`, `editor_text_font_fallback_status=ready`, `editor_text_glyph_atlas_status=ready`,
  `editor_text_dependency_license_records=ready`, `editor_text_dependency_gated_rows=3`, and `editor_text_native_handles_exposed=0`, private
  Windows TSF text-input/IME session selection through existing `MK_ui` platform text-input, IME composition, and
  committed-text contracts with `editor_shell_ime_status=win32_tsf_selected`, first-party Phase 6 IME parity evidence through
  `TextInputParityEvidenceRequest`, private `NativeEditorTsfTextStoreEvidence` over app-owned `ITextStoreACP`, smoke rows
  `editor_ime_parity_status=ready`, `editor_ime_windows_tsf_status=ready`, `editor_ime_macos_status=host_gated`,
  `editor_ime_linux_ibus_status=host_gated`, `editor_ime_linux_fcitx_status=host_gated`, `editor_ime_android_status=host_gated`,
  `editor_ime_ios_status=host_gated`, and `editor_ime_native_handles_exposed=0`, private Windows UIA provider publication with
  `editor_shell_accessibility_status=uia_provider_ready`, screen-space bounds, hosted-root null runtime ids, and child `UiaAppendRuntimeId`
  rows, first-party accessibility parity evidence rows including `editor_accessibility_parity_status=ready`,
  `editor_accessibility_windows_uia_patterns_ready=1`, `editor_accessibility_windows_uia_events_ready=1`,
  `editor_accessibility_macos_status=host_gated`, `editor_accessibility_linux_at_spi_status=host_gated`,
  `editor_accessibility_android_status=host_gated`, `editor_accessibility_ios_status=host_gated`,
  positive live-region/pattern/event rows, and `editor_accessibility_native_handles_exposed=0`, native viewport/material preview
  lifecycle gates, private `native_texture_display_adapter.*` RHI evidence, and private
  `native_editor_visible_texture_compositor.*` presentation for requested private D3D12 texture display plus host/toolchain-gated
  private Vulkan texture-display rows and Apple-host-gated private Metal texture-display rows, offscreen targets, descriptor updates,
  resource-barrier and synchronization2 evidence, Metal command-queue/metallib/feature/pipeline/render-pass/texture-sampling/present/completion evidence, fence
  readiness, viewport resize-safe teardown, visible-compositor consumption, positive visible texture composite smoke counters,
  material-preview host-private frame preparation, `editor_shell_viewport_vulkan_status`,
  `editor_shell_material_preview_vulkan_status`, `editor_shell_vulkan_validation_layer_ready`,
  `editor_shell_vulkan_native_handles_exposed=0`, `editor_shell_viewport_metal_status`,
  `editor_shell_material_preview_metal_status`, `editor_shell_metal_feature_set_ready`, and
  `editor_shell_metal_native_handles_exposed=0`, retained `ui_retained_*` diff/cache smoke rows
  (`ui_retained_diff_status=ready`, zero dirty/miss/rebuild/native-handle rows), first-party `EditorDockMultiWindowLayout` /
  `EditorDockWindowCommandPlan` planning, clean-break `GameEngine.Workspace.v3` persistence, AI command ids
  `editor.dock.window.create`, `editor.dock.window.close`, `editor.dock.panel.tear_off`, `editor.dock.panel.move_to_window`,
  `editor.dock.window.merge`, and `editor.dock.window.reset_all`, and smoke rows
  `editor_shell_multi_window_docking_status=ready`, `editor_shell_workspace_v3_status=ready`, and
  `editor_shell_multi_window_native_handles_exposed=0` without native handle exposure, plus first-party editable rich-text
  document command/history/clipboard contracts through `EditorRichTextEditCommandKind`, `EditorRichTextEditRequest`,
  `EditorRichTextEditResult`, `EditorRichTextClipboardPayload`, `editor_rich_text_revision`,
  `normalize_editor_rich_text_selection`, `apply_editor_rich_text_edit_command`, and smoke rows
  `editor_rich_text_edit_status=ready`, `editor_rich_text_clipboard_plain_ready=1`,
  `editor_rich_text_clipboard_rich_ready=1`, and `editor_rich_text_native_handles_exposed=0`. Phase 11 AI operation rows include
  `editor.ai.window.layout`, `editor.ai.dock.multi_window`, `editor.ai.rich_text.editable_documents`,
  `editor.ai.viewport.backend_parity`, `editor.ai.material_preview.backend_parity`, `editor.ai.performance.budgets`, and
  `editor.ai.operation.excellence`; reviewed non-mutating commands include `editor.text_font.diagnostics.copy`,
  `editor.accessibility.diagnostics.copy`, `editor.viewport.backend_readiness.refresh`, and
  `editor.material_preview.backend_readiness.refresh`, and smoke rows include `editor_ai_operation_excellence_status=ready`,
  `editor_ai_operation_mutating_commands_revision_checked=1`, and `editor_ai_operation_native_handles_exposed=0`. Every mutating
  AI command requires `expected_revision`; package scripts, file mutation, validation recipes, shell/process execution, screen
  coordinates, and native-handle requests fail closed. Visible OS-level multi-window drag/drop
  shell restoration, help rich text, Direct2D GPU text rendering/upload, broad shaping/bidi/fallback, custom native IME candidate UI,
  non-Windows IME execution, external OS accessibility-tool execution beyond first-party evidence,
  default visible-shell Vulkan or Metal backend selection, broader material-preview GPU parity, cross-platform accessibility parity, and
  cross-platform font adapters remain future phases. `EditorCrossPlatformShellAdapterPlan` / `editor.ai.shell.cross_platform` /
  `editor_shell_cross_platform_status=host_gated` record first-party host-gated macOS/Linux shell adapter contracts only, with
  `editor_shell_android_status=unsupported`, `editor_shell_ios_status=unsupported`, and
  `editor_shell_cross_platform_native_handles_exposed=0`; do not treat those rows as actual macOS/Linux shell execution. Do not infer Metal readiness from D3D12 or Vulkan evidence; `metal_texture_ready`
  requires the reviewed `renderer-metal-apple-host-evidence` Apple-host recipe and no native handle exposure.
- When selecting a future editor text/accessibility milestone, start with first-party editable-rich-text core and AI-operable text commands before GPU upload, custom IME candidate UI, non-Windows IME execution, or external accessibility-tool execution. Own the editor document, command, semantic, and adapter contracts in `MK_editor_core`; keep Unicode shaping, bidi, font fallback/rasterization, TSF/IME protocol, accessibility bridges, and platform rendering details behind official SDK or audited-dependency adapters.
- Cross-platform editor adapter work is future-gated for dependency-backed parity and host-gated with first-party shell rows plus legacy `editor.cross_platform.adapter.*` dependency rows: macOS AppKit/Core Text/`NSTextInputClient`/`NSAccessibilityProtocol`/Metal presentation,
  Linux X11-or-Wayland/AT-SPI2/IBus/Fcitx/Vulkan presentation, Android/iOS visible editor shells, and HarfBuzz/FreeType/ICU-class adapters
  must not be claimed from Windows DirectWrite/TSF/UIA/D3D12 evidence. HarfBuzz, FreeType, ICU, and font packages stay `dependency_gated` until
  selected by a license/dependency slice; generated games and runtime UI stay on public `mirakana::ui`, and new text/font/image dependencies
  require `license-audit`, `vcpkg.json`, `docs/dependencies.md`, and `THIRD_PARTY_NOTICES.md`.
- Environment authoring readiness rows such as `environment.readiness.physical_sky.package_status`, `environment.readiness.backend.metal_status`, and `environment.readiness.unsupported.environment_ready` are non-editable Inspector diagnostics from `make_environment_authoring_inspector_model`; do not treat them as backend/package execution, public handle exposure, or broad `environment_ready`.
- Read `references/full-guidance.md` only when detailed retained row ids, panel contracts, visible-shell boundary rules, or detailed validation lanes are needed.
- Prefer focused `MK_editor_core_tests` or `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate.
- When adding retained UI ids or CMake target literals enforced by `check-ai-integration.ps1`, update Needles and Codex/Claude skill twins together.
- Generated Game Studio v1 uses `EditorAiGeneratedGameStudioV1Model`, `make_editor_ai_generated_game_studio_v1_model`, `make_editor_ai_generated_game_studio_v1_ui_model`, and retained `generated_game_studio` rows over existing AI playtest/operator workflow models; keep it read-only, GUI-independent in `editor/core`, and free of validation execution, manifest mutation, engine-internal edits, native handles, renderer/RHI residency, Metal readiness, or broad editor productization.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and detailed validation evidence. Load only the sections needed for the current task.
