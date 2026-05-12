# Generated Desktop Runtime Package Scaffold v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1` generate a package-ready desktop runtime game scaffold whose `game.agent.json.runtimePackageFiles`, CMake registration, runtime config, and validation recipes are aligned from creation.

**Architecture:** Keep generation in repository tooling and generated sample/game files. Generated desktop runtime games use `mirakana::SdlDesktopGameHost` with deterministic `NullRenderer` fallback and first-party package/config validation, while package file ownership stays in `game.agent.json.runtimePackageFiles` plus `PACKAGE_FILES_FROM_MANIFEST`. Do not expose SDL3, native window handles, RHI handles, Dear ImGui, editor APIs, or backend interop to public gameplay code.

**Tech Stack:** Existing PowerShell generator/check scripts, `games/CMakeLists.txt`, `game.agent.json`, desktop runtime package validation, C++23 sample game code, no new third-party dependencies.

---

## Context

Manifest-Driven Desktop Runtime Package Authoring v0 made `runtimePackageFiles` the source of truth for registered desktop runtime package files. The remaining generated-game drift is that `tools/new-game.ps1` only creates headless `mirakana_add_game` projects. AI-generated desktop games still need manual CMake registration, package file lists, config files, and validation recipes. A production authoring loop needs a scaffold mode that emits a desktop runtime package lane correctly on day one.

## Constraints

- No new dependencies.
- Do not change public gameplay APIs.
- Do not expose SDL3, OS, native window, RHI, GPU, Dear ImGui, editor, or backend handles to game public APIs.
- Keep the default `new-game` behavior headless and compatible with current docs.
- Generated package files must be game-relative manifest entries and consumed through `PACKAGE_FILES_FROM_MANIFEST`.
- v0 does not generate shader artifacts, require D3D12/Vulkan GPU paths, or claim visible GPU rendering.
- v0 may validate a first-party runtime config and package lane, but authored scene cooking and full material/scene editor UX remain follow-up work.
- Public headers are not expected to change; if they do, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- `tools/new-game.ps1` supports a desktop runtime package scaffold mode while keeping the default headless mode.
- The desktop scaffold generates `main.cpp`, `README.md`, `game.agent.json`, `runtime/<game>.config`, and CMake registration using `mirakana_add_desktop_runtime_game(... PACKAGE_FILES_FROM_MANIFEST)`.
- Generated desktop manifests declare honest `desktop-game-runtime` and `desktop-runtime-release` validation recipes plus `runtimePackageFiles`.
- Static AI integration checks exercise both headless and desktop package scaffold modes without polluting the repository.
- Docs, plan registry, roadmap/gap docs, manifest, Codex/Claude skills, and relevant subagent guidance are synchronized.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have been run.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Create: this plan

- [x] **Step 1: Create this dated focused plan.**
- [x] **Step 2: Move Manifest-Driven Desktop Runtime Package Authoring v0 to completed and mark this plan as active.**

### Task 2: Write The Failing Scaffold Contract

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Exercise: `tools/new-game.ps1`

- [x] **Step 1: Add a check that `new-game.ps1 -Template DesktopRuntimePackage` can dry-run or generate into an isolated test root.**
- [x] **Step 2: Confirm `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` fails before implementation because the template parameter/scaffold contract does not exist yet.**

### Task 3: Implement Desktop Runtime Package Scaffold

**Files:**
- Modify: `tools/new-game.ps1`
- Modify: `games/CMakeLists.txt` only if a stable generation anchor is needed.

- [x] **Step 1: Add a template selector while preserving default headless output.**
- [x] **Step 2: Generate desktop runtime package `main.cpp` using public `mirakana::` host/runtime contracts and `NullRenderer` fallback.**
- [x] **Step 3: Generate `runtime/<game>.config`, manifest `runtimePackageFiles`, package targets, and validation recipes.**
- [x] **Step 4: Append `mirakana_add_desktop_runtime_game` registration with `GAME_MANIFEST`, smoke args, package smoke args, and `PACKAGE_FILES_FROM_MANIFEST`.**

### Task 4: Harden Scaffold Checks

**Files:**
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Validate generated headless scaffolds still use `mirakana_add_game`.**
- [x] **Step 2: Validate generated desktop scaffolds include runtime config, manifest package file entries, desktop package recipes, and manifest-derived CMake registration.**
- [x] **Step 3: Ensure checks run in a disposable workspace so the repository is not dirtied.**

### Task 5: Sync Docs And AI Guidance

**Files:**
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `docs/specs/game-template.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Document the new scaffold mode honestly.**
- [x] **Step 2: Keep non-goals explicit: shader generation, authored scene cooking, visible GPU proof, editor UX, and mobile package lanes remain separate.**

### Task 6: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
```

- [x] **Step 2: Run required final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 3: Run `cpp-reviewer` or relevant read-only reviewer and fix actionable findings.**

## Validation Evidence

- RED confirmed before implementation: after adding the scaffold contract to `tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed because `tools/new-game.ps1` did not support `-RepositoryRoot`; after moving the disposable test root from `C:\tmp` to repository `out/new-game-scaffold-checks`, the failure isolated the missing generator contract.
- Static scaffold proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after implementation, creating and validating disposable headless and `DesktopRuntimePackage` scaffolds without polluting the repository.
- Static docs/contract proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed after docs, manifest, Codex/Claude skills, and gameplay-builder guidance were synchronized.
- Compile/runtime proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed after rerunning outside the sandbox because the first sandboxed run hit the known vcpkg 7zip extraction blocker `CreateFileW stdin failed with 5 (Access is denied.)`. The focused lane built `sample_generated_desktop_runtime_package` and passed 10/10 tests including `sample_generated_desktop_runtime_package_smoke`.
- Selected generated package proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_package` passed after rerunning outside the sandbox for the same vcpkg extraction blocker. Installed validation copied `bin/runtime/sample-generated-desktop-runtime-package.config`, ran metadata-selected `--require-config` smoke, validated the installed manifest/package files, and produced the CPack ZIP.
- Default package regression proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` passed after rerunning outside the sandbox for the same vcpkg extraction blocker, keeping the default `sample_desktop_runtime_shell` package lane green.
- Final validation proof: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Existing diagnostic-only host/toolchain gates remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict tidy compile database unavailable for the Visual Studio `dev` preset.
- Reviewer proof: `cpp-reviewer` found actionable issues in generated display-name escaping, generated CMake target collision handling, desktop runtime validation coverage for the new generated sample, disposable check cleanup bounds, and Codex/Claude skill drift. The implementation now rejects control characters, C++-escapes generated titles, preserves raw config display names safely, rejects existing `mirakana_add_game` / `mirakana_add_desktop_runtime_game` target collisions, pins `sample_generated_desktop_runtime_package` in `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, bounds disposable cleanup to the scaffold check workspace, and re-synchronizes the Claude game-development skill.
