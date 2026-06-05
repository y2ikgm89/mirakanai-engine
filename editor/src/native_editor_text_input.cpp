// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_text_input.hpp"

#include <algorithm>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] ui::AdapterPayloadDiagnostic make_diagnostic(ui::ElementId id, ui::AdapterPayloadDiagnosticCode code,
                                                           std::string message) {
    return ui::AdapterPayloadDiagnostic{.id = std::move(id), .code = code, .message = std::move(message)};
}

void append_diagnostics(std::vector<ui::AdapterPayloadDiagnostic>& target,
                        const std::vector<ui::AdapterPayloadDiagnostic>& source) {
    target.insert(target.end(), source.begin(), source.end());
}

[[nodiscard]] bool is_utf8_continuation(unsigned char value) noexcept {
    return (value & 0xC0U) == 0x80U;
}

[[nodiscard]] std::size_t next_scalar_boundary(std::string_view text, std::size_t offset) noexcept {
    if (offset >= text.size()) {
        return text.size();
    }
    std::size_t next = offset + 1U;
    while (next < text.size() && is_utf8_continuation(static_cast<unsigned char>(text[next]))) {
        ++next;
    }
    return next;
}

[[nodiscard]] std::vector<ui::TextBoundaryEvidence> make_scalar_grapheme_boundary_evidence(std::string_view text) {
    std::vector<ui::TextBoundaryEvidence> boundaries;
    for (std::size_t offset = 0U; offset < text.size();) {
        const auto next = next_scalar_boundary(text, offset);
        boundaries.push_back(ui::TextBoundaryEvidence{
            .kind = ui::TextBoundaryEvidenceKind::grapheme_cluster,
            .start_byte = offset,
            .end_byte = next,
        });
        offset = next;
    }
    return boundaries;
}

} // namespace

bool NativeEditorTextInputFocusPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool NativeEditorTextInputFocusResult::succeeded() const noexcept {
    return accepted && diagnostics.empty();
}

bool NativeEditorTextInputEndResult::succeeded() const noexcept {
    return accepted && diagnostics.empty();
}

bool NativeEditorImeCompositionResult::succeeded() const noexcept {
    return accepted && published && diagnostics.empty();
}

bool NativeEditorTextInputCommitResult::succeeded() const noexcept {
    return accepted && committed && diagnostics.empty();
}

NativeEditorTextInputTargetDesc make_native_editor_project_name_text_input_target(std::string_view project_name) {
    std::string text{project_name};
    return NativeEditorTextInputTargetDesc{
        .edit_state =
            ui::TextEditState{
                .target = ui::ElementId{.value = "editor.panel.project_settings.name.text_field"},
                .text = text,
                .cursor_byte_offset = text.size(),
            },
        .caret_bounds = ui::Rect{.x = 16.0F, .y = 48.0F, .width = 320.0F, .height = 24.0F},
        .editable = true,
    };
}

NativeEditorTextInputState make_native_editor_text_input_state(const NativeEditorTextInputTargetDesc& target) {
    const auto platform_plan = ui::plan_platform_text_input_session(ui::PlatformTextInputRequest{
        .target = target.edit_state.target,
        .text_bounds = target.caret_bounds,
        .surrounding_text = target.edit_state.text,
        .cursor_byte_offset = target.edit_state.cursor_byte_offset,
        .selection_byte_length = target.edit_state.selection_byte_length,
    });
    const bool ready = target.editable && platform_plan.ready();

    return NativeEditorTextInputState{
        .edit_state = target.edit_state,
        .caret_bounds = target.caret_bounds,
        .composition = ui::ImeComposition{.target = target.edit_state.target},
        .surrounding_text = target.edit_state.text,
        .target_registered = ready,
        .session_active = false,
        .composition_active = false,
        .commit_applied = false,
        .caret_rect_ready = ready,
        .surrounding_text_ready = ready,
        .candidate_ui_host_owned = true,
        .native_handles_exposed = false,
        .parity_evidence = make_native_editor_text_input_parity_evidence(target.edit_state, target.caret_bounds),
    };
}

