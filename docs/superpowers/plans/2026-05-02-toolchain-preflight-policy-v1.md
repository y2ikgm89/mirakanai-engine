# Toolchain Preflight Policy v1 (2026-05-02)

## Goal

Make local CMake/Visual Studio toolchain readiness explicit and consistent across repository wrappers, docs, AI guidance, and the machine-readable engine manifest.

## Context

Windows PowerShell may not expose `cmake` directly on `PATH`, while the repository wrappers can still resolve Visual Studio's bundled CMake. That is useful for CI and agents, but it was under-documented and made direct `cmake --preset ...` guidance look more authoritative than the wrapper path.

Official CMake practice keeps project-wide settings in `CMakePresets.json` and local developer details in `CMakeUserPresets.json`. Microsoft documents Visual Studio Developer PowerShell/Command Prompt as the normal command-line environment for Visual Studio build tools.

## Constraints

- Do not change CMake target structure or vcpkg dependency policy.
- Do not reintroduce configure-time vcpkg package restore.
- Keep default validation through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- Keep direct CMake commands available for properly initialized shells.

## Design

Add `tools/check-toolchain.ps1` and expose it as `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`. The check reports the resolved CMake, CTest, optional CPack, Visual Studio, and MSBuild paths and enforces CMake/CTest 3.30+ for repository wrappers.

Expose `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` for the stricter direct-command path. It fails when `cmake` is not available on `PATH`, which is the right precondition before asking humans or agents to run direct `cmake --preset ...` commands.

Keep repository wrappers able to use the resolved full path to CMake so automation does not depend on a human shell profile. Document that direct CMake commands require CMake on `PATH`; on Windows that means Visual Studio Developer PowerShell/Command Prompt or official CMake 3.30+ on `PATH`.

Ignore `CMakeUserPresets.json` so local developer presets do not become project policy.

## Done When

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` reports the selected CMake/CTest/Visual Studio tools and passes on supported hosts.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` enforces direct `cmake` availability when a task needs direct CMake commands.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` includes the toolchain preflight before CMake configure/build/test work.
- README, testing/workflow docs, AGENTS guidance, CMake skills, and `engine/agent/manifest.json` describe the same policy.
- Static JSON/AI checks and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete host blockers.

## Evidence

- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` passed. It resolved Visual Studio Build Tools CMake/CTest/CPack `3.31.6`, reported MSBuild, classified the shell as `ordinary-shell`, and reported direct `cmake --preset ...` unavailable because `cmake` is not on `PATH`.
- EXPECTED BLOCKER: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1 -RequireDirectCMake` failed in the current ordinary PowerShell because direct `cmake` is not on `PATH`. The failure message points to Visual Studio Developer PowerShell/Command Prompt or official CMake 3.30+ on `PATH`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after the direct-check split. CMake configure/build completed through the resolved Visual Studio bundled CMake, CTest reported 28/28 tests passing, and diagnostic-only host gates remained Metal tools missing, Apple packaging requiring macOS/Xcode, Android release signing/device smoke not fully configured, and strict clang-tidy compile database availability.
