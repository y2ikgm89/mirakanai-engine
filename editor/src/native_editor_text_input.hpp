// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct NativeEditorTextInputTargetDesc {
    ui::TextEditState edit_state;
    ui::Rect caret_bounds;
    bool editable{true};
};

struct NativeEditorTextInputState {
    ui::TextEditState edit_state;
    ui::Rect caret_bounds;
    ui::ImeComposition composition;
    std::string surrounding_text;
    bool target_registered{false};
    bool session_active{false};
    bool composition_active{false};
    bool commit_applied{false};
    bool caret_rect_ready{false};
    bool surrounding_text_ready{false};
    bool candidate_ui_host_owned{true};
    bool tsf_adapter_selected{false};
    bool native_handles_exposed{false};
    ui::TextInputParityEvidenceSummary parity_evidence;
};

struct NativeEditorTextInputFocusPlan {
    ui::PlatformTextInputRequest request;
    ui::ElementId previous_target;
    bool begin_session{false};
    bool end_previous_session{false};
    std::vector<ui::AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool ready() const noexcept;
};

struct NativeEditorTextInputFocusResult {
    bool accepted{false};
    bool session_begun{false};
    bool previous_session_ended{false};
    std::vector<ui::AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct NativeEditorTextInputEndResult {
    bool accepted{false};
    bool session_ended{false};
    std::vector<ui::AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct NativeEditorImeCompositionResult {
    bool accepted{false};
    bool published{false};
    std::vector<ui::AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct NativeEditorTextInputCommitResult {
    bool accepted{false};
    bool committed{false};
    ui::TextEditState state;
    std::vector<ui::AdapterPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] NativeEditorTextInputTargetDesc
make_native_editor_project_name_text_input_target(std::string_view project_name);

[[nodiscard]] NativeEditorTextInputState
make_native_editor_text_input_state(const NativeEditorTextInputTargetDesc& target);

[[nodiscard]] ui::TextInputParityEvidenceSummary
make_native_editor_text_input_parity_evidence(const ui::TextEditState& edit_state, ui::Rect caret_bounds);

[[nodiscard]] NativeEditorTextInputFocusPlan
plan_native_editor_text_input_focus_change(const NativeEditorTextInputState& current,
                                           const NativeEditorTextInputTargetDesc& target);

[[nodiscard]] std::string native_editor_text_input_status(const NativeEditorTextInputState& state);

} // namespace mirakana::editor