ui::TextInputParityEvidenceSummary make_native_editor_text_input_parity_evidence(const ui::TextEditState& edit_state,
                                                                                 ui::Rect caret_bounds) {
    const std::string selected_candidate = edit_state.text.empty() ? std::string{"MIRAIKANAI"} : edit_state.text;
    const std::size_t range_length = edit_state.text.empty() ? 0U : edit_state.text.size();
    const auto request = ui::TextInputParityEvidenceRequest{
        .edit_state = edit_state,
        .grapheme_boundaries = make_scalar_grapheme_boundary_evidence(edit_state.text),
        .composition =
            ui::ImeComposition{
                .target = edit_state.target,
                .composition_text = selected_candidate,
                .cursor_index = selected_candidate.size(),
            },
        .composition_start_byte_offset = 0U,
        .composition_byte_length = range_length,
        .committed_text =
            ui::CommittedTextInput{
                .target = edit_state.target,
                .text = selected_candidate,
            },
        .platform_request =
            ui::PlatformTextInputRequest{
                .target = edit_state.target,
                .text_bounds = caret_bounds,
                .surrounding_text = edit_state.text,
                .cursor_byte_offset = edit_state.cursor_byte_offset,
                .selection_byte_length = edit_state.selection_byte_length,
            },
        .candidate_selection =
            ui::TextInputCandidateSelection{
                .target = edit_state.target,
                .candidates = {selected_candidate},
                .selected_index = 0U,
                .candidate_ui_host_owned = true,
            },
        .reconversion_request =
            ui::TextInputReconversionRequest{
                .target = edit_state.target,
                .start_byte_offset = 0U,
                .byte_length = range_length,
            },
        .caret_rect = caret_bounds,
        .requested_native_handle_access = false,
    };
    return ui::plan_text_input_parity_evidence(request);
}

NativeEditorTextInputFocusPlan
plan_native_editor_text_input_focus_change(const NativeEditorTextInputState& current,
                                           const NativeEditorTextInputTargetDesc& target) {
    NativeEditorTextInputFocusPlan plan;
    plan.request = ui::PlatformTextInputRequest{
        .target = target.edit_state.target,
        .text_bounds = target.caret_bounds,
        .surrounding_text = target.edit_state.text,
        .cursor_byte_offset = target.edit_state.cursor_byte_offset,
        .selection_byte_length = target.edit_state.selection_byte_length,
    };
    plan.previous_target = current.edit_state.target;

    if (!target.editable) {
        plan.diagnostics.push_back(make_diagnostic(
            target.edit_state.target, ui::AdapterPayloadDiagnosticCode::invalid_platform_text_input_target,
            "native editor text input target must be editable before starting platform text input"));
    }
    if (target.edit_state.cursor_byte_offset > target.edit_state.text.size()) {
        plan.diagnostics.push_back(make_diagnostic(target.edit_state.target,
                                                   ui::AdapterPayloadDiagnosticCode::invalid_text_edit_cursor,
                                                   "native editor text input cursor must be within the target text"));
    }
    if (target.edit_state.cursor_byte_offset <= target.edit_state.text.size() &&
        target.edit_state.selection_byte_length >
            target.edit_state.text.size() - target.edit_state.cursor_byte_offset) {
        plan.diagnostics.push_back(
            make_diagnostic(target.edit_state.target, ui::AdapterPayloadDiagnosticCode::invalid_text_edit_selection,
                            "native editor text input selection must fit within the target text"));
    }

    const auto platform_plan = ui::plan_platform_text_input_session(plan.request);
    append_diagnostics(plan.diagnostics, platform_plan.diagnostics);

    if (plan.ready()) {
        plan.begin_session = !current.session_active || current.edit_state.target != target.edit_state.target;
        plan.end_previous_session = current.session_active && current.edit_state.target != target.edit_state.target;
    }
    return plan;
}

std::string native_editor_text_input_status(const NativeEditorTextInputState& state) {
    if (!state.target_registered) {
        return "value_text_input_not_started";
    }
    if (state.composition_active) {
        return "value_ime_composition_active";
    }
    if (state.commit_applied) {
        return "value_text_input_commit_applied";
    }
    if (state.session_active) {
        return state.tsf_adapter_selected ? "win32_tsf_session_active" : "value_text_input_session_active";
    }
    return state.tsf_adapter_selected ? "win32_tsf_selected" : "value_text_input_controller_ready";
}

} // namespace mirakana::editor
