# Editor Game Module Driver MK Editor Visible Reload Transaction Recipe Evidence v1 (2026-05-11)

**Plan ID:** `editor-game-module-driver-mk-editor-visible-reload-transaction-recipe-evidence-v1`  
**Gap:** `editor-productization` (hot reload + stable ABI stream — MK_editor evidence for `run-validation-recipe`)  
**Parent stream:** [2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md](2026-05-11-editor-productization-hot-reload-stable-abi-stream-v1.md)  

## Goal

Close the stream exit wording **“validation recipes that prove safe reload on Windows with `MK_editor` evidence rows”** by surfacing the already-allowlisted `dev-windows-editor-game-module-driver-load-tests` recipe id, host-gate acknowledgement id, and reviewed **DryRun** / **Execute** `pwsh … tools/run-validation-recipe.ps1` command lines in **visible** `mirakana_editor` Game Module Driver controls, backed by retained `play_in_editor.game_module_driver.reload_transaction_recipe_evidence` MK_ui labels and `MK_editor_core_tests` element-id coverage.

## Context

- Reload transaction **implementation** evidence remains in `MK_editor_game_module_driver_load_tests` and the validation recipe slice [2026-05-11-editor-game-module-driver-reload-transaction-validation-recipe-v1.md](2026-05-11-editor-game-module-driver-reload-transaction-validation-recipe-v1.md).
- Operators still need a **visible editor handoff** next to CTest probe labels so the reviewed runner path is obvious without opening manifest JSON.

## Constraints

- **No** `run-validation-recipe` execution from `mirakana_editor`; display-only handoff (same class as CTest probe evidence rows).
- **No** mid-play DLL replacement, active-session hot reload, or stable third-party ABI ready claims.
- Windows-only recipe remains host-gated; non-Windows builds must not imply execution support.

## Done when

- `EditorGameModuleDriverReloadTransactionRecipeEvidenceModel`, `make_editor_game_module_driver_reload_transaction_recipe_evidence_model`, `make_editor_game_module_driver_reload_transaction_recipe_evidence_ui_model`, and `editor_game_module_driver_reload_transaction_recipe_evidence_contract_v1()` exist in `editor/core` with retained element ids under `play_in_editor.game_module_driver.reload_transaction_recipe_evidence` including `.contract_label` → `ge.editor.editor_game_module_driver_reload_transaction_recipe_evidence.v1`.
- `mirakana_editor` `draw_game_module_driver_controls` shows the contract label plus recipe id, host gate id, and both reviewed command strings.
- `tools/check-ai-integration.ps1` editor-shell needles include the recipe id and contract root string; `MK_editor_core_tests` asserts serialized document ids.
- Hot reload + stable ABI stream ledger marks this slice and **stream exit** for the hot-reload track; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the integration host.

## Validation evidence

| Check | Command / artifact | Result |
| --- | --- | --- |
| Unit | `ctest --preset dev --output-on-failure -R MK_editor_core_tests` (from repo root after `cmake --preset dev`) | Passed (2026-05-11; included in `validate.ps1` / `build.ps1` dev CTest lane) |
| Integration | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed (exit code 0; `ai-integration-check: ok`, `100% tests passed, 0 tests failed out of 47`) |
