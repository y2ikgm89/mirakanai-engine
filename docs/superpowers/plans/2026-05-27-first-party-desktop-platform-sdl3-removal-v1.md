# 2026-05-27 First-Party Desktop Platform And SDL3 Removal v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the optional SDL3 desktop runtime/editor/audio stack with first-party Windows desktop backends, then remove SDL3 from build, package, generated-game, documentation, and agent contract surfaces.

**Architecture:** Keep `mirakana::` public APIs first-party, value-based, and backend-neutral. Add native Windows adapter modules behind existing `MK_platform`, `MK_runtime_host`, `MK_audio`, `MK_ui`, `MK_renderer`, and `MK_rhi` contracts before deleting SDL3, so every removal step has an equivalent validated replacement. Treat Windows as the first release desktop target; macOS/Linux desktop parity remains host-gated future work until native backends are separately designed and validated.

**Tech Stack:** C++23, PowerShell 7 repository tools, CMake/CTest/CPack, Windows SDK Win32, Raw Input, XInput/GameInput-class controller gates, WASAPI, DXGI/D3D12 presentation, Dear ImGui Win32/D3D12 editor backend if the visible editor is retained, first-party manifest fragments, JSON schemas, static contract checks, and existing `MK_*` engine modules. SDL3 is retained only as legacy replacement context until final deletion and must not be a dependency of the closed plan.

---

## Plan ID

`first-party-desktop-platform-sdl3-removal-v1`

## Status

**Status:** Active.

Selected long-running milestone by operator direction on 2026-05-27. This plan replaces `reviewed-importers-codecs-shader-generation-v1` as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` for the SDL3 removal track; the importer/codecs/shader-generation plan is paused and remains reviewable future work. Phases 0-6 are merged; Phase 7 is the current candidate slice.

## Context

The current repository already keeps SDL3 behind adapter boundaries:

- `engine/platform/sdl3`
- `engine/runtime_host/sdl3`
- `engine/audio/sdl3`
- optional `desktop-runtime` and `desktop-gui` vcpkg features
- desktop runtime/package scripts and generated-game templates

That makes removal feasible before release, but not cheap. SDL3 is referenced by CMake targets, tests, install/package checks, generated-game templates, current capabilities, roadmap prose, validation recipe rows, game manifests, static checks, and third-party notices. The correct long-term shape is not "delete SDL3 first"; it is "prove first-party native backends, move proof lanes, then delete SDL3."

The target architecture is:

```text
MK_platform
  -> MK_platform_win32

MK_runtime_host
  -> MK_runtime_host_win32
  -> MK_runtime_host_win32_presentation

MK_audio
  -> MK_audio_wasapi

MK_editor_core
  -> unchanged GUI-independent core

MK_editor
  -> Win32 + Dear ImGui + D3D12 shell, or deferred visible editor lane
