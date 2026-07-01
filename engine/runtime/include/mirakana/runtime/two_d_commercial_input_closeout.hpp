// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/session_services.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class Runtime2DCommercialInputOfficialSourceKind : std::uint8_t {
    microsoft_gameinput,
    microsoft_raw_input,
    microsoft_pointer_input,
    microsoft_uia,
    repository_legal_policy,
};

enum class Runtime2DCommercialInputCloseoutDiagnosticCode : std::uint8_t {
    action_map_not_ready,
    rebinding_not_ready,
    device_ux_not_ready,
    accessibility_navigation_not_ready,
    official_source_not_ready,
    selected_package_claim_missing,
    public_native_handles,
    input_middleware_claim,
    external_engine_compatibility_claim,
    cross_platform_parity_claim,
    legal_approval_claim,
};

struct Runtime2DCommercialInputOfficialSourceRow {
    std::string id;
    Runtime2DCommercialInputOfficialSourceKind kind{Runtime2DCommercialInputOfficialSourceKind::microsoft_gameinput};
    bool ready{false};
    bool official{false};
    bool public_docs_only{false};
};

struct Runtime2DCommercialInputNavigationRow {
    std::string id;
    std::string context;
    std::string action;
    std::string focus_scope_id;
    std::string keyboard_glyph_lookup_key;
    std::string controller_glyph_lookup_key;
    std::string accessibility_label;
    bool reachable_by_keyboard{false};
    bool reachable_by_controller{false};
    bool selected{false};
    bool ready{false};
};

struct Runtime2DCommercialInputCloseoutDiagnostic {
    Runtime2DCommercialInputCloseoutDiagnosticCode code{
        Runtime2DCommercialInputCloseoutDiagnosticCode::action_map_not_ready};
    std::string row_id;
    std::string message;
};

struct Runtime2DCommercialInputCloseoutDesc {
    RuntimeInputActionMap base_actions;
    RuntimeInputRebindingProfile rebinding_profile;
    RuntimeInputDeviceProductionUxPlan device_ux_plan;
    std::vector<Runtime2DCommercialInputNavigationRow> navigation_rows;
    std::array<Runtime2DCommercialInputOfficialSourceRow, 5U> official_source_rows{};
    bool selected_2d_package_ready_claim{false};
    bool public_native_handles{false};
    bool input_middleware_claim{false};
    bool external_engine_compatibility_claim{false};
    bool cross_platform_parity_claim{false};
    bool legal_approval_claim{false};
};

struct Runtime2DCommercialInputCloseoutResult {
    bool ready{false};
    bool action_map_ready{false};
    bool rebinding_ready{false};
    bool device_ux_ready{false};
    bool accessibility_navigation_ready{false};
    bool official_source_ready{false};
    std::vector<Runtime2DCommercialInputCloseoutDiagnostic> diagnostics;
    std::size_t action_binding_rows{0U};
    std::size_t axis_binding_rows{0U};
    std::size_t profile_overlay_rows{0U};
    std::size_t presentation_rows{0U};
    std::size_t presentation_glyph_lookup_keys{0U};
    std::size_t keyboard_mouse_device_rows{0U};
    std::size_t gamepad_device_rows{0U};
    std::size_t touch_gesture_rows{0U};
    std::size_t multiplayer_device_assignment_rows{0U};
    std::size_t per_device_profile_rows{0U};
    std::size_t glyph_asset_lookup_rows{0U};
    std::size_t keyboard_layout_label_rows{0U};
    std::size_t accessibility_navigation_rows{0U};
    std::size_t keyboard_accessible_rows{0U};
    std::size_t controller_accessible_rows{0U};
    std::size_t official_source_rows{0U};
    std::size_t native_handle_access_rows{0U};
    std::size_t input_middleware_rows{0U};
    std::size_t ui_rendering_rows{0U};
    std::size_t glyph_rendering_rows{0U};
    std::size_t ui_widget_rows{0U};
    std::size_t external_engine_claim_rows{0U};
    std::size_t cross_platform_parity_claim_rows{0U};
    std::size_t legal_approval_claim_rows{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::string_view
runtime_2d_commercial_input_official_source_kind_name(Runtime2DCommercialInputOfficialSourceKind kind) noexcept;
[[nodiscard]] Runtime2DCommercialInputCloseoutResult
evaluate_runtime_2d_commercial_input_closeout(const Runtime2DCommercialInputCloseoutDesc& desc);

} // namespace mirakana::runtime
