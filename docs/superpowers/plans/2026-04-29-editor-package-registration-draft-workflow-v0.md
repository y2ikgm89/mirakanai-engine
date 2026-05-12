# Editor Package Registration Draft Workflow v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn visible editor scene package candidate rows into a deterministic, GUI-independent package registration draft so authors can see which runtime files still need `game.agent.json.runtimePackageFiles` entries before package automation writes manifests.

**Architecture:** Keep `mirakana_editor_core` as the source of package registration review behavior. `mirakana_editor` should only adapt rows and diagnostics through Dear ImGui. Do not add JSON dependencies, do not mutate `game.agent.json` in this v0 slice, and do not invoke CMake/package tools from the editor shell.

**Tech Stack:** Existing C++23 `mirakana_editor_core`, `ScenePackageCandidateRow`, `AssetRegistry`/text-store style validation patterns, Dear ImGui shell adapter, existing schema/agent/GUI/default validation. No new third-party dependencies.

---

## Context

Editor Scene/Prefab GUI Wiring + Package Candidate Workflow v0 made package candidates visible in the Assets panel, but the rows are still read-only hints. Manifest-driven desktop package registration already exists in CMake and validation tooling; the next editor gap is a safe draft layer that compares package candidates against existing manifest package-file entries and explains what would be added, skipped, or rejected before any manifest write workflow is accepted.

## Constraints

- No new dependencies.
- Do not parse or write JSON in C++ for this v0 slice.
- Keep `editor/core` GUI-independent and default-preset buildable.
- Dear ImGui remains only an optional editor adapter.
- Do not invoke CMake, package scripts, or shell commands from the editor.
- Do not change public game APIs.

## Done When

- `mirakana_editor_core` exposes deterministic package registration draft rows and diagnostics from package candidates plus caller-supplied existing package-file paths.
- Draft validation rejects unsafe paths, source-only candidates, duplicate additions, and already-registered entries while preserving runtime package candidates in stable order.
- The visible Assets/package UI renders the draft rows and diagnostics as review-only data beside package candidate rows.
- Docs, roadmap, gap analysis, manifest, Codex/Claude editor guidance, and relevant subagent guidance are synchronized.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass with existing host/toolchain gates recorded.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Create: this plan
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`

- [x] **Step 1: Create this dated focused plan.**
- [x] **Step 2: Move Editor Scene/Prefab GUI Wiring + Package Candidate Workflow v0 to completed and mark this plan as active.**
- [x] **Step 3: Keep gap analysis focused on package registration draft workflow.**

### Task 2: Write The Failing Core Contract

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Add tests for package registration draft rows from scene package candidates and existing package entries.**
- [x] **Step 2: Add static AI integration checks for the new editor package registration draft contract and Assets panel adapter.**
- [x] **Step 3: Run focused tests or `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and confirm RED.**

### Task 3: Implement The GUI-Independent Draft Model

**Files:**
- Modify: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Modify: `editor/core/src/scene_authoring.cpp`

- [x] **Step 1: Add value-type draft rows with per-row diagnostics and stable kind/status labels.**
- [x] **Step 2: Build rows from `ScenePackageCandidateRow` plus caller-supplied existing `runtimePackageFiles` paths.**
- [x] **Step 3: Reject source-only, unsafe, duplicate, empty, and already-registered entries without throwing for user data.**
- [x] **Step 4: Keep output sorted in package candidate order with deterministic diagnostics.**

### Task 4: Wire The Editor Adapter

**Files:**
- Modify: `editor/src/main.cpp`

- [x] **Step 1: Add a small editor-shell source of existing package entries for the current project manifest lane.**
- [x] **Step 2: Render package registration draft rows and diagnostics next to package candidate rows in the Assets panel.**
- [x] **Step 3: Keep the UI review-only; manifest writes remain a separate accepted design.**

### Task 5: Sync Docs, Manifest, Skills, And Subagents

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `.codex/agents/engine-architect.toml`
- Modify: `.claude/agents/engine-architect.md`

- [x] **Step 1: Document the draft model and editor adapter honestly.**
- [x] **Step 2: Keep non-goals explicit: JSON manifest writes, CMake invocation, package builds, prefab overrides, and play-in-editor isolation remain follow-up work.**
- [x] **Step 3: Keep Codex and Claude guidance behaviorally equivalent.**

### Task 6: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
```

- [x] **Step 2: Run API boundary validation if public editor headers changed.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

- [x] **Step 3: Run required final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 4: Run `cpp-reviewer` and fix actionable findings.**

## Validation Evidence

- RED:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: FAIL before implementation with `editor/core/include/mirakana/editor/scene_authoring.hpp missing editor package registration draft contract`.
- Focused PASS:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`: PASS with `mirakana_editor_core_tests` and 32/32 desktop GUI CTest tests passing.
- Sandbox note:
  - Normal sandbox `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` repeatedly hit the known vcpkg 7zip `CreateFileW stdin failed with 5 (Access is denied.)` host/sandbox issue. The same command passed when rerun with escalation.
- Review:
  - First `cpp-reviewer` pass found an empty existing-package adapter source and Windows case drift in project-root stripping. Both were fixed.
  - Second `cpp-reviewer` pass reported no findings.
- Final default validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS.
  - Existing diagnostic-only host gates remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy diagnostic-only because the Visual Studio dev preset does not emit the expected compile database.
