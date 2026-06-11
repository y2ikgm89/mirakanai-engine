# 2026-06-11 Environment Settings Productization v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Complete the first-party environment settings feature as an operator-usable editor-to-preview-to-package workflow over the existing environment system evidence.

**Architecture:** Keep `MK_environment` as the backend-neutral value, profile, volume, weather, quality-budget, and IO contract. Keep `MK_editor_core` responsible for deterministic authoring documents, validation rows, command planning, undo/dirty state, package registration drafts, and preview requests. Keep visible editor surfaces in first-party `MK_editor` / `mirakana::ui` panels. Keep renderer/RHI/runtime execution in existing backend-private runtime/package recipes; editor-core may request or review execution but must not execute backend work, package scripts, validation recipes, native handles, Dear ImGui, SDL3, or UI middleware.

**Tech Stack:** C++23, PowerShell 7 repository tools, `MK_environment`, `MK_editor_core`, `MK_editor`, first-party `mirakana::ui`, `MK_runtime`, `MK_renderer`, `MK_rhi` backend-private evidence lanes, `sample_desktop_runtime_game` package smokes, official engine/editor documentation, and Context7 re-checks before implementation.

---

**Plan ID:** `environment-settings-productization-v1`

**Status:** Active. Selected implementation slice in this branch. `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` points `currentActivePlan` at this plan; compose `engine/agent/manifest.json` after every manifest-fragment edit.

## Definition Of Completion

This plan completes the **environment settings feature**, not broad renderer/backend parity.

Complete means:

- Operators can open a dedicated first-party Environment Settings surface, inspect and edit `GameEngine.EnvironmentProfile.v2` global settings, local volumes, weather timeline keyframes, quality preset/budget status, selected readiness rows, preview requests, and package-registration drafts.
- Editor-core emits deterministic command plans for every user-visible setting mutation and rejects unsafe backend execution, package-script execution, validation-recipe execution, arbitrary shell execution, and public native-handle access.
- The visible native editor shell exposes environment settings as a first-class panel and keeps the existing Inspector projection for selected context.
- Preview and package workflows are operator-reviewed handoffs over existing runtime/package recipes; the editor does not silently execute package scripts or backend work.
- Installed package validation can prove a narrow productized settings status such as `environment_settings_productized_status=ready`, plus exact counters for profile-v2, rows, commands, preview request, package draft, validation recipe selection, native-handle absence, and broad `environment_ready` absence.
- `tools/validate-installed-desktop-runtime.ps1` continues to reject a broad `environment_ready=` status field unless a separate architecture decision changes that contract. This plan must not infer D3D12, Vulkan, Metal, backend parity, broad optimization, or all-environment readiness from the productized editor workflow.

## Current Truth

Already implemented and reused by this plan:

- `Environment System v1` completed `MK_environment` profile validation/text IO/package rows, scene/runtime binding, renderer policy planning, selected D3D12 and strict host-gated Vulkan feature evidence, and first-party editor-core authoring foundations.
- `Environment Production Excellence v1` is retained evidence with completed Phases 0-8. It records selected D3D12/Vulkan/Metal-host-gated environment rendering/package evidence, material weathering evidence, weather-audio mixer/render evidence, first-party editor Inspector rows, and fail-closed quality-budget counters.
- `editor/core/include/mirakana/editor/environment_authoring.hpp` already exposes `EnvironmentAuthoringDocument`, `EnvironmentAuthoringInspectorModel`, `EnvironmentAuthoringCommandPlan`, `make_environment_authoring_inspector_model`, `make_environment_authoring_ui_model`, `make_environment_package_candidate_rows`, and package registration draft rows.
- `tests/unit/editor_environment_tests.cpp` already proves clean-break profile-v2 load/save/undo/dirty behavior, retained Inspector rows, reviewed commands for volume/weather/quality/cubemap request, package registration draft review, and native editor Inspector projection without Dear ImGui, SDL3, backend execution, package-script execution, or native handles.
- `docs/testing.md` states that installed environment package smokes reject broad `environment_ready=` and keep exact selected evidence rows.

Not complete yet as a productized Environment Settings feature:

