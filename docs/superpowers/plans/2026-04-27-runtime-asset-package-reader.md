# Runtime Asset Package Reader Implementation Plan (2026-04-27)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent runtime asset package reader and first-party runtime asset handles so games can consume cooked packages without depending on source formats, tools, or native backend handles.

**Architecture:** Add a new `MK_runtime` module instead of making `MK_assets` depend on platform IO. `MK_runtime` depends on `MK_assets` and `MK_platform`, reads `AssetCookedPackageIndex` text through `IFileSystem`, validates package records and payload hashes, and exposes immutable runtime asset records through stable first-party handles. Runtime package replacement is all-or-nothing: a failed staged package never replaces the active package.

**Tech Stack:** C++23, CMake target `MK_runtime`, existing `MK_assets` cooked package index, existing `MK_platform::IFileSystem`, CTest, PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## File Structure

- Create `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
  - Public runtime package reader API.
  - Owns `RuntimeAssetHandle`, `RuntimeAssetRecord`, `RuntimeAssetPackage`, `RuntimeAssetPackageLoadResult`, and `RuntimeAssetPackageStore`.
  - Must not expose native OS, renderer, RHI, or tool handles.
- Create `engine/runtime/src/asset_runtime.cpp`
  - Implements path validation, package index reading, payload hash checks, dependency existence checks, immutable package construction, and staging/commit behavior.
- Create `engine/runtime/CMakeLists.txt`
  - Defines `MK_runtime`, public include dirs, links `MK_assets` and `MK_platform`, applies shared target options.
- Create `tests/unit/runtime_tests.cpp`
  - Focused tests for runtime package load success, invalid hashes, dependency validation, unsupported formats, and replacement staging.
- Modify `CMakeLists.txt`
  - Add `engine/runtime`.
  - Link `MK_runtime` into sample/game/test targets where public runtime API should be available.
  - Install/export the new target and public headers.
- Modify `tools/check-public-api-boundaries.ps1`
  - Include `engine/runtime/include`.
- Modify `docs/architecture.md`, `docs/roadmap.md`, `docs/testing.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, and `engine/agent/manifest.json`
  - Record the new runtime content contract honestly.

## Task 1: Runtime Package API And Success Path

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Create: `engine/runtime/src/asset_runtime.cpp`
- Create: `engine/runtime/CMakeLists.txt`
- Create: `tests/unit/runtime_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tools/check-public-api-boundaries.ps1`

- [x] **Step 1: Write the failing test**

Add this test to `tests/unit/runtime_tests.cpp`:

```cpp
#include "test_framework.hpp"

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/runtime/asset_runtime.hpp"

MK_TEST("runtime asset package loads cooked payloads with stable handles") {
    mirakana::MemoryFileSystem fs;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";

    const auto index = mirakana::build_asset_cooked_package_index(
        {mirakana::AssetCookedArtifact{texture, mirakana::AssetKind::texture, "assets/textures/player.texture", payload, 3, {}}},
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/textures/player.texture", payload);

    const auto result = mirakana::runtime::load_runtime_asset_package(
        fs, mirakana::runtime::RuntimeAssetPackageDesc{"packages/main.geindex"});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.package.records().size() == 1);

    const auto* by_asset = result.package.find(texture);
    MK_REQUIRE(by_asset != nullptr);
    MK_REQUIRE(by_asset->asset == texture);
    MK_REQUIRE(by_asset->kind == mirakana::AssetKind::texture);
    MK_REQUIRE(by_asset->content == payload);
    MK_REQUIRE(by_asset->handle.value == 1);

    const auto* by_handle = result.package.find(by_asset->handle);
    MK_REQUIRE(by_handle == by_asset);
}
```

- [x] **Step 2: Run the test and verify RED**

Run: `cmake --build --preset dev --target MK_runtime_tests`

Expected: configure/build fails because `mirakana/runtime/asset_runtime.hpp` and target `MK_runtime_tests` do not exist.

- [x] **Step 3: Add the minimal public API**

Create `engine/runtime/include/mirakana/runtime/asset_runtime.hpp` with:

```cpp
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

struct RuntimeAssetHandle {
    std::uint32_t value{0};

    friend bool operator==(RuntimeAssetHandle lhs, RuntimeAssetHandle rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

struct RuntimeAssetRecord {
    RuntimeAssetHandle handle;
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string path;
    std::uint64_t content_hash{0};
    std::uint64_t source_revision{0};
    std::vector<AssetId> dependencies;
    std::string content;
};

struct RuntimeAssetPackageDesc {
    std::string index_path;
    std::string content_root;
};

struct RuntimeAssetPackageLoadFailure {
    AssetId asset;
    std::string path;
    std::string diagnostic;
};

class RuntimeAssetPackage {
  public:
    explicit RuntimeAssetPackage(std::vector<RuntimeAssetRecord> records = {});

    [[nodiscard]] const std::vector<RuntimeAssetRecord>& records() const noexcept;
    [[nodiscard]] const RuntimeAssetRecord* find(AssetId asset) const noexcept;
    [[nodiscard]] const RuntimeAssetRecord* find(RuntimeAssetHandle handle) const noexcept;
    [[nodiscard]] bool empty() const noexcept;

  private:
    std::vector<RuntimeAssetRecord> records_;
};

struct RuntimeAssetPackageLoadResult {
    RuntimeAssetPackage package;
    std::vector<RuntimeAssetPackageLoadFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimeAssetPackageLoadResult load_runtime_asset_package(IFileSystem& filesystem,
                                                                      const RuntimeAssetPackageDesc& desc);

} // namespace mirakana::runtime
```

- [x] **Step 4: Add minimal implementation**

Create `engine/runtime/src/asset_runtime.cpp` with:

```cpp
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/asset_runtime.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool valid_package_path(std::string_view path) noexcept {
    return !path.empty() && path.find('\0') == std::string_view::npos && path.find('\n') == std::string_view::npos &&
           path.find('\r') == std::string_view::npos && path.find('\\') == std::string_view::npos &&
           path.find(':') == std::string_view::npos && path.front() != '/' && path.find("..") == std::string_view::npos;
}

[[nodiscard]] std::string join_root(std::string_view root, std::string_view path) {
    if (!valid_package_path(path)) {
        throw std::invalid_argument("runtime asset package path is invalid");
    }
    if (root.empty()) {
        return std::string(path);
    }
    if (!valid_package_path(root)) {
        throw std::invalid_argument("runtime asset package content root is invalid");
    }
    std::string result(root);
    if (result.back() != '/') {
        result.push_back('/');
    }
    result.append(path);
    return result;
}

} // namespace

RuntimeAssetPackage::RuntimeAssetPackage(std::vector<RuntimeAssetRecord> records) : records_(std::move(records)) {}

const std::vector<RuntimeAssetRecord>& RuntimeAssetPackage::records() const noexcept {
    return records_;
}

const RuntimeAssetRecord* RuntimeAssetPackage::find(AssetId asset) const noexcept {
    const auto it = std::find_if(records_.begin(), records_.end(),
                                 [asset](const RuntimeAssetRecord& record) { return record.asset == asset; });
    return it == records_.end() ? nullptr : &*it;
}

const RuntimeAssetRecord* RuntimeAssetPackage::find(RuntimeAssetHandle handle) const noexcept {
    if (handle.value == 0 || handle.value > records_.size()) {
        return nullptr;
    }
    return &records_[handle.value - 1U];
}

bool RuntimeAssetPackage::empty() const noexcept {
    return records_.empty();
}

bool RuntimeAssetPackageLoadResult::succeeded() const noexcept {
    return failures.empty();
}

RuntimeAssetPackageLoadResult load_runtime_asset_package(IFileSystem& filesystem, const RuntimeAssetPackageDesc& desc) {
    if (!valid_package_path(desc.index_path)) {
        throw std::invalid_argument("runtime asset package index path is invalid");
    }

    const auto index = deserialize_asset_cooked_package_index(filesystem.read_text(desc.index_path));
    std::vector<RuntimeAssetRecord> records;
    std::vector<RuntimeAssetPackageLoadFailure> failures;
    std::unordered_set<AssetId, AssetIdHash> assets;

    records.reserve(index.entries.size());
    for (const auto& entry : index.entries) {
        if (!is_valid_asset_cooked_package_entry(entry)) {
            failures.push_back(RuntimeAssetPackageLoadFailure{entry.asset, entry.path, "invalid package entry"});
            continue;
        }
        if (!assets.insert(entry.asset).second) {
            failures.push_back(RuntimeAssetPackageLoadFailure{entry.asset, entry.path, "duplicate package asset"});
            continue;
        }
        const auto payload_path = join_root(desc.content_root, entry.path);
        if (!filesystem.exists(payload_path)) {
            failures.push_back(RuntimeAssetPackageLoadFailure{entry.asset, payload_path, "missing cooked payload"});
            continue;
        }
        auto content = filesystem.read_text(payload_path);
        if (hash_asset_cooked_content(content) != entry.content_hash) {
            failures.push_back(RuntimeAssetPackageLoadFailure{entry.asset, payload_path, "cooked payload hash mismatch"});
            continue;
        }
        records.push_back(RuntimeAssetRecord{RuntimeAssetHandle{static_cast<std::uint32_t>(records.size() + 1U)},
                                             entry.asset,
                                             entry.kind,
                                             payload_path,
                                             entry.content_hash,
                                             entry.source_revision,
                                             entry.dependencies,
                                             std::move(content)});
    }

    if (!failures.empty()) {
        return RuntimeAssetPackageLoadResult{RuntimeAssetPackage{}, std::move(failures)};
    }
    return RuntimeAssetPackageLoadResult{RuntimeAssetPackage{std::move(records)}, {}};
}

} // namespace mirakana::runtime
```

