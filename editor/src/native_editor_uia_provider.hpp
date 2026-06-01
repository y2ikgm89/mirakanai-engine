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
    std::vector<std::int32_t> runtime_id;
    std::vector<std::string> actions;
};

struct NativeEditorUiaProviderState {
    std::string service_id{"win32_uia"};
    std::string status_id{"uia_provider_not_published"};
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
