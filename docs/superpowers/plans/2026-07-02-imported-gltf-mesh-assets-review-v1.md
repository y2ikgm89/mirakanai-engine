# Imported glTF Mesh Assets Review v1

## Goal

Close the editor-core retained UX gap for selected imported glTF mesh assets by turning a host/tooling-owned
`GltfMeshInspectReport` into deterministic Assets-panel review rows and an inspect command without reading source files,
executing importers, mutating project files, packaging assets, running validation recipes, or exposing native handles.

## Context

- Khronos glTF 2.0 remains the authoritative format boundary for `.gltf` / `.glb` mesh inspection.
- `MK_tools` owns optional `fastgltf` parsing behind the existing `asset-importers` feature.
- `MK_editor_core` owns value-only retained rows consumed by Source Pulse / Assets panel models.
- Real large-corpus importer readiness remains host-gated by `Asset Import Regression Workflow Rows v1` and retained
  report/triage/preset/axis-unit artifacts.

## Constraints

- Keep a clean-break first-party API: no compatibility shims, no Unity/Unreal/Godot project import, no copied external
  engine UI/workflow/schema/code/assets, and no external-engine compatibility/equivalence/parity claim.
- Treat legal review as engineering evidence only; do not claim legal approval or legal advice.
- Fail closed when the selected asset is unsupported, hidden from retained source rows, missing an inspect report, has a
  failed report, or any caller input claims mutation, importer execution, package script execution, validation recipe
  execution, or native-handle exposure.
- Keep editor-core independent from runtime source parsing and host filesystem IO.

## Implementation

- Add `EditorGltfMeshInspectSelectionDesc`, `EditorGltfMeshInspectSelectionModel`,
  `EditorGltfMeshInspectSelectionStatus`, `make_editor_gltf_mesh_inspect_selection_model`, and
  `make_editor_gltf_mesh_inspect_selection_retained_ui_desc` to `gltf_mesh_catalog.hpp`.
- Emit retained command id `asset_browser.selection.inspect` as enabled only for ready selected `.gltf` / `.glb` reports.
- Emit retained workflow row id `asset_browser.import_workflow.gltf_mesh_inspect.<asset_id>` with primitive/warning
  counts, selected/source visibility, host-owned status, and unsafe-execution flags.
- Reuse existing inspector rows from `gltf_mesh_inspect_report_to_inspector_rows`.

## Done When

- `MK_editor_core_tests` proves ready, unsupported, and failed-report selection rows.
- Docs, manifest fragments, and static AI-integration needles mention the new retained glTF Assets review contract.
- Focused validation and final repository validation pass, or any blocker is recorded with the exact failing command.

## Validation Evidence

| Command | Purpose | Result |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Configure linked worktree after `prepare-worktree`. | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` | TDD red check after adding the test. | Failed as expected: new glTF selection API was undefined. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` | Build editor-core test target after implementation. | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests` | Focused retained row/command contract tests. | Passed: 1/1 test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Repository C++ and text formatting. | Passed after `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/gltf_mesh_catalog.cpp,tests/unit/editor_core_tests.cpp` | Focused clang-tidy for changed C++ implementation/test. | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Manifest composition and JSON contracts. | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Agent-surface drift and retained row/static needles. | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Agent baseline consistency after static-surface edits. | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Public API boundary check. | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Full slice gate. | Passed: 172/172 tests. |
