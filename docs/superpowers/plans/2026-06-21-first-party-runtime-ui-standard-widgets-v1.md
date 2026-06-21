# First-Party Runtime UI Standard Widgets v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add first-party runtime UI standard widgets for HUD meters, pause/menu flows, and package-visible sample counters on top of existing `MK_ui` / `MK_ui_renderer`, while proving the implementation is independent of Unity, Unreal Engine, Godot, and third-party UI middleware.

**Architecture:** Keep the feature in `mirakana::ui` as value-model planning and `UiDocument` composition. Reuse existing retained-mode UI primitives, renderer submission, runtime menu HUD, accessibility/localization fields, and sample package smoke lanes. Add a fail-closed provenance review API that blocks copied external code, copied UI assets, external engine public names, and unreviewed UI middleware before any runtime widget plan can report ready.

**Tech Stack:** C++23, `MK_ui`, `MK_ui_renderer`, `sample_2d_desktop_runtime_package`, `sample_generated_desktop_runtime_3d_package`, PowerShell 7 validation scripts, first-party repository documentation and manifest fragments.

---

## Legal And Independence Position

This plan is an engineering compliance plan, not legal advice. The implementation boundary is exact:

- The implementation uses no Unity, Unreal Engine, or Godot source code.
- The implementation uses no Unity, Unreal Engine, or Godot sample code, UI assets, icons, fonts, themes, screenshots, templates, editor layouts, widget blueprints, scenes, project files, or marketplace assets.
- The implementation uses no public API names that identify external engine UI systems, including `uGUI`, `UMG`, `Slate`, `Widget Blueprint`, `Control`, `CanvasLayer`, `UIDocument`, or `VisualElement`.
- The implementation uses no third-party UI middleware, including Dear ImGui, Qt, Slint, RmlUi, Noesis, or webview UI.
- The implementation adds no dependency by default and keeps `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md` unchanged unless a new external material is explicitly introduced through the repository dependency process.
- Official Unity, Unreal Engine, and Godot documentation is used only to identify common functional categories: retained UI trees, menus, HUD overlays, layout containers, buttons, labels, sliders, progress or meter visuals, styling, localization, accessibility, and renderer handoff.

The implementation must fail validation if any copied expression or unreviewed external material is introduced.

## Legal And IP Risk Model

The legal engineering position is:

- HUD meters, HP/MP bars, labels, buttons, menus, focus order, retained UI documents, layout containers, styling tokens, localization metadata, accessibility labels, and renderer handoff are general UI/game-engine functions. They can be implemented independently when the project supplies its own code, data model, naming, tests, and visual expression.
- Copyright risk is controlled by excluding copied expression. The U.S. Copyright Office states that copyright protects expression and not ideas, procedures, methods, systems, processes, concepts, principles, or discoveries. This plan treats functional UI categories as reference-only ideas and forbids copying examples, documentation text, source code, asset files, screenshots, themes, or distinctive layouts.
- Unity risk is controlled by using Unity documentation only as official category research. Unity Editor rights and source-code terms are license-bound; no Unity Editor, Unity source, UXML, USS, C# sample, package asset, icon, font, or UI name is allowed in the implementation.
- Unreal Engine risk is controlled by using Epic documentation only as official category research. Unreal Engine Licensed Technology and Epic Licensed Content are governed by Epic agreements; no Unreal source, Blueprint graph, Slate declarative syntax, UMG designer structure, Marketplace/Learn/Quixel content, starter content, icon, font, or UI name is allowed in the implementation.
- Godot risk is controlled by not incorporating Godot source or assets. Godot is MIT licensed and may be used with notice obligations if copied or distributed, but this plan does not copy or distribute Godot code, binaries, default themes, sample scenes, docs examples, or assets, so no Godot notice is introduced by this feature.
- Trademark and trade-dress risk is controlled by not presenting this feature as Unity-, Unreal-, or Godot-compatible; not using their names in public API identifiers, product-facing feature names, sample output, or marketing claims; and not copying their editor/runtime visual appearance.
- Patent risk is not cleared by this plan. The feature intentionally stays in ordinary value-model HUD/menu composition and avoids claims of equivalence to proprietary systems. Any later patent-sensitive feature such as visual scripting compatibility, proprietary UI builder import, or engine-specific file/API compatibility requires a separate legal review gate before implementation.

