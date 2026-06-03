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

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-cmake-build-system/SKILL.md` |
| Codex | `.agents/skills/cmake-build-system/SKILL.md` |
| Scoped rule | `.cursor/rules/mirakana-cmake-vcpkg.mdc` |
| Baseline | `AGENTS.md` |

Read the Claude skill for CMake/vcpkg/MSVC/static-analysis procedures. Use `tools/check-toolchain.ps1`, `tools/cmake.ps1`, and `tools/ctest.ps1` for local loops.

For performance flags, ISA-specific code, PGO, LTO, or release-performance presets, also read `.cursor/skills/gameengine-performance-optimization/SKILL.md`.
