# Desktop Runtime Package Registration Apply Tool v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a safe repository tool that applies reviewed desktop runtime package file entries to `game.agent.json.runtimePackageFiles`.

**Architecture:** Keep JSON mutation in PowerShell tooling, not in `mirakana_editor_core` or the visible editor shell. Reuse the desktop runtime path normalization rules from `tools/common.ps1`, keep `runtimePackageFiles` game-relative, and rely on the existing CMake/schema/package validation lanes for final manifest correctness.

**Tech Stack:** Existing PowerShell validation tools, `ConvertFrom-Json` / `ConvertTo-Json`, `tools/common.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`, and default `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`. No new third-party dependencies.

---

## Context

Editor Package Registration Draft Workflow v0 gives authors a review-only list of runtime files that should be added, skipped, or rejected. The next production gap is a host-feasible apply workflow that can safely update a game manifest after those entries are reviewed, while keeping C++ editor/core free of JSON mutation and keeping package validation authoritative.

## Constraints

- No new dependencies.
- Do not add JSON parsing or writing to C++ editor/core.
- Do not invoke CMake, package scripts, or shell commands from `mirakana_editor`.
- Do not mutate existing committed game manifests during validation; use disposable manifests under temporary scaffold roots.
- Do not accept repository-relative `games/...`, absolute, parent-traversal, empty, directory, missing, duplicate, or CMake-list-separator package entries.
- Preserve existing manifest fields and append only normalized, game-relative `runtimePackageFiles` entries.

## Done When

- A new tool updates `game.agent.json.runtimePackageFiles` from caller-supplied entries with deterministic normalization, duplicate handling, and clear diagnostics.
- Static and disposable dynamic `agent-check` coverage proves add, already-present, invalid path, missing file, and duplicate handling without changing committed samples.
- Docs, roadmap, gap analysis, manifest, Codex/Claude guidance, and relevant subagents are synchronized.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass with existing host/toolchain gates recorded.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Create: this plan

- [x] **Step 1: Create this dated focused plan.**
- [x] **Step 2: Move Editor Package Registration Draft Workflow v0 to completed and mark this plan as active.**
- [x] **Step 3: Keep gap analysis focused on manifest apply tooling, not editor JSON mutation.**

### Task 2: Write The Failing Tool Contract

**Files:**
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Add static checks for `tools/register-runtime-package-files.ps1`.**
- [x] **Step 2: Add a disposable manifest test that creates `games/package-apply-game/game.agent.json` and runtime files under a temporary scaffold root.**
- [x] **Step 3: Assert the new tool adds normalized runtime entries, keeps existing entries once, and rejects unsafe or missing entries.**
- [x] **Step 4: Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and confirm RED because the tool is missing.**

### Task 3: Implement The PowerShell Apply Tool

**Files:**
- Create: `tools/register-runtime-package-files.ps1`

- [x] **Step 1: Add parameters `-GameManifest`, `-RuntimePackageFile`, optional `-RepositoryRoot`, and `-DryRun`.**
- [x] **Step 2: Dot-source `tools/common.ps1` and resolve the manifest under the repository root.**
- [x] **Step 3: Validate `games/<name>/game.agent.json`, normalize entries with `ConvertTo-DesktopRuntimeMetadataRelativePath`, reject `games/`, `;`, missing files, directories, and duplicates using `ConvertTo-DesktopRuntimePathKey`.**
- [x] **Step 4: Preserve existing manifest fields, create `runtimePackageFiles` when absent, append only new normalized entries, and write deterministic JSON unless `-DryRun` is set.**
- [x] **Step 5: Print stable summary lines for added, already-present, and skipped entries.**

### Task 4: Wire Disposable Validation

**Files:**
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Exercise the tool on a disposable manifest with one existing runtime file and two new runtime files.**
- [x] **Step 2: Exercise idempotence by running the same add command twice and asserting no duplicate entries.**
- [x] **Step 3: Exercise invalid entries in separate disposable calls and assert non-zero exit for `../escape.txt`, `games/package-apply-game/runtime/file.txt`, missing file, empty path, absolute path, directory, CMake list separator, and duplicate request inputs.**
- [x] **Step 4: Keep temporary roots removed through the existing scaffold cleanup pattern.**

### Task 5: Sync Docs, Manifest, Skills, And Subagents

**Files:**
- Modify: `docs/workflows.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/release.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Document the tool as the manifest apply lane after editor draft review.**
- [x] **Step 2: Keep non-goals explicit: no editor-shell command execution, no C++ JSON writer, no package build invocation, and no automatic prefab override workflow.**
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
```

- [x] **Step 2: Run package validation for the default desktop runtime lane.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
```

- [x] **Step 3: Run required final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 4: Run `cpp-reviewer` or `build-fixer` if validation exposes C++ or build concerns; otherwise use read-only architect/explorer review already completed for the tooling boundary.**

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` initially failed because `tools/register-runtime-package-files.ps1` was missing.
- GREEN focused validation after implementation and duplicate-request hardening:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- Package validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: first sandbox attempt failed with the known vcpkg 7zip `CreateFileW stdin failed with 5 (Access is denied.)` host/sandbox gate.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` rerun with escalated permissions: PASS; 7/7 desktop-runtime-release CTest tests passed, installed desktop runtime validation passed for `sample_desktop_runtime_shell`, and CPack generated `GameEngine-0.1.0-Windows-AMD64.zip` plus `.sha256`.
- Reviewer:
  - `cpp-reviewer` found plan evidence drift and duplicate request acceptance. Duplicate request handling now fails in the tool and is covered by disposable `agent-check`; plan evidence is updated.
- Final validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS; 22/22 dev CTest tests passed.
  - Existing diagnostic-only host gates remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode `xcodebuild`/`xcrun`, Android release signing not configured, Android device smoke not connected, and strict clang-tidy remains diagnostic-only because the Visual Studio dev preset compile database path is unavailable before configure.
