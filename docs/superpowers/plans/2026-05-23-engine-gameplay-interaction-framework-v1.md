# Engine Gameplay Interaction Framework v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a reusable first-party runtime gameplay interaction contract for trigger, damage/heal, pickup, objective, restart, win/loss, and feedback rows, then prove it through runtime tests and 2D/3D package counters.

**Plan ID:** `engine-gameplay-interaction-framework-v1`

**Status:** Local implementation validated; PR/hosted checks pending.

**Gap:** `engine-gameplay-interaction-framework-v1`

**Architecture:** Introduce a backend-neutral `MK_runtime` value API that updates caller-owned gameplay interaction state and emits deterministic rows/feedback rows. Keep execution of UI/audio/VFX game-owned; the runtime contract only names feedback ids and produces counters. Package samples consume the public API through `--require-gameplay-systems`, while docs, manifest fragments, schemas/static checks, and quality-rubric examples are synchronized in the same slice.

**Tech Stack:** C++23, CMake `MK_runtime`, repository `tools/*.ps1` validation entrypoints, `game.agent.json` contracts, composed engine agent manifest fragments.

---

## Classification

Large: this changes public C++ runtime API, sample package behavior, package-visible counters, quality-rubric evidence, docs, manifest fragments, and static checks. It requires subagent design review, TDD red/green, focused C++ build/test/static checks, and full `tools/validate.ps1`.

## Goal / Context / Constraints / Done When

**Goal:** Provide reusable gameplay interaction state transitions that generated and sample games can call without scene bindings, renderer/RHI/native handles, Dear ImGui, SDL3, or middleware exposure.

**Context:** `runtime_scene` already resolves scene gameplay interaction intent rows, but session-level restart/win/loss/objective/feedback flows need a reusable game-state contract. Generated 2D/3D package quality rubrics already ask for feedback and fail/restart evidence, but package counters currently only prove broad gameplay systems readiness.

