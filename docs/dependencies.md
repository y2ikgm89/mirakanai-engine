# Dependencies

## Policy

Dependencies are introduced only through official source repositories or official package managers. The preferred C++ dependency path is vcpkg manifest mode because it keeps project dependencies isolated, records direct dependencies in `vcpkg.json`, and supports reproducible version resolution through a pinned `builtin-baseline`.

`vcpkg.json` is pinned to the official Microsoft vcpkg registry commit:

```text
3909e67a639d426ea939d9bff77bfe1d10443476
```

Update the baseline only as an explicit dependency-maintenance task: update the official `external/vcpkg` checkout, verify the selected port versions and licenses, update this document and `THIRD_PARTY_NOTICES.md`, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Current Runtime Dependencies

None are required for the default headless build.

On Windows, the default validation build uses Windows SDK system libraries for the Win32 platform adapter, WASAPI audio adapter, D3D12 backend, DirectWrite runtime UI text/font adapter, Windows TSF runtime UI IME session adapter, Windows UI Automation runtime UI accessibility adapter, DirectWrite editor text/font adapter, and tests:

- `ole32`
- `oleaut32`
- `shell32`
- `user32`
- `d3d12`
- `dxgi`
- `d3dcompiler`
- `dwrite`
- `uiautomationcore`

These are official platform SDK libraries and are not bundled in the repository.

The optional desktop runtime, asset importer, Win32 DirectStorage adapter, native physics middleware adapter, and network transport adapter lanes use vcpkg manifest features so optional package dependencies remain isolated from the default build and from system-wide package locations. The current `desktop-runtime` feature is dependency-free and uses host SDK libraries. The native `desktop-editor` lane is also dependency-free: it is a CMake preset/tooling lane for the first-party Win32/D3D12/DirectWrite editor shell, not a vcpkg feature and not a UI middleware dependency path.

Run the optional vcpkg dependency bootstrap with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
```

The repository PowerShell wrappers configure vcpkg through official process environment variables before optional dependency steps. `VCPKG_DOWNLOADS` points at `out/vcpkg/downloads`, `VCPKG_DEFAULT_BINARY_CACHE` points at `out/vcpkg/binary-cache`, and `VCPKG_BINARY_SOURCES` is set to a file provider over that binary cache. This keeps optional dependency downloads and binary packages out of user-global locations and avoids relying on sandbox-inherited cache state.

`bootstrap-deps` is the only wrapper that runs `vcpkg install`. By default or with `-All`, it installs the `desktop-runtime`, `asset-importers`, `physics-jolt`, `network-enet`, and `directstorage-win32` manifest features into the repository root `vcpkg_installed` tree; `-Feature <name>` installs an explicit subset. The `asset-importers` feature owns the optional PNG, glTF, OpenEXR, KTX2/Basis, and common-audio package set. Optional CMake presets set `VCPKG_MANIFEST_INSTALL=OFF` and `VCPKG_INSTALLED_DIR=${sourceDir}/vcpkg_installed`, so CMake configure consumes the already-bootstrapped manifest install tree instead of downloading, extracting tools, or running vcpkg during configure.

GitHub Actions restores the gitignored `external/vcpkg` tool checkout before calling `bootstrap-deps`, then checks out the `vcpkg.json` `builtin-baseline` commit. Local hosts must still provide or restore `external/vcpkg` before running optional vcpkg-backed lanes.

On restricted sandboxed hosts, `bootstrap-deps` can still require an unrestricted run because it is the step that intentionally launches vcpkg, downloads archives, extracts helper tools, and builds dependency ports. After it succeeds, normal configure/build/package lanes should not invoke vcpkg.

The visible desktop editor shell is built through the dependency-free native Win32 + first-party retained UI + Direct3D 12 `MK_editor` target. The wrapper below is the supported entrypoint for configuring, building, and testing that lane:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
```

`build-editor` uses the `desktop-editor` CMake preset with `MK_ENABLE_DESKTOP_EDITOR=ON` and no vcpkg toolchain. `MK_editor_core` remains covered by the default validation lane.

