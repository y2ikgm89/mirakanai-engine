# Editor Input Rebinding Profile Persistence v1 (2026-05-10)

**Plan ID:** `editor-input-rebinding-profile-persistence-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Status:** Completed  

## Goal

Add **explicit, reviewed** Save and Load of in-memory `RuntimeInputRebindingProfile` to project-store-relative paths using the existing `GameEngine.RuntimeInputRebindingProfile.v1` text contract (`serialize_runtime_input_rebinding_profile` / `deserialize_runtime_input_rebinding_profile`), with editor-core path validation, diagnostics, retained `MK_ui` rows under `input_rebinding.persistence`, and visible `MK_editor` controls. No automatic save, cloud sync, or multiplayer device assignment.

## Context

- Runtime IO already exposes `serialize_runtime_input_rebinding_profile`, `deserialize_runtime_input_rebinding_profile`, and `write_runtime_input_rebinding_profile` over `IFileSystem`; the editor uses `ITextStore` (`FileTextStore`) for project-relative paths.
- `EditorInputRebindingProfileReviewModel::ready_for_save` gates serialization readiness; capture flows remain in-memory until the operator uses Save.

## Constraints

- Paths must be **project-relative**, reject `..`, absolute segments, and unsafe characters consistent with other editor file rows.
- Allowed extension for this slice: **`.inputrebinding`** only (fail-closed validation before read/write).
- Do not add cloud/binary save backends, automatic persistence, SDL3/Dear ImGui in `MK_editor_core`, native handle exposure, or broaden `editor-productization` ready claims beyond this persistence lane.

## Done when

- `validate_editor_input_rebinding_profile_store_path`, `save_editor_input_rebinding_profile_to_project_store`, and `load_editor_input_rebinding_profile_from_project_store` live in `MK_editor_core` with `MemoryTextStore` unit coverage.
- `make_input_rebinding_profile_panel_ui_model` accepts optional persistence UI payload; retained ids include `input_rebinding.persistence.path`, `input_rebinding.persistence.last_status`, and deterministic `input_rebinding.persistence.diagnostics.*` rows when diagnostics exist.
- `MK_editor` exposes path input (char buffer), **Save profile** / **Load profile** buttons, and surfaces last operation status; load replaces the in-memory profile only on success.
- `tools/check-ai-integration.ps1` needles, `docs/editor.md` (short note), `docs/superpowers/plans/README.md`, and `engine/agent/manifest.json` `editor-productization` notes / `currentActivePlan` / `recommendedNextPlan` evidence align with this slice.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the development host.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
