# Directory layout tools split v1 (2026-05-11)

## Goal

Split `engine/tools` implementation sources into named subdirectories with CMake `OBJECT` libraries aggregated by `MK_tools`, per [ADR 0003](../adr/0003-directory-layout-clean-break.md) and [directory layout target v1](../../specs/2026-05-11-directory-layout-target-v1.md).

## Context

- Public headers remain `engine/tools/include/mirakana/tools/*.hpp`.
- `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` reference specific `.cpp` paths for contract needles; those paths move with the split.
- Follow-up: each `MK_tools_*` `OBJECT` target uses minimal `target_link_libraries` for compile; `MK_tools` retains the full `PUBLIC` link set for consumers (see spec invariant 4).

## Done when

- [x] Subdirs `engine/tools/shader`, `gltf`, `asset`, `scene` exist with sources and `CMakeLists.txt` fragments.
- [x] `MK_tools` is a `STATIC` library built from `$<TARGET_OBJECTS:...>` of the four object targets.
- [x] Optional importer defines and `miniaudio` / `fastgltf` / `spng` wiring behave as before.
- [x] Validation scripts updated for new `.cpp` paths.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` green on a capable host.

## Validation

| Check | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass (Windows dev preset; host-gated Apple/Metal diagnostics only). |

## Integration

- Plan registry and master plan **Context** treat this slice as **completed foundation**, not a gap burn-down. Navigation and clean-break doc policy: [2026-05-11-production-documentation-stack-v1.md](2026-05-11-production-documentation-stack-v1.md).