Validate or package the editor-independent desktop game runtime shell with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
```

`desktop-game-runtime` uses the `desktop-runtime` preset and the dependency-free `desktop-runtime` vcpkg feature so it does not require Dear ImGui or external desktop packages. `package-desktop-runtime` uses the `desktop-runtime-release` preset, cleans its desktop runtime install prefix, installs the selected registered desktop runtime game executable, validates the installed executable, rejects legacy desktop runtime DLLs in the install tree or CPack ZIP, and then creates a CPack ZIP. Registered desktop runtime game targets generate `desktop-runtime-games.json` metadata for their `games/<game_name>/game.agent.json` manifest path, required source-tree smoke args, installed package smoke args, selected package target, target-specific shader-artifact paths, shader-artifact requirements, and runtime package files. Package files should be authored once in `game.agent.json.runtimePackageFiles` and registered through `PACKAGE_FILES_FROM_MANIFEST`; literal CMake `PACKAGE_FILES` remains valid only when it intentionally mirrors the manifest. Static schema checks and focused/package validation confirm that `desktop-runtime-release` manifest claims match CMake registration, `GAME_MANIFEST`, safe `runtimePackageFiles`, and package recipes. The default package target remains `sample_desktop_runtime_shell`, uses the Windows-native `Win32DesktopGameHost`, and requires generated DXIL plus SPIR-V shader artifacts through metadata while validating D3D12 presentation on the installed smoke. `sample_desktop_runtime_game`, committed generated desktop runtime package samples, and generated desktop runtime templates are Win32-backed through `Win32DesktopGameHost` / `Win32DesktopPresentation`. The same selected game target can validate target-specific scene SPIR-V artifacts and host-owned Vulkan scene GPU binding with `-RequireVulkanShaders` plus explicit Vulkan smoke args on a ready Windows/Vulkan host. `tools/package-linux-runtime.ps1` is a separate Linux-only host gate that uses the `desktop-runtime-linux-release` Ninja preset, `VCPKG_TARGET_TRIPLET=x64-linux`, host-aware `tools/bootstrap-deps.ps1` vcpkg resolution, `out/build/desktop-runtime-linux-release`, `out/install/linux-runtime-release`, and `tools/validate-installed-linux-runtime.ps1`; it is prerequisite package-smoke infrastructure only and must not be treated as Linux Vulkan readiness until Linux presentation/readback/clean validation-log counters pass on a Linux host.

## Windows Host Diagnostics Tooling

The recommended Windows diagnostics stack uses official Microsoft tools and is host-local. These tools are not repository dependencies, are not redistributed, and do not require entries in `THIRD_PARTY_NOTICES.md` unless their binaries or outputs are bundled later.

- Debugging Tools for Windows from the Windows SDK `OptionId.WindowsDesktopDebuggers`: `cdb.exe`, `windbg.exe`, `kd.exe`, and `ntsd.exe` for native crash, dump, and debugger-path work. Configure symbols with `_NT_SYMBOL_PATH=srv*C:\Symbols*https://msdl.microsoft.com/download/symbols`.
- Windows Graphics Tools optional capability `Tools.Graphics.DirectX~~~~0.0.1.0`: installs the D3D12 debug layer files such as `C:\Windows\System32\d3d12SDKLayers.dll` and `C:\Windows\SysWOW64\d3d12SDKLayers.dll`.
- PIX on Windows: `WinPix.exe` and `pixtool.exe` for D3D12 GPU captures, timing, and counter investigations. Visual Studio Graphics Diagnostics is useful but is not the primary D3D12 PIX replacement for this project.
- Windows Performance Toolkit from the Windows ADK `OptionId.WindowsPerformanceToolkit`: `wpr.exe`, `wpa.exe`, and `xperf.exe` for ETW tracing and CPU/system performance investigations.

Apply ADK servicing patches only when they match installed ADK features. Do not force unrelated `.msp` packages for uninstalled ADK components, and do not move host diagnostics installation into CMake configure or repository bootstrap scripts.

## Optional Features

