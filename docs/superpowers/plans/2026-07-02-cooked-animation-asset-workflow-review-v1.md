# 2026-07-02 Cooked Animation Asset Workflow Review v1

**Plan ID:** `cooked-animation-asset-workflow-review-v1`

**Status:** Completed.

## Goal

Close the selected cooked quaternion animation workflow review as a first-party, value-only `MK_tools` evidence gate. The gate must prove the path from glTF node TRS review into `GameEngine.AnimationQuaternionClipSource.v1`, `GameEngine.CookedAnimationQuaternionClip.v1`, runtime quaternion clip payload review, and generated 3D package smoke review without promoting broad animation production readiness.

The slice may promote only:

- `cooked_animation_asset_workflow_review_ready=1`
- `cooked_animation_quaternion_clip_workflow_ready=1`
- `cooked_animation_gltf_animation_spec_reviewed=1`
- `cooked_animation_gltf_quaternion_import_reviewed=1`
- `cooked_animation_source_document_ready=1`
- `cooked_animation_cooked_payload_ready=1`
- `cooked_animation_runtime_payload_ready=1`
- `cooked_animation_generated_package_smoke_ready=1`
- `cooked_animation_clean_room_legal_boundary_ready=1`

It must keep these explicit non-claims:

- `cooked_animation_runtime_gltf_parsing_ready=0`
- `cooked_animation_animation_graph_ready=0`
- `cooked_animation_retargeting_ready=0`
- `cooked_animation_renderer_rhi_execution_ready=0`
- `cooked_animation_broad_skeletal_animation_ready=0`
- `cooked_animation_native_handles_exposed=0`
- `cooked_animation_external_engine_compatibility=0`
- `cooked_animation_legal_approval=0`

## Context

Context7 was checked on 2026-07-02 for current build/script/schema guidance:

- CMake `/kitware/cmake` confirmed the existing repository pattern of `add_executable`, `target_link_libraries`, `target_include_directories`, and `add_test(NAME ... COMMAND ...)` for a focused C++ unit-test target.
- Microsoft PowerShell `/microsoftdocs/powershell-docs` confirmed advanced script patterns with `#requires`, `[CmdletBinding()]`, `Write-Error`, `Write-Information`, and `ShouldProcess`; no new validator script was added after agent-surface audit found the focused CTest target sufficient.
- JSON Schema `/json-schema-org/json-schema-spec` confirmed `required`, `const`, and fail-closed object validation patterns; no JSON Schema was added because this slice adds a C++ value review model, not a JSON report contract.

Official sources and legal boundaries reviewed:

- Khronos glTF 2.0 Specification: animation channels target `"translation"`, `"rotation"`, `"scale"`, and `"weights"` paths, duplicate target `(node,path)` pairs are forbidden within one animation, samplers carry interpolation modes, inputs are time accessors, and LINEAR rotation interpolation is quaternion-based. The selected workflow only reviews LINEAR node TRS into cooked quaternion clip rows.
- SPDX License List 3.28.0 provides standardized license identifiers and canonical URLs for third-party notices.
- Creative Commons license summaries distinguish commercial, NonCommercial, and NoDerivatives terms; NC/ND rows must not be auto-promoted into commercial-ready asset evidence.
- Epic Content License records UE-Only and Non-Compatible License restrictions; Unreal/Epic content is not reusable evidence for this engine unless separately licensed and reviewed.
- Unity trademark guidance lists Unity marks and attribution requirements; this slice may mention Unity only as a non-claim category.
- Godot is MIT-licensed with attribution obligations and third-party notice requirements; this slice does not copy Godot code/assets or claim Godot compatibility.

## Constraints

- Use first-party C++23 value types in `mirakana::`.
- Do not parse glTF at runtime, execute importers, mutate packages, or run renderer/RHI work.
- Do not add dependencies, SDK headers, middleware, schema adapters, or compatibility shims.
- Do not copy Unity, Unreal, Godot, third-party sample code, assets, shaders, schemas, UI, or documentation text.
- Do not claim legal approval; this is engineering review evidence only.
- Fail closed for wrong, blank, duplicate, or placeholder evidence identities.

## Implementation

1. Add `cooked_animation_workflow_review.hpp/.cpp` with `CookedAnimationWorkflowEvidenceRow`, `CookedAnimationWorkflowReviewResult`, diagnostics, and `review_cooked_animation_asset_workflow`.
2. Require exact owned `row_id` and `official_source_id` identities for all seven evidence rows:
   - `cooked_animation.workflow.gltf_spec` / `khronos-gltf-2.0-spec-animation`
   - `cooked_animation.workflow.gltf_quaternion_trs_import` / `mirakanai-gltf-node-animation-import`
   - `cooked_animation.workflow.source_document` / `mirakanai-animation-quaternion-clip-source-v1`
   - `cooked_animation.workflow.cooked_payload` / `mirakanai-cooked-animation-quaternion-clip-v1`
   - `cooked_animation.workflow.runtime_payload` / `mirakanai-runtime-animation-quaternion-clip-payload`
   - `cooked_animation.workflow.generated_package_smoke` / `mirakanai-generated-3d-quaternion-package-smoke`
   - `cooked_animation.workflow.clean_room_legal_boundary` / `mirakanai-clean-room-legal-boundary`
3. Require reviewed glTF 2.0 animation obligations, explicit mesh preset, coordinate normalization, unit-quaternion validation, unsupported interpolation fail-closed behavior, source document validation, cooked deterministic float32 payload, runtime payload reader, generated package smoke, and clean-room legal boundary evidence.
4. Block runtime glTF parsing readiness, animation graph readiness, retargeting readiness, renderer/RHI execution readiness, broad skeletal animation readiness, native handle exposure, copied external engine material, Unity/Unreal/Godot compatibility claims, and legal approval claims.
5. Register the public header in the `MK_tools` manifest fragment and compose `engine/agent/manifest.json`.
6. Update capabilities, roadmap, plan registry, and AI static integration checks.

## Done When

- Focused tests cover ready evidence, exact identity rejection, duplicate/blank identity rejection, missing glTF obligations, missing required rows, clean-room legal boundary evidence, and unsupported broad/external/legal claims.
- `engine/agent/manifest.json` includes the new public header and retained evidence after manifest compose.
- Static checks lock the public API, docs, manifest, and forbidden-claim boundary.
- Full validation passes or the exact blocker is recorded.

## Validation Evidence

Completed local validation on 2026-07-02:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` passed and regenerated `engine/agent/manifest.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_cooked_animation_workflow_review_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_cooked_animation_workflow_review_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` passed with zero tracked text rewrites.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `git diff --check` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok` and 172/172 tests passed.