The practical verdict is: implementing first-party HUD meters and menus is acceptable under this plan's controls; copying or emulating another engine's protected code, assets, samples, distinctive visual expression, public UI-system naming, or compatibility surface is not acceptable and must fail the task.

## Research Verdicts

Official documentation confirms the comparison target:

- Unity has runtime UI systems through UI Toolkit and Unity UI, with runtime documents, UI elements, styles, and layout workflows. This plan does not copy Unity UXML, USS, C# API shapes, samples, package assets, names, or visual style.
- Unreal Engine has UMG, Slate, HUD, widget blueprints, progress bars, menus, and widget interaction. This plan does not copy Unreal C++/Blueprint API shapes, Slate declarative syntax, UMG designer structures, samples, starter content, names, or visual style.
- Godot has `Control`-based UI nodes, containers, labels, buttons, progress bars, anchors, HUD examples, and MIT-licensed source. This plan does not copy Godot source, scene trees, documentation examples, default theme assets, names, or visual style.
- The repository policy already requires first-party `MK_ui` / `MK_editor_ui` surfaces, audited adapters for low-level text/font/accessibility/image/platform work, and notice updates for any external dependency or asset.
- The Context7 refresh on 2026-06-22 selected official Unity, Unreal Engine, and Godot documentation sources and found the same common functional categories: runtime UI systems, widget/control trees, HUDs, menus, layout/styling, buttons/labels, progress or meter controls, interaction/focus, and renderer/display handoff. These categories inform feature coverage only; they do not authorize code, asset, API, sample, theme, or visual-expression reuse.
- The legal-source refresh on 2026-06-22 checked official Unity, Epic, Godot, and U.S. Copyright Office pages. The implementation remains first-party because no external engine licensed technology, licensed content, source, binary, or asset is copied or distributed, and Godot MIT notice obligations are not triggered unless Godot software or substantial copied portions enter the distribution.

## Non-Copy Design Differentiators

This feature must remain visibly and structurally project-owned:

- Namespace and API: use `mirakana::ui`, `RuntimeUi*`, `UiDocument`, `SemanticRole`, and value-plan types. Do not introduce `uGUI`, `UMG`, `Slate`, `Widget Blueprint`, `Control`, `CanvasLayer`, `UIDocument`, `VisualElement`, `UXML`, `USS`, `Blueprint`, `Node`, or `Theme` as public feature terms.
- Data model: use plain C++ value descriptors and planning results. Do not create engine-compatibility loaders, external scene/widget blueprint importers, serialized Unity/Godot/Unreal schema readers, or wrappers around their object models.
- Rendering contract: emit first-party retained `UiDocument` elements and existing renderer-submission evidence. Do not copy external renderer paths, widget lifecycles, sample tree structures, or declarative syntax.
- Visual expression: tests and samples may use project-owned tokens such as `hud.meter.health.fill`, `hud.meter.mana.fill`, and `hud.meter.warning.fill`. Do not copy default color palettes, fonts, icons, progress-bar geometry, editor window layouts, sample screenshots, or starter UI themes from external engines.
- Documentation usage: external official docs may appear only as source-reference URLs and category-research rows. Do not quote long passages, paste examples, or derive fixture strings from docs or tutorials.

## Source Rules For Implementers

- Use only repository code and original implementation written for this task.
- When reading external engine documentation, record it as `official_documentation` provenance and do not paste examples into source, tests, docs, comments, or sample output.
- If a design note, test fixture, color token, geometry, layout, icon name, or string was derived from an external engine artifact, delete it and replace it with a first-party design.
- Keep all user-visible sample widget names generic or project-owned: `health`, `mana`, `stamina`, `runtime_menu`, `pause_menu`, `inventory_menu`, `hud_meter`, and `standard_hud`.
- Keep all public API names under `mirakana::ui` and `RuntimeUi*`.
- Do not add copyright notices for Unity, Epic, or Godot because no code, assets, or documentation text from those sources may enter the repository.