- [x] **Step 5: Add CMake wiring**

Create `engine/runtime/CMakeLists.txt`:

```cmake
add_library(MK_runtime
    src/asset_runtime.cpp
)

target_include_directories(MK_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_runtime
    PUBLIC
        MK_assets
        MK_platform
)

MK_apply_common_target_options(MK_runtime)
```

Modify root `CMakeLists.txt`:

```cmake
add_subdirectory(engine/runtime)
```

Add `MK_runtime_tests` under `if(BUILD_TESTING)`:

```cmake
add_executable(MK_runtime_tests
    tests/unit/runtime_tests.cpp
)
target_link_libraries(MK_runtime_tests PRIVATE MK_runtime)
target_include_directories(MK_runtime_tests PRIVATE tests)
MK_apply_common_target_options(MK_runtime_tests)
add_test(NAME MK_runtime_tests COMMAND MK_runtime_tests)
```

Add export/install entries:

```cmake
MK_set_export_name(MK_runtime runtime)
```

Add `MK_runtime` to `MK_LIBRARY_TARGETS`, install `engine/runtime/include/`, link it in `MK_add_game`, `MK_sandbox`, and `MK_core_tests`.

Add `engine/runtime/include` to `$publicRoots` in `tools/check-public-api-boundaries.ps1`.

- [x] **Step 6: Run GREEN verification**

Run: `cmake --preset dev`

Expected: configure succeeds.

Run: `cmake --build --preset dev --target MK_runtime_tests`

Expected: build succeeds.

Run: `ctest --test-dir out/build/dev -C Debug -R MK_runtime_tests --output-on-failure`

Expected: `MK_runtime_tests` passes.

## Task 2: Failure Diagnostics And Dependency Validation

**Files:**
- Modify: `tests/unit/runtime_tests.cpp`
- Modify: `engine/runtime/src/asset_runtime.cpp`

- [x] **Step 1: Write failing tests**

Add tests for:

```cpp
MK_TEST("runtime asset package rejects payload hash mismatches without partial package") {
    mirakana::MemoryFileSystem fs;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const auto good_payload = std::string{"format=GameEngine.CookedTexture.v1\ntexture.width=4\n"};
    const auto bad_payload = std::string{"format=GameEngine.CookedTexture.v1\ntexture.width=8\n"};
    const auto index = mirakana::build_asset_cooked_package_index(
        {mirakana::AssetCookedArtifact{texture, mirakana::AssetKind::texture, "assets/textures/player.texture", good_payload, 3, {}}},
        {});
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/textures/player.texture", bad_payload);

    const auto result = mirakana::runtime::load_runtime_asset_package(fs, {"packages/main.geindex"});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.package.empty());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("hash") != std::string::npos);
}

MK_TEST("runtime asset package rejects dependencies missing from the package") {
    mirakana::MemoryFileSystem fs;
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const auto payload = std::string{"format=GameEngine.Material.v1\nmaterial.name=Player\n"};
    const auto index = mirakana::AssetCookedPackageIndex{
        {mirakana::AssetCookedPackageEntry{material, mirakana::AssetKind::material, "assets/materials/player.material",
                                     mirakana::hash_asset_cooked_content(payload), 1, {texture}}},
        {},
    };
    fs.write_text("packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    fs.write_text("assets/materials/player.material", payload);

    const auto result = mirakana::runtime::load_runtime_asset_package(fs, {"packages/main.geindex"});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.package.empty());
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("dependency") != std::string::npos);
}
```

