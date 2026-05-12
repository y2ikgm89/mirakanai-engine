# Runtime UI Accessibility Publish Plan v1 Implementation Plan (2026-05-07)

**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `mirakana_ui` accessibility publish plan so runtime UI accessibility rows are validated before an OS accessibility adapter receives them.

**Architecture:** Keep `mirakana_ui` independent from OS accessibility APIs. The new contract plans and gates publication to the existing `IAccessibilityAdapter` boundary, but does not implement UI Automation, NSAccessibility, AT-SPI, Android accessibility, iOS accessibility, or any native handle bridge.

**Tech Stack:** C++23 `mirakana_ui`, existing first-party UI tests, existing validation scripts.

---

## Goal

- Add `AccessibilityPublishPlan` / `AccessibilityPublishResult` to `mirakana/ui/ui.hpp`.
- Add `plan_accessibility_publish` to copy validated accessibility rows and preserve blocking diagnostics.
- Add `publish_accessibility_payload` to call `IAccessibilityAdapter::publish_nodes` only when the payload is valid.
- Update docs, manifest, and static checks so `production-ui-importer-platform-adapters` records this host-independent accessibility publish contract while OS accessibility bridge work remains unsupported.

## Context

- Master plan gap: `production-ui-importer-platform-adapters`.
- Existing `mirakana_ui` already builds deterministic `AccessibilityPayload` rows from `RendererSubmission`.
- Existing `IAccessibilityAdapter` is a boundary only; there is no reviewed plan/result helper to prevent invalid rows from reaching a host adapter.

## Constraints

- Do not add third-party dependencies or platform SDK calls.
- Do not expose OS handles, native accessibility objects, or middleware APIs.
- Do not claim OS accessibility bridge readiness.
- Keep the implementation deterministic and testable on the default Windows host.

## Done When

- Unit tests prove valid accessibility payloads publish once through a supplied adapter.
- Unit tests prove invalid accessibility payload diagnostics block adapter publication.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` and `ctest --preset dev --output-on-failure -R mirakana_ui_renderer_tests` pass, followed by required static/final validation.
- `production-ui-importer-platform-adapters` remains `planned` or explicitly non-ready for OS bridge work while recording this host-independent foundation.

## File Plan

- Modify `engine/ui/include/mirakana/ui/ui.hpp`: add publish plan/result contracts and functions.
- Modify `engine/ui/src/ui.cpp`: implement planning, result status, and safe adapter dispatch.
- Modify `tests/unit/ui_renderer_tests.cpp`: add RED tests for valid publish and invalid diagnostic blocking.
- Modify docs/manifest/static checks after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add tests for valid accessibility publish through a capture adapter.
- [x] Add tests for invalid accessibility bounds blocking adapter publication.
- [x] Run focused `mirakana_ui_renderer_tests` and record the expected compile/test failure.

### Task 2: Implement Accessibility Publish Contract

- [x] Add `AccessibilityPublishPlan` and `AccessibilityPublishResult`.
- [x] Implement `plan_accessibility_publish`.
- [x] Implement `publish_accessibility_payload`.
- [x] Run focused `mirakana_ui_renderer_tests`.

### Task 3: Docs And Static Contract Sync

- [x] Update current capabilities, roadmap, master plan, registry, and manifest.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to require the new API/docs evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused `mirakana_ui_renderer_tests` (now `MK_ui_renderer_tests`) | Expected fail | `plan_accessibility_publish` / `publish_accessibility_payload` were not declared yet. |
| Focused `mirakana_ui_renderer_tests` | Pass | `ctest --preset dev --output-on-failure -R mirakana_ui_renderer_tests` passed 1/1. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; unsupported gap count remains 11 and `production-ui-importer-platform-adapters` remains `planned`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `format-check: ok`. |
| `git diff --check` | Pass | No whitespace errors; Git reported existing LF-to-CRLF working-copy warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; CTest passed 29/29. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default dev build completed. |
