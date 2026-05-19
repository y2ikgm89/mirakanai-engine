# Engine UI Game Menu HUD v1 (2026-05-19)

**Plan ID:** `engine-ui-game-menu-hud-v1`
**Status:** Completed.
**Current pointer rule:** Set `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` to this plan while the milestone is active. Keep `unsupportedProductionGaps = []`; this is a developer-owned capability milestone, not a reopened Engine 1.0 production gap.

## Goal

Add reusable runtime UI primitives that AI-generated games can use for game-owned HUD rows, pause/restart menu intent, prompts, and simple menu commands without depending on Dear ImGui, editor code, renderer internals, native handles, or platform-specific UI services.

## Context

- Engine 1.0 closeout remains zero-gap.
- `gameplay-authoring-foundation-v1` completed runtime scene gameplay binding and interaction planning.
- `engine-save-settings-profile-v1` completed runtime profile path and document bundle primitives.
- The developer-owned backlog identifies `engine-ui-game-menu-hud-v1` as the next foundational unblocker after save/settings/profile work.
- Existing `MK_ui`, `MK_ui_renderer`, runtime input rebinding presentation rows, and sample HUD submissions provide low-level pieces; the missing layer is a small game-facing menu/HUD intent model that generated games can own from game code/data.

## Constraints

- Keep `unsupportedProductionGaps` empty. If this work requires reopening an Engine 1.0 production gap, stop.
- Keep runtime UI game-facing contracts first-party, value-oriented, and host-independent.
- Do not expose Dear ImGui, editor APIs, SDL3, renderer/RHI internals, D3D12, Vulkan, Metal, OS accessibility/IME handles, native handles, UI middleware, or platform user-directory assumptions.
- Keep game-specific menu content and styling in game-owned code/data. Promote only reusable menu/HUD primitives into `MK_ui` or adjacent runtime-facing modules.
- Use RED tests before behavior/API changes and close C++/runtime/public-contract phases with focused checks plus one fresh `tools/validate.ps1`.

## Phase 0: Pointer Sync

**Status:** Completed.

### Goal

Make the manifest pointers, master-plan ledger, and plan registry agree that `engine-ui-game-menu-hud-v1` is the active developer-owned capability after save/settings/profile foundation, while keeping `unsupportedProductionGaps = []`.

### Done When

- `docs/superpowers/plans/README.md` lists this plan as the active `currentActivePlan` slice.
- The production master plan and readiness ledger name this milestone as developer-owned capability work, not an Engine 1.0 unsupported gap.
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and composed `engine/agent/manifest.json` point `currentActivePlan` and `recommendedNextPlan` at this plan while preserving `unsupportedProductionGaps = []`.

## Phase 1: Runtime Menu HUD Intent Model

**Status:** Completed.

### Goal

Add a small runtime/game-facing value model for HUD and menu command intent that generated games can populate before handing rows to existing UI layout/rendering layers.

### Planned API Shape

- Runtime HUD/menu row descriptors for labels, prompts, counters, and command ids.
- A deterministic planning helper that validates duplicate ids, missing command ids, unsupported row states, and pause/restart command intent before game code executes UI commands.
- A source-tree sample consumer using existing `MK_ui`/runtime gameplay code without editor or renderer-native dependencies.

### Implemented Surface

- `MK_ui` exposes `RuntimeMenuHudRowDesc`, `RuntimeMenuHudRowKind`, `RuntimeMenuHudCommandIntent`, `RuntimeMenuHudCommandTarget`, `RuntimeMenuHudPlan`, and `plan_runtime_menu_hud`.
- Valid plans return deterministic `RuntimeMenuHudDisplayRow` and `RuntimeMenuHudCommandRow` values for label, counter, prompt, pause, resume, restart, menu, and custom command intent rows.
- Invalid plans fail closed with diagnostics for missing or duplicate row ids, unsupported row kinds, missing or duplicate command ids, invalid command intents, and invalid command targets; display and command rows remain empty.
- `sample_ui_audio_assets` demonstrates source-tree HUD/menu intent validation before existing `MK_ui` element construction.

### Done When

- RED tests prove valid HUD/menu rows produce deterministic display/command rows.
- RED tests prove duplicate ids, missing command ids, and invalid command targets fail closed.
- Focused UI/runtime sample tests pass.
- Public API boundary, agent/static drift checks, composed manifest, docs, and full validation are updated.

## Validation Evidence

- Phase 1 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` failed before implementation because `RuntimeMenuHudRowDesc`, `RuntimeMenuHudRowKind`, `RuntimeMenuHudCommandIntent`, `RuntimeMenuHudCommandTarget`, `RuntimeMenuHudDiagnosticCode`, and `plan_runtime_menu_hud` were undefined.
- Phase 1 focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests` passed.
- Phase 1 focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_core_tests` passed.
- Phase 1 focused: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_ui_audio_assets` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R sample_ui_audio_assets` passed. One parallel `sample_ui_audio_assets` build hit transient MSVC C1041 PDB contention and passed when rerun alone.
- Phase 1 static: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Phase 1 full phase gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; `production-readiness-audit` reported `unsupported_gaps=0`.
