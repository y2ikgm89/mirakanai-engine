// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/sdl3/sdl_window.hpp"
#include "mirakana/ui/ui.hpp"

#include <optional>

namespace mirakana {

[[nodiscard]] std::optional<ui::ImeComposition> sdl3_ime_composition_from_window_event(const ui::ElementId& target,
                                                                                       const SdlWindowEvent& event);

[[nodiscard]] ui::ImeCompositionPublishResult
publish_sdl3_ime_composition_event(ui::IImeAdapter& adapter, const ui::ElementId& target, const SdlWindowEvent& event);

[[nodiscard]] std::optional<ui::CommittedTextInput> sdl3_committed_text_from_window_event(const ui::ElementId& target,
                                                                                          const SdlWindowEvent& event);

[[nodiscard]] ui::TextEditCommitResult apply_sdl3_committed_text_event(const ui::TextEditState& state,
                                                                       const ui::ElementId& target,
                                                                       const SdlWindowEvent& event);

[[nodiscard]] std::optional<ui::TextEditCommand> sdl3_text_edit_command_from_window_event(const ui::ElementId& target,
                                                                                          const SdlWindowEvent& event);

[[nodiscard]] ui::TextEditCommandResult apply_sdl3_text_edit_command_event(const ui::TextEditState& state,
                                                                           const ui::ElementId& target,
                                                                           const SdlWindowEvent& event);

[[nodiscard]] std::optional<ui::TextEditClipboardCommand>
sdl3_text_edit_clipboard_command_from_window_event(const ui::ElementId& target, const SdlWindowEvent& event);

[[nodiscard]] ui::TextEditClipboardCommandResult
apply_sdl3_text_edit_clipboard_command_event(ui::IClipboardTextAdapter& adapter, const ui::TextEditState& state,
                                             const ui::ElementId& target, const SdlWindowEvent& event);

class SdlPlatformIntegrationAdapter final : public ui::IPlatformIntegrationAdapter {
  public:
    // SDL3 text input APIs are main-thread window operations.
    explicit SdlPlatformIntegrationAdapter(SdlWindow& window) noexcept;

    void begin_text_input(const ui::PlatformTextInputRequest& request) override;
    void end_text_input(const ui::ElementId& target) override;

  private:
    SdlWindow* window_{nullptr};
};

} // namespace mirakana
