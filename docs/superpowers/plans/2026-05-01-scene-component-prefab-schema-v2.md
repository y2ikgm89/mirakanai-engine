# Scene Component Prefab Schema v2 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the first stable-id, schema-driven scene/component/prefab authoring contract that future AI commands, 2D/3D vertical slices, and editor-core productization can share.

**Architecture:** This slice adds additive v2 authoring value types, validation, deterministic text serialization, and stable prefab override paths inside `MK_scene`. It does not migrate runtime package loading, editor GUI, renderer submission, or existing v1 scene/prefab samples yet; those follow after the v2 contract is validated.

**Tech Stack:** C++23, `MK_scene`, `MK_assets` asset ids, `MK_math` transforms, repository test framework, CMake target registration, Markdown docs, `engine/agent/manifest.json`, and existing `tools/*.ps1` validation commands.

---

## Goal

Create a production data-spine foundation for future AI editing:

- stable authoring ids for scene nodes and components
- schema-driven component rows using first-party component type ids
- deterministic validation diagnostics instead of throwing for authoring mistakes
- deterministic text serialization designed for AI-friendly diffs
- stable prefab override paths that do not depend on node array indices

## Context

- The active AI-operable production loop contract marks `scene-component-prefab-schema-v2` as the first recommended next plan.
- Existing `MK_scene` has v1 `Scene`, fixed optional `SceneNodeComponents`, `GameEngine.Scene.v1` text IO, `GameEngine.Prefab.v1`, and index-based prefab variant overrides.
- Existing editor-core authoring models and desktop runtime package scaffolds rely on the v1 flow and should not be migrated in this slice without a separate plan.
- Future 2D/3D vertical slices need stable ids and schema-driven component rows before AI can safely edit scene, prefab, and component data.

## Constraints

- Keep `engine/scene` independent from renderer, RHI, platform, editor, SDL3, Dear ImGui, and native handles.
- Do not add third-party dependencies.
- Keep gameplay-facing APIs in the `mirakana::` namespace and avoid backend/native handles.
- Do not parse external source asset formats in runtime/game code.
- Do not claim the planned 2D/3D playable vertical slices are ready after this slice.
- Update manifest/docs/checks with any new public capability or unsupported gap changes.

## Done When

- `MK_scene` exposes stable authoring id, component schema, scene document, prefab document, and prefab override path v2 value APIs.
- Focused tests prove validation, deterministic serialization, duplicate-id rejection, stable override path composition, and round trips.
- Existing v1 scene/prefab tests still pass.
- `engine/agent/manifest.json`, generated-game docs, and AI checks mark Scene/Component/Prefab Schema v2 as implemented-contract-only, not a 2D/3D/editor migration.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## File Structure

- Create `engine/scene/include/mirakana/scene/schema_v2.hpp`: public stable-id scene/component/prefab v2 value types and function declarations.
- Create `engine/scene/src/schema_v2.cpp`: validation, deterministic serialization, deserialization, and prefab override composition.
- Modify `engine/scene/CMakeLists.txt`: add `src/schema_v2.cpp`.
- Create `tests/unit/scene_schema_v2_tests.cpp`: focused tests for the v2 contract.
- Modify `CMakeLists.txt`: register `MK_scene_schema_v2_tests`.
- Modify `engine/agent/manifest.json`: update `MK_scene` purpose/status and `aiOperableProductionLoop.unsupportedProductionGaps`.
- Modify `schemas/engine-agent.schema.json`, `tools/check-ai-integration.ps1`, and `tools/check-json-contracts.ps1` if manifest shape or ready/planned assertions change.
- Modify `docs/architecture.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/specs/generated-game-validation-scenarios.md`, `docs/specs/game-prompt-pack.md`, and this plan with evidence.

## Implementation Tasks

### Task 1: RED Tests For Stable Authoring Ids And Component Rows

