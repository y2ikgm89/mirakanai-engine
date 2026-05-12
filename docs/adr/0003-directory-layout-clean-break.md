# ADR 0003: Directory Layout Clean-Break (SDK-Style, CMake-First)

## Status

Accepted.

## Context

The repository follows an SDK-style layout (`engine/<module>/`, `games/`, `editor/`, `platform/` templates) documented in [architecture-directory-verification.md](../architecture-directory-verification.md). There is no ISO or industry-wide canonical game-engine folder tree; maintainability comes from **CMake target boundaries**, [cpp-style.md](../cpp-style.md) public include layout, and [architecture.md](../architecture.md) dependency direction.

`engine/tools` aggregated many unrelated development-time bridges in one flat `src/` tree, increasing review cost and “god module” risk while staying a single `MK_tools` target.

## Decision

1. **Goals**
   - Keep **public** `#include <mirakana/tools/...>` paths and **`MK_tools`** / **`mirakana::tools`** export names stable unless a separate ADR authorizes a breaking rename.
   - Split `engine/tools` **implementation** into **named subdirectories** with **CMake `OBJECT` libraries** aggregated by a single **`MK_tools`** static library (umbrella). This improves ownership and build structure without forcing consumers to link multiple targets.
   - Record the **target repository layout** in [docs/specs/2026-05-11-directory-layout-target-v1.md](../specs/2026-05-11-directory-layout-target-v1.md) and keep [architecture.md](../architecture.md) aligned.

2. **Non-goals**
   - Mimicking Unreal, Unity, Godot, or O3DE directory branding.
   - Renaming `mirakana::` namespaces or shuffling `engine/<module>/include/mirakana/...` prefixes without a dedicated migration plan.
   - Moving `engine/agent` (AI contract) under a C++ module include tree.

3. **Validation gates**
   - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after each coherent slice.
   - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` when public headers or install layout change.
   - `examples/installed_consumer` continues to configure against the installed `Mirakanai` package when install/export paths are touched.

4. **Optional follow-up (Phase C)**
   - Physical regrouping of other `engine/*` modules (e.g. bridge layers) requires a new dated plan, evidence of pain (navigation or dependency leaks), and the same gates.

## Rationale

- **OBJECT + umbrella** preserves a single linkable `MK_tools` for games, editor, and tests while making source ownership visible on disk.
- Aligns with CMake best practice (target-scoped sources) without multiplying installed package components for this slice.

## Consequences

- More `CMakeLists.txt` files under `engine/tools/`.
- Contributors place new tool sources in the subdirectory that matches responsibility (shader toolchain vs asset vs glTF vs scene/package).

## Rollback

- Revert subdirectory CMake fragments and move sources back to a flat `engine/tools/src/`, restoring the previous single `add_library(MK_tools ...)` list.
