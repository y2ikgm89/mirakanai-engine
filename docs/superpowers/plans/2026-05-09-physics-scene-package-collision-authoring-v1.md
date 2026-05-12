# Physics Scene Package Collision Authoring Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote first-party 3D collision authoring from in-memory rows into reviewed scene/package data that generated desktop packages can validate, load, and convert into `mirakana::PhysicsWorld3D`.

**Architecture:** Add a first-party cooked collision-scene package payload beside the existing package index and runtime payload contracts, then bridge selected package rows into `PhysicsAuthoredCollisionScene3DDesc` without exposing middleware or native handles. Keep the generated-game proof narrow: one selected package collision smoke with deterministic counters, not broad physics production readiness.

**Tech Stack:** C++23, `MK_assets`, `MK_runtime`, `MK_physics`, `MK_tools`, `DesktopRuntime3DPackage`, CMake desktop-runtime metadata, PowerShell validators/static checks.

---

**Plan ID:** `physics-scene-package-collision-authoring-v1`  
**Status:** Completed.  
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Previous Slice:** [2026-05-09-generated-3d-visible-production-game-proof-v1.md](2026-05-09-generated-3d-visible-production-game-proof-v1.md)

## Context

- The master plan's Physics 1.0 section selects `physics-scene-package-collision-authoring-v1` as the first recommended collision-system production slice.
- Current physics already has `PhysicsAuthoredCollisionScene3DDesc`, `PhysicsAuthoredCollisionBody3DDesc`, `PhysicsAuthoredCollisionScene3DBuildResult`, and `build_physics_world_3d_from_authored_collision_scene`, but generated packages still create collision rows in executable code.
- `sample_generated_desktop_runtime_3d_package` already proves visible D3D12 generated package execution through `--require-visible-3d-production-proof`. The next narrow production gap is making collision world data package-visible and fail-closed.
- This slice intentionally uses first-party deterministic text package payloads. It does not add Jolt, native backend handles, exact shape sweeps, CCD, joints, editor physics debugging UI, or dynamic controller push policy.

## Constraints

- Keep `engine/core` and `engine/physics` independent from package files, scene formats, OS APIs, GPU APIs, and editor code.
- Put runtime package decoding in `engine/runtime`; put authoring/package update helpers in `engine/tools`; keep physics world construction in `engine/physics`.
- Add a new first-party `AssetKind` only if the package format needs a distinct kind. Update all package/identity/source-kind string conversions in the same slice.
- Use deterministic line-oriented payloads compatible with existing `GameEngine.*.v1` package documents. Do not parse free-form JSON inside the runtime payload.
- Keep collision rows primitive-first: AABB, sphere, capsule, dynamic/static, trigger flag, layer, mask, position, velocity, mass, linear damping, half extents, radius, half height, and optional material name.
- Compound colliders in this slice are authored as deterministic named groups of primitive rows with shared metadata. They are flattened into `PhysicsBody3DDesc` rows for the current first-party physics world.
- Do not mark `physics-1-0-runtime-collision-system`, `3d-playable-vertical-slice`, or broad generated 3D production readiness as ready from this slice alone.

## Done When

- Runtime can parse `GameEngine.PhysicsCollisionScene3D.v1` package records into a typed payload and convert that payload into `PhysicsAuthoredCollisionScene3DDesc`.
- The package decoder rejects wrong asset kinds, malformed payloads, invalid/duplicate body names, invalid shape data, invalid layer/mask rows, and unsupported native-backend requests with deterministic diagnostics.
- `sample_generated_desktop_runtime_3d_package` ships a cooked collision payload row in its `.geindex` and supports `--require-scene-collision-package`.
- The selected installed package smoke validates exact counters:
  - `collision_package_status=ready`
  - `collision_package_ready=1`
  - `collision_package_diagnostics=0`
  - `collision_package_bodies=3`
  - `collision_package_triggers=1`
  - `collision_package_contacts>=1`
  - `collision_package_trigger_overlaps>=1`
  - `collision_package_world_ready=1`
  - `gameplay_systems_collision_package_ready=1`