- No dedicated `environment_settings` first-party panel exists in the editor dock/workspace catalog.
- Existing environment rows are primarily projected through the Inspector; there is no complete settings workflow model with tabs/sections for Global, Volumes, Weather, Quality, Preview, Package, and Readiness.
- Preview request rows are value-level review rows; there is no productized operator handoff model that connects a preview request to reviewed validation recipe choices and package smoke evidence.
- Package registration drafts exist, but the visible workflow and package validation counters do not yet prove the end-to-end settings workflow as one productized feature.
- There is no narrow final status/counter family such as `environment_settings_productized_*` to say the settings workflow is ready while keeping broad `environment_ready` rejected.
- Static checks do not yet guard the new productized settings row IDs, workflow non-claims, or broad-ready rejection as a feature-level contract.

## Official Source Baseline

Re-open current official sources before changing implementation. Do not copy sample code; use these as product and platform constraints for first-party implementation.

Product/editor references used for this planning pass:

- Unreal Engine Environment Light Mixer: <https://dev.epicgames.com/documentation/unreal-engine/environment-light-mixer-in-unreal-engine?lang=en-US>
- Unreal Engine Using the Light Mixer: <https://dev.epicgames.com/documentation/unreal-engine/using-the-light-mixer-in-unreal-engine?lang=en-US>
- Unity HDRP Visual Environment: <https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition%4012.0/manual/Override-Visual-Environment.html>
- Unity HDRP Visual Environment volume override reference: <https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4016.0/manual/visual-environment-volume-override-reference.html>
- Unity HDRP Volume Profile: <https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition%4012.0/manual/Volume-Profile.html>
- Unity UI Toolkit custom Inspector: <https://docs.unity3d.com/6000.4/Documentation/Manual/UIE-HowTo-CreateCustomInspector.html>
- Godot Environment and post-processing: <https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html>
- Godot custom post-processing: <https://docs.godotengine.org/en/stable/tutorials/shaders/custom_postprocessing.html>

Implementation references to refresh if preview/package/rendering contracts change:

- Microsoft Direct3D 12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- Microsoft Direct3D 12 descriptors and resource binding: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding-flow-of-control>
- Khronos Vulkan synchronization: <https://docs.vulkan.org/spec/latest/chapters/synchronization.html>
- Khronos Vulkan synchronization2 guide: <https://docs.vulkan.org/guide/latest/extensions/VK_KHR_synchronization2.html>
- Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/resource-synchronization>
- Apple Metal capabilities: <https://developer.apple.com/metal/capabilities/>

Context7 status on 2026-06-11:

- `mcp__context7.resolve_library_id` for Godot documentation failed with `Invalid or expired OAuth token. Please re-authenticate to obtain a new token.`
- Before implementation starts, rerun Context7 for the documentation touched by the task: Godot Environment docs, Unity UI Toolkit / HDRP docs, Unreal environment editor docs, Microsoft D3D12 docs if D3D12 preview/package code changes, Khronos Vulkan docs if Vulkan recipe code changes, and Apple Metal docs if Metal host gates change.
- If Context7 remains unavailable, record the blocker in the task evidence and rely only on official web documentation listed above plus repository tests/docs. Do not proceed from memory for SDK/library behavior.

Source implications for this repo:

- Unreal's environment mixer pattern supports a dedicated unified editor surface for environment lighting/sky/fog/cloud state. This repo should implement a first-party Environment Settings panel instead of burying the workflow only in a generic Inspector.
- Unity HDRP's Volume/Profile model supports the existing `EnvironmentProfileDocumentV2` global plus local-volume design. This repo should preserve deterministic profile/volume rows and undoable editor binding without adopting Unity APIs.
- Unity custom Inspector guidance supports data-bound editor controls and undo-safe editing. This repo should implement equivalent first-party command plans and dirty/undo state through `MK_editor_core`.
- Godot separates Environment resources from performance/quality project settings in Godot 4. This repo should keep visual profile rows and quality-budget rows explicit rather than mixing broad optimization claims into environment readiness.
- Backend docs are relevant only when a task changes runtime/renderer execution. Editor workflow changes must not infer backend readiness.