`desktop-runtime` in `vcpkg.json` is currently a dependency-free feature. It enables the Windows-native platform/audio/runtime host lane through `MK_ENABLE_DESKTOP_RUNTIME=ON` while relying on host SDK libraries such as Win32, WASAPI, DXGI, and D3D12. It is intentionally separate from `desktop-editor` so windowed games can be validated and packaged without `MK_editor`.

`desktop-editor` is not a `vcpkg.json` feature. It is a dependency-free CMake preset that enables the Windows-native first-party `MK_editor` shell through `MK_ENABLE_DESKTOP_EDITOR=ON` while relying on host SDK libraries and first-party `mirakana::ui` / `MK_ui_renderer` contracts. `MK_editor_core` remains the supported editor logic target, and the native visible editor shell must keep Win32, D3D12, DXGI, DirectWrite, COM, and native handles in private editor implementation files rather than public engine, gameplay, runtime UI, or editor-core APIs.

### Editor native module boundary (not a vcpkg dependency)

Optional **same-build** loading of an editor game module driver uses the Windows (or platform) dynamic loader against a **caller-selected path** for a module built with the same MIRAIKANAI Engine sources. That surface is **not** introduced through `vcpkg.json` and does **not** create a third-party package record. **Stable binary ABI** guarantees for out-of-tree vendor DLLs are **explicitly out of scope for Engine 1.0**; see `docs/legal-and-licensing.md` (Editor game module driver and third-party DLL ABI).

`directstorage-win32` in `vcpkg.json` declares:

- `dstorage` / Microsoft DirectStorage SDK 1.3.0 for the optional Windows-only first-party MAVG byte-range IO adapter

This feature is installed only by `tools/bootstrap-deps.ps1 -Feature directstorage-win32` or `tools/bootstrap-deps.ps1 -All`. The adapter must keep `dstorage.h`, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `DStorageGetFactory`, DirectStorage DLL/import-library linkage, and Microsoft DirectStorage SDK license files behind the private adapter boundary in `MK_platform_win32`. Public runtime/gameplay APIs must continue to expose only first-party `IByteRangeIoExecutor`, `ByteRangeIoBackendKind::direct_storage`, and value diagnostics; DirectStorage SDK types, `IDStorage*`, `IUnknown*`, D3D12/DXGI resources, `HANDLE`, `void*`, or native queue/status/file objects must not leak into public gameplay/runtime APIs. This is a Windows-only dependency lane and does not claim GPU destinations, GDeflate, DirectStorage 1.4 preview APIs, async-overlap performance proof, mesh shaders, Metal readiness, Nanite equivalence, or broad MAVG backend readiness.

`asset-importers` in `vcpkg.json` declares reviewed dependencies for optional production source importer adapters:

- `libspng` for PNG decoding
- `fastgltf` for glTF 2.0 parsing
- `openexr` for OpenEXR HDR scene-linear source review and the future EXR importer adapter
- `ktx` / KTX Software for KTX2/Basis texture container review and offline transcode-target planning
- `miniaudio` for WAV/MP3/FLAC audio decoding