- `tools/validate-installed-desktop-runtime.ps1` enforces those fields when smoke args include `--require-scene-collision-package`.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` propagates the collision package payload, manifest recipe, README text, generated static markers, and validation expectations.
- `engine/agent/manifest.json`, docs, plan registry, Codex/Claude skills, and static checks describe this as scene/package collision authoring, not broad physics readiness.
- Focused build/test commands, selected installed package smoke, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `git diff --check`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/tool blocker.

## File Structure

- Modify `engine/assets/include/mirakana/assets/asset_registry.hpp`: add `AssetKind::physics_collision_scene` when implementing the distinct package kind.
- Modify `engine/assets/src/asset_package.cpp`, `engine/assets/src/asset_identity.cpp`, and `engine/assets/src/source_asset_registry.cpp`: serialize/parse/display the new kind.
- Modify `engine/runtime/include/mirakana/runtime/asset_runtime.hpp` and `engine/runtime/src/asset_runtime.cpp`: add `RuntimePhysicsCollisionScene3DPayload` and `runtime_physics_collision_scene_3d_payload`.
- Modify `engine/runtime/include/mirakana/runtime/physics_collision_runtime.hpp` and `engine/runtime/src/physics_collision_runtime.cpp`: add conversion from typed runtime payload to `PhysicsAuthoredCollisionScene3DDesc` and `PhysicsAuthoredCollisionScene3DBuildResult`.
- Modify `engine/runtime/CMakeLists.txt`: register the new runtime source/header if a separate runtime file is added.
- Modify `engine/tools/include/mirakana/tools/physics_collision_package_tool.hpp` and `engine/tools/src/physics_collision_package_tool.cpp`: add deterministic authoring and package update helpers for collision payload rows.
- Modify `engine/tools/CMakeLists.txt`: register the new tools source/header if separate files are added.
- Modify `tests/unit/core_tests.cpp`, `tests/unit/runtime_tests.cpp`, and `tests/unit/tools_tests.cpp`: add RED/GREEN coverage for physics build, runtime payload decoding, and package authoring.
- Modify `games/sample_generated_desktop_runtime_3d_package/main.cpp`, `game.agent.json`, `README.md`, runtime payload files, and `.geindex`: add the selected generated-package proof.
- Modify `games/CMakeLists.txt` and `tools/new-game.ps1`: propagate default package smoke args and generated template output.
- Modify `tools/validate-installed-desktop-runtime.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-ai-integration.ps1`: add fail-closed validation and static drift guards.
- Modify `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, and the master plan: document exact claims and remaining unsupported physics work.
- Modify `.agents/skills/gameengine-game-development/SKILL.md` and `.claude/skills/gameengine-game-development/SKILL.md`: teach agents to use package collision rows when the selected generated 3D package smoke is required.

## 2026-05-09 Replan After Master Update

The updated master plan keeps this child as the active Physics 1.0 entry point. Continue this slice, but execute it with the corrected proof shape below.