## Resolved Scope Decisions

- Environment settings means the first-party editor/runtime workflow for configuring and validating existing environment capability rows. It is not a request to redo Environment System v1.
- The plan should build on retained Environment Production Excellence evidence and should not reopen completed milestone phases.
- The final ready row for this plan is a narrow environment-settings productization row, not broad `environment_ready`.
- `environment_ready` remains rejected by package validation unless a later separate aggregate plan changes the static contract and proves every constituent meaning.
- D3D12 remains the primary local preview/package smoke lane where a host is required. Vulkan and Metal rows remain exact selected recipe rows and cannot be inferred.
- The visible editor shell must use first-party `mirakana::ui` and existing dock/workspace patterns. Do not add Dear ImGui, SDL3, Qt, Slint, RmlUi, or other UI middleware.
- The editor can emit reviewed preview/package/validation handoff rows, but must not silently execute package scripts, shell commands, validation recipes, backend work, or native-handle access.
- Optional texture/HDR/importer dependencies such as OpenEXR, KTX, Basis, and source image decoders remain excluded unless a selected dependency/license plan updates `vcpkg.json`, dependency docs, legal notices, package evidence, and validation.

## Target Productized Claim Matrix

| Claim | Current state | Promotion target |
| --- | --- | --- |
| `environment_profile_v2_status` | Selected package evidence exists. | Reused as prerequisite; no new parser or compatibility shim. |
| `environment_authoring_core_status` | Inspector rows and reviewed commands exist. | Extend only if RED tests expose missing user-visible settings commands; preserve no backend execution and no native handles. |
| `environment_settings_panel_status` | Missing. | Dedicated first-party native panel registered in workspace/dock catalog with stable row IDs and tests. |
| `environment_settings_workflow_status` | Missing. | Global, Volumes, Weather, Quality, Preview, Package, and Readiness sections/tabs represented in a deterministic UI/workflow model. |
| `environment_settings_preview_request_status` | Value-level cubemap request exists. | Operator-reviewed preview handoff rows map requests to exact validation recipe choices without executing backend work in editor-core. |
| `environment_settings_package_draft_status` | Package candidate/draft rows exist. | Visible workflow and package validation counters prove source/cooked/index drafts, duplicate/unsafe path rejection, and existing runtime-file handling. |
| `environment_settings_quality_budget_status` | Selected quality-budget counters exist. | Panel exposes preset, budget row count, diagnostics, violations, and broad optimization/environment-ready non-claims. |
| `environment_settings_readiness_status` | Generic Inspector readiness rows exist. | Productized readiness dashboard summarizes exact selected rows, host gates, blockers, and unsupported broad claims. |
| `environment_settings_productized_status` | Missing. | Installed package smoke/static checks prove the workflow is complete without broad `environment_ready`. |
| Broad `environment_ready` | Explicitly rejected/unclaimed. | Remains rejected by this plan. Future aggregate requires a separate architecture decision and static contract. |

## File Responsibility Map

Expected files for implementation. A task may touch fewer files if RED tests prove the narrower path is enough.

| Area | Files |
| --- | --- |
| Editor-core environment settings workflow | `editor/core/include/mirakana/editor/environment_authoring.hpp`, `editor/core/src/environment_authoring.cpp`, `tests/unit/editor_environment_tests.cpp` |
| Workspace/dock/panel registration | `editor/core/include/mirakana/editor/workspace.hpp`, `editor/core/src/workspace.cpp`, `editor/core/include/mirakana/editor/editor_dock_layout.hpp`, `editor/core/src/editor_dock_layout.cpp` |
| Native visible editor shell | `editor/src/native_editor_app.hpp`, `editor/src/native_editor_app.cpp`, `editor/src/first_party_editor_document.*`, `tests/unit/editor_native_shell_tests.cpp`, `tests/unit/editor_environment_tests.cpp` |
| Runtime/package evidence | `games/sample_desktop_runtime_game/main.cpp`, `games/sample_desktop_runtime_game/game.agent.json`, `tools/validate-installed-desktop-runtime.ps1`, `tools/validation-recipe-core.ps1`, `tools/run-validation-recipe-plans.ps1` |
| Agent/docs/static contracts | `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.fragments/*.json`, generated `engine/agent/manifest.json`, focused `tools/check-ai-integration-*.ps1` or `tools/check-json-contracts-*.ps1` if new contract literals need enforcement |

