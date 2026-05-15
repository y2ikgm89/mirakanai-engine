# C++ Style and Layout

## Purpose

This document defines the GameEngine house style for C++ source layout, public API names, CMake targets, and installable SDK packaging.

C++ does not define an official game engine directory layout. GameEngine therefore follows the language rules, C++ Core Guidelines consistency guidance, and target-based CMake packaging conventions.

## Source Layout

- Engine modules live under `engine/<module>/`.
- `MK_tools` implementation sources are grouped under `engine/tools/shader/`, `engine/tools/gltf/`, `engine/tools/asset/`, and `engine/tools/scene/` (CMake `OBJECT` targets); public headers remain `engine/tools/include/mirakana/tools/*.hpp`.
- Public headers live under `engine/<module>/include/mirakana/<module>/*.hpp` for new/maintained code.
- Implementation files live under `engine/<module>/src/*.cpp`.
- Backend-specific RHI modules live under `engine/rhi/<backend>/`.
- Optional platform backends live under `engine/platform/<backend>/`.
- Platform package templates live under top-level `platform/<target>/`; these are app/package scaffolds, not engine runtime modules.
- GUI-independent editor code lives under `editor/core/`.
- Desktop GUI shell code lives under `editor/src/`.
- Game projects live under `games/<game_name>/`, where `<game_name>` matches `^[a-z][a-z0-9_]*$`, and include `game.agent.json` with backend readiness, importer requirements, packaging targets, and validation recipes.
- Tests live under `tests/unit/` with supporting fixtures under `tests/fixtures/`.
- Generated output and local dependency installs stay out of the source contract: `out/`, `build/`, `external/`, and `vcpkg_installed/`.

Public include paths must be stable and collision-resistant. Code outside a module should include headers through the `mirakana/...` prefix:

```cpp
#include <mirakana/core/application.hpp>
#include <mirakana/rhi/rhi.hpp>
```

Do not include from another module's `src/` directory.

## Dependency Direction

Follow `docs/architecture.md`.

- `engine/core` stays standard-library-only.
- Platform APIs stay behind `engine/platform` contracts.
- Graphics APIs stay behind `engine/rhi` and renderer contracts.
- Editor code may depend on public engine modules, but engine runtime modules must not depend on editor code.
- Native OS, window, GPU, and tool handles must stay private behind backend/PIMPL or first-party opaque handles unless an accepted interop design says otherwise.

## C++ Naming

Use this house style for new code:

- Namespace: `mirakana` for public engine API, with nested namespaces only when they clarify ownership, for example `mirakana::rhi` and `mirakana::rhi::vulkan`.
- Types: `PascalCase`, for example `RunConfig`, `SceneRenderPacket`, `RhiFrameRenderer`.
- Pure abstract service contracts: prefix with `I` only for interface-style contracts, for example `ILogger`, `IRenderer`, `IRhiDevice`.
- Functions and methods: `snake_case`, for example `run_headless`, `build_scene_render_packet`.
- Variables and data members: `snake_case`.
- Enum classes: type name in `PascalCase`, enumerators in `snake_case`, for example `BackendKind::d3d12`.
- Constants: `snake_case` for typed constants; use macros only when C/C++ preprocessing is required.
- Macros: `MK_*` and `ALL_CAPS`; do not use all-caps names for normal constants, functions, types, or variables.
- Files and directories: lowercase `snake_case`, with `.hpp` for public headers and `.cpp` for implementation.
- Tests: `<area>_tests.cpp`, with test target names prefixed by `MK_`.

`mirakana::` is the canonical public API namespace; avoid adding new public symbols under compatibility aliases unless an explicit migration plan authorizes temporary bridges.

Repository artifacts that are not C++ source can keep ecosystem-required names when the owning tool requires them. Examples include PowerShell command-style script names, `.agents/skills/<skill-name>/SKILL.md`, and JSON ID values. Game manifest `name`, validation recipe names, runtime scene validation IDs, and asset keys use stable lowercase kebab-case identifiers, while source directories and file paths stay lowercase `snake_case`.

`engine/agent/manifest.json` is the engine-facing AI integration contract, not a C++ module. It is intentionally exempt from the `engine/<module>/include/mirakana/<module>/*.hpp` layout.

Avoid C++ reserved identifier forms in project-defined names:

- Do not define names containing `__`.
- Do not define names beginning with `_` followed by an uppercase letter.
- Do not define global names beginning with `_`.
- Do not add symbols to namespace `std`.

Compiler, OS, and SDK-provided macros such as `_WIN32`, `__APPLE__`, and `__LINE__` may be used only as platform/compiler probes.

## CMake Naming

- Local CMake targets use the `MK_<module>` prefix, for example `MK_core`, `MK_rhi`, `MK_scene_renderer`.
- Backend targets use `MK_rhi_<backend>` or `MK_platform_<backend>`.
- Test targets use `MK_<area>_tests`.
- Installed package targets use the `mirakana::` namespace and concise component names, for example `mirakana::core`, `mirakana::rhi`, and `mirakana::scene_renderer`.
- Use target-scoped commands: `target_link_libraries`, `target_include_directories`, `target_compile_features`, `target_compile_options`, and `target_sources`.
- Do not use global include directories or global compiler definitions unless every target truly needs them.
- The required C++ standard is configured by `MK_CXX_STANDARD=23`. Do not lower this for compatibility without a new architecture decision.
- MSVC C++23 builds use the centralized `MK_MSVC_CXX23_STANDARD_OPTION` cache value. Keep it at `/std:c++23preview` until a stable `/std:c++23` path is available, then switch the central value rather than editing individual targets.
- C++ module sources must be added through CMake `FILE_SET CXX_MODULES`, not ad hoc compiler flags.
- `import std;` is allowed only when the active CMake generator/toolchain reports C++23 standard-library module support. Keep public installed SDK headers available until module export/install design is accepted.
- C++23-only standard-library features are allowed after local validation. Encapsulate compiler-specific workarounds in implementation files or backend modules, not public API compatibility shims.
- Public include directories must provide both build-tree and install-tree interfaces:

