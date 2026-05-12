# Asset Identity Runtime Resource v2 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a small asset identity layer plus generation-checked runtime resource handles so Scene/Component/Prefab Schema v2 can reference assets without depending on importer, package, renderer, or editor internals.

**Architecture:** `MK_assets` owns stable asset identity documents and deterministic identity validation/serialization. `MK_runtime` owns runtime resource handles and catalog state built from cooked package records, without depending on renderer/RHI or parsing source formats. This slice does not migrate all existing asset references, editor panels, RHI upload ownership, or package mount streaming.

**Tech Stack:** C++23, `MK_assets`, `MK_runtime`, `MK_platform` test filesystem helpers, repository test framework, CMake target registration, `engine/agent/manifest.json`, and `tools/*.ps1` validation commands.

---

## Goal

Create the first production data-spine bridge after Scene/Component/Prefab Schema v2:

- stable asset keys that are readable by AI and deterministic across hosts
- explicit mapping from stable keys to current `mirakana::AssetId` values and asset kinds
- validation diagnostics for malformed identities and duplicate rows
- deterministic `GameEngine.AssetIdentity.v2` text IO
- generation-checked runtime resource handles that reject stale handles after package replacement
- runtime resource catalog diagnostics that do not expose package internals, renderer handles, or native backend objects

## Context

- Scene/Component/Prefab Schema v2 added stable authoring ids and schema-driven component rows in `MK_scene`.
- Existing `MK_assets` has `AssetId`, `AssetKind`, `AssetRecord`, import metadata, package indexes, and material documents.
- Existing `MK_runtime` has `RuntimeAssetHandle`, `RuntimeAssetPackage`, typed payload access, and safe-point package replacement.
- Future 2D/3D vertical slices need asset identity and runtime resource handles before AI can safely place sprites, meshes, materials, audio cues, UI images, and packages without reaching into importer/package details.

## Constraints

- Keep `engine/assets` independent from renderer, RHI, platform, editor, SDL3, Dear ImGui, and native handles.
- Keep `engine/runtime` independent from renderer, RHI backends, editor, SDL3, Dear ImGui, and native handles.
- Do not add third-party dependencies.
- Do not parse external source asset formats in runtime/game code.
- Do not migrate existing scene/material/package flows in this slice unless a test explicitly requires a narrow bridge.
- Keep future 2D/3D playable recipes `planned`.

## Done When

- `MK_assets` exposes Asset Identity v2 value APIs, validation, deterministic serialization, and stable `AssetId` derivation.
- `MK_runtime` exposes Runtime Resource v2 value APIs with generation-checked handles and package-record catalog construction.
- Focused tests prove duplicate identity rejection, deterministic text IO, handle lookup, stale-handle rejection after replacement, and no renderer/RHI dependency.
- Manifest/docs/checks mark Asset Identity + Runtime Resource v2 as implemented foundation only, not renderer resource residency, package streaming, or 2D/3D vertical slice readiness.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## File Structure

- Create `engine/assets/include/mirakana/assets/asset_identity.hpp`: public Asset Identity v2 value types and function declarations.
- Create `engine/assets/src/asset_identity.cpp`: validation, deterministic serialization/deserialization, and `AssetId` derivation.
- Modify `engine/assets/CMakeLists.txt`: add `src/asset_identity.cpp`.
- Create `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`: public Runtime Resource v2 handle/catalog value APIs.
- Create `engine/runtime/src/resource_runtime.cpp`: catalog construction, lookup, stale-handle validation, and replacement generation behavior.
- Modify `engine/runtime/CMakeLists.txt`: add `src/resource_runtime.cpp`.
- Create `tests/unit/asset_identity_runtime_resource_tests.cpp`: focused tests for identity and runtime resource contracts.
- Modify `CMakeLists.txt`: register `MK_asset_identity_runtime_resource_tests`.
- Modify `engine/agent/manifest.json`: update `MK_assets`, `MK_runtime`, `aiOperableProductionLoop.authoringSurfaces`, and `unsupportedProductionGaps`.
- Modify `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1`: enforce honest foundation-only claims.
- Modify `docs/architecture.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/specs/generated-game-validation-scenarios.md`, `docs/specs/game-prompt-pack.md`, this plan, and the plan registry.

## Implementation Tasks

### Task 1: RED Tests For Asset Identity Documents

**Files:**
- Create: `tests/unit/asset_identity_runtime_resource_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add failing tests for duplicate keys and deterministic identity IO**

Add:

```cpp
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_identity.hpp"