## Execution Strategy

- Use a dedicated implementation worktree and keep this candidate plan docs-only until selected.
- Use TDD for every behavior change: add focused RED tests, confirm they fail for the intended reason, implement, then run focused GREEN checks.
- Reuse current `EnvironmentAuthoringDocument` and command-plan shapes before adding new public API.
- Add panel/workflow rows in small phases so docs/manifest/static checks can remain honest after each slice.
- Batch docs/manifest/static-check sync after behavior is green unless those files are the behavior under test.
- Run full `tools/validate.ps1` once at the close of any C++/runtime/package/public-contract slice.
- Run `tools/check-publication-preflight.ps1` before staging, pushing, PR creation, ready conversion, or merge automation.

## Task 0: Selection, Current Audit, And Source Refresh

**Goal:** Select this candidate only when implementation starts, then eliminate stale assumptions before code edits.

**Files:**

- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` only when selecting this plan.
- Generate `engine/agent/manifest.json` only when manifest fragments change.
- Update this plan's status after selection.

- [x] Confirm the worktree is isolated and clean:

```powershell
git status --short --branch
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal
```

- [x] Re-audit current environment evidence with targeted searches:

```powershell
rg -n "environment_settings|environment_ready|environment_profile_v2|environment_authoring|environment_material_weathering|environment_audio_playback|environment_lighting" editor engine tests tools docs games
```

- [x] Re-open the official docs listed in this plan for any area being changed.
- [x] Rerun Context7 lookups for the documentation touched by the phase. If Context7 auth is still invalid, record the blocker and official-doc fallback evidence in this task section.
- [x] If selected, set `currentActivePlan` and `recommendedNextPlan` to this plan, compose the manifest, and run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Expected: this plan becomes the selected implementation slice without claiming new runtime behavior.

Task 0 evidence on 2026-06-11:

- `git status --short --branch`: clean linked worktree on `codex/environment-settings-productization-plan...origin/codex/environment-settings-productization-plan`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal`: confirmed pre-selection `currentActivePlan` was the production-completion master plan and `recommendedNextPlan.id = next-production-gap-selection`.
- Targeted `rg` audit confirmed `environment_settings_*` exists only in this plan, while existing `environment_authoring`, `environment_profile_v2`, `environment_material_weathering`, `environment_audio_playback`, `environment_lighting`, and broad `environment_ready` rejection evidence already exist in editor/runtime/docs/tools.
- Context7 `resolve_library_id` for Godot documentation failed again with `Invalid or expired OAuth token. Please re-authenticate to obtain a new token.` Implementation proceeds from official docs only until Context7 auth is fixed.
- Official docs re-opened for this slice: Unreal Environment Light Mixer / Using the Light Mixer, Unity HDRP Visual Environment / Volume Profile / UI Toolkit custom Inspector, and Godot Environment and post-processing / custom post-processing. The implementation implication remains a dedicated first-party environment settings panel/workflow over profile/volume rows, undo-safe command planning, explicit quality-budget separation, and no backend-readiness inference.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` now selects `environment-settings-productization-v1`, with retained static-check legacy context preserved instead of deleting historical MAVG/frame-graph/selection-gate evidence.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`: wrote `engine/agent/manifest.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: `json-contract-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: `ai-integration-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`: `agent-config-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`: `text-format-check: ok`.

## Task 1: Environment Settings Workflow Model

**Goal:** Add a deterministic editor-core workflow model for the full settings surface while reusing existing authoring data.

**Files:** `environment_authoring.hpp`, `environment_authoring.cpp`, `editor_environment_tests.cpp`.

- [x] Add RED tests for a settings workflow model with stable section/tab IDs:
  - `environment_settings.global`
  - `environment_settings.volumes`
  - `environment_settings.weather`
  - `environment_settings.quality`
  - `environment_settings.preview`
  - `environment_settings.package`
  - `environment_settings.readiness`