```cmake
target_include_directories(MK_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
```

## Unit tests (`tests/unit/*.cpp`)

Large translation units (for example `core_tests.cpp`, `editor_core_tests.cpp`) should stay clean under **clang** warnings and **clang-tidy** (`misc-include-cleaner`, aggregate-init, `readability-container-contains`, optional `bugprone-swapped-arguments` false positives on ordered APIs):

- **Aggregate literals**: Prefer **designated initializers** and list every field the toolchain expects (empty vectors, default nested structs, `std::nullopt` / value-init for optionals, empty **`std::function`** as **`{}`** when the struct ends with a callable member). Avoid half-filled braced structs when the compiler or IDE warns about missing fields. When `-Wmissing-designated-field-initializers` (or clang-tidy) complains about skipped members, follow the **struct’s declaration order** in the same braced initializer (for example list `acknowledge_host_gates` before `acknowledged_host_gates` when both appear in `EditorAiReviewedValidationExecutionDesc`, or list every `ScenePrefabInstanceRefreshPolicy` member through **`load_prefab_for_nested_propagation`** in `mirakana::editor::scene_authoring.hpp` order when tests construct policies with `{ ... }`).
- **Substring checks on `std::string` and `std::string_view`**: Prefer C++23 **`contains`** and **`starts_with`** on the view or string instead of `find(...) == npos`, `!= npos`, or `find(...) == 0` for plain substring/prefix membership (including multi-line string literals joined with adjacent `"..."` tokens).
- **Rule of Five for resource-like adapters**: If a class defines a non-trivial destructor and **deleted** copy construction/assignment (for example a file-local `I*` adapter over a C function table), also **delete or define** move construction and move assignment so special members are explicit and clang-tidy special-member checks stay satisfied. When the instance must never relocate duplicate non-owning callback state, **`= delete` move** is normal; **`std::unique_ptr<T>`** transfers pointer ownership between `unique_ptr` objects **without** move-constructing the owned `T`.
- **File-local helpers**: Put helper `struct` / small test doubles used only in one `.cpp` in an **anonymous namespace** at file scope unless they must be shared across TUs. For large `tests/unit/*.cpp` files that define many `MK_TEST` bodies **and** file-local helpers, use **one** anonymous namespace opened after `#include` lines and closed with `} // namespace` **immediately before** `int main()` so `main` stays at global scope while helpers and test static functions share internal linkage.
- **Test doubles with observable state**: Prefer a `class` test double with **private** scalar/vector state and small **`const` getters** (for example `begin_count()`) over public data members when `cppcoreguidelines-non-private-member-variables-in-classes` fires; reserve narrow `NOLINT` only for documented false positives or legacy aggregates intentionally modeled as plain `struct` POD.
- **Ambiguous two-parameter calls**: If clang-tidy reports swapped-argument suspicion on a correctly ordered call (for example scene parenting), clarify intent with well-named locals first; use a **narrow** `NOLINTNEXTLINE` only for persistent false positives.
- **Small closed `enum class`**: When clang-tidy **`performance-enum-size`** applies (few enumerators, no need for `int` width), prefer an explicit fixed-width underlying type such as **`std::uint8_t`** so layout stays tight; confirm every enumerator fits in the chosen type and treat the choice as an **ABI-visible** decision for structs that embed the enumeration (greenfield APIs may prefer cleanliness over default `int` compatibility).
- **`std::optional` and `bugprone-unchecked-optional-access`**: Clang-tidy does not always propagate invariants from helper predicates (for example a `matches_source(...)` that implies `has_value()`). Prefer an explicit **`has_value()`** (or a local `const T&` from `value()` after a guard) on the same control-flow path before `operator->` / `operator*`.

## Desktop editor shell (`editor/src/main.cpp`)

The optional Dear ImGui adapter for `MK_editor` is a large translation unit, with companion shell sources under `editor/src/` (for example `material_preview_gpu_cache.cpp`, `sdl_viewport_texture.cpp`). Patterns for ImGui varargs vs `std::string_view`, `std::array` scratch buffers, `_WIN32` `<windows.h>` include order (no duplicate `windows.h` in one TU), environment reads (`GetEnvironmentVariable` vs `_dupenv_s`), material GPU preview and SDL display bridge ownership (`editor/include/mirakana/editor/material_preview_gpu_cache.hpp`, `sdl_viewport_texture.hpp` plus their `.cpp`), keeping optional RHI backend implementation includes in the `.cpp` when the public header API does not name those types, and updating **`editor/include/compile_flags.txt`** / **`editor/src/compile_flags.txt`** when clangd needs new include roots, are spelled out under **Testing** and **AI Development Workflow** in `AGENTS.md` and under **Dear ImGui shell** / **MK_editor Windows and material-preview host cache** in `.claude/skills/gameengine-editor/SKILL.md` (Codex twin: `.agents/skills/editor-change/SKILL.md`). For **`editor/core`** implementation TUs (not the ImGui shell), see **Editor core C++** in the same editor skills and production adapter notes under **Unit tests** in this file.

## AI Development Notes

Before changing public API shape, module layout, CMake target names, or game scaffolding:

1. Read `AGENTS.md`.
2. Read this document.
3. Read `docs/architecture.md`.
4. Run or inspect `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`.
5. Update AI-facing docs, skills, rules, manifests, or subagents if the change alters how agents should work.
6. Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before reporting completion.
