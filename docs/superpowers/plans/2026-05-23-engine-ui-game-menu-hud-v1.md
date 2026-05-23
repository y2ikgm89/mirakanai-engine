# Engine UI Game Menu HUD v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the existing runtime menu/HUD intent foundation into a package-visible generated-game UI slice by adding simple dialogue-box and input-binding prompt row kinds plus selected 2D/3D package smoke counters.

**Plan ID:** `engine-ui-game-menu-hud-v1`

**Status:** Local validation passed; PR/hosted checks pending.

**Gap:** `engine-ui-game-menu-hud-v1`

**Architecture:** Extend the first-party `MK_ui` value-only runtime menu/HUD planner. Keep renderer submission, text shaping, IME, accessibility bridge publication, input rebinding execution, command dispatch, widgets, middleware, editor/Dear ImGui, SDL3, and native handles outside the public game-facing contract.

**Tech Stack:** C++23, `MK_ui`, `MK_core_tests`, selected 2D/3D sample package smokes, repository `tools/*.ps1` validation, composed engine agent manifest fragments.

---

## Classification

Large: this changes public C++ UI API, unit tests, selected generated-game package smoke behavior, docs, manifest fragments, and static checks. It requires TDD red/green, focused C++ build/test/static checks, final diff review, and full `tools/validate.ps1`.

## Goal / Context / Constraints / Done When

**Goal:** Let generated games validate HUD labels/counters/prompts, pause/restart/menu commands, simple dialogue boxes, and input-binding prompt rows through a first-party backend-neutral contract before package-visible menu/HUD claims.

**Context:** `MK_ui` already exposes `RuntimeMenuHudRowDesc` and `plan_runtime_menu_hud` for label/counter/prompt/command rows. The remaining backlog evidence is package-visible menu/HUD counters plus explicit dialogue/rebinding row support while preserving adapter limits.

**Constraints:**
- Clean breaking greenfield implementation; no compatibility shim, deprecated alias, duplicate API, or migration layer.
- Public API stays first-party and backend-neutral; no native handles, SDL3, Dear ImGui, renderer/RHI, or middleware types leak into game-facing contracts.
- `engine/agent/manifest.json` is never edited directly; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/*.ps1`; no `bun run`.
- Keep task-owned changes only in this worktree.

**Done when:**
- Unit tests prove dialogue-box and input-binding prompt row planning plus failure behavior.
- 2D and 3D package samples expose `--require-runtime-menu-hud` counters for display, command, dialogue, and binding-prompt rows.
- Docs, master backlog, manifest fragments, and static checks name the supported contract and unsupported adapter boundaries.
- Focused build/test/static lanes and final `tools/validate.ps1` pass.
- PR hosted checks pass, PR merges, main fast-forwards, and the worktree is removed with `tools/remove-merged-worktree.ps1`.

## Task 1: Runtime UI Red Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`

- [x] **Step 1: Add failing unit/static guards**

Add tests and static guards for:
- `RuntimeMenuHudRowKind::dialogue_box` and `RuntimeMenuHudRowKind::input_binding_prompt` display rows;
- package-visible `--require-runtime-menu-hud` sample counters for display, command, dialogue, and input-binding prompt rows.

- [x] **Step 2: Verify red**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Result: `MK_core_tests` build failed on the missing `dialogue_box` / `input_binding_prompt` row kinds, and `tools/check-ai-integration.ps1` failed on the missing runtime-menu-HUD authoring surface/counter needles.

## Task 2: Runtime UI Contract Green

**Files:**
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`

- [x] **Step 1: Add row kinds**

Add `dialogue_box` and `input_binding_prompt` row kinds to `RuntimeMenuHudRowKind` and planner validation.

- [x] **Step 2: Verify green**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests
```

## Task 3: Package Menu/HUD Counters

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: sample package READMEs and manifests as needed

- [x] **Step 1: Add selected package smoke flag**

Add `--require-runtime-menu-hud` and deterministic `runtime_menu_hud_*` counters to the selected package samples.

- [x] **Step 2: Focused sample verification**

Run targeted sample builds and source-tree smokes with the new flag.

## Task 4: Agent Surfaces And Closeout

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/*.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`

- [x] **Step 1: Sync durable guidance**

Record the new UI contract, package counters, clean non-goals, and validation evidence.

- [x] **Step 2: Compose and validate agent surfaces**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

## Validation Evidence

| Check | Command | Result |
| --- | --- | --- |
| Runtime UI red | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Failed as expected before implementation |
| Runtime UI green | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` | Passed |
| Package samples | Targeted 2D/3D builds and source-tree smokes with `--require-runtime-menu-hud` | Passed; both smokes emitted `runtime_menu_hud_ready=1`, display rows `6`, command rows `2`, dialogue rows `1`, and input-binding prompt rows `1` |
| Public API boundary | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed |
| Agent surfaces | `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1` | Passed; final review found missing dedicated 3D runtime-menu-HUD recipe/gate, then `game.agent.json` and static guards were fixed |
| Format/tidy | `tools/check-format.ps1`; `tools/check-tidy.ps1 -Files 'engine/ui/src/ui.cpp,games/sample_2d_desktop_runtime_package/main.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp,tests/unit/core_tests.cpp'` | Passed after `tools/format.ps1` |
| Full gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed; 74/74 CTest passed |
