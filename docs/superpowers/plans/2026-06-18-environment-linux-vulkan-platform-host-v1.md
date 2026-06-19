# 2026-06-18 Environment Linux Vulkan Platform Host v1

**Plan ID:** `environment-linux-vulkan-platform-host-v1`
**Status:** Active child slice
**Owner:** `engine/runtime_host` / `tools` / `MK_renderer` / agent-surface governance
**Parent:** [Environment Commercial Excellence v1](2026-06-13-environment-commercial-excellence-v1.md)

## Goal

Add the clean-break Linux Vulkan platform readiness path required before `environment_platform_linux_vulkan_ready` can ever move from host-gated to ready.

This plan deliberately starts with the validation contract, not a ready claim: Linux Vulkan must be proven by a first-party Linux desktop/runtime host and a Linux package validation lane. Windows Vulkan, Win32 packages, Android Vulkan, compile-only Linux builds, backend parity rows, or commercial aggregate rows cannot satisfy this plan.

## Official Source Baseline

Use Context7 for Vulkan, CMake, and GitHub Actions documentation when the MCP is available. If Context7 is unavailable, use primary official sources only:

- Khronos / LunarG Linux Vulkan SDK setup and tool documentation.
- Khronos Vulkan validation layer guidance, including `VK_LAYER_KHRONOS_validation`.
- Khronos SPIR-V tooling expectations, including `spirv-val`.
- CMake preset, build preset, test preset, and CPack documentation.
- GitHub Actions hosted-runner documentation for any future hosted Linux lane.

No blog, sample, Stack Overflow, or unlicensed code may be copied into the implementation.

## Current Truth

- `sample_desktop_runtime_game`, `sample_desktop_runtime_shell`, `tools/package-desktop-runtime.ps1`, `tools/validate-desktop-game-runtime.ps1`, and `tools/validate-installed-desktop-runtime.ps1` are Win32/x64-windows lanes.
- `MK_add_desktop_runtime_game` accepts only `HOST_BACKEND win32` and rejects non-Windows hosts.
- `DesktopGameRunner`, `HeadlessWindow`, `VirtualLifecycle`, `IFileSystem`, `NullRenderer`, and `shader_bytecode` are reusable cross-platform foundations.
- The Vulkan RHI has the Linux XCB surface foundation needed by the future Linux presentation path: `SurfaceHandle::context`, `SurfacePlatform::xcb`, Linux `VK_KHR_surface` + `VK_KHR_xcb_surface` instance-extension planning, and a private `vkCreateXcbSurfaceKHR` surface-support probe. This is a prerequisite only; it does not create Linux packaging, installed validation, strict package smoke, or Linux readiness.
- `MK_runtime_host_linux` now adds the first-party Linux XCB runtime host foundation: `LinuxDesktopHostRequest`, `LinuxDesktopHostReadinessReport`, `LinuxXcbWindow`, `LinuxDesktopEventPump`, and `LinuxDesktopGameHost` behind private dynamic `libxcb.so.1` loading, no public XCB/Vulkan handles, and `NullRenderer` fallback. This is still not Linux Vulkan presentation, package tooling, installed validation, strict package smoke, or Linux readiness.
- `environment_platform_windows_vulkan_ready=1` is a Windows Vulkan row only.
- `environment_platform_linux_vulkan_ready=0`, `environment_platform_requires_linux_vulkan_host_evidence=1`, and `environment_all_platform_unconditional_ready=0` must remain true until this plan's ready gate is fully implemented and validated.

## Constraints

- Do not change the Windows desktop runtime package lane into a cross-platform script.
- Do not add SDL3, GLFW, Qt, or UI/window middleware without a separate architecture decision and dependency/license updates.
- Do not expose public native window, Vulkan instance/device/surface/swapchain, or RHI handles.
- Do not mark `environment_platform_linux_vulkan_ready`, `environment_all_platform_unconditional_ready`, `environment_commercial_ready`, or broad `environment_ready` ready in this plan until every exact Linux evidence row exists.
- Keep Linux and Windows Vulkan gates separate: `vulkan-strict` is Windows/package evidence, `vulkan-strict-linux` is Linux platform evidence.
- Keep optional dependencies out of CMake configure-time installation. Use `tools/bootstrap-deps.ps1` for dependency installation if future Linux packages require optional dependencies.

## Implementation Tasks