- [x] **Step 2: Run and verify RED**

Run: `cmake --build --preset dev --target MK_runtime_tests && ctest --test-dir out/build/dev -C Debug -R MK_runtime_tests --output-on-failure`

Expected: hash mismatch test may pass from Task 1; dependency-missing test fails until dependency validation is implemented.

- [x] **Step 3: Implement dependency validation**

After reading entries and before returning success, validate every `RuntimeAssetRecord::dependencies` points to a loaded asset:

```cpp
for (const auto& record : records) {
    for (const auto dependency : record.dependencies) {
        if (assets.find(dependency) == assets.end()) {
            failures.push_back(RuntimeAssetPackageLoadFailure{record.asset, record.path,
                                                              "runtime asset package dependency is missing"});
        }
    }
}
```

If `failures` is no longer empty after this check, return an empty package.

- [x] **Step 4: Run GREEN verification**

Run: `cmake --build --preset dev --target MK_runtime_tests && ctest --test-dir out/build/dev -C Debug -R MK_runtime_tests --output-on-failure`

Expected: all runtime tests pass.

## Task 3: All-Or-Nothing Runtime Package Store

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Modify: `engine/runtime/src/asset_runtime.cpp`
- Modify: `tests/unit/runtime_tests.cpp`

- [x] **Step 1: Write failing test**

Add:

```cpp
MK_TEST("runtime asset package store stages and commits replacements at a safe point") {
    mirakana::runtime::RuntimeAssetPackageStore store;
    const auto texture = mirakana::AssetId::from_name("textures/player");

    mirakana::runtime::RuntimeAssetPackage initial({
        mirakana::runtime::RuntimeAssetRecord{mirakana::runtime::RuntimeAssetHandle{1}, texture, mirakana::AssetKind::texture,
                                        "assets/textures/player.texture", 10, 1, {}, "v1"},
    });
    mirakana::runtime::RuntimeAssetPackage replacement({
        mirakana::runtime::RuntimeAssetRecord{mirakana::runtime::RuntimeAssetHandle{1}, texture, mirakana::AssetKind::texture,
                                        "assets/textures/player.texture", 20, 2, {}, "v2"},
    });

    store.seed(std::move(initial));
    MK_REQUIRE(store.active() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "v1");

    store.stage(std::move(replacement));
    MK_REQUIRE(store.pending() != nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "v1");

    MK_REQUIRE(store.commit_safe_point());
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "v2");
}

MK_TEST("runtime asset package store keeps active package when staged load fails") {
    mirakana::runtime::RuntimeAssetPackageStore store;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    store.seed(mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{mirakana::runtime::RuntimeAssetHandle{1}, texture, mirakana::AssetKind::texture,
                                        "assets/textures/player.texture", 10, 1, {}, "v1"},
    }));

    const auto failed = mirakana::runtime::RuntimeAssetPackageLoadResult{
        mirakana::runtime::RuntimeAssetPackage{},
        {mirakana::runtime::RuntimeAssetPackageLoadFailure{texture, "assets/textures/player.texture", "hash mismatch"}},
    };

    MK_REQUIRE(!store.stage_if_loaded(std::move(failed)));
    MK_REQUIRE(store.pending() == nullptr);
    MK_REQUIRE(store.active()->find(texture)->content == "v1");
}
```

- [x] **Step 2: Run and verify RED**

Run: `cmake --build --preset dev --target MK_runtime_tests`

Expected: build fails because `RuntimeAssetPackageStore` does not exist.

- [x] **Step 3: Implement the store API**

Add to the header:

