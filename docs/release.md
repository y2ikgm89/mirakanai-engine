# Release Packaging

## SDK Layout

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1` builds the `release` preset, runs Release CTest, installs the SDK to `out/install/release`, validates installed SDK metadata, validates a clean installed consumer project, creates a CPack ZIP under `out/build/release`, and validates the current ZIP plus SHA-256 sidecar.

The installed SDK layout is:

- `bin/`: sample executables and runtime tools produced by enabled targets.
- `lib/`: static libraries and `cmake/Mirakanai/MirakanaiTargets.cmake`.
- `include/`: public `mirakana::` headers.
- `share/Mirakanai/manifest.json`: AI-facing engine manifest.
- `share/Mirakanai/schemas/`: installed JSON schemas for engine and game manifests.
- `share/Mirakanai/tools/`: PowerShell workflow scripts.
- `share/Mirakanai/examples/`: source examples, including `installed_consumer`.
- `share/Mirakanai/samples/`: sample game sources and `game.agent.json` files.
- `share/doc/Mirakanai/`: project documentation.

## Desktop Runtime Package

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` builds the `desktop-runtime-release` preset with `MK_ENABLE_DESKTOP_RUNTIME=ON`, the vcpkg `desktop-runtime` feature, and a selected registered desktop runtime game target. It keeps the default SDK package lean while producing a release artifact for an editor-independent SDL3 desktop runtime game path.

Registered desktop runtime game targets declare their `games/<game_name>/game.agent.json` path, required source-tree smoke args, package smoke args, shader-artifact requirements, target-specific shader artifact install paths, and optional runtime package files through CMake metadata generated as `desktop-runtime-games.json`. Package files should be authored once in `game.agent.json.runtimePackageFiles` and registered with `PACKAGE_FILES_FROM_MANIFEST`; use `tools/register-runtime-package-files.ps1 -GameManifest games/<game_name>/game.agent.json -RuntimePackageFile <game-relative-file>` to append reviewed manifest entries safely for existing games before schema/package validation. Literal CMake `PACKAGE_FILES` remains supported only when it exactly mirrors the manifest and is not mixed with manifest-derived mode. Static JSON contract checks require any `desktop-runtime-release` game manifest to match `MK_add_desktop_runtime_game`, `GAME_MANIFEST`, finite `SMOKE_ARGS`, safe `runtimePackageFiles`, and a package validation recipe. The default target remains `sample_desktop_runtime_shell`, with metadata-driven installed D3D12 DXIL and Vulkan SPIR-V shader-artifact smoke validation. Installed validation reads `selectedPackageTarget` from the installed metadata by default, verifies the selected installed sample manifest, validates selected-target package files plus declared D3D12 DXIL and explicitly requested Vulkan SPIR-V shader artifacts under `bin/`, and rejects explicit target mismatches so reused install prefixes cannot mask the selected package target. A non-shell proof target with cooked scene package files and host-owned D3D12 scene GPU binding validation by default, plus optional host-owned Vulkan scene GPU binding validation when `-RequireVulkanShaders` and explicit Vulkan smoke args are selected, can be packaged with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
```

A representative generated material/shader scaffold package can be validated the same way:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package
```

That generated target proves manifest-derived config and cooked scene package files, keeps `source/materials/*.material` and `shaders/*.hlsl` out of runtime package files, installs target-specific D3D12 scene/postprocess DXIL artifacts, and validates host-owned D3D12 scene GPU plus Frame Graph/Postprocess status on a ready Windows/DXC host.

The desktop runtime install tree adds:

- `bin/<selected-game-target>.exe`
- `bin/SDL3.dll` on Windows
- selected target shader artifacts such as `bin/shaders/runtime_shell.*.dxil` plus `bin/shaders/runtime_shell.*.spv` for the shell sample, `bin/shaders/sample_desktop_runtime_game_scene.*.dxil` for the cooked-scene game sample, `bin/shaders/sample_generated_desktop_runtime_material_shader_package_*.dxil` for the generated material/shader scaffold, and matching `.spv` artifacts only when a selected game package lane is run with `-RequireVulkanShaders`
- selected runtime package files derived from the selected target manifest or declared literally by the target, for example `bin/runtime/sample_desktop_runtime_game.config`
- `share/Mirakanai/desktop-runtime-games.json` describing registered desktop runtime game package metadata, manifest paths, target-specific shader artifact paths, and package file source/install paths

The package lane cleans `out/install/desktop-runtime-release` before installation, validates the installed SDK consumer, and then runs the selected installed executable with its smoke arguments. It does not require Dear ImGui or the `MK_editor` executable.

## Installed Consumer Validation

Run after installing the release build:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-sdk.ps1
```

The validation first runs `Assert-InstalledSdkMetadata` from `tools/installed-sdk-validation.ps1`. It checks the installed CMake package config/version files, parses `share/Mirakanai/manifest.json`, parses both installed JSON schemas, and requires non-empty installed workflow tools, the installed consumer example, at least one sample `game.agent.json`, docs README, third-party notices, and the proprietary license payload. It then configures `examples/installed_consumer` with only `Mirakanai_DIR` pointing at the installed package, so exported `mirakana::` CMake targets and public headers are still proven by a clean consumer build.

This is SDK payload validation only. It does not add package signing, upload, notarization, symbol-server publication, crash/telemetry backends, desktop-runtime selected-game smoke validation, Android device matrix evidence, or Apple-host validation.

## Package Metadata

CPack emits ZIP packages named with the engine version, host system, and host processor. `tools/package.ps1` runs `Assert-ReleasePackageArtifacts` from `tools/release-package-artifacts.ps1` after CPack generation. The validator derives the expected artifact basename from the current build's `CPackConfig.cmake`, verifies the matching `.zip.sha256` sidecar against the ZIP bytes, ignores unrelated stale ZIPs, and inspects the ZIP central directory without extraction for the installed CMake package files, SDK manifest/schemas, workflow tools, installed consumer example, docs README, third-party notices, and proprietary license payload.

This is desktop SDK artifact integrity validation only. It does not add signing, upload, notarization, symbol-server publication, crash/telemetry backends, desktop-runtime selected-game package proof, Android device matrix evidence, or Apple-host validation.