**Files:**
- Create: `tests/unit/scene_schema_v2_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add failing tests for ids and component validation**

Add tests that include `mirakana/scene/schema_v2.hpp` and expect these behaviors:

```cpp
MK_TEST("scene schema v2 rejects duplicate node and component authoring ids") {
    mirakana::SceneDocumentV2 scene;
    scene.name = "Level";
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{mirakana::AuthoringId{"node/player"}, "Player"});
    scene.nodes.push_back(mirakana::SceneNodeDocumentV2{mirakana::AuthoringId{"node/player"}, "Duplicate"});
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        mirakana::AuthoringId{"component/player/transform"},
        mirakana::AuthoringId{"node/player"},
        mirakana::SceneComponentTypeId{"transform3d"},
        {{"position", "0 0 0"}}
    });
    scene.components.push_back(mirakana::SceneComponentDocumentV2{
        mirakana::AuthoringId{"component/player/transform"},
        mirakana::AuthoringId{"node/player"},
        mirakana::SceneComponentTypeId{"mesh_renderer"},
        {{"mesh", "assets/meshes/player"}}
    });

    const auto diagnostics = mirakana::validate_scene_document_v2(scene);

    MK_REQUIRE(diagnostics.size() == 2);
    MK_REQUIRE(diagnostics[0].code == mirakana::SceneSchemaV2DiagnosticCode::duplicate_node_id);
    MK_REQUIRE(diagnostics[1].code == mirakana::SceneSchemaV2DiagnosticCode::duplicate_component_id);
}
```

- [x] **Step 2: Register the focused test target**

Add `MK_scene_schema_v2_tests` in the root `CMakeLists.txt`, link it to `MK_scene`, include `tests`, and register it with `add_test`.

- [x] **Step 3: Verify RED**

Run:

```powershell
cmake --build --preset dev --target MK_scene_schema_v2_tests
```

Expected: compile failure because `mirakana/scene/schema_v2.hpp` and the v2 types do not exist yet.

### Task 2: Minimal V2 Types And Validation

**Files:**
- Create: `engine/scene/include/mirakana/scene/schema_v2.hpp`
- Create: `engine/scene/src/schema_v2.cpp`
- Modify: `engine/scene/CMakeLists.txt`

- [x] **Step 1: Add public value types**

Define dependency-light types:

```cpp
namespace mirakana {

struct AuthoringId {
    std::string value;
};

struct SceneComponentTypeId {
    std::string value;
};

struct SceneComponentPropertyV2 {
    std::string name;
    std::string value;
};

struct SceneNodeDocumentV2 {
    AuthoringId id;
    std::string name;
    Transform3D transform;
    AuthoringId parent;
};

struct SceneComponentDocumentV2 {
    AuthoringId id;
    AuthoringId node;
    SceneComponentTypeId type;
    std::vector<SceneComponentPropertyV2> properties;
};

struct SceneDocumentV2 {
    std::string name;
    std::vector<SceneNodeDocumentV2> nodes;
    std::vector<SceneComponentDocumentV2> components;
};

enum class SceneSchemaV2DiagnosticCode {
    invalid_scene_name,
    invalid_authoring_id,
    duplicate_node_id,
    duplicate_component_id,
    missing_parent_node,
    missing_component_node,
    invalid_component_type,
    duplicate_component_property,
    invalid_component_property
};

struct SceneSchemaV2Diagnostic {
    SceneSchemaV2DiagnosticCode code{SceneSchemaV2DiagnosticCode::invalid_scene_name};
    AuthoringId node;
    AuthoringId component;
    SceneComponentTypeId component_type;
    std::string property;
};

[[nodiscard]] std::vector<SceneSchemaV2Diagnostic> validate_scene_document_v2(const SceneDocumentV2& scene);

} // namespace mirakana
```

- [x] **Step 2: Implement minimal validation**

Validation rules:

- scene name must be non-empty
- authoring ids must be non-empty, contain no ASCII control characters, and not contain whitespace
- component type id must be one of `transform3d`, `camera`, `light`, `mesh_renderer`, `sprite_renderer`, `tilemap`, `audio_cue`, `rigid_body2d`, `collider2d`, `nav_agent`, `animation`, or `ui_attachment`
- node ids and component ids must be unique
- non-empty parent ids must reference an existing node id
- component node ids must reference an existing node id
- component property names must be non-empty and unique per component

- [x] **Step 3: Verify GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_scene_schema_v2_tests
ctest --preset dev --output-on-failure -R MK_scene_schema_v2_tests
```

Expected: `MK_scene_schema_v2_tests` builds and the duplicate-id test passes.

### Task 3: Deterministic Serialization And Round Trip

**Files:**
- Modify: `engine/scene/include/mirakana/scene/schema_v2.hpp`
- Modify: `engine/scene/src/schema_v2.cpp`
- Modify: `tests/unit/scene_schema_v2_tests.cpp`

- [x] **Step 1: Add failing round-trip tests**

Add tests for exact text output:

```cpp
const std::string expected =
    "format=GameEngine.Scene.v2\n"
    "scene.name=Level\n"
    "node.0.id=node/player\n"
    "node.0.name=Player\n"
    "node.0.parent=\n"
    "node.0.position=1 2 3\n"
    "node.0.rotation=0 0.25 0\n"
    "node.0.scale=1 1 1\n"
    "component.0.id=component/player/mesh\n"
    "component.0.node=node/player\n"
    "component.0.type=mesh_renderer\n"
    "component.0.property.0.name=mesh\n"
    "component.0.property.0.value=assets/meshes/player\n";

MK_REQUIRE(mirakana::serialize_scene_document_v2(scene) == expected);
MK_REQUIRE(mirakana::deserialize_scene_document_v2(expected).nodes[0].id.value == "node/player");
```

- [x] **Step 2: Add serializer/deserializer declarations**

Add:

```cpp
[[nodiscard]] std::string serialize_scene_document_v2(const SceneDocumentV2& scene);
[[nodiscard]] SceneDocumentV2 deserialize_scene_document_v2(std::string_view text);
```

- [x] **Step 3: Implement stable text IO**

Rules:

- Emit nodes in vector order and components in vector order.
- Emit component properties sorted by property name during serialization.
- Reject unsupported `format` values with `std::invalid_argument`.
- Reuse validation before serialization and after deserialization.
- Preserve authoring ids verbatim.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_scene_schema_v2_tests
ctest --preset dev --output-on-failure -R MK_scene_schema_v2_tests
```

Expected: serialization and duplicate-id tests pass.

### Task 4: Stable Prefab V2 Documents And Override Paths

**Files:**
- Modify: `engine/scene/include/mirakana/scene/schema_v2.hpp`
- Modify: `engine/scene/src/schema_v2.cpp`
- Modify: `tests/unit/scene_schema_v2_tests.cpp`

- [x] **Step 1: Add failing prefab override tests**

Test that a variant override targets a stable path such as:

```text
nodes/node/player/components/component/player/mesh/properties/material
```

Expected behavior:

- overriding a property by stable component id changes the composed prefab
- missing target ids produce `missing_override_target`
- duplicate override paths produce `duplicate_override_path`
- node order changes in the base prefab do not change override targeting

- [x] **Step 2: Add prefab v2 types**

Define:

```cpp
struct PrefabDocumentV2 {
    std::string name;
    SceneDocumentV2 scene;
};

struct PrefabOverridePathV2 {
    std::string value;
};

struct PrefabOverrideV2 {
    PrefabOverridePathV2 path;
    std::string value;
};

struct PrefabVariantDocumentV2 {
    std::string name;
    PrefabDocumentV2 base_prefab;
    std::vector<PrefabOverrideV2> overrides;
};

struct PrefabVariantComposeResultV2 {
    bool success{false};
    PrefabDocumentV2 prefab;
    std::vector<SceneSchemaV2Diagnostic> diagnostics;
};

[[nodiscard]] PrefabVariantComposeResultV2 compose_prefab_variant_v2(const PrefabVariantDocumentV2& variant);
```

- [x] **Step 3: Implement stable path composition**

Support these initial paths:

- `nodes/<node-id>/name`
- `nodes/<node-id>/parent`
- `nodes/<node-id>/components/<component-id>/properties/<property-name>`

Keep transform overrides out of this initial path set unless tests explicitly define the text encoding for each transform field.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_scene_schema_v2_tests
ctest --preset dev --output-on-failure -R MK_scene_schema_v2_tests
```

Expected: stable override path tests pass and v1 prefab tests remain unaffected.

### Task 5: Manifest, Docs, And AI Checks

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] **Step 1: Update manifest truthfully**

Set the `MK_scene` module status to an implemented schema-v2 contract status and update its purpose to mention stable authoring ids, schema-driven component rows, deterministic validation, v2 serialization, and stable prefab override paths.

- [x] **Step 2: Keep future recipes planned**

Keep `future-2d-playable-vertical-slice` and `future-3d-playable-vertical-slice` as `planned`. This slice does not make them ready.

- [x] **Step 3: Strengthen checks**

Make checks fail if manifest/docs claim:

- Scene v2 migrated editor productization
- 2D playable vertical slice is ready
- 3D playable vertical slice is ready
- prefab variants no longer need nested prefab/conflict follow-up work

- [x] **Step 4: Update docs**

Document the v2 contract as an authoring data contract only. Keep runtime game code on cooked packages and public `mirakana::` APIs.

### Task 6: Validation And Closure

**Files:**
- Modify: `docs/superpowers/plans/2026-05-01-scene-component-prefab-schema-v2.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] **Step 1: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_scene_schema_v2_tests
ctest --preset dev --output-on-failure -R MK_scene_schema_v2_tests
```

- [x] **Step 2: Run contract validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

- [x] **Step 3: Run completion validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 4: Record evidence**

Append command results to this plan's Validation Evidence section and move the plan from active to completed in `docs/superpowers/plans/README.md`.

## Validation Evidence

- RED: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_scene_schema_v2_tests` failed because `mirakana/scene/schema_v2.hpp` did not exist.
- GREEN validation: focused `MK_scene_schema_v2_tests` build and `ctest --preset dev --output-on-failure -R MK_scene_schema_v2_tests` passed after adding v2 validation, deterministic text IO, and stable prefab override composition.
- Serialization RED: focused build failed because `mirakana::serialize_scene_document_v2` and `mirakana::deserialize_scene_document_v2` were missing.
- Prefab RED: focused build failed because `PrefabDocumentV2`, `PrefabVariantDocumentV2`, `PrefabOverrideV2`, and `compose_prefab_variant_v2` were missing.
- Contract checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` initially failed on missing explicit runtime-package-migration wording in the Scene v2 unsupported gap, then passed after tightening manifest notes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and emitted the updated `productionLoop`, `productionRecipes`, `aiCommandSurfaces`, `unsupportedProductionGaps`, `hostGates`, and `recommendedNextPlan` data.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after adding the public `mirakana/scene/schema_v2.hpp` API.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 23/23 tests, including `MK_scene_schema_v2_tests`. Diagnostic-only gates remained Metal tools missing, Apple packaging blocked on non-macOS/Xcode, Android release signing/device smoke not fully configured, and tidy compile database availability.
