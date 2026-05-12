# Renderer RHI Resource Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a backend-neutral renderer/RHI resource lifetime foundation with explicit resource ids, debug names, deferred release, and deterministic diagnostics without exposing native GPU handles.

**Architecture:** `MK_rhi` owns the first value-only resource lifetime registry because it is the backend-neutral contract boundary shared by D3D12, Vulkan, Metal, renderer, and runtime upload code. This slice records ownership, debug names, deferred-release generations, and marker-style event rows; it does not migrate D3D12/Vulkan/Metal backend internals, implement GPU memory allocators, or make 2D/3D vertical slices ready.

**Tech Stack:** C++23, `MK_rhi`, repository unit test framework, CMake target registration, `engine/agent/manifest.json`, static AI integration checks, and `tools/*.ps1` validation commands.

---

## Implementation Notes

The final API intentionally tightens the initial sketch: `register_resource` returns `RhiResourceRegistrationResult` so invalid registrations carry diagnostics, resource lifetime event rows use `RhiResourceLifetimeEventKind` instead of free-form strings, and `RhiResourceKind` distinguishes descriptor set layouts, descriptor sets, pipeline layouts, and graphics pipelines. Retired resources are removed from `records()` and preserved through `retire` event rows; there is no tombstone state in this foundation slice.

## Goal

Create the renderer/RHI resource lifetime foundation after Asset Identity v2 and Runtime Resource v2:

- stable first-party resource ids for backend-neutral RHI resources
- explicit resource kind, owner label, debug name, generation, and live/deferred/retired state
- deterministic diagnostics for invalid ids, duplicate registrations, stale releases, and duplicate releases
- frame-indexed deferred release retirement that is testable without GPU handles
- marker-style resource lifetime event rows for future diagnostics and editor panels

## Context

- `MK_rhi` already exposes backend-neutral resource contracts and private backend implementations keep native handles hidden.
- `MK_runtime_rhi` and `MK_runtime_scene_rhi` upload cooked payloads into RHI resources but resource lifetime, debug names, residency, and markers are not yet centralized.
- `Runtime Resource v2` added generation-checked cooked-package resource handles in `MK_runtime`; this plan applies the same honesty to renderer/RHI resource ownership without coupling runtime to GPU backends.

## Constraints

- Keep gameplay-facing APIs free of SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, native GPU handles, and backend handles.
- Keep `engine/core` unchanged.
- Do not add third-party dependencies.
- Do not migrate backend implementation ownership in this slice unless a focused test requires a narrow value bridge.
- Keep future 2D/3D playable recipes `planned`.

## Done When

- `MK_rhi` exposes a backend-neutral resource lifetime registry value API.
- Focused tests prove registration, duplicate rejection, debug name updates, stale/duplicate release diagnostics, deferred retirement by frame index, and marker event rows.
- Manifest/docs/checks mark Renderer/RHI Resource Foundation v1 as implemented foundation-only, not package streaming, upload/staging, frame graph, GPU allocator, or 2D/3D readiness.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## File Structure

- Create `engine/rhi/include/mirakana/rhi/resource_lifetime.hpp`: public value types and function/class declarations.
- Create `engine/rhi/src/resource_lifetime.cpp`: registry implementation and diagnostics.
- Modify `engine/rhi/CMakeLists.txt`: add `src/resource_lifetime.cpp`.
- Create `tests/unit/rhi_resource_lifetime_tests.cpp`: focused tests.
- Modify root `CMakeLists.txt`: register `MK_rhi_resource_lifetime_tests`.
- Modify `engine/agent/manifest.json`: update `MK_rhi`, `aiOperableProductionLoop.unsupportedProductionGaps.renderer-rhi-resource-foundation`, and recommended next plan.
- Modify `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1`: enforce foundation-only claims.
- Modify `docs/architecture.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/specs/generated-game-validation-scenarios.md`, `docs/specs/game-prompt-pack.md`, this plan, and the plan registry.

## Implementation Tasks

### Task 1: RED Tests For Resource Lifetime Registry

