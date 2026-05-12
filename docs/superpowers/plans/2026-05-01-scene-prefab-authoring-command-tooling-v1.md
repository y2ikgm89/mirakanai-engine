# Scene Prefab Authoring Command Tooling v1 Implementation Plan (2026-05-01)

**Status:** Completed on 2026-05-01.

> **For agentic workers:** Use this as the next focused C-phase slice after `validation-recipe-runner-tooling-v1`. Do not append this work to completed scene/package/material/UI/validation runner plans.

**Goal:** Add a reviewed AI-safe dry-run/apply command surface for stable-id Scene/Component/Prefab Schema v2 authoring operations, so agents can create scenes, add nodes/components, create prefabs, and instantiate prefabs through typed engine tooling instead of ad hoc JSON/text edits.

**Architecture:** Keep `mirakana_scene` as the renderer/platform/editor-independent owner of Scene/Component/Prefab v2 value contracts. Put command planning/apply helpers in `mirakana_tools` or editor-core-neutral tooling that consumes public `mirakana_scene` APIs. Do not make gameplay-facing APIs depend on SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, RHI/backend/native handles, or editor UI implementation.

**Tech Stack:** C++23 `mirakana_scene`/`mirakana_tools`, focused C++ tests, `engine/agent/manifest.json`, schema/static checks, `tools/agent-context.ps1`, existing validation recipe runner, and docs.

---

## Goal

Make stable-id scene and prefab authoring AI-operable through reviewed typed operations:

- dry-run a scene/prefab authoring request and report planned file/model mutations
- apply only reviewed structured operations over `GameEngine.Scene.v2` and `GameEngine.Prefab.v2` documents
- support initial operations for create scene, add node, add/update component, create prefab, and instantiate prefab
- reject unsafe paths, duplicate authoring ids, malformed component rows, stale prefab references, unsupported free-form edits, and package/runtime migration claims
- return deterministic diagnostics, changed file rows, validation recipe recommendations, and placeholder undo metadata

## Context

- `Scene/Component/Prefab Schema v2` is implemented as a contract-only `mirakana_scene` data spine.
- Existing `update-scene-package` updates first-party `GameEngine.Scene.v1` cooked package files plus `.geindex` dependency rows; it does not expose stable-id Scene v2 or prefab authoring commands.
- AI command descriptors now exist, and `run-validation-recipe` can execute allowlisted validation recipes through a reviewed path.
- The next practical AI authoring gap is moving from package-row updates to structured scene/prefab model edits without making editor UI, runtime package migration, or renderer/RHI residency claims.

## Constraints

- Do not add third-party dependencies.
- Do not parse arbitrary shell or free-form edit commands.
- Do not make `mirakana_scene`, `mirakana_assets`, or `mirakana_ui` depend on renderer, RHI, platform, SDL3, Dear ImGui, or backend APIs.
- Do not migrate runtime package loading to Scene v2 in this slice unless a focused RED check proves it is required.
- Do not claim nested prefab propagation/conflict UX, play-in-editor isolation, source asset import, package streaming, renderer/RHI residency, material/shader graphs, live shader generation, editor productization, or Metal readiness.
- Preserve existing Scene v1 package apply tooling as a separate cooked-package surface.

## Done When

