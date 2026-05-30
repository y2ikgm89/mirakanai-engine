# 2026-05-30 Native Win32 Editor Shell v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restore a launchable `MK_editor` visible desktop editor shell as a clean Windows-native Win32 + Dear ImGui + Direct3D 12 application after SDL3 removal.

**Architecture:** Keep `MK_editor_core` as the GUI-independent source of editor state, models, project IO, undoable authoring operations, and AI-operable rows. Put all Win32, D3D12, DXGI, COM, and Dear ImGui integration in private `editor/src` implementation targets, with no SDL3 compatibility layer, no public native handles, and no dependency from game/runtime UI to Dear ImGui. The first closeable slice proves a real window can launch, render ImGui frames, display core-backed panels, run a deterministic smoke, and shut down cleanly; later phases reintroduce viewport and material-preview GPU display through new native adapters.

**Tech Stack:** C++23, PowerShell 7 repository wrappers, CMake presets, vcpkg manifest feature `desktop-gui`, Dear ImGui upstream Win32 + DirectX 12 backends, Windows SDK Win32/DXGI/D3D12, existing `MK_platform_win32`, `MK_editor_core`, `MK_runtime_host_win32`, `MK_rhi_d3d12`, `MK_renderer`, static contract checks, and agent manifest fragments.

---

## Plan ID

`native-win32-editor-shell-v1`

## Status

**Status:** Active.

Selected as the long-running milestone for restoring the native editor shell. `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points at this plan while its candidate PRs are implemented, validated, published, and merged.

## Current Baseline

The completed `First-Party Desktop Platform And SDL3 Removal v1` milestone removed the old SDL3/Dear ImGui shell from active build lanes. At plan creation, baseline behavior was intentionally fail-closed:

- `editor/CMakeLists.txt` emits `MK_editor visible shell is deferred after SDL3 removal` when `MK_ENABLE_DESKTOP_GUI=ON`.
- `tools/build-gui.ps1` exits with the same deferral message.
- `tools/evaluate-cpp23.ps1 -Gui` exits with the GUI deferral message.
- `vcpkg.json` keeps `desktop-gui` as an empty feature with no dependencies.
- `MK_editor_core` remains supported and validated by the default lane.

This plan replaces that deferred state with a new native Windows editor shell. It must not resurrect any deleted SDL3 source, target, vcpkg feature, runtime DLL expectation, generated-game hook, or compatibility alias.

Current execution state:

- Phase 1 completed the audited `desktop-gui` Dear ImGui Win32/DX12 dependency gate through PR #316.
- Phase 2 replaces the old CMake deferral with a minimal Windows-only `MK_editor` launch-contract skeleton and `MK_editor_native_shell_tests`.
- `tools/build-gui.ps1` remains fail-closed until Phase 3/4 wires the Win32/Dear ImGui/D3D12 host and GUI smoke lane; its current message points operators at `MK_editor_native_shell_tests`.

## Official Source Refresh Gate

Before implementation of each phase, re-check the current official documentation or source listed below and record the checked source in phase evidence.

- Win32 window creation and message loops: <https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window> and <https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues>
- DXGI swap chains, resize, and tearing policy: <https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi> and <https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays>
- D3D12 device, command queue, command allocator/list, descriptor heap, fence, and resource lifetime guidance: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-programming-guide>
- Dear ImGui upstream Win32 + DirectX 12 setup and backend source expectations: <https://github.com/ocornut/imgui/wiki/Getting-Started> and upstream `examples/example_win32_directx12/main.cpp`
- vcpkg `imgui` port feature names in the checked-in Microsoft vcpkg registry: `external/vcpkg/ports/imgui/vcpkg.json`
- CMake target-based executable and private dependency guidance through Context7 `/kitware/cmake`

If any source changes an API shape, backend initialization sequence, or vcpkg feature name, update the phase implementation and dependency/legal records rather than adapting stale SDL3-era code.

## Non-Negotiable Constraints

- Do not add or reintroduce SDL3 dependencies, source files, CMake targets, generated-game template references, runtime DLL checks, or compatibility shims.
- Do not expose `HWND`, `HINSTANCE`, `HANDLE`, `IDXGISwapChain*`, `ID3D12*`, `D3D12_*`, `DXGI_*`, COM pointers, or Dear ImGui symbols through `editor/core/include`, `editor/include`, engine public headers, gameplay APIs, runtime UI APIs, or generated-game contracts.
- Do not move Dear ImGui, Win32, D3D12, DXGI, file dialogs, process execution, renderer/RHI ownership, or platform-native handles into `MK_editor_core`.
- Do not make Dear ImGui the production game UI foundation. Dear ImGui is an optional developer/editor shell only.
- Do not keep old `Sdl*` type names, `SDL_*` symbols, `sdl_viewport_texture` file names, or renderer-driver hints as transitional aliases.
- Do not weaken `tools/check-native-desktop-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, or dependency policy checks to make the shell build.
- Do not claim macOS/Linux editor shell parity from this Windows-native work.

## Target Architecture

```text
MK_editor_core
  owns editor documents, retained editor models, project IO policies, undo actions, and diagnostics.

MK_editor_shell_win32 (private implementation target)
  owns native window/message bridge, Dear ImGui context/backend lifecycle, D3D12 device/queue/swapchain,
  descriptor allocator, frame synchronization, smoke-frame execution, and panel rendering adapters.

MK_editor
  is a thin executable entrypoint that parses launch options, constructs the shell, runs the frame loop,
  returns a process exit code, and exposes no public SDK surface.
```

The first launchable shell should show a real window, draw ImGui content, drive `MK_editor_core` models, and terminate deterministically under `--smoke-frames`. The initial viewport may be a retained diagnostics panel. Native viewport image display and material-preview GPU display are later phases in this plan.

