// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/win32/win32_input.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <optional>
#include <string>

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

[[nodiscard]] Win32TextInputSessionPlan plan_win32_text_input_begin(const ui::PlatformTextInputRequest& request);
[[nodiscard]] Win32TextInputEndPlan plan_win32_text_input_end(const ui::ElementId& target);
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