This feature is installed only by `tools/bootstrap-deps.ps1`; current optional adapter targets find or link external packages only when `MK_ENABLE_ASSET_IMPORTERS=ON` through the `asset-importers` CMake preset. `MK_tools` owns the optional adapters and converts source files into first-party source documents before cooked artifacts are written. Audited PNG bytes decode to RGBA8 `TextureSourceDocument` through `mirakana::decode_audited_png_rgba8` (`mirakana/tools/source_image_decode.hpp`), shared with `PngTextureExternalAssetImporter` and the optional `PngImageDecodingAdapter` bridge for `mirakana::ui::IImageDecodingAdapter`. OpenEXR remains an optional dependency/legal/bootstrap gate for HDR scene-linear source import; `MK_ENABLE_ASSET_IMPORTERS=ON` requires `find_package(OpenEXR CONFIG REQUIRED)` so missing bootstrap fails at optional configure with an explicit missing-package blocker. The optional `MK_tools` asset-importers lane reviews OpenEXR metadata, decodes selected scanline pixels through official OpenEXR APIs, cooks deterministic first-party payload rows, and participates in `environment_asset_pipeline_openexr_ktx_basis_ready`; default builds and runtime/game code still do not parse EXR source files, expose OpenEXR headers or types through public APIs, link OpenEXR from installed Mirakanai package config targets, or run OpenEXR tools. KTX Software remains optional for reviewed KTX2/Basis container validation, offline transcode-target planning, selected RGBA8 payload transcode, and backend-target BC7/ASTC package decisions; runtime/game code still does not parse KTX source files, execute Basis transcode at runtime, or expose KTX/native handles. The dependency-free packed UI atlas bridge (`author_packed_ui_atlas_from_decoded_images`, `plan_packed_ui_atlas_package_update`) consumes already validated `ImageDecodeResult` rows and emits first-party `GameEngine.CookedTexture.v1` plus `GameEngine.UiAtlas.v1` package artifacts without adding a new dependency. The dependency-free glyph atlas bridge (`author_packed_ui_glyph_atlas_from_rasterized_glyphs`, `plan_packed_ui_glyph_atlas_package_update`, `UiAtlasMetadataGlyph`, `RuntimeUiAtlasGlyph`) consumes already-rasterized RGBA8 glyph pixels and emits first-party cooked glyph atlas package metadata without adding a font, shaping, rasterization, platform SDK, or renderer upload dependency. Runtime/game code must consume cooked assets and must not parse external source formats directly.

Installed SDKs built with `MK_ENABLE_ASSET_IMPORTERS=ON` advertise `Mirakanai_HAS_ASSET_IMPORTERS` in `MirakanaiConfig.cmake` and resolve `SPNG`, `fastgltf`, and `Ktx` before loading exported Mirakanai targets. `miniaudio` remains a private header-only implementation dependency of the optional adapter build.

Validate the optional importer lane with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
```

`physics-jolt` in `vcpkg.json` declares:

- `joltphysics` with `default-features` disabled

This feature builds the optional `MK_physics_jolt` adapter when `MK_ENABLE_PHYSICS_JOLT=ON`. The public facade stays in `MK_physics`; Jolt headers, `JPH::*` symbols, and native handles remain private to `engine/physics/jolt`. Installed SDKs built with this option advertise `Mirakanai_HAS_PHYSICS_JOLT` in `MirakanaiConfig.cmake` and resolve `Jolt CONFIG` before loading exported Mirakanai targets. The adapter currently supports collision-enabled, non-trigger authored 3D solid scenes with Jolt mask-based filters, fails closed for single-backend-body scenes, disabled-collision bodies, triggers, unsupported Jolt filter bits, and Jolt update capacity errors, and sizes body-pair/contact budgets from enabled scene density. The stock vcpkg Jolt port is treated as a native middleware lane only; it does not establish cross-platform deterministic parity for the default/generated-game ready surface.

Validate the optional Jolt lane with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-physics-jolt.ps1
```

`network-enet` in `vcpkg.json` declares:

- `enet` with `default-features` disabled

This feature builds the optional `MK_runtime_network_enet` adapter when `MK_ENABLE_NETWORK_ENET=ON`. The public facade stays in `MK_runtime`; ENet headers, `ENet*` symbols, socket handles, and native transport handles remain private to `engine/runtime/network/enet`. Installed SDKs built with this option advertise `Mirakanai_HAS_NETWORK_ENET` in `MirakanaiConfig.cmake` and resolve `unofficial-enet CONFIG` before loading exported Mirakanai targets. On Windows, `MK_runtime_network_enet` also links the Windows SDK `winmm` and `ws2_32` libraries required by ENet's timer and Winsock implementation. The adapter currently supports one local loopback client peer with reliable and unreliable packet rows through the first-party `IRuntimeNetworkTransportAdapter` facade. It is not external network readiness, encryption/authentication, matchmaking, NAT traversal, replication, rollback, or broad multiplayer readiness.

