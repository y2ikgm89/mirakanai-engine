# Editor Productization Hot Reload And Stable ABI Stream v1 (2026-05-11)

**Plan ID:** `editor-productization-hot-reload-stable-abi-stream-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Status:** Completed (hot reload track exit; stable ABI exclusion path complete; mid-play DLL replacement remains unsupported)  

## Goal

Separate **same-engine-build editor game module driver** evidence (already landed) from future **active-session hot reload** and **stable third-party binary ABI** tracks, each with its own dated plan, tests, and manifest boundaries.

## Context

- Dynamic probe, contract metadata, safe reload review, play session gates, and CTest probe evidence UI narrow the gap without claiming hot reload or third-party ABI.
- `GameEngine.EditorGameModuleDriver.v1` remains the reviewed interchange surface.

## Constraints

- Hot reload must preserve editor undo stacks, scene isolation, and fail-closed diagnostics; no silent DLL replacement during play without an explicit reviewed policy UI.
- Third-party ABI stability is a **distinct** legal/engineering decision (versioned C ABI exports, symbol visibility, MSVC runtime pairing)—do not conflate with same-repo driver builds.

## Done when (stream exit)

- **Hot reload track:** explicit session states, reload barriers, and validation recipes that prove safe reload on Windows with `MK_editor` evidence rows. **Met** through session phase snapshot + barrier/policy rows, `dev-windows-editor-game-module-driver-load-tests` `run-validation-recipe` wiring, and visible `mirakana_editor` reviewed argv handoff rows (`reload_transaction_recipe_evidence`) without mid-play DLL replacement.
- **Stable ABI track:** documented external SDK header + `.def` + versioned factory contract, or manifest exclusion from 1.0 ready surface with rationale. **Exclusion path for 1.0:** completed via [2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md](2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md) (`docs/legal-and-licensing.md`, `docs/dependencies.md`; no vendor-stable C ABI for 1.0).

## Next suggested child slices

1. ~~Hot reload state machine spec in `docs/specs/` tied to a dated implementation plan.~~ Implemented: [2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md](../specs/2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-v1.md) and [2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-spec-v1.md](2026-05-11-editor-game-module-driver-hot-reload-session-state-machine-spec-v1.md) (`EditorGameModuleDriverHostSessionPhase`, `play_in_editor.game_module_driver.session`; does not implement active-session hot reload).
2. ~~Reload transaction tests using existing dynamic library loader with double-load forbidden cases.~~ Implemented: [2026-05-11-editor-game-module-driver-reload-transaction-load-tests-v1.md](2026-05-11-editor-game-module-driver-reload-transaction-load-tests-v1.md) (`MK_editor_game_module_driver_load_tests` full unload + second load session; paired `DynamicLibrary` refcount on Windows; does not implement active-session hot reload).
3. ~~Stable ABI:~~ **1.0 exclusion documented** — [2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md](2026-05-11-editor-game-module-driver-stable-third-party-abi-1-0-exclusion-v1.md). A future **shipping** vendor-stable C SDK would still require a separate ABI gate program (headers, `.def`, versioned factory, compatibility tests); that is not claimed for 1.0.
4. **Completed (active-session DLL barrier policy UI):** [2026-05-11-editor-game-module-driver-active-session-dll-barriers-policy-ui-v1.md](2026-05-11-editor-game-module-driver-active-session-dll-barriers-policy-ui-v1.md) — retained `play_in_editor.game_module_driver.session` rows `barriers_contract_label` (`ge.editor.editor_game_module_driver_host_session_dll_barriers.v1`), `barrier.play_dll_surface_mutation.status`, `policy.active_session_hot_reload`, `policy.stopped_state_reload_scope`; snapshot fields derived from `play_session_active`; does not implement active-session hot reload or operator overrides.
5. **Completed (reload transaction validation recipe):** [2026-05-11-editor-game-module-driver-reload-transaction-validation-recipe-v1.md](2026-05-11-editor-game-module-driver-reload-transaction-validation-recipe-v1.md) — `dev-windows-editor-game-module-driver-load-tests` in `run-validation-recipe` allowlist, `tools/run-editor-game-module-driver-load-tests.ps1`, `windows-msvc-dev-editor-game-module-driver-ctest` host gate; Windows-only `MK_editor_game_module_driver_load_tests` CTest execution path; does not launch `mirakana_editor` or implement active-session hot reload.
6. **Completed (fail-closed DLL mutation order guidance):** [2026-05-11-editor-game-module-driver-active-session-hot-reload-fail-closed-order-v1.md](2026-05-11-editor-game-module-driver-active-session-hot-reload-fail-closed-order-v1.md) — `EditorGameModuleDriverHostSessionSnapshot::policy_dll_mutation_order_guidance`, retained `play_in_editor.game_module_driver.session.policy.dll_mutation_order_guidance`, phase-keyed stable strings, `MK_editor_core_tests` play-active load/reload/unload barrier matrix; does not implement mid-play DLL replacement.
7. **Completed (MK_editor visible DLL mutation order guidance):** [2026-05-11-editor-game-module-driver-mk-editor-visible-dll-mutation-order-guidance-v1.md](2026-05-11-editor-game-module-driver-mk-editor-visible-dll-mutation-order-guidance-v1.md) — `mirakana_editor` `draw_game_module_driver_controls` renders `DLL mutation order guidance:` from `session_snapshot.policy_dll_mutation_order_guidance` next to other host-session policy lines; `check-ai-integration` `editor/src/main.cpp` needle; does not implement mid-play DLL replacement.
8. **Completed (MK_editor visible reload transaction recipe evidence):** [2026-05-11-editor-game-module-driver-mk-editor-visible-reload-transaction-recipe-evidence-v1.md](2026-05-11-editor-game-module-driver-mk-editor-visible-reload-transaction-recipe-evidence-v1.md) — retained `play_in_editor.game_module_driver.reload_transaction_recipe_evidence` rows with `ge.editor.editor_game_module_driver_reload_transaction_recipe_evidence.v1` contract label plus visible `mirakana_editor` reviewed `run-validation-recipe` DryRun/Execute argv lines for `dev-windows-editor-game-module-driver-load-tests` and `windows-msvc-dev-editor-game-module-driver-ctest`; `MK_editor_core_tests`; does not execute recipes from the editor shell or implement mid-play DLL replacement.
