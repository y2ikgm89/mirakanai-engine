# Editor Input Rebinding Axis Capture (Gamepad) v1 (2026-05-10)

**Plan ID:** `editor-input-rebinding-axis-capture-gamepad-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Status:** Completed  

## Goal

Add `capture_runtime_input_rebinding_axis` for deterministic gamepad axis detection and wire the editor Input Rebinding panel so axis bindings get a Capture workflow (gamepad stick/trigger only; keyboard key-pair capture stays out of scope).

## Context

Action rebinding capture already exists end-to-end; axis presentation and profile overrides exist, but there was no runtime capture path or editor controls for axis rows.

## Constraints

- MVP captures **gamepad axes** only; keyboard `key_pair` axis capture remains a separate slice.
- Keep SDL3 out of capture; use `RuntimeInputStateView` only.
- Do not broaden `editor-productization` ready claims beyond this narrow rebinding UX.

## Done when

- Public runtime API `capture_runtime_input_rebinding_axis` with unit tests (blocked / waiting / captured).
- Editor panel exposes Axis Capture controls and applies captured axis overrides to the in-memory profile when capture succeeds.
- `docs/editor.md` notes gamepad axis capture; `engine/agent/manifest.json` reflects new surfaces where required by validation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
