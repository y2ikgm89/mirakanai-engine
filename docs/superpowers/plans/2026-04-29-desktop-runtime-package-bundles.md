# Desktop Runtime Package Bundles Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let registered desktop runtime games declare runtime package files, install those files beside the selected executable, and validate the source-tree and installed bundle contract from metadata.

**Architecture:** Keep the contract target-driven: `MK_add_desktop_runtime_game` owns bundle declarations, `desktop-runtime-games.json` records source and installed relative paths, and PowerShell validation checks the selected package target. Game code remains on public `mirakana::` APIs; SDL3, OS, RHI, D3D12/Vulkan/Metal, and editor handles stay in host/backend modules.

**Tech Stack:** CMake 3.30, PowerShell validation scripts, C++23 sample code, existing `mirakana::RootedFileSystem`, and JSON manifests.

---

## Context

- Current helper: `games/CMakeLists.txt` function `MK_add_desktop_runtime_game`.
- Current metadata writer: root `CMakeLists.txt` function `MK_write_desktop_runtime_game_metadata`.
- Current validation helpers: `tools/common.ps1`.
- Current source-tree validation: `tools/validate-desktop-game-runtime.ps1`.
- Current package validation: `tools/package-desktop-runtime.ps1` and `tools/validate-installed-desktop-runtime.ps1`.
- Proof target: `games/sample_desktop_runtime_game` because it is the non-shell package target and does not require D3D12 DXIL artifacts.

## Constraints

- C++ remains C++23.
- Public game APIs remain `mirakana::`.
- Do not expose SDL3, OS window handles, GPU handles, D3D12/Vulkan/Metal handles, Dear ImGui, or editor APIs to game public APIs.
- Do not add third-party dependencies or external assets.
- Preserve headless samples and the default `sample_desktop_runtime_shell` package behavior.
- Keep this slice to package metadata, installed files, validation, docs, and one sample proof.

## Done When

- [x] `MK_add_desktop_runtime_game` accepts package file declarations and records safe repo-relative source paths plus install-relative paths.
- [x] `desktop-runtime-games.json` records `packageFiles` for each registered game.
- [x] Source-tree validation rejects unsafe/missing package file metadata.
- [x] Installed validation rejects missing or empty selected-target package files.
- [x] `sample_desktop_runtime_game` declares and loads a bundled runtime config file during package smoke validation.
- [x] Docs, roadmap, manifest, skills, and Codex/Claude subagent guidance describe package bundle metadata honestly.
- [x] Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete environment blockers are recorded.

---

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS, 7/7 focused desktop runtime tests.
- `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: PASS, installed `bin/runtime/sample_desktop_runtime_game.config` and validated `--require-config runtime/sample_desktop_runtime_game.config`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: PASS, default `sample_desktop_runtime_shell` package lane and installed DXIL artifact smoke preserved.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS; Metal and Apple packaging remain diagnostic-only host/toolchain blockers.

---

### Task 1: Static Manifest And Registration Contract

**Files:**
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `schemas/game-agent.schema.json`

- [x] **Step 1: Add a failing static check**

Add `runtimePackageFiles` to `games/sample_desktop_runtime_game/game.agent.json`:

```json
  "runtimePackageFiles": [
    "runtime/sample_desktop_runtime_game.config"
  ],
```

Add `runtimePackageFiles` as an optional array of safe relative strings in `schemas/game-agent.schema.json`.

In `tools/check-json-contracts.ps1`, extend `Get-DesktopRuntimeGameRegistrations` to parse `PACKAGE_FILES`, and assert that every manifest `runtimePackageFiles` entry for a `desktop-runtime-release` game maps to `games/<game-name>/<entry>` in the CMake registration.

- [x] **Step 2: Verify red**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: FAIL because `sample_desktop_runtime_game` declares `runtimePackageFiles` but its `MK_add_desktop_runtime_game` call does not yet declare `PACKAGE_FILES`.

- [x] **Step 3: Implement registration declaration**

Add `PACKAGE_FILES games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.config` to the `sample_desktop_runtime_game` registration in `games/CMakeLists.txt`.

- [x] **Step 4: Verify green**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: PASS.

---

### Task 2: CMake Metadata And Install Rules

**Files:**
- Modify: `games/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Create: `games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.config`

