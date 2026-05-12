# Directory layout target v1 (2026-05-11)

## Status

Accepted as the repository **target layout** for `engine/tools` implementation sources. Top-level product tree (`engine/<module>/`, `games/`, `editor/`, `platform/`, `tests/`, `tools/`) remains SDK-style per [architecture-directory-verification.md](../architecture-directory-verification.md).

## Authority

- [ADR 0003: Directory layout clean-break](../adr/0003-directory-layout-clean-break.md)
- [docs/cpp-style.md](../cpp-style.md) (public include paths, CMake target names)
- [docs/architecture.md](../architecture.md) (dependency direction)

## Invariants

1. **Public headers** for tools stay under `engine/tools/include/mirakana/tools/*.hpp` (no subdirectory split in the install include tree for this v1).
2. **`MK_tools`** remains the single linkable CMake target for consumers; implementation is split into **`OBJECT` libraries** under `engine/tools/{shader,gltf,asset,scene}/` aggregated by `MK_tools` `STATIC`.
3. **`engine/agent`** and composed `manifest.json` continue to list module path `engine/tools` and header paths under `engine/tools/include/...` only.
4. **Per-cluster compile links:** Each `MK_tools_*` `OBJECT` target declares the minimal `PUBLIC` `target_link_libraries` needed to compile that cluster (see `engine/tools/{shader,gltf,asset,scene}/CMakeLists.txt`). **`MK_tools`** keeps the full `PUBLIC` `MK_*` link set so consumers and the installable package preserve the same link closure as before this refinement.

## `MK_tools` source inventory (by cluster)

| Cluster | Directory | Translation units | Role |
| --- | --- | --- | --- |
| Shader | `engine/tools/shader/` | `shader_compile_action.cpp`, `shader_tool_process.cpp`, `shader_toolchain.cpp`, `material_graph_shader_pipeline.cpp` | Shader toolchain runner, compile actions, material graph shader pipeline |
| glTF | `engine/tools/gltf/` | `gltf_mesh_inspect.cpp`, `gltf_skin_animation_inspect.cpp`, `gltf_skin_animation_import.cpp`, `gltf_morph_animation_import.cpp`, `gltf_node_animation_import.cpp` | glTF inspect/import adapters (importer-gated sections) |
| Asset | `engine/tools/asset/` | `asset_file_scanner.cpp`, `asset_import_adapters.cpp`, `asset_import_tool.cpp`, `asset_package_tool.cpp`, `morph_mesh_cpu_source_bridge.cpp`, `registered_source_asset_cook_package_tool.cpp`, `source_asset_registration_tool.cpp`, `source_image_decode.cpp`, `material_tool.cpp`, `tilemap_tool.cpp`, `ui_atlas_tool.cpp` | Asset registration, import, packages, cook, materials, UI atlas |
| Scene / package | `engine/tools/scene/` | `physics_collision_package_tool.cpp`, `runtime_scene_package_validation_tool.cpp`, `scene_prefab_authoring_tool.cpp`, `scene_tool.cpp`, `scene_v2_runtime_package_migration_tool.cpp` | Scene authoring, migration, runtime package validation, collision package helpers |

## Validation evidence (implementation slice)

Recorded in [2026-05-11-directory-layout-tools-split-v1.md](../superpowers/plans/2026-05-11-directory-layout-tools-split-v1.md): `tools/validate.ps1` after the split, and updated `tools/check-json-contracts.ps1` / `tools/check-ai-integration.ps1` paths for moved `.cpp` files.

## Optional follow-ups (not v1)

- Physical regrouping of other `engine/*` modules (for example bridge layers) — separate plan + ADR amendment only when justified.
- Splitting installed `mirakana::tools` into multiple exported components — only if consumers need smaller link units.
