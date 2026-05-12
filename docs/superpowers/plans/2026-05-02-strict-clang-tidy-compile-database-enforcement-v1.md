# Strict Clang-Tidy Compile Database Enforcement v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict` enforce compile database availability on the default Windows `dev` preset by generating a clang-tidy-compatible compile database from CMake File API codemodel data when the active generator does not emit `compile_commands.json`.

**Architecture:** Keep the slice in repository validation tooling and agent/docs surfaces. `tools/check-tidy.ps1` remains the clang-tidy entrypoint: it verifies `.clang-tidy`, requests CMake File API codemodel data, configures the selected preset when needed, synthesizes `out/build/<preset>/compile_commands.json` for Visual Studio generator builds, and then runs clang-tidy over repository C++ sources. No engine runtime behavior, public `mirakana::` API, renderer/RHI, editor productization, package cooking, or dependency policy changes are included.

**Tech Stack:** PowerShell 7, CMake 3.30+ File API codemodel v2, Visual Studio generator metadata, clang-tidy, existing repository validation scripts, docs/manifest synchronization, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Next-Production-Gap Selection

- Latest completed slice: `docs/superpowers/plans/2026-05-02-animation-cpu-skinned-bitangent-handedness-v1.md`.
- `engine/agent/manifest.json` routes through `recommendedNextPlan.id = next-production-gap-selection` and names strict clang-tidy compile-database enforcement as one candidate.
- Candidate investigation confirmed the default `dev` preset sets `CMAKE_EXPORT_COMPILE_COMMANDS=ON`, but the existing `out/build/dev` uses `Visual Studio 17 2022`; CMake's native compile database export is implemented only for Makefile and Ninja generators, so Visual Studio ignores it.
- CMake File API codemodel data is available after a codemodel query and includes `compileGroups`, `compileCommandFragments`, `includes`, and source indexes for the Visual Studio generator.
- Selected focused slice: synthesize a clang-tidy-compatible compile database from File API codemodel data for the default Windows `dev` preset and make `-Strict` fail only on real missing tool/configuration issues or clang-tidy failures, not on Visual Studio generator compile database absence.

## Goal

Resolve the diagnostic-only `compile_commands.json missing for preset dev` blocker for the Windows Visual Studio generator path without changing the engine build generator, requiring a developer shell, adding third-party dependencies, or broadening renderer/game/editor readiness claims.

## Context

- `tools/check-tidy.ps1` currently verifies `.clang-tidy`, then exits diagnostic-only when `out/build/dev/compile_commands.json` is absent.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` runs `check-tidy.ps1` before `tools/build.ps1`, so a fresh checkout needs `tidy-check` to be able to configure or generate its own compile database input.
- `tools/common.ps1` already resolves CMake, clang-tidy, Visual Studio Build Tools, and MSBuild, and repository wrappers use resolved tool paths instead of requiring direct commands.
- CMake File API can be requested from the build tree and produces codemodel reply files for Visual Studio generator builds.

## Constraints

- Keep the change in validation tooling, docs, and manifest/guidance only.
- Do not change C++ runtime behavior, public `mirakana::` APIs, CMake target graph semantics, dependency bootstrap policy, vcpkg features, renderer/RHI code, editor UI/productization, package scripts, or game templates.
- Do not mark Apple/iOS/Metal, production renderer quality, editor productization, production UI/importer/platform adapters, broad generated 3D readiness, or GPU skinning/upload as ready.
- Do not add third-party dependencies.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` without `-Strict` must keep reporting clear diagnostic blockers when clang-tidy or CMake is unavailable.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -MaxFiles 1` must move from the compile database missing RED failure to a GREEN pass on this Windows host.

## Done When

- [x] RED evidence shows `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -MaxFiles 1` fails because `compile_commands.json` is missing for the Visual Studio `dev` preset.
- [x] `tools/check-tidy.ps1` requests CMake File API codemodel data for the selected preset/configuration when no native `compile_commands.json` exists.
- [x] `tools/check-tidy.ps1` configures the selected preset through the resolved CMake wrapper when File API replies or native compile commands are missing.
- [x] `tools/check-tidy.ps1` synthesizes `compile_commands.json` with stable `arguments` rows for repository C++ translation units from Visual Studio codemodel `compileGroups`.
- [x] Strict mode fails if the compile database still cannot be produced, and non-strict mode keeps a diagnostic-only blocker for missing CMake or clang-tidy tools.
- [x] `docs/superpowers/plans/README.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, and `engine/agent/manifest.json` describe strict clang-tidy compile database enforcement as ready for the default Windows Visual Studio `dev` preset.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -MaxFiles 1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have PASS evidence or exact environment blockers recorded here.
- [x] Registry and manifest are re-read after completion to select the next focused slice or stop on host-gated/broad-only work.

