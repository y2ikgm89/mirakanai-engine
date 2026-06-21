// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

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

[[nodiscard]] RuntimeUiStandardWidgetProvenancePlan
review_runtime_ui_standard_widget_provenance(const RuntimeUiStandardWidgetProvenanceDesc& desc);

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
[[nodiscard]] UiDocument build_runtime_ui_menu_stack_document(const RuntimeUiMenuStackDesc& desc,
                                                              const RuntimeUiMenuStackPlan& plan, Rect viewport);

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
