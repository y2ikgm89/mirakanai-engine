---
name: editor-change
description: Guides editor/core, Dear ImGui shell, project IO, asset UI, viewport tools, and editor tests. Use when editing files under editor/ or editor-related tests.
paths:
  - "editor/**"
---

# Editor Change

## Scope

Use this skill for editor/core models, Dear ImGui shell wiring, project IO, asset UI, viewport tools, and editor tests.

## Context Budget Rules

- Start with targeted file reads, targeted manifest fragments, and `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard` whenever possible.
- Do not load `references/full-guidance.md` by default. Load it only when the current task needs exact API names, validation counters, retained ids, package lanes, or backend/editor details not present here.
- Keep implementation slices small, clean-break, and evidence-backed. Do not add compatibility shims, stale aliases, broad ready claims, or unsupported host assumptions.
- Prefer focused build/test/static loops while iterating, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the coherent slice gate.

## Required Discipline

- Keep `editor/core` GUI-independent and route persistent behavior through editor-core models before shell wiring.
- Read `references/full-guidance.md` only when detailed retained row ids, panel contracts, ImGui style rules, or historical validation lanes are needed.
- Prefer focused `MK_editor_core_tests` or `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate.
- When adding retained UI ids or CMake target literals enforced by `check-ai-integration.ps1`, update Needles and Codex/Claude skill twins together.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and historical validation evidence. Load only the sections needed for the current task.
