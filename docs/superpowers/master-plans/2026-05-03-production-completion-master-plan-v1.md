# Production Completion Master Plan v1 (2026-05-03)

Plan ID: `production-completion-master-plan-v1`
Status: Active lightweight index.
Detailed corpus: [production-completion-v1/01-one-dot-zero-readiness-ledger.md](production-completion-v1/01-one-dot-zero-readiness-ledger.md), [04-developer-owned-engine-capability-backlog.md](production-completion-v1/04-developer-owned-engine-capability-backlog.md), [05-projections-and-scenarios.md](production-completion-v1/05-projections-and-scenarios.md), and [99-historical-verdict-archive.md](production-completion-v1/99-historical-verdict-archive.md).
Active execution pointer: `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Purpose

This file is the master plan index. It intentionally stays small so Codex, Claude Code, Cursor, and other agents can find the right production-completion chapter without spending context on the full historical plan.

Use the chapter that matches the current decision. Do not bulk-read every chapter unless the task requires a full production readiness audit.

## Current Verdict

- 1.0 closeout readiness remains manifest-led: every `unsupportedProductionGaps` row in `engine/agent/manifest.json.aiOperableProductionLoop` must be implemented, host-gated with evidence, or explicitly excluded with evidence before a ready claim; the current composed manifest has no remaining rows.
- Current execution remains manifest-led. Do not hand-edit `engine/agent/manifest.json`; edit `engine/agent/manifest.fragments/*.json` and run the compose script when manifest state changes.
- Current active plan: `docs/superpowers/plans/2026-06-05-mavg-directstorage-request-plan-v1.md`. `recommendedNextPlan.id = mavg-directstorage-request-plan-v1`; `unsupportedProductionGaps = [].` Parent milestone: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`.
- Current manifest state: `unsupportedProductionGaps = []`; `recommendedNextPlan.id = mavg-directstorage-request-plan-v1`. Active child `mavg-directstorage-request-plan-v1` (`docs/superpowers/plans/2026-06-05-mavg-directstorage-request-plan-v1.md`) adds value-only DirectStorage request planning through `RuntimeMavgPayloadDirectStorageRequestPlanDesc`, `RuntimeMavgPayloadDirectStorageRequestRow`, `RuntimeMavgPayloadDirectStorageRequestPlanResult`, `RuntimeMavgPayloadDirectStorageFenceWaitPoint`, and `plan_runtime_mavg_payload_directstorage_requests` over selected validated `GameEngine.MavgClusterPayload.v1` page rows with `requires_native_directstorage_sdk=true`, `used_native_directstorage=false`, and no file IO, native enqueue, native submit, native fence signal, background worker, resident mount mutation, renderer/RHI handle access, or async-overlap/performance claim. Completed child `mavg-payload-byte-range-file-io-v1` (`docs/superpowers/plans/2026-06-05-mavg-payload-byte-range-file-io-v1.md`) adds first-party `IFileSystem` `read_bytes` / `write_bytes` / `read_byte_range` binary APIs, `MemoryFileSystem` / `RootedFileSystem` exact byte-range coverage, and `load_runtime_mavg_payload_file_pages` / `RuntimeMavgPayloadPageFileLoadResult` with `invoked_file_io=true` and `used_native_directstorage=false`. Completed child `mavg-page-addressable-payload-schema-v1` (`docs/superpowers/plans/2026-06-05-mavg-page-addressable-payload-schema-v1.md`) adds deterministic `GameEngine.MavgClusterPayload.v1` page-addressable payload validation through `mavg_cluster_payload.hpp`, `MavgClusterPayloadDocument`, `MavgClusterPayloadPage`, `MavgClusterPayloadCluster`, `validate_mavg_cluster_payload`, cooked `page.data_hex` logical page bytes, and no-IO selected page extraction through `mavg_payload_pages.hpp`, `RuntimeMavgPayloadPageSliceResult`, and `extract_runtime_mavg_payload_page_slices`. Completed child `mavg-package-streaming-residency-dispatch-v1` (`docs/superpowers/plans/2026-06-05-mavg-package-streaming-residency-dispatch-v1.md`) adds value-only MAVG package streaming residency dispatch rows through `RuntimeMavgPageStreamingDispatchPlan` and `plan_runtime_mavg_page_streaming_dispatches`, copying caller-owned `RuntimeMavgPageStreamingDrainDesc` values for safe-point/background queue handoff. Completed child `mavg-d3d12-gpu-culling-execution-v1` (`docs/superpowers/plans/2026-06-05-mavg-d3d12-gpu-culling-execution-v1.md`) / MAVG D3D12 GPU Culling Execution v1 adds a D3D12 WARP-backed MAVG GPU culling dispatch proof through PR #452 over `BufferUsage::storage | BufferUsage::indirect` indexed indirect buffers: `MavgGpuCullingIndirectPlan` supplies the CPU reference expectation, a compute shader consumes selected-cluster command rows plus visibility rows, compacts only the visible cluster into a same-buffer indexed indirect argument/count layout, executes it through `ExecuteIndirect`, verifies visible/culled pixels, and records compute dispatch / queue wait / GPU-generated indirect / UAV barrier / indirect transition stats. Native DirectStorage/Win32 async IO execution, autonomous background workers, async-overlap/performance, automatic eviction policy, GPU memory pressure integration, generic GPU culling frameworks, generic GPU-generated count-buffer systems beyond this D3D12 storage-indirect lane, generic storage-buffer state management, Vulkan indirect draw execution, Metal indirect command buffers, mesh shaders, Nanite compatibility/equivalence/superiority, and broad CPU/GPU/memory optimization remain unclaimed.
- Retained MAVG D3D12 indexed indirect draw prerequisites: `mavg-d3d12-indexed-indirect-draw-execution-v1` completed D3D12 `ExecuteIndirect` execution for CPU-generated upload indexed indirect argument buffers without count-buffer execution; `mavg-d3d12-indexed-indirect-count-buffer-execution-v1` completed CPU-generated upload count-buffer execution with zero-count and clamp evidence; `mavg-d3d12-compute-generated-indirect-execution-v1` completed D3D12 compute-generated indexed indirect argument/count buffer execution for default-heap `BufferUsage::storage | BufferUsage::indirect` buffers, explicit compute-to-graphics queue wait evidence, backend-private storage UAV transition/barrier tracking, transition to `D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT`, same-buffer argument/count offset execution, visible WARP-backed proof, fail-closed usage validation, and no CPU decode/stat overclaim for GPU-generated command bytes. Vulkan indirect draw execution, Metal indirect command buffers, broad GPU culling framework work, and Nanite compatibility/equivalence/superiority remain unclaimed.
- Retained historical/static-check closeout pointers: Completed gap burn-down, `recommendedNextPlan.id = next-production-gap-selection`, Renderer RHI Resource Foundation 1.0 Scope Closeout v1, Job Execution Placement Policy v1, Windows CPU Set Worker Placement v1, Native Win32 Editor Shell v1, and Renderer Backend Parity Metal Apple Evidence v1.
- Renderer Metal CI Host Recipe v1 completed through PR #386 / merge commit `12fde1bf7ea369e97a45d95a0ee271b4e2515b3d`; hosted `macOS Metal CMake` now runs the reviewed `renderer-metal-apple-host-evidence` recipe before the full macOS configure/build/CTest path, reaches `apple-host-evidence-check: ready`, passes `MK_backend_scaffold_tests` and `MK_renderer_quality_matrix_tests`, and ends with `renderer-metal-apple-check: ok` without promoting `metal-apple`, broad Metal parity, or broad renderer quality.
- Current gap cluster: no Engine 1.0 unsupported gap is open; the SDL3 removal milestone has closed with first-party Windows desktop runtime/editor/audio replacements, SDL3 removal from build, package, generated-game, docs, legal, and agent contract surfaces, and no macOS/Linux desktop parity claim from Windows evidence. The reviewed importer/codecs/shader-generation child also closed selected KTX2/Basis, glTF scene import, source image/audio codec, and shader generation/cache review rows while keeping broad import/codec/shader/runtime compiler/renderer residency/Metal claims fail-closed. Physics Navigation Commercial Coverage v1 closed optional adapter boundary, host validation recipe, and lifecycle evidence gates for physics/navigation breadth rows while keeping native handles, runtime bake/source mutation, and broad middleware parity fail-closed. Renderer Backend Parity Metal Apple Evidence v1 and Closeout v1 completed local/hosted Apple/Metal guardrails and returned the manifest to zero unsupported gaps. Renderer Postprocess Tone Mapping Evidence v1 completed a fail-closed D3D12/Vulkan/Metal tone-mapping proof contract without claiming broad postprocess execution or SDL3/native-handle readiness. Generic 2D Sandbox Production Lane v1 completed Phases 1-10 through PR #313 while keeping broad ready claims out of the manifest. First-Party Editor Shell v1 completed the dependency-free `desktop-editor` lane, first-party `mirakana::ui` / `MK_ui_renderer` shell contract, AI-operable editor rows, active Dear ImGui dependency removal, and inactive implementation deletion without compatibility aliases.

## Plan map

| Need | Read |
| --- | --- |
| 1.0 readiness ledger, Current Verdict, user-scenario mapping | [01-one-dot-zero-readiness-ledger.md](production-completion-v1/01-one-dot-zero-readiness-ledger.md) |
| Canonical Post-1.0 / 1.x Capability Backlog | [04-developer-owned-engine-capability-backlog.md](production-completion-v1/04-developer-owned-engine-capability-backlog.md) |
| Official practice gates, projections, and gameplay scenarios | [05-projections-and-scenarios.md](production-completion-v1/05-projections-and-scenarios.md) |
| Historical verdict archive and retained static-check evidence | [99-historical-verdict-archive.md](production-completion-v1/99-historical-verdict-archive.md) |
| Clean-break broad production sequencing | [Clean Break Broad Production Readiness Master Plan v1](2026-05-27-clean-break-broad-production-readiness-master-plan-v1.md) |

## Official implementation rule

Prefer official documentation and SDK guidance first: CMake, vcpkg, Direct3D 12, Vulkan, Metal, Android GameActivity, platform SDKs, OpenAI Codex docs, Anthropic Claude Code docs, and vendor engine references where explicitly cited in the chapter.

This is a greenfield engine. Use clean breaking changes when they remove duplicated contracts, stale compatibility layers, or ambiguous API ownership.

## AI readability contract

- This plan describes a general-purpose game engine, not a genre-specific engine.
- Read the parent index first, then load only the chapter that owns the current decision.
- Treat gameplay archetypes as validation scenarios, not product templates or mandatory engine feature bundles.
- Prefer reusable engine primitives: input, simulation, scene, assets, rendering, audio, UI, persistence, networking, tooling, validation, and packaging.
- Keep game-specific rules, balance, content, and presentation in game-owned files unless the same capability is reusable across multiple gameplay families.
- Missing reusable engine capability becomes a developer-owned backlog row; game-creation agents must not patch engine internals.

## Generic engine decision rule

Promote a feature into the engine only when it is reusable across at least two gameplay families, has a stable public contract, can be validated without one specific game, and can be exposed safely to AI through manifests or reviewed tools. Otherwise keep it in sample/game code.
## AI game-creation boundary

Game-creation agents may generate and edit game-owned files, code, and assets under `games/<game_name>/`. They must not mutate engine internals, editor internals, shared tools, schemas, agent policy, CMake/vcpkg contracts, or validation policy while acting as game creators.

When a generated game needs an engine capability that is missing, record a developer-owned handoff in the capability backlog instead of patching engine internals from the game-creation flow.

## Maintenance contract

- Keep this index small.
- Put durable details in the chapter that owns them.
- Keep static checks pointed at this index plus the four-file production-completion corpus.
- Update `AGENTS.md`, skills, rules, subagents, manifest fragments, schemas, and validation checks when durable workflow or AI-operable contracts change.