## Proposed File Structure

- Create: `editor/src/main.cpp`
  - Thin executable entrypoint, CLI parsing, process exit mapping.
- Create: `editor/src/native_editor_launch.hpp`
  - Private launch options, validation rows, smoke settings, and startup result structs without native handles.
- Create: `editor/src/native_editor_launch.cpp`
  - Launch-option parsing and validation.
- Create: `editor/src/native_editor_app.hpp`
  - Private app state that owns `MK_editor_core` documents, panel visibility, transient UI state, and services.
- Create: `editor/src/native_editor_app.cpp`
  - Core-backed panel rendering orchestration, no Win32/D3D12 ownership.
- Create: `editor/src/native_editor_panels.hpp`
  - Private declarations for panel rendering functions that consume `NativeEditorApp`.
- Create: `editor/src/native_editor_panels.cpp`
  - First restored panels backed by editor-core retained models.
- Create: `editor/src/win32_imgui_d3d12_host.hpp`
  - Private host class declarations for Win32 + Dear ImGui + D3D12 lifecycle; no installed/public headers.
- Create: `editor/src/win32_imgui_d3d12_host.cpp`
  - Native Win32/D3D12/ImGui implementation.
- Create: `editor/src/win32_imgui_message_bridge.hpp`
  - Private WndProc bridge declarations and copied message result rows.
- Create: `editor/src/win32_imgui_message_bridge.cpp`
  - `ImGui_ImplWin32_WndProcHandler` forwarding plus first-party copied event rows.
- Create: `editor/src/win32_imgui_descriptor_allocator.hpp`
  - Private D3D12 SRV descriptor allocator for Dear ImGui backend callback requirements.
- Create: `editor/src/win32_imgui_descriptor_allocator.cpp`
  - Descriptor free-list implementation and unit-testable planning helpers.
- Create: `tests/unit/editor_native_shell_tests.cpp`
  - Launch-option, descriptor allocator, smoke-plan, and public-boundary tests.
- Modify: `editor/CMakeLists.txt`
  - Add private `MK_editor_shell_win32`, `MK_editor`, and Windows-only dependency wiring.
- Modify: `CMakeLists.txt`
  - Register `MK_editor_native_shell_tests` and runtime target inclusion when `MK_editor` exists.
- Modify: `vcpkg.json`
  - Add only audited `imgui` dependency features for `desktop-gui`.
- Modify: `tools/build-gui.ps1`
  - Configure, build, and test `desktop-gui` instead of fail-closed deferral.
- Modify: `tools/evaluate-cpp23.ps1`
  - Restore `-Gui` lane over `cpp23-desktop-gui-eval`.
- Modify: dependency, docs, manifest, and static-check files listed in later phases.

## Phase 0 - Selection, Baseline, And Drift Audit

**Goal:** Make this plan safe to activate without conflicting with the current production plan stack or local repository state.

**Files:**
- Modify if selected: `docs/superpowers/plans/README.md`
- Modify if selected: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate if selected: `engine/agent/manifest.json`
- Audit: `editor/CMakeLists.txt`
- Audit: `tools/build-gui.ps1`
- Audit: `tools/evaluate-cpp23.ps1`
- Audit: `vcpkg.json`
- Audit: `tools/check-dependency-policy.ps1`
- Audit: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Audit: `tools/check-ai-integration-040-agent-surfaces.ps1`
- Audit: `tools/check-ai-integration-060-editor-workflows.ps1`

- [ ] **Step 1: Confirm clean baseline**

Run:

```powershell
git status --short --branch
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: clean or task-owned changes only, toolchain check passes, and full validation passes before the editor shell work starts.

- [ ] **Step 2: Confirm current fail-closed GUI baseline**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
```

Expected at plan creation: nonzero exit with `visible editor shell is deferred after SDL3 removal`. After Phase 2, the expected fail-closed message is the launch-contract skeleton message until the native host and GUI smoke lane are wired.

- [ ] **Step 3: Inventory deferred editor needles**

Run:

```powershell
rg -n "visible editor shell is deferred|MK_editor visible shell is deferred|build-gui.ps1|MK_ENABLE_DESKTOP_GUI|desktop-gui" editor tools docs engine/agent CMakeLists.txt CMakePresets.json vcpkg.json
```

Expected: one auditable list of every file that needs to move from fail-closed deferred shell text to native shell readiness text during later phases.

- [ ] **Step 4: Select this plan only when implementation begins**

If implementation is selected, update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` so `currentActivePlan` points to this file and `recommendedNextPlan.id` is `native-win32-editor-shell-v1`. Compose and verify:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: manifest composition and AI integration checks pass.

**Done When:** The baseline is validated, deferred GUI references are inventoried, and this plan is either still Proposed or explicitly selected in manifest fragments.

**Phase Evidence:** Candidate boundaries selected in `codex/native-win32-editor-dependency-gate-v1`; linked worktree prepared with `tools/prepare-worktree.ps1`, vcpkg/CMake/Dear ImGui official documentation rechecked, local static baseline checks passed, and the active milestone pointer is being moved to this plan in `codex/native-win32-editor-shell-skeleton-v1`.

## Phase 1 - Dear ImGui Dependency, Legal, And Policy Gate

**Goal:** Add Dear ImGui back as an audited `desktop-gui` dependency without bringing back SDL3.

**Files:**
- Modify: `vcpkg.json`
- Modify: `tools/check-dependency-policy.ps1`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`

- [ ] **Step 1: Re-check vcpkg imgui feature names**

Run:

```powershell
Get-Content -Path external/vcpkg/ports/imgui/vcpkg.json -Raw
```

Expected: the port still offers `win32-binding` and `dx12-binding`, and SDL3 binding features remain separate.

