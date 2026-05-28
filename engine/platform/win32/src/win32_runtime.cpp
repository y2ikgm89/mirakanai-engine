// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_runtime.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <stdexcept>
#include <string>
#include <string_view>

namespace mirakana::win32 {
namespace {

[[nodiscard]] std::wstring utf8_to_wide(std::string_view text) {
    if (text.empty()) {
        return {};
    }

    const auto required =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) {
        std::wstring fallback;
        fallback.reserve(text.size());
        for (const auto character : text) {
            fallback.push_back(static_cast<unsigned char>(character));
        }
        return fallback;
    }

    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(),
                        required);
    return wide;
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

LRESULT CALLBACK win32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    return DefWindowProcW(window, message, wparam, lparam);
}

inline constexpr WORD arrow_cursor_resource_id = 32512;

void initialize_dpi_awareness_best_effort() noexcept {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
}

} // namespace

Win32RuntimeStartupPlan plan_win32_runtime_startup(const Win32RuntimeDesc& desc) {
    Win32RuntimeStartupPlan plan;
    plan.window_class_name = desc.window_class_name;
    plan.initialize_dpi_awareness = desc.dpi_aware;
    if (plan.window_class_name.empty()) {
        plan.diagnostic = "win32 window class name must not be empty";
        return plan;
    }

    plan.register_window_class = true;
    return plan;
}

Win32Runtime::Win32Runtime(Win32RuntimeDesc desc) {
    auto startup = plan_win32_runtime_startup(desc);
    if (!startup.succeeded()) {
        throw std::invalid_argument(startup.diagnostic);
    }

    window_class_name_ = std::move(startup.window_class_name);
    if (startup.initialize_dpi_awareness) {
        initialize_dpi_awareness_best_effort();
    }

    auto* const instance = GetModuleHandleW(nullptr);
    instance_token_ = reinterpret_cast<std::uintptr_t>(instance);

    WNDCLASSEXW window_class{};
    window_class.cbSize = sizeof(WNDCLASSEXW);
    window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.hInstance = instance;
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(arrow_cursor_resource_id));
    const auto wide_class_name = utf8_to_wide(window_class_name_);
    window_class.lpszClassName = wide_class_name.c_str();

    if (RegisterClassExW(&window_class) == 0) {
        const auto error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            throw std::runtime_error(last_error_message("RegisterClassExW", error));
        }
    } else {
        registered_window_class_ = true;
    }
}

Win32Runtime::~Win32Runtime() {
    if (registered_window_class_) {
        auto* const instance = reinterpret_cast<HINSTANCE>(instance_token_);
        const auto wide_class_name = utf8_to_wide(window_class_name_);
        UnregisterClassW(wide_class_name.c_str(), instance);
    }
}

std::string_view Win32Runtime::window_class_name() const noexcept {
    return window_class_name_;
}

std::uintptr_t Win32Runtime::native_instance_token() const noexcept {
    return instance_token_;
}

Win32RuntimeShutdownPlan plan_win32_runtime_shutdown(const Win32RuntimeStartupPlan& startup) {
    Win32RuntimeShutdownPlan plan;
    if (!startup.succeeded()) {
        plan.diagnostic = "win32 runtime startup did not succeed";
        return plan;
    }

    plan.unregister_window_class = startup.register_window_class;
    return plan;
}

} // namespace mirakana::win32