## Implementation Tasks

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -MaxFiles 1` and record the expected RED compile database missing failure.
- [x] Add small path/argument helpers to `tools/check-tidy.ps1` for safe absolute path normalization, command-fragment tokenization, CMake File API query creation, reply index discovery, and diagnostic handling.
- [x] Add `Ensure-ClangTidyCompileDatabase` to `tools/check-tidy.ps1` so it first accepts native `compile_commands.json`, then requests/configures File API data, then writes a synthesized compile database.
- [x] Update the existing file collection path to call `Ensure-ClangTidyCompileDatabase` before reading entries.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -MaxFiles 1` and fix the synthesized command rows until clang-tidy accepts the first translation unit.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` to confirm the non-strict wrapper uses the same generated database path without reporting the old blocker.
- [x] Synchronize docs/registry/manifest and any static guidance that refers to strict clang-tidy as compile-database-gated.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record final validation evidence and next-step decision in this plan.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -MaxFiles 1` before implementation | RED | Failed as expected with `tidy-check: blocker - compile_commands.json missing for preset 'dev' at G:\workspace\development\GameEngine\out\build\dev\compile_commands.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after adding static guidance requirements | RED | Failed as expected because `AGENTS.md` did not yet contain `CMake File API`, confirming the agent-facing guidance sync was missing before implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after adding validate smoke requirement | RED | Failed as expected because `tools/validate.ps1` did not yet contain `-MaxFiles 1`, confirming default validation still tried to run full clang-tidy once compile database generation became ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -MaxFiles 1` | PASS | `check-tidy.ps1` configured the `dev` preset, generated `compile_commands.json` from CMake File API for configuration `Debug` with 155 files, ran clang-tidy on 1 file, and reported `tidy-check: ok (1 files)`. Existing profile warnings in `engine/ai/src/behavior_tree.cpp` are warnings, not errors, and are not part of this compile-database slice. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1` | PASS | Non-strict mode used the generated File API compile database path and reported `tidy-check: ok (1 files)` instead of the old compile database blocker. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok` after AGENTS/docs/skills/subagents/manifest/static checks were synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `json-contract-check: ok` after manifest host-gate and next-plan sync. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | PASS | `validation-recipe-runner-check: ok` after default recipe no longer reported `tidy-compile-database` as a host gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; default validation generated a 155-file clang-tidy compile database from CMake File API, ran the `-MaxFiles 1` tidy smoke, built the Visual Studio `dev` preset, and CTest reported 28/28 tests passed. Diagnostic-only gates remain Metal shader/library tools missing on this Windows host, Apple packaging blocked by no macOS/Xcode, Android release signing not configured, and Android device smoke not connected. |

## Next-Step Decision

After completion, `docs/superpowers/plans/README.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, and `engine/agent/manifest.json` were re-read and synchronized. `tidy-compile-database` is now ready for the default Windows Visual Studio `dev` validation path; remaining next-production-gap candidates are either broad and need a new dated focused plan before implementation (`3d-playable-vertical-slice`, `editor-productization`, `production-ui-importer-platform-adapters`, full clang-tidy warning cleanup if selected) or host-gated (`Apple/iOS/Metal` on macOS/Xcode/metal/metallib). The manifest returns `recommendedNextPlan` to blocked `next-production-gap-selection` with those reasons.

## Non-Goals

- Changing the default CMake generator or requiring a Visual Studio developer shell.
- Making clang-tidy warnings into errors beyond the existing `.clang-tidy` profile policy.
- Full repository static-analysis cleanup if clang-tidy finds pre-existing warnings.
- C++ runtime behavior changes, public API changes, renderer/RHI work, package cooking, game scaffold behavior, dependency additions, or third-party license changes.
- Apple/iOS/Metal readiness, Vulkan readiness beyond existing host gates, editor productization, production UI/importer/platform adapters, broad generated 3D production readiness, GPU skinning/upload, package streaming, public native/RHI handles, or general renderer quality.
