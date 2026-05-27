// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_clipboard.hpp"

#include "win32_utf.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstring>
#include <stdexcept>

namespace mirakana::win32 {
namespace {

class ClipboardScope final {
  public:
    ClipboardScope() {
        if (OpenClipboard(nullptr) == 0) {
            throw std::runtime_error("OpenClipboard failed");
        }
    }

    ~ClipboardScope() {
        CloseClipboard();
    }

    ClipboardScope(const ClipboardScope&) = delete;
    ClipboardScope& operator=(const ClipboardScope&) = delete;
};

class GlobalLockScope final {
  public:
    explicit GlobalLockScope(HGLOBAL handle) : handle_(handle), value_(GlobalLock(handle)) {
        if (value_ == nullptr) {
            throw std::runtime_error("GlobalLock failed");
        }
    }

    ~GlobalLockScope() {
        if (value_ != nullptr) {
            GlobalUnlock(handle_);
        }
    }

    GlobalLockScope(const GlobalLockScope&) = delete;
    GlobalLockScope& operator=(const GlobalLockScope&) = delete;

    [[nodiscard]] void* get() const noexcept {
        return value_;
    }

  private:
    HGLOBAL handle_{nullptr};
    void* value_{nullptr};
};

[[nodiscard]] std::runtime_error win32_clipboard_error(const char* operation) {
    return std::runtime_error(std::string(operation) + " failed");
}

void set_unicode_clipboard_text(std::wstring text) {
    text.push_back(L'\0');
    const auto byte_size = text.size() * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, byte_size);
    if (memory == nullptr) {
        throw win32_clipboard_error("GlobalAlloc");
    }

    try {
        GlobalLockScope lock(memory);
        std::memcpy(lock.get(), text.data(), byte_size);
    } catch (...) {
        GlobalFree(memory);
        throw;
    }

    if (SetClipboardData(CF_UNICODETEXT, memory) == nullptr) {
        GlobalFree(memory);
        throw win32_clipboard_error("SetClipboardData");
    }
}

} // namespace

Win32ClipboardTextWritePlan plan_win32_clipboard_write(std::string_view text) {
    Win32ClipboardTextWritePlan plan{
        .open_clipboard = true,
        .empty_clipboard = true,
        .set_unicode_text = !text.empty(),
    };

    try {
        plan.utf16_text = detail::utf16_from_utf8(text);
    } catch (const std::exception& exception) {
        plan.diagnostic = exception.what();
    }
    return plan;
}

Win32ClipboardTextReadPlan plan_win32_clipboard_read() noexcept {
    return {};
}

bool Win32Clipboard::has_text() const {
    return IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
}

std::string Win32Clipboard::text() const {
    ClipboardScope clipboard;
    if (IsClipboardFormatAvailable(CF_UNICODETEXT) == 0) {
        return {};
    }

    HGLOBAL memory = GetClipboardData(CF_UNICODETEXT);
    if (memory == nullptr) {
        throw win32_clipboard_error("GetClipboardData");
    }

    GlobalLockScope lock(memory);
    const auto* value = static_cast<const wchar_t*>(lock.get());
    return detail::utf8_from_wide(std::wstring_view{value});
}

void Win32Clipboard::set_text(std::string_view text) {
    const auto plan = plan_win32_clipboard_write(text);
    if (!plan.succeeded()) {
        throw std::invalid_argument(plan.diagnostic);
    }

    ClipboardScope clipboard;
    if (EmptyClipboard() == 0) {
        throw win32_clipboard_error("EmptyClipboard");
    }
    if (plan.set_unicode_text) {
        set_unicode_clipboard_text(detail::wide_from_utf8(text));
    }
}

void Win32Clipboard::clear() {
    ClipboardScope clipboard;
    if (EmptyClipboard() == 0) {
        throw win32_clipboard_error("EmptyClipboard");
    }
}

Win32ClipboardTextAdapter::Win32ClipboardTextAdapter(Win32Clipboard& clipboard) noexcept : clipboard_(&clipboard) {}

void Win32ClipboardTextAdapter::set_clipboard_text(std::string_view text) {
    clipboard_->set_text(text);
}

bool Win32ClipboardTextAdapter::has_clipboard_text() const {
    return clipboard_->has_text();
}

std::string Win32ClipboardTextAdapter::clipboard_text() const {
    return clipboard_->text();
}

} // namespace mirakana::win32
