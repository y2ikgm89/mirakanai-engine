# Editor Input Rebinding Axis Capture Keyboard Key Pair v1 (2026-05-10)

**Plan ID:** `editor-input-rebinding-axis-capture-keyboard-key-pair-v1`  
**Status:** Completed.

## Goal

Add deterministic keyboard key-pair axis capture to `capture_runtime_input_rebinding_axis` after gamepad-axis probing, expose `capture_keyboard_key_pair_axes` on `RuntimeInputRebindingAxisCaptureRequest`, wire `EditorInputRebindingAxisCaptureDesc`, and update the visible `MK_editor` axis capture workflow so operators can assign axis overrides from two simultaneously held keys.

## Context

- Follows [`2026-05-10-editor-input-rebinding-axis-capture-gamepad-v1.md`](2026-05-10-editor-input-rebinding-axis-capture-gamepad-v1.md).
- Key-pair negative/positive assignment uses ascending `Key` enum order among held keys (deterministic tie-break).

## Constraints

- In-memory profile mutation only; no file save, SDL3 handles, or gameplay focus consumption.
- Gamepad axes remain first in capture resolution when `capture_gamepad_axes` is true.

## Done When

- Runtime API, editor messaging, and tests agree on two-held-key capture semantics.

## Validation evidence

| Check | Result |
| --- | --- |
| `cmake --build --preset dev --target mirakana_runtime MK_runtime_tests MK_editor_core MK_editor_core_tests` | PASS (local) |
| `MK_runtime_tests` includes keyboard key-pair axis capture case | PASS |
| Manifest MK_runtime purpose and `gameCodeGuidance.currentInputRebindingProfiles` mention keyboard key-pair axis capture alongside gamepad-axis capture | PASS |
