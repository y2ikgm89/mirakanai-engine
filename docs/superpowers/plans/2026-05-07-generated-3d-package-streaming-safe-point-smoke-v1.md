# Generated 3D Package Streaming Safe Point Smoke v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow generated `DesktopRuntime3DPackage` smoke proof that the selected packaged scene can pass runtime-scene validation, load through the first-party runtime package loader, and commit through host-gated safe-point package streaming without exposing native/RHI handles or claiming background streaming.

**Architecture:** Reuse `mirakana::runtime::execute_selected_runtime_package_streaming_safe_point` from `mirakana_runtime`. The committed and generated 3D package executable will accept `--require-package-streaming-safe-point`, build a descriptor from the manifest-aligned packaged scene target, execute the safe-point commit against a host-owned `RuntimeAssetPackageStore` / `RuntimeResourceCatalogV2`, and emit first-party `package_streaming_*` status fields on the existing package smoke line.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_runtime_host_sdl3`, generated `DesktopRuntime3DPackage`, PowerShell validation scripts, CMake registered desktop runtime game metadata.

---

## Context

- Master plan: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Current gap: `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps[3d-playable-vertical-slice]` still lists `broad dependency cooking and package streaming` as a required-before-ready claim.
- Existing package streaming API: `engine/runtime/include/mirakana/runtime/package_streaming.hpp`.
- Existing generated 3D sample: `games/sample_generated_desktop_runtime_3d_package`.
- Existing generated 3D manifest already declares `packageStreamingResidencyTargets` with `mode=host-gated-safe-point`.

## Constraints

- Keep generated gameplay on public `mirakana::` APIs and cooked runtime packages only.
- Keep safe-point streaming host-owned in the game executable; do not expose `IRhiDevice`, descriptor handles, native handles, background streaming threads, eviction commands, allocator handles, GPU budgets, or renderer quality claims.
- Treat this as a selected package-smoke proof only: no runtime source parsing, broad dependency cooking, async/background streaming, texture streaming, package streaming execution for 2D, or Metal readiness.
- Update generated template output and committed sample together.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before completion reporting.

## Done When

- `sample_generated_desktop_runtime_3d_package --smoke ... --require-package-streaming-safe-point` reports:
  - `package_streaming_status=committed`
  - `package_streaming_ready=1`
  - positive `package_streaming_resident_bytes`
  - positive `package_streaming_committed_records`
  - `package_streaming_diagnostics=0`
- `tools/validate-installed-desktop-runtime.ps1` verifies those fields whenever smoke args include `--require-package-streaming-safe-point`.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` emits the same option, counters, manifest recipe, and docs text.
- `games/sample_generated_desktop_runtime_3d_package/game.agent.json`, `games/CMakeLists.txt`, docs, static checks, and skills distinguish this narrow safe-point proof from broad/background package streaming.
- Focused build/smoke validation, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete host/tool blockers.

## Tasks

### Task 1: RED

- [x] Run the committed generated 3D package smoke with `--require-package-streaming-safe-point` before implementation.
- [x] Record the expected failure: the argument is unknown or required `package_streaming_*` fields are absent.

### Task 2: Implement Committed Sample

- [x] Add `mirakana/runtime/package_streaming.hpp` and `mirakana/runtime/resource_runtime.hpp` to `games/sample_generated_desktop_runtime_3d_package/main.cpp`.
- [x] Add `DesktopRuntimeOptions::require_package_streaming_safe_point` and parse `--require-package-streaming-safe-point`.
- [x] Execute `mirakana::runtime::execute_selected_runtime_package_streaming_safe_point` after the required scene package is loaded and validated.
- [x] Emit `package_streaming_*` fields on the existing status line and fail smoke when required fields do not prove a committed safe-point replacement.

### Task 3: Validation Script and Metadata

- [x] Update `tools/validate-installed-desktop-runtime.ps1` so installed validation enforces the new fields when `--require-package-streaming-safe-point` is present.
- [x] Add the requirement to `games/CMakeLists.txt` package smoke args for `sample_generated_desktop_runtime_3d_package`.
- [x] Update `games/sample_generated_desktop_runtime_3d_package/game.agent.json` validation recipes and manifest text.

### Task 4: Generated Template

- [x] Update `tools/new-game.ps1` so generated `DesktopRuntime3DPackage` games contain the same code, recipe args, manifest fields, and README text.

### Task 5: Docs and Static Checks

- [x] Update `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, this registry, the master plan, `engine/agent/manifest.json`, and relevant skills.
- [x] Update `tools/check-ai-integration.ps1` to require the new safe-point package streaming markers for generated 3D package scaffolds.

### Task 6: Validate and Commit

- [x] Run focused build/test and package smoke.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before the slice-closing commit.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED generated 3D package smoke with `--require-package-streaming-safe-point` | FAIL as expected | `out/build/desktop-runtime/.../sample_generated_desktop_runtime_3d_package.exe --smoke ... --require-package-streaming-safe-point` exited 1 with `unknown argument: --require-package-streaming-safe-point`. |
| Focused build/test | PASS | `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package` passed. Source-tree smoke with `--require-package-streaming-safe-point` exited 0 and reported `package_streaming_status=committed`, `package_streaming_ready=1`, `package_streaming_resident_bytes=5436`, `package_streaming_committed_records=9`, and `package_streaming_diagnostics=0`. |
| Installed D3D12 package smoke | PASS | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passed and installed validation reported `package_streaming_status=committed`, `package_streaming_ready=1`, `package_streaming_resident_bytes=5436`, `package_streaming_committed_records=9`, and `package_streaming_diagnostics=0`. |
| Installed Vulkan package smoke | PASS | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -RequireVulkanShaders -SmokeArgs ... --require-package-streaming-safe-point --require-vulkan-renderer ...` passed and installed validation reported the same committed package-streaming counters on the Vulkan path. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | `format-check: ok`. |
| `git diff --check` | PASS | No whitespace errors; Git reported only CRLF conversion warnings for touched and pre-existing dirty files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | `validate: ok`; CTest reported `100% tests passed, 0 tests failed out of 29`. Metal/iOS diagnostics remain host-gated on this Windows machine. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | `tools/build.ps1` completed the `dev` preset build with exit 0. |