**Files:**
- Create: `tests/unit/rhi_resource_lifetime_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add failing tests for resource registration, debug names, and deferred release**

Add:

```cpp
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/resource_lifetime.hpp"

MK_TEST("rhi resource lifetime registry assigns generations and debug names") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;

    const auto texture = registry.register_resource(
        mirakana::rhi::RhiResourceRegistrationDesc{
            mirakana::rhi::RhiResourceKind::texture,
            "runtime-scene",
            "player-albedo",
        });

    MK_REQUIRE(texture.id.value == 1);
    MK_REQUIRE(texture.generation == 1);
    MK_REQUIRE(registry.records().size() == 1);
    MK_REQUIRE(registry.records()[0].debug_name == "player-albedo");

    const auto rename_result = registry.set_debug_name(texture, "player-albedo-streamed");
    MK_REQUIRE(rename_result.succeeded());
    MK_REQUIRE(registry.records()[0].debug_name == "player-albedo-streamed");
}

MK_TEST("rhi resource lifetime registry defers release until retire frame") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;
    const auto buffer = registry.register_resource(
        mirakana::rhi::RhiResourceRegistrationDesc{
            mirakana::rhi::RhiResourceKind::buffer,
            "upload",
            "mesh-vertices",
        });

    const auto release = registry.release_resource_deferred(buffer, 12);
    MK_REQUIRE(release.succeeded());
    MK_REQUIRE(!registry.is_live(buffer));
    MK_REQUIRE(registry.records().size() == 1);

    MK_REQUIRE(registry.retire_released_resources(11) == 0);
    MK_REQUIRE(registry.records().size() == 1);
    MK_REQUIRE(registry.retire_released_resources(12) == 1);
    MK_REQUIRE(registry.records().empty());
}

int main() {
    return mirakana::test::run_all();
}
```

- [x] **Step 2: Register the focused test target**

Add inside root `CMakeLists.txt` `if(BUILD_TESTING)`:

```cmake
add_executable(MK_rhi_resource_lifetime_tests
    tests/unit/rhi_resource_lifetime_tests.cpp
)
target_link_libraries(MK_rhi_resource_lifetime_tests PRIVATE MK_rhi)
target_include_directories(MK_rhi_resource_lifetime_tests PRIVATE tests)
MK_apply_common_target_options(MK_rhi_resource_lifetime_tests)
add_test(NAME MK_rhi_resource_lifetime_tests COMMAND MK_rhi_resource_lifetime_tests)
```

- [x] **Step 3: Verify RED**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_resource_lifetime_tests
```

Expected: compile failure because `mirakana/rhi/resource_lifetime.hpp` does not exist.

### Task 2: Resource Lifetime Public API And Implementation

**Files:**
- Create: `engine/rhi/include/mirakana/rhi/resource_lifetime.hpp`
- Create: `engine/rhi/src/resource_lifetime.cpp`
- Modify: `engine/rhi/CMakeLists.txt`

- [x] **Step 1: Add public value API**

Add:

```cpp
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::rhi {

struct RhiResourceId {
    std::uint64_t value{0};

    friend bool operator==(RhiResourceId lhs, RhiResourceId rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

enum class RhiResourceKind { unknown, buffer, texture, sampler, shader, descriptor_set, pipeline, swapchain };
enum class RhiResourceLifetimeState { live, deferred_release };
enum class RhiResourceLifetimeDiagnosticCode {
    invalid_resource,
    duplicate_release,
    stale_generation,
    invalid_debug_name,
    invalid_registration,
};

struct RhiResourceHandle {
    RhiResourceId id;
    std::uint32_t generation{0};

    friend bool operator==(RhiResourceHandle lhs, RhiResourceHandle rhs) noexcept {
        return lhs.id == rhs.id && lhs.generation == rhs.generation;
    }
};

struct RhiResourceRegistrationDesc {
    RhiResourceKind kind{RhiResourceKind::unknown};
    std::string owner;
    std::string debug_name;
};

struct RhiResourceLifetimeDiagnostic {
    RhiResourceLifetimeDiagnosticCode code{RhiResourceLifetimeDiagnosticCode::invalid_resource};
    RhiResourceHandle handle;
    std::string message;
};

struct RhiResourceLifetimeResult {
    std::vector<RhiResourceLifetimeDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RhiResourceLifetimeRecord {
    RhiResourceHandle handle;
    RhiResourceKind kind{RhiResourceKind::unknown};
    RhiResourceLifetimeState state{RhiResourceLifetimeState::live};
    std::string owner;
    std::string debug_name;
    std::uint64_t release_frame{0};
};

struct RhiResourceLifetimeEvent {
    std::string event;
    RhiResourceHandle handle;
    RhiResourceKind kind{RhiResourceKind::unknown};
    std::string owner;
    std::string debug_name;
    std::uint64_t frame{0};
};

class RhiResourceLifetimeRegistry {
  public:
    [[nodiscard]] RhiResourceHandle register_resource(RhiResourceRegistrationDesc desc);
    [[nodiscard]] RhiResourceLifetimeResult set_debug_name(RhiResourceHandle handle, std::string debug_name);
    [[nodiscard]] RhiResourceLifetimeResult release_resource_deferred(RhiResourceHandle handle,
                                                                      std::uint64_t release_frame);
    [[nodiscard]] std::uint32_t retire_released_resources(std::uint64_t completed_frame);
    [[nodiscard]] bool is_live(RhiResourceHandle handle) const noexcept;
    [[nodiscard]] const std::vector<RhiResourceLifetimeRecord>& records() const noexcept;
    [[nodiscard]] const std::vector<RhiResourceLifetimeEvent>& events() const noexcept;

  private:
    std::vector<RhiResourceLifetimeRecord> records_;
    std::vector<RhiResourceLifetimeEvent> events_;
    std::uint64_t next_id_{1};
};

} // namespace mirakana::rhi
```

- [x] **Step 2: Implement deterministic registry behavior**

Rules:

- registration rejects `unknown` kind or empty owner by returning `{0,0}` and emitting an `invalid_registration` event row
- ids are monotonic and one-based
- generation starts at `1`
- debug names must not contain ASCII control characters
- release rejects unknown id, stale generation, or already deferred records with diagnostics
- deferred release marks the record non-live immediately but keeps it until `retire_released_resources(completed_frame)` sees `release_frame <= completed_frame`
- event rows are appended for `register`, `rename`, `defer_release`, `retire`, and invalid operations

- [x] **Step 3: Verify GREEN**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_resource_lifetime_tests
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_rhi_resource_lifetime_tests
```

Expected: focused RHI resource lifetime tests pass.

### Task 3: Diagnostics And Stale Handle Tests

**Files:**
- Modify: `tests/unit/rhi_resource_lifetime_tests.cpp`
- Modify: `engine/rhi/src/resource_lifetime.cpp`

- [x] **Step 1: Add failing diagnostics tests**

Append tests for duplicate release, stale generation, invalid debug names, and event rows:

```cpp
MK_TEST("rhi resource lifetime registry diagnoses duplicate and stale releases") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;
    const auto texture = registry.register_resource(
        mirakana::rhi::RhiResourceRegistrationDesc{mirakana::rhi::RhiResourceKind::texture, "renderer", "shadow-map"});

    MK_REQUIRE(registry.release_resource_deferred(texture, 4).succeeded());

    const auto duplicate = registry.release_resource_deferred(texture, 5);
    MK_REQUIRE(!duplicate.succeeded());
    MK_REQUIRE(duplicate.diagnostics[0].code == mirakana::rhi::RhiResourceLifetimeDiagnosticCode::duplicate_release);

    const auto stale = registry.set_debug_name(
        mirakana::rhi::RhiResourceHandle{texture.id, texture.generation + 1U},
        "stale-name");
    MK_REQUIRE(!stale.succeeded());
    MK_REQUIRE(stale.diagnostics[0].code == mirakana::rhi::RhiResourceLifetimeDiagnosticCode::stale_generation);
}

