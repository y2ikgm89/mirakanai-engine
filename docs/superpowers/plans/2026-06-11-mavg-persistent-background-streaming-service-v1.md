# MAVG Persistent Background Streaming Service v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add MAVG Persistent Background Streaming Service v1 (`mavg-persistent-background-streaming-service-v1`), a narrow caller-owned persistent MAVG background streaming service tick that retains reviewed page requests across frames and dispatches bounded background package loads through the existing first-party job pool.

**Architecture:** Keep the service in `MK_runtime` beside `mavg_page_streaming.hpp`; it owns only value state supplied by the caller and delegates actual package candidate file IO to MAVG Background Streaming Dispatch v1 (`mavg-background-streaming-dispatch-v1`) through `dispatch_runtime_mavg_page_streaming_background_loads`. The service must not mutate resident mounts/catalogs, execute DirectStorage, upload GPU resources, touch renderer/RHI handles, claim autonomous scheduling/backpressure/cancellation, claim GPU memory pressure policy execution, or claim async-overlap/performance proof.

**Tech Stack:** C++23, `MK_runtime`, `JobExecutionPool`, `MemoryFileSystem`, existing `MK_runtime_mavg_page_streaming_tests`.

**Plan ID:** `mavg-persistent-background-streaming-service-v1`

---

### Task 1: Persistent Service Tick Contract

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Modify: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`

- [x] **Step 1: Write the failing service tests**

Add tests that prove:
- a service tick accepts reviewed rows, dispatches a bounded subset, and keeps remaining rows pending for a later tick;
- duplicate reviewed rows already pending are coalesced deterministically without double dispatch;
- invalid graph rows fail closed without dispatching or mutating pending state;
- idle ticks with no pending rows and no new rows succeed without invoking background workers.

- [x] **Step 2: Run RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
```

Expected: compile failure for the new service state, descriptor, result, and tick function.

- [x] **Step 3: Implement the service API**

Add:
- `RuntimeMavgPageStreamingBackgroundServiceState`
- `RuntimeMavgPageStreamingBackgroundServiceTickDesc`
- `RuntimeMavgPageStreamingBackgroundServiceTickResult`
- `tick_runtime_mavg_page_streaming_background_service`

The tick must validate graph ownership before mutating state, append non-duplicate reviewed rows up to `max_pending_pages`, sort pending rows deterministically by priority/page, dispatch at most `max_dispatch_pages`, remove completed dispatched rows from pending, and propagate existing background load diagnostics.

- [x] **Step 4: Verify focused behavior**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_page_streaming_tests"
```

Evidence: `MK_runtime_mavg_page_streaming_tests` built successfully and `ctest -R "MK_runtime_mavg_page_streaming_tests"` passed on 2026-06-11.

### Task 2: Durable Surfaces And Publication

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-115-mavg-payload-byte-range-page-loader.ps1`

- [x] **Step 1: Sync docs and manifest fragments**

Record the narrow service claim and non-claims. Keep `currentActivePlan` at the production-completion master plan and `unsupportedProductionGaps = []`.

- [x] **Step 2: Compose and run focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp"
```

Evidence: `compose-agent-manifest.ps1 -Write`, `check-ai-integration.ps1`, `check-json-contracts.ps1`, `check-public-api-boundaries.ps1`, and targeted `check-tidy.ps1 -Files "engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp"` passed on 2026-06-11.

- [ ] **Step 3: Close validation and publish**

Run full validation, publication preflight, commit, push, and open a draft PR:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Evidence before publication: full `tools/validate.ps1` passed with `validate: ok` on 2026-06-11 after code, docs, manifest composition, static guards, and formatting were settled.
