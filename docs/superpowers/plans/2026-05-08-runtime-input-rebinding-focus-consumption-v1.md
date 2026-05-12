# Runtime Input Rebinding Focus Consumption v1 Implementation Plan (2026-05-08)

**Status:** Completed.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent runtime input rebinding capture guard that keeps a UI/menu capture focused and consumes gameplay input while a digital action rebinding capture is waiting or captured.

**Architecture:** Keep the bridge in `mirakana_runtime` so it depends only on first-party runtime input state and plain string UI/focus ids. UI, editor, and game shells can map their own `mirakana_ui::InteractionState` or menu focus rows into the guard without making `mirakana_runtime` depend on `mirakana_ui`, SDL3, Dear ImGui, files, cloud saves, or native handles.

**Tech Stack:** C++23 `mirakana_runtime`, existing `RuntimeInputRebindingCaptureRequest`, existing `MK_runtime_tests`, existing validation scripts.

---

## Context

- Master plan gap: runtime systems that generated games depend on need production-minimum persisted input rebinding profiles and runtime/game rebinding UX.
- Runtime Input Rebinding Capture Contract v1 can capture a key, pointer, or gamepad button into a candidate profile, and Editor Input Rebinding Action Capture Panel v1 exposes that in the editor only.
- Remaining unsupported work includes runtime/game rebinding panels outside the editor lane, UI focus/consumption, glyph generation, axis capture, multiplayer device assignment, native handles, and cloud/binary saves.
- This slice narrows the focus/consumption part only: a runtime UI/menu can arm one capture row, require that capture row to own focus/modal state, call the existing capture helper, and receive deterministic `gameplay_input_consumed` / `focus_retained` rows.

## Constraints

- Do not add SDL3/native handles, Dear ImGui/editor dependencies, file/cloud/binary save mutation, input glyph generation, axis capture, multiplayer device assignment, UI rendering, or platform input APIs.
- Keep the API host-independent and deterministic.
- Do not change existing action-map evaluation semantics.
- Treat focus guard failures as blocked capture attempts before consuming gameplay input.
- Preserve existing `capture_runtime_input_rebinding_action` behavior; add a narrower wrapper for UI/menu focus ownership.

## Done When

- `RuntimeInputRebindingFocusCaptureRequest` and `RuntimeInputRebindingFocusCaptureResult` are public `mirakana_runtime` contracts.
- `capture_runtime_input_rebinding_action_with_focus` blocks when the capture is not armed, the capture id is empty, the focused id does not match, or a non-empty modal layer does not match the capture id.
- The wrapper delegates to `capture_runtime_input_rebinding_action` only after the focus guard passes.
- Waiting and captured results set `gameplay_input_consumed` when requested so the same press cannot leak into gameplay.
- Waiting results set `focus_retained`; captured/blocked results do not.
- Focused tests prove waiting consumption, captured consumption/candidate profile output, and blocked focus guard diagnostics.
- Docs, manifest, master plan, plan registry, and static checks record this as runtime/game rebinding focus-consumption evidence only.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/session_services.hpp`
- Modify: `engine/runtime/src/session_services.cpp`
- Modify: `tests/unit/runtime_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

## Tasks

### Task 1: RED Focus/Consumption Tests

- [x] Add focused `MK_runtime_tests` for waiting capture consumption/focus retention, captured candidate profile consumption, and blocked focus guard diagnostics.
- [x] Run focused build/test and record the expected failure because the focus-capture symbols do not exist yet.

### Task 2: Runtime Focus Capture Wrapper

- [x] Add focus-capture request/result contracts and diagnostic codes to `session_services.hpp`.
- [x] Implement focus guard validation and safe delegation in `session_services.cpp`.
- [x] Run focused `MK_runtime_tests`.

### Task 3: Docs, Manifest, And Static Checks

- [x] Update current capabilities, AI game development guidance, roadmap, master plan, plan registry, and manifest.
- [x] Update static checks so the new API/docs markers cannot drift.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add runtime input rebinding focus consumption`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused `MK_runtime_tests` | Expected failure | Focused build failed because `RuntimeInputRebindingFocusCaptureRequest`, `capture_runtime_input_rebinding_action_with_focus`, and the focus guard diagnostics are not implemented yet. |
| Focused `MK_runtime_tests` | Passed | `cmake --build --preset dev --target MK_runtime_tests` and `ctest --preset dev --output-on-failure -R "^MK_runtime_tests$"` passed after implementing the focus capture wrapper. |
| Focused `MK_runtime_tests` after edge-case coverage | Passed | Re-ran the focused build/test after adding empty capture id and `consume_gameplay_input=false` coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `production-readiness-audit-check: ok` with 11 known unsupported production gaps still tracked. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | Timed out | Full-repository tidy did not finish within 15 minutes; this is broader than the validation lane. Followed with focused changed-file tidy and final `validate` uses `check-tidy.ps1 -MaxFiles 1`. |
| `tools/check-tidy.ps1 -Files engine/runtime/src/session_services.cpp,tests/unit/runtime_tests.cpp` | Passed | Focused changed-file clang-tidy completed with current repository warning profile and `tidy-check: ok (2 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Passed after applying `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `git diff --check` | Passed | No whitespace errors; Git reported line-ending normalization warnings only. |
| Focused `MK_runtime_tests` after formatting | Passed | Re-ran focused build/test after formatting the touched C++ files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed, including all 29 CTest tests. Metal/Apple checks reported expected diagnostic/host-gated blockers on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Full dev preset build passed after validation. |
