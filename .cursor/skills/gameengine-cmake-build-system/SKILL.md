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

Validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` and related wrappers in `AGENTS.md`.

**MK_tools CMake:** add sources under `engine/tools/{shader,gltf,asset,scene}/` (`OBJECT` targets with cluster-minimal links; `MK_tools` umbrella keeps full `PUBLIC` deps); see canonical skill and `docs/specs/2026-05-11-directory-layout-target-v1.md`.

**Worktree cleanup:** Deleting `out/` is fine. **Do not delete `external/vcpkg`** (Microsoft vcpkg clone; `CMAKE_TOOLCHAIN_FILE` in presets). If it is missing, `git clone https://github.com/microsoft/vcpkg.git external/vcpkg`, bootstrap `vcpkg.exe`, then `cmake --preset dev` or `tools/bootstrap-deps.ps1`. Optional: remove `vcpkg_installed/` only when you will rerun `tools/bootstrap-deps.ps1`.
