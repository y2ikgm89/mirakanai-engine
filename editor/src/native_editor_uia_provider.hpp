// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace mirakana::editor {

struct NativeEditorUiaNode {
    ui::ElementId id;
    ui::ElementId parent;
    ui::ElementId previous_sibling;
    ui::ElementId next_sibling;
    ui::ElementId first_child;
    ui::ElementId last_child;
    ui::SemanticRole role{ui::SemanticRole::none};
    std::string name;
    std::string control_type_id;
    ui::Rect bounds;
    bool enabled{true};
    bool keyboard_focusable{false};
    bool has_keyboard_focus{false};
    bool control_element{true};
    bool content_element{true};
    bool live_region{false};
    std::vector<std::int32_t> runtime_id;
    std::vector<std::string> actions;
    std::vector<std::string> control_patterns;
    std::vector<std::string> event_ids;
};

struct NativeEditorUiaProviderState {
    std::string service_id{"win32_uia"};
    std::string status_id{"uia_provider_not_published"};
    std::string parity_status_id{"not_ready"};
    std::vector<NativeEditorUiaNode> nodes;
    std::vector<ui::AdapterPayloadDiagnostic> diagnostics;
    std::uint32_t role_rows{0};
    std::uint32_t name_rows{0};
    std::uint32_t state_rows{0};
    std::uint32_t focus_rows{0};
    std::uint32_t action_rows{0};
    std::uint32_t relationship_rows{0};
    std::uint32_t tree_navigation_rows{0};
    std::uint32_t missing_name_diagnostics{0};
    std::uint32_t missing_role_diagnostics{0};
    std::uint32_t invalid_bounds_diagnostics{0};
    std::uint32_t hidden_nodes{0};
    std::uint32_t unsupported_pattern_diagnostics{0};
    std::uint32_t live_region_rows{0};
    std::uint32_t automation_id_rows{0};
    std::uint32_t runtime_id_opaque_rows{0};
    std::uint32_t name_property_rows{0};
    std::uint32_t control_type_rows{0};
    std::uint32_t invoke_pattern_rows{0};
    std::uint32_t value_pattern_rows{0};
    std::uint32_t selection_pattern_rows{0};
    std::uint32_t text_pattern_rows{0};
    std::uint32_t text_edit_pattern_rows{0};
    std::uint32_t scroll_pattern_rows{0};
    std::uint32_t window_pattern_rows{0};
    std::uint32_t toggle_pattern_rows{0};
    std::uint32_t uia_pattern_rows{0};
    std::uint32_t focus_event_rows{0};
    std::uint32_t property_change_event_rows{0};
    std::uint32_t text_edit_event_rows{0};
    std::uint32_t selection_change_event_rows{0};
    std::uint32_t structure_change_event_rows{0};
    std::uint32_t window_event_rows{0};
    std::uint32_t live_region_event_rows{0};
    std::uint32_t uia_event_rows{0};
    std::string macos_status{"host_gated"};
    std::string linux_at_spi_status{"host_gated"};
    std::string android_status{"host_gated"};
    std::string ios_status{"host_gated"};
    bool windows_uia_patterns_ready{false};
    bool windows_uia_events_ready{false};
    bool provider_available{false};
    bool native_handles_exposed{false};
};

struct NativeEditorUiaScreenOrigin {
    float x{0.0F};
    float y{0.0F};
};

class NativeEditorUiaAccessibilityAdapter final : public ui::IAccessibilityAdapter {
  public:
    NativeEditorUiaAccessibilityAdapter();
    explicit NativeEditorUiaAccessibilityAdapter(std::uintptr_t owner_window_token);
    ~NativeEditorUiaAccessibilityAdapter() override;

    NativeEditorUiaAccessibilityAdapter(const NativeEditorUiaAccessibilityAdapter&) = delete;
    NativeEditorUiaAccessibilityAdapter& operator=(const NativeEditorUiaAccessibilityAdapter&) = delete;
    NativeEditorUiaAccessibilityAdapter(NativeEditorUiaAccessibilityAdapter&&) noexcept;
    NativeEditorUiaAccessibilityAdapter& operator=(NativeEditorUiaAccessibilityAdapter&&) noexcept;

    void set_focused_element(ui::ElementId focused);
    void publish_nodes(const std::vector<ui::AccessibilityNode>& nodes) override;

    [[nodiscard]] const NativeEditorUiaProviderState& state() const noexcept;
    [[nodiscard]] bool native_handles_exposed() const noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] NativeEditorUiaProviderState
plan_native_editor_uia_provider_tree(const ui::AccessibilityPayload& payload, const ui::ElementId& focused,
                                     NativeEditorUiaScreenOrigin screen_origin = {});

[[nodiscard]] std::unique_ptr<NativeEditorUiaAccessibilityAdapter> make_native_editor_uia_accessibility_adapter();

[[nodiscard]] std::unique_ptr<NativeEditorUiaAccessibilityAdapter>
make_native_editor_win32_uia_accessibility_adapter(std::uintptr_t owner_window_token);

[[nodiscard]] bool native_editor_uia_provider_exposes_native_handles() noexcept;

} // namespace mirakana::editor
