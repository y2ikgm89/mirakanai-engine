# Editor Game Module Driver MK Editor Visible DLL Mutation Order Guidance v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-mk-editor-visible-dll-mutation-order-guidance-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Stream:** [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md)

## Goal

Expose **`EditorGameModuleDriverHostSessionSnapshot::policy_dll_mutation_order_guidance`** in the visible **`mirakana_editor`** Run/Viewport **Game Module Driver** section alongside existing host-session barrier and policy lines, so operators see the same canonical order hints that `MK_editor_core_tests` and retained `MK_ui` rows already encode. This slice **does not** implement mid-play DLL replacement, active-session hot reload execution, or operator overrides.

## Context

- [Fail-closed order guidance v1](2026-05-11-editor-game-module-driver-active-session-hot-reload-fail-closed-order-v1.md) landed snapshot fields, retained `play_in_editor.game_module_driver.session.policy.dll_mutation_order_guidance`, and core tests; `editor/src/main.cpp` still omitted the visible line next to other session diagnostics.

## Constraints

- No change to `requiredBeforeReadyClaim` semantics for `editor-productization`; no ready-claim expansion.
- Use the same `session_snapshot` instance as phase, barrier, and other policy lines (no duplicate session logic in the shell).
- Fragment-edit `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` then compose; do not hand-edit `engine/agent/manifest.json`.

## Done when

- `draw_game_module_driver_controls` renders a stable `ImGui::TextDisabled` label plus `session_snapshot.policy_dll_mutation_order_guidance`.
- `tools/check-ai-integration.ps1` includes a `editor/src/main.cpp` needle for the visible label string.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` after fragment updates.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes (or a host blocker is recorded here).

## Validation evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` — exit code 0 (includes `check-ai-integration.ps1`, `build.ps1`, `ctest --preset dev`; 47/47 tests passed including `MK_editor_core_tests` and `MK_editor_game_module_driver_load_tests`).