**Completed foundation in this session:**
- RED was observed for the first runtime payload test: `cmake --build --preset dev --target MK_runtime_tests --config Debug` failed on missing `AssetKind::physics_collision_scene`, `runtime_physics_collision_scene_3d_payload`, and public physics types.
- `AssetKind::physics_collision_scene` now round-trips through cooked package indexes, Asset Identity v2 text, and Source Asset Registry unvalidated text parsing while remaining unsupported as a source-import kind.
- `runtime_physics_collision_scene_3d_payload` decodes `GameEngine.PhysicsCollisionScene3D.v1` body rows, rejects duplicate names and invalid body rows, and participates in runtime diagnostics.
- Focused verification is green: `MK_runtime_tests`, `MK_core_tests`, `MK_asset_identity_runtime_resource_tests`, and `MK_tools_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `git diff --check`.
- Runtime payload hardening now requires `backend.native=unsupported` and rejects native backend requests. RED: `ctest --preset dev -R MK_runtime_tests --output-on-failure` failed on `runtime physics collision scene payload rejects native backend requests`. GREEN: `cmake --build --preset dev --target MK_runtime_tests --config Debug` and `ctest --preset dev -R MK_runtime_tests --output-on-failure` passed.
- Runtime bridge is implemented through `engine/runtime/include/mirakana/runtime/physics_collision_runtime.hpp` and `engine/runtime/src/physics_collision_runtime.cpp` as `build_physics_world_3d_from_runtime_collision_scene`. RED: `cmake --build --preset dev --target MK_runtime_tests --config Debug` failed on missing `mirakana/runtime/physics_collision_runtime.hpp`. GREEN: `MK_runtime_tests` passed with 3 bodies, non-trigger contact evidence, and one trigger-overlap row.

**Plan corrections:**
- Use a 3-body generated package proof: static floor, non-trigger collision probe, and trigger pickup. `PhysicsWorld3D::contacts()` intentionally excludes trigger pairs, so a 2-body proof with one trigger cannot honestly produce `collision_package_contacts>=1`.
- Add `collision_package_trigger_overlaps>=1` as a distinct smoke field. Do not count trigger overlaps as contacts.
- Add an explicit package payload backend claim row, `backend.native=unsupported`, and reject any native/backend-required value in the runtime decoder and package authoring tool. This satisfies the unsupported native-backend request gate without adding middleware or native handles.
- Keep future work ordered after this slice as the master plan states: broader exact casts/sweeps, contact manifold stability, CCD, character/dynamic policy, joints, benchmark/determinism gates, then optional Jolt gate.

**Replanned execution order from here:**
1. Add deterministic `MK_tools` package authoring/update helpers and package-index verification for collision payload rows.
2. Wire `sample_generated_desktop_runtime_3d_package`, package files, `.geindex`, resident kind hints, installed validator, `games/CMakeLists.txt`, and `tools/new-game.ps1`.
3. Sync manifest/docs/skills/static checks, then run the selected package smoke and full repository validation.

**Subagent use:**
- Use read-only/explorer subagents for sample/template/validator and docs/static drift scans.
- Keep write work local or assign disjoint write scopes only after the runtime/tool API is stable, because `main.cpp`, `new-game.ps1`, and manifest/docs updates are conflict-prone.

## Task 1: RED Runtime Collision Payload Contract

**Files:**
- Modify: `tests/unit/runtime_tests.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Modify: `engine/runtime/src/asset_runtime.cpp`

- [x] **Step 1: Add the failing runtime payload test**

Add this test near the existing runtime typed payload tests:

```cpp
MK_TEST("runtime physics collision scene payload decodes deterministic body rows") {
    const mirakana::AssetId collision_scene = mirakana::AssetId::from_name("physics/collision/main");
    const std::string payload =
        "format=GameEngine.PhysicsCollisionScene3D.v1\n"
        "asset.id=" + std::to_string(collision_scene.value) + "\n"
        "asset.kind=physics_collision_scene\n"
        "world.gravity=0,-9.80665,0\n"
        "body.count=2\n"
        "body.0.name=floor\n"
        "body.0.shape=aabb\n"
        "body.0.position=0,-0.5,0\n"
        "body.0.velocity=0,0,0\n"
        "body.0.dynamic=false\n"
        "body.0.mass=0\n"
        "body.0.linear_damping=0\n"
        "body.0.half_extents=5,0.5,5\n"
        "body.0.radius=0.5\n"
        "body.0.half_height=0.5\n"
        "body.0.layer=1\n"
        "body.0.mask=4294967295\n"
        "body.0.trigger=false\n"
        "body.0.material=stone\n"
        "body.1.name=pickup_trigger\n"
        "body.1.shape=sphere\n"
        "body.1.position=0,0.75,0\n"
        "body.1.velocity=0,0,0\n"
        "body.1.dynamic=false\n"
        "body.1.mass=0\n"
        "body.1.linear_damping=0\n"
        "body.1.half_extents=0.5,0.5,0.5\n"
        "body.1.radius=0.75\n"
        "body.1.half_height=0.5\n"
        "body.1.layer=2\n"
        "body.1.mask=1\n"
        "body.1.trigger=true\n"
        "body.1.material=trigger\n";

    const mirakana::runtime::RuntimeAssetRecord record{
        mirakana::runtime::RuntimeAssetHandle{1},
        collision_scene,
        mirakana::AssetKind::physics_collision_scene,
        "runtime/assets/physics/main.collision3d",
        mirakana::hash_asset_cooked_content(payload),
        7,
        {},
        payload,
    };

    const auto result = mirakana::runtime::runtime_physics_collision_scene_3d_payload(record);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.payload.asset == collision_scene);
    MK_REQUIRE(result.payload.bodies.size() == 2);
    MK_REQUIRE(result.payload.bodies[0].name == "floor");
    MK_REQUIRE(result.payload.bodies[0].material == "stone");
    MK_REQUIRE(result.payload.bodies[0].body.shape == mirakana::PhysicsShape3DKind::aabb);
    MK_REQUIRE(!result.payload.bodies[0].body.dynamic);
    MK_REQUIRE(result.payload.bodies[1].name == "pickup_trigger");
    MK_REQUIRE(result.payload.bodies[1].body.shape == mirakana::PhysicsShape3DKind::sphere);
    MK_REQUIRE(result.payload.bodies[1].body.trigger);
    MK_REQUIRE(result.payload.bodies[1].body.collision_layer == 2U);
    MK_REQUIRE(result.payload.bodies[1].body.collision_mask == 1U);
}
```

- [x] **Step 2: Run the focused runtime target and confirm RED**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_tests --config Debug
```

Expected: FAIL to compile on missing `AssetKind::physics_collision_scene`, `RuntimePhysicsCollisionScene3DPayload`, or `runtime_physics_collision_scene_3d_payload`.

- [x] **Step 3: Add the runtime payload API**

Add these declarations:

```cpp
struct RuntimePhysicsCollisionBody3DPayload {
    std::string name;
    PhysicsBody3DDesc body;
    std::string material;
};

struct RuntimePhysicsCollisionScene3DPayload {
    AssetId asset;
    RuntimeAssetHandle handle;
    PhysicsWorld3DConfig world_config{};
    std::vector<RuntimePhysicsCollisionBody3DPayload> bodies;
};

[[nodiscard]] RuntimePayloadAccessResult<RuntimePhysicsCollisionScene3DPayload>
runtime_physics_collision_scene_3d_payload(const RuntimeAssetRecord& record);
```

Include `mirakana/physics/physics3d.hpp` from `asset_runtime.hpp` or move the payload declarations into `physics_collision_runtime.hpp` if that keeps include boundaries cleaner.

- [x] **Step 4: Implement the parser**

Implement line-oriented parsing in `asset_runtime.cpp` with these accepted field names and exact shape values:

```cpp
format=GameEngine.PhysicsCollisionScene3D.v1
asset.id=<uint64>
asset.kind=physics_collision_scene
world.gravity=<float>,<float>,<float>
body.count=<uint32>
body.N.name=<non-empty token>
body.N.shape=aabb|sphere|capsule
body.N.position=<float>,<float>,<float>
body.N.velocity=<float>,<float>,<float>
body.N.dynamic=true|false
body.N.mass=<float>
body.N.linear_damping=<float>
body.N.half_extents=<float>,<float>,<float>
body.N.radius=<float>
body.N.half_height=<float>
body.N.layer=<uint32>
body.N.mask=<uint32>
body.N.trigger=true|false
body.N.material=<token-or-empty>
```

Reject invalid rows by returning `payload_failure<RuntimePhysicsCollisionScene3DPayload>(...)`; do not throw through the public function.

- [x] **Step 5: Run the focused runtime target and confirm GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_tests --config Debug
ctest --preset dev -R MK_runtime_tests --output-on-failure
```