**Constraints:**
- Clean breaking greenfield implementation; no compatibility shim, deprecated alias, migration layer, or duplicate old API.
- Public API stays first-party and backend-neutral.
- `engine/agent/manifest.json` is never edited directly; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/*.ps1`; no `bun run`.
- Keep task-owned changes only in this worktree.

**Done when:**
- Runtime gameplay interaction tests prove deterministic accepted rows, feedback rows, state updates, restart without scene binding, and fail-closed diagnostics.
- 2D and 3D package samples report interaction counters under `--require-gameplay-systems`.
- Quality-rubric examples and docs reference the new counters for feedback and fail/restart evidence.
- Static checks guard the public header, manifest/docs/rubric counters, and backlog closeout.
- Focused build/test/static lanes and final `tools/validate.ps1` pass.
- PR hosted checks pass, PR merges, main fast-forwards, and the worktree is removed with `tools/remove-merged-worktree.ps1`.

## Task 1: Runtime API Red Test

**Files:**
- Create: `tests/unit/runtime_gameplay_interaction_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Write the failing test**

Add tests that include `mirakana/runtime/gameplay_interaction.hpp` and assert:
- accepted trigger, damage, heal, pickup, objective progress, feedback, win, and restart rows are deterministic;
- restart works from a terminal state without source or scene binding ids;
- invalid ids/amounts fail closed with unchanged state and no rows.

- [x] **Step 2: Verify red**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_gameplay_interaction_tests
```

Expected: build fails because `mirakana/runtime/gameplay_interaction.hpp` is missing.

## Task 2: Runtime API Green

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/gameplay_interaction.hpp`
- Create: `engine/runtime/src/gameplay_interaction.cpp`
- Modify: `engine/runtime/CMakeLists.txt`

- [x] **Step 1: Add public value types**

Define `RuntimeGameplayInteractionKind`, `RuntimeGameplaySessionState`, state rows for entities/pickups/objectives, `RuntimeGameplayInteractionEvent`, deterministic result rows, feedback rows, diagnostics, and `RuntimeGameplayInteractionPlan`.

- [x] **Step 2: Add planner**

Implement `plan_runtime_gameplay_interactions` with fail-closed prevalidation and deterministic state transitions:
- `trigger` emits a row and optional feedback.
- `damage` / `heal` require positive amount and valid source/target entities, clamp health, and update active state.
- `pickup` requires an available pickup and marks it unavailable.
- `objective_progress` / `objective_complete` update progress/completion.
- `feedback` emits a feedback row only.
- `win` / `loss` / `restart` transition session state; `restart` is allowed from terminal state without source entity.

- [x] **Step 3: Verify green**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_gameplay_interaction_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_gameplay_interaction_tests
```

Expected: target builds and the new test passes.

## Task 3: Package Counter Red/Green

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/README.md`
- Modify: `games/sample_generated_desktop_runtime_3d_package/README.md`

- [x] **Step 1: Add package-visible counter expectations**

Both samples should use the new public runtime planner inside existing gameplay systems probes and report:
- `gameplay_systems_interaction_ready=1`
- `gameplay_systems_interaction_rows`
- `gameplay_systems_interaction_feedback_rows`
- `gameplay_systems_interaction_final_session_state=running`

- [x] **Step 2: Run focused sample targets**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "sample_2d_desktop_runtime_package|sample_generated_desktop_runtime_3d_package"
```

Expected: build/tests pass and counters are emitted by the package smoke paths.

## Task 4: Agent/Docs/Static Synchronization

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify as needed: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Add/modify static check sections under `tools/`
- Compose: `engine/agent/manifest.json`

- [x] **Step 1: Add failing static guard**

Add static guard needles for the new header, sample counters, quality-rubric counter links, manifest claim, and backlog closeout. Run the guard before the docs/manifest updates and confirm it fails.

- [x] **Step 2: Update surfaces**

Update docs, game manifests, manifest fragments, backlog row, plan registry, and compose output.

- [x] **Step 3: Verify static green**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: static contracts pass.

## Task 5: Focused and Full Validation

**Files:**
- All task-owned changes.

- [x] **Step 1: Run C++ focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/include/mirakana/runtime/gameplay_interaction.hpp,engine/runtime/src/gameplay_interaction.cpp,tests/unit/runtime_gameplay_interaction_tests.cpp,games/sample_2d_desktop_runtime_package/main.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

Expected: focused static and formatting checks pass.

- [x] **Step 2: Run full validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: full validation passes or any host-gated blocker is recorded with exact evidence.

## Task 6: PR, Hosted Checks, Merge, Cleanup

**Files:**
- No new edits unless hosted CI exposes a real root-cause failure.

- [ ] **Step 1: Commit and push task-owned changes**

Run status, stage only task-owned changes, `git diff --cached --check`, commit, and push `codex/engine-gameplay-interaction-framework-v1`.

- [ ] **Step 2: Create PR**

PR body includes summary, clean-breaking decision, validation evidence, hosted-check expectations, and any host-gated blockers.

- [ ] **Step 3: Merge only after hosted checks**

Confirm PR head SHA, require same-SHA hosted checks, use auto-merge with `--match-head-commit`, then sync local main and cleanup with `tools/remove-merged-worktree.ps1`.

## Evidence Log

| Phase | Evidence |
| --- | --- |
| Subagent review | Read-only `engine-architect` review completed; recommendation accepted to make feedback a first-party value contract and keep execution game-owned. |
| Red test | `tools/check-toolchain.ps1` and `tools/cmake.ps1 --preset dev` passed; `tools/cmake.ps1 --build --preset dev --target MK_runtime_gameplay_interaction_tests` failed as expected before `mirakana/runtime/gameplay_interaction.hpp` existed. |
| Final diff review | Read-only `cpp-reviewer` feedback was accepted for same-plan terminal-state validation, `loss -> restart` package evidence, full interaction counter guards, and PowerShell variable hygiene. Samples now prove 10 interaction rows and 10 feedback rows. |
| Focused validation | `tools/cmake.ps1 --build --preset dev --target MK_runtime_gameplay_interaction_tests sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package` passed; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_gameplay_interaction_tests|sample_2d_desktop_runtime_package|sample_generated_desktop_runtime_3d_package"` passed 6/6 tests; `tools/check-public-api-boundaries.ps1`, `tools/check-tidy.ps1 -Files engine/runtime/src/gameplay_interaction.cpp,tests/unit/runtime_gameplay_interaction_tests.cpp,games/sample_2d_desktop_runtime_package/main.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1` passed after review fixes. |
| Full validation | `tools/validate.ps1` passed. Diagnostic-only host gates remained expected on this Windows host for Metal/Apple packaging evidence. |
| Hosted checks | Pending. |
