# Editor Documentation Input Rebinding Keyboard Key Pair Parity v1 (2026-05-10)

**Plan ID:** `editor-documentation-input-rebinding-keyboard-key-pair-parity-v1`  
**Status:** Completed.

## Goal

Align human-facing editor/roadmap guidance and Codex/Claude editor skills with the already-landed keyboard key-pair axis capture (`capture_keyboard_key_pair_axes`, [`2026-05-10-editor-input-rebinding-axis-capture-keyboard-key-pair-v1.md`](2026-05-10-editor-input-rebinding-axis-capture-keyboard-key-pair-v1.md)) so `docs/editor.md`, [`docs/roadmap.md`](../../roadmap.md), and editor-change skills no longer claim keyboard key-pair axis capture is unsupported follow-up work.

## Context

- Gap burn-down: `editor-productization` (`unsupportedProductionGaps`).
- Runtime/editor implementation and tests already cover keyboard key-pair axis capture; stale prose blocked accurate operator and agent expectations.

## Constraints

- Do not broaden editor ready claims beyond reviewed capture semantics (in-memory profile only, no file mutation from capture path, etc.).
- Keep [`engine/agent/manifest.json`](../../../engine/agent/manifest.json) `recommendedNextPlan.reason` needles required by `tools/check-ai-integration.ps1` intact when editing substrings.

## Done When

- [`docs/editor.md`](../../editor.md) and [`docs/roadmap.md`](../../roadmap.md) describe axis capture including optional keyboard key-pair where appropriate.
- [`.agents/skills/editor-change/SKILL.md`](../../../.agents/skills/editor-change/SKILL.md) and [`.claude/skills/gameengine-editor/SKILL.md`](../../../.claude/skills/gameengine-editor/SKILL.md) match the capture contract.
- `recommendedNextPlan.reason` no longer lists keyboard key-pair axis capture as a follow-up next to completed gamepad/action capture slices.

## Validation evidence

| Check | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pending — requires `engine/agent/manifest.json` `recommendedNextPlan.reason` substring fixes (see Done When) |
| Doc/skill parity | **PASS** — [`docs/editor.md`](../../editor.md), [`docs/roadmap.md`](../../roadmap.md), [`.agents/skills/editor-change/SKILL.md`](../../../.agents/skills/editor-change/SKILL.md), [`.claude/skills/gameengine-editor/SKILL.md`](../../../.claude/skills/gameengine-editor/SKILL.md) |
| Registry | **PASS** — [`README.md`](./README.md) Latest completed slice updated |

Apply these replacements to `recommendedNextPlan.reason` (preserves `check-ai-integration.ps1` needles):

1. Replace `keyboard-layout localization, keyboard key-pair axis capture, native handles` with `keyboard-layout localization, native handles`.

2. Replace the sentence beginning `Editor Input Rebinding Axis Capture Gamepad v1 is complete through RuntimeInputRebindingAxisCaptureRequest` through `gamepad stick/trigger detection only.` with: `Editor Input Rebinding Axis Capture v1 is complete through RuntimeInputRebindingAxisCaptureRequest, capture_runtime_input_rebinding_axis, EditorInputRebindingAxisCaptureDesc capture_keyboard_key_pair_axes, make_editor_input_rebinding_capture_axis_model, retained input_rebinding.capture.axis rows, and visible MK_editor axis-row Capture controls for gamepad-axis probing and optional two-held keyboard key-pair axis sources.`

Then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