Expected: PASS with the new payload decode test included.

- [x] **Step 6: Harden native backend claim rows**

Added `backend.native=unsupported` to the deterministic runtime payload fixture, added RED coverage for `backend.native=required`, then implemented fail-closed runtime parsing.

Run:

```powershell
cmake --build --preset dev --target MK_runtime_tests --config Debug
ctest --preset dev -R MK_runtime_tests --output-on-failure
```

Result: PASS after the decoder rejected native backend requests with a deterministic diagnostic containing `native`.

## Task 2: Asset Kind And Package Index Support

**Files:**
- Modify: `engine/assets/include/mirakana/assets/asset_registry.hpp`
- Modify: `engine/assets/src/asset_package.cpp`
- Modify: `engine/assets/src/asset_identity.cpp`
- Modify: `engine/assets/src/source_asset_registry.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/asset_identity_runtime_resource_tests.cpp`

- [x] **Step 1: Add failing round-trip tests**

Add assertions that a package index row with `kind=physics_collision_scene` round-trips through `serialize_asset_cooked_package_index` and `deserialize_asset_cooked_package_index`, and that asset identity/source registry string conversion recognizes the same kind.

Use this package row in the test:

```cpp
mirakana::AssetCookedArtifact{
    mirakana::AssetId::from_name("physics/collision/main"),
    mirakana::AssetKind::physics_collision_scene,
    "runtime/assets/physics/main.collision3d",
    "format=GameEngine.PhysicsCollisionScene3D.v1\nasset.id=1\nasset.kind=physics_collision_scene\n",
    1,
    {},
}
```

- [x] **Step 2: Run the focused tests and confirm RED**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests MK_asset_identity_runtime_resource_tests --config Debug
```

Expected: FAIL to compile or fail parsing because `physics_collision_scene` is not a known `AssetKind`.

- [x] **Step 3: Add `AssetKind::physics_collision_scene` and string mappings**

Update every local switch that already handles `AssetKind::tilemap` to include:

```cpp
case AssetKind::physics_collision_scene:
    return "physics_collision_scene";
```

Update every parser that maps strings to `AssetKind` to accept exactly `"physics_collision_scene"`.

- [x] **Step 4: Run the focused tests and confirm GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests MK_asset_identity_runtime_resource_tests --config Debug
ctest --preset dev -R "MK_core_tests|MK_asset_identity_runtime_resource_tests" --output-on-failure
```

Expected: PASS with the new package/identity kind tests included.

## Task 3: Runtime Physics Bridge

**Files:**
- Create or modify: `engine/runtime/include/mirakana/runtime/physics_collision_runtime.hpp`
- Create or modify: `engine/runtime/src/physics_collision_runtime.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Modify: `tests/unit/runtime_tests.cpp`

- [x] **Step 1: Add failing bridge tests**

Add a test that builds a `RuntimePhysicsCollisionScene3DPayload`, calls `build_physics_world_3d_from_runtime_collision_scene`, and verifies:

```cpp
MK_REQUIRE(result.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::success);
MK_REQUIRE(result.diagnostic == mirakana::PhysicsAuthoredCollision3DDiagnostic::none);
MK_REQUIRE(result.bodies.size() == 3);
MK_REQUIRE(result.world.bodies().size() == 3);
MK_REQUIRE(!result.world.contacts().empty());
MK_REQUIRE(result.world.trigger_overlaps().size() == 1);
```

Add a second test with duplicate `name` rows and verify:

```cpp
MK_REQUIRE(result.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::duplicate_name);
MK_REQUIRE(result.diagnostic == mirakana::PhysicsAuthoredCollision3DDiagnostic::duplicate_body_name);
```

- [x] **Step 2: Run the focused runtime target and confirm RED**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_tests --config Debug
```