- [ ] **Step 2: Update `desktop-gui` dependency shape**

Change `vcpkg.json` so `desktop-gui` declares only:

```json
{
  "name": "imgui",
  "features": [
    "win32-binding",
    "dx12-binding"
  ]
}
```

Expected: no `sdl3-binding`, no `sdl3-renderer-binding`, and no `sdl3` dependency in `desktop-gui`.

- [ ] **Step 3: Update dependency policy assertions**

Change `tools/check-dependency-policy.ps1` so it requires `desktop-gui` to include `imgui` with `win32-binding` and `dx12-binding`, still rejects `sdl3`, and requires Dear ImGui notices while the GUI feature declares it.

Required static policy outcomes:

```text
desktop-gui feature must declare dependency: imgui
desktop-gui imgui dependency must include feature: win32-binding
desktop-gui imgui dependency must include feature: dx12-binding
desktop-gui feature must not declare dependency: sdl3
```

- [ ] **Step 4: Update legal and dependency docs**

Record Dear ImGui as an MIT-licensed optional editor-shell dependency in:

```text
docs/dependencies.md
docs/legal-and-licensing.md
THIRD_PARTY_NOTICES.md
```

Required wording boundaries:

```text
Dear ImGui is optional and editor/developer-shell only.
Dear ImGui is not the production runtime game UI foundation.
The selected desktop-gui feature uses Win32 and DirectX 12 backends and must not enable SDL3 bindings.
```

- [ ] **Step 5: Bootstrap and validate dependency policy**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

Expected: vcpkg installs the selected imgui feature set, dependency policy passes, and formatting passes.

**Done When:** `desktop-gui` has an audited Dear ImGui Win32/DX12 dependency and no SDL3 dependency can enter through policy.

**Phase Evidence:** Completed through PR #316 / merge commit `b1a55f6d52d86bcb4cfde8134592b23fff5bf4c5` from candidate `codex/native-win32-editor-dependency-gate-v1`. Local validation passed static policy, JSON contract, format, vcpkg environment, AI integration, and full `tools/validate.ps1`; local `tools/bootstrap-deps.ps1` remained prompt-gated in the no-approval Codex session, while hosted Windows MSVC evidence covered dependency bootstrap and `PR Gate` passed.

## Phase 2 - Private Native Shell Skeleton And Tests

**Goal:** Replace fail-closed `MK_editor` deferral with a private native shell target that compiles, validates launch options, and exposes a deterministic smoke entrypoint.

**Files:**
- Create: `editor/src/main.cpp`
- Create: `editor/src/native_editor_launch.hpp`
- Create: `editor/src/native_editor_launch.cpp`
- Create: `editor/src/native_editor_app.hpp`
- Create: `editor/src/native_editor_app.cpp`
- Create: `tests/unit/editor_native_shell_tests.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`

- [ ] **Step 1: Write RED launch-option tests**

Add `tests/unit/editor_native_shell_tests.cpp` with tests that require these private launch contracts:

```cpp
// Test names to add:
// - editor native shell launch options default to interactive window
// - editor native shell launch options accept bounded smoke frames
// - editor native shell launch options reject zero window extent
// - editor native shell launch options reject negative smoke frames
```

Expected initial build failure: `NativeEditorLaunchOptions`, `NativeEditorLaunchParseResult`, `parse_native_editor_launch`, or `validate_native_editor_launch` does not exist.

- [ ] **Step 2: Build RED target**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-gui
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-gui --target MK_editor_native_shell_tests
```

Expected: build fails on missing native editor launch types.

- [ ] **Step 3: Add private launch contracts**

Create `editor/src/native_editor_launch.hpp` and `editor/src/native_editor_launch.cpp` with:

```cpp
namespace mirakana::editor {

struct NativeEditorLaunchOptions {
    std::uint32_t width{1280};
    std::uint32_t height{720};
    std::int32_t smoke_frames{-1};
    bool no_user_config{false};
};

struct NativeEditorLaunchValidation {
    bool valid{false};
    std::string diagnostic;
};

struct NativeEditorLaunchParseResult {
    NativeEditorLaunchOptions options;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] NativeEditorLaunchParseResult parse_native_editor_launch(int argc, char** argv);
[[nodiscard]] NativeEditorLaunchValidation validate_native_editor_launch(const NativeEditorLaunchParseResult& launch);
[[nodiscard]] constexpr int native_editor_launch_usage_error_exit_code() noexcept;

} // namespace mirakana::editor
```

Validation rules:

```text
unknown options are rejected
missing option values are rejected
non-numeric option values are rejected
width > 0
height > 0
smoke_frames == -1 or smoke_frames > 0
no native handles in the launch contract
```

- [ ] **Step 4: Add CMake skeleton**

Change `editor/CMakeLists.txt` so `MK_ENABLE_DESKTOP_GUI=ON` creates:

```cmake
add_library(MK_editor_shell_common
    src/native_editor_launch.cpp
)

if(NOT MK_ENABLE_DESKTOP_GUI)
    return()
endif()

if(NOT WIN32)
    message(FATAL_ERROR "MK_ENABLE_DESKTOP_GUI is Windows-only for the native Win32 editor shell")
endif()

if(NOT TARGET MK_platform_win32)
    message(FATAL_ERROR "MK_ENABLE_DESKTOP_GUI requires MK_platform_win32")
endif()

add_library(MK_editor_shell_win32
    src/native_editor_app.cpp
)

target_link_libraries(MK_editor_shell_win32
    PRIVATE
        MK_editor_core
        MK_editor_shell_common
        MK_platform_win32
)

