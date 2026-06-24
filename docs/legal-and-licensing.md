# Legal and Licensing Policy

This is an engineering policy, not legal advice.

## Default License

First-party files use:

```text
SPDX-License-Identifier: LicenseRef-Proprietary
```

The license text is stored in `LICENSES/LicenseRef-Proprietary.txt`.

## Allowed Without Extra Review

These licenses are generally acceptable for tooling or runtime dependencies when attribution and notice obligations are recorded:

- MIT
- BSD-2-Clause
- BSD-3-Clause
- Apache-2.0
- Zlib
- ISC
- BSL-1.0
- CC0-1.0 for assets

## Requires Review

Do not add these without an explicit licensing review note:

- GPL
- AGPL
- LGPL
- MPL
- EPL
- CC-BY-SA
- CC-BY-NC
- CC-BY-ND
- custom marketplace asset licenses

## Required Records

Every external dependency or asset must be recorded in `THIRD_PARTY_NOTICES.md` before it is distributed with the engine, editor, tools, examples, or sample games.

Optional package-manager declarations in `vcpkg.json` must also be mirrored in `THIRD_PARTY_NOTICES.md` as planned dependencies before integration work starts. The current runtime UI Windows DirectWrite text/font adapter, Windows TSF native IME session adapter, Windows UI Automation runtime UI accessibility adapter, and `desktop-editor` lane are first-party and dependency-free: they use host SDK Win32/DXGI/D3D12/DirectWrite/TSF/UIA boundaries behind private implementation files and must not make UI middleware, native handles, COM interfaces, bundled fonts, redistributable font files, native candidate UI, cross-platform IME parity, full screen-reader/UIA pattern-event parity, cross-platform accessibility parity, or graphics API types part of the production runtime game UI foundation. Future HarfBuzz, FreeType, ICU, bundled/distributable font, text, image, or platform adapter dependencies are legal/dependency-gated until `license-audit`, `vcpkg.json`, `docs/dependencies.md`, and `THIRD_PARTY_NOTICES.md` are updated. The `asset-importers` feature includes OpenEXR as an optional HDR scene-linear source dependency gate: OpenEXR, Imath, libdeflate, and OpenJPH are recorded in notices and must remain isolated behind future `MK_tools` adapter code without leaking OpenEXR headers, third-party pixel/channel types, runtime source parsing, OpenEXR tool execution, or broad environment asset-pipeline readiness claims into public gameplay APIs. The `asset-importers` feature also includes KTX Software as an optional KTX2/Basis texture review dependency: it is recorded in notices with its upstream `LICENSES/*` bundle and must remain isolated behind `MK_assets` / `MK_tools` review rows without leaking KTX headers, native texture handles, runtime transcoding, compression-tool execution, or GPU upload claims into public gameplay APIs. The `directstorage-win32` feature includes Microsoft DirectStorage SDK 1.3.0 through the vcpkg `dstorage` port as an optional Windows-only MAVG byte-range IO adapter dependency. It is recorded in notices and must remain isolated behind a private adapter in `MK_platform_win32`; public runtime/gameplay APIs must not leak `dstorage.h`, `IDStorage*`, `IUnknown*`, `HANDLE`, `void*`, D3D12/DXGI resources, DirectStorage queue/status/file objects, GPU destinations, GDeflate, DirectStorage 1.4 preview APIs, or broad readiness claims. The `physics-jolt` feature is the current native physics middleware adapter example: Jolt Physics is optional, MIT licensed, recorded in notices, and must remain isolated behind `MK_physics_jolt` without leaking third-party types into public gameplay APIs. The `network-enet` feature follows the same policy: ENet is optional, MIT licensed, and recorded in notices, and must remain isolated behind `MK_runtime_network_enet` without leaking ENet headers, symbols, sockets, or native transport handles into public game APIs.

