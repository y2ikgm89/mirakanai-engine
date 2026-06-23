// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/win32/win32_input.hpp"
#include "mirakana/ui/runtime_ui_platform_production.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::win32 {

enum class Win32TextInputMessageId : std::uint8_t {
    unknown = 0,
    char_input,
    ime_composition,
    ime_end_composition,
};

struct Win32CopiedTextInputMessage {
    Win32TextInputMessageId message{Win32TextInputMessageId::unknown};
    std::u16string utf16_text;
    std::uintptr_t window_token{0};
};

struct Win32TextInputSessionPlan {
    ui::PlatformTextInputRequest request;
    bool start_session{false};
    bool update_composition_window{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct Win32TextInputEndPlan {
    ui::ElementId target;
    bool end_session{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

enum class Win32TsfTextSessionDiagnosticCode : std::uint8_t {
    missing_active_target,
    invalid_caret_rect,
    invalid_surrounding_text,
    composition_update_without_begin,
    committed_text_target_mismatch,
    candidate_rows_without_session,
    public_native_handles_exposed,
    broad_ime_parity_claim,
    row_budget_exceeded,
    tsf_thread_manager_unavailable,
    tsf_document_manager_unavailable,
    tsf_context_unavailable,
};

struct Win32TsfTextSessionDiagnostic {
    Win32TsfTextSessionDiagnosticCode code{Win32TsfTextSessionDiagnosticCode::missing_active_target};
    std::string message;
};

struct Win32TsfCompositionRow {
    ui::ElementId target;
    std::string composition_text;
    std::size_t cursor_byte_offset{0U};
    bool begin{false};
    bool update{false};
    bool end{false};
};

struct Win32TsfCandidateIntentRow {
    ui::ElementId target;
    std::size_t candidate_count{0U};
    std::size_t selected_index{0U};
    bool native_candidate_ui_requested{false};
    bool native_candidate_ui_ready{false};
};

struct Win32TsfTextAreaRow {
    ui::ElementId target;
    ui::Rect caret_rect;
    ui::Rect text_bounds;
    std::string surrounding_text;
    std::size_t cursor_byte_offset{0U};
};

struct Win32TsfTextSessionDesc {
    ui::PlatformTextInputRequest active_request;
    bool request_tsf_session{true};
    bool composition_begin{false};
    std::string composition_text;
    std::size_t composition_cursor_byte_offset{0U};
    bool composition_end{false};
    std::vector<ui::CommittedTextInput> committed_text_rows;
    std::vector<Win32TsfCandidateIntentRow> candidate_intent_rows;
    bool public_native_handles_exposed{false};
    bool claims_cross_platform_ime_ready{false};
    std::size_t row_budget{32U};
};

struct Win32TsfTextSessionResult {
    bool ready{false};
    bool tsf_thread_manager_available{false};
    bool tsf_document_manager_available{false};
    bool tsf_context_available{false};
    std::size_t focus_sink_rows{0U};
    std::size_t text_store_lock_rows{0U};
    bool native_candidate_ui_ready{false};
    bool cross_platform_ime_ready{false};
    bool public_native_handles_exposed{false};
    std::vector<Win32TsfCompositionRow> composition_rows;
    std::vector<ui::CommittedTextInput> committed_text_rows;
    std::vector<Win32TsfCandidateIntentRow> candidate_intent_rows;
    std::vector<Win32TsfTextAreaRow> text_area_rows;
    std::vector<Win32TsfTextSessionDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] Win32TextInputSessionPlan plan_win32_text_input_begin(const ui::PlatformTextInputRequest& request);
[[nodiscard]] Win32TextInputEndPlan plan_win32_text_input_end(const ui::ElementId& target);
[[nodiscard]] Win32TsfTextSessionResult plan_win32_tsf_text_session(const Win32TsfTextSessionDesc& desc);
[[nodiscard]] ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_tsf_native_ime_production_evidence(const Win32TsfTextSessionResult& result);
[[nodiscard]] std::optional<ui::CommittedTextInput>
win32_committed_text_from_message(const ui::ElementId& target, const Win32CopiedTextInputMessage& message);
[[nodiscard]] ui::TextEditCommitResult apply_win32_committed_text_message(const ui::TextEditState& state,
                                                                          const ui::ElementId& target,
                                                                          const Win32CopiedTextInputMessage& message);
[[nodiscard]] std::optional<ui::TextEditCommand> win32_text_edit_command_from_input_event(const ui::ElementId& target,
                                                                                          const Win32InputEvent& event);
[[nodiscard]] ui::TextEditCommandResult apply_win32_text_edit_command_event(const ui::TextEditState& state,
                                                                            const ui::ElementId& target,
                                                                            const Win32InputEvent& event);
[[nodiscard]] std::optional<ui::TextEditClipboardCommand>
win32_text_edit_clipboard_command_from_input_event(const ui::ElementId& target, const Win32InputEvent& event);
[[nodiscard]] ui::TextEditClipboardCommandResult
apply_win32_text_edit_clipboard_command_event(ui::IClipboardTextAdapter& adapter, const ui::TextEditState& state,
                                              const ui::ElementId& target, const Win32InputEvent& event);

class Win32PlatformIntegrationAdapter final : public ui::IPlatformIntegrationAdapter {
  public:
    void begin_text_input(const ui::PlatformTextInputRequest& request) override;
    void end_text_input(const ui::ElementId& target) override;

    [[nodiscard]] const std::optional<ui::PlatformTextInputRequest>& active_request() const noexcept;

  private:
    std::optional<ui::PlatformTextInputRequest> active_request_;
};

} // namespace mirakana::win32