add_executable(MK_editor src/main.cpp)
target_link_libraries(MK_editor PRIVATE MK_editor_shell_win32)
```

Keep all native shell includes private to build targets; do not install `editor/src` headers. The launch-contract skeleton intentionally does not consume `imgui::imgui` until Phase 3 wires the Dear ImGui backend lifecycle.

- [ ] **Step 5: Add thin executable entrypoint**

Create `editor/src/main.cpp` so it parses options, validates them, prints a diagnostic plus usage to `stderr` on invalid input, and returns deterministic usage-error exit code `2` before native window creation. At this phase, a valid invocation may return success after validation while the host is not wired.

- [ ] **Step 6: Prove skeleton tests pass**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-gui --target MK_editor MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-gui --output-on-failure -R "editor_native_shell"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: `MK_editor` and `MK_editor_native_shell_tests` build, launch-option tests pass, and public API guards pass.

**Done When:** `MK_editor` exists as a Windows-only target again, but only validates launch options and carries no SDL3 or public native-handle surface.

**Phase Evidence:** Completed locally in candidate `codex/native-win32-editor-shell-skeleton-v1`. RED build failed before the parse-result contract existed; GREEN evidence includes `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests`, `tools/cmake.ps1 --preset desktop-gui`, `tools/cmake.ps1 --build --preset desktop-gui --target MK_editor MK_editor_native_shell_tests`, `tools/ctest.ps1 --preset desktop-gui --output-on-failure -R MK_editor_native_shell_tests`, an `MK_editor.exe --width wide` invalid-launch smoke returning exit code 2, focused static checks, and full `tools/validate.ps1` with 85/85 tests passing. The implementation keeps the launch-contract skeleton dependency-free until the Phase 3 Dear ImGui backend lifecycle is introduced, rejects malformed CLI input before any future interactive window path, and makes non-Windows `MK_ENABLE_DESKTOP_GUI` failure explicit, so `desktop-gui` can build `MK_editor` locally even when dependency bootstrap is prompt-gated.

## Phase 3 - Win32 + Dear ImGui + D3D12 Host

**Goal:** Implement the official Dear ImGui Win32 + DirectX 12 backend lifecycle in a private editor host that can render frames and shut down cleanly.

**Files:**
- Create: `editor/src/win32_imgui_d3d12_host.hpp`
- Create: `editor/src/win32_imgui_d3d12_host.cpp`
- Create: `editor/src/win32_imgui_message_bridge.hpp`
- Create: `editor/src/win32_imgui_message_bridge.cpp`
- Create: `editor/src/win32_imgui_descriptor_allocator.hpp`
- Create: `editor/src/win32_imgui_descriptor_allocator.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_native_shell_tests.cpp`

- [x] **Step 1: Re-check upstream ImGui DX12 initialization shape**

Confirm the current `ImGui_ImplDX12_InitInfo` fields and descriptor allocation callback requirements from upstream docs/source before coding this phase.

Record in phase evidence:

```text
Checked upstream Dear ImGui Win32 + DirectX 12 backend on <date>.
ImGui_ImplDX12_InitInfo requires Device, CommandQueue, NumFramesInFlight, RTVFormat, SrvDescriptorHeap, SrvDescriptorAllocFn, and SrvDescriptorFreeFn.
```

- [x] **Step 2: Write RED descriptor allocator tests**

Extend `tests/unit/editor_native_shell_tests.cpp` with:

```cpp
// Test names to add:
// - editor imgui descriptor allocator rejects zero capacity
// - editor imgui descriptor allocator leases and reuses descriptor slots
// - editor imgui descriptor allocator reports exhaustion without corrupting free list
```

Expected initial build failure: `EditorImguiDescriptorAllocatorPlan` or equivalent private allocator types do not exist.

- [x] **Step 3: Implement descriptor allocator**

Create a private allocator that stores CPU/GPU base handles, descriptor size, capacity, and free indices. It returns stable slot rows for ImGui backend callbacks and records exhaustion as a diagnostic. Keep native D3D12 handle fields only in `editor/src` private headers.

- [x] **Step 4: Implement message bridge**

Create a private WndProc bridge that:

```text
calls ImGui_ImplWin32_WndProcHandler first,
copies relevant Win32 resize/focus/close messages into first-party rows,
chains to the previous WndProc,
does not expose HWND or WNDPROC outside editor/src.
```

The bridge may use `win32::Win32Window::native_window_token()` inside `editor/src` and cast it to `HWND` privately.

- [x] **Step 5: Implement D3D12 host lifecycle**

Create `Win32ImguiD3d12Host` with private methods:

```text
create_window
create_d3d12_device_and_queue
create_swapchain
create_render_targets
create_imgui_context
begin_frame
render_frame
present
resize
shutdown
```

Follow official order:

```text
IMGUI_CHECKVERSION
ImGui::CreateContext
ImGui_ImplWin32_Init
ImGui_ImplDX12_Init with ImGui_ImplDX12_InitInfo
ImGui_ImplDX12_NewFrame
ImGui_ImplWin32_NewFrame
ImGui::NewFrame
ImGui::Render
ImGui_ImplDX12_RenderDrawData
Present
ImGui_ImplDX12_Shutdown
ImGui_ImplWin32_Shutdown
ImGui::DestroyContext
```

- [x] **Step 6: Use WARP fallback only as a recorded host diagnostic**

If hardware adapter creation fails, the editor host may use WARP for smoke validation. Record selected adapter kind in a private smoke result row. Do not claim hardware graphics readiness from WARP.

- [x] **Step 7: Run focused native host tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-gui --target MK_editor MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-gui --output-on-failure -R "editor_native_shell"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/src/win32_imgui_d3d12_host.cpp,editor/src/win32_imgui_message_bridge.cpp,editor/src/win32_imgui_descriptor_allocator.cpp,tests/unit/editor_native_shell_tests.cpp
```

