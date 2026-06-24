// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/runtime_ui_platform_production.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::win32 {

enum class Win32UiaProviderPublicationDiagnosticCode : std::uint8_t {
    missing_provider_root_id,
    missing_runtime_id,
    duplicate_runtime_id,
    missing_accessible_name,
    invalid_bounds,
    focusable_without_action_pattern,
    child_without_parent,
    unsupported_pattern_claim,
    event_claim_without_provider_root,
    public_native_handles_exposed,
    broad_accessibility_parity_claim,
    row_budget_exceeded,
    uia_provider_unavailable,
};

struct Win32UiaProviderPublicationDiagnostic {
    Win32UiaProviderPublicationDiagnosticCode code{Win32UiaProviderPublicationDiagnosticCode::missing_provider_root_id};
    ui::ElementId runtime_id;
    std::string message;
};

struct Win32UiaRuntimeNodeRow {
    ui::ElementId runtime_id;
    ui::ElementId parent_runtime_id;
    std::vector<ui::ElementId> child_runtime_ids;
    ui::SemanticRole role{ui::SemanticRole::none};
    std::string name;
    std::string description;
    ui::Rect screen_bounds;
    bool enabled{true};
    bool visible{true};
    bool focusable{false};
    bool focused{false};
    std::vector<std::string> action_ids;
    bool invoke_pattern_supported{false};
    bool unsupported_pattern_claim{false};
    std::size_t reading_order{0U};
    std::string live_region_status;
    std::string keyboard_shortcut;
    bool event_publication_requested{false};
    bool event_publication_ready{false};
};

struct Win32UiaProviderPublicationDesc {
    ui::ElementId provider_root_id;
    std::vector<Win32UiaRuntimeNodeRow> nodes;
    bool publish_events{false};
    bool public_native_handles_exposed{false};
    bool claims_cross_platform_accessibility_ready{false};
    std::size_t row_budget{64U};
};

struct Win32UiaProviderPublicationResult {
    bool ready{false};
    ui::ElementId provider_root_id;
    bool uia_provider_root_available{false};
    std::vector<Win32UiaRuntimeNodeRow> node_rows;
    std::size_t role_rows{0U};
    std::size_t name_rows{0U};
    std::size_t description_rows{0U};
    std::size_t state_rows{0U};
    std::size_t focus_rows{0U};
    std::size_t action_rows{0U};
    std::size_t relationship_rows{0U};
    std::size_t reading_order_rows{0U};
    std::size_t live_region_rows{0U};
    std::size_t keyboard_pattern_rows{0U};
    std::size_t bounds_rows{0U};
    std::size_t event_publication_rows{0U};
    bool cross_platform_accessibility_ready{false};
    bool public_native_handles_exposed{false};
    std::vector<Win32UiaProviderPublicationDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] Win32UiaProviderPublicationResult
publish_runtime_ui_to_win32_uia(const Win32UiaProviderPublicationDesc& desc);
[[nodiscard]] ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_uia_accessibility_publication_production_evidence(const Win32UiaProviderPublicationResult& result);

} // namespace mirakana::win32
