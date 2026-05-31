---
name: editor-change
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
- Dock command planning lives in `MK_editor_core` as `EditorDockCommandKind`, `EditorDockCommandRequest`, `EditorDockCommandPlan`, `plan_editor_dock_command`, and `apply_editor_dock_command`; AI operation overloads expose reviewed dock layout rows, rich-text `rich_text_rows`, and show/hide/activate/move/split/reset command rows, workspace v2 persists core dock layouts, and the native shell renders single-window dock tab headers/gutters, active/focused panel state, hidden-tab disabled commands, keyboard focus traversal over dock tabs, Console diagnostics, AI Commands status/command/evidence rows, and Inspector property rows as read-only `EditorRichText*` spans, docking smoke counters, private Windows DirectWrite text-layout/glyph-raster adapter validation, selected editor text atlas handoff evidence, and first-party value-only focused text-input/IME controller rows through existing `MK_ui` platform text-input, IME composition, and committed-text contracts. Multi-window drag/tear-off, help rich text, broad editable rich text, Direct2D GPU text rendering/upload, broad shaping/bidi/fallback, private Windows TSF COM adapter execution, native IME candidate UI, accessibility publication, and cross-platform font adapters remain future phases.
- Read `references/full-guidance.md` only when detailed retained row ids, panel contracts, visible-shell boundary rules, or detailed validation lanes are needed.
- Prefer focused `MK_editor_core_tests` or `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate.
- When adding retained UI ids or CMake target literals enforced by `check-ai-integration.ps1`, update Needles and Codex/Claude skill twins together.
- Generated Game Studio v1 uses `EditorAiGeneratedGameStudioV1Model`, `make_editor_ai_generated_game_studio_v1_model`, `make_editor_ai_generated_game_studio_v1_ui_model`, and retained `generated_game_studio` rows over existing AI playtest/operator workflow models; keep it read-only, GUI-independent in `editor/core`, and free of validation execution, manifest mutation, engine-internal edits, native handles, renderer/RHI residency, Metal readiness, or broad editor productization.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and detailed validation evidence. Load only the sections needed for the current task.
