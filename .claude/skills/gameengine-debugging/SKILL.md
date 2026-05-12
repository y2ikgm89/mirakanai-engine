---
name: gameengine-debugging
description: Debugs C++ engine crashes, incorrect runtime behavior, ownership and lifetime bugs, undefined behavior, and test failures. Use when investigating engine or unit test code under engine/ or tests/.
paths:
  - "engine/**/*.cpp"
  - "engine/**/*.hpp"
  - "tests/**/*.cpp"
  - "tools/check-tidy.ps1"
  - "tools/check-toolchain.ps1"
---

# GameEngine Debugging

1. Reproduce with the narrowest command.
2. Read the complete error output.
3. Identify whether the issue is code, build configuration, missing local tools, or environment.
4. Add or update a failing test for behavior bugs when possible; target the smallest public behavior/API guarantee that would have caught the defect.
5. Make the smallest fix.
6. Rerun the narrow reproduction and focused target checks first, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the slice-closing gate.

Do not guess around compiler or CMake errors. Fix the root cause.
Do not add broad or implementation-mirroring tests when one focused regression test or an existing test update captures the durable guarantee.

## Windows Native Debugging

- Use official Debugging Tools for Windows from the Windows SDK for native debugger, dump, and crash work on the MSVC lane; verify with `cdb -version`.
- Configure Microsoft public symbols with `_NT_SYMBOL_PATH=srv*C:\Symbols*https://msdl.microsoft.com/download/symbols` when stack quality matters.
- Treat missing `cdb` or `windbg` as a host diagnostics blocker only for tasks that need a native debugger; do not require `gdb` or `lldb` for normal Windows/MSVC validation.

## Windows GPU capture (D3D12)

- For debug-layer validation, use Windows Graphics Tools and verify `d3d12SDKLayers.dll` when the task needs it.
- For PIX UI from the host (not `MK_editor`), optional `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/launch-pix-host-helper.ps1` resolves `WinPix.exe` (typical install under `Program Files\Microsoft PIX\<version>\`), legacy `PIX.exe`, `PATH`, or `MIRAKANA_PIX_EXE`; use `-SkipLaunch` to verify resolution only. CLI checks remain `pixtool --help`. See `docs/ai-integration.md` for editor vs operator capture handoff.

## Static analysis

- Prefer `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` over raw `clang-tidy`. The wrapper verifies `.clang-tidy`, reuses native Makefile/Ninja `compile_commands.json` when present, or synthesizes `compile_commands.json` under the active preset `binaryDir` from CMake File API codemodel data on the default Windows Visual Studio `dev` lane. Missing CMake or `clang-tidy` is a host blocker, not an engine defect.
- Noisy warnings in **`tests/unit/*.cpp`** and similar churn in **`editor/core`** production TUs: fix patterns first (complete designated initializers—including declaration order when the toolchain flags skipped members—C++23 **`std::string::contains` / `starts_with`** and the same on **`std::string_view`**, one anonymous namespace through test bodies with `int main()` outside for large TUs, `class` test doubles with private state and `const` getters instead of public counters, **Rule of Five** on file-local C API adapters when special-member clang-tidy checks fire). See **Unit tests** and production notes in `docs/cpp-style.md`. Use narrow NOLINT only after the pattern fix or for documented false positives.
