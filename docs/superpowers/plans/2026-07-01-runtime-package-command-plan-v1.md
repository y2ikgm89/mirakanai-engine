# 2026-07-01 Runtime Package Command Plan v1

**Status:** Completed.

## Goal

Promote the `cook-runtime-package` AI command surface from manifest-only planned status to a reviewed `MK_tools` dry-run value contract. The command must validate a game manifest path, package target, runtime package file rows, selected validation recipes, backend and shader host gates, and unsupported capability claims without executing package builds or mutating files.

## Context

- The production-completion selection gate has no open `unsupportedProductionGaps`; this slice is a command-surface hardening step, not a new broad readiness claim.
- The real asset import regression corpus remains host-gated until an approved corpus is mounted. This plan is the next unblocked command contract work.
- Existing command surfaces such as `register-source-asset` and `cook-registered-source-assets` already use deterministic C++ request/result structs, fail-closed diagnostics, placeholder undo tokens, and explicit unsupported sentinels.
- Context7 was used for current CMake target guidance: add the new implementation source to the existing target instead of creating a new package surface.
- Legal/trademark review uses official public sources only as constraints: Unity legal/trademark pages, Unreal Engine EULA/branding guidance, and Godot license/trademark pages. This slice uses no external engine code, samples, schemas, assets, UI, branding, logos, or project-import behavior.

## Constraints

- `dry-run` may return planned command/evidence rows, capability gates, validation recipes, and blocked-by rows. It must not write files, launch scripts, run package builds, compile shaders, load packages, or open native/RHI handles.
- `apply` remains blocked until package cooking/execution becomes command-owned through a separate reviewed design.
- Paths must be safe, deterministic, forward-slash repository/game-relative paths with no absolute paths, drive names, dot segments, backslashes, semicolons, control characters, or alias-prone forms.
- `packageTarget` must stay compatible with the existing PowerShell recipe target policy: `^[A-Za-z_][A-Za-z0-9_]*$`.
- Smoke arguments and shader artifact requirements must be treated as typed values, not shell text.
- The result must keep legal and clean-room non-claims explicit: no legal advice, no legal approval, no external-engine compatibility/parity/equivalence/replacement claim, and no Unity/Unreal/Godot trademark or workflow imitation.

## Implementation Plan

1. Add RED tests in `MK_tools_tests` for a valid dry-run, unsafe path/argument rejection, unsupported capability rejection, and blocked apply mode.
2. Add `mirakana/tools/runtime_package_command_tool.hpp` plus `engine/tools/asset/runtime_package_command_tool.cpp`.
3. Register the source in `engine/tools/asset/CMakeLists.txt`.
4. Promote the `cook-runtime-package` manifest row to ready dry-run / blocked apply and compose `engine/agent/manifest.json`.
5. Update current capability docs, plan registry, and static AI-integration guard needles.
6. Validate with focused `MK_tools_tests`, agent/static contract checks, format checks, then full `tools/validate.ps1`.

## Done When

- `mirakana::plan_runtime_package_command` returns deterministic `GameEngine.AiCommand.CookRuntimePackage.Result.v1` rows for valid dry-run input.
- Invalid paths, unsafe typed values, native/RHI/package execution claims, arbitrary shell, free-form edits, and external-engine claims fail closed with diagnostics and unsupported gap ids.
- Manifest/docs/static checks describe exactly the ready dry-run surface and blocked apply surface.
- Validation evidence is recorded in this plan before closeout.

## Validation Evidence

| Check | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed; configured `dev` preset in this worktree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` | Passed after adding `runtime_package_command_tool.cpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests` | Passed, 1/1 test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed after `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed after moving the static guard to `check-ai-integration-154-runtime-package-command.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed, 166/166 tests. |