MK_TEST("rhi resource lifetime registry records marker-style event rows") {
    mirakana::rhi::RhiResourceLifetimeRegistry registry;
    const auto pipeline = registry.register_resource(
        mirakana::rhi::RhiResourceRegistrationDesc{mirakana::rhi::RhiResourceKind::pipeline, "scene", "lit-pipeline"});
    MK_REQUIRE(registry.set_debug_name(pipeline, "lit-pipeline-v2").succeeded());
    MK_REQUIRE(registry.release_resource_deferred(pipeline, 8).succeeded());
    MK_REQUIRE(registry.retire_released_resources(8) == 1);

    MK_REQUIRE(registry.events().size() == 4);
    MK_REQUIRE(registry.events()[0].event == "register");
    MK_REQUIRE(registry.events()[1].event == "rename");
    MK_REQUIRE(registry.events()[2].event == "defer_release");
    MK_REQUIRE(registry.events()[3].event == "retire");
}
```

- [x] **Step 2: Verify RED**

Run the focused build and CTest command from Task 2.

Expected: tests fail until duplicate/stale diagnostics and event rows are implemented.

- [x] **Step 3: Implement missing diagnostics**

Add the minimum code needed so duplicate release, stale generation, invalid debug names, and event rows behave as specified.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_resource_lifetime_tests
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_rhi_resource_lifetime_tests
```

Expected: all focused resource lifetime tests pass.

### Task 4: Manifest, Docs, And Static Checks

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

- `MK_rhi.status` to a Renderer/RHI Resource Foundation v1 status.
- `MK_rhi.publicHeaders` includes `engine/rhi/include/mirakana/rhi/resource_lifetime.hpp`.
- `aiOperableProductionLoop.unsupportedProductionGaps.renderer-rhi-resource-foundation.status` to `implemented-foundation-only`.

Keep `frame-graph-v1`, `upload-staging-v1`, `future-2d-playable-vertical-slice`, and `future-3d-playable-vertical-slice` as `planned`.

- [x] **Step 2: Strengthen checks**

Make checks fail when:

- `MK_rhi` omits `resource_lifetime.hpp`
- Renderer/RHI Resource Foundation is marked `ready` instead of `implemented-foundation-only`
- docs claim GPU allocator, package streaming, upload/staging, frame graph, or 2D/3D vertical-slice readiness from this foundation
- future 2D/3D recipes stop being `planned`

- [x] **Step 3: Update docs**

Document this as foundation-only:

- Resource lifetime registry gives ids, debug names, deferred-release state, and marker-style event rows.
- Native backend handles remain private.
- GPU memory allocation, residency budgets, backend migration, upload/staging, frame graph, and 2D/3D vertical slices remain follow-up work.

### Task 5: Validation And Closure

**Files:**
- Modify: `docs/superpowers/plans/2026-05-01-renderer-rhi-resource-foundation-v1.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] **Step 1: Run focused validation**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_resource_lifetime_tests
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_rhi_resource_lifetime_tests
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

Record command results here while implementing this plan.

- RED: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --preset dev; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_resource_lifetime_tests` failed as expected because `mirakana/rhi/resource_lifetime.hpp` did not exist.
- RED: after tightening the planned API to `RhiResourceRegistrationResult` plus `RhiResourceLifetimeEventKind`, `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_resource_lifetime_tests` failed as expected because the implementation still returned `RhiResourceHandle` and event rows still used strings.
- GREEN: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_resource_lifetime_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_rhi_resource_lifetime_tests` passed with 1/1 focused test target.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` initially failed because the new renderer/RHI foundation gap check required `RhiResourceLifetimeRegistry` in manifest notes; after updating the manifest, it passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and emitted the updated active plan plus renderer/RHI foundation-only gap status.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` reported D3D12 DXIL ready, Vulkan SPIR-V ready, DXC SPIR-V CodeGen ready, and Metal `metal`/`metallib` missing as diagnostic-only blockers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed: license, agent config, AI integration, JSON contracts, dependency policy, C++ standard policy, shader toolchain diagnostic gate, mobile diagnostic gate, public API boundary, tidy config diagnostic gate, CMake configure/build, generated MSVC C++23 mode check, and CTest 25/25 tests passed.
