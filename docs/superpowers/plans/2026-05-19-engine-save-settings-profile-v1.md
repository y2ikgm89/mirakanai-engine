# Engine Save Settings Profile v1 (2026-05-19)

**Plan ID:** `engine-save-settings-profile-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is a developer-owned capability milestone, not a reopened Engine 1.0 production gap.

## Goal

Turn the existing `MK_runtime` save data, settings, and input rebinding profile documents into a safe game-local profile workflow that AI-generated games can use for deterministic save/load, settings reset, corrupt or old-version diagnostics, and profile file placement without native handles or platform-specific paths.

## Context

- Engine 1.0 closeout remains zero-gap.
- `gameplay-authoring-foundation-v1` completed Runtime Scene Gameplay Binding v1 and Runtime Scene Gameplay Interaction Framework v1.
- The developer-owned backlog identifies `engine-save-settings-profile-v1` as the next foundational unblocker.
- `MK_runtime` already owns `RuntimeSaveData`, `RuntimeSettings`, and `RuntimeInputRebindingProfile` serialization/load/write helpers through `IFileSystem`; the missing piece is an explicit user-data/profile policy that composes those documents for generated games.

## Constraints

- Keep `unsupportedProductionGaps` empty. If this work requires reopening an Engine 1.0 production gap, stop.
- Keep the API value-oriented and host-independent. Do not expose OS paths, cloud saves, binary blobs, registry APIs, native handles, SDL3, editor, renderer, RHI, D3D12, Vulkan, Metal, or middleware.
- Preserve strict deterministic text documents and fail-closed diagnostics for malformed/corrupt inputs.
- Use RED tests before behavior/API changes and close C++/runtime/public-contract phases with focused checks plus one fresh `tools/validate.ps1`.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Make the manifest pointers, master-plan ledger, and plan registry agree that `engine-save-settings-profile-v1` is the active developer-owned capability after gameplay authoring foundation, while keeping `unsupportedProductionGaps = []`.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` slice.
- The production master plan and readiness ledger name this milestone as developer-owned capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Runtime Profile Path Policy

**Status:** Completed.

### Goal

Add a small `MK_runtime` public contract that derives safe, deterministic game-local save/settings/input-profile file paths from a game id and profile id.

### Planned API Shape

- `RuntimeSessionProfilePathRequest`: game id, profile id, and optional root path.
- `RuntimeSessionProfilePathPlan`: save data path, settings path, input rebinding profile path, diagnostics, and `succeeded()`.
- `plan_runtime_session_profile_paths`: fail-closed path policy helper.

### Implemented Surface

- `MK_runtime` exposes `RuntimeSessionProfilePathRequest`, `RuntimeSessionProfilePathDiagnosticCode`, `RuntimeSessionProfilePathDiagnostic`, `RuntimeSessionProfilePathPlan`, and `plan_runtime_session_profile_paths`.
- Valid plans produce project-relative `save.gesave`, `settings.settings`, and `input.geinputprofile` paths under `<root>/<game_id>/<profile_id>/`.
- Invalid game ids, profile ids, root paths, absolute paths, parent traversal, path separators in ids, and control characters produce diagnostics and no partial paths.

### Done When

- RED tests prove valid game/profile ids produce deterministic profile-local paths for save data, settings, and input rebinding profile documents.
- RED tests prove empty ids, unsafe path segments, absolute paths, parent traversal, and control characters produce diagnostics and no partial paths.
- Focused `MK_runtime_tests` passes.
- Public API boundary, agent/static drift checks, composed manifest, docs, and full validation are updated.

## Phase 2: Runtime Profile Document Bundle

**Status:** Completed.

### Goal

Layer a value-type profile document bundle over the path policy so generated games can load existing documents, apply defaults for missing optional documents, and report corrupt/unsupported-version diagnostics before gameplay starts.

### Implemented Surface

- `MK_runtime` exposes `RuntimeSessionProfileDocuments`, `RuntimeSessionProfileDocumentRow`, `RuntimeSessionProfileDocumentLoadRequest`, `RuntimeSessionProfileDocumentLoadResult`, `RuntimeSessionProfileDocumentWriteRequest`, `RuntimeSessionProfileDocumentWriteResult`, `load_runtime_session_profile_documents`, and `write_runtime_session_profile_documents`.
- Load rows distinguish `loaded`, `defaulted_missing`, `failed_corrupt`, `failed_unsupported_version`, and `failed_invalid_path` statuses for save/settings/input-profile documents.
- Write rows validate all three reviewed default documents before writing `save.gesave`, `settings.settings`, and `input.geinputprofile`; unrelated profile-root files are not deleted.
- `sample_gameplay_foundation` demonstrates source-tree default write and load before gameplay starts.

### Done When

- Save/settings/input-profile load rows are deterministic and report missing, corrupt, and unsupported-version inputs separately.
- Reset/write helpers can produce reviewed default profile documents without deleting unrelated files or guessing platform paths.
- Sample source-tree usage demonstrates the workflow.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests` failed before implementation because `RuntimeSessionProfilePathRequest`, `RuntimeSessionProfilePathDiagnosticCode`, and `plan_runtime_session_profile_paths` were undefined.
- Focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests` passed.
- Focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests` passed.
- Static/drift: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-ai-integration.ps1` passed after docs/manifest/static guard updates.
- Phase 1 full phase gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit` reported `unsupported_gaps=0`.
- Phase 2 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests` failed before implementation because `RuntimeSessionProfileDocuments`, `RuntimeSessionProfileDocumentRow`, `load_runtime_session_profile_documents`, and `write_runtime_session_profile_documents` were undefined.
- Phase 2 focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_tests` and `--target sample_gameplay_foundation` passed. One parallel `sample_gameplay_foundation` build hit transient MSVC C1041 PDB contention and passed when rerun alone.
- Phase 2 focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_tests` and `-R sample_gameplay_foundation` passed.
- Phase 2 static/drift: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, and `tools/check-ai-integration.ps1` passed after docs/manifest/static guard updates.
- Phase 2 full phase gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit` reported `unsupported_gaps=0`.