Expected: FAIL to compile on missing `build_physics_world_3d_from_runtime_collision_scene`.

- [x] **Step 3: Implement the bridge**

Expose this function:

```cpp
[[nodiscard]] PhysicsAuthoredCollisionScene3DBuildResult
build_physics_world_3d_from_runtime_collision_scene(const RuntimePhysicsCollisionScene3DPayload& payload);
```

Implementation shape:

```cpp
PhysicsAuthoredCollisionScene3DDesc desc;
desc.world_config = payload.world_config;
desc.bodies.reserve(payload.bodies.size());
for (const auto& row : payload.bodies) {
    desc.bodies.push_back(PhysicsAuthoredCollisionBody3DDesc{row.name, row.body});
}
return build_physics_world_3d_from_authored_collision_scene(desc);
```

- [x] **Step 4: Run the focused runtime target and confirm GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_runtime_tests --config Debug
ctest --preset dev -R MK_runtime_tests --output-on-failure
```

Expected: PASS with decode and bridge tests.

## Task 4: Authoring And Package Update Tool

**Files:**
- Create or modify: `engine/tools/include/mirakana/tools/physics_collision_package_tool.hpp`
- Create or modify: `engine/tools/src/physics_collision_package_tool.cpp`
- Modify: `engine/tools/CMakeLists.txt`
- Modify: `tests/unit/tools_tests.cpp`

- [x] **Step 1: Add failing authoring tests**

Add tests for a package update helper that takes collision bodies and emits:

```cpp
PhysicsCollisionPackageUpdateDesc{
    .package_index_path = "runtime/sample_generated_desktop_runtime_3d_package.geindex",
    .collision_asset = mirakana::AssetId::from_name("sample_generated_desktop_runtime_3d_package.physics.collision"),
    .collision_path = "runtime/assets/3d/collision.scene3d",
    .source_revision = 1,
    .world_config = mirakana::PhysicsWorld3DConfig{},
    .bodies = {
        PhysicsCollisionPackageBodyDesc{"floor", floor_desc, "stone"},
        PhysicsCollisionPackageBodyDesc{"collision_probe", probe_desc, "metal"},
        PhysicsCollisionPackageBodyDesc{"pickup_trigger", trigger_desc, "trigger"},
    },
}
```

Verify `plan_physics_collision_package_update` emits one changed collision payload and one changed `.geindex` payload, both with stable content hashes. Verify duplicate names produce a diagnostic code `duplicate_collision_body_name`.

- [x] **Step 2: Run the focused tools target and confirm RED**

Run:

```powershell
cmake --build --preset dev --target MK_tools_tests --config Debug
```

Expected: FAIL to compile on missing `PhysicsCollisionPackageUpdateDesc` and `plan_physics_collision_package_update`.

- [x] **Step 3: Implement the package authoring helper**

Expose:

```cpp
struct PhysicsCollisionPackageBodyDesc {
    std::string name;
    PhysicsBody3DDesc body;
    std::string material;
};

struct PhysicsCollisionPackageChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct PhysicsCollisionPackageDiagnostic {
    std::string code;
    std::string message;
    std::string path;
    std::size_t body_index{0};
};

struct PhysicsCollisionPackageUpdateDesc {
    std::string package_index_path;
    AssetId collision_asset;
    std::string collision_path;
    std::uint64_t source_revision{1};
    PhysicsWorld3DConfig world_config{};
    std::vector<PhysicsCollisionPackageBodyDesc> bodies;
};

struct PhysicsCollisionPackageUpdateResult {
    std::vector<PhysicsCollisionPackageChangedFile> changed_files;
    std::vector<PhysicsCollisionPackageDiagnostic> diagnostics;
    [[nodiscard]] bool succeeded() const noexcept { return diagnostics.empty(); }
};