- Focused RED -> GREEN evidence is recorded here.
- A reviewed scene/prefab authoring dry-run/apply API exists for the initial operation allowlist.
- Manifest command descriptors expose only the validated authoring subset as ready.
- Static checks reject stale broad claims, arbitrary edit text, Scene v2 runtime/package migration claims, and editor productization claims.
- Docs tell agents when to use Scene v2 authoring commands versus Scene v1 package updates.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused C++ tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`, selected cheap runner execute test, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory And Contract

**Files:**
- Read: `engine/agent/manifest.json`
- Read: `engine/scene/include/mirakana/scene/schema_v2.hpp`
- Read: `engine/scene/src/schema_v2.cpp`
- Read: `engine/scene/tests/scene_schema_v2_tests.cpp`
- Read: `engine/tools/include/mirakana/tools/*`
- Read: `engine/tools/src/*`
- Read: `engine/tools/tests/*`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `tools/agent-context.ps1`
- Read: `docs/ai-game-development.md`
- Read: `docs/testing.md`

- [x] Decide the initial operation allowlist and request/result field names.
  - Allowlist: `create_scene`, `add_node`, `add_or_update_component`, `create_prefab`, `instantiate_prefab`.
  - C++ API: shared typed helper surface `ScenePrefabAuthoringRequest`, `ScenePrefabAuthoringResult`, `ScenePrefabAuthoringChangedFile`, `ScenePrefabAuthoringModelMutation`, `ScenePrefabAuthoringDiagnostic`, `plan_scene_prefab_authoring`, and `apply_scene_prefab_authoring`.
- [x] Define stable path and authoring-id validation rules.
  - Paths must be safe repository-relative authored paths: no absolute roots, drive letters, backslashes, empty segments, `.`, `..`, NUL, CR, or LF.
  - Scene authoring paths use `.scene`; prefab authoring paths use `.prefab`; Scene v1 cooked package update paths remain on `update-scene-package`.
  - Authoring ids use the canonical `mirakana_scene` Schema v2 validation and must stay non-empty, whitespace/control-free, unique, and reference existing parent/component nodes after each operation.
- [x] Define dry-run changed-file/model-mutation rows and apply diagnostics.
  - Dry-run returns planned `changed_files` with path/content/hash and `model_mutations` with operation kind and target path without filesystem mutation.
  - Apply re-runs the same planner from current `IFileSystem` contents and writes only planned changed files after diagnostics are empty.
- [x] Decide whether the first surface is one command id with typed operations or separate command ids.
  - Decision: keep the existing manifest command ids (`create-scene`, `add-scene-node`, `add-or-update-component`, `create-prefab`, `instantiate-prefab`) and promote them as separate reviewed ready descriptors, while implementing the C++ behavior through one shared typed helper. This avoids adding a duplicate AI command id over the already-declared command surfaces.
- [x] Record explicit non-goals.
  - Non-goals remain Scene v2 runtime/package migration, source asset import, broad package cooking, editor productization, nested prefab propagation/conflict UX, play-in-editor isolation, arbitrary shell execution, arbitrary free-form edits, renderer/RHI residency, package streaming, material/shader graphs, live shader generation, Metal readiness, general production renderer quality, and public SDL3/Win32/D3D12/Vulkan/Metal/Dear ImGui/RHI/native handle exposure.

### Task 2: RED Checks And Tests

**Files:**
- Modify: focused `mirakana_tools` or `mirakana_scene` tests
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: this plan

- [x] Add failing focused tests for dry-run create scene, add node, add/update component, create prefab, and instantiate prefab.
- [x] Add failing focused tests for apply changed files and deterministic serialization.
- [x] Add failing focused tests for duplicate ids, unsafe paths, unsupported component payloads, stale prefab refs, and unsupported free-form edits.
- [x] Add failing static checks requiring a reviewed Scene/Prefab v2 authoring command descriptor.
- [x] Add failing static checks rejecting docs/manifest claims that Scene v2 runtime package migration, editor productization, or nested prefab conflict UX are ready.
- [x] Record RED evidence before production implementation.

### Task 3: Production Implementation

**Files:**
- Modify or create: `mirakana_tools` scene/prefab authoring helpers
- Modify: focused tests
- Modify: CMake target registration if new test files are added

- [x] Implement typed request/result value APIs with deterministic diagnostics.
- [x] Implement dry-run planning without filesystem mutation.
- [x] Implement apply through validated file writes only.
- [x] Preserve Scene/Component/Prefab v2 canonical validation and serialization as the source of truth.
- [x] Keep Scene v1 cooked package updates on the existing `update-scene-package` surface.

### Task 4: Manifest, Docs, And Agent Context Sync

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/agent-context.ps1` if the top-level output needs a new field
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] Promote only the reviewed Scene/Prefab v2 authoring subset to ready.
- [x] Keep runtime package migration, source asset import, broad package cooking, editor productization, nested prefab conflict UX, and renderer/RHI residency planned.
- [x] Update generated-game guidance so agents use Scene v2 authoring commands for authoring data and Scene v1 package update tooling only for reviewed cooked package rows.
- [x] Keep `currentActivePlan` and `recommendedNextPlan` synchronized with the plan registry.

### Task 5: Validation

**Files:**
- Modify: this plan's Validation Evidence section
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/roadmap.md`

- [x] Run focused scene/prefab authoring tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`.
- [x] Run a cheap validation runner execute test, for example `agent-contract`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record evidence, mark this plan complete, and create the next dated focused plan.

## Validation Evidence

Record command results here while implementing this plan.

- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and confirmed `aiOperableProductionLoop.currentActivePlan` is `docs/superpowers/plans/2026-05-01-scene-prefab-authoring-command-tooling-v1.md`.
- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected after adding static checks: `engine/agent/manifest.json aiOperableProductionLoop must expose one ready Scene/Prefab v2 authoring command surface: create-scene`.
- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after adding matching JSON contract checks: `engine manifest aiOperableProductionLoop must expose one ready Scene/Prefab v2 authoring command surface: create-scene`.
- RED, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding focused authoring tests: `tests/unit/tools_tests.cpp(24,10): error C1083: include file 'mirakana/tools/scene_prefab_authoring_tool.hpp': No such file or directory`.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed, 28/28, including `mirakana_scene_schema_v2_tests` and `mirakana_tools_tests` coverage for Scene/Prefab v2 dry-run/apply, duplicate ids, unsafe paths, unsupported payloads, stale prefab refs, free-form edit rejection, sparse-index rejection, line-control rejection, finite/non-zero transforms, apply pre-read path validation, and prefab instance root transform application.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed with the active plan still pointing to this slice before registry rollover.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only with D3D12 DXIL, Vulkan SPIR-V, and DXC SPIR-V CodeGen ready; Metal `metal`/`metallib` remain missing host-gated diagnostics.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed and executed only the reviewed `agent-check` and `schema-check` command plan.
- GREEN, 2026-05-01: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, 28/28 CTest, with existing diagnostic-only host gates for Metal tools, Apple packaging/Xcode, Android release signing/device smoke, and tidy compile database availability.