Validate the optional ENet lane with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-network-enet.ps1
```

## Mobile Packaging Tooling

Mobile packaging templates are present but toolchain-gated. They are not used by the default headless build.

`platform/android` declares an Android GameActivity package template using:

- Android Gradle Plugin 9.1.0
- Android SDK Platform 36.1 and Build Tools 36.0.0 from the official Android SDK manager
- Android Emulator 36.5.11, API 36 Google APIs x86_64 system image, and a local `Mirakanai_API36` AVD from the official Android SDK manager for local smoke
- Android NDK 28.2.13676358 from the official Android SDK manager
- CMake 4.1.2 from the official Android SDK manager
- Android NDK Vulkan loader and AAudio platform libraries
- JDK 17
- Gradle 9.3.1
- `androidx.appcompat:appcompat:1.7.1`
- `androidx.core:core:1.18.0`
- `androidx.games:games-activity:3.0.4`

`platform/ios` declares a CMake/Xcode bundle template for Apple hosts. Full Xcode, UIKit, Foundation, Metal, QuartzCore, iPhoneOS/iPhone Simulator SDKs, iOS Simulator runtimes, signing identities, and Apple SDK assets are host toolchain requirements and are not redistributed by this repository.

Run mobile diagnostics with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mobile-packaging.ps1
```

Android and Apple package builds intentionally fail with explicit blockers when local SDKs are missing:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-android.ps1 -Game sample_headless -Configuration Debug
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-mobile-apple.ps1 -Game sample_headless -Configuration Debug
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/smoke-ios-package.ps1 -Game sample_headless -Configuration Debug
```

UI dependency policy:

- `MK_ui` and `MK_editor_ui` public contracts must stay first-party.
- Low-level text shaping, font rasterization, IME, accessibility bridge, image decoding, and platform integration code may use official SDKs or audited libraries only behind adapters.
- First-party runtime UI clean-room source gate: `docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md` and `tools/check-first-party-ui-clean-room.ps1` add provenance and public-token enforcement without adding a dependency. Official Unity, Unreal Engine, Godot, Microsoft, Apple, HarfBuzz, FreeType, AT-SPI2, Vulkan, and W3C documentation references are category or adapter-boundary research only; they are not repository dependencies and do not authorize copying source, samples, serialized UI formats, themes, fonts, icons, screenshots, API shapes, or public UI system names.
- HarfBuzz, FreeType, ICU, or similar text/font libraries are future dependency-gated adapter candidates only; selecting any of them requires `license-audit`, `vcpkg.json`, this document, and `THIRD_PARTY_NOTICES.md` updates before use.
- Qt, NoesisGUI, Slint, RmlUi, fonts, text/font libraries, or other UI middleware must be evaluated only after `license-audit`, `vcpkg.json`, this document, and `THIRD_PARTY_NOTICES.md` are updated.
- No UI dependency may expose its headers, object model, native handles, or license obligations through public game APIs without an accepted architecture decision.

Validated local package versions:

| Package | Version | Use |
| --- | --- | --- |
| libspng | vcpkg baseline selected | Optional `MK_tools` PNG source importer |
| fastgltf | vcpkg baseline selected | Optional `MK_tools` glTF 2.0 source importer |
| OpenEXR | 3.4.10 | Optional `asset-importers` dependency/legal/bootstrap gate for future HDR scene-linear EXR source review |
| Imath | 3.2.2 | Optional `asset-importers` build output through OpenEXR |
| libdeflate | 1.25 | Optional `asset-importers` build output through OpenEXR |
| OpenJPH | 0.27.0 | Optional `asset-importers` build output through OpenEXR |
| KTX Software | 4.4.2 | Optional `MK_tools` KTX2/Basis texture review dependency/legal evidence |
| Zstandard | 1.5.7 | Optional `MK_tools` build output through KTX Software |
| OpenGL Registry | 2026-01-26 | Optional `MK_tools` build output through KTX Software |
| EGL Registry | 2025-05-27 | Optional `MK_tools` build output through KTX Software |
| miniaudio | vcpkg baseline selected | Optional `MK_tools` WAV/MP3/FLAC source importer |
| Microsoft DirectStorage SDK / dstorage | 1.3.0 | Optional Windows-only `MK_platform_win32` MAVG byte-range IO adapter |
| Jolt Physics | 5.5.0 | Optional `MK_physics_jolt` native physics middleware adapter |
| ENet | 1.3.18 | Optional `MK_runtime_network_enet` loopback network transport adapter |
| Android Gradle Plugin | 9.1.0 | Toolchain-gated Android package template |
| Android NDK platform libraries | 28.2.13676358 | Toolchain-gated Android Vulkan surface and AAudio output adapters |
| Android Emulator | 36.5.11 | Local Android package install/launch smoke |
| AndroidX AppCompat | 1.7.1 | Toolchain-gated Android GameActivity package template |
| AndroidX Core | 1.18.0 | Toolchain-gated Android package template |
| AndroidX Games Activity | 3.0.4 | Toolchain-gated Android GameActivity package template |
| vcpkg-cmake | 2024-04-23 | vcpkg CMake helper |
| vcpkg-cmake-config | 2024-05-23 | vcpkg CMake config helper |

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` includes `tools/check-dependency-policy.ps1`, which verifies that the default build has no third-party dependencies, optional `desktop-runtime`, `asset-importers`, `directstorage-win32`, `physics-jolt`, and `network-enet` features keep their dependency shapes, OpenEXR/KTX/Basis/DirectStorage dependency/legal records remain present, the removed `desktop-gui` / `imgui` dependency path stays absent, `builtin-baseline` is present, notices exist, optional CMake presets disable configure-time vcpkg manifest install and use the root install tree, `bootstrap-deps` installs all optional feature dependency sets, and `tools/validate-physics-jolt.ps1` / `tools/validate-network-enet.ps1` remain the dedicated optional adapter build/test/install wrappers.

