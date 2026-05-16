---
name: gameengine-cmake-build-system
description: Adds CMake targets, presets, compiler options, tests, toolchains, and vcpkg wiring in this repository. Use when editing CMakeLists, CMakePresets.json, vcpkg.json, or bootstrap scripts.
paths:
  - "CMakeLists.txt"
  - "**/CMakeLists.txt"
  - "CMakePresets.json"
  - "vcpkg.json"
  - "cmake/**"
  - "docs/building.md"
  - "tools/prepare-worktree.ps1"
  - "tools/bootstrap-deps.ps1"
  - "tools/check-toolchain.ps1"
---

# GameEngine CMake build system (Cursor)

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-cmake-build-system/SKILL.md` |
| Codex | `.agents/skills/cmake-build-system/SKILL.md` |
| Baseline | `AGENTS.md` |

Validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`, `tools/cmake.ps1`, `tools/ctest.ps1`, and related wrappers in `AGENTS.md`.

**MK_tools CMake:** add sources under `engine/tools/{shader,gltf,asset,scene}/` (`OBJECT` targets with cluster-minimal links; `MK_tools` umbrella keeps full `PUBLIC` deps); see canonical skill and `docs/specs/2026-05-11-directory-layout-target-v1.md`.

**Coverage filters:** hosted `lcov` 2.x treats unmatched remove filters as errors. Keep optional `lcovRemovePatterns` guarded by `lcov --ignore-errors unused` and update `tools/check-coverage-thresholds.ps1` with coverage filter changes.

**Static analysis:** use `tools/check-tidy.ps1`, keep `.clang-tidy` `HeaderFilterRegex` absolute-path and Windows/Linux separator aware, keep strict CI on `--warnings-as-errors=*`, prefer `-Files` for focused local TUs and `-Jobs 0` for full hosted lanes, and suppress only summary lines like `NN warnings generated.` after actionable diagnostics and exit codes are preserved.

**Worktree setup/cleanup:** After manual `git worktree add`, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` inside the linked worktree. It verifies ignored worktree roots and links an existing local `external/vcpkg` checkout plus any existing local `vcpkg_installed/` package tree. Deleting `out/` is fine. **Do not delete `external/vcpkg`** (Microsoft vcpkg clone; `CMAKE_TOOLCHAIN_FILE` in presets). If it is missing, `git clone https://github.com/microsoft/vcpkg.git external/vcpkg`, bootstrap `vcpkg.exe`, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` or `tools/bootstrap-deps.ps1`. Optional: remove `vcpkg_installed/` only when you will rerun `tools/bootstrap-deps.ps1`. Visible configure/build presets inherit `normalized-configure-environment` / `normalized-build-environment`; presets normalize raw `cmake --preset ...` PATH/Path behavior, and `tools/cmake.ps1` / `tools/ctest.ps1` additionally normalize the launched child process for local loops.
