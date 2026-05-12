// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/clipboard.hpp"
#include "mirakana/ui/ui.hpp"

#include <string>
#include <string_view>

namespace mirakana {

class SdlClipboard final : public IClipboard {
  public:
    [[nodiscard]] bool has_text() const override;
    [[nodiscard]] std::string text() const override;

    void set_text(std::string_view text) override;
    void clear() override;
};

class SdlClipboardTextAdapter final : public ui::IClipboardTextAdapter {
  public:
    // SDL3 clipboard APIs are main-thread platform operations.
    explicit SdlClipboardTextAdapter(SdlClipboard& clipboard) noexcept;

    void set_clipboard_text(std::string_view text) override;
    [[nodiscard]] bool has_clipboard_text() const override;
    [[nodiscard]] std::string clipboard_text() const override;

  private:
    SdlClipboard* clipboard_{nullptr};
};

} // namespace mirakana