## Official References

- vcpkg manifest mode: https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode
- vcpkg CMake integration: https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration
- Microsoft DirectStorage SDK & API: https://devblogs.microsoft.com/directx/directstorage-api-downloads/
- Microsoft DirectStorage API reference: https://learn.microsoft.com/en-us/windows/win32/dstorage/
- Microsoft.Direct3D.DirectStorage 1.3.0: https://www.nuget.org/packages/Microsoft.Direct3D.DirectStorage/1.3.0
- DirectStorage developer guidance: https://github.com/microsoft/DirectStorage/blob/main/Docs/DeveloperGuidance.md
- Windows Core Audio / WASAPI: https://learn.microsoft.com/en-us/windows/win32/coreaudio/wasapi
- DirectWrite: https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal
- Text Rendering with Direct2D and DirectWrite: https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-and-directwrite
- libspng: https://libspng.org/
- fastgltf: https://github.com/spnda/fastgltf
- OpenEXR: https://www.openexr.com/
- OpenEXR technical introduction: https://openexr.com/en/latest/TechnicalIntroduction.html
- OpenEXR scene linear: https://openexr.com/en/latest/SceneLinear.html
- Imath: https://github.com/AcademySoftwareFoundation/Imath
- libdeflate: https://github.com/ebiggers/libdeflate
- OpenJPH: https://github.com/aous72/OpenJPH
- KTX Software: https://github.com/KhronosGroup/KTX-Software
- miniaudio: https://miniaud.io/
- Jolt Physics: https://github.com/jrouwe/JoltPhysics
- Jolt Physics documentation: https://jrouwe.github.io/JoltPhysics/
- ENet: https://github.com/lsalzman/enet
- ENet tutorial: https://github.com/lsalzman/enet/blob/master/docs/tutorial.dox
- Android Gradle Plugin: https://developer.android.com/build/releases/gradle-plugin
- GameActivity: https://developer.android.com/games/agdk/game-activity/get-started
- Android NDK AAudio: https://developer.android.com/ndk/guides/audio/aaudio/aaudio
- Android NDK audio latency: https://developer.android.com/ndk/guides/audio/audio-latency
- Android Vulkan: https://developer.android.com/ndk/guides/graphics
- Android app signing: https://developer.android.com/studio/publish/app-signing
- Android Emulator command line: https://developer.android.com/studio/run/emulator-commandline
- Android Debug Bridge: https://developer.android.com/studio/command-line/adb
- AndroidX AppCompat releases: https://developer.android.com/jetpack/androidx/releases/appcompat
- AndroidX Core releases: https://developer.android.com/jetpack/androidx/releases/core
- AndroidX Games releases: https://developer.android.com/jetpack/androidx/releases/games
- Apple Metal: https://developer.apple.com/metal/
- Apple Xcode command-line tools: https://developer.apple.com/documentation/xcode/xcode-command-line-tool-reference
- Apple Xcode Simulator runtimes: https://developer.apple.com/documentation/xcode/installing-additional-simulator-runtimes
- GitHub-hosted macOS runners: https://docs.github.com/en/actions/reference/runners/github-hosted-runners
- Debugging Tools for Windows: https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/
- Microsoft public symbol server: https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/microsoft-public-symbols
- Direct3D 12 programming environment and debug layer: https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-environment-set-up
- PIX on Windows download: https://devblogs.microsoft.com/pix/download/
- Windows Performance Toolkit: https://learn.microsoft.com/en-us/windows-hardware/test/wpt/
- Download and install the Windows ADK: https://learn.microsoft.com/en-us/windows-hardware/get-started/adk-install
- Windows ADK servicing patches: https://learn.microsoft.com/en-us/windows-hardware/get-started/adk-servicing

