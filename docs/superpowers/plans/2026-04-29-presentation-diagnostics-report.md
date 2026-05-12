# Presentation Diagnostics Report Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a backend-neutral presentation diagnostics report for SDL3 desktop game windows so games, samples, package validation, and AI agents can deterministically inspect requested backend, selected backend, fallback stage, fallback reason, diagnostic counts, and scene GPU binding status without native handles.

**Architecture:** Keep all SDL3, Win32, D3D12, Vulkan, swapchain, and `IRhiDevice` details private to `mirakana_runtime_host_sdl3_presentation`. Expose only first-party value types and spans over immutable report rows through `SdlDesktopPresentation` and `SdlDesktopGameHost`.

**Tech Stack:** C++23, `mirakana_runtime_host_sdl3_presentation`, SDL3 dummy and Windows presentation tests, desktop runtime/package validation, API boundary validation.

---

## Context

- D3D12 and Vulkan SDL3 game-window presentation paths exist and can fall back to `NullRenderer` with flat diagnostics.
- Scene GPU binding status and stats are already exposed separately, but there is no single deterministic summary for requested/selected backend and fallback stage.
- Package and generated-game tooling needs backend-neutral readiness data without parsing renderer names or native backend messages.

## Constraints

- Do not expose SDL, Win32, D3D12, Vulkan, Metal, swapchain, RHI-device, or Dear ImGui handles.
- Do not change default D3D12-first behavior or strict Vulkan/D3D12 smoke semantics.
- Keep this slice limited to reporting and forwarding. Do not add Vulkan scene GPU parity, GPU markers, frame profiler UI, or renderer feature work.
- Do not add third-party dependencies.

## Done When

- [x] `SdlDesktopPresentation` exposes a deterministic value summary for requested backend, selected backend, fallback usage, fallback reason, scene GPU status, and diagnostic/report counts.
- [x] `SdlDesktopPresentation` exposes backend report rows that identify backend-specific readiness/failure stage without native details.
- [x] `SdlDesktopGameHost` forwards the report and rows.
- [x] Unit tests cover dummy D3D12 fallback, invalid Vulkan shader fallback, Vulkan native-surface fallback, not-requested fallback, and game-host forwarding.
- [x] Public API compile coverage proves the new value types and name helpers.
- [x] Docs, roadmap, gap analysis, manifest, skills, and Codex/Claude guidance describe the report honestly.
- [x] Focused runtime-host validation, desktop runtime validation, package validation, API boundary check, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Presentation Report Tests

**Files:**
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_public_api_compile.cpp`

- [x] Add tests for missing D3D12 bytecode reporting `requested_backend=d3d12`, `selected_backend=null_renderer`, `used_null_fallback=true`, and pipeline-unavailable backend report.
- [x] Add tests for missing Vulkan SPIR-V and dummy Vulkan surface fallback report rows.
- [x] Add tests that `SdlDesktopGameHost` forwards report summary and backend rows.
- [x] Add compile-only coverage for value types and report status name helpers.

### Task 2: Public API And Implementation

**Files:**
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp`
- Modify: `engine/runtime_host/sdl3/include/mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_presentation.cpp`
- Modify: `engine/runtime_host/sdl3/src/sdl_desktop_game_host.cpp`

- [x] Add first-party report enums/structs for backend report stage and summary.
- [x] Track requested backend and null-fallback policy in `SdlDesktopPresentation::Impl`.
- [x] Record one backend report row for each selected backend attempt and preserve existing diagnostics.
- [x] Forward report access through `SdlDesktopGameHost`.

### Task 3: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/architecture.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/workflows.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-rendering/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify: `.claude/agents/rendering-auditor.md`

- [x] Document the diagnostics report as implemented backend-neutral metadata, not as GPU timing/profiling.
- [x] Keep Vulkan scene GPU parity and Metal presentation as future/host-gated work.
- [x] Fix stale Vulkan active-slice references in the gap analysis.

### Task 4: Verification

- [x] Run focused SDL3 runtime-host build/tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run relevant package validation.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

- Focused build: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests mirakana_runtime_host_sdl3_public_api_compile sample_desktop_runtime_shell sample_desktop_runtime_game` PASS. Direct `& $tools.CMake ...` initially hit a host PowerShell/MSBuild environment issue because the process contained both `PATH` and `Path`; the repo-standard `Invoke-CheckedCommand` environment normalization path built successfully and the build-fixer confirmed this was not caused by report code.
- Focused CTest: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset desktop-runtime --output-on-failure -R "mirakana_runtime_host_sdl3_tests|mirakana_runtime_host_sdl3_public_api_compile|sample_desktop_runtime_shell(_vulkan_shader_artifacts)?_smoke|sample_desktop_runtime_game_smoke"` PASS, 5/5 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: diagnostic-only PASS; D3D12 DXIL and Vulkan SPIR-V are ready through `C:\VulkanSDK\1.4.341.1\Bin\dxc.exe` and `spirv-val.exe`, while Metal `metal` and `metallib` are missing on this Windows host.
- Strict source-tree Vulkan smoke: `out\build\desktop-runtime\games\Debug\sample_desktop_runtime_shell.exe --smoke --video-driver windows --require-vulkan-shaders --require-vulkan-renderer` PASS with `presentation_selected=vulkan`, `presentation_backend_report=vulkan:ready:none`, `renderer=vulkan`, `frames=2`, and `game_frames=2`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS after the known sandbox vcpkg 7zip `CreateFileW stdin failed with 5` blocker was rerun with approved escalation; 9/9 tests passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: PASS after the same sandbox vcpkg escalation. Installed shell smoke output included `presentation_selected=null`, `presentation_backend_reports=1`, and an installed validation check for presentation report fields.
- Selected package validation: `tools\package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` PASS after the same sandbox vcpkg escalation. Installed game smoke output included `presentation_selected=d3d12`, `presentation_backend_report=d3d12:ready:none`, `scene_gpu_status=ready`, `scene_gpu_mesh_resolved=2`, and `scene_gpu_material_resolved=2`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Diagnostic-only blockers remain Metal shader/library tools missing, Apple packaging missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict tidy compile database unavailable for the active generator.
