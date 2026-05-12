# Frame Graph RHI Texture Barrier Recording v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Connect successful Frame Graph v1 barrier schedule rows to backend-neutral `IRhiCommandList::transition_texture` calls through explicit texture bindings.

**Plan ID:** `frame-graph-rhi-texture-barrier-recording-v1`

**Status:** Completed

**Architecture:** Keep the pure `mirakana/renderer/frame_graph.hpp` planner RHI-free. Add a narrow `mirakana/renderer/frame_graph_rhi.hpp` adapter that maps `FrameGraphAccess` to `mirakana::rhi::ResourceState`, validates caller-owned resource-name to texture-handle bindings, records barrier steps only, updates binding state after successful recording, and returns deterministic diagnostics without exposing native handles.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `NullRhiDevice`, `tests/unit/renderer_rhi_tests.cpp`.

---

## Context

- Active roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Target unsupported gap: `frame-graph-v1`, currently `implemented-foundation-only`.
- Existing v1 implementation emits `FrameGraphBarrier` rows and `FrameGraphExecutionStep` schedules, but renderer hosts still manually translate resource transitions.
- This slice closes only the host-independent adapter between schedule rows and public RHI command-list recording.

## Constraints

- Do not move RHI types into `engine/core`.
- Do not expose D3D12, Vulkan, Metal, SDL3, swapchain-frame, or native handles.
- Do not implement transient heaps, aliasing, multi-queue scheduling, pass callbacks, package streaming, or production render-graph ownership.
- Do not remove current renderer manual transitions in this slice; existing renderer behavior must remain stable.
- New behavior must have failing tests before implementation.

## Files

- Create: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Create: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify after green: `docs/superpowers/plans/README.md`
- Modify after green: `docs/current-capabilities.md`
- Modify after green: `docs/roadmap.md`
- Modify after green: `engine/agent/manifest.json`

## Tasks

### Task 1: RED tests for access-to-state mapping and command recording

**Files:**
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] **Step 1: Add failing tests**

Add tests near the existing Frame Graph v1 tests:

```cpp
MK_TEST("frame graph v1 maps texture barrier accesses to rhi resource states") {
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::color_attachment_write) ==
               mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::depth_attachment_write) ==
               mirakana::rhi::ResourceState::depth_write);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::shader_read) ==
               mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::copy_source) ==
               mirakana::rhi::ResourceState::copy_source);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::copy_destination) ==
               mirakana::rhi::ResourceState::copy_destination);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::present) ==
               mirakana::rhi::ResourceState::present);
    MK_REQUIRE(!mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::unknown).has_value());
}

MK_TEST("frame graph v1 records scheduled texture barriers through rhi command lists") {
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
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        mirakana::rhi::Extent3D{16, 16, 1},
        mirakana::rhi::Format::rgba8_unorm,
        mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined, mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        "scene-color",
        texture,
        mirakana::rhi::ResourceState::render_target,
    }};
    const auto result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, bindings);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 1);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(device.stats().resource_transitions == 2);
}

MK_TEST("frame graph v1 texture barrier recording diagnoses missing and stale bindings") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            "scene-color",
            "scene",
            "postprocess",
            mirakana::FrameGraphAccess::color_attachment_write,
            mirakana::FrameGraphAccess::shader_read,
        }),
    };

    mirakana::rhi::NullRhiDevice device;
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    std::vector<mirakana::FrameGraphTextureBinding> no_bindings;
    const auto missing = mirakana::record_frame_graph_texture_barriers(*commands, schedule, no_bindings);
    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.diagnostics.size() == 1);
    MK_REQUIRE(missing.diagnostics[0].resource == "scene-color");

    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        mirakana::rhi::Extent3D{8, 8, 1},
        mirakana::rhi::Format::rgba8_unorm,
        mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    std::vector<mirakana::FrameGraphTextureBinding> stale{mirakana::FrameGraphTextureBinding{
        "scene-color",
        texture,
        mirakana::rhi::ResourceState::undefined,
    }};
    const auto stale_result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, stale);
    MK_REQUIRE(!stale_result.succeeded());
    MK_REQUIRE(stale_result.diagnostics.size() == 1);
    MK_REQUIRE(stale_result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(stale[0].current_state == mirakana::rhi::ResourceState::undefined);
}
```

- [x] **Step 2: Run the focused test and confirm RED**

