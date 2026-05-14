# Runtime Session Services Foundation Design

## Goal

Add the first host-independent runtime services every generated game needs per session: save data, settings, localization catalog lookup, and keyboard input action mapping.

## Context

`mirakana_runtime` already depends on `mirakana_platform` for `IFileSystem` and is the game-facing runtime contract module. `mirakana_platform` owns low-level `VirtualInput`, pointer, gamepad, filesystem, and mobile storage primitives. This slice keeps the game-facing document formats in `mirakana_runtime` while reusing those platform contracts.

## Constraints

- Keep `engine/core` unchanged and independent from filesystem, platform, assets, editor, and renderer code.
- Use only first-party `IFileSystem`/`RootedFileSystem` style path access; do not expose OS paths or native handles.
- Keep document formats deterministic and strict: stable key ordering, explicit format lines, versioned save/settings schema values, duplicate-key rejection on load, and no control characters.
- Keep localization as key-to-text data. UI layout and renderer code keep localization keys, not resolved text ownership.
- Keep this first slice to keyboard actions evaluated against `VirtualInput`. Pointer/gamepad action mapping, rebinding UX, binary/cloud saves, runtime diagnostics, and sample-game flows are follow-up work.
- Do not add third-party dependencies.

## Public API Shape

- `mirakana::runtime::RuntimeSaveData`
- `mirakana::runtime::RuntimeSettings`
- `mirakana::runtime::RuntimeLocalizationCatalog`
- `mirakana::runtime::RuntimeInputActionMap`
- `serialize_*`, `deserialize_*`, `load_*`, and `write_*` helpers for each document type through `IFileSystem`
- `RuntimeInputActionMap::action_down`, `action_pressed`, and `action_released` for `VirtualInput`

## Done When

- Unit tests cover save/settings/localization/input action round trips and malformed settings rejection.
- `mirakana_runtime` exposes the new public header and source through CMake.
- `engine/agent/manifest.json`, `docs/roadmap.md`, gap analysis, and generated-game guidance describe the implemented and missing scope honestly.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or reports concrete environment blockers.
