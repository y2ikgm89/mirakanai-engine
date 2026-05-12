# Frame Graph and Upload/Staging Foundation v1 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the next backend-neutral planning layer for resource-declared frame graph execution and upload/staging retirement without exposing native GPU handles or claiming production renderer readiness.

**Architecture:** Extend existing `MK_renderer` Frame Graph v0 value contracts into a narrow Frame Graph v1 plan that records resource usages, barrier intent, pass execution order, and transient/imported resource policy. Add a separate `MK_rhi` upload/staging value contract for upload batches, staging allocations, copy requests, and fence-based retirement; do not wire native D3D12/Vulkan/Metal resource destruction or package streaming in this slice.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, repository unit test framework, CMake target registration, `engine/agent/manifest.json`, static AI integration checks, and `tools/*.ps1` validation commands.

---

## Goal

Move the production renderer roadmap one step past Renderer/RHI Resource Foundation v1:

- frame graph pass/resource declarations include access intent and deterministic barrier rows
- imported versus transient resources are explicit
- upload/staging plans can batch buffer/texture copies and retire staged allocations by fence value
- diagnostics remain deterministic and backend-neutral
- existing postprocess/shadow smoke paths stay honest as Frame Graph/Postprocess v0 users until separately migrated

## Context

- `MK_renderer` already provides `compile_frame_graph_v0` for ordered pass/resource diagnostics.
- `MK_rhi` now provides `RhiResourceLifetimeRegistry` as a foundation-only lifetime registry.
- `NullRhiDevice`, D3D12, Vulkan, and Metal backend internals should not be migrated in this slice.

## Constraints

- Keep gameplay-facing APIs free of SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, native GPU handles, and backend handles.
- Keep `engine/core` unchanged.
- Do not add third-party dependencies.
- Keep future 2D/3D playable recipes `planned`.
- Do not claim GPU allocator, residency budgets, package streaming, native async uploads, or production render graph scheduling until their own validation lands.

## Done When

- `MK_renderer` exposes a backend-neutral Frame Graph v1 planning API for resource access, pass ordering, and barrier intent.
- `MK_rhi` exposes a backend-neutral upload/staging planning API for upload batches and fence retirement.
- Focused tests prove valid ordering/barriers, invalid access diagnostics, transient/imported resource rules, batched upload copy rows, and retirement by completed fence.
- Manifest/docs/checks mark Frame Graph and Upload/Staging Foundation v1 as foundation-only, not production renderer, package streaming, GPU allocator, or 2D/3D readiness.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## File Structure

- Modify `engine/renderer/include/mirakana/renderer/frame_graph.hpp`: add Frame Graph v1 value types and planner declaration.
- Modify `engine/renderer/src/frame_graph.cpp`: implement v1 planning beside v0.
- Create `engine/rhi/include/mirakana/rhi/upload_staging.hpp`: public upload/staging value types and planner declaration.
- Create `engine/rhi/src/upload_staging.cpp`: upload/staging validation, batching, and retirement.
- Modify `engine/rhi/CMakeLists.txt`: add `src/upload_staging.cpp`.
- Modify `tests/unit/renderer_rhi_tests.cpp`: add focused Frame Graph v1 planning tests.
- Create `tests/unit/rhi_upload_staging_tests.cpp`: focused upload/staging tests.
- Modify root `CMakeLists.txt`: register `MK_rhi_upload_staging_tests`.
- Modify `engine/agent/manifest.json`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `docs/rhi.md`, `docs/architecture.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/specs/generated-game-validation-scenarios.md`, `docs/specs/game-prompt-pack.md`, this plan, and the plan registry.

## Implementation Tasks

### Task 1: RED Tests For Frame Graph v1 Barrier Planning

**Files:**
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] **Step 1: Add failing tests for resource access and barriers**

Append tests that use the desired API:

```cpp
MK_TEST("frame graph v1 plans deterministic barriers between writer and reader") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{"scene-color", mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        "scene",
        {},
        {mirakana::FrameGraphResourceAccess{"scene-color", mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        "postprocess",
        {mirakana::FrameGraphResourceAccess{"scene-color", mirakana::FrameGraphAccess::shader_read}},
        {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.ordered_passes.size() == 2);
    MK_REQUIRE(plan.ordered_passes[0] == "scene");
    MK_REQUIRE(plan.ordered_passes[1] == "postprocess");
    MK_REQUIRE(plan.barriers.size() == 1);
    MK_REQUIRE(plan.barriers[0].resource == "scene-color");
    MK_REQUIRE(plan.barriers[0].from == mirakana::FrameGraphAccess::color_attachment_write);
    MK_REQUIRE(plan.barriers[0].to == mirakana::FrameGraphAccess::shader_read);
}
```

