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
- Current active plan: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`. `recommendedNextPlan.id = next-production-gap-selection`; `unsupportedProductionGaps = []`.
- Current manifest state: `unsupportedProductionGaps = []`; `recommendedNextPlan.id = next-production-gap-selection`. MAVG Asset Graph v1 completed through PR #516 / merge commit `9c3b6ad51caa48f4d872cef6e046de5045ab3c09`, MAVG runtime LOD selection plan selection through PR #517, MAVG Runtime LOD graph through PR #518, static MAVG cook payload through PR #519, CPU LOD selector through PR #520, runtime LOD residency through PR #521, and MAVG conventional renderer submission closeout through the current range-aware indexed draw and `MeshCommand` planning slice. MAVG Runtime LOD Milestone v1 includes clean-break `MavgClusterGraphCluster` hierarchy/error/fallback/draw-range rows, expanded `GameEngine.MavgClusterGraph.v1` validation/serialization, draw-ready static `GameEngine.MavgClusterPayload.v1` vertex/index rows, CPU reference selection through `mavg_lod_selection.hpp`, `MavgLodViewDesc`, `MavgLodResidentPageSet`, `MavgLodSelectionResult`, and `select_mavg_lod_clusters`, runtime resident-page evidence through `mavg_lod_residency.hpp`, `RuntimeMavgLodResidencyDesc`, `RuntimeMavgLodResidencyResult`, and `build_runtime_mavg_lod_residency`, range-aware conventional indexed draws through `MeshIndexedDrawRange` and `rhi::IRhiCommandList::draw_indexed`, and conventional scene submission planning through `mavg_scene_lod.hpp` / `plan_mavg_scene_lod_mesh_commands`. MAVG Page Streaming Eviction Review v1 completed the caller-reviewed eviction-protection follow-up over completed page queue evidence, and MAVG Automatic Eviction Policy v1 adds deterministic resident-page candidate ordering through `RuntimeMavgPageStreamingAutomaticEvictionPlanDesc`, `plan_runtime_mavg_page_streaming_automatic_evictions`, `planned_automatic_eviction_policy`, `automatic_eviction_candidate_count`, and `protected_eviction_candidate_skip_count` without runtime-inferred LRU/frequency inference. MAVG Resident Page Recency Eviction Order v1 (`mavg-resident-page-recency-eviction-order-v1`) adds caller-supplied recency ordering through `RuntimeMavgPageStreamingAutomaticEvictionPolicyKind::caller_supplied_recency`, `RuntimeMavgPageStreamingRecencyRow`, `resident_page_last_used_generation`, `applied_caller_supplied_recency_policy`, `recency_eviction_candidate_count`, `duplicate_recency_row_count`, and `missing_recency_row_count` without runtime-inferred LRU/frequency policy, GPU memory pressure integration, or DirectStorage execution. MAVG Phase 0 completed the official-source checks, clean-room/legal guardrails, benchmark methodology, current engine anchors, and stale-doc cleanup before implementation; no SDL3/Dear ImGui, public native handles, Nanite/UE compatibility, Nanite equivalence, or Nanite superiority claim is allowed. Environment System v1, Environment Rendering Readiness v1, and Environment Production Excellence v1 through Phase 8 remain retained evidence, including fail-closed quality-budget package counters `environment_quality_preset`, `environment_quality_budget_violations`, and `environment_quality_budget_broad_optimization_claimed`. GPU culling, indirect draw execution, mesh shaders, background package streaming execution, runtime-inferred LRU/frequency eviction policy, deformation, ray tracing, benchmark runner/results, Vulkan/Metal readiness by inference, broad environment readiness, backend parity, and broad CPU/GPU/memory optimization remain unclaimed. First-Party Desktop Platform And SDL3 Removal v1, MK_platform_win32, MK_runtime_host_win32_presentation, MK_audio_wasapi, Renderer RHI Resource Foundation 1.0 Scope Closeout v1, Renderer Backend Parity Host Recipe Proof v1, Native Win32 Editor Shell v1, Job Execution Placement Policy v1 (`job_execution_placement_policy_status=ready`, host-independent CPU placement policy evidence), Windows CPU Set Worker Placement v1 (`windows_cpu_set_worker_placement_status=ready`, `windows_cpu_set_worker_placement_native_thread_handles_exposed=0`), Long-Running Performance Readiness v1, SIMD Dispatch Policy And Evidence v1, and AVX2 Reviewed Target Execution v1 remain completed evidence layers.
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
