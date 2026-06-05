# Third-Party Notices

No third-party source code, binary libraries, fonts, images, audio, models, shaders, or other distributable assets are included in the default headless build.

Optional asset-importer, DirectStorage SDK, native physics adapter, and network adapter dependencies are declared in `vcpkg.json`, installed through vcpkg manifest mode, and excluded from source control. The `desktop-runtime` feature and dependency-free `desktop-editor` lane use first-party or host SDK paths, including Windows SDK Win32/DXGI/D3D12/DirectWrite libraries for the native editor, and declare no package dependencies. Optional dependencies are used only by explicit optional build lanes such as `tools/bootstrap-deps.ps1`, `tools/build-asset-importers.ps1`, `tools/validate-directstorage-sdk.ps1`, `tools/validate-physics-jolt.ps1`, and `tools/validate-network-enet.ps1`.

| Name | Source | Retrieved | Version | Copyright holder | License | Modified | Distribution target |
| --- | --- | --- | --- | --- | --- | --- | --- |
| libspng | https://github.com/randy408/libspng | 2026-04-27 | 0.7.4 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | libspng contributors | BSD-2-Clause | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output |
| zlib | https://www.zlib.net/ | 2026-04-27 | 1.3.2 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Jean-loup Gailly and Mark Adler | Zlib | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output through libspng |
| fastgltf | https://github.com/spnda/fastgltf | 2026-04-27 | 0.9.0 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | fastgltf contributors | MIT | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output |
| simdjson | https://github.com/simdjson/simdjson | 2026-04-27 | 4.6.3 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | simdjson contributors | Apache-2.0 OR MIT | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output through fastgltf |
| KTX Software | https://github.com/KhronosGroup/KTX-Software | 2026-05-28 | 4.4.2 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Mark Callow, Khronos Group, and KTX Software contributors | Apache-2.0 with bundled upstream `LICENSES/*` records including `LicenseRef-ETCSLA` special case | No first-party modifications; vcpkg applies packaging patches | Optional `asset-importers` `mirakana_tools` KTX2/Basis texture review build output |
| Zstandard | https://github.com/facebook/zstd | 2026-05-28 | 1.5.7 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Meta Platforms, Inc. and affiliates | BSD-3-Clause OR GPL-2.0-only | No first-party modifications; vcpkg applies packaging patches | Optional `asset-importers` `mirakana_tools` build output through KTX Software |
| OpenGL Registry | https://github.com/KhronosGroup/OpenGL-Registry | 2026-05-28 | commit `0b449b97cdf1043eef5e1f0e235cbbab6ec10c86` via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Khronos Group and registry contributors | LicenseRef-Khronos-Registry-Per-File | No first-party modifications | Optional `asset-importers` `mirakana_tools` build output through KTX Software |
| EGL Registry | https://github.com/KhronosGroup/EGL-Registry | 2026-05-28 | commit `3ae2b7c48690d2ce13cc6db3db02dfc0572be65e` via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Khronos Group and registry contributors | LicenseRef-Khronos-Registry-Per-File | No first-party modifications | Optional `asset-importers` `mirakana_tools` build output through KTX Software |
| miniaudio | https://github.com/mackron/miniaudio | 2026-04-27 | 0.11.25 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | David Reid and miniaudio contributors | Unlicense OR MIT-0 | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output |
| Microsoft DirectStorage SDK | https://www.nuget.org/packages/Microsoft.Direct3D.DirectStorage/1.3.0 | 2026-06-06 | 1.3.0 via vcpkg `dstorage` port at baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Microsoft Corporation | `LicenseRef-Microsoft-DirectStorage-SDK` for `native/bin`; MIT for `native/include` headers through `LICENSE-CODE.txt` | No first-party modifications | Optional `directstorage-sdk` compile/link/package-copy smoke build output |
| Jolt Physics | https://github.com/jrouwe/JoltPhysics | 2026-05-24 | 5.5.0 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Jorrit Rouwe and contributors | MIT | No first-party modifications | Optional `physics-jolt` `mirakana_physics_jolt` adapter build output |
| ENet | https://github.com/lsalzman/enet | 2026-05-24 | 1.3.18 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Lee Salzman | MIT | No first-party modifications | Optional `network-enet` `mirakana_runtime_network_enet` adapter build output |
| Android Gradle Plugin | https://developer.android.com/build/releases/gradle-plugin | 2026-04-27 | 9.1.0 from Google Maven through the Android package template | Google LLC and Android Open Source Project contributors | Apache-2.0 | No first-party modifications | Toolchain-gated Android package build only |
| AndroidX AppCompat | https://developer.android.com/jetpack/androidx/releases/appcompat | 2026-04-27 | 1.7.1 from Google Maven through the Android package template | Android Open Source Project contributors | Apache-2.0 | No first-party modifications | Toolchain-gated Android GameActivity package build only |
| AndroidX Core | https://developer.android.com/jetpack/androidx/releases/core | 2026-04-27 | 1.18.0 from Google Maven through the Android package template | Android Open Source Project contributors | Apache-2.0 | No first-party modifications | Toolchain-gated Android package build only |
| AndroidX Games Activity | https://developer.android.com/jetpack/androidx/releases/games | 2026-04-27 | 3.0.4 from Google Maven through the Android package template | Google LLC and Android Open Source Project contributors | Apache-2.0 | No first-party modifications | Toolchain-gated Android GameActivity package build only |

Before adding any external material, record:

- name
- source URL
- retrieved date
- version or commit
- author or copyright holder
- SPDX license identifier or custom license reference
- modification status
- distribution target