```cpp
class RuntimeAssetPackageStore {
  public:
    void seed(RuntimeAssetPackage package);
    void stage(RuntimeAssetPackage package);
    [[nodiscard]] bool stage_if_loaded(RuntimeAssetPackageLoadResult result);
    [[nodiscard]] bool commit_safe_point();
    void rollback_pending() noexcept;

    [[nodiscard]] const RuntimeAssetPackage* active() const noexcept;
    [[nodiscard]] const RuntimeAssetPackage* pending() const noexcept;

  private:
    RuntimeAssetPackage active_;
    RuntimeAssetPackage pending_;
    bool has_active_{false};
    bool has_pending_{false};
};
```

Add matching implementation with `std::move`, no heap allocation, and all-or-nothing replacement:

```cpp
void RuntimeAssetPackageStore::seed(RuntimeAssetPackage package) {
    active_ = std::move(package);
    has_active_ = true;
    pending_ = RuntimeAssetPackage{};
    has_pending_ = false;
}

void RuntimeAssetPackageStore::stage(RuntimeAssetPackage package) {
    pending_ = std::move(package);
    has_pending_ = true;
}

bool RuntimeAssetPackageStore::stage_if_loaded(RuntimeAssetPackageLoadResult result) {
    if (!result.succeeded()) {
        return false;
    }
    stage(std::move(result.package));
    return true;
}

bool RuntimeAssetPackageStore::commit_safe_point() {
    if (!has_pending_) {
        return false;
    }
    active_ = std::move(pending_);
    has_active_ = true;
    pending_ = RuntimeAssetPackage{};
    has_pending_ = false;
    return true;
}

void RuntimeAssetPackageStore::rollback_pending() noexcept {
    pending_ = RuntimeAssetPackage{};
    has_pending_ = false;
}

const RuntimeAssetPackage* RuntimeAssetPackageStore::active() const noexcept {
    return has_active_ ? &active_ : nullptr;
}

const RuntimeAssetPackage* RuntimeAssetPackageStore::pending() const noexcept {
    return has_pending_ ? &pending_ : nullptr;
}
```

- [x] **Step 4: Run GREEN verification**

Run: `cmake --build --preset dev --target MK_runtime_tests && ctest --test-dir out/build/dev -C Debug -R MK_runtime_tests --output-on-failure`

Expected: all runtime tests pass.

## Task 4: Public API, Docs, Manifest, And Installed SDK

**Files:**
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `CMakeLists.txt`
- Modify: `tools/check-public-api-boundaries.ps1`

- [x] **Step 1: Update docs and manifest**

Record:

- `MK_runtime` owns game-facing runtime package loading and active/staged package state.
- `MK_assets` remains source/cook/index ownership and does not depend on platform IO.
- Runtime package reads use `IFileSystem`.
- Runtime package replacement is safe-point based and all-or-nothing.
- AI-generated games may use `mirakana::runtime` handles but must not parse source formats.

- [x] **Step 2: Run manifest/schema checks**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`

Expected: `json-contract-check: ok`

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`

Expected: `ai-integration-check: ok`

- [x] **Step 3: Run public API boundary check**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`

Expected: `public-api-boundary-check: ok`

## Task 5: Full Validation

**Files:**
- No new files beyond previous tasks.

- [x] **Step 1: Run focused build/test**

Run: `cmake --build --preset dev --target MK_runtime_tests`

Expected: build succeeds.

Run: `ctest --test-dir out/build/dev -C Debug -R MK_runtime_tests --output-on-failure`

Expected: `MK_runtime_tests` passes.

- [x] **Step 2: Run default validation**

Run: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

Expected: `validate: ok` and all CTest tests pass. Diagnostic-only shader/mobile/tidy blockers may still be reported exactly as existing environment blockers.

- [x] **Step 3: Update roadmap checkbox only if scope is actually complete**

Do not mark Apple/Metal host-gated roadmap item complete. This work is a new production-essential slice, not the final Wave 9 item.

## Self-Review

- Spec coverage: Covers runtime package reading, version rejection through existing index deserialization, content hash validation, dependency existence validation, first-party runtime handles, and staged replacement without replacing active content on failure.
- Dependency direction: `MK_assets` stays standard-library-only. `MK_runtime` owns `IFileSystem` use.
- No native handles: Public API uses only first-party asset ids, asset kinds, strings, vectors, and runtime handles.
- Testing: The plan starts with failing tests and verifies with focused CTest plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- Deferred by design: Binary package format, memory-mapped IO, renderer texture upload, GPU material binding, and editor GUI are intentionally not in this slice.
