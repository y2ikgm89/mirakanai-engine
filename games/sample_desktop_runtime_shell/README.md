# Sample Desktop Runtime Shell

This sample proves the first editor-independent desktop game runtime shell.

- Implements `mirakana::GameApp`.
- Runs through reusable `mirakana::Win32DesktopGameHost`, which owns Win32 runtime/window setup, event pumping, renderer presentation selection, virtual input/lifecycle state, and `mirakana::DesktopGameRunner` service wiring.
- Selects its renderer through host-owned `mirakana::Win32DesktopPresentation`, which keeps native surface probing, optional D3D12 renderer creation, backend-neutral presentation report rows, and fallback diagnostics in host code.
- On Windows desktop runtime builds, CMake compiles `shaders/runtime_shell.hlsl` into build-output D3D12 DXIL and Vulkan SPIR-V when `dxc` with SPIR-V CodeGen is available. The sample loads those packaged artifacts with `mirakana::load_desktop_shader_bytecode_pair`; D3D12 can opt into `RhiFrameRenderer` without runtime shader compilation, while Vulkan artifacts remain package validation inputs until non-SDL Vulkan presentation is designed.
- If shader artifacts, a native surface, or D3D12 are unavailable, the sample reports deterministic presentation report fields plus diagnostics and keeps the `mirakana::NullRenderer` fallback unless a `--require-*` flag requests failure.
- Supports `--smoke` for a finite Win32 run that can be registered in CTest.
- Does not include SDL3, Dear ImGui, editor, OS, or GPU native handles in game code.

Build with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
```

For interactive use, run the `sample_desktop_runtime_shell` executable from the `desktop-runtime` build output and close the window or press Escape to stop. For a finite local smoke run, use:

```powershell
out\build\desktop-runtime\games\Debug\sample_desktop_runtime_shell.exe --smoke
```

To require the packaged shader artifacts without requiring a real D3D12 surface, use:

```powershell
out\build\desktop-runtime\games\Debug\sample_desktop_runtime_shell.exe --smoke --require-d3d12-shaders
```

To require the packaged Vulkan SPIR-V artifacts without requiring a real Vulkan surface, use:

```powershell
out\build\desktop-runtime\games\Debug\sample_desktop_runtime_shell.exe --smoke --require-vulkan-shaders
```

On a Windows desktop with D3D12 support, this finite smoke path requires the native renderer:

```powershell
out\build\desktop-runtime\games\Debug\sample_desktop_runtime_shell.exe --smoke --require-d3d12-shaders --require-d3d12-renderer
```

Package the Release desktop runtime shell with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
```

The installed executable is validated from `out\install\desktop-runtime-release\bin` with metadata-driven `--smoke --require-d3d12-shaders --require-vulkan-shaders`; installed validation requires the selected sample status line before accepting `presentation_selected` and `presentation_backend_reports`, and fails if `SDL3.dll` is present. On Windows the install tree includes `bin\shaders\runtime_shell.*.dxil`, `bin\shaders\runtime_shell.*.spv`, and `share\GameEngine\desktop-runtime-games.json`. Other registered desktop runtime game targets can be selected with `tools/package-desktop-runtime.ps1 -GameTarget <target>` when they declare source-tree and package smoke metadata.
