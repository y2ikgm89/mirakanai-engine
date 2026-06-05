# 2026-06-06 MAVG DirectStorage SDK Dependency Gate v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-directstorage-sdk-dependency-gate-v1`

**Status:** Completed/published as draft PR #469.

Focused child over `mavg-runtime-lod-milestone-v1`, stacked after draft PR #466 (`mavg-win32-iocp-file-io-worker-v1`).

**Goal:** Add the clean optional dependency/legal/tooling gate for Microsoft DirectStorage SDK so later MAVG native queue execution can compile against the official SDK without making DirectStorage part of the default build, public API, installed Mirakanai SDK, or current LOD readiness claim.

**Architecture:** Keep `MK_runtime` and public gameplay/runtime headers free of `dstorage.h`, `IDStorage*`, `DSTORAGE_*`, `ID3D12Fence`, and native handles. The only implementation in this child is a Windows-only optional CMake lane that uses vcpkg `dstorage`, links the imported `Microsoft::DirectStorage` target, copies the redistributable DLLs next to a smoke executable, and runs a no-factory-call compile/link smoke test. Later children may add a private DirectStorage adapter only after this dependency gate is green.

**Tech Stack:** C++23, CMake 3.30 presets, vcpkg manifest features, Microsoft DirectStorage SDK 1.3.0 via vcpkg `dstorage`, `Microsoft::DirectStorage`, PowerShell validation wrappers, dependency/legal/static policy scripts, Context7 `/microsoft/vcpkg`, Microsoft Learn DirectStorage docs, NuGet package/license metadata.

---

## Official Source Audit

Checked on 2026-06-06:

- NuGet `Microsoft.Direct3D.DirectStorage` stable package is 1.3.0, with newer prerelease `1.4.0-preview1-2603.504`. This gate selects stable 1.3.0 through the repository-pinned vcpkg port.
- NuGet package metadata states that the package includes the DirectStorage SDK and redistributable binaries, that `LICENSE.txt` applies to `native/bin`, and that `LICENSE-CODE.txt` applies to `native/include`.
- NuGet license metadata for 1.3.0 exposes the Microsoft DirectStorage SDK license for the distributable binary side; package changelog states `dstorage.h` and `dstorageerr.h` are covered by MIT as of 1.2.
- Microsoft Learn DirectStorage interface docs state DirectStorage programming interfaces are declared in `dstorage.h`; `DStorageSetConfiguration1` is also declared in `dstorage.h` and must be called before `DStorageGetFactory` when configuration is needed. This smoke does not call either API.
- Context7 `/microsoft/vcpkg` confirms optional dependencies belong in manifest `features`, selected features are installed through `--x-feature`, and `VCPKG_MANIFEST_INSTALL=OFF` means dependencies must be installed before configure.
- Local official vcpkg port audit found `external/vcpkg/ports/dstorage` version 1.3.0 with usage:

```cmake
find_package(dstorage CONFIG REQUIRED)
target_link_libraries(main PRIVATE Microsoft::DirectStorage)
```

## Scope

In scope:

- Add `directstorage-sdk` to `vcpkg.json` with only the official `dstorage` dependency.
- Add `--x-feature=directstorage-sdk` to `tools/bootstrap-deps.ps1`.
- Add `MK_ENABLE_DIRECTSTORAGE_SDK`, `directstorage-sdk` configure/build/test presets, and `tools/validate-directstorage-sdk.ps1`.
- Add `MK_runtime_host_win32_directstorage_sdk_tests` as a Windows-only compile/link/package-copy smoke that includes `dstorage.h` and `dstorageerr.h`, links `Microsoft::DirectStorage`, copies `Microsoft::DirectStorage` / `Microsoft::DirectStorageCore` DLLs when present, and verifies the import entry point is link-visible without executing `DStorageGetFactory`.
- Extend dependency/legal docs, `THIRD_PARTY_NOTICES.md`, `check-dependency-policy`, native desktop/public API guards, agent docs, manifest fragments, composed manifest, and static checks.
- Keep #466 IOCP worker evidence retained while selecting this dependency gate as the active child.

Out of scope:

- Calling `DStorageGetFactory`, `DStorageSetConfiguration1`, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `IDStorageQueue3`, `ID3D12Fence`, or any DirectStorage request enqueue/status/fence API.
- DirectStorage file IO execution, resident mount mutation, renderer/RHI resource access, autonomous package streaming worker execution, async-overlap/performance claims, automatic eviction policy, GPU memory pressure integration, Vulkan/Metal native IO parity, mesh shaders, deformation, ray tracing, Nanite compatibility/equivalence/superiority, benchmark superiority, or broad optimization.
- Installing/exporting DirectStorage through the public `MirakanaiConfig.cmake` SDK or default validation lane.