#include <string>

MK_TEST("asset identity v2 rejects duplicate keys deterministically") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        mirakana::AssetKeyV2{"textures/player/albedo"},
        mirakana::AssetKind::texture,
        "source/textures/player_albedo.png",
    });
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        mirakana::AssetKeyV2{"textures/player/albedo"},
        mirakana::AssetKind::material,
        "source/materials/player.material",
    });

    const auto diagnostics = mirakana::validate_asset_identity_document_v2(document);

    MK_REQUIRE(diagnostics.size() == 1);
    MK_REQUIRE(diagnostics[0].code == mirakana::AssetIdentityDiagnosticCodeV2::duplicate_key);
    MK_REQUIRE(diagnostics[0].key.value == "textures/player/albedo");
}

MK_TEST("asset identity v2 serializes stable text and derives asset ids from keys") {
    mirakana::AssetIdentityDocumentV2 document;
    document.assets.push_back(mirakana::AssetIdentityRowV2{
        mirakana::AssetKeyV2{"materials/player"},
        mirakana::AssetKind::material,
        "source/materials/player.material",
    });

    const std::string expected =
        "format=GameEngine.AssetIdentity.v2\n"
        "asset.0.key=materials/player\n"
        "asset.0.id=" +
        std::to_string(mirakana::AssetId::from_name("materials/player").value) +
        "\n"
        "asset.0.kind=material\n"
        "asset.0.source=source/materials/player.material\n";

    MK_REQUIRE(mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{"materials/player"}) ==
               mirakana::AssetId::from_name("materials/player"));
    MK_REQUIRE(mirakana::serialize_asset_identity_document_v2(document) == expected);

    const auto round_trip = mirakana::deserialize_asset_identity_document_v2(expected);
    MK_REQUIRE(round_trip.assets.size() == 1);
    MK_REQUIRE(round_trip.assets[0].key.value == "materials/player");
    MK_REQUIRE(round_trip.assets[0].kind == mirakana::AssetKind::material);
}

int main() {
    return mirakana::test::run_all();
}
```

- [x] **Step 2: Register the focused test target**

Add this test target in root `CMakeLists.txt` inside `if(BUILD_TESTING)`:

```cmake
add_executable(MK_asset_identity_runtime_resource_tests
    tests/unit/asset_identity_runtime_resource_tests.cpp
)
target_link_libraries(MK_asset_identity_runtime_resource_tests PRIVATE MK_assets MK_runtime MK_platform)
target_include_directories(MK_asset_identity_runtime_resource_tests PRIVATE tests)
MK_apply_common_target_options(MK_asset_identity_runtime_resource_tests)
add_test(NAME MK_asset_identity_runtime_resource_tests COMMAND MK_asset_identity_runtime_resource_tests)
```

- [x] **Step 3: Verify RED**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests
```

Expected: compile failure because `mirakana/assets/asset_identity.hpp` does not exist.

### Task 2: Asset Identity v2 Value Types And Text IO

**Files:**
- Create: `engine/assets/include/mirakana/assets/asset_identity.hpp`
- Create: `engine/assets/src/asset_identity.cpp`
- Modify: `engine/assets/CMakeLists.txt`

- [x] **Step 1: Add public value types**

Add:

```cpp
#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct AssetKeyV2 {
    std::string value;
};

struct AssetIdentityRowV2 {
    AssetKeyV2 key;
    AssetKind kind{AssetKind::unknown};
    std::string source_path;
};

struct AssetIdentityDocumentV2 {
    std::vector<AssetIdentityRowV2> assets;
};

enum class AssetIdentityDiagnosticCodeV2 {
    invalid_key,
    duplicate_key,
    invalid_kind,
    invalid_source_path,
    duplicate_source_path,
};

struct AssetIdentityDiagnosticV2 {
    AssetIdentityDiagnosticCodeV2 code{AssetIdentityDiagnosticCodeV2::invalid_key};
    AssetKeyV2 key;
    std::string source_path;
};

[[nodiscard]] AssetId asset_id_from_key_v2(AssetKeyV2 key) noexcept;
[[nodiscard]] std::vector<AssetIdentityDiagnosticV2>
validate_asset_identity_document_v2(const AssetIdentityDocumentV2& document);
[[nodiscard]] std::string serialize_asset_identity_document_v2(const AssetIdentityDocumentV2& document);
[[nodiscard]] AssetIdentityDocumentV2 deserialize_asset_identity_document_v2(std::string_view text);

} // namespace mirakana
```