- [x] **Step 1: Write the failing metadata proof**

Run:

```powershell
cmake --preset desktop-runtime-release -DMK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET=sample_desktop_runtime_game
```

Expected before implementation: metadata is generated but does not include `packageFiles` for `sample_desktop_runtime_game`.

- [x] **Step 2: Implement metadata and install**

Update `MK_add_desktop_runtime_game` to accept `PACKAGE_FILES`, validate each file is repository-relative, under the game directory, has no parent-directory segment, and exists. Store target properties:

- `MK_DESKTOP_RUNTIME_PACKAGE_FILES`
- `MK_DESKTOP_RUNTIME_PACKAGE_FILE_INSTALL_PATHS`

Update `MK_write_desktop_runtime_game_metadata` to emit:

```json
"packageFiles": [
  {
    "source": "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.config",
    "install": "bin/runtime/sample_desktop_runtime_game.config"
  }
]
```

Install package files only for the selected package target, preserving the path below `games/<name>/` under `${CMAKE_INSTALL_BINDIR}`.

- [x] **Step 3: Verify green**

Run:

```powershell
cmake --preset desktop-runtime-release -DMK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET=sample_desktop_runtime_game
```

Expected: PASS and `out/build/desktop-runtime-release/desktop-runtime-games.json` includes the `packageFiles` entry.

---

### Task 3: Package And Installed Validation

**Files:**
- Modify: `tools/common.ps1`
- Modify: `tools/package-desktop-runtime.ps1`
- Modify: `tools/validate-desktop-game-runtime.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`

- [x] **Step 1: Write validation checks**

In `tools/common.ps1`, add package file metadata helpers that reject:

- missing `packageFiles`
- absolute `source` or `install`
- `..` segments
- source paths outside `games/<name>/`
- install paths outside `bin/`
- missing source files
- missing or empty installed files

Call source validation from `tools/validate-desktop-game-runtime.ps1` and `tools/package-desktop-runtime.ps1`. Call installed validation from `tools/validate-installed-desktop-runtime.ps1`.

- [x] **Step 2: Verify source metadata**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
```

Expected: PASS after CMake metadata emits valid source package file records.

- [x] **Step 3: Verify installed metadata**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
```

Expected: PASS after install rules copy `bin/runtime/sample_desktop_runtime_game.config`.

---

### Task 4: Runtime Sample Smoke

**Files:**
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_desktop_runtime_game/README.md`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`

- [x] **Step 1: Add the package smoke requirement**

Change `sample_desktop_runtime_game` package smoke args in `games/CMakeLists.txt` to:

```cmake
PACKAGE_SMOKE_ARGS
    --smoke
    --require-config
    runtime/sample_desktop_runtime_game.config
```

Update the game manifest package recipe to mention the bundled config validation.

- [x] **Step 2: Verify red**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
```

Expected before sample support: FAIL because `sample_desktop_runtime_game` does not accept `--require-config`.

- [x] **Step 3: Implement config loading**

Update `sample_desktop_runtime_game/main.cpp` to parse `--require-config <path>`, root file access at the executable directory through `mirakana::RootedFileSystem`, read the file, and fail with exit code `4` when it is missing or empty. Keep normal source-tree `--smoke` behavior unchanged.

- [x] **Step 4: Verify green**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
```

Expected: PASS.

---

### Task 5: Documentation And Agent Guidance

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/release.md`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/dependencies.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.agents/skills/gameengine-agent-integration/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Synchronize docs and AI contracts**

Update docs and manifests to state that registered desktop runtime games can declare package files, metadata records source/install paths, validation checks source and installed bundles, and `sample_desktop_runtime_game` proves a bundled runtime config without D3D12 shader artifacts.

- [x] **Step 2: Verify agent-facing contracts**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: PASS.

---

### Task 6: Final Validation

**Files:**
- No new files.

- [x] **Step 1: Run focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: PASS. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` is conservative because this slice touches package/manifest behavior but not public C++ headers.

- [x] **Step 2: Run final validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: PASS, with host-gated Metal/Apple/mobile diagnostics remaining diagnostic-only where applicable.
