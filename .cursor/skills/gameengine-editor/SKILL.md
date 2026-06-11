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

AI operation status stays in first-party value rows such as `EditorAiOperationSnapshot.status_rows`, including `editor.ai.ime.session`.
Reviewed rich-text commands use `<rich_text_document_id>.copy_selection_plain_text`; keep the exact row ids, dock contracts, and native
shell evidence in the Claude/Codex editor skills.

Environment authoring row and command contracts live in the Claude/Codex editor skills. Keep `GameEngine.EnvironmentProfile.v2`,
`environment.command.*`, `environment.readiness.*`, `EnvironmentSettingsWorkflowModel`, `make_environment_settings_workflow_model`,
the first-party `environment_settings` panel, reviewed preview/package rows, and `environment_settings_productized_status=ready`
editor-core value-only. Keep `environment_settings_broad_environment_ready_claimed=0`; do not claim backend execution, validation
recipe execution, package scripts, package-script execution from editor-core, native handles, Dear ImGui, SDL3, or broad `environment_ready`.

When selecting a future editor text/accessibility milestone, start with first-party editable-rich-text core and AI-operable text commands before GPU upload, custom IME candidate UI, or full UIA parity. Own the editor document, command, semantic, and adapter contracts in `MK_editor_core`; keep Unicode shaping, bidi, font fallback/rasterization, TSF/IME protocol, accessibility bridges, and platform rendering details behind official SDK or audited-dependency adapters.

Cross-platform editor adapter work is future-gated with `editor.cross_platform.adapter.*` rows. Do not claim HarfBuzz/FreeType/ICU-class
adapter readiness from Windows-only evidence; dependency additions still require license and dependency-record updates.

Validation: focused `MK_editor_core_tests` / `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate per `AGENTS.md`.