- [x] **Step 2: Implement validation and serialization**

Rules:

- key must be non-empty, contain no ASCII control characters, contain no whitespace, not start with `/`, and not contain `..` path segments
- source path must be non-empty, relative, contain no control characters, not start with `/`, not contain `..` path segments, and not contain `;`
- kind must not be `AssetKind::unknown`
- keys must be unique
- source paths must be unique after exact string comparison
- `asset_id_from_key_v2` returns `AssetId::from_name(key.value)`
- serialization emits rows in vector order
- deserialization rejects unsupported `format` values and validates the result

- [x] **Step 3: Verify GREEN**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests
```

Expected: asset identity tests pass.

### Task 3: RED Tests For Runtime Resource Handles

**Files:**
- Modify: `tests/unit/asset_identity_runtime_resource_tests.cpp`

- [x] **Step 1: Add failing runtime resource tests**

Append:

```cpp
#include "mirakana/runtime/resource_runtime.hpp"

MK_TEST("runtime resource v2 resolves handles and rejects stale generations") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeAssetPackage first_package({
        mirakana::runtime::RuntimeAssetRecord{
            mirakana::runtime::RuntimeAssetHandle{7},
            texture,
            mirakana::AssetKind::texture,
            "assets/textures/player.texture",
            11,
            1,
            {},
            "format=GameEngine.CookedTexture.v1\ntexture.width=4\n",
        },
    });

    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto first_build = mirakana::runtime::build_runtime_resource_catalog_v2(catalog, first_package);

    MK_REQUIRE(first_build.succeeded());
    const auto first_handle = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(first_handle.has_value());
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, *first_handle));

    mirakana::runtime::RuntimeAssetPackage second_package({
        mirakana::runtime::RuntimeAssetRecord{
            mirakana::runtime::RuntimeAssetHandle{9},
            texture,
            mirakana::AssetKind::texture,
            "assets/textures/player.texture",
            22,
            2,
            {},
            "format=GameEngine.CookedTexture.v1\ntexture.width=8\n",
        },
    });

    const auto second_build = mirakana::runtime::build_runtime_resource_catalog_v2(catalog, second_package);
    MK_REQUIRE(second_build.succeeded());

    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog, *first_handle));
    const auto second_handle = mirakana::runtime::find_runtime_resource_v2(catalog, texture);
    MK_REQUIRE(second_handle.has_value());
    MK_REQUIRE(second_handle->generation != first_handle->generation);
    const auto* record = mirakana::runtime::runtime_resource_record_v2(catalog, *second_handle);
    MK_REQUIRE(record != nullptr);
    MK_REQUIRE(record->asset == texture);
    MK_REQUIRE(record->package_handle == mirakana::runtime::RuntimeAssetHandle{9});
}
```

- [x] **Step 2: Verify RED**

Run the focused build command from Task 2.

Expected: compile failure because `mirakana/runtime/resource_runtime.hpp` and runtime resource APIs do not exist.

### Task 4: Runtime Resource v2 Catalog

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`
- Create: `engine/runtime/src/resource_runtime.cpp`
- Modify: `engine/runtime/CMakeLists.txt`

- [x] **Step 1: Add public runtime resource value APIs**

Add:

```cpp
#pragma once

#include "mirakana/runtime/asset_runtime.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::runtime {

struct RuntimeResourceHandleV2 {
    std::uint32_t index{0};
    std::uint32_t generation{0};

    friend bool operator==(RuntimeResourceHandleV2 lhs, RuntimeResourceHandleV2 rhs) noexcept {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }
};

struct RuntimeResourceRecordV2 {
    RuntimeResourceHandleV2 handle;
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    RuntimeAssetHandle package_handle;
    std::string path;
    std::uint64_t content_hash{0};
    std::uint64_t source_revision{0};
};

struct RuntimeResourceCatalogBuildDiagnosticV2 {
    AssetId asset;
    std::string diagnostic;
};

struct RuntimeResourceCatalogBuildResultV2 {
    std::vector<RuntimeResourceCatalogBuildDiagnosticV2> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

class RuntimeResourceCatalogV2 {
  public:
    [[nodiscard]] const std::vector<RuntimeResourceRecordV2>& records() const noexcept;

  private:
    friend RuntimeResourceCatalogBuildResultV2 build_runtime_resource_catalog_v2(
        RuntimeResourceCatalogV2& catalog, const RuntimeAssetPackage& package);

    std::vector<RuntimeResourceRecordV2> records_;
    std::uint32_t generation_{0};
};

[[nodiscard]] RuntimeResourceCatalogBuildResultV2 build_runtime_resource_catalog_v2(
    RuntimeResourceCatalogV2& catalog, const RuntimeAssetPackage& package);
[[nodiscard]] std::optional<RuntimeResourceHandleV2> find_runtime_resource_v2(
    const RuntimeResourceCatalogV2& catalog, AssetId asset) noexcept;
[[nodiscard]] bool is_runtime_resource_handle_live_v2(
    const RuntimeResourceCatalogV2& catalog, RuntimeResourceHandleV2 handle) noexcept;
[[nodiscard]] const RuntimeResourceRecordV2* runtime_resource_record_v2(
    const RuntimeResourceCatalogV2& catalog, RuntimeResourceHandleV2 handle) noexcept;

} // namespace mirakana::runtime
```

