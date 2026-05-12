# Manifest-Driven Desktop Runtime Package Authoring v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `game.agent.json.runtimePackageFiles` the source of truth for registered desktop-runtime package files, so generated games can move authored/cooked scene package candidates into package metadata without duplicating every file in `games/CMakeLists.txt`.

**Architecture:** Keep package file authoring in the game manifest and CMake metadata layer. `mirakana_add_desktop_runtime_game` derives repository-relative `PACKAGE_FILES` from `runtimePackageFiles` when explicitly requested, reusing the existing safety checks, build-output copy, install, and `desktop-runtime-games.json` metadata path. No runtime, renderer, RHI, SDL3, Dear ImGui, or editor API is exposed to gameplay.

**Tech Stack:** CMake 3.30 `string(JSON ...)`, existing PowerShell schema/agent checks, `game.agent.json`, `games/CMakeLists.txt`, desktop runtime package validation, no new third-party dependencies.

---

## Context

Editor Scene/Prefab Package Authoring v0 now produces scene/prefab package candidate rows, and Automatic Cooked Package Assembly v0 can build runtime-loadable `.geindex` packages. The remaining drift risk is that generated games must manually mirror the same runtime package file list in both `game.agent.json.runtimePackageFiles` and `mirakana_add_desktop_runtime_game(PACKAGE_FILES ...)`. A production-ready generated-game loop should make the manifest the durable source of truth and let CMake/package metadata derive from it.

## Constraints

- No new dependencies.
- Do not change public gameplay APIs.
- Do not expose SDL3, OS, RHI, GPU, Dear ImGui, editor, or native handles.
- `runtimePackageFiles` entries remain game-relative paths, never repository-relative, absolute, empty, parent-escaping, directories, or duplicate files.
- Manifest-derived files must use the exact existing `PACKAGE_FILES` safety checks, copy behavior, install behavior, and metadata emission.
- Literal `PACKAGE_FILES` and manifest-derived package files must not be mixed in the same registration.
- Public headers are not expected to change; if they do, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- `mirakana_add_desktop_runtime_game` supports an explicit manifest-derived package-file mode.
- `sample_desktop_runtime_game` packages its config, `.geindex`, and cooked payloads from `game.agent.json.runtimePackageFiles`, not duplicated literal CMake entries.
- Static JSON contract checks understand manifest-derived desktop runtime registrations and keep manifest/CMake/package recipes honest.
- Package metadata still emits selected package file source and install paths for installed validation.
- Focused package validation proves source-tree desktop runtime and installed desktop-runtime package lanes.
- Docs, plan registry, roadmap, gap analysis, manifest, Codex/Claude skills, and subagent guidance are synchronized.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have been run.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Create: this plan

- [x] **Step 1: Create this dated focused plan.**
- [x] **Step 2: Move Editor Scene/Prefab Package Authoring v0 to completed and mark this plan as active.**

### Task 2: Write The Failing Registration Proof

**Files:**
- Modify: `games/CMakeLists.txt`
- Exercise: `tools/check-json-contracts.ps1`

- [x] **Step 1: Switch `sample_desktop_runtime_game` to the new manifest-derived registration token before implementing it.**
- [x] **Step 2: Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` and confirm it fails because the registration no longer exposes literal `PACKAGE_FILES`.**

### Task 3: Implement Manifest-Derived Package Files

**Files:**
- Modify: `games/CMakeLists.txt`

- [x] **Step 1: Add an explicit `PACKAGE_FILES_FROM_MANIFEST` option to `mirakana_add_desktop_runtime_game`.**
- [x] **Step 2: Load `GAME_MANIFEST` with CMake `file(READ)` plus `string(JSON ...)`, derive repository-relative package files from `runtimePackageFiles`, and reject missing/non-array/empty/unsafe entries.**
- [x] **Step 3: Reject mixing literal `PACKAGE_FILES` with `PACKAGE_FILES_FROM_MANIFEST`.**
- [x] **Step 4: Reuse the existing package-file normalization, duplicate detection, copy, install, and metadata emission path.**

### Task 4: Update Static Contract Checks

**Files:**
- Modify: `tools/check-json-contracts.ps1`

- [x] **Step 1: Parse `PACKAGE_FILES_FROM_MANIFEST` registrations.**
- [x] **Step 2: Treat manifest-derived registrations as matching the manifest runtime package files while still rejecting literal drift, missing recipes, duplicate registrations, and unsafe manifest paths.**
- [x] **Step 3: Cover `REQUIRES_VULKAN_SHADERS` as a registration token boundary while touching parser logic.**

### Task 5: Sync Docs And AI Guidance

**Files:**
- Modify: `docs/release.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/cmake-build-system/SKILL.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Document manifest-driven package file registration honestly.**
- [x] **Step 2: Keep remaining gaps explicit: generated-game scaffolding UI, scene/prefab GUI wiring, prefab overrides, and broader authored/cooked asset conventions.**

### Task 6: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
```

- [x] **Step 2: Run required final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 3: Run `cpp-reviewer` or relevant read-only reviewer and fix actionable findings.**

## Validation Evidence

- RED confirmed before implementation: after switching `sample_desktop_runtime_game` to `PACKAGE_FILES_FROM_MANIFEST`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed because `games/sample_desktop_runtime_game/game.agent.json` declared `runtimePackageFiles` entries that were no longer declared in literal `mirakana_add_desktop_runtime_game PACKAGE_FILES`.
- Green schema proof: after adding `PACKAGE_FILES_FROM_MANIFEST` support in CMake and `tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- Static docs/agent proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after synchronizing docs, manifest, skills, and gameplay-builder subagent guidance.
- Focused desktop runtime proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed after rerunning outside the sandbox because the first sandboxed run hit the known vcpkg 7zip extraction blocker `CreateFileW stdin failed with 5 (Access is denied.)`.
- Selected package proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed after rerunning outside the sandbox for the same vcpkg extraction blocker. The install log included manifest-derived runtime files under `bin/runtime/...`, D3D12 scene/postprocess shader artifacts, installed consumer validation, installed sample smoke, and CPack ZIP generation.
- Default package regression proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` passed after rerunning outside the sandbox for the same vcpkg extraction blocker, keeping the default `sample_desktop_runtime_shell` lane green.
- Final validation proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Existing diagnostic-only host/toolchain gates remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict tidy compile database unavailable for the Visual Studio `dev` preset.
- Reviewer proof: `cpp-reviewer` found that `runtimePackageFiles` static typing, metadata-vs-manifest package file matching, D3D12 shader flag policy, and one stale testing label needed tightening. Fixed by adding explicit `runtimePackageFiles` array/string/segment checks in `tools/check-json-contracts.ps1`, metadata-to-manifest package file equality checks in `tools/common.ps1` for source and installed validation, D3D12 artifact-based flag acceptance plus selected-target DXIL requirement propagation, and the stale docs wording.
- Post-review focused validation passed again: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`. Desktop runtime/package commands required the known sandbox workaround for vcpkg 7zip extraction, then passed outside the sandbox.