- [x] **Step 2: Verify RED**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests
```

Expected: compile failure because `FrameGraphV1Desc` and `compile_frame_graph_v1` do not exist.

### Task 2: Frame Graph v1 Value API And Planner

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/frame_graph.hpp`
- Modify: `engine/renderer/src/frame_graph.cpp`

- [x] **Step 1: Add public v1 value types**

Add these types beside the v0 declarations:

```cpp
enum class FrameGraphResourceLifetime { imported, transient };
enum class FrameGraphAccess { unknown, color_attachment_write, depth_attachment_write, shader_read, copy_source, copy_destination, present };

struct FrameGraphResourceV1Desc {
    std::string name;
    FrameGraphResourceLifetime lifetime{FrameGraphResourceLifetime::transient};
};

struct FrameGraphResourceAccess {
    std::string resource;
    FrameGraphAccess access{FrameGraphAccess::unknown};
};

struct FrameGraphPassV1Desc {
    std::string name;
    std::vector<FrameGraphResourceAccess> reads;
    std::vector<FrameGraphResourceAccess> writes;
};

struct FrameGraphV1Desc {
    std::vector<FrameGraphResourceV1Desc> resources;
    std::vector<FrameGraphPassV1Desc> passes;
};

struct FrameGraphBarrier {
    std::string resource;
    std::string from_pass;
    std::string to_pass;
    FrameGraphAccess from{FrameGraphAccess::unknown};
    FrameGraphAccess to{FrameGraphAccess::unknown};
};

struct FrameGraphV1BuildResult {
    std::vector<std::string> ordered_passes;
    std::vector<FrameGraphBarrier> barriers;
    std::vector<FrameGraphDiagnostic> diagnostics;
    std::size_t pass_count{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] FrameGraphV1BuildResult compile_frame_graph_v1(const FrameGraphV1Desc& desc);
```

- [x] **Step 2: Implement minimal planner**

Implement deterministic validation by reusing the v0 ordering model:

- reject empty resource/pass names with existing `FrameGraphDiagnosticCode::invalid_resource` / `invalid_pass`
- reject `FrameGraphAccess::unknown`
- imported resources may be read with no producer
- transient resources must have a producer before any reader
- multiple writers for one resource are `write_write_hazard`
- same-pass read/write remains `read_write_hazard`
- produce one `FrameGraphBarrier` when a resource writer pass is ordered before a later reader pass with a different access

- [x] **Step 3: Verify GREEN**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_renderer_tests
```

Expected: renderer tests pass.

### Task 3: RED Tests For Upload/Staging Foundation

**Files:**
- Create: `tests/unit/rhi_upload_staging_tests.cpp`
- Modify: root `CMakeLists.txt`

- [x] **Step 1: Add failing upload/staging tests**

Create tests for the desired API:

```cpp
MK_TEST("rhi upload staging plan batches copies and retires by fence") {
    mirakana::rhi::RhiUploadStagingPlan plan;
    const auto allocation = plan.allocate_staging_bytes(mirakana::rhi::RhiUploadAllocationDesc{128, "mesh-upload"});
    MK_REQUIRE(allocation.succeeded());
    MK_REQUIRE(allocation.handle.id.value == 1);

    const auto copy = plan.enqueue_buffer_upload(mirakana::rhi::RhiBufferUploadDesc{allocation.handle, 0, 64, "mesh-vertices"});
    MK_REQUIRE(copy.succeeded());
    MK_REQUIRE(plan.pending_copies().size() == 1);

    MK_REQUIRE(plan.retire_completed_uploads(mirakana::rhi::FenceValue{4}) == 0);
    MK_REQUIRE(plan.mark_submitted(allocation.handle, mirakana::rhi::FenceValue{5}).succeeded());
    MK_REQUIRE(plan.retire_completed_uploads(mirakana::rhi::FenceValue{5}) == 1);
}
```

- [x] **Step 2: Register the focused test target**

Add inside root `CMakeLists.txt` `if(BUILD_TESTING)`:

```cmake
add_executable(MK_rhi_upload_staging_tests
    tests/unit/rhi_upload_staging_tests.cpp
)
target_link_libraries(MK_rhi_upload_staging_tests PRIVATE MK_rhi)
target_include_directories(MK_rhi_upload_staging_tests PRIVATE tests)
MK_apply_common_target_options(MK_rhi_upload_staging_tests)
add_test(NAME MK_rhi_upload_staging_tests COMMAND MK_rhi_upload_staging_tests)
```

- [x] **Step 3: Verify RED**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_upload_staging_tests
```

