# Local Shader Toolchain Discovery Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make locally installed shader tools discoverable from User/Machine environment state so Vulkan SPIR-V validation can become ready without committing tool binaries.

**Architecture:** Keep tool binaries outside the repository and detect them through first-party validation scripts and editor-private discovery. Public game APIs remain unchanged; discovery stays in tools/editor integration surfaces.

**Tech Stack:** PowerShell, Chocolatey/Vulkan SDK, vcpkg tool installs, C++23 editor shell discovery, JSON agent manifest, Markdown docs.

---

### Task 1: Local Tool Installation Evidence

**Files:**
- Environment: User `MK_SHADER_TOOLCHAIN_ROOT`
- Install roots: `C:/VulkanSDK/1.4.341.1` and `C:/tmp/gameengine-vcpkg-toolchain/x64-windows/tools`

- [x] **Step 1: Install user-local fallback shader tools**

Run:

```powershell
external\vcpkg\vcpkg.exe install 'spirv-tools[tools]' directx-dxc --triplet x64-windows --x-install-root=C:\tmp\gameengine-vcpkg-toolchain --classic --recurse
```

Expected: `spirv-val.exe` and `dxc.exe` exist under `C:\tmp\gameengine-vcpkg-toolchain\x64-windows\tools`.

- [x] **Step 2: Install the official Vulkan SDK**

Run:

```powershell
winget install --id KhronosGroup.VulkanSDK --exact --source winget --accept-package-agreements --accept-source-agreements --silent --disable-interactivity
```

Expected: `dxc.exe` and `spirv-val.exe` exist under `C:\VulkanSDK\1.4.341.1\Bin`.

- [x] **Step 3: Record a process/user toolchain root**

Run:

```powershell
[Environment]::SetEnvironmentVariable('MK_SHADER_TOOLCHAIN_ROOT', 'C:\VulkanSDK\1.4.341.1', 'User')
```

Expected: future shells and validation scripts can discover the tool root without relying on the current process PATH.

### Task 2: Script Discovery

**Files:**
- Modify: `tools/check-shader-toolchain.ps1`

- [x] **Step 1: Reuse common environment helpers**

Dot-source `tools/common.ps1` so shader tool detection can use `Find-CommandOnCombinedPath` and `Get-EnvironmentVariableAnyScope`.

- [x] **Step 2: Add configured tool roots**

Search `MK_SHADER_TOOLCHAIN_ROOT`, `VULKAN_SDK`, `VK_SDK_PATH`, and vcpkg tool roots before falling back to Windows SDK DXC.

- [x] **Step 3: Verify Vulkan readiness**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
```

Expected: `vulkan_spirv=ready` and `dxc_spirv_codegen=ready` when `MK_SHADER_TOOLCHAIN_ROOT` points at the local vcpkg tool install.

### Task 3: Editor and Agent Sync

**Files:**
- Modify: `editor/src/main.cpp`
- Modify: `docs/editor.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

- [x] **Step 1: Extend editor-private discovery**

Add `MK_SHADER_TOOLCHAIN_ROOT` and vcpkg tool-root discovery to editor-known installed shader tools without changing public headers.

- [x] **Step 2: Update docs and AI guidance**

Document User/Machine PATH, `MK_SHADER_TOOLCHAIN_ROOT`, Vulkan SDK variables, and vcpkg tool roots as reviewed installed tool locations.

### Task 4: Validation

**Files:**
- Update: `docs/superpowers/plans/README.md`
- Update: `docs/roadmap.md`

- [x] **Step 1: Run focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

- [x] **Step 2: Run default validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Result: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Vulkan SPIR-V readiness is now ready through `MK_SHADER_TOOLCHAIN_ROOT`; remaining diagnostics are Metal/Apple host-gated tools, Android signing/device state, and strict tidy compile database availability for the active generator.