Expected: private tests pass and clang-tidy accepts the new editor host sources.

**Done When:** The private host can initialize ImGui + D3D12 resources, render an empty frame, present, resize, and shut down without leaking native handles into public headers.

**Phase Evidence:** Completed through PR #318 / merge commit `4611f7165f4423a242e299d17f9f022c0f241a0b`. Checked current Dear ImGui Win32 + DirectX 12 backend documentation/source on 2026-05-30; `ImGui_ImplDX12_InitInfo` is used with `Device`, `CommandQueue`, `NumFramesInFlight`, `RTVFormat`, `SrvDescriptorHeap`, `SrvDescriptorAllocFn`, and `SrvDescriptorFreeFn`. Added private `Win32ImguiDescriptorAllocator`, `Win32ImguiMessageBridge`, and `Win32ImguiD3d12Host` under `editor/src` without public Win32/D3D12/ImGui handle exposure. Local focused checks passed before publication, local `desktop-gui` configure remained dependency-blocked because the linked worktree `vcpkg_installed` tree did not contain `imgui`, and hosted Windows MSVC plus `PR Gate` provided the GUI compile/smoke proof for PR #318.

## Phase 4 - Smoke-Launchable `MK_editor`

**Goal:** Make `MK_editor --smoke-frames 2 --no-user-config` open the native shell, render deterministic frames, and exit successfully.

**Files:**
- Modify: `editor/src/main.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/win32_imgui_d3d12_host.hpp`
- Modify: `editor/src/win32_imgui_d3d12_host.cpp`
- Modify: `CMakeLists.txt`
- Modify: `tools/build-gui.ps1`
- Modify: `tools/evaluate-cpp23.ps1`

- [x] **Step 1: Add an editor smoke CTest**

Register a Windows-only CTest named `MK_editor_smoke` that runs:

```powershell
$<TARGET_FILE:MK_editor> --smoke-frames 2 --no-user-config
```

Expected initial result before full host wiring: test fails because `MK_editor` does not complete the smoke frame loop.

- [x] **Step 2: Implement smoke frame loop**

The smoke loop must:

```text
create the host,
create NativeEditorApp,
render exactly N frames when smoke_frames > 0,
return exit code 0 when N frames complete,
return nonzero with a bounded diagnostic when native initialization fails.
```

- [x] **Step 3: Replace fail-closed build script**

Change `tools/build-gui.ps1` to:

```powershell
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "common.ps1")
$null = Assert-VcpkgExecutable -Purpose "the native desktop GUI build"
Set-MirakanaiVcpkgEnvironment | Out-Null
$tools = Assert-CppBuildTools
Invoke-CheckedCommand $tools.CMake --preset desktop-gui
Invoke-CheckedCommand $tools.CMake --build --preset desktop-gui
Invoke-CheckedCommand $tools.CTest --preset desktop-gui --output-on-failure
```

- [x] **Step 4: Restore C++23 GUI evaluation**

Change `tools/evaluate-cpp23.ps1 -Gui` so it configures, builds, and tests `cpp23-desktop-gui-eval` instead of emitting the deferral message.

- [x] **Step 5: Run GUI smoke lane**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Gui
```

Expected: both commands pass and include `MK_editor_smoke`.

**Done When:** A real `MK_editor` process can launch the native shell, render deterministic smoke frames, and exit successfully through supported wrappers.

**Phase Evidence:** Completed through PR #318 / merge commit `4611f7165f4423a242e299d17f9f022c0f241a0b`. `MK_editor_smoke` runs `MK_editor --smoke-frames 2 --smoke-resize --no-user-config` and requires `editor_shell_status=ready`; smoke output also reports `editor_shell_sdl3=0`, frame count, resize count, and adapter kind. `tools/build-gui.ps1` configures/builds/tests `desktop-gui`, `tools/evaluate-cpp23.ps1 -Gui` targets `cpp23-desktop-gui-eval`, and the Windows MSVC workflow runs `tools/build-gui.ps1` after dependency bootstrap. Local GUI execution was dependency-blocked by missing linked-worktree `imgui`; hosted Windows MSVC plus `PR Gate` provided the GUI smoke proof.

## Phase 5 - Core-Backed Editor Panels

**Goal:** Restore useful editor UI panels over `MK_editor_core` without moving persistent behavior into the ImGui layer.

**Files:**
- Create: `editor/src/native_editor_panels.hpp`
- Create: `editor/src/native_editor_panels.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `tests/unit/editor_core_tests.cpp` only when core model behavior needs new coverage.
- Modify: `tests/unit/editor_native_shell_tests.cpp`

- [x] **Step 1: Define first panel set**

Implement these panels first:

```text
Main Menu
Scene
Inspector
Assets
Console
Resources
AI Commands
Profiler
Timeline
Project Settings
```

The panels consume existing editor-core models and transient shell state. They must not execute package scripts, validation recipes, PIX helpers, or process commands except through already-reviewed editor-core/process-runner gates.

- [x] **Step 2: Keep panel rendering private**

Declare rendering functions in `editor/src/native_editor_panels.hpp`:

```cpp
namespace mirakana::editor {

class NativeEditorApp;

void render_native_editor_main_menu(NativeEditorApp& app);
void render_native_editor_scene_panel(NativeEditorApp& app);
void render_native_editor_inspector_panel(NativeEditorApp& app);
void render_native_editor_assets_panel(NativeEditorApp& app);
void render_native_editor_console_panel(NativeEditorApp& app);
void render_native_editor_resources_panel(NativeEditorApp& app);
void render_native_editor_ai_commands_panel(NativeEditorApp& app);
void render_native_editor_profiler_panel(NativeEditorApp& app);
void render_native_editor_timeline_panel(NativeEditorApp& app);
void render_native_editor_project_settings_panel(NativeEditorApp& app);

} // namespace mirakana::editor
```