- [x] Add `environment-platform-linux-vulkan-host-gate` to the reviewed validation recipe surface.
- [x] Add `tools/validate-linux-vulkan-runtime-host.ps1` to report the exact Linux blockers and fail when `-RequireReady` is used before the Linux lane exists.
- [x] Add `vulkan-strict-linux` as a separate manifest host gate.
- [x] Wire `environment_platform_linux_vulkan_ready` and `environmentPlatformReadinessRows.environment_platform_linux_vulkan` to the new host-gate recipe while keeping state `host-gated`.
- [x] Add static runner coverage for dry-run and missing host-gate acknowledgement.
- [x] Add the Linux XCB Vulkan surface foundation in the RHI without exposing native XCB, Vulkan instance/device/surface, or swapchain handles.
- [x] Implement `engine/runtime_host/linux` with first-party Linux host/event-pump/window boundaries.
- [x] Add the Linux-only `MK_runtime_host_linux` CMake target without weakening `MK_runtime_host_win32`.
- [ ] Add future Linux Vulkan presentation target such as `MK_runtime_host_linux_presentation` without exposing native handles.
- [ ] Add a Linux desktop/runtime package script and installed package validator, rather than extending the Win32 package script.
- [ ] Add Linux Vulkan package smoke that proves Vulkan SDK tools, Linux ICD/runtime/driver, `VK_LAYER_KHRONOS_validation`, DXC SPIR-V CodeGen, `spirv-val`, strict package execution, synchronization/readback counters, zero diagnostics, and zero native-handle access.
- [ ] Promote `environment_platform_linux_vulkan_ready=1` only after the Linux package smoke emits package-visible counters and static guards reject Windows/compile-only inference.

## Done When

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-platform-linux-vulkan-host-gate` returns a reviewed command plan for `tools/validate-linux-vulkan-runtime-host.ps1 -RequireReady`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-platform-linux-vulkan-host-gate` rejects execution unless `HostGateAcknowledgements` contains `vulkan-strict-linux`.
- On non-ready hosts, `tools/validate-linux-vulkan-runtime-host.ps1` reports exact blockers and keeps `environment_platform_linux_vulkan_ready=0`.
- Manifest fragments, composed manifest, game guidance, validation recipe map, static checks, and plan registry agree that Linux Vulkan is host-gated and separate from Windows Vulkan.
- A future ready promotion includes focused Linux host/package tests, Linux package smoke, installed package validation, shader/toolchain validation, manifest/static guard updates, and full `tools/validate.ps1`.

## Validation Evidence

| Check | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed | Rebuilt `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-linux-vulkan-runtime-host.ps1` | Passed | Report-only mode emitted `host=windows`, `status=host-gated`, `environment_platform_linux_vulkan_ready=0`, `environment_platform_requires_linux_vulkan_host_evidence=1`, `environment_all_platform_unconditional_ready=0`, `environment_platform_windows_vulkan_inferred=0`, and non-Linux host-gated Linux tool/package/installed-validator counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe environment-platform-linux-vulkan-host-gate` | Passed | Returned `tools/validate-linux-vulkan-runtime-host.ps1 -RequireReady` with host gate `vulkan-strict-linux`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe environment-platform-linux-vulkan-host-gate` | Rejected as expected | Failed closed with `missing-host-gate-acknowledgement` until `vulkan-strict-linux` is acknowledged. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-linux-vulkan-runtime-host.ps1 -RequireReady` | Failed as expected on Windows | Reported exact blockers and refused ready evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-validation-recipe-runner.ps1` | Passed | Dry-run and missing host-gate contracts cover `environment-platform-linux-vulkan-host-gate`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest, game manifest, recipe ids, and static JSON contracts agree. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent surfaces agree with the new host-gated contract. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Agent instruction/skill/rule consistency stayed green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed | Text formatting stayed green. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Full format check stayed green. |
| `git diff --check` | Passed | No whitespace errors. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | Passed | 19 static checks plus diagnostic-only mobile/apple host gates passed; no C++ runtime build was required for this contract-only slice. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_backend_scaffold_tests` | Passed | Linux host extension planning and XCB surface-handle validation compile through the RHI backend scaffold tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_backend_scaffold_tests` | Passed | `MK_backend_scaffold_tests` validates `VK_KHR_xcb_surface` planning and fail-closed XCB context/window diagnostics without promoting Linux readiness. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full validation passed with `validate: ok` and `100% tests passed, 0 tests failed out of 131`; Apple/Metal host checks remain host-gated or diagnostic-only on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_tests` | Passed | Common Linux desktop host contract compiled on Windows and kept non-Linux host-gated behavior value-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_host_tests` | Passed | `MK_runtime_host_tests` covered invalid Linux host requests, non-Linux host gating, fallback reporting, and zero native-handle access. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-linux-vulkan-runtime-host.ps1` | Passed | Validator now checks `engine/runtime_host/linux`, `linux_desktop_game_host.hpp`, and `MK_runtime_host_linux` CMake presence for the first-party host row on Linux; Windows remains host-gated and does not promote readiness. |