## Files To Change

- Create `engine/ui/include/mirakana/ui/runtime_ui_standard_widgets.hpp`.
- Create `engine/ui/src/runtime_ui_standard_widgets.cpp`.
- Create `tests/unit/runtime_ui_standard_widgets_tests.cpp`.
- Modify `engine/ui/CMakeLists.txt` to compile `src/runtime_ui_standard_widgets.cpp` into `mirakana_ui`.
- Modify root `CMakeLists.txt` to add `MK_runtime_ui_standard_widgets_tests`.
- Modify `games/sample_2d_desktop_runtime_package/main.cpp` to add the `--require-runtime-ui-standard-widgets` smoke lane.
- Modify `games/sample_generated_desktop_runtime_3d_package/main.cpp` to add the same smoke lane.
- Modify `docs/ui.md`, `docs/current-capabilities.md`, and `docs/roadmap.md` to describe the supported and unsupported widget boundaries.
- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and the targeted game guidance fragment that lists runtime UI capabilities.
- Run `tools/compose-agent-manifest.ps1 -Write` after manifest fragment edits.
- Modify `tools/check-ai-integration.ps1` or the targeted static contract chapter when adding manifest or package counter needles.

## Public API Shape

Add this first-party header shape. Field names are fixed to avoid external engine terminology:

```cpp
// engine/ui/include/mirakana/ui/runtime_ui_standard_widgets.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <mirakana/ui/ui.hpp>

namespace mirakana::ui {

enum class RuntimeUiSourceReferenceKind : std::uint8_t {
    official_documentation,
    first_party_design,
    third_party_code,
    third_party_asset,
    marketplace_asset,
    ai_generated_asset,
};

struct RuntimeUiSourceReference {
    RuntimeUiSourceReferenceKind kind{RuntimeUiSourceReferenceKind::first_party_design};
    std::string name;
    std::string source_url;
    std::string license_id;
    bool contains_copied_expression{false};
    bool distributed_with_runtime{false};
};

enum class RuntimeUiProvenanceDiagnosticCode : std::uint8_t {
    copied_external_expression,
    third_party_code_without_notice,
    third_party_asset_without_notice,
    marketplace_asset_without_review,
    external_engine_public_name,
    unsupported_ui_middleware_reference,
};

struct RuntimeUiProvenanceDiagnostic {
    RuntimeUiProvenanceDiagnosticCode code{RuntimeUiProvenanceDiagnosticCode::copied_external_expression};
    std::string subject;
};

struct RuntimeUiStandardWidgetProvenanceDesc {
    std::string feature_id;
    std::vector<RuntimeUiSourceReference> source_references;
    bool uses_external_engine_code{false};
    bool uses_external_engine_assets{false};
    bool uses_external_engine_public_names{false};
    bool uses_ui_middleware{false};
    bool third_party_notices_updated{false};
};

struct RuntimeUiStandardWidgetProvenancePlan {
    bool ready{false};
    std::size_t official_documentation_rows{0};
    std::size_t first_party_design_rows{0};
    std::size_t third_party_rows{0};
    std::vector<RuntimeUiProvenanceDiagnostic> diagnostics;
};

[[nodiscard]] RuntimeUiStandardWidgetProvenancePlan review_runtime_ui_standard_widget_provenance(
    const RuntimeUiStandardWidgetProvenanceDesc& desc);

enum class RuntimeUiMeterKind : std::uint8_t {
    health,
    mana,
    stamina,
    experience,
    cooldown,
    custom,
};

enum class RuntimeUiMeterFillDirection : std::uint8_t {
    left_to_right,
    right_to_left,
    bottom_to_top,
    top_to_bottom,
};

struct RuntimeUiMeterDesc {
    std::string id;
    RuntimeUiMeterKind kind{RuntimeUiMeterKind::custom};
    std::string label;
    std::string localization_key;
    std::string accessibility_label;
    float value{0.0F};
    float maximum{1.0F};
    float warning_threshold{0.25F};
    RuntimeUiMeterFillDirection fill_direction{RuntimeUiMeterFillDirection::left_to_right};
    std::string track_token;
    std::string fill_token;
    std::string warning_token;
    bool visible{true};
};

struct RuntimeUiMeterRow {
    std::string id;
    RuntimeUiMeterKind kind{RuntimeUiMeterKind::custom};
    std::string label;
    std::string localization_key;
    std::string accessibility_label;
    float value{0.0F};
    float maximum{1.0F};
    float normalized_value{0.0F};
    RuntimeUiMeterFillDirection fill_direction{RuntimeUiMeterFillDirection::left_to_right};
    std::string track_token;
    std::string fill_token;
    std::string warning_token;
    bool warning{false};
    bool depleted{false};
    bool visible{true};
};

struct RuntimeUiMeterPlan {
    bool ready{false};
    std::vector<RuntimeUiMeterRow> rows;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] RuntimeUiMeterPlan plan_runtime_ui_meters(std::vector<RuntimeUiMeterDesc> meters);
[[nodiscard]] UiDocument build_runtime_ui_meter_document(const RuntimeUiMeterPlan& plan, Rect viewport);

enum class RuntimeUiMenuActionIntent : std::uint8_t {
    resume_game,
    pause_game,
    restart_session,
    open_screen,
    close_screen,
    custom,
};

struct RuntimeUiMenuActionDesc {
    std::string id;
    std::string label;
    std::string localization_key;
    RuntimeUiMenuActionIntent intent{RuntimeUiMenuActionIntent::custom};
    std::string target_screen_id;
    bool enabled{true};
};

struct RuntimeUiMenuScreenDesc {
    std::string id;
    std::string title_localization_key;
    std::string accessibility_label;
    std::vector<RuntimeUiMenuActionDesc> actions;
};

struct RuntimeUiMenuStackDesc {
    std::string id;
    std::string active_screen_id;
    std::vector<RuntimeUiMenuScreenDesc> screens;
};

struct RuntimeUiMenuStackPlan {
    bool ready{false};
    std::string active_screen_id;
    std::size_t screen_count{0};
    std::size_t action_count{0};
    std::vector<std::string> focus_order;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] RuntimeUiMenuStackPlan plan_runtime_ui_menu_stack(const RuntimeUiMenuStackDesc& desc);
[[nodiscard]] UiDocument build_runtime_ui_menu_stack_document(const RuntimeUiMenuStackDesc& desc, const RuntimeUiMenuStackPlan& plan, Rect viewport);

struct RuntimeUiStandardHudDesc {
    std::string id;
    Rect viewport;
    std::vector<RuntimeUiMeterDesc> meters;
    RuntimeUiMenuStackDesc menu_stack;
    RuntimeUiStandardWidgetProvenanceDesc provenance;
};

struct RuntimeUiStandardHudPlan {
    bool ready{false};
    RuntimeUiMeterPlan meter_plan;
    RuntimeUiMenuStackPlan menu_plan;
    RuntimeUiStandardWidgetProvenancePlan provenance_plan;
    UiDocument document;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] RuntimeUiStandardHudPlan plan_runtime_ui_standard_hud(const RuntimeUiStandardHudDesc& desc);

} // namespace mirakana::ui
```

## Implementation Tasks

- [x] Add provenance review red tests in `tests/unit/runtime_ui_standard_widgets_tests.cpp`.
  - Test `accepts_official_docs_and_first_party_design_only` with one Unity official docs row, one Unreal official docs row, one Godot official docs row, and one first-party design row. Expected result: `ready == true`, `official_documentation_rows == 3`, `first_party_design_rows == 1`, `third_party_rows == 0`, and no diagnostics.
  - Test `rejects_copied_external_expression` with `contains_copied_expression == true`. Expected diagnostic: `copied_external_expression`.
  - Test `rejects_external_engine_code_even_when_not_distributed` with `uses_external_engine_code == true`. Expected diagnostic: `third_party_code_without_notice`.
  - Test `rejects_external_engine_assets` with `uses_external_engine_assets == true`. Expected diagnostic: `third_party_asset_without_notice`.
  - Test `rejects_external_engine_public_names` with `uses_external_engine_public_names == true` and source names containing `UMG`, `Slate`, `uGUI`, `VisualElement`, `Control`, and `CanvasLayer`. Expected diagnostic: `external_engine_public_name`.
  - Test `rejects_ui_middleware_reference` with `uses_ui_middleware == true`. Expected diagnostic: `unsupported_ui_middleware_reference`.

