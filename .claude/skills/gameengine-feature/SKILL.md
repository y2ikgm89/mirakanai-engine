---
name: gameengine-feature
description: Adds engine features, public C++ APIs, sample games, and engine-facing gameplay systems. Use when editing engine/, games/, or feature specs under docs/specs/.
paths:
  - "engine/**"
  - "games/**"
  - "docs/specs/**"
---

# GameEngine Feature

1. Read targeted engine context: prefer `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal` or `Standard`, or a targeted `engine/agent/manifest.fragments/*.json` file. Use full `engine/agent/manifest.json` only when the current decision needs it. When you **change** the engine agent contract, edit fragments and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` (never hand-edit `engine/agent/manifest.json`).
2. Start documentation navigation from `docs/README.md`; use `docs/roadmap.md` for current status and `docs/superpowers/plans/README.md` as the plan registry.
3. Read `docs/cpp-style.md` before changing public API names, file layout, or CMake target structure.
4. Read the relevant public headers under `engine/*/include`.
5. Keep game code outside engine modules unless changing the engine itself.
6. Add or update tests before production behavior when the toolchain can run; lock the smallest externally meaningful behavior/API/regression guarantee, and prefer updating existing tests when they already cover the contract.
7. Update docs/specs/plans for non-trivial API or architecture changes. Create a new dated focused plan only for work with its own behavior/API/validation boundary; keep validation-only, docs/manifest/static-check, and small mechanical follow-up inside the active plan checklist.
8. Keep `MK_CXX_STANDARD=23` as the required standard; do not add C++20 compatibility shims.
9. Prefer C++23-native features, project modules, and `import std;` when the active CMake toolchain support is validated.
10. Keep optional C++ dependencies in vcpkg manifest features and preserve the official `builtin-baseline` unless doing an explicit dependency-maintenance task.
11. When changing vcpkg dependencies or feature membership, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`; do not rely on CMake configure to install, restore, or download vcpkg packages.
12. Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` after public header or backend interop changes.
13. Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` after shared C++ implementation-pattern changes; prefer it over raw `clang-tidy` (native `compile_commands.json` or CMake File API synthesis under the `dev` preset `binaryDir`).
14. Use focused target builds/tests/static checks while iterating, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the coherent slice-closing gate.

## Boundaries

- Keep `engine/core` standard-library-only and independent from platform, renderer, asset format, editor, SDL3, Dear ImGui, and native handles.
- Keep platform work behind `engine/platform` interfaces and graphics API work behind renderer/RHI interfaces.
- Keep runtime game UI on public `mirakana::ui` contracts, not editor, SDL3, Dear ImGui, or middleware APIs.
- Use official documentation through Context7 for library, SDK, build-system, and toolchain questions before relying on memory.
- Prefer clean breaking greenfield implementation over compatibility shims, duplicate APIs, or migration layers unless a future release policy explicitly requires compatibility.

## Do Not

Do not add third-party dependencies or assets without updating `docs/dependencies.md`, `vcpkg.json`, and `THIRD_PARTY_NOTICES.md`.
Do not reintroduce configure-time vcpkg restore/install or move optional feature selection into CMake presets.
Do not add compatibility shims for old C++ standards.
Do not add tests that merely mirror implementation details, duplicate an existing guarantee, or over-specify incidental ordering.
Do not claim planned or host-gated capability as implemented without validation evidence.
