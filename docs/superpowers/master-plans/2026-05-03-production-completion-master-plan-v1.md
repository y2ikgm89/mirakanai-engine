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
- Current active plan: this master plan is the selection gate after [Job Execution Topology Policy v1](../plans/2026-06-02-job-execution-topology-policy-v1.md) completed through PR #382 / merge commit `f277e990dfc27afea4492c7a1990969fd47a7c39`; the selection-gate closeout synced through PR #383 / merge commit `8d8ddbcc5090f70c6d826ed4a538c77e1c9b1c91`.
- Current manifest state: `unsupportedProductionGaps = []`; `recommendedNextPlan.id = job-execution-work-stealing-v1`. Job Execution Work Stealing v1 is the active selected optimization capability over completed Job Scheduling Evidence v1, Job Execution Worker Pool v1, and Job Execution Topology Policy v1. It adds opt-in first-party `JobExecutionPool` work stealing, `JobExecutionPoolDesc::work_stealing_enabled`, `JobExecutionTopologyPolicyDesc::enable_work_stealing`, runtime steal attempt/success/wait counters, deterministic publish order preservation, and selected `sample_desktop_runtime_game --require-job-execution-work-stealing` package-visible `job_execution_work_stealing_*` evidence without claiming affinity, NUMA placement execution, SMT/hybrid CPU policy, SIMD dispatch, GPU async overlap, CUDA/HIP/SYCL, cross-vendor/backend parity, or broad all-core CPU/GPU/memory optimization. Renderer Backend Parity Recipe Allowlist v1 completed through PR #388 / merge commit `4a65b1ff`, Renderer Metal CI Host Recipe v1 completed through PR #386 / merge commit `12fde1bf7ea369e97a45d95a0ee271b4e2515b3d`, and `metal-apple`, broad Metal parity, and broad renderer quality remain host-gated or unclaimed. Frame/Thread Scratch v1, Memory Diagnostics v1, Memory Lifetime Taxonomy v1, Performance Baseline v1, AI-Operable Performance Budget And Evidence v1, Native Win32 Editor Shell v1, First-Party Editor Shell v1, and First-Party UI Editor Production Stack v1 remain completed foundation/productization layers. The current Engine 1.0 backlog has no pure `candidate` or `production-candidate` rows; remaining non-completed rows are host-gated or future selected work. Broad all-core CPU scheduling beyond selected worker-pool evidence, affinity pinning, NUMA placement, SMT/hybrid CPU policy, SIMD dispatch, GPU async overlap, CUDA/HIP/SYCL, cross-vendor/backend parity, allocator enforcement, broad renderer quality, broad editable rich text, Direct2D GPU text rendering/upload, broad shaping/bidi/fallback, full app-owned `ITextStoreACP` callback coverage, native IME candidate UI, reconversion, full UIA control pattern/event parity, cross-platform editor/accessibility parity, Vulkan/Metal editor texture-display parity, broader material-preview GPU parity, SDL3, public native handles, filesystem/package mutation, runtime source parsing, runtime game UI middleware, and broad performance readiness remain unclaimed until focused phases validate them.
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