- [x] The RED tests must prove the model includes profile path, dirty/revision state, profile-v2 volume count, weather keyframe count, quality preset, diagnostics, selected readiness rows, package draft summary, and unsupported broad-claim rows.
- [x] Add the smallest API needed, for example `EnvironmentSettingsWorkflowModel` and `make_environment_settings_workflow_model`, if existing `EnvironmentAuthoringInspectorModel` cannot represent the workflow cleanly.
- [x] Preserve existing `make_environment_authoring_inspector_model` and Inspector row behavior unless a RED test proves a contract should move.
- [x] Verify no backend execution, package-script execution, validation-recipe execution, shell execution, public native handles, Dear ImGui, SDL3, or middleware flags can become true from editor-core.

Focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_editor_environment_tests$"
```

Task 1 evidence on 2026-06-11:

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests` failed with missing `EnvironmentSettingsWorkflowSectionRow`, `EnvironmentSettingsWorkflowModel`, `EnvironmentSettingsWorkflowDesc`, and `make_environment_settings_workflow_model`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_editor_environment_tests --output-on-failure` passed.
- Format: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.

## Task 2: Complete User-Visible Setting Commands

**Goal:** Ensure every editable environment settings control has a reviewed command or remains intentionally read-only.

**Files:** `environment_authoring.hpp`, `environment_authoring.cpp`, `editor_environment_tests.cpp`.

- [ ] Add RED tests for command coverage across:
  - global profile replacement through existing undoable profile edit action,
  - volume add/remove/reorder/edit,
  - weather keyframe add/remove/edit/reorder if missing,
  - quality preset selection,
  - cubemap/preview request,
  - package draft review request if represented as a command.
- [ ] If current code lacks a visible command that a real settings surface needs, add a clean-break command kind and deterministic `environment.command.*` ID.
- [ ] Reject unsafe requests with diagnostics if they request backend execution, package-script execution, validation-recipe execution, arbitrary shell execution, or native-handle access.
- [ ] Keep read-only readiness rows non-editable and test that they cannot be mutated through settings commands.

Focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^MK_editor_environment_tests$"
```

## Task 3: Dedicated Native Environment Settings Panel

**Goal:** Promote environment settings from Inspector-only projection to a first-class native editor panel.

**Files:** `workspace.hpp`, `workspace.cpp`, `editor_dock_layout.hpp`, `editor_dock_layout.cpp`, native editor shell files, `editor_native_shell_tests.cpp`, `editor_environment_tests.cpp`.

- [ ] Add RED tests proving `PanelId::environment_settings`, `panel_id_to_string`, serialization/deserialization, default workspace state, and dock catalog registration.
- [ ] Add RED native shell tests proving `app.has_native_panel("environment_settings")` and a stable `editor.panel.environment_settings` UI root exists.
- [ ] Compose the Environment Settings panel from the workflow model with first-party `mirakana::ui` elements and stable element IDs.
- [ ] Keep the existing Inspector projection as context rows; do not remove current Inspector tests.
- [ ] Ensure panel visibility and layout changes do not resize or mutate unrelated panels unexpectedly.
- [ ] Verify no Dear ImGui, SDL3, UI middleware, public native handles, or backend execution are introduced.

Focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_editor_environment_tests|MK_editor_native_shell_tests)$"
```

## Task 4: Preview And Validation Handoff Rows

**Goal:** Make preview requests actionable without editor-core executing runtime/backend work.

**Files:** editor-core environment authoring files, native editor shell files, AI command/review model files only if existing models need extension, tests.

- [ ] Add RED tests for preview handoff rows that connect `environment.command.capture.cubemap.request` to reviewed recipe IDs and host gates.
- [ ] Include exact recipe/status rows for D3D12 local smoke, strict Vulkan smoke when host/toolchain gates are available, and Metal Apple-host recipe when relevant.
- [ ] Encode whether a preview request is `available`, `requested`, `blocked_by_validation`, `host_gated`, or `ready_for_operator_handoff`.
- [ ] Ensure preview handoff rows cannot execute package scripts or validation recipes from editor-core.
- [ ] Ensure host-gated recipes surface blockers rather than silently passing.

Focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_environment_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_editor_environment_tests|MK_editor_native_shell_tests)$"
```

