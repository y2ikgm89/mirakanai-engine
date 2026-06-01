---
name: gameengine-editor
description: Guides editor/core, native visible editor shell boundaries, project IO, asset UI, viewport tools, and editor tests. Use when editing files under editor/ or editor-related tests.
paths:
  - "editor/**"
---

# GameEngine editor (Cursor)

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-editor/SKILL.md` |
| Detailed reference | `.claude/skills/gameengine-editor/references/full-guidance.md` |
| Codex | `.agents/skills/editor-change/SKILL.md` |
| Baseline | `AGENTS.md` |

Read the Claude router first. Load the detailed reference from the table when retained ids, editor-shell boundaries, validation lanes, or contract needles are required.

The visible `MK_editor` shell is active through the dependency-free `desktop-editor` lane; keep `MK_editor_core` GUI-independent and in the default validation lane. See `.cursor/rules/mirakana-editor-first-party-shell.mdc`.

Prefer first-party editor documents, stable `mirakana::ui` ids, semantic roles, and private shell adapters; do not adapt editor work to Dear ImGui or UI middleware.

Dock command planning uses `EditorDockCommandKind`, `EditorDockCommandRequest`, `EditorDockCommandPlan`, `plan_editor_dock_command`, and `apply_editor_dock_command` in `MK_editor_core`; AI operation overloads expose reviewed dock layout rows, rich-text `rich_text_rows`, and show/hide/activate/move/split/reset command rows, workspace v2 persists core dock layouts, and the native shell renders single-window dock tab headers/gutters, active/focused panel state, hidden-tab disabled commands, keyboard focus traversal over dock tabs, Console diagnostics, AI Commands status/command/evidence rows, and Inspector property rows as read-only `EditorRichText*` spans, docking smoke counters, private Windows DirectWrite text-layout/glyph-raster adapter validation, selected editor text atlas handoff evidence, private Windows TSF text-input/IME session selection through existing `MK_ui` platform text-input, IME composition, and committed-text contracts with `editor_shell_ime_status=win32_tsf_selected`, and private Windows UIA provider publication with `editor_shell_accessibility_status=uia_provider_ready`, screen-space bounds, hosted-root null runtime ids, and child `UiaAppendRuntimeId` rows. Multi-window drag/tear-off, help rich text, broad editable rich text, Direct2D GPU text rendering/upload, broad shaping/bidi/fallback, full app-owned `ITextStoreACP` callback coverage, native IME candidate UI, reconversion, full UIA control pattern/event parity, cross-platform accessibility parity, and cross-platform font adapters remain future phases.

Validation: focused `MK_editor_core_tests` / `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate per `AGENTS.md`.