## License Notes

- libspng is BSD-2-Clause.
- fastgltf is MIT and currently pulls `simdjson` through vcpkg.
- OpenEXR is BSD-3-Clause and is kept behind the optional `asset-importers` lane as a dependency/legal/bootstrap gate for HDR scene-linear source import. Its vcpkg `tools` feature is not selected by the repository manifest; selected EXR metadata review, scanline pixel decode, cooked first-party payload rows, and `environment_asset_pipeline_openexr_ktx_basis_ready` are owned by `MK_tools` while OpenEXR tool execution, runtime source parsing, and public OpenEXR types remain unsupported.
- Imath is BSD-3-Clause and is pulled through OpenEXR.
- libdeflate is MIT and is pulled through OpenEXR for DEFLATE-related support.
- OpenJPH is BSD-2-Clause and is pulled through OpenEXR; its vcpkg `tools` feature is not selected by the repository manifest.
- KTX Software is primarily Apache-2.0 for repository-unique files and ships an upstream `LICENSES/*` bundle including a non-open Ericsson `LicenseRef-ETCSLA` special case recorded by its copyright file; it is kept behind the optional `asset-importers` lane.
- Zstandard is BSD-3-Clause OR GPL-2.0-only and is pulled through KTX Software.
- Khronos OpenGL/EGL registry files use per-file license comments and are pulled through KTX Software.
- miniaudio is public domain or MIT No Attribution.
- Microsoft DirectStorage SDK 1.3.0 is distributed through the `dstorage` vcpkg port from the official `Microsoft.Direct3D.DirectStorage` NuGet package. Package headers are MIT licensed through `LICENSE-CODE.txt`; redistributed binaries are governed by `LICENSE.txt` (`LicenseRef-Microsoft-DirectStorage-SDK`). Keep both license files with redistributed package outputs and keep SDK objects private to the Win32 adapter.
- The `desktop-editor` developer/editor shell lane is first-party and dependency-free; any future UI, font, text, or platform adapter dependency must be added through `license-audit`, `vcpkg.json`, this document, and `THIRD_PARTY_NOTICES.md` before use.
- Jolt Physics is MIT licensed and isolated to the optional `physics-jolt` adapter lane.
- ENet is MIT licensed and isolated to the optional `network-enet` adapter lane.
- Android Gradle Plugin, AndroidX AppCompat, AndroidX Core, and AndroidX Games Activity are Apache-2.0 licensed Android toolchain/template dependencies and are not part of the default build.
- Apple SDK frameworks are official platform SDK dependencies and are not redistributed by this repository.
- Debugging Tools for Windows, Windows Graphics Tools, PIX on Windows, and Windows Performance Toolkit are official Microsoft host diagnostics tools and are not redistributed by this repository.
- Any bundled source, binary, font, icon, texture, shader, or generated asset still requires a record in `THIRD_PARTY_NOTICES.md`.
