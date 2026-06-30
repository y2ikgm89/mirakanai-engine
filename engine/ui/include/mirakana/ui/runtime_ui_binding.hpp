// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::ui {

enum class RuntimeUiBindingValueType : std::uint8_t {
    text,
    boolean,
};

enum class RuntimeUiBindingTarget : std::uint8_t {
    text,
    enabled,
    visible,
};

enum class RuntimeUiBindingPlanStatus : std::uint8_t {
    invalid_request,
    ready,
    diagnostics,
};

enum class RuntimeUiBindingDiagnosticCode : std::uint8_t {
    missing_binding_id,
    duplicate_binding_id,
    missing_binding_key,
    type_mismatch,
    missing_element,
    missing_command_id,
    duplicate_command_id,
    disabled_command_invocation,
    missing_focus_scope,
    missing_navigation_target,
    navigation_cycle,
    modal_focus_escape,
    unknown_controller_glyph_ref,
    pointer_capture_conflict,
};

struct RuntimeUiBindingDiagnostic {
    RuntimeUiBindingDiagnosticCode code{RuntimeUiBindingDiagnosticCode::missing_binding_id};
    std::string subject;
    std::string message;
};

struct RuntimeUiBindingValueRow {
    std::string key;
    RuntimeUiBindingValueType type{RuntimeUiBindingValueType::text};
    std::string text;
    bool boolean{false};
};

struct RuntimeUiBindingRow {
    std::string id;
    ElementId element;
    std::string source_key;
    RuntimeUiBindingTarget target{RuntimeUiBindingTarget::text};
    RuntimeUiBindingValueType expected_type{RuntimeUiBindingValueType::text};
};

struct RuntimeUiCommandAvailabilityRow {
    std::string id;
    std::string command_id;
    ElementId element;
    bool available{true};
};

struct RuntimeUiFocusScope {
    std::string id;
    ElementId root;
    ElementId initial_focus;
    bool modal{false};
};

struct RuntimeUiNavigationEdge {
    std::string id;
    std::string scope_id;
    ElementId from;
    ElementId to;
    NavigationDirection direction{NavigationDirection::next};
};

struct RuntimeUiControllerGlyphRef {
    std::string id;
    ElementId element;
    std::string glyph_id;
    std::string input_source_id;
};

struct RuntimeUiPointerCaptureRow {
    std::string id;
    ElementId element;
    std::uint32_t pointer_id{0U};
};

struct RuntimeUiCommandInvocationRow {
    std::string id;
    std::string command_id;
    ElementId source_element;
};

struct RuntimeUiBindingDocument {
    UiDocument document;
    std::vector<RuntimeUiBindingValueRow> values;
    std::vector<RuntimeUiBindingRow> binding_rows;
    std::vector<RuntimeUiCommandAvailabilityRow> command_rows;
    std::vector<RuntimeUiFocusScope> focus_scopes;
    std::vector<RuntimeUiNavigationEdge> navigation_edges;
    std::vector<RuntimeUiControllerGlyphRef> controller_glyph_refs;
    std::vector<std::string> known_controller_glyph_ids;
    std::vector<RuntimeUiPointerCaptureRow> pointer_captures;
    std::vector<RuntimeUiCommandInvocationRow> command_invocations;
};

struct RuntimeUiInputRoutingPlan {
    RuntimeUiBindingPlanStatus status{RuntimeUiBindingPlanStatus::invalid_request};
    bool ready{false};
    bool input_routing_ready{false};
    std::size_t command_rows{0U};
    std::size_t focus_scopes{0U};
    std::size_t navigation_edges{0U};
    std::size_t controller_glyph_refs{0U};
    std::size_t pointer_captures{0U};
    std::size_t gameplay_commands_executed{0U};
    std::vector<RuntimeUiBindingDiagnostic> diagnostics;
};

struct RuntimeUiBindingPlan {
    RuntimeUiBindingPlanStatus status{RuntimeUiBindingPlanStatus::invalid_request};
    bool ready{false};
    bool input_routing_ready{false};
    UiDocument document;
    std::size_t binding_rows{0U};
    std::size_t command_rows{0U};
    std::size_t focus_scopes{0U};
    std::size_t navigation_edges{0U};
    std::size_t controller_glyph_refs{0U};
    std::size_t gameplay_commands_executed{0U};
    std::vector<RuntimeUiBindingDiagnostic> diagnostics;
};

[[nodiscard]] std::string_view runtime_ui_binding_value_type_name(RuntimeUiBindingValueType type) noexcept;
[[nodiscard]] std::string_view runtime_ui_binding_target_name(RuntimeUiBindingTarget target) noexcept;
[[nodiscard]] std::string_view runtime_ui_binding_plan_status_name(RuntimeUiBindingPlanStatus status) noexcept;
[[nodiscard]] RuntimeUiBindingPlan plan_runtime_ui_binding(RuntimeUiBindingDocument document);
[[nodiscard]] RuntimeUiInputRoutingPlan plan_runtime_ui_input_routing(const RuntimeUiBindingDocument& document);

} // namespace mirakana::ui
