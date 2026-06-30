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
`environment.profile_v2.volume_count`, `environment.profile_v2.weather_keyframes`, `environment.volume.<index>.id`,
`environment.volume.<index>.shape`, `environment.volume.<index>.priority`, `environment.volume.<index>.blend_weight`,
`environment.volume.<index>.fade_distance_m`, `environment.weather_keyframe.<index>.time_of_day_hours`,
`environment.weather_keyframe.<index>.weather`, `environment.weather_keyframe.<index>.precipitation`,
`environment.weather_keyframe.<index>.quality_preset`, `environment.quality.tier`, `environment.capture.cubemap.request_status`,
`environment.command.volume.add`, `environment.command.volume.remove`, `environment.command.volume.reorder`,
`environment.command.weather_keyframe.edit`, `environment.command.quality_preset.select`, `environment.command.capture.cubemap.request`,
`environment.preset_library.pack.id`, `environment.preset_library.package.index_registered`,
`environment.preset_library.sample.consumption_evidence`, `environment.preset_library.preset.<index>.id`,
`environment.readiness.unsupported.environment_aaa_preset_library_ready`, and `environment.readiness.*` rows editor-core value-only;
do not claim backend execution, validation recipe execution, package scripts, native handles, Dear ImGui, SDL3, broad
`environment_ready`, or broad AAA preset-library readiness.
Asset import regression workflow ids live in the Claude/Codex skills; keep `asset_browser.import_workflow.*` and `asset_browser.importer_corpus.*` / `asset_browser.import.*` command ids editor-core value-only without execution, mutation, native handles, or Unity/Unreal/Godot compatibility claims.
When selecting a future editor text/accessibility milestone, start with first-party editable-rich-text core and AI-operable text commands before GPU upload, custom IME candidate UI, or full UIA parity. Own the editor document, command, semantic, and adapter contracts in `MK_editor_core`; keep Unicode shaping, bidi, font fallback/rasterization, TSF/IME protocol, accessibility bridges, and platform rendering details behind official SDK or audited-dependency adapters.

Cross-platform editor adapter work is future-gated with `editor.cross_platform.adapter.*` rows; do not claim HarfBuzz/FreeType/ICU-class adapter readiness from Windows-only evidence.

Validation: focused `MK_editor_core_tests` / `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate per `AGENTS.md`.
