# Third-Party Notices

No third-party source code, binary libraries, fonts, images, audio, models, shaders, or other distributable assets are included in the default headless build.

Optional asset-importer, native physics adapter, and network adapter dependencies are declared in `vcpkg.json`, installed through vcpkg manifest mode, and excluded from source control. The `desktop-runtime` and `desktop-gui` features currently use first-party or host SDK paths and declare no package dependencies. Optional dependencies are used only by explicit optional build lanes such as `tools/build-asset-importers.ps1`, `tools/validate-physics-jolt.ps1`, and `tools/validate-network-enet.ps1`.

| Name | Source | Retrieved | Version | Copyright holder | License | Modified | Distribution target |
| --- | --- | --- | --- | --- | --- | --- | --- |
| libspng | https://github.com/randy408/libspng | 2026-04-27 | 0.7.4 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | libspng contributors | BSD-2-Clause | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output |
| zlib | https://www.zlib.net/ | 2026-04-27 | 1.3.2 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | Jean-loup Gailly and Mark Adler | Zlib | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output through libspng |
| fastgltf | https://github.com/spnda/fastgltf | 2026-04-27 | 0.9.0 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | fastgltf contributors | MIT | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output |
| simdjson | https://github.com/simdjson/simdjson | 2026-04-27 | 4.6.3 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | simdjson contributors | Apache-2.0 OR MIT | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output through fastgltf |
| miniaudio | https://github.com/mackron/miniaudio | 2026-04-27 | 0.11.25 via vcpkg baseline `3909e67a639d426ea939d9bff77bfe1d10443476` | David Reid and miniaudio contributors | Unlicense OR MIT-0 | No first-party modifications | Optional `asset-importers` `mirakana_tools` adapter build output |
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
