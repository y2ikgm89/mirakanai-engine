// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "win32_imgui_message_bridge.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <imgui_impl_win32.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

inline constexpr wchar_t bridge_property_name[] = L"MIRAIKANAI.Editor.ImguiMessageBridge";

[[nodiscard]] HWND window_from_token(std::uintptr_t token) noexcept {
    return reinterpret_cast<HWND>(token);
}

[[nodiscard]] std::uintptr_t token_from_window(HWND window) noexcept {
    return reinterpret_cast<std::uintptr_t>(window);
}

[[nodiscard]] std::string last_error_message(std::string_view prefix, DWORD error = GetLastError()) {
    LPSTR buffer = nullptr;
    const auto chars = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

    std::string message(prefix);
    message.append(" failed with error ");
    message.append(std::to_string(error));
    if (chars != 0 && buffer != nullptr) {
        message.append(": ");
        message.append(buffer, chars);
        LocalFree(buffer);
    }
    return message;
}

[[nodiscard]] mirakana::win32::Win32MessageId message_id_from_native(UINT message) noexcept {
    switch (message) {
    case WM_CLOSE:
        return mirakana::win32::Win32MessageId::close;
    case WM_DESTROY:
        return mirakana::win32::Win32MessageId::destroy;
    case WM_SIZE:
        return mirakana::win32::Win32MessageId::size;
    case WM_MOVE:
        return mirakana::win32::Win32MessageId::move;
    case WM_SETFOCUS:
        return mirakana::win32::Win32MessageId::set_focus;
    case WM_KILLFOCUS:
        return mirakana::win32::Win32MessageId::kill_focus;
    case WM_DISPLAYCHANGE:
        return mirakana::win32::Win32MessageId::display_change;
    case WM_DPICHANGED:
        return mirakana::win32::Win32MessageId::dpi_changed;
    default:
        return mirakana::win32::Win32MessageId::unknown;
    }
}

[[nodiscard]] mirakana::win32::Win32CopiedMessage copy_message(HWND window, UINT message, WPARAM wparam,
                                                               LPARAM lparam) noexcept {
    return mirakana::win32::Win32CopiedMessage{
        .message = message_id_from_native(message),
        .wparam = static_cast<std::uint64_t>(wparam),
        .low_word = LOWORD(lparam),
        .high_word = HIWORD(lparam),
        .window_token = token_from_window(window),
    };
}

} // namespace

struct Win32ImguiMessageBridgeImpl {
    HWND window{nullptr};
    WNDPROC previous_proc{nullptr};
    std::vector<Win32ImguiCopiedMessage> copied_messages;

    void record(HWND target_window, UINT message, WPARAM wparam, LPARAM lparam, bool imgui_handled) {
        const auto copied = copy_message(target_window, message, wparam, lparam);
        if (copied.message != mirakana::win32::Win32MessageId::unknown) {
            copied_messages.push_back(Win32ImguiCopiedMessage{.message = copied, .imgui_handled = imgui_handled});
        }
    }
};

namespace {

[[nodiscard]] Win32ImguiMessageBridgeImpl* bridge_from_window(HWND window) noexcept {
    return reinterpret_cast<Win32ImguiMessageBridgeImpl*>(GetPropW(window, bridge_property_name));
}

LRESULT CALLBACK imgui_bridge_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* const bridge = bridge_from_window(window);
    const LRESULT imgui_result = ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam);
    const bool imgui_handled = imgui_result != 0;
    if (bridge != nullptr) {
        bridge->record(window, message, wparam, lparam, imgui_handled);
        if (imgui_handled) {
            return imgui_result;
        }
        if (bridge->previous_proc != nullptr) {
            return CallWindowProcW(bridge->previous_proc, window, message, wparam, lparam);
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

} // namespace

Win32ImguiMessageBridge::Win32ImguiMessageBridge(std::uintptr_t window_token)
    : impl_(std::make_unique<Win32ImguiMessageBridgeImpl>()) {
    impl_->window = window_from_token(window_token);
    if (impl_->window == nullptr || IsWindow(impl_->window) == 0) {
        throw std::invalid_argument("win32 imgui message bridge requires a valid window token");
    }
    if (SetPropW(impl_->window, bridge_property_name, impl_.get()) == 0) {
        const auto diagnostic = last_error_message("SetPropW");
        throw std::runtime_error(diagnostic);
    }

    const auto previous = SetWindowLongPtrW(impl_->window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(imgui_bridge_proc));
    if (previous == 0) {
        const auto error = GetLastError();
        if (error != 0) {
            RemovePropW(impl_->window, bridge_property_name);
            const auto diagnostic = last_error_message("SetWindowLongPtrW", error);
            throw std::runtime_error(diagnostic);
        }
    }
    impl_->previous_proc = reinterpret_cast<WNDPROC>(previous);
}

Win32ImguiMessageBridge::~Win32ImguiMessageBridge() {
    if (impl_ == nullptr) {
        return;
    }
    if (impl_->window != nullptr && IsWindow(impl_->window) != 0) {
        if (impl_->previous_proc != nullptr) {
            SetWindowLongPtrW(impl_->window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(impl_->previous_proc));
        }
        RemovePropW(impl_->window, bridge_property_name);
    }
    impl_.reset();
}

std::vector<Win32ImguiCopiedMessage> Win32ImguiMessageBridge::drain_messages() {
    auto messages = std::move(impl_->copied_messages);
    impl_->copied_messages.clear();
    return messages;
}

} // namespace mirakana::editor