These declarations stay private under `editor/src`.

- [x] **Step 3: Add panel smoke assertions**

Extend `MK_editor_smoke` output so it prints bounded lines:

```text
editor_shell_status=ready
editor_shell_backend=d3d12
editor_shell_frames=2
editor_shell_panels=10
editor_shell_sdl3=0
```

Add static or CTest output checks where existing tooling supports them.

- [x] **Step 4: Run focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-gui --target MK_editor MK_editor_core_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-gui --output-on-failure -R "MK_editor_smoke|editor_native_shell|MK_editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1
```

Expected: panel smoke and existing editor-core tests pass, public API checks pass.

**Done When:** The native editor shell displays core-backed panels and reports deterministic smoke counters without adding native behavior to `MK_editor_core`.

**Phase Evidence:** Completed through PR #319 / merge commit `61674e0f05379146894425f7c46d3306afd38f55`. RED tests first failed because `NativeEditorApp` lacked core-backed panel contracts, deterministic panel counters, ImGui user-config policy, and native host resource availability updates. The implementation adds private `native_editor_panels.*`, renders Main Menu, Scene, Inspector, Assets, Console, Resources, AI Commands, Profiler, Timeline, and Project Settings over `MK_editor_core` data, disables Dear ImGui `.ini`/log persistence when `--no-user-config` is set, refreshes the Resources panel from live native D3D12 host availability before rendering, and extends smoke expectations to `editor_shell_backend=d3d12`, `editor_shell_panels=10`, and `editor_shell_sdl3=0`. Local evidence passed: `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests`, `tools/check-tidy.ps1 -Files editor/src/native_editor_app.cpp,editor/src/native_editor_launch.cpp,tests/unit/editor_native_shell_tests.cpp`, `tools/check-format.ps1`, `tools/check-native-desktop-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-validation-recipe-runner.ps1`, and full `tools/validate.ps1` with 85/85 tests passing. Local `tools/build-gui.ps1` remained blocked because the linked worktree `vcpkg_installed` tree lacked `imguiConfig.cmake`; hosted Windows MSVC, `PR Gate`, Linux, macOS, iOS, static analysis, Agent Static Guards, and CodeQL provided publication evidence for the panel slice.

## Phase 6 - Win32 Services Integration

**Goal:** Wire native clipboard, file dialogs, and reviewed process-runner workflows through existing first-party services.

**Files:**
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/native_editor_panels.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify if new core behavior is needed: `tests/unit/editor_core_tests.cpp`

- [x] **Step 1: Use existing Win32 file dialog service**

Construct `mirakana::win32::Win32FileDialogService` with the private owner window token inside `editor/src`. Route existing editor-core file-dialog request rows through that service.

- [x] **Step 2: Use existing Win32 clipboard service**

Construct `mirakana::win32::Win32Clipboard` and `mirakana::win32::Win32ClipboardTextAdapter` in `NativeEditorApp`. Route text clipboard operations through `MK_ui` / editor-core contracts where those contracts already exist.

- [x] **Step 3: Preserve reviewed process execution boundaries**

When panels need validation recipe execution or PIX helper review, keep using `Win32ProcessRunner` and existing allowlisted command review helpers. The ImGui shell must show review state and user confirmation before execution.

- [x] **Step 4: Add smoke counters**

Extend smoke output with:

```text
editor_shell_file_dialog_service=win32
editor_shell_clipboard_service=win32
editor_shell_reviewed_process_runner=win32
```

- [ ] **Step 5: Run service-focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-gui --target MK_editor MK_editor_native_shell_tests MK_platform_process_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-gui --output-on-failure -R "MK_editor_smoke|editor_native_shell|platform_process"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: service smoke passes and process policy remains reviewed.

**Done When:** Native shell services are wired through existing Win32 adapters and reviewed execution gates, not ad hoc shell code.

**Phase Evidence:** Candidate 5 is in progress in `codex/native-win32-editor-services-v1`. Official-source refresh used Microsoft Learn Common Item Dialog, Clipboard, `SetClipboardData`, and `CreateProcessW` documentation plus Context7 CMake `target_link_libraries` usage-requirement guidance. RED tests first failed because `NativeEditorApp` lacked service binding, file-dialog routing, clipboard routing, and reviewed process execution APIs. The implementation adds private `NativeEditorWin32Services` under `editor/src`, binds `mirakana::win32::Win32FileDialogService` with the private owner-window token, binds `mirakana::win32::Win32Clipboard` through `Win32ClipboardTextAdapter`, binds `Win32ProcessRunner`, requires explicit user confirmation before executing reviewed allowlisted `ProcessCommand` values, and extends smoke expectations to `editor_shell_file_dialog_service=win32`, `editor_shell_clipboard_service=win32`, and `editor_shell_reviewed_process_runner=win32`. Local dev evidence passed: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests MK_platform_process_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests|MK_platform_process_tests"`, `tools/check-tidy.ps1 -Files editor/src/native_editor_app.cpp,editor/src/native_editor_win32_services.cpp,tests/unit/editor_native_shell_tests.cpp`, `tools/check-public-api-boundaries.ps1`, `tools/check-native-desktop-contracts.ps1`, `tools/check-json-contracts.ps1`, `tools/check-validation-recipe-runner.ps1`, `tools/check-agents.ps1`, and full `tools/validate.ps1` with 85/85 tests passing. Local `tools/build-gui.ps1` and `desktop-gui` service smoke remain blocked at configure because the linked worktree `vcpkg_installed` tree lacks `imguiConfig.cmake`; `tools/bootstrap-deps.ps1` is policy-gated in this no-approval session, so hosted Windows MSVC remains required before this phase is publication-complete.