- [x] **Step 2: Implement catalog construction and stale-handle checks**

Rules:

- each build increments the catalog generation by one, wrapping from `UINT32_MAX` to `1`
- catalog records are rebuilt from the package's current records in package order
- record handle index is one-based to keep `{0,0}` invalid
- duplicate package asset ids produce one diagnostic and leave the old catalog unchanged
- `find_runtime_resource_v2` returns the first live handle for an asset
- `is_runtime_resource_handle_live_v2` checks nonzero index, index range, and generation match
- `runtime_resource_record_v2` returns `nullptr` for stale or out-of-range handles

- [x] **Step 3: Verify GREEN**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests
```

Expected: identity and runtime resource tests pass.

### Task 5: Manifest, Docs, And Static Checks

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`

- [x] **Step 1: Update manifest truthfully**

Set:

- `MK_assets.status` to an Asset Identity v2 foundation status and include `engine/assets/include/mirakana/assets/asset_identity.hpp`.
- `MK_runtime.status` to a Runtime Resource v2 foundation status and include `engine/runtime/include/mirakana/runtime/resource_runtime.hpp`.
- `aiOperableProductionLoop.unsupportedProductionGaps.asset-identity-v2.status` to `implemented-foundation-only`.
- `aiOperableProductionLoop.unsupportedProductionGaps.runtime-resource-v2.status` to `implemented-foundation-only`.

Keep `future-2d-playable-vertical-slice` and `future-3d-playable-vertical-slice` as `planned`.

- [x] **Step 2: Strengthen checks**

Make checks fail when:

- `MK_assets` omits `asset_identity.hpp`
- `MK_runtime` omits `resource_runtime.hpp`
- Asset Identity v2 or Runtime Resource v2 are marked `ready` instead of `implemented-foundation-only`
- docs claim renderer residency, streaming package mounts, editor migration, or 2D/3D vertical-slice readiness from this foundation
- future 2D/3D recipes stop being `planned`

- [x] **Step 3: Update docs**

Document this as foundation-only:

- Asset Identity v2 gives stable keys and deterministic mapping to current `AssetId`.
- Runtime Resource v2 gives generation-checked handles over cooked package records.
- Renderer/RHI residency, upload/staging, package mounts, editor asset browser migration, and 2D/3D vertical slices remain follow-up work.

### Task 6: Validation And Closure

**Files:**
- Modify: `docs/superpowers/plans/2026-05-01-asset-identity-runtime-resource-v2.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] **Step 1: Run focused validation**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests
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

- RED asset identity test:
  - Command: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests`
  - Result: failed as expected with `fatal error C1083: cannot open include file: 'mirakana/assets/asset_identity.hpp'`.
- Asset Identity v2 GREEN:
  - Command: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests`
  - Result: PASS.
  - Command: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests`
  - Result: PASS, 1/1 focused tests.
- RED runtime resource test:
  - Command: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests`
  - Result: failed as expected with `fatal error C1083: cannot open include file: 'mirakana/runtime/resource_runtime.hpp'`.
- Runtime Resource v2 GREEN:
  - Command: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests`
  - Result: PASS.
  - Command: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_asset_identity_runtime_resource_tests`
  - Result: PASS, 1/1 focused tests.
- Contract checks:
  - Command: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
  - Result: PASS.
  - Command: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - Result: PASS, `json-contract-check: ok`.
  - Command: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - Result: PASS, `ai-integration-check: ok`.
- Completion validation:
  - Command: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`
  - Result: PASS.
  - Command: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  - Result: PASS after this evidence update; validation output records the exact CTest count.
