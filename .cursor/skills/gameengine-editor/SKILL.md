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

AI operation status stays in first-party value rows such as `EditorAiOperationSnapshot.status_rows`, including `editor.ai.ime.session`, `editor.ai.ime.parity`, `editor.ai.accessibility.uia_provider`, `editor.ai.accessibility.parity`, `editor.ai.viewport.backend_parity`, and `editor.ai.operation.excellence`.
Reviewed rich-text commands keep read-only copy rows such as `<rich_text_document_id>.copy_selection_plain_text`; editable documents may expose the edit/copy/paste rows documented in Claude/Codex skills. Reviewed diagnostics/refresh commands include `editor.text_font.diagnostics.copy`, `editor.accessibility.diagnostics.copy`, `editor.viewport.backend_readiness.refresh`, and `editor.material_preview.backend_readiness.refresh`; every mutating AI command requires `expected_revision`.

Phase 12 closes the excellence milestone with exact selected-row aggregate counters:
`first_party_editor_excellence_status=ready`, `first_party_editor_excellence=1`, Windows D3D12 `=1`, and Vulkan/Metal/cross-platform/text/accessibility/broad-optimization/native-handle claims `=0`.
Keep the exact selected-row aggregate explicit, including `first_party_editor_excellence_text_parity=0` and `first_party_editor_excellence_accessibility_parity=0`.

Selected Windows DirectWrite/TSF/UIA rows are first-party smoke evidence only: `editor_text_shaping_status=ready`, `editor_ime_parity_status=ready`, `NativeEditorTsfTextStoreEvidence`, `NativeEditorUiaProviderState`, `editor_accessibility_parity_status=ready`, `editor_accessibility_windows_uia_patterns_ready=1`, `editor_accessibility_windows_uia_events_ready=1`, non-Windows `editor_ime_*_status=host_gated` and `editor_accessibility_*_status=host_gated`, and zero native-handle rows.

Selected editor texture rows include D3D12 visible readiness plus host/toolchain-gated Vulkan and Apple-host-gated Metal counter evidence.
Do not infer Metal readiness from D3D12 or Vulkan; `metal_texture_ready` requires `renderer-metal-apple-host-evidence` on Apple host.

When selecting a future editor text/accessibility milestone, start with first-party editable-rich-text core and AI-operable text commands before GPU upload, custom IME candidate UI, non-Windows IME execution, or external accessibility-tool execution. Own the editor document, command, semantic, and adapter contracts in `MK_editor_core`; keep Unicode shaping, bidi, font fallback/rasterization, TSF/IME protocol, accessibility bridges, and platform rendering details behind official SDK or audited-dependency adapters.

Cross-platform editor adapter work is future-gated for dependency-backed parity and host-gated with first-party shell rows such as `EditorCrossPlatformShellAdapterPlan`,
`editor.ai.shell.cross_platform`, and `editor_shell_cross_platform_status=host_gated`, plus legacy `editor.cross_platform.adapter.*`
dependency rows. Do not claim macOS/Linux shell execution, Android/iOS editor shells, or HarfBuzz/FreeType/ICU-class adapter readiness
from Windows-only evidence; HarfBuzz, FreeType, ICU, and font packages stay `dependency_gated` until dependency additions go through
license and dependency-record updates.
Validation: focused `MK_editor_core_tests` / `check-tidy.ps1 -Files` loops while iterating, then `tools/validate.ps1` at the slice gate per `AGENTS.md`.