## Task 5: Package Draft Workflow And Runtime Smoke Counters

**Goal:** Prove the settings workflow can prepare package additions and that installed package validation can verify the productized settings status.

**Files:** `games/sample_desktop_runtime_game/main.cpp`, `game.agent.json`, package/validation PowerShell scripts, editor environment tests.

- [ ] Add RED package smoke expectations for narrow productization counters:
  - `environment_settings_productized_status=ready`
  - `environment_settings_productized_ready=1`
  - `environment_settings_profile_v2_ready=1`
  - `environment_settings_panel_rows>0`
  - `environment_settings_command_rows>0`
  - `environment_settings_preview_request_rows>0`
  - `environment_settings_package_draft_rows>0`
  - `environment_settings_validation_recipe_rows>0`
  - `environment_settings_native_handle_access=0`
  - `environment_settings_backend_execution_from_editor=0`
  - `environment_settings_package_script_execution_from_editor=0`
  - `environment_settings_broad_environment_ready_claimed=0`
- [ ] Add or update validation recipe entries for a selected installed smoke such as `desktop-runtime-sample-game-environment-settings-productized`.
- [ ] Ensure the recipe reuses existing environment profile, quality budget, lighting, precipitation, material weathering, and audio evidence instead of duplicating renderer logic.
- [ ] Keep validation failure messages concrete for missing panel rows, missing package drafts, unsafe editor execution flags, missing selected evidence, and unexpected broad-ready claims.
- [ ] Update `game.agent.json` with exact evidence text and non-claims.

Focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders -SmokeArgs @('--smoke','--max-frames','2','--require-config','runtime/sample_desktop_runtime_game.config','--require-scene-package','runtime/sample_desktop_runtime_game.geindex','--require-d3d12-scene-shaders','--require-d3d12-renderer','--require-scene-gpu-bindings','--require-environment-settings-productized')
```

## Task 6: Static Guards And Agent Surface Sync

**Goal:** Make the new productized feature contract durable and AI-operable.

**Files:** docs, manifest fragments, generated manifest, static checks, plan registry.

- [ ] Update `docs/testing.md` with the exact `environment_settings_productized_*` recipe and counters.
- [ ] Update `docs/current-capabilities.md` and `docs/roadmap.md` with the new ready claim and non-claims.
- [ ] Update `docs/superpowers/plans/README.md` from candidate to active/completed status as appropriate.
- [ ] Update `engine/agent/manifest.fragments/009-validationRecipes.json` and other relevant fragments; run `tools/compose-agent-manifest.ps1 -Write`.
- [ ] Add focused static checks for:
  - Environment Settings panel row IDs.
  - Productized settings package recipe ID.
  - `environment_settings_broad_environment_ready_claimed=0`.
  - Continued rejection or absence of broad `environment_ready=` status.
  - No Dear ImGui/SDL3/native-handle claims.
- [ ] Keep Codex/Claude/Cursor agent surfaces synchronized only if durable workflow guidance changes.

Focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

## Task 7: Final Validation, Closeout, And Publication

**Goal:** Close the productized environment settings slice with full evidence.

- [ ] Run focused tests affected by the final patch.
- [ ] Run public API checks if public headers changed.
- [ ] Run full validation because implementation will touch C++ editor/runtime/package contracts:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Run publication preflight before staging/push/PR:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
git diff --check
```

- [ ] Update this plan's status with exact validation evidence.
- [ ] If this plan was selected, move `currentActivePlan` back to the production-completion master plan or the next selected plan in the same closeout change; compose the manifest and run JSON/static checks.
- [ ] Open a focused PR with validation evidence and explicit non-claims.

## Non-Claims

This plan does not claim broad `environment_ready`, broad backend parity, broad renderer quality, broad CPU/GPU/memory optimization, all-platform parity, unmanaged package-script execution, direct editor-core backend execution, physical audio endpoint playback, OpenEXR/KTX/Basis/source-image importer readiness, native/backend handle exposure, Dear ImGui, SDL3, Qt, Slint, RmlUi, or compatibility shims.
