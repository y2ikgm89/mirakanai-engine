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

Optional package-manager declarations in `vcpkg.json` must also be mirrored in `THIRD_PARTY_NOTICES.md` as planned dependencies before integration work starts. The current `desktop-editor` lane is first-party and dependency-free: it uses host SDK Win32/DXGI/D3D12/DirectWrite boundaries behind private editor implementation files and must not make UI middleware, native handles, or graphics API types part of the production runtime game UI foundation. Future HarfBuzz, FreeType, ICU, font, text, image, or platform adapter dependencies are legal/dependency-gated until `license-audit`, `vcpkg.json`, `docs/dependencies.md`, and `THIRD_PARTY_NOTICES.md` are updated. The `asset-importers` feature includes KTX Software as an optional KTX2/Basis texture review dependency: it is recorded in notices with its upstream `LICENSES/*` bundle and must remain isolated behind `MK_assets` / `MK_tools` review rows without leaking KTX headers, native texture handles, runtime transcoding, compression-tool execution, or GPU upload claims into public gameplay APIs. The `directstorage-sdk` feature is an optional Microsoft DirectStorage SDK dependency gate: Microsoft DirectStorage SDK 1.3.0 is recorded in notices, `native/bin` remains governed by `LicenseRef-Microsoft-DirectStorage-SDK`, `native/include` headers are MIT through `LICENSE-CODE.txt`, and `dstorage.h`, `IDStorage*`, `DSTORAGE_*`, `ID3D12Fence`, or redistributable DLL handling must remain private to the explicit SDK smoke or later accepted DirectStorage adapter plans without leaking into public gameplay APIs. The `physics-jolt` feature is the current native physics middleware adapter example: Jolt Physics is optional, MIT licensed, recorded in notices, and must remain isolated behind `MK_physics_jolt` without leaking third-party types into public gameplay APIs. The `network-enet` feature follows the same policy: ENet is optional, MIT licensed, recorded in notices, and must remain isolated behind `MK_runtime_network_enet` without leaking ENet headers, symbols, sockets, or native transport handles into public game APIs.

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

## AI Output

AI-generated code and assets still require review. Record substantial AI-generated assets, prompts, tool names, generation dates, and review decisions when they become distributable content.

## Editor game module driver and third-party DLL ABI (1.0 scope)

For MIRAIKANAI Engine **1.0**, the editor **Play-In-Editor** path may load a **same-repository, same-engine-build** native module that implements `GameEngine.EditorGameModuleDriver.v1` and the reviewed factory export (`mirakana_create_editor_game_module_driver_v1`). That contract is an **internal interchange** for AI-operable workflows and diagnostics; it is **not** a supported **stable third-party binary ABI** for shipping arbitrary out-of-tree game or middleware DLLs against multiple engine minors or toolchains.

A **vendor-stable** editor extension surface would require its own program of work: explicit versioned C (or stable C++) ABI, export surface and calling convention documentation, symbol visibility and MSVC runtime pairing policy, compatibility tests across releases, and distribution/legal packaging for third-party binaries. None of that is in the **1.0 ready claim** for this repository. Future releases may introduce a separate SDK plan; until then, treat out-of-tree editor DLLs as **unsupported** for compatibility guarantees.

Implementation evidence and session policy rows remain governed by `docs/specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md` plus archived `editor-productization` evidence in `docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md`; see also `docs/dependencies.md` (Editor native module boundary).

Host loaders and search-path policy for Windows remain defined by the platform SDK (for example [LoadLibraryExW](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw)); MIRAIKANAI does not redefine those APIs and does not promise that arbitrary third-party binaries loaded through them remain compatible across engine releases.
