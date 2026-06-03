---
paths:
  - "engine/**/*.hpp"
  - "engine/**/*.cpp"
  - "editor/**/*.hpp"
  - "editor/**/*.cpp"
  - "tests/**/*.cpp"
  - "examples/**/*.cpp"
  - "games/**/*.cpp"
---

# C++ Engine Rules

- Use C++23 as the required language level.
- Prefer C++23-native designs and allow C++23-only language/library features when they simplify ownership, APIs, or compile-time structure.
- Use project C++ modules through CMake `FILE_SET CXX_MODULES`; keep public installed headers available until module export/install support is intentionally designed.
- Use `import std;` only where the active CMake generator/toolchain reports support; keep it gated by the central CMake policy.
- Do not add C++20 compatibility shims or lower the engine standard without a new architecture decision.
- Follow `docs/cpp-style.md` for naming, source layout, public include paths, CMake target naming, and installable package targets.
- In **`tests/unit/*.cpp`**, keep **designated aggregate literals** complete in **declaration order** when structs gain fields (empty **`std::function`** as **`{}`**); see **`docs/cpp-style.md`** (**Unit tests**).
- Keep `engine/core` standard-library-only.
- Add or update tests before production behavior when the toolchain can run; lock the smallest externally meaningful behavior/API/regression guarantee and prefer existing-test updates when they already cover the contract.
- Avoid tests that merely mirror implementation details, duplicate an existing guarantee, or over-specify incidental ordering unless deterministic ordering is the public contract.
- Prefer RAII, value types, and explicit ownership.
- Prefer C++23 **`std::string_view::contains`** / **`starts_with`** (and `std::string` equivalents) over `find` + `npos` for plain substring/prefix checks. Types with a non-trivial destructor and deleted copy operations must also declare **move construction and move assignment** (delete, default, or custom) so special members stay explicit (**Rule of Five**); see `docs/cpp-style.md`.
- Keep native OS, window, GPU, and tool interop handles behind backend/PIMPL or first-party opaque handles unless an explicit interop design is accepted.
- Treat removed Dear ImGui shell implementation as historical evidence only; runtime game UI should use public `mirakana::ui` contracts and must not depend on editor, removed SDL3 adapters, Dear ImGui, or UI middleware APIs.
- The optional Windows-native `MK_editor` shell is active through the dependency-free `desktop-editor` lane. Do not recreate the removed SDL3 `editor/src/main.cpp`, material preview GPU cache, SDL viewport bridge, or editor-local `compile_flags.txt` fallbacks.
- The current Windows D3D12 viewport/material visible texture compositor stays private to `editor/src`; future Vulkan/Metal parity, broader material-preview GPU parity, or renderer-quality claims must keep backend handles out of public APIs and add focused validation evidence before broadening ready claims.
- On `_WIN32`, prefer `GetEnvironmentVariableA`/`W` into `std::string` over `_dupenv_s` + CRT `free` for read-only environment lookups in `editor/` or other first-party sources unless a stronger adapter already wraps the read.
- Keep future editor shell caches behind explicit APIs; avoid public mutable fields on ad-hoc aggregates. Keep optional RHI backend implementation includes in implementing `.cpp` files when public headers do not need those types for their API (`misc-include-cleaner`).
- Keep text shaping, font rasterization, IME, accessibility bridges, image decoding, and platform integration behind first-party adapter boundaries instead of exposing those dependencies through public game APIs.
- Use `mirakana::` for public engine API in new or migrated code.
- For optional C++ dependencies, use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` before vcpkg-backed lanes and keep CMake configure lanes configure-only with `VCPKG_MANIFEST_INSTALL=OFF`.
- Presets reference **`external/vcpkg/scripts/buildsystems/vcpkg.cmake`**; do not delete `external/vcpkg` as part of cache cleanup. Restore the clone from upstream vcpkg when missing before bootstrap or configure.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1` after entering a manual linked worktree so ignored roots and `external/vcpkg` are ready before configure.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` before diagnosing CMake, compiler, CTest, CPack,
  `clang-format`, Visual Studio, MSBuild, or `PATH` failures; use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
  -RequireDirectCMake` only when raw `cmake --preset ...` commands must work from the active shell. Local focused loops should use
  `tools/cmake.ps1` and `tools/ctest.ps1`; visible CMake configure/build presets must inherit `normalized-configure-environment` /
  `normalized-build-environment` and keep raw preset `PATH`/`Path` normalization, while the wrappers pass Windows CMake/MSBuild one
  canonical child `Path` instead of duplicate or uppercase-only `PATH`/`Path` keys.
- Prefer `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` for formatting. Raw `clang-format --dry-run ...` commands require `clang-format` on `PATH`; check `direct-clang-format-status` before treating raw lookup failure as a format blocker.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` after changing shared C++ implementation patterns; it verifies
  `.clang-tidy` and supplies `clang-tidy` with native or File-API-synthesized `compile_commands.json` for the default `dev` preset (see
  `AGENTS.md` for `Get-RepoRoot` string usage in tooling scripts). Full `tools/validate.ps1` reuses the File API reply generated by its
  build gate for the tidy smoke, while standalone tidy keeps configure-on-demand behavior. For a single TU after `pwsh -NoProfile
  -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`, use `check-tidy.ps1 -Files <path>`; `.clang-tidy` enables
  `misc-include-cleaner` (IWYU-style includes), so prefer direct `#include`s that match actual symbols (see `docs/cpp-style.md`).
- On Windows, use official Debugging Tools for Windows for native debugger work, Windows Graphics Tools for the D3D12 debug layer, PIX on Windows for D3D12 captures, and Windows Performance Toolkit for ETW/performance diagnostics. Treat them as host diagnostics, not default build dependencies. Optional PIX UI launch from a non-repo scratch directory: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/launch-pix-host-helper.ps1` (see `AGENTS.md` Testing and `docs/ai-integration.md`).
- Use focused target builds/tests/static checks while iterating, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the C++/runtime/build/packaging/public-contract slice gate before completion.