## Phase 7 - Native Viewport Display Foundation

**Goal:** Replace the removed SDL renderer viewport texture path with a first-party native D3D12 viewport display path.

**Files:**
- Create: `editor/src/native_viewport_surface.hpp`
- Create: `editor/src/native_viewport_surface.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/native_editor_panels.cpp`
- Modify: `editor/src/win32_imgui_d3d12_host.hpp`
- Modify: `editor/src/win32_imgui_d3d12_host.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify if backend-neutral renderer contracts change: `engine/renderer/**`
- Modify if RHI contracts change: `engine/rhi/**`

- [ ] **Step 1: Add RED viewport display tests**

Add tests with these names:

```text
editor native viewport display plan rejects missing d3d12 host
editor native viewport display plan records diagnostic-only viewport when renderer output is unavailable
editor native viewport display plan does not expose native texture handles
```

Expected initial failure: viewport display planning types do not exist.

- [ ] **Step 2: Implement diagnostic-first viewport**

The first implementation displays a stable ImGui child region with viewport diagnostics from `MK_editor_core` and smoke output:

```text
editor_shell_viewport_status=diagnostic_only
editor_shell_viewport_native_handles_exposed=0
```

- [ ] **Step 3: Implement D3D12 texture display only behind private adapter**

When promoting from diagnostic-only viewport output to D3D12 texture display, the adapter must own:

```text
offscreen render target allocation,
resource state transitions,
SRV descriptor lease,
ImTextureID conversion inside editor/src only,
resize-safe teardown,
smoke diagnostics.
```

Do not add `ImTextureID`, D3D12 descriptors, or native texture handles to editor-core or public engine headers.

- [ ] **Step 4: Run viewport checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-gui --target MK_editor MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-gui --output-on-failure -R "MK_editor_smoke|editor_native_shell"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1
```

Expected: diagnostic-only or native texture display path passes without public native handle leakage.

**Done When:** The editor viewport no longer depends on SDL-era texture display and has either validated diagnostic-only output or a private D3D12 texture display path with explicit smoke evidence.

**Phase Evidence:** Not started.

## Phase 8 - Material Preview GPU Display

**Goal:** Reintroduce editor material preview display through the native D3D12 editor shell without restoring the removed SDL texture bridge.

**Files:**
- Create or modify: `editor/src/native_material_preview_cache.hpp`
- Create or modify: `editor/src/native_material_preview_cache.cpp`
- Modify: `editor/src/native_editor_panels.cpp`
- Modify: `editor/src/win32_imgui_d3d12_host.hpp`
- Modify: `editor/src/win32_imgui_d3d12_host.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify if core model evidence changes: `tests/unit/editor_core_tests.cpp`

- [ ] **Step 1: Add RED material preview native-display tests**

Add tests with these names:

```text
editor native material preview plan rejects missing shader artifacts
editor native material preview plan reports diagnostic-only preview without gpu display
editor native material preview plan keeps d3d12 handles private
```

Expected initial failure: native material preview plan types do not exist.

- [ ] **Step 2: Keep preview readiness in `MK_editor_core`**

Do not move shader artifact discovery, material metadata, cooked material validation, or preview readiness policy out of `MK_editor_core`. The native shell consumes the existing readiness rows.

- [ ] **Step 3: Add private D3D12 preview display**

The native preview cache may create private D3D12 resources and ImGui texture IDs inside `editor/src`. It must report:

```text
editor_shell_material_preview_status=diagnostic_only
```

or:

```text
editor_shell_material_preview_status=d3d12
editor_shell_material_preview_native_handles_exposed=0
```

- [ ] **Step 4: Run preview checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-gui --target MK_editor MK_editor_native_shell_tests MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-gui --output-on-failure -R "MK_editor_smoke|editor_native_shell|MK_editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: material preview status is deterministic and native handles remain private.

**Done When:** Material preview display has a native shell path or an explicit diagnostic-only status without any SDL compatibility bridge.

**Phase Evidence:** Not started.

## Phase 9 - GUI Static Guards And Agent Surface Update

**Goal:** Replace deferred-shell agent/static needles with native-shell readiness needles so future drift is caught.

**Files:**
- Modify: `tools/check-json-contracts-030-tooling-contracts.ps1`
- Modify: `tools/check-vcpkg-environment.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-ai-integration-040-agent-surfaces.ps1`
- Modify: `tools/check-ai-integration-050-game-generation.ps1`
- Modify: `tools/check-ai-integration-051-generated-game-studio.ps1`
- Modify: `tools/check-ai-integration-060-editor-workflows.ps1`
- Modify: `tools/check-ai-integration-070-production-ledger.ps1`
- Modify: `tools/check-ai-integration-080-scaffold-smokes.ps1`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` if this plan is selected or closed.
- Generate: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `.cursor/skills/gameengine-editor/SKILL.md` if present.

- [x] **Step 1: Replace fail-closed needles**

Remove active checks that require:

```text
MK_editor visible shell is deferred after SDL3 removal
```

Replace them with active checks that require:

```text
MK_editor native Win32/D3D12 shell
--smoke-frames
editor_shell_sdl3=0
imgui win32-binding dx12-binding
```

Keep historical references only in archive/plan evidence where static checks allow historical prose.

- [x] **Step 2: Update manifest module status**

Change the `MK_editor` manifest row from `visible-shell-deferred-during-sdl3-removal` to a precise status such as:

```text
windows-native-editor-shell-smoke-ready
```

The purpose text must state what is ready and what remains unsupported:

```text
Ready: Win32 window, Dear ImGui Win32/D3D12 backend, smoke frames, core-backed panels.
Unsupported or later: cross-platform editor shells, full viewport display if Phase 7 is diagnostic-only, broad material preview GPU parity if Phase 8 is diagnostic-only, docking/multi-viewport unless separately selected.
```

- [x] **Step 3: Compose and verify agent surfaces**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: generated manifest and AI surface checks pass.

**Done When:** Static checks no longer require the deferred shell and now guard the native Win32 editor shell truth.

**Phase Evidence:** Candidate 3 updated deferred-shell needles to native-shell readiness guards for the host/smoke/build-gui slice. `engine/agent/manifest.fragments/005-applications.json`, `009-validationRecipes.json`, `010-aiOperableProductionLoop.json`, and `015-aiDrivenGameWorkflow.json` were updated and `engine/agent/manifest.json` was regenerated. `tools/check-json-contracts-030-tooling-contracts.ps1`, `tools/check-ai-integration-020-engine-manifest.ps1`, and `tools/check-validation-recipe-runner.ps1` now guard the active `desktop-gui` validation recipe and run-validation allowlist. Evidence passed: `tools/compose-agent-manifest.ps1 -Write`, `tools/check-validation-recipe-runner.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1`.

## Phase 10 - Documentation, Validation Closeout, And Publication

**Goal:** Close the milestone with complete docs, validation evidence, and GitHub Flow publication.

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/dependencies.md`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/agent-operational-reference.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: this plan file with phase evidence.
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` if closing the selected plan.
- Generate: `engine/agent/manifest.json`

- [ ] **Step 1: Update user-facing editor docs**

`docs/editor.md` must describe:

```text
how to build MK_editor,
how to launch smoke frames,
what panels are ready,
what viewport/material-preview states are ready or diagnostic-only,
that SDL3 is absent,
that Dear ImGui is editor-shell-only.
```

- [ ] **Step 2: Update validation docs**

`docs/testing.md` and `docs/workflows.md` must state:

```text
tools/build-gui.ps1 is the supported GUI validation lane,
tools/evaluate-cpp23.ps1 -Gui is restored,
desktop-gui uses vcpkg bootstrap and does not configure-time install dependencies,
GUI validation is Windows-native and does not claim macOS/Linux parity.
```

- [ ] **Step 3: Run final local validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/evaluate-cpp23.ps1 -Gui
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: all checks pass, or any host/tool blocker is recorded with exact command and failure.

- [ ] **Step 4: Prepare publication**

Use GitHub Flow:

```powershell
git status --short
gh pr view --json headRefOid,statusCheckRollup,url
```

Then publish through the repository-supported PR path. Do not push to protected branches, force-push, bypass checks, or mark this plan complete before the latest PR head has hosted evidence appropriate to the change.

**Done When:** `MK_editor` launches through the supported GUI lane, docs and agent surfaces describe the native shell accurately, SDL3 remains absent, local validation passes, and hosted PR evidence is recorded.

**Phase Evidence:** Not started.

## Risk Ledger

| Risk | Mitigation |
| --- | --- |
| Dear ImGui backend API changes across versions. | Re-check upstream docs/source at each phase and pin behavior through compile and smoke tests. |
| vcpkg `imgui` feature names or backend build shape change. | Validate against `external/vcpkg/ports/imgui/vcpkg.json` and keep dependency-policy checks specific to selected features. |
| Hosted Windows GUI smoke is flaky. | Keep smoke bounded to two frames, use deterministic exit, record WARP fallback separately from hardware readiness, and preserve focused unit tests for launch/allocator/message logic. |
| Native handles leak while wiring ImGui/D3D12. | Keep all native code under `editor/src`, run public API and native desktop contract checks before closeout, and add explicit static needles for no public ImGui/native symbols. |
| Scope expands into full editor productization. | Initial closeout requires launch, smoke, and core-backed panels only. Viewport and material preview have separate phases and explicit diagnostic-only statuses. |
| Old SDL3 code is copied back for convenience. | Static policy must reject SDL3 dependency/source/target references in active GUI surfaces; Git history may be read only to understand panel behavior. |
| Runtime/game UI accidentally depends on Dear ImGui. | Dependency and docs must state Dear ImGui is editor-shell-only; runtime UI remains `MK_ui` and renderer adapters. |

## Completion Definition

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` configures, builds, and tests the native GUI lane.
- `MK_editor --smoke-frames 2 --no-user-config` exits successfully and reports deterministic `editor_shell_*` counters including `editor_shell_sdl3=0`.
- `vcpkg.json` `desktop-gui` declares Dear ImGui with Win32/DX12 backend features and no SDL3 dependencies.
- `THIRD_PARTY_NOTICES.md`, `docs/dependencies.md`, and `docs/legal-and-licensing.md` record Dear ImGui correctly.
- `MK_editor_core` remains free of Dear ImGui, Win32, D3D12, DXGI, COM, renderer/RHI ownership, and native handles.
- Public API and native desktop contract checks pass.
- Agent manifests, static checks, docs, and skills describe the native shell instead of the deferred shell.
- Cross-platform editor shell parity, docking, multi-viewport, full material-preview GPU parity, and full viewport rendering are claimed only if their phases have actual validation evidence; otherwise they remain explicit future work.

## Self-Review Notes

- Spec coverage: the plan covers dependency/legal work, private native shell architecture, launch smoke, core-backed panels, services, viewport/material-preview follow-ups, static/agent updates, and final validation.
- Open-ended gap scan: no phase leaves required behavior as unspecified work; unsupported work is explicitly out of scope or assigned to a named later phase in this plan.
- Type consistency: plan names use `NativeEditorLaunchOptions`, `MK_editor_shell_win32`, `Win32ImguiD3d12Host`, and `MK_editor_native_shell_tests` consistently.
