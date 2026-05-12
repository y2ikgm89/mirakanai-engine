# Runtime Input Rebinding Presentation Rows v1 Implementation Plan (2026-05-08)

**Plan ID:** `runtime-input-rebinding-presentation-rows-v1`
**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add host-independent `mirakana_runtime` input rebinding presentation rows so runtime/game rebinding UI can display reviewed base/profile bindings and symbolic glyph lookup keys without depending on editor code, SDL3, native handles, or platform glyph generation.

**Architecture:** Keep the contract in `mirakana_runtime` over `RuntimeInputActionMap`, `RuntimeInputRebindingProfile`, and first-party input enum values only. The API may produce deterministic labels and symbolic glyph lookup keys, but it must not generate image glyphs, query OS keyboard layouts, open devices, mutate files, or depend on `mirakana_ui`, `mirakana_editor`, SDL3, Dear ImGui, or input middleware.

**Tech Stack:** C++23 `mirakana_runtime`, existing `MK_runtime_tests`, existing docs/manifest/static checks.

---

## Goal

- Add `RuntimeInputRebindingPresentationToken`, `RuntimeInputRebindingPresentationRow`, and `RuntimeInputRebindingPresentationModel`.
- Add `present_runtime_input_action_trigger` and `present_runtime_input_axis_source` for deterministic trigger/source labels plus symbolic `glyph_lookup_key` values.
- Add `make_runtime_input_rebinding_presentation` to produce action/axis rows for base bindings and profile overrides.
- Update docs, manifest, and static checks so runtime/game rebinding UX records this display-ready contract while real platform glyph generation, keyboard-layout localization, files/cloud saves, axis capture, device assignment, and full runtime panels remain unsupported.

## Context

- Runtime Input Rebinding Capture Contract v1 and Runtime Input Rebinding Focus Consumption v1 provide host-independent capture and focus/consumption guards.
- Editor Input Rebinding Profile Panel v1 has editor-local string formatting for binding rows, but generated games and runtime UI must not depend on editor code.
- Remaining unsupported work includes full runtime/game rebinding panels, real input glyph generation, platform keyboard layout localization, axis capture, multiplayer device assignment, SDL3/native handles, file/cloud/binary saves, and broad input middleware.

## Constraints

- Do not add dependencies, SDL3/native handles, OS keyboard APIs, input middleware, file/cloud/binary save mutation, editor dependencies, `mirakana_ui` dependencies, or generated image glyph assets.
- Keep labels and glyph lookup keys deterministic and ASCII.
- Treat symbolic glyph lookup keys as identifiers only; they are not generated glyph textures or platform-specific button art.
- Reuse existing runtime input validation and rebinding diagnostics instead of inventing a separate diagnostic channel.
- Preserve existing action-map, profile-apply, and capture semantics.

## Done When

- Tests prove action rows expose base and profile trigger presentation tokens with stable labels and symbolic glyph lookup keys.
- Tests prove axis rows expose key-pair and gamepad-axis presentation tokens, including scale/deadzone metadata.
- Tests prove invalid profile rows report existing rebinding diagnostics, mark the presentation model not ready, and keep action diagnostic paths aligned with presentation row ids (`model.diagnostics[0].path == row->id`).
- Focused `MK_runtime_tests` pass, followed by required static/final validation.
- Docs/manifest/static checks record runtime input rebinding presentation rows as display contracts only, without claiming platform glyph generation or full runtime rebinding panels.

## File Plan

- Modify `engine/runtime/include/mirakana/runtime/session_services.hpp`: add presentation enums/structs and function declarations.
- Modify `engine/runtime/src/session_services.cpp`: implement deterministic labels, symbolic glyph lookup keys, and presentation row building.
- Modify `tests/unit/runtime_tests.cpp`: add RED tests for action rows, axis rows, and invalid profile diagnostics.
- Modify docs/manifest/static checks after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add focused `MK_runtime_tests` for action binding presentation rows.
- [x] Add focused `MK_runtime_tests` for axis binding presentation rows.
- [x] Add focused `MK_runtime_tests` for invalid profile diagnostics and not-ready presentation models.
- [x] Run focused `MK_runtime_tests` and record the expected compile/test failure.

### Task 2: Runtime Presentation Contract

- [x] Add presentation token/row/model contracts.
- [x] Implement trigger/source presentation helpers.
- [x] Implement `make_runtime_input_rebinding_presentation`.
- [x] Run focused `MK_runtime_tests`.

### Task 3: Docs And Static Contract Sync

- [x] Update current capabilities, AI game development guidance, roadmap, master plan, plan registry, and manifest.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to require the new API/docs evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run focused changed-file tidy.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused `MK_runtime_tests` | Expected failure | `cmake --build --preset dev --target MK_runtime_tests` failed because `RuntimeInputRebindingPresentationRow`, `RuntimeInputRebindingPresentationTokenKind`, `RuntimeInputRebindingPresentationRowKind`, and `make_runtime_input_rebinding_presentation` were not implemented yet. |
| Focused `MK_runtime_tests` | Pass | `cmake --build --preset dev --target MK_runtime_tests` and `ctest --preset dev --output-on-failure -R "^MK_runtime_tests$"` passed after implementing presentation rows, symbolic glyph lookup keys, and action diagnostic path/row-id correlation (`model.diagnostics[0].path == row->id`). |
| Review follow-up focused validation | Pass | After review, action presentation row ids now use the existing `bind.<context>.<action>` diagnostic namespace. `cmake --build --preset dev --target MK_runtime_tests` and `ctest --preset dev --output-on-failure -R "^MK_runtime_tests$"` passed with the duplicate-override diagnostic path matching the presentation row id. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok` after docs/static contract sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok` after manifest/guidance/static contract sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; unsupported production gap count remains 11. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary check passed for the new `mirakana_runtime` header contract. |
| Focused changed-file tidy | Pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/session_services.cpp,tests/unit/runtime_tests.cpp` reported `tidy-check: ok (2 files)` with existing warnings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` was applied, then `format-check: ok`. |
| `git diff --check` | Pass | No whitespace errors; Git reported CRLF conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; CTest reported 29/29 tests passed. Windows host-gated Apple/Metal diagnostics remained non-blocking. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default dev build completed successfully. |