- [x] Implement provenance review.
  - Add `runtime_ui_standard_widgets.hpp` and `runtime_ui_standard_widgets.cpp`.
  - Treat `official_documentation` and `first_party_design` rows as allowed when `contains_copied_expression == false`.
  - Treat any `third_party_code`, `third_party_asset`, `marketplace_asset`, or `ai_generated_asset` row as blocking unless this plan is amended with explicit dependency, asset, and notice changes.
  - Add a private helper in the `.cpp` file named `contains_external_engine_public_token(std::string_view value)` with exact token matches for `uGUI`, `UMG`, `Slate`, `Widget Blueprint`, `Control`, `CanvasLayer`, `UIDocument`, and `VisualElement`.
  - Wire the new source into `engine/ui/CMakeLists.txt`.
  - Add `MK_runtime_ui_standard_widgets_tests` in root `CMakeLists.txt` and link it with `MK_ui`.

- [x] Add a copy-avoidance static guard.
  - Extend the targeted static contract chapter for this feature so the implementation must keep these positive proof needles: `RuntimeUiStandardWidgetProvenanceDesc`, `review_runtime_ui_standard_widget_provenance`, `runtime_ui_standard_widgets_external_engine_code=0`, `runtime_ui_standard_widgets_external_engine_assets=0`, and `runtime_ui_standard_widgets_ui_middleware=0`.
  - Keep external-engine UI tokens only in the private deny-list helper, tests that prove fail-closed behavior, and documentation that explains the prohibition. Do not allow them in public API type names, sample feature IDs, package counters, manifest capability IDs, or generated game guidance as supported terms.
  - Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` after the static guard update. Expected: PASS.

- [x] Add meter planning tests.
  - Test `plans_health_mana_stamina_meters` with three rows:
    - `health`: `value=75.0F`, `maximum=100.0F`, expected `normalized_value=0.75F`, `warning=false`, `depleted=false`.
    - `mana`: `value=40.0F`, `maximum=100.0F`, expected `normalized_value=0.40F`, `warning=false`, `depleted=false`.
    - `stamina`: `value=10.0F`, `maximum=100.0F`, `warning_threshold=0.25F`, expected `normalized_value=0.10F`, `warning=true`, `depleted=false`.
  - Test `marks_depleted_meter` with `value=0.0F`, expected `normalized_value=0.0F`, `depleted=true`, `warning=true`.
  - Test `rejects_invalid_meter_maximum` with `maximum=0.0F`, expected `ready=false`, no row for that id, and diagnostic `invalid_meter_maximum`.
  - Test `rejects_duplicate_meter_ids`, expected `ready=false` and diagnostic `duplicate_meter_id`.
  - Test `builds_meter_document_with_accessibility_labels`, expected a root element, one meter panel per row, a text label per row, and non-empty accessibility labels.

- [x] Implement meter planning.
  - Add `SemanticRole::meter` to `engine/ui/include/mirakana/ui/ui.hpp` and `semantic_role_id`.
  - Keep meter layout first-party: root panel, meter panel, track element, fill element, label element.
  - Use `background_token`, `foreground_token`, and first-party `track_token` / `fill_token` strings supplied by the caller.
  - Do not add default colors derived from external engines.
  - Reject non-finite `value`, `maximum`, or `warning_threshold`.
  - Clamp visible row `normalized_value` to `[0.0F, 1.0F]` after validation.

- [x] Add menu stack tests.
  - Test `plans_pause_menu_stack` with two screens: `pause_menu` and `settings_menu`.
  - Expected active screen: `pause_menu`.
  - Expected action count: `resume_game`, `open_settings`, `restart_session`, and `close_settings`.
  - Expected focus order: enabled actions from the active screen in input order.
  - Test `rejects_unknown_active_screen`, expected `ready=false` and diagnostic `unknown_active_screen`.
  - Test `rejects_duplicate_screen_ids`, expected diagnostic `duplicate_screen_id`.
  - Test `rejects_duplicate_action_ids_per_screen`, expected diagnostic `duplicate_action_id`.
  - Test `rejects_unknown_action_target_screen`, expected diagnostic `unknown_target_screen`.
  - Test `rejects_external_engine_public_tokens_in_ids`, expected diagnostic `external_engine_public_name`.

- [x] Implement menu stack planning.
  - Keep menu stack as deterministic value data and `UiDocument` composition.
  - Use existing `SemanticRole::dialog`, `SemanticRole::panel`, `SemanticRole::button`, and `SemanticRole::label`.
  - Do not create Blueprint-like graph concepts, Godot node-scene concepts, Unity UXML concepts, or external engine slot terminology.
  - Preserve caller-owned command dispatch; this feature only plans action intents and focus order.

- [x] Add standard HUD composition tests.
  - Test `standard_hud_requires_clean_provenance` where provenance is dirty. Expected `ready=false`, no promoted document ready claim.
  - Test `standard_hud_composes_meters_and_menu_stack` with clean provenance, three meters, and pause menu. Expected `ready=true`, meter rows `3`, menu screens `2`, menu actions `4`, and document size greater than meter-only document size.
  - Test `standard_hud_does_not_report_ready_with_invalid_meter`, expected `ready=false` and diagnostic propagated from `meter_plan`.
  - Test `standard_hud_does_not_report_ready_with_invalid_menu`, expected `ready=false` and diagnostic propagated from `menu_plan`.

- [x] Implement standard HUD composition.
  - Call `review_runtime_ui_standard_widget_provenance`, `plan_runtime_ui_meters`, and `plan_runtime_ui_menu_stack`.
  - Set `RuntimeUiStandardHudPlan::ready` only when all three subplans are ready.
  - Compose one `UiDocument` rooted at `runtime_ui.standard_hud.<id>.root`.
  - Keep the function side-effect-free and dependency-free.

- [x] Add sample package smoke lane.
  - Add `--require-runtime-ui-standard-widgets` to `sample_2d_desktop_runtime_package`.
  - Add the same option to `sample_generated_desktop_runtime_3d_package`.
  - Emit exact counters:
    - `runtime_ui_standard_widgets_ready=1`
    - `runtime_ui_standard_widgets_provenance_ready=1`
    - `runtime_ui_standard_widgets_meter_rows=3`
    - `runtime_ui_meter_health_normalized=0.750`
    - `runtime_ui_meter_mana_normalized=0.400`
    - `runtime_ui_meter_stamina_normalized=0.100`
    - `runtime_ui_standard_widgets_menu_screens=2`
    - `runtime_ui_standard_widgets_menu_actions=4`
    - `runtime_ui_standard_widgets_document_elements=19`
    - `runtime_ui_standard_widgets_health_accessibility_ready=1`
    - `runtime_ui_standard_widgets_mana_localization_ready=1`
    - `runtime_ui_standard_widgets_stamina_warning_style_ready=1`
    - `runtime_ui_standard_widgets_diagnostics=0`
    - `runtime_ui_standard_widgets_external_engine_code=0`
    - `runtime_ui_standard_widgets_external_engine_assets=0`
    - `runtime_ui_standard_widgets_ui_middleware=0`

- [x] Update docs, manifest, and static checks.
  - In `docs/ui.md`, add the supported contract: first-party runtime HUD meters, menu stack planning, accessibility/localization metadata, and `UiDocument` composition.
  - In `docs/ui.md`, add unsupported boundaries: no UI editor, no visual designer, no third-party UI middleware, no external engine compatibility layer, no copied themes/assets, no live renderer backend execution claim beyond existing renderer submission.
  - In `docs/current-capabilities.md`, add the new ready claim only after tests and samples pass.
  - In `docs/roadmap.md`, move this item from candidate to implemented only after validation evidence is recorded.
  - In manifest fragments, add a machine-readable row with `first_party_runtime_ui_standard_widgets_v1`, `runtime_ui_standard_widgets_ready`, `runtime_ui_standard_widgets_external_engine_code`, `runtime_ui_standard_widgets_external_engine_assets`, and `runtime_ui_standard_widgets_ui_middleware`.
  - Extend static checks so manifest and package counters cannot drift from this plan.
  - Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

- [x] Run focused validation.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_standard_widgets_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_ui_standard_widgets_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package`
  - Run the two sample executables with `--require-runtime-ui-standard-widgets` from their configured output paths and verify every required counter.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package -SmokeArgs @('--smoke','--require-config','runtime/sample_2d_desktop_runtime_package.config','--require-scene-package','runtime/sample_2d_desktop_runtime_package.geindex','--require-runtime-ui-standard-widgets')"`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -Command "& .\tools\package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs @('--smoke','--require-config','runtime/sample_generated_desktop_runtime_3d_package.config','--require-scene-package','runtime/sample_generated_desktop_runtime_3d_package.geindex','--require-runtime-ui-standard-widgets')"`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/ui/src/runtime_ui_standard_widgets.cpp,tests/unit/runtime_ui_standard_widgets_tests.cpp`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files games/sample_2d_desktop_runtime_package/main.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