```

## Official Source Refresh Gate

Before each code phase, re-check current official documentation for the exact API being touched and record the checked source in the phase evidence. Representative anchors for the initial plan:

- Win32 window creation: <https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window>
- Win32 messages and message queues: <https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues>
- Raw Input overview: <https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input>
- DXGI overview and resize behavior: <https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi>
- DXGI variable refresh / tearing flags: <https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays>
- WASAPI render client: <https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-iaudiorenderclient>
- Windows audio stream management: <https://learn.microsoft.com/en-us/windows/win32/coreaudio/stream-management>
- Common Item Dialog: <https://learn.microsoft.com/windows/win32/shell/common-file-dialog>
- Text Services Framework: <https://learn.microsoft.com/windows/desktop/TSF/text-services-framework>
- XInput programming guide: <https://learn.microsoft.com/windows/desktop/xinput/programming-guide>
- GameInput hardware interfaces, if selected: <https://learn.microsoft.com/en-us/gaming/gdk/docs/features/common/input/hardware/input-hardware-interfaces>

If an API requires SDK availability, redistribution terms, or host configuration not present on the current machine, record the missing host evidence rather than weakening validation.

## Constraints

- No public gameplay, runtime UI, renderer, audio, editor-core, or generated-game API may expose `HWND`, `HINSTANCE`, `HANDLE`, `IDXGISwapChain*`, `IAudioClient*`, `XINPUT_STATE`, `SDL_*`, Dear ImGui, or backend-native handles.
- No compatibility shims for SDL3 names. This is a clean breaking greenfield replacement.
- Do not delete SDL3 until a native replacement lane proves source-tree runtime, installed package validation, generated-game templates, and editor strategy.
- Do not claim macOS/Linux native desktop parity from Windows native work.
- Controller support starts as scoped Windows evidence. Broad controller compatibility is not ready until XInput/GameInput/raw HID policy, device hotplug, mapping, and package-visible evidence land.
- IME/text input starts with explicit first-party text-session rows and minimal committed text evidence. Full TSF, candidate UI, grapheme editing, and accessibility publication remain separate gates.
- WASAPI starts with shared-mode default playback. Exclusive mode, capture, device selection UI, spatial audio, HRTF, and broad latency claims remain separate gates.
- Optional dependencies removed from `vcpkg.json` require dependency docs, legal records, notices, package checks, and static policy updates in the same slice.
- Existing active production plans that mention SDL3 must be reconciled before this plan becomes active implementation work.

## Long-Running Operation Model

- Use one branch/PR per phase or tightly related phase pair. Do not batch backend addition and SDL3 deletion into one PR.
- Keep each phase reviewable: tests first, implementation second, docs/manifest/static checks last.
- After each phase, update the plan evidence with exact commands and outcome.
- Keep the visible editor lane independent from the package runtime lane. Runtime/package proof must not wait on full editor productization.
- If a phase reveals that a public contract must change, update all callers, tests, generated templates, game manifests, docs, and agent surfaces in that same phase.

## Phase 0 - Selection, Drift Audit, And Baseline

**Goal:** Make this plan executable without silently conflicting with the current active milestone.

**Files:**

- Modify: `docs/superpowers/plans/README.md`
- Modify if selected as active: `docs/superpowers/plans/2026-05-27-reviewed-importers-codecs-shader-generation-v1.md`
- Modify if selected as active: `docs/superpowers/master-plans/2026-05-27-clean-break-broad-production-readiness-master-plan-v1.md`
- Modify if selected as active: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify if selected as active: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate if selected as active: `engine/agent/manifest.json`
- Audit: `CMakeLists.txt`
- Audit: `vcpkg.json`
- Audit: `tools/check-dependency-policy.ps1`
- Audit: `tools/check-json-contracts*.ps1`
- Audit: `tools/check-ai-integration*.ps1`
- Audit: `tools/new-game-templates.ps1`

- [x] Record the decision that Windows desktop native backends are the first replacement target and macOS/Linux desktop parity is future host-gated work.
- [x] Run a scoped SDL3 surface inventory:

```powershell
rg -n "SDL3|sdl3|MK_platform_sdl3|MK_audio_sdl3|MK_runtime_host_sdl3|MK_ENABLE_DESKTOP_RUNTIME|MK_ENABLE_DESKTOP_GUI" CMakeLists.txt vcpkg.json docs engine editor games tests tools
```

Expected: a list of all remaining SDL3 build, test, doc, manifest, package, template, and policy references.

- [x] Classify every match as `replace-before-delete`, `delete-after-replacement`, `historical-only`, or `static-guard-update`.
- [x] If this plan becomes active, update the manifest fragments so `currentActivePlan` points here and `recommendedNextPlan` describes this Windows-first native desktop milestone.
- [x] If this plan stays proposed, leave manifest pointers unchanged and keep the registry entry as proposed future work.
- [x] Run focused docs/agent checks after any registry or manifest update:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

**Done When:** The repository has an explicit selected/proposed status for this plan, a complete SDL3 surface inventory, and no active plan text silently requiring new SDL3 evidence for future ready claims.

**Phase Evidence:** Phase 0 selection and inventory started in isolated worktree `codex/first-party-desktop-sdl3-removal-v1`.

- Decision: Windows desktop native backends are the first replacement target. macOS/Linux desktop parity, Metal surface presentation, platform-specific IME/accessibility parity, and broad controller parity stay host-gated future work until separately designed and validated.
- Scoped inventory command run:

```powershell
rg -n "SDL3|sdl3|MK_platform_sdl3|MK_audio_sdl3|MK_runtime_host_sdl3|MK_ENABLE_DESKTOP_RUNTIME|MK_ENABLE_DESKTOP_GUI" CMakeLists.txt vcpkg.json docs engine editor games tests tools .github
```

- Inventory summary: 96 files still mention SDL3-related targets, flags, docs, manifests, tests, scripts, generated-game templates, or policy checks. Largest surfaces by match count are `tests/unit/sdl3_platform_tests.cpp`, `engine/agent/manifest.json`, this plan, root `CMakeLists.txt`, `tools/check-ai-integration-020-engine-manifest.ps1`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, `engine/agent/manifest.fragments/004-modules.json`, `docs/roadmap.md`, `tools/new-game-templates.ps1`, and the current SDL3 engine adapter modules.
- `replace-before-delete`: `engine/platform/sdl3`, `engine/runtime_host/sdl3`, `engine/audio/sdl3`, SDL3 public compile/unit tests, desktop runtime sample `main.cpp` files, `editor/src/main.cpp`, `editor/src/sdl_viewport_texture.cpp`, runtime UI SDL3 text/clipboard/IME bridge call sites, desktop package smoke scripts, and generated-game template code that currently includes `mirakana/runtime_host/sdl3/*`.
- `delete-after-replacement`: `vcpkg.json` `sdl3` and Dear ImGui SDL binding feature dependencies, root CMake SDL3 options/export/install/package branches, SDL3 test targets, `SDL3.dll` install validation, `engine/*/sdl3` source trees, `tests/unit/*sdl3*`, SDL3 package target metadata, and `THIRD_PARTY_NOTICES.md` SDL3 rows after no binary output ships SDL3.
- `static-guard-update`: `tools/check-dependency-policy.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts-020-game-contracts.ps1`, `tools/check-json-contracts-050-generated-games.ps1`, `tools/check-ai-integration-010-agent-baseline.ps1`, `tools/check-ai-integration-020-engine-manifest.ps1`, `tools/check-ai-integration-070-production-ledger.ps1`, `tools/check-ai-integration-080-scaffold-smokes.ps1`, `tools/check-ai-integration-099-runtime-ui-production-stack.ps1`, and `tools/check-ai-integration-103-audio-production-proof.ps1`.
- `historical-only`: completed plan/archive prose and current docs that only preserve evidence of already-closed SDL3-backed slices. These must move to explicit historical wording or archive-only references before final deletion, not silently disappear from closeout evidence.
- First PR boundary: Phase 0 active stack switch plus inventory only. Win32/WASAPI implementation, package migration, generated-game migration, editor migration, and SDL3 deletion remain later candidate PRs.
- Active stack switch updated `docs/superpowers/plans/README.md`, the production-completion master plan, clean-break master plan, current capability/roadmap pointers, the production-completion corpus selection rows, and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`; `engine/agent/manifest.json` was regenerated with `tools/compose-agent-manifest.ps1 -Write`.
- Focused validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass; 96/96 CTest tests passed, including `MK_wasapi_audio_tests`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass.
  - `git diff --check`: pass.

## Phase 1 - Native Desktop Contract Guards

**Goal:** Add static and unit-test guards that describe the replacement contract before new native code lands.

**Files:**

- Modify: `tools/check-public-api-boundaries.ps1`
- Modify: `tools/check-dependency-policy.ps1`
- Create or modify: `tools/check-native-desktop-contracts.ps1`
- Modify: `tools/validate.ps1`
- Create: `tests/unit/native_desktop_contract_public_api_compile.cpp`
- Modify: `CMakeLists.txt`

- [x] Add static checks that reject Windows native types from public gameplay-facing headers outside backend-private implementation paths.
- [x] Add static checks that reject `SDL3/` includes in all public headers and, after Phase 9, reject `SDL3/` includes repository-wide except historical docs.
- [x] Add a compile-only public header contract target for backend-neutral window, input, clipboard, file, process, audio, runtime-host, UI, renderer, and editor-core rows; keep existing focused unit tests as the behavioral contract source.
- [x] Wire the new contract check into `tools/validate.ps1` only after it is stable and scoped enough to avoid false positives.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_native_desktop_contract_public_api_compile
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "native_desktop_contract"
```

**Done When:** Public API leakage guards exist before Win32/WASAPI implementation starts, and failures point to exact files to fix.

**Phase Evidence:** Phase 1 implemented in isolated worktree `codex/native-desktop-contract-guards-v1`, with Phase 0 PR #271 merged before Phase 1 publication.

- Official source refresh:
  - Microsoft Learn Win32 desktop application guidance confirms `HWND`/`HINSTANCE` and Win32 message-loop types are native desktop implementation details.
  - Microsoft Learn WASAPI `IAudioClient` / `IAudioRenderClient` docs confirm `audioclient.h` COM interfaces are native endpoint stream details.
  - Microsoft Learn XInput docs confirm `XINPUT_STATE` / `XINPUT_GAMEPAD` are native controller API rows.
  - Context7 `/kitware/cmake` refresh confirmed the target-based `add_executable` plus `add_test(NAME ... COMMAND ...)` CTest pattern used by the repository.
- Added `tools/check-native-desktop-contracts.ps1`, scanning engine, editor-core, and visible-editor public include roots and failing on public exposure of native Win32, D3D12/DXGI, WASAPI/Core Audio, XInput/GameInput, COM, or SDL3 C API/header symbols.
- Extended `tools/check-public-api-boundaries.ps1` so the existing public API boundary guard also rejects WASAPI/Core Audio and Windows controller native symbols.
- Added dependency-policy assertions that keep the native desktop guard present, validate-integrated, and explicitly covering `IAudioClient`, `XINPUT_STATE`, `IUnknown`, `SDL3/`, and `editor/include`.
- Added `nativeDesktopContractCheck` to the composed engine agent manifest command surface.
- Added `MK_native_desktop_contract_public_api_compile` for the Phase-specific guarantee that engine and editor native desktop public headers compile from a consumer TU through first-party opaque/value contracts without linking SDL3 or exposing native desktop SDK types.
- Removed the visible editor shell texture bridge from `editor/include` and kept SDL3, Dear ImGui, and D3D12 display-texture details in `editor/src` private headers.
- Wired `tools/check-native-desktop-contracts.ps1` into `tools/validate.ps1` static tasks.
- Focused validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_native_desktop_contract_public_api_compile`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "native_desktop_contract"`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass; 95/95 CTest tests passed, including `MK_win32_platform_tests`, `MK_runtime_ui_workbench_tests`, and `MK_runtime_ui_production_stack_tests`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass; 95/95 CTest tests passed, including `MK_win32_platform_tests`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass; 94/94 CTest tests passed, including `MK_native_desktop_contract_public_api_compile`.

## Phase 2 - `MK_platform_win32` Window, Lifecycle, Display, And Cursor

**Goal:** Provide a first-party Windows window and event pump adapter behind `MK_platform` without SDL3.

**Files:**

- Create: `engine/platform/win32/CMakeLists.txt`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_runtime.hpp`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_window.hpp`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_event_pump.hpp`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_cursor.hpp`
- Create: `engine/platform/win32/src/win32_runtime.cpp`
- Create: `engine/platform/win32/src/win32_window.cpp`
- Create: `engine/platform/win32/src/win32_event_pump.cpp`
- Create: `engine/platform/win32/src/win32_cursor.cpp`
- Modify: `CMakeLists.txt`
- Create: `tests/unit/win32_platform_tests.cpp`

- [x] Re-check Win32 window, message queue, DPI, cursor, and monitor/display official docs.
- [x] Add RED tests for descriptor validation, lifecycle startup/shutdown rows, resize/focus/minimize/close event translation, and cursor mode planning.
- [x] Add `mirakana_platform_win32` and `MK_platform_win32` targets behind a Windows-only build condition.
- [x] Keep `HWND`, `HINSTANCE`, `HCURSOR`, and raw message data private to `src/` files or backend-private handles that never cross gameplay APIs.
- [x] Implement a deterministic copied event row model similar in spirit to the existing SDL3 copied event rows, but named `Win32WindowEvent` or another first-party backend-specific name.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "win32_platform"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Done When:** A native Win32 window can be created, pumped, resized, closed, and translated to first-party platform events in focused tests without linking SDL3.

**Phase Evidence:** Phase 2 implemented in isolated worktree `codex/win32-platform-shell-v1`.

- Official practice re-check: Context7 selected Microsoft Learn Win32 API documentation. The implementation follows the documented `RegisterClassExW`/`CreateWindowExW` window-class pattern, `PeekMessageW` message-loop polling, Win32 cursor resource loading, DPI awareness context setup, and monitor/client display-state queries while keeping native Win32 types in `.cpp` files.
- RED evidence: after adding `tests/unit/win32_platform_tests.cpp` and `MK_win32_platform_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests` failed because `Win32Runtime`, `Win32Window`, `Win32EventPump`, `native_window_token`, `display_state`, and `post_close_request` were not implemented.
- Implementation: added Windows-only `engine/platform/win32` with `Win32Runtime`, `Win32Window`, `Win32EventPump`, `Win32WindowModel`, and cursor-mode planning. `MK_platform_win32` is exported and installed as an optional Windows SDK target while current SDL3 runtime/editor/audio paths remain unchanged.
- Hosted install-lane fix: Windows optional install presets also need `mirakana_platform_win32` built before `cmake --install`, so `tools/validate-network-enet.ps1` and `tools/validate-physics-jolt.ps1` include it in their SDK target lists.
- Agent surface drift: added `MK_platform_win32` to `engine/agent/manifest.fragments/004-modules.json`, `win32-native` platform readiness to `006-runtimeBackendReadiness.json`, and `win32-platform` validation recipe to `009-validationRecipes.json`, then regenerated `engine/agent/manifest.json`.
- Focused validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R win32_platform`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/platform/win32/src/win32_runtime.cpp,engine/platform/win32/src/win32_window.cpp,engine/platform/win32/src/win32_event_pump.cpp,engine/platform/win32/src/win32_cursor.cpp,tests/unit/win32_platform_tests.cpp`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass; 95/95 CTest tests passed, including `MK_win32_platform_tests`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-network-enet.ps1`: locally blocked before configure because the `network-enet` vcpkg feature is not installed and `tools/bootstrap-deps.ps1` is session-policy blocked; hosted Windows MSVC is the proof lane for this optional preset.

## Phase 3 - Win32 Input, Clipboard, File Dialog, And Text Input Foundation

**Goal:** Replace SDL3 input, clipboard, file-dialog, and minimal text-input bridges with first-party Win32 adapters.

**Files:**

- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_input.hpp`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_clipboard.hpp`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_file_dialog.hpp`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_text_input.hpp`
- Create: `engine/platform/win32/src/win32_input.cpp`
- Create: `engine/platform/win32/src/win32_clipboard.cpp`
- Create: `engine/platform/win32/src/win32_file_dialog.cpp`
- Create: `engine/platform/win32/src/win32_text_input.cpp`
- Create: `engine/platform/win32/src/win32_utf.cpp`
- Create: `engine/platform/win32/src/win32_utf.hpp`
- Modify: `tests/unit/win32_platform_tests.cpp`
- Modify: `tests/unit/runtime_ui_*` only where SDL3-specific event tests are replaced by backend-neutral or Win32-specific tests.

- [x] Re-check Raw Input, XInput/GameInput candidate docs, Common Item Dialog, clipboard, IMM/TSF, and Unicode/UTF-16 conversion docs.
- [x] Add RED tests for keyboard/mouse translation into `VirtualInput`, pointer rows, modifier rows, clipboard read/write plans, file dialog request/result rows, and committed text input rows.
- [x] Decide controller policy for the first native release:
  - `keyboard_mouse_ready`
  - `xinput_scoped_ready`
  - `gameinput_host_gated`
  - `broad_controller_compatibility_unsupported`
- [x] Implement keyboard/mouse first; implement controller only to the selected scoped policy.
- [x] Implement clipboard and Common Item Dialog adapters with project-relative path conversion kept outside native handle ownership.
- [x] Implement minimal text input evidence: begin/end session rows plus committed text rows. Full TSF composition/candidate UI remains future unless separately selected.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests MK_runtime_ui_workbench_tests MK_runtime_ui_production_stack_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "win32_platform|runtime_ui"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Done When:** Runtime/game/editor-core code can consume Windows-native copied input, clipboard, file-dialog, and minimal text rows through first-party contracts with no SDL3 dependency.

**Phase Evidence:** Phase 3 implemented in isolated worktree `codex/win32-input-shell-v1`.

- Official practice re-check: Context7 selected Microsoft Learn Win32 API documentation for Raw Input, clipboard, and Common Item Dialog. The implementation follows the documented Raw Input keyboard/mouse scope (`GetRawInputData`, RAWINPUT/RAWKEYBOARD/RAWMOUSE rows), Win32 keyboard/mouse message rows, `CF_UNICODETEXT` clipboard ownership through `OpenClipboard` / `EmptyClipboard` / `SetClipboardData` with movable global memory, Common Item Dialog COM launch through `IFileOpenDialog` / `IFileSaveDialog`, and UTF-16 committed text conversion. Microsoft Learn TSF/IMM references confirm full TSF composition/candidate UI is a COM/IME integration surface and remains deferred beyond this minimal committed-text phase.
- RED evidence: after extending `tests/unit/win32_platform_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests` failed on missing `mirakana/platform/win32/win32_clipboard.hpp`, proving the new Phase 3 headers/contract rows did not already exist.
- Controller policy: Phase 3 selects `keyboard_mouse_ready`; XInput/GameInput and broad controller compatibility remain later scoped/host-gated work.
- Implementation: added Win32 copied input translation and application into `VirtualInput`/`VirtualPointerInput`, keyboard/mouse-scoped Raw Input registration plans, `Win32Clipboard` and `Win32ClipboardTextAdapter` behind `CF_UNICODETEXT`, `Win32FileDialogService` backed by Common Item Dialog rows, minimal `Win32PlatformIntegrationAdapter` begin/end session rows, UTF-16 committed text conversion, text-edit command mapping, and Ctrl/GUI + C/X/V clipboard command mapping. Public headers expose only first-party values and opaque integer tokens; Win32 SDK, COM, and SDL3 types stay in `.cpp` files.
- Agent surface drift: updated `engine/agent/manifest.fragments/004-modules.json`, `006-runtimeBackendReadiness.json`, and `009-validationRecipes.json`, then regenerated `engine/agent/manifest.json`.
- Focused validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests MK_runtime_ui_workbench_tests MK_runtime_ui_production_stack_tests`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "win32_platform|runtime_ui"`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_native_desktop_contract_public_api_compile`: pass after the first long compile was allowed to finish and a no-op rerun returned success.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/platform/win32/src/win32_clipboard.cpp,engine/platform/win32/src/win32_file_dialog.cpp,engine/platform/win32/src/win32_input.cpp,engine/platform/win32/src/win32_text_input.cpp,engine/platform/win32/src/win32_utf.cpp,tests/unit/win32_platform_tests.cpp`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.

## Phase 4 - `MK_audio_wasapi` Shared-Mode Playback

**Goal:** Replace SDL3 audio playback evidence with a Windows WASAPI adapter behind `MK_audio`.

**Files:**

- Create: `engine/audio/wasapi/CMakeLists.txt`
- Create: `engine/audio/wasapi/include/mirakana/audio/wasapi/wasapi_audio_device.hpp`
- Create: `engine/audio/wasapi/src/wasapi_audio_device.cpp`
- Modify: `CMakeLists.txt`
- Create: `tests/unit/wasapi_audio_tests.cpp`
- Modify: `tools/check-dependency-policy.ps1`
- Modify: `docs/dependencies.md`
- No change: `THIRD_PARTY_NOTICES.md` because WASAPI is an official Windows SDK surface and no third-party redistributable was added.

- [x] Re-check WASAPI `IAudioClient`, `IAudioRenderClient`, stream management, COM initialization, shared-mode render stream, device invalidation, and thread-affinity docs.
- [x] Add RED tests for audio format negotiation rows, shared-mode stream plan rows, queued-frame/underrun diagnostics, silence submission, pause/resume/clear, and adapter lifecycle.
- [x] Add `mirakana_audio_wasapi` and `MK_audio_wasapi` targets for Windows only.
- [x] Keep COM interfaces, endpoint IDs, and device pointers private to implementation files.
- [x] Implement default playback shared-mode stream first. Do not claim exclusive mode, capture, device selection UI, spatial audio, or broad latency readiness.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_audio_tests MK_wasapi_audio_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "audio|wasapi"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Done When:** Selected audio package/runtime paths can use WASAPI for default playback evidence without SDL3 audio.

**Phase Evidence:** Phase 4 implemented in isolated worktree `codex/wasapi-audio-foundation-v1`.

- Official practice re-check: Context7 selected Microsoft Learn Win32 API documentation for WASAPI shared-mode render streams. The implementation follows the documented COM-initialized `IMMDeviceEnumerator` default `eRender` / `eConsole` endpoint flow, `IAudioClient::GetMixFormat`, one-time shared-mode `IAudioClient::Initialize`, `IAudioClient::GetBufferSize`, `IAudioClient::GetCurrentPadding`, `IAudioRenderClient::GetBuffer` / `ReleaseBuffer`, `AUDCLNT_BUFFERFLAGS_SILENT`, and `Start` / `Stop` / `Reset` lifecycle controls. COM, endpoint, `WAVEFORMATEX`, and render-client details stay private to implementation files.
- RED evidence: after adding `tests/unit/wasapi_audio_tests.cpp` and `MK_wasapi_audio_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_wasapi_audio_tests` failed on missing `mirakana/audio/wasapi/wasapi_audio_device.hpp`, proving the new Phase 4 contract did not already exist.
- Implementation: added Windows-only `engine/audio/wasapi` with `WasapiAudioRuntime`, `WasapiAudioDevice`, `plan_wasapi_audio_runtime`, `plan_wasapi_shared_mode_stream`, `plan_wasapi_render_queue`, and `wasapi_audio_device_lifecycle_evidence`. The adapter can open the default shared-mode render endpoint, report queued frames, queue silence, queue interleaved float PCM into float32 or pcm16 endpoint formats, pause/resume, and clear via WASAPI while public headers expose only first-party values.
- Boundary and dependency policy: added `mirakana_audio_wasapi` / `MK_audio_wasapi`, installed SDK headers, optional install-lane target closure for `validate-network-enet.ps1` and `validate-physics-jolt.ps1`, public API boundary coverage for `engine/audio/wasapi/include`, and dependency-policy evidence for Windows SDK `ole32`. No third-party dependency or notice was added.
- Agent surface drift: updated `engine/agent/manifest.fragments/004-modules.json`, `006-runtimeBackendReadiness.json`, `009-validationRecipes.json`, and `014-gameCodeGuidance.json`, then regenerated `engine/agent/manifest.json`. Updated audio production static needles to cover WASAPI.
- Focused validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_audio_tests MK_wasapi_audio_tests`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "audio|wasapi"`: pass after building existing `MK_sdl3_audio_tests` and `sample_ui_audio_assets` targets matched by that regex.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/audio/wasapi/src/wasapi_audio_device.cpp,tests/unit/wasapi_audio_tests.cpp`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass after `tools/format.ps1`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`: pass.

## Phase 5 - `MK_runtime_host_win32` And D3D12 Presentation

**Goal:** Replace `SdlDesktopGameHost` and SDL3 presentation package proof with a Windows native runtime host.

**Files:**

- Create: `engine/runtime_host/win32/CMakeLists.txt`
- Create: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_game_host.hpp`
- Create: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_event_pump.hpp`
- Create: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp`
- Create: `engine/runtime_host/win32/src/win32_desktop_game_host.cpp`
- Create: `engine/runtime_host/win32/src/win32_desktop_event_pump.cpp`
- Create: `engine/runtime_host/win32/src/win32_desktop_presentation.cpp`
- Modify: `CMakeLists.txt`
- Modify: `engine/runtime_host/CMakeLists.txt`
- Create: `tests/unit/runtime_host_win32_tests.cpp`
- Create: `tests/unit/runtime_host_win32_public_api_compile.cpp`

- [x] Re-check DXGI swap chains, D3D12 `CreateSwapChainForHwnd`, resize buffers, tearing/VRR flags, present modes, and window-resize interaction.
- [x] Add RED tests for host descriptor validation, lifecycle/run loop rows, `NullRenderer` fallback, D3D12 presentation selection, resize handling, package-visible report rows, and no native handle leakage.
- [x] Implement `Win32DesktopGameHost` with the existing `mirakana::GameApp` contract.
- [x] Implement a D3D12-first presentation adapter that consumes existing `MK_rhi_d3d12`, `MK_renderer`, and scene/UI renderer evidence without exposing native handles.
- [x] Keep Vulkan presentation host-gated until a separate Windows-native Vulkan surface path is explicitly selected.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_win32_public_api_compile MK_runtime_host_win32_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_host_win32"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Done When:** A Windows native desktop host can run the engine host loop and package-visible presentation reports without SDL3.

**Phase Evidence:** Phase 5 implemented in isolated worktree `codex/win32-runtime-host-d3d12-v1`.

- Official practice re-check: Context7 selected Microsoft Learn Win32/DXGI/Direct3D documentation. The implementation follows the documented `CreateSwapChainForHwnd` model for HWND swap chains, direct command queue ownership for D3D12, flip-discard swap effect with buffer count greater than one, `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` gating, `ResizeBuffers` resize behavior, `DXGI_MWA_NO_ALT_ENTER`, and the requirement that back buffers are in present state before present.
- RED evidence: after adding `engine/runtime_host/win32/CMakeLists.txt`, `tests/unit/runtime_host_win32_tests.cpp`, `tests/unit/runtime_host_win32_public_api_compile.cpp`, and the Windows-only CMake wiring, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` failed because `win32_desktop_event_pump.cpp`, `win32_desktop_game_host.cpp`, and `win32_desktop_presentation.cpp` did not exist.
- Implementation: added Windows-only `MK_runtime_host_win32` and `MK_runtime_host_win32_presentation` targets with `Win32DesktopEventPump`, `Win32DesktopGameHost`, `Win32DesktopPresentation`, and `plan_win32_d3d12_swapchain`. The host owns `Win32Runtime`, `Win32Window`, the event pump, first-party input/pointer/gamepad/lifecycle state, renderer selection, and `DesktopGameRunner` service wiring. The presentation path supports deterministic `NullRenderer` fallback, D3D12 RHI frame renderer creation from host-supplied shader bytecode, resize/present report rows, and explicit Vulkan host-gated status.
- Boundary policy: new Win32 runtime-host public headers expose only first-party values, `win32::Win32Window*`, renderer stats, RHI stats, and status/report rows. Targeted public API and native desktop checks reject `SurfaceHandle`, `NativeWindowHandle`, `SdlNativeWindowHandle`, `native_window_token`, and native-window helper exposure from `engine/runtime_host/win32/include`.
- Agent surface drift: updated module, runtime-readiness, validation-recipe, game-guidance, current-capability, and production-loop manifest/docs surfaces for the new Win32 runtime-host foundation while keeping package/generated-game/editor SDL3 migration as later phases.
- Focused validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_win32_tests MK_runtime_host_win32_public_api_compile`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_host_win32_tests|MK_runtime_host_win32_public_api_compile"`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime_host/win32/src/win32_desktop_event_pump.cpp,engine/runtime_host/win32/src/win32_desktop_game_host.cpp,engine/runtime_host/win32/src/win32_desktop_presentation.cpp,tests/unit/runtime_host_win32_tests.cpp,tests/unit/runtime_host_win32_public_api_compile.cpp`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass; 98/98 CTest tests passed, including `MK_runtime_host_win32_tests` and `MK_runtime_host_win32_public_api_compile`.

## Phase 6 - Desktop Runtime Scripts, Packages, And Installed Validation

**Goal:** Move source-tree and installed desktop runtime validation from SDL3 host assumptions to Windows native host assumptions.

**Files:**

- Modify: `tools/validate-desktop-game-runtime.ps1`
- Modify: `tools/package-desktop-runtime.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/installed-sdk-validation.ps1`
- Modify: `tools/release-package-artifacts.ps1`
- Modify: `tools/check-json-contracts*.ps1`
- Modify: `tools/check-ci-matrix.ps1`
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json` only if preset names or feature gates change.
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_desktop_runtime_shell/main.cpp`
- Modify: `games/sample_desktop_runtime_shell/game.agent.json`
- Modify: `games/sample_desktop_runtime_shell/README.md`

- [x] Rename or reinterpret the desktop runtime lane so it no longer implies SDL3.
- [x] Replace `SDL3.dll` installed-package assertions with native Windows runtime artifact assertions.
- [x] Update CTest patterns from `sdl3` tests to `win32` / `wasapi` / `runtime_host_win32` tests.
- [x] Ensure release package artifact validation checks that no SDL3 runtime DLL ships.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-desktop-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1
```

**Done When:** Desktop runtime validation and installed package validation prove the Windows native host path and no longer require SDL3.

**Phase Evidence:** Candidate locally validated.

- RED evidence: after adding static checks to `tools/check-json-contracts-030-tooling-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed because `tools/validate-desktop-game-runtime.ps1` still lacked `MK_runtime_host_win32_public_api_compile` and still used SDL3 test targets.
- Implementation: `tools/validate-desktop-game-runtime.ps1` and `tools/package-desktop-runtime.ps1` now select `MK_runtime_host_win32_*`, `MK_win32_platform_tests`, and `MK_wasapi_audio_tests` instead of SDL3 tests. `tools/validate-installed-desktop-runtime.ps1`, `tools/installed-sdk-validation.ps1`, and `tools/release-package-artifacts.ps1` reject `SDL3.dll` in installed or archived runtime artifacts. `sample_desktop_runtime_shell` now uses `Win32DesktopGameHost` / `Win32DesktopPresentation` with D3D12 installed smoke evidence while retaining Vulkan SPIR-V artifact validation only.
- Validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_desktop_runtime_shell MK_runtime_host_win32_tests MK_runtime_host_win32_public_api_compile MK_win32_platform_tests MK_wasapi_audio_tests`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "MK_runtime_host_tests|MK_runtime_host_win32_public_api_compile|MK_runtime_host_win32_tests|MK_win32_platform_tests|MK_wasapi_audio_tests|sample_desktop_runtime_shell(_shader_artifacts|_vulkan_shader_artifacts)?_smoke"`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: pass; 18/18 selected desktop-runtime tests passed.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: pass; installed `sample_desktop_runtime_shell` reported `renderer=d3d12`, `presentation_selected=d3d12`, and `desktop-runtime-package: ok`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-desktop-runtime.ps1`: pass; installed `sample_desktop_runtime_shell` reported `renderer=d3d12` and `installed-desktop-runtime-validation: ok`.
  - `Get-ChildItem -LiteralPath 'out/install/desktop-runtime-release/bin' -Filter 'SDL3.dll' -File`: pass; no `SDL3.dll` was present.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files games/sample_desktop_runtime_shell/main.cpp`: pass.
  - `git diff --check`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass; 98/98 CTest tests passed, including `MK_runtime_host_win32_tests`, `MK_runtime_host_win32_public_api_compile`, `MK_win32_platform_tests`, `MK_wasapi_audio_tests`, and the Win32-backed `sample_desktop_runtime_shell_*_smoke` tests.

## Phase 7 - Generated Game Templates And Sample Migration

**Goal:** Move generated desktop games and committed sample package proofs from SDL3 host modules to Windows native host modules.

**Files:**

- Modify: `tools/new-game-templates.ps1`
- Modify: `tools/new-game.ps1` only if template ids or validation descriptors change.
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_2d_desktop_runtime_package/README.md`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `games/sample_desktop_runtime_game/README.md`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/README.md`
- Modify: `games/sample_generated_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_package/README.md`
- Modify: `games/sample_generated_desktop_runtime_cooked_scene_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_cooked_scene_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_cooked_scene_package/README.md`
- Modify: `games/sample_generated_desktop_runtime_material_shader_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_material_shader_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_material_shader_package/README.md`
- Modify: additional sample `game.agent.json` files that declare `MK_platform_sdl3`, `MK_runtime_host_sdl3`, or `sdl3-desktop`.
- Modify: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_game_host.hpp`
- Modify: `engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp`
- Modify: `engine/runtime_host/win32/src/win32_desktop_game_host.cpp`
- Modify: `engine/runtime_host/win32/src/win32_desktop_presentation.cpp`
- Add: `engine/runtime_host/win32/src/scene_gpu_binding_injecting_renderer.hpp`
- Modify: `tests/unit/runtime_host_win32_public_api_compile.cpp`
- Modify: `tools/check-json-contracts-020-game-contracts.ps1`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-json-contracts-050-generated-games.ps1`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-ai-integration-070-production-ledger.ps1`
- Modify: `tools/check-ai-integration-080-scaffold-smokes.ps1`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-template.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/*.json` and composed `engine/agent/manifest.json`
- Modify: game-development agent skills and references for Codex and Claude

- [x] Add RED static checks for generated template descriptors that still reference SDL3 after migration.
- [x] Replace `mirakana/runtime_host/sdl3/...` includes with Windows native host includes.
- [x] Rename backend readiness strings from `sdl3-desktop` / `sdl3-desktop-host-gated` to explicit Windows native names.
- [x] Keep package-visible counters stable where behavior is equivalent; rename only counters that explicitly contain `sdl3`.
- [x] Regenerate or update sample manifests so modules list `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, and `MK_audio_wasapi` only when their phase evidence exists.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name native_desktop_smoke -Template DesktopRuntimePackage
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "sample_2d_desktop_runtime_package|sample_generated_desktop_runtime_3d_package"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

**Done When:** Generated and committed desktop game lanes target Windows native host modules without SDL3 strings in ready claims.

**Phase Evidence:**

- RED evidence: after adding static checks in `tools/check-json-contracts-030-tooling-contracts.ps1`, `tools/check-json-contracts-050-generated-games.ps1`, and `tools/check-ai-integration-080-scaffold-smokes.ps1`, the repository still contained SDL3-backed generated template and sample package expectations, which failed the new Win32-only checks until migration.
- Implementation: `tools/new-game-templates.ps1`, generated desktop runtime package samples, `sample_desktop_runtime_game`, game manifests, package sample CMake registrations, static checks, manifest fragments, current capability docs, game-template specs, and game-development agent skills now use `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, and `MK_audio_wasapi` for generated and committed desktop runtime game lanes. `--video-driver`, `video_driver_hint`, `sdl3-desktop`, and `mirakana/runtime_host/sdl3` assumptions were removed from these active lanes.
- Win32 parity extension: `MK_runtime_host_win32_presentation` now exposes the package-visible scene GPU binding, postprocess, directional shadow, native UI overlay, texture atlas overlay, quality gate, GPU memory, and debug profiling reports needed by the generated package proofs, with first-party Win32/D3D12/Vulkan status rows.
- Focused validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_win32_public_api_compile MK_runtime_host_win32_tests sample_desktop_runtime_game sample_2d_desktop_runtime_package sample_generated_desktop_runtime_package sample_generated_desktop_runtime_cooked_scene_package sample_generated_desktop_runtime_material_shader_package sample_generated_desktop_runtime_3d_package`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "sample_desktop_runtime_game|sample_2d_desktop_runtime_package|sample_generated_desktop_runtime_package|sample_generated_desktop_runtime_cooked_scene_package|sample_generated_desktop_runtime_material_shader_package|sample_generated_desktop_runtime_3d_package|runtime_host_win32"`: pass; 12/12 tests passed.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: pass; 18/18 tests passed, including the Win32 host, WASAPI, runtime shell, non-shell sample, 2D package, generated package, cooked-scene package, material-shader package, and 3D package smokes.
- Full slice validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: pass.
  - `git diff --check`: pass.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: pass; 98/98 CTest tests passed.

## Phase 8 - Visible Editor Migration Or Deferral

**Goal:** Remove SDL3 from the visible editor path without forcing unrelated editor productization.

**Files:**

- Modify: `editor/src/**`
- Modify: `editor/CMakeLists.txt` or relevant root `CMakeLists.txt` sections.
- Modify: `tools/build-gui.ps1`
- Modify: `tools/evaluate-cpp23.ps1`
- Modify: `tests/unit/editor_*` where visible-shell assumptions are checked.
- Modify: `vcpkg.json`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`

- [ ] Choose one editor strategy:
  - `native_editor_shell_now`: migrate `MK_editor` to Win32 + Dear ImGui + D3D12 backend in this milestone.
  - `editor_shell_deferred`: keep `MK_editor_core` ready, temporarily remove or host-gate the visible editor shell until native shell lands.
- [ ] If migrating now, re-check Dear ImGui Win32/D3D12 backend docs and license/package implications.
- [ ] Add RED build or static checks that fail if `MK_editor` links SDL3.
- [ ] Replace SDL3 window/event/file-dialog/clipboard integration with `MK_platform_win32` adapters.
- [ ] Keep editor core independent from native handles.
- [ ] Run the selected lane:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Gui
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Done When:** The visible editor either builds without SDL3 through native Windows adapters or is explicitly host-gated/deferred without leaving SDL3 as a required dependency.

**Phase Evidence:** Not started.

## Phase 9 - SDL3 Dependency And Source Removal

**Goal:** Delete SDL3 adapters and dependency records after native replacement evidence exists.

**Files:**

- Delete: `engine/platform/sdl3/**`
- Delete: `engine/runtime_host/sdl3/**`
- Delete: `engine/audio/sdl3/**`
- Delete or rewrite: `tests/unit/sdl3_platform_tests.cpp`
- Delete or rewrite: `tests/unit/sdl3_audio_tests.cpp`
- Delete or rewrite: `tests/unit/runtime_host_sdl3_tests.cpp`
- Delete or rewrite: `tests/unit/runtime_host_sdl3_public_api_compile.cpp`
- Modify: `CMakeLists.txt`
- Modify: `vcpkg.json`
- Modify: `tools/check-dependency-policy.ps1`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `tools/check-public-api-boundaries.ps1`
- Modify: `tools/check-json-contracts*.ps1`
- Modify: `tools/check-ai-integration*.ps1`

- [ ] Remove `sdl3` from `vcpkg.json` features.
- [ ] Remove Dear ImGui SDL3 binding features if the visible editor no longer uses them.
- [ ] Remove SDL3 CMake targets, aliases, install rules, package feature flags, and test target lists.
- [ ] Remove dependency-policy requirements that require SDL3.
- [ ] Remove third-party notices for SDL3 only when no shipped artifact or source dependency still uses it.
- [ ] Add a final static check that fails on non-historical SDL3 references.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

**Done When:** `rg -n "SDL3|sdl3" CMakeLists.txt vcpkg.json engine editor games tests tools docs` returns only approved historical references or no references, and no build/package/install lane depends on SDL3.

**Phase Evidence:** Not started.

## Phase 10 - Manifest, Docs, CI, And Plan Closeout

**Goal:** Make native desktop the current truth and close this milestone only with full validation evidence.

**Files:**

- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/building.md`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/ai-integration.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-27-first-party-desktop-platform-sdl3-removal-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generate: `engine/agent/manifest.json`
- Modify: `.agents/skills/*/SKILL.md`, `.claude/skills/*/SKILL.md`, `.cursor/skills/*/SKILL.md`, and subagents only if durable workflow or backend guidance changed.

- [ ] Update current capabilities from SDL3 optional desktop runtime to Windows native desktop runtime.
- [ ] Update roadmap so desktop runtime productization no longer describes SDL3 as current proof.
- [ ] Update architecture docs with native Windows backend boundaries and public native-handle denial.
- [ ] Update generated-game guidance and validation scenarios to use native desktop host modules.
- [ ] Compose the manifest and verify generated output.
- [ ] Run final validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Publish through GitHub Flow after validation. Do not push directly to `main`, force-push, or merge without hosted checks for the latest PR head.

**Done When:** SDL3 is absent from shipped dependencies and active ready claims, Windows native desktop runtime/package/editor strategy is documented and validated, and `currentActivePlan` no longer points at this plan after closeout.

**Phase Evidence:** Not started.

## Risk Ledger

| Risk | Mitigation |
| --- | --- |
| Win32 message/input behavior diverges from SDL3 and breaks sample controls. | Keep copied event rows deterministic, add focused event translation tests, and keep generated-game controls simple until controller evidence lands. |
| WASAPI introduces host-specific failures or audio glitches. | Start with shared-mode default playback, deterministic stream plan rows, silence/underrun diagnostics, and host-gated broader claims. |
| IME/text input grows too large. | Land only begin/end and committed text evidence first; TSF composition/candidate UI is a later phase unless explicitly selected. |
| Visible editor migration blocks runtime/package proof. | Decouple editor shell from runtime package phases. Allow `editor_shell_deferred` if native runtime/package proof is ready first. |
| SDL3 references remain hidden in docs/manifests/templates. | Add final static checks and keep Phase 9 as deletion-only after replacement phases pass. |
| Cross-platform desktop support regresses. | State Windows-only native desktop readiness clearly; keep macOS/Linux desktop parity as future host-gated work. |
| AI agents over-claim native readiness. | Update manifest fragments, generated-game guidance, static checks, and current capabilities with exact ready/host-gated/unsupported rows. |

## Completion Definition

- `vcpkg.json` no longer contains `sdl3`.
- No CMake target, test, generated-game template, package script, installed validation script, current capability claim, or manifest ready row depends on SDL3.
- Windows native desktop runtime validates source-tree and installed package lanes.
- Sample 2D and generated 3D desktop runtime package proofs run through native host modules.
- The visible editor either builds through native Windows adapters or is explicitly deferred/host-gated without retaining SDL3 as a dependency.
- `tools/check-dependency-policy.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `tools/validate-desktop-game-runtime.ps1`, package lanes, and `tools/validate.ps1` pass or record concrete host/tool blockers.
- Legal/dependency records remove SDL3 only after no source or artifact uses it.
- The plan registry, roadmap, current capabilities, generated-game docs, manifest fragments, and composed manifest agree on the final native desktop status.