[[nodiscard]] PhysicsCollisionPackageUpdateResult
plan_physics_collision_package_update(const PhysicsCollisionPackageUpdateDesc& desc);
```

Do not write files in the plan function. Add `apply_physics_collision_package_update(IFileSystem&, const PhysicsCollisionPackageUpdateDesc&)` only after the plan function is covered.

- [x] **Step 4: Run the focused tools target and confirm GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_tools_tests --config Debug
ctest --preset dev -R MK_tools_tests --output-on-failure
```

Expected: PASS with authoring, duplicate, invalid-body, and package-index content tests.

## Task 5: Generated 3D Package Smoke

**Files:**
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex`
- Add: `games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/collision.scene3d`
- Modify: `games/CMakeLists.txt`
- Modify: `tools/validate-installed-desktop-runtime.ps1`

- [x] **Step 1: Add the failing installed smoke**

Run:

```powershell
$smokeArgs = @('--smoke', '--require-scene-collision-package')
& .\tools\package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs $smokeArgs
```

Expected: FAIL because the executable does not accept `--require-scene-collision-package`.

- [x] **Step 2: Add executable support**

Add a `require_scene_collision_package` option and load the collision package record from the already loaded runtime package. Emit these fields:

```text
collision_package_status=ready
collision_package_ready=1
collision_package_diagnostics=0
collision_package_bodies=3
collision_package_triggers=1
collision_package_contacts=1
collision_package_trigger_overlaps=1
collision_package_world_ready=1
gameplay_systems_collision_package_ready=1
```

Fail closed when the package record is absent, the record kind is wrong, payload decode fails, the physics bridge fails, no trigger row exists, or contact/trigger evidence is zero.

- [x] **Step 3: Add validator checks**

Teach `tools/validate-installed-desktop-runtime.ps1` to require exact values for all fields above when `--require-scene-collision-package` is present. Treat `collision_package_contacts` and `collision_package_trigger_overlaps` as numeric and require at least `1`.

- [x] **Step 4: Register the selected package smoke arg**

Add `--require-scene-collision-package` to the generated 3D package target metadata in `games/CMakeLists.txt` after `--require-visible-3d-production-proof`.

- [x] **Step 5: Run the selected package smoke and confirm GREEN**

Run:

```powershell
$smokeArgs = @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_3d_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_3d_package.geindex', '--require-scene-collision-package')
& .\tools\package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs $smokeArgs
```

Expected: PASS and validator output includes the exact `collision_package_*` fields.

## Task 6: Scaffold, Manifest, Docs, Skills, And Static Guards

**Files:**
- Modify: `tools/new-game.ps1`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Add static checks first**

Add checks for:

```text
--require-scene-collision-package
installed-d3d12-3d-scene-collision-package-smoke
GameEngine.PhysicsCollisionScene3D.v1
collision_package_status=ready
collision_package_bodies=3
collision_package_trigger_overlaps=1
gameplay_systems_collision_package_ready=1
physics-scene-package-collision-authoring-v1
```

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected before docs/template sync: FAIL on missing markers.

- [x] **Step 2: Update generated scaffold output**

Teach `tools/new-game.ps1 -Template DesktopRuntime3DPackage` to generate the collision payload file, `.geindex` row, executable flag, README command, and `game.agent.json` recipe.

- [x] **Step 3: Update machine-readable production guidance**

Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while active. Add guidance that this slice packages collision rows and keeps exact casts/sweeps, CCD, joints, Jolt/native backends, dynamic push/step policy, and broad physics readiness unsupported.

- [x] **Step 4: Update human docs and skills**

Document that generated 3D packages can use selected package collision rows through `--require-scene-collision-package`, but broader physics production readiness remains open until the later Physics 1.0 child plans finish.

- [x] **Step 5: Run static checks and confirm GREEN**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: PASS, with `physics-1-0-runtime-collision-system` still non-ready unless all required-before-ready claims are resolved by later slices.

## Task 7: Full Verification And Slice Closeout

**Files:**
- Modify: `docs/superpowers/plans/2026-05-09-physics-scene-package-collision-authoring-v1.md`

- [x] **Step 1: Run focused validation**

Run:

```powershell
cmake --build --preset dev --target MK_core_tests MK_runtime_tests MK_tools_tests sample_generated_desktop_runtime_3d_package --config Debug
ctest --preset dev -R "MK_core_tests|MK_runtime_tests|MK_tools_tests" --output-on-failure
```

Expected: PASS.

- [x] **Step 2: Run the selected generated package smoke**

Run:

```powershell
$smokeArgs = @('--smoke', '--require-scene-collision-package')
& .\tools\package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs $smokeArgs
```

Expected: PASS with `collision_package_status=ready` and `gameplay_systems_collision_package_ready=1`.

- [x] **Step 3: Run repository checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

Expected: PASS, or record the exact host/tool blocker in this plan.

- [x] **Step 4: Record evidence and move the pointer**

After all selected checks pass, update this file:

```markdown
**Status:** Completed.
```

Append a validation evidence table with the exact command, result, and relevant `collision_package_*` output. Then move `currentActivePlan` either to the next dated child plan selected from the master Physics 1.0 order or back to the master plan with `recommendedNextPlan.id=next-production-gap-selection`.

## Self-Review

- Spec coverage: this plan covers the first Physics 1.0 recommended production order item only: scene/package collision authoring and generated package smoke evidence.
- Exclusions: exact casts/sweeps, contact manifold stability, CCD, dynamic controller policy, joints, benchmarks, optional Jolt adapter, editor physics debugging UX, and broad physics readiness are intentionally deferred to later dated child plans.
- Placeholder scan: no step relies on a future unspecified function without naming the declaration, file, and validation command that introduces it.
- Type consistency: payload, bridge, package tool, smoke flag, recipe id, and status field names match across tasks.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_core_tests MK_runtime_tests MK_tools_tests --config Debug` | PASS | Focused assets/runtime/tools coverage for `AssetKind::physics_collision_scene`, runtime payload decoding, bridge conversion, package authoring, deterministic float formatting, invalid token rejection, and rollback behavior. |
| `ctest --preset dev -R "MK_core_tests|MK_runtime_tests|MK_tools_tests" --output-on-failure` | PASS | Focused CTest coverage for core package index round trips, runtime collision payload diagnostics, bridge construction, and package authoring/update helpers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public runtime/tools/physics-facing headers are accepted by the repository boundary check. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest/schema checks accept `physics_collision_scene`, `--require-scene-collision-package`, and generated 3D scaffold metadata. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent manifest, docs, skills, static markers, and generated 3D collision package markers are synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Known non-ready production gaps remain non-ready; this slice does not promote broad physics readiness. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository format wrapper accepted the touched source and docs after formatting. |
| `git diff --check` | PASS | Whitespace check exits 0; PowerShell reports existing LF-to-CRLF normalization warnings only. |
| `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs @('--smoke','--require-config','runtime/sample_generated_desktop_runtime_3d_package.config','--require-scene-package','runtime/sample_generated_desktop_runtime_3d_package.geindex','--require-scene-collision-package')` | PASS | Selected installed package smoke reports `collision_package_status=ready`, `collision_package_ready=1`, `collision_package_diagnostics=0`, `collision_package_bodies=3`, `collision_package_triggers=1`, `collision_package_contacts=1`, `collision_package_trigger_overlaps=1`, `collision_package_world_ready=1`, and `gameplay_systems_collision_package_ready=1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed after pointer/doc/manifest sync. Windows host gates Apple/Metal diagnostics as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Full dev preset build passed after validation. |