## Files

- Modify: `vcpkg.json`
- Modify: `tools/bootstrap-deps.ps1`
- Add: `tools/validate-directstorage-sdk.ps1`
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`
- Add: `tests/unit/runtime_host_win32_directstorage_sdk_tests.cpp`
- Modify: `tools/check-dependency-policy.ps1`
- Modify: `tools/check-native-desktop-contracts.ps1`
- Modify: `tools/check-public-api-boundaries.ps1`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `docs/superpowers/plans/2026-06-06-mavg-win32-iocp-file-io-worker-v1.md`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-010-agent-baseline.ps1`
- Modify: `tools/check-ai-integration-117-mavg-win32-iocp-file-io-worker.ps1`
- Add: `tools/check-ai-integration-118-mavg-directstorage-sdk-dependency-gate.ps1`
- Modify if needed: `tools/check-json-contracts-030-tooling-contracts.ps1`

## Tasks

### Task 1: Add Dependency, Legal, And Static Policy Gate

- [x] Add `directstorage-sdk` vcpkg feature and bootstrap installation wiring.
- [x] Record Microsoft DirectStorage SDK 1.3.0 in dependency docs, legal docs, and third-party notices.
- [x] Extend dependency and public API boundary checks so `dstorage.h`, `IDStorage*`, and `DSTORAGE_*` cannot leak into public headers.

### Task 2: Add DirectStorage SDK Compile/Link/Package-Copy Smoke

- [x] Add `MK_ENABLE_DIRECTSTORAGE_SDK`, `directstorage-sdk` presets, and `tools/validate-directstorage-sdk.ps1`.
- [x] Add `MK_runtime_host_win32_directstorage_sdk_tests` that includes SDK headers, links `Microsoft::DirectStorage`, copies runtime DLLs, and runs without creating a DirectStorage factory.
- [ ] Run `tools/bootstrap-deps.ps1`, inspect installed license/header evidence, then run `tools/validate-directstorage-sdk.ps1`.

Evidence: `tools/bootstrap-deps.ps1` is command-policy approval-gated in this no-approval session. `vcpkg_installed/x64-windows/share/dstorage/dstorage-config.cmake` and `vcpkg_installed/x64-windows/include/dstorage.h` were absent. `tools/validate-directstorage-sdk.ps1` reached CMake configure and failed at `find_package(dstorage CONFIG REQUIRED)` because `dstorage` is not installed. The local `external/vcpkg/ports/dstorage/vcpkg.json` port audit still shows official vcpkg port version `1.3.0`.

### Task 3: Sync Agent Surfaces And Validate

- [x] Mark `mavg-win32-iocp-file-io-worker-v1` completed/published through draft PR #466 and select `mavg-directstorage-sdk-dependency-gate-v1` as active.
- [x] Update current capabilities, roadmap, MAVG architecture spec, parent milestone, master plan pointer evidence, plan registry, manifest fragments, composed manifest, and static checks.
- [x] Run focused policy/static validation: `tools/check-dependency-policy.ps1`, `tools/check-native-desktop-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, formatting, and `git diff --check`.
- [x] Run full `tools/validate.ps1` unless a concrete host/tool blocker is found after focused checks.

Evidence: `tools/check-dependency-policy.ps1`, `tools/check-native-desktop-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/format.ps1`, `tools/check-format.ps1`, and `git diff --check` passed after agent-surface sync. Full `tools/validate.ps1` passed on 2026-06-06 with 109/109 CTest tests passing. The optional `tools/validate-directstorage-sdk.ps1` lane remains blocked until `tools/bootstrap-deps.ps1` can run in an approval-capable session and install vcpkg `dstorage`. Published as stacked draft PR #469 (`https://github.com/y2ikgm89/mirakanai-engine/pull/469`) on branch `codex/mavg-directstorage-sdk-dependency-gate-v1`.

## Done When

- `directstorage-sdk` is optional, vcpkg-backed, documented, and legally recorded.
- The default build remains dependency-free and no public header exposes DirectStorage symbols.
- The optional DirectStorage SDK lane can compile/link/run its smoke executable after `bootstrap-deps`.
- Manifest/docs/static checks describe only SDK dependency readiness, not native DirectStorage IO execution or Nanite/performance claims.
- Full validation passes or records a concrete host/tool blocker.

## Non-Claims

- No DirectStorage factory/queue/status/fence/request execution.
- No DirectStorage-backed MAVG payload loading yet.
- No autonomous streaming workers beyond completed Win32 IOCP evidence.
- No async-overlap/performance, benchmark superiority, or broad optimization claim.
- No Vulkan/Metal native IO parity.
- No mesh shader, deformation, ray tracing, Nanite compatibility/equivalence/superiority, or legal freedom-to-operate claim.