Run:

```powershell
cmake --build --preset dev --target MK_renderer_tests
ctest --preset dev --output-on-failure -R "^MK_renderer_tests$"
```

Expected: compile failure because `frame_graph_rhi.hpp`, `FrameGraphTextureBinding`, `frame_graph_texture_state_for_access`, and `record_frame_graph_texture_barriers` do not exist yet.

### Task 2: GREEN implementation

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Create: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] **Step 1: Add the public adapter header**

Define:

```cpp
struct FrameGraphTextureBinding {
    std::string resource;
    rhi::TextureHandle texture;
    rhi::ResourceState current_state{rhi::ResourceState::undefined};
};

struct FrameGraphTextureBarrierRecordResult {
    std::size_t barriers_recorded{0};
    std::vector<FrameGraphDiagnostic> diagnostics;
    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::optional<rhi::ResourceState>
frame_graph_texture_state_for_access(FrameGraphAccess access) noexcept;

[[nodiscard]] FrameGraphTextureBarrierRecordResult
record_frame_graph_texture_barriers(rhi::IRhiCommandList& commands,
                                    std::span<const FrameGraphExecutionStep> schedule,
                                    std::span<FrameGraphTextureBinding> texture_bindings);
```

- [x] **Step 2: Implement deterministic validation and recording**

Implementation rules:
- Reject closed command lists before recording.
- Reject empty binding resource names.
- Reject duplicate binding resource names.
- Reject missing bindings for barrier resources.
- Reject unsupported `FrameGraphAccess::unknown`.
- Reject stale binding state when `binding.current_state != mapped_before_state`.
- Call `commands.transition_texture(binding.texture, before, after)` only after validation for that barrier passes.
- On success, set `binding.current_state = after` and increment `barriers_recorded`.
- Convert thrown `std::exception` to a deterministic `FrameGraphDiagnostic` with the barrier pass/resource row.

- [x] **Step 3: Wire the source into `MK_renderer`**

Add `src/frame_graph_rhi.cpp` to `engine/renderer/CMakeLists.txt`.

- [x] **Step 4: Include the new header in tests**

Add:

```cpp
#include "mirakana/renderer/frame_graph_rhi.hpp"
```

- [x] **Step 5: Run focused renderer tests and confirm GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_renderer_tests
ctest --preset dev --output-on-failure -R "^MK_renderer_tests$"
```

Expected: `MK_renderer_tests` passes.

### Task 3: Documentation, manifest, and validation closeout

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/2026-05-08-frame-graph-rhi-texture-barrier-recording-v1.md`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Update capability text**

Record that Frame Graph v1 now has a host-independent RHI texture barrier recording adapter for explicit bindings. Keep these unsupported:
- production render graph ownership
- pass execution callbacks
- transient heap allocation
- aliasing
- multi-queue/general scheduling
- renderer-wide migration away from manual transitions
- package streaming integration

- [x] **Step 2: Update active/latest plan pointers**

After validation, change:
- plan registry active slice back to the master plan
- plan registry latest completed slice to this plan
- manifest `currentActivePlan` back to the master plan
- manifest `recommendedNextPlan.id` back to `next-production-gap-selection`

- [x] **Step 3: Run focused static checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/frame_graph_rhi.cpp,tests/unit/renderer_rhi_tests.cpp -MaxFiles 2
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

- [x] **Step 4: Run final repository validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

## Done When

- [x] Texture state mapping is tested for every current `FrameGraphAccess`.
- [x] Scheduled barrier rows can record one texture transition through `IRhiCommandList::transition_texture`.
- [x] Missing/stale binding rows produce deterministic diagnostics and do not mutate binding state.
- [x] The pure Frame Graph planner remains independent from RHI through a separate adapter header/source.
- [x] Docs and manifest describe the new capability without claiming full production render graph readiness.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass, or exact blockers are recorded.

## Validation Evidence

| Command | Result | Date |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_renderer_tests` | PASS | 2026-05-08 |
| `ctest --preset dev --output-on-failure -R "^MK_renderer_tests$"` | PASS | 2026-05-08 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | 2026-05-08 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/frame_graph_rhi.cpp,tests/unit/renderer_rhi_tests.cpp -MaxFiles 2` | PASS | 2026-05-08 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | 2026-05-08 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | 2026-05-08 |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | 2026-05-08 |
