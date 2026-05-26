# Implementation Plan Registry

This directory stores the active implementation-plan registry plus remaining plan evidence that is still referenced by current docs, manifests, skills, or static checks. It is not the canonical product description and it is not a full changelog.

Historical dated plans that have no references outside this registry may be removed from the working tree. Use Git history for deleted evidence rather than recreating hundreds of completed plan files.

Use these current-truth docs first:

- [Current capabilities](../../current-capabilities.md)
- [Roadmap](../../roadmap.md)
- [Architecture](../../architecture.md)
- [Workflows](../../workflows.md)
- `engine/agent/manifest.json`
- [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)
- [Directory layout target v1](../../specs/2026-05-11-directory-layout-target-v1.md) and [ADR 0003](../../adr/0003-directory-layout-clean-break.md) for `engine/tools` physical layout and `MK_tools` CMake invariants

## Plan Lifecycle

- **Active roadmap:** broad sequencing for current production work.
- **Active gap burn-down:** gap-level plan for one selected `unsupportedProductionGaps` row.
- **Active milestone:** phase-gated milestone plan selected by `currentActivePlan` when tightly related slices share one end-to-end objective.
- **Active phase/child:** focused phase or child plan selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.
- **Docs/governance slice:** short documentation or agent-surface cleanup that must not overwrite the active engine slice unless the manifest is intentionally changed.
- **Completed:** historical implementation evidence; do not keep extending it as a live task list.
- **Host-gated:** planned or partially implemented work that still needs the required host/toolchain validation.
- **Historical:** preserved for context only.

When closing the active plan, do not leave `currentActivePlan` pointing at a completed plan. In the same closeout change, either select the next active dated plan or point back to the master plan, update `recommendedNextPlan`, registry, ledger, and manifest fragments, compose the manifest, then run `check-json-contracts` before the final validation gate.

## Dated Plan And Spec Filenames

- Prefix dated files under `docs/superpowers/plans/` and dated specs under `docs/specs/` with `YYYY-MM-DD` matching the actual authoring calendar date.
- Authority order: session/host `Today's date` when provided, explicit operator-stated date, then local host date from `Get-Date -Format yyyy-MM-dd`.
- Filename date and the first dated heading must match.

## Plan Volume Policy

- Keep the live execution stack to at most three layers: the active roadmap, one active gap-cluster burn-down or milestone, and one active phase/child plan selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.
- Create a new dated plan for a capability/gap-cluster/milestone, not for every behavior slice.
- Plan width is wider than PR width: one dated plan may contain multiple phase checkpoints, but each phase/PR should stay one behavior/API/validation boundary and one reviewable purpose.
- Do not split, merge, or rename dated plans to chase a commit, push, or PR count.
- Distinguish plan files from execution steps: checkboxes can be small actions, and each phase behavior/API/validation boundary should close with focused evidence.
- Do not create new plan files for validation-only follow-up, docs/manifest/static-check synchronization, small mechanical cleanup, or substeps that fit the current active plan checklist.
- Completed plan files are retained only while referenced by current guidance, manifest/static checks, or active decisions. Unreferenced completed plans may be deleted after a reference scan; their evidence remains available through Git history.
- Static checks must not require retired dated plan files as source artifacts. When a historical literal is still contract-relevant, assert current source/docs/manifest truth or move the literal to [99-historical-verdict-archive.md](../master-plans/production-completion-v1/99-historical-verdict-archive.md).
- Keep the registry biased toward current active work. Put historical/static-check retained literals in [99-historical-verdict-archive.md](../master-plans/production-completion-v1/99-historical-verdict-archive.md), not in this registry.

## Plan Scope Heuristics