## Completion Evidence Required

The implementation is complete only when all items are true:

- `MK_runtime_ui_standard_widgets_tests` passes.
- Both sample package smoke lanes emit `runtime_ui_standard_widgets_ready=1`.
- `runtime_ui_standard_widgets_external_engine_code=0`, `runtime_ui_standard_widgets_external_engine_assets=0`, and `runtime_ui_standard_widgets_ui_middleware=0` are emitted by both smoke lanes.
- Provenance review reports `runtime_ui_standard_widgets_official_documentation_rows=3`, `runtime_ui_standard_widgets_first_party_design_rows=1`, `runtime_ui_standard_widgets_diagnostics=0`, and no third-party rows.
- Static checks prove external engine UI tokens are fail-closed review terms only, not supported public API or compatibility terms.
- `tools/check-dependency-policy.ps1` passes with no UI dependency changes.
- `THIRD_PARTY_NOTICES.md` remains unchanged or contains a reviewed notice for any explicitly approved external material.
- `docs/ui.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, and manifest fragments describe the same supported and unsupported boundaries.
- `tools/check-ai-integration.ps1` passes after manifest composition.
- `tools/validate.ps1` passes, or a concrete host/tool blocker is recorded with the exact failing command.

## Official Research Sources

- Unity Manual: [Get started with runtime UI](https://docs.unity3d.com/6000.0/Documentation/Manual/UIE-get-started-with-runtime-ui.html)
- Unity Manual: [Comparison of UI systems in Unity](https://docs.unity3d.com/6000.0/Documentation/Manual/UI-system-compare.html)
- Unity Legal: [Unity Terms of Service](https://unity.com/legal/terms-of-service)
- Unity Legal: [Unity Editor Software Terms](https://unity.com/legal/editor-terms-of-service/software)
- Unity Legal: [Unity Editor Source Code Terms](https://unity.com/legal/editor-source-code-terms)
- Unreal Engine Documentation: [Building Your UI in Unreal Engine](https://dev.epicgames.com/documentation/en-us/unreal-engine/building-your-ui-in-unreal-engine)
- Unreal Engine Documentation: [Slate UI Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-user-interface-programming-framework-for-unreal-engine)
- Unreal Engine Legal: [Unreal Engine EULA](https://www.unrealengine.com/eula/unreal)
- Epic Legal: [Epic Content License Agreement](https://www.unrealengine.com/eula/content)
- Godot Documentation: [User interface nodes](https://docs.godotengine.org/en/stable/tutorials/ui/index.html)
- Godot Documentation: [ProgressBar](https://docs.godotengine.org/en/stable/classes/class_progressbar.html)
- Godot Legal: [Godot license](https://godotengine.org/license/)
- Godot Documentation: [Complying with licenses](https://docs.godotengine.org/en/stable/about/complying_with_licenses.html)
- U.S. Copyright Office: [Copyright in General FAQ](https://www.copyright.gov/help/faq/faq-general.html)
- U.S. Copyright Office: [What is Copyright?](https://www.copyright.gov/what-is-copyright/)
