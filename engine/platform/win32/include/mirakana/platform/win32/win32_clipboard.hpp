// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/clipboard.hpp"
#include "mirakana/ui/ui.hpp"

#include <string>
#include <string_view>

namespace mirakana::win32 {

struct Win32ClipboardTextWritePlan {
    std::u16string utf16_text;
    bool open_clipboard{false};
    bool empty_clipboard{false};
    bool set_unicode_text{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct Win32ClipboardTextReadPlan {
    bool open_clipboard{true};
    bool request_unicode_text{true};
};

[[nodiscard]] Win32ClipboardTextWritePlan plan_win32_clipboard_write(std::string_view text);
[[nodiscard]] Win32ClipboardTextReadPlan plan_win32_clipboard_read() noexcept;

class Win32Clipboard final : public IClipboard {
  public:
    [[nodiscard]] bool has_text() const override;
    [[nodiscard]] std::string text() const override;

    void set_text(std::string_view text) override;
    void clear() override;
};

class Win32ClipboardTextAdapter final : public ui::IClipboardTextAdapter {
  public:
    explicit Win32ClipboardTextAdapter(Win32Clipboard& clipboard) noexcept;

    void set_clipboard_text(std::string_view text) override;
    [[nodiscard]] bool has_clipboard_text() const override;
    [[nodiscard]] std::string clipboard_text() const override;

  private:
    Win32Clipboard* clipboard_{nullptr};
};

} // namespace mirakana::win32