Expected: compile failure because `mirakana/rhi/upload_staging.hpp` does not exist.

### Task 4: Upload/Staging Public API And Implementation

**Files:**
- Create: `engine/rhi/include/mirakana/rhi/upload_staging.hpp`
- Create: `engine/rhi/src/upload_staging.cpp`
- Modify: `engine/rhi/CMakeLists.txt`

- [x] **Step 1: Add public value API**

Define `RhiUploadStagingPlan`, `RhiUploadAllocationHandle`, `RhiUploadAllocationDesc`, `RhiBufferUploadDesc`, `RhiUploadCopyRecord`, diagnostics, and result types. Use first-party `FenceValue` from `mirakana/rhi/rhi.hpp` for submitted/retired fence values.

- [x] **Step 2: Implement deterministic behavior**

Rules:

- staging allocation ids are one-based and monotonic
- allocation size must be greater than zero
- upload copy range must fit within the staging allocation
- copies may be queued before submission
- `mark_submitted` stores the fence value on an allocation
- `retire_completed_uploads(completed_fence)` removes submitted allocations with `submitted_fence.value <= completed_fence.value`
- pending copies referring to retired allocations are removed during retirement

- [x] **Step 3: Verify GREEN**

Run:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_upload_staging_tests
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_rhi_upload_staging_tests
```

Expected: upload/staging tests pass.

### Task 5: Manifest, Docs, Checks, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/rhi.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/superpowers/plans/2026-05-01-frame-graph-upload-staging-foundation-v1.md`
- Modify: `docs/superpowers/plans/README.md`

- [x] **Step 1: Update machine-readable truth**

Set `frame-graph-v1` and `upload-staging-v1` gaps to `implemented-foundation-only` only after focused tests pass. Keep future 2D/3D recipes `planned`.

- [x] **Step 2: Strengthen static checks**

Make checks fail when Frame Graph/Upload/Staging is marked `ready`, when docs claim production renderer/GPU allocator/package streaming/2D/3D readiness, or when the new public headers are omitted from manifest module public headers.

- [x] **Step 3: Run validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 4: Record evidence and advance the registry**

Append command results to this plan's Validation Evidence section and move this plan from active to completed in `docs/superpowers/plans/README.md`. Create the next dated focused plan before continuing.

## Validation Evidence

Record command results here while implementing this plan.

- 2026-05-01 RED: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests` failed as expected after adding Frame Graph v1 tests. MSVC reported `FrameGraphV1Desc`, `FrameGraphResourceV1Desc`, `FrameGraphResourceLifetime`, `FrameGraphResourceAccess`, `FrameGraphAccess`, and `compile_frame_graph_v1` are not members of `ge`.
- 2026-05-01 RED: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_upload_staging_tests` failed as expected after adding upload/staging tests. MSVC reported `mirakana/rhi/upload_staging.hpp` could not be opened because the header does not exist yet.
- 2026-05-01 GREEN: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_renderer_tests` passed after adding Frame Graph v1 value types and planner. `Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_renderer_tests` passed 1/1.
- 2026-05-01 GREEN: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_rhi_upload_staging_tests; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_rhi_upload_staging_tests` passed 1/1 after adding `RhiUploadStagingPlan`, buffer/texture copy rows, and fence retirement.
- 2026-05-01 FINAL: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed with `public-api-boundary-check: ok`.
- 2026-05-01 FINAL: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` passed as diagnostic-only. D3D12 DXIL and Vulkan SPIR-V tooling were ready; Metal `metal`/`metallib` remained missing and recorded as host-gated blockers.
- 2026-05-01 FINAL: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reported `ai-command-surface-foundation-v1` as the next active/recommended plan.
- 2026-05-01 FINAL: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed with `json-contract-check: ok`.
- 2026-05-01 FINAL: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed with `ai-integration-check: ok` after the docs/check guards were synchronized for the new foundation-only `frame-graph-v1` and `upload-staging-v1` claims.
- 2026-05-01 FINAL: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok` and CTest reported 26/26 tests passing, including `MK_renderer_tests` and `MK_rhi_upload_staging_tests`.
