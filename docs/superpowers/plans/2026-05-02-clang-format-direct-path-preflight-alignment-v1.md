# Clang-Format Direct PATH Preflight Alignment v1 (2026-05-02)

## Goal

Keep raw `clang-format --dry-run ...` PATH failures distinguishable from real formatting violations across docs, skills, rules, subagents, manifest guidance, and validation checks.

## Context

This chat started from a report that raw `clang-format --dry-run` could not run in the active PowerShell because `clang-format` was not on `PATH`. Investigation found that Visual Studio Build Tools LLVM provided `clang-format.exe` at `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\Llvm\x64\bin\clang-format.exe`, while repository wrappers could already resolve that tool through `tools/common.ps1`. A user-local shim at `C:\Users\y2ikg\.local\bin\clang-format.cmd` made raw `clang-format` available in the active PATH without changing repository files.

## Constraints

- Do not change CMake target structure, C++ source behavior, vcpkg policy, or formatting style.
- Do not treat a raw PATH lookup failure as a code formatting violation.
- Keep `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` as the repeatable repository formatting entrypoints.
- Keep Codex and Claude Code guidance behaviorally equivalent.
- Keep `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` as the tool discovery entrypoint.

## Done When

- `toolchain-check` reports the resolved formatter and `direct-clang-format-status`.
- AGENTS, README/docs, Codex/Claude CMake skills, agent-integration skills, Claude rules, build-fixer subagents, and `engine/agent/manifest.json` describe the same raw-vs-wrapper policy.
- `tools/check-ai-integration.ps1` statically checks the synchronized guidance.
- Raw `clang-format` smoke, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass.

## Evidence

- RED: `Get-Command clang-format -All` failed in the active PowerShell before the user-local shim because `clang-format` was not on `PATH`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` reached formatter execution through the repository resolver, proving wrapper resolution was not blocked by PATH. It then failed on pre-existing repository-wide formatting violations, which is a separate formatting state.
- GREEN: `C:\Users\y2ikg\.local\bin\clang-format.cmd` now resolves raw `clang-format` to Visual Studio Build Tools LLVM `clang-format.exe` version `19.1.5`.
- GREEN: raw `clang-format --dry-run --Werror` smoke passed on a temporary formatted file under `out/`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` reported `direct-clang-format-status=ready`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with 28/28 CTest entries.