First-party sample environment preset packs such as `games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/environment_presets.gepresetpack` are proprietary repository content under `LicenseRef-Proprietary`. They must carry internal provenance, license, package-size, installed-size, decoded-memory, GPU-memory, and preset-level validation metadata before distribution; no `THIRD_PARTY_NOTICES.md` entry is required unless an external asset, marketplace license, or third-party generated content is added. Environment Highest Commercial Readiness v1 Task 8 keeps the production AAA preset asset-library validator first-party only: `environment_aaa_preset_asset_library_ready=1` requires complete license/provenance rows for all 156 objective asset rows, package-budget rows for all 156 rows, zero external asset rows, zero missing objective rows, and no backend/package-script/native-handle execution. Adding marketplace or third-party preset assets must update `THIRD_PARTY_NOTICES.md` before any distribution claim.

Mobile template dependencies declared through Gradle or official platform SDK tooling must also be recorded before package builds are distributed. Official Android SDK, Android NDK, Google Maven artifacts, Xcode, UIKit, Foundation, Metal, and related Apple SDK frameworks are toolchain/platform dependencies; do not vendor their binaries into the repository. Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit are official Microsoft host diagnostics; do not vendor their binaries, installers, symbol caches, or captured traces into the repository.

For each item, record:

- name
- source URL
- retrieved date
- version or commit
- author or copyright holder
- SPDX license identifier or custom license reference
- modification status
- distribution target

## Prohibited Sources

Do not copy from:

- license-less GitHub repositories
- Stack Overflow answers without license review
- blog snippets without explicit license
- book sample code unless the license permits use
- decompiled or extracted commercial game assets
- AI outputs that appear to reproduce identifiable third-party code or assets

## First-Party Runtime UI Clean-Room Source Gate

`docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md` records the allowed source classes for runtime/editor UI platform work. `tools/check-first-party-ui-clean-room.ps1` enforces the matching public-token guard through `check-ai-integration`. This gate does not add third-party code, assets, fonts, icons, screenshots, themes, samples, package-manager dependencies, or redistribution obligations.

Unity, Unreal Engine, Godot, UI middleware, Microsoft SDK, Apple SDK, HarfBuzz, FreeType, AT-SPI2, Vulkan, and W3C references may be used only as official documentation for category research, host gates, or private adapter-boundary planning. They must not be copied into implementation source, sample assets, serialized UI imports, product-facing names, public API shapes, editor layouts, visual themes, or marketing compatibility/parity claims. Any future dependency, redistributable font/icon/image asset, UI middleware, or third-party adapter selection still requires `license-audit`, `vcpkg.json`, `docs/dependencies.md`, and `THIRD_PARTY_NOTICES.md` updates before use.

## AI Output

AI-generated code and assets still require review. Record substantial AI-generated assets, prompts, tool names, generation dates, and review decisions when they become distributable content.

## Editor game module driver and third-party DLL ABI (1.0 scope)

For MIRAIKANAI Engine **1.0**, the editor **Play-In-Editor** path may load a **same-repository, same-engine-build** native module that implements `GameEngine.EditorGameModuleDriver.v1` and the reviewed factory export (`mirakana_create_editor_game_module_driver_v1`). That contract is an **internal interchange** for AI-operable workflows and diagnostics; it is **not** a supported **stable third-party binary ABI** for shipping arbitrary out-of-tree game or middleware DLLs against multiple engine minors or toolchains.

A **vendor-stable** editor extension surface would require its own program of work: explicit versioned C (or stable C++) ABI, export surface and calling convention documentation, symbol visibility and MSVC runtime pairing policy, compatibility tests across releases, and distribution/legal packaging for third-party binaries. None of that is in the **1.0 ready claim** for this repository. Future releases may introduce a separate SDK plan; until then, treat out-of-tree editor DLLs as **unsupported** for compatibility guarantees.

Implementation evidence and session policy rows remain governed by `docs/specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md` plus archived `editor-productization` evidence in `docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md`; see also `docs/dependencies.md` (Editor native module boundary).

Host loaders and search-path policy for Windows remain defined by the platform SDK (for example [LoadLibraryExW](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw)); MIRAIKANAI does not redefine those APIs and does not promise that arbitrary third-party binaries loaded through them remain compatible across engine releases.