- A good dated plan is usually one production capability, gap cluster, or milestone, not one assertion, mechanical test, or single API helper.
- Prefer one gap-level burn-down or milestone when slices share the same architecture decision, public API family, and validation surface; create child plans only when the phase would exceed one safe implementation/review context.
- If consecutive slices keep reloading the same context, touching the same manifest/docs, and proving the same validation surface, broaden the active plan and keep each PR as the reviewable checkpoint.
- Do not create a new plan for docs-only synchronization, manifest pointer cleanup, static-check churn, existing-test initializer updates, or validation reruns unless that documentation or validation surface is itself the product behavior under test.
- Keep active context short: link historical evidence instead of copying long completed-slice prose into active sections or `recommendedNextPlan.completedContext`.

## Production Execution Prompt

For `production-completion-master-plan-v1`, drive from `engine/agent/manifest.json.aiOperableProductionLoop`, finish one selected unsupported gap or active phase at a time, prefer official best practices and clean breaking implementation over compatibility shims, optimize context and rate use with focused reads/tests and bounded independent subagents, keep the tree free of temporary/dead files, and close each phase only after code, tests, docs, skills, manifest, static checks, remaining gaps, next plan, and host-gated blockers match the validation evidence.

## Current Active Work

| Status | Plan | Notes |
| --- | --- | --- |
| Active milestone (`currentActivePlan`) | [Engine General Production Quality Expansion v1](2026-05-26-engine-general-production-quality-expansion-v1.md) | Selected post-1.0 production breadth plan for renderer quality, runtime UI text/font/IME/accessibility, broad reviewed import, physics/navigation breadth, networking execution/security, and audio production evidence. It keeps `unsupportedProductionGaps = []` because this is not a reopened Engine 1.0 blocker. Phases 1-5 are implemented through Renderer General Quality Matrix v1, Runtime UI Production Stack Evidence v1, Asset Import Production Review v1, Physics/Navigation Production Breadth v1, and Runtime Network Production Security Gate v1; Phase 5 uses `plan_runtime_network_production_security_gate` for threat-modeled selected evidence without NAT traversal or broad online claims. None claims broad production readiness beyond selected evidence and host-gated rows. |
| Active phase checkpoint | [Engine General Production Quality Expansion v1 - Phase 6](2026-05-26-engine-general-production-quality-expansion-v1.md#phase-6---audio-production-playback-streaming-dsp-and-spatialization) | Audio Production Playback/Streaming/DSP/Spatialization Evidence v1 adds `review_audio_production_readiness` for reviewed decoded source, streaming chunk, conversion, bus/voice budget, DSP graph, spatial/listener, HRTF host-gate, and device lifecycle rows plus selected 2D package `audio_production_*` counters. It keeps device and HRTF host evidence separate and does not claim broad codec support, audio middleware parity, HRTF execution parity, native handles, background streaming execution, or general production audio quality. |
| Parent roadmap | [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md) | Foundation Ready Burn-down v1 completed all 15 canonical `foundation-ready` rows through PR checkpoints #212-#226; the new production track keeps `unsupportedProductionGaps = []` and does not reopen completed foundation rows. |
| Completed slice | [Renderer Production VFX Profiling Host Gate v1](2026-05-26-renderer-production-vfx-profiling-host-gate-v1.md) | Completed developer-owned renderer production evidence slice for `production-rendering-vfx-profiling-v1`; it adds explicit D3D12 and strict Vulkan host-evidence-backed VFX/profiling counters, package-visible backend evidence rows, and fail-closed cross-backend diagnostics while keeping Metal Apple-host-gated and broad renderer readiness out of the claim. |
| Completed slice | [Engine 1.0 Gap Matrix v1](2026-05-26-engine-1-0-gap-matrix-v1.md) | Completed docs/manifest governance selection pass; it selected and then closed the first follow-up `renderer-backend-parity-v1` strict Vulkan evidence work through PR #248, while keeping Metal Apple-host-gated and returning the manifest pointer to `next-production-gap-selection`. |
| Completed milestone | [Generated Game Studio v1](2026-05-26-generated-game-studio-v1.md) | Completed successor game-surface milestone that composes AI game design, generation, package review, validation preflight, evidence import, remediation, quality rubric, and editor AI command surfaces into one reviewable 2D/3D studio loop while keeping engine internals, arbitrary shell, native handles, renderer/RHI residency, Metal readiness, and broad editor productization outside the claim. |
| Completed milestone | [General Purpose Game Production v1](2026-05-25-general-purpose-game-production-v1.md) | Completed post-foundation production track; Phases 1-10 implement reusable gameplay runtime scheduling, world/entity ownership, addressable content streaming, production authoring workflow review, runtime UI workbench review, `genre-rpg-systems-pack-v1` through `plan_runtime_rpg_systems`, `genre-sandbox-world-pack-v1` through `plan_runtime_sandbox_world_mutation`, `genre-simulation-management-pack-v1` through `plan_runtime_simulation_management`, `production-network-replication-v1` through `plan_runtime_network_replication`, and `production-rendering-vfx-profiling-v1` through `plan_renderer_production_vfx_profiling`; renderer/VFX/profiling remains host-gated for Metal evidence and broad renderer readiness. |
| Completed milestone | [Foundation Ready Burn-down v1](2026-05-24-foundation-ready-burn-down-v1.md) | Completed all 15 canonical `foundation-ready` rows through PR checkpoints #212-#226 plus final procedural-content-generation-v1 merge commit `ccb446f3ef825cb7fdca87b1cb3b7262ed520a62`; `unsupportedProductionGaps = []` remains the 1.0 truth. |
| Completed slice | [Networking And Multiplayer v1](2026-05-24-networking-and-multiplayer-v1.md) | Completed selected optional network transport adapter facade and ENet loopback proof through PR #210 / merge commit `e63799b64716ad1cccb6841850edd7c5aac9d8ef`; `MK_runtime_network_enet` remains optional behind the `network-enet` vcpkg feature and does not expand generated-game multiplayer readiness. |
| Completed slice | [Native Physics Middleware Adapter v1](2026-05-24-native-physics-middleware-adapter-v1.md) | Completed selected optional native physics middleware adapter through PR #209 / merge commit `ccce45701ff3a361977d6286affd46f0ebc16f0d`; `MK_physics_jolt` remains optional behind the `physics-jolt` vcpkg feature and does not expand the default/generated-game Physics 1.0 ready surface. |
| Completed slice | [Scripting And Mod Sandbox v1](2026-05-24-scripting-and-mod-sandbox-v1.md) | Completed selected optional-adapter scripting facade through PR #206 / merge commit `f77bd630b6bc2eea5b0ce4c7bdd01931f114743a`; runtime execution remained dependency-free and denied filesystem/network/process/native-plugin access by default. |
| Completed milestone | [Candidate Backlog Burn-down v1](2026-05-23-candidate-backlog-burn-down-v1.md) | Completed all seven canonical post-1.0 candidate rows through PR #204 / merge commit `971cee3f6c5b42965721c06974bc506f1b35508c`. |

## Current Evidence Pointers

| Need | Read |
| --- | --- |
| 1.0 readiness, current verdict, and supported/unsupported boundaries | [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md) and [01-one-dot-zero-readiness-ledger.md](../master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md) |
| Post-1.0 / 1.x developer-owned capability backlog | [04-developer-owned-engine-capability-backlog.md](../master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md) |
| Gameplay scenario projections and official practice gates | [05-projections-and-scenarios.md](../master-plans/production-completion-v1/05-projections-and-scenarios.md) |
| AI game design spec implementation evidence | [AI Game Design Spec v1](2026-05-23-ai-game-design-spec-v1.md) |
| AI game generation orchestrator implementation evidence | [AI Game Generation Orchestrator v1](2026-05-23-ai-game-generation-orchestrator-v1.md) |
| AI safe content mutation ledger implementation evidence | [AI Safe Content Mutation Ledger v1](2026-05-23-ai-safe-content-mutation-ledger-v1.md) |
| AI placeholder asset pipeline implementation evidence | [AI Placeholder Asset Pipeline v1](2026-05-23-ai-placeholder-asset-pipeline-v1.md) |
| AI generated game playtest loop implementation evidence | [AI Generated Game Playtest Loop v1](2026-05-23-ai-generated-game-playtest-loop-v1.md) |
| AI validation remediation recipes implementation evidence | [AI Validation Remediation Recipes v1](2026-05-23-ai-validation-remediation-recipes-v1.md) |
| AI generated game quality rubric implementation evidence | [AI Generated Game Quality Rubric v1](2026-05-23-ai-generated-game-quality-rubric-v1.md) |
| AI Codex/Claude/Cursor agent surface implementation evidence | [AI Codex/Claude/Cursor Agent Surface v1](2026-05-23-ai-codex-claude-agent-surface-v1.md) |
| Runtime network transport adapter implementation evidence | [Networking And Multiplayer v1](2026-05-24-networking-and-multiplayer-v1.md) |
| Runtime scripting sandbox implementation evidence | [Scripting And Mod Sandbox v1](2026-05-24-scripting-and-mod-sandbox-v1.md) |
| Engine gameplay interaction framework implementation evidence | [Engine Gameplay Interaction Framework v1](2026-05-23-engine-gameplay-interaction-framework-v1.md) |
| Historical Physics 1.0 Jolt exclusion evidence (`physics-jolt-adapter-gate-v1`) | [99-historical-verdict-archive.md](../master-plans/production-completion-v1/99-historical-verdict-archive.md) |
| Engine save settings profile implementation evidence | [Engine Save Settings Profile v1](2026-05-23-engine-save-settings-profile-v1.md) |
| Engine UI game menu HUD implementation evidence | [Engine UI Game Menu HUD v1](2026-05-23-engine-ui-game-menu-hud-v1.md) |
| Engine audio gameplay mixer implementation evidence | [Engine Audio Gameplay Mixer v1](2026-05-23-engine-audio-gameplay-mixer-v1.md) |
| AI engine capability handoff implementation evidence | [AI Engine Capability Handoff v1](2026-05-22-ai-engine-capability-handoff-v1.md) |
| Historical/static-check retained literals | [99-historical-verdict-archive.md](../master-plans/production-completion-v1/99-historical-verdict-archive.md) |
| Recent completed implementation evidence | Search [99-historical-verdict-archive.md](../master-plans/production-completion-v1/99-historical-verdict-archive.md) and Git history; do not treat this registry as a changelog. |

## Pinned Foundation Plans

Manifest-pinned foundation records remain as dated files in this directory, and selected completed capability evidence plans may also remain while current docs, manifests, static checks, or active decisions still reference them. Completed governance, bridge, and capability plans that no longer need direct links are archived in [99-historical-verdict-archive.md](../master-plans/production-completion-v1/99-historical-verdict-archive.md).

| Plan | Why it remains |
| --- | --- |
| [2026-05-01-core-first-mvp-closure.md](2026-05-01-core-first-mvp-closure.md) | `engine/agent/manifest.json.stageClosurePlan` historical baseline. |
| [2026-05-01-ai-operable-production-loop-foundation-v1.md](2026-05-01-ai-operable-production-loop-foundation-v1.md) | `engine/agent/manifest.json.aiOperableProductionLoop.foundationPlan` baseline. |

## Host-Gated Blockers

- Apple/iOS/Metal validation requires macOS with Xcode tools plus simulator/device signing paths.
- Vulkan SPIR-V validation is toolchain-gated and requires `spirv-val` plus DXC with SPIR-V CodeGen.
- Metal shader/library work requires Apple toolchains.
- Full repository clang-tidy work depends on CMake/clang-tidy availability. Compile database synthesis is ready for the default Windows Visual Studio `dev` preset through CMake File API codemodel data.

These blockers may appear in validation output as diagnostic-only checks. Do not mark host-gated work complete unless a local or CI lane has actually validated it.
