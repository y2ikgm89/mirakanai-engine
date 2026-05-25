# 2026-05-26 Generated Game Studio v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `generated-game-studio-v1`

**Status:** Temporarily parked while `engine-contract-version-suffix-cleanup` is the selected `currentActivePlan`.

**Goal:** Turn the completed AI game-creation descriptors, package review loops, validation recipes, evidence import, remediation queue, and quality rubric into one reviewable Generated Game Studio workflow for AI-assisted 2D and 3D game iteration.

**Context:** `General Purpose Game Production v1` completed its independently reviewable phases. `unsupportedProductionGaps` remains empty, so this milestone is not a new 1.0 blocker burn-down. It is a successor game-surface milestone that composes existing ready surfaces and adds only first-party value contracts, editor-core retained UI snapshots, docs, manifests, and static guards needed for a coherent studio loop.

**Constraints:**

- Keep the autonomous game-creation lane game-surface scoped. Missing engine capability becomes a typed `ai-engine-capability-handoff-v1` row, not an engine-internal edit from this lane.
- Keep editor/core models GUI-independent, deterministic, read-only by default, and free of shell execution or manifest mutation.
- Do not expose SDL3, Dear ImGui, renderer/RHI/backend handles, native OS handles, or middleware types in public contracts.
- Prefer clean breaking first-party contracts over compatibility shims.
- Use official documentation or Context7 for touched SDK/tooling boundaries, then record validation evidence before closeout.

**Done When:** The plan registry, manifest, docs, static checks, editor-core value contracts, retained UI model, focused tests, full validation, candidate commits, PRs, hosted checks, merge, and main-sync evidence all agree that Generated Game Studio v1 is implemented or explicitly host-gated.

---

## Decision

`Generated Game Studio v1` is a new successor milestone, not Phase 11 of `General Purpose Game Production v1`. The completed production track already promoted reusable gameplay, world/entity, content, authoring, UI, genre-pack, networking, and rendering/VFX/profiling surfaces. This milestone binds the AI-facing game workflow over those surfaces:

- `ai-game-design-spec-v1`
- `ai-game-generation-orchestrator-v1`
- `ai-safe-content-mutation-ledger-v1`
- `ai-placeholder-asset-pipeline-v1`
- `ai-generated-game-playtest-loop-v1`
- `ai-validation-remediation-recipes-v1`
- `ai-generated-game-quality-rubric-v1`
- `ai-codex-claude-agent-surface-v1`
- `ai-engine-capability-handoff-v1`
- existing `MK_editor_core` playtest package review, validation preflight, operator handoff, evidence import, remediation queue, and AI command panel models

The workflow claims studio orchestration and review readiness only. It does not claim autonomous commercial game design, arbitrary shell access, broad editor productization, engine internals mutation, renderer/RHI residency, Metal readiness, platform parity, or broad renderer quality.

## Official Practice Baseline

- CMake and CTest remain target-based and are invoked only through repository PowerShell wrappers.
- SDL3 and Dear ImGui remain editor/platform shell details. Editor-core value contracts must stay independent of both APIs.
- GitHub Flow remains the publication model: topic branch, candidate commit, PR, hosted checks, merge, branch cleanup, main sync.
- Context7/official docs are required before changing SDK/toolchain/library-facing code. This milestone currently uses existing CMake/SDL3 guidance for build and editor-shell boundaries; Dear ImGui guidance is required before UI-shell changes.

## Candidate Split

1. [x] `generated-game-studio-selection-v1`: close out the old active milestone, register this plan, update backlog/projection docs, switch manifest pointers, compose the manifest, and add static guard needles.
2. [x] `generated-game-studio-session-contract-v1`: add GUI-independent `MK_editor_core` `EditorAiGeneratedGameStudioV1Model` / `make_editor_ai_generated_game_studio_v1_model` studio session value contracts that aggregate 2D/3D loop readiness, package diagnostics, validation preflight, operator handoff, evidence summary, remediation queue/handoff, and AI command panel status.
3. [x] `generated-game-studio-loop-aggregation-v1`: add deterministic 2D/3D loop rows for design-spec to package-smoke evidence, host-gated proof, remediation status, and typed engine-capability handoff blockers.
4. [x] `generated-game-studio-dashboard-v1`: expose the studio model through retained `MK_ui` snapshots and the existing Dear ImGui AI Commands panel without adding editor-core execution or mutation.
5. [x] `generated-game-studio-closeout-v1`: reconcile docs, manifest fragments, schema/static checks, validation recipes, plan evidence, and host-gated non-goals; run focused validation plus full `tools/validate.ps1`.

Each candidate should land as its own validated commit and PR when independently reviewable. If the editor-core contract and dashboard shell remain tightly coupled during implementation, keep small validated commits but publish them in one PR with the coupling reason recorded.

## File Structure

- Create: `docs/superpowers/plans/2026-05-26-generated-game-studio-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify/generated: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-051-generated-game-studio.ps1`
- Create: `editor/core/include/mirakana/editor/generated_game_studio.hpp`
- Create: `editor/core/src/generated_game_studio.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify as needed: `editor/src/main.cpp`

## Validation

Focused loops:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Slice closeout:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Phase Evidence

- 2026-05-26 local implementation evidence:
  - `tools/check-json-contracts.ps1`: passed.
  - `tools/check-ai-integration.ps1`: passed.
  - `tools/check-format.ps1`: passed.
  - `tools/check-agents.ps1`: passed.
  - `tools/validate.ps1`: passed, including build and 87/87 CTest tests.
  - Diagnostic-only host gates remain expected for Metal/Apple host evidence on this Windows host; `unsupportedProductionGaps` remains empty.
- Publication evidence:
  - PR: https://github.com/y2ikgm89/mirakanai-engine/pull/239
  - Branch: `codex/generated-game-studio-v1`
  - Initial commit: `3eaead43` (`Add Generated Game Studio v1`)
  - Closeout evidence commit: `b1620769` (`Record Generated Game Studio PR evidence`)
  - Hosted PR Gate: passed, https://github.com/y2ikgm89/mirakanai-engine/actions/runs/26412257019/job/77750252570
  - Hosted selected checks passed for Agent Static Guards, Windows MSVC, Linux CMake, Linux Coverage, Linux Clang ASan/UBSan, Full Repository Static Analysis shards, macOS Metal CMake, iOS Simulator smoke, and CodeQL.
  - Merge commit: `b4bc09733106855ce71213a7235eccc053199e7c`
  - Main sync verification: after `git fetch origin --prune`, `git log --oneline origin/main..b16207690459423ae28e8978ca4ec417463df666` produced no commits.
