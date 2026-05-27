// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_window.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana::win32 {

namespace {

[[nodiscard]] bool valid_extent(WindowExtent extent) noexcept {
    return extent.width > 0 && extent.height > 0;
}

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

[[nodiscard]] HWND window_from_token(std::uintptr_t token) noexcept {
    return reinterpret_cast<HWND>(token);
}

[[nodiscard]] std::uintptr_t token_from_window(HWND window) noexcept {
    return reinterpret_cast<std::uintptr_t>(window);
}

[[nodiscard]] RECT outer_rect_for_client_extent(WindowExtent extent) {
    RECT rect{
        .left = 0, .top = 0, .right = static_cast<LONG>(extent.width), .bottom = static_cast<LONG>(extent.height)};
    if (AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0) == 0) {
        throw std::runtime_error(last_error_message("AdjustWindowRectEx"));
    }
    return rect;
}

[[nodiscard]] int outer_width(const RECT& rect) noexcept {
    return static_cast<int>(rect.right - rect.left);
}

[[nodiscard]] int outer_height(const RECT& rect) noexcept {
    return static_cast<int>(rect.bottom - rect.top);
}

void destroy_window_token(std::uintptr_t token) noexcept {
    auto* const window = window_from_token(token);
    if (window != nullptr && IsWindow(window) != 0) {
        DestroyWindow(window);
    }
}

} // namespace

Win32WindowCreationPlan plan_win32_window_creation(const WindowDesc& desc) {
    Win32WindowCreationPlan plan;
    plan.title = desc.title;
    plan.client_extent = desc.extent;
    plan.position = desc.position;
    plan.resizable = true;

    if (plan.title.empty()) {
        plan.diagnostic = "win32 window title must not be empty";
        return plan;
    }
    if (!valid_extent(plan.client_extent)) {
        plan.diagnostic = "win32 window client extent must be non-zero";
    }
    return plan;
}

Win32WindowModel::Win32WindowModel(WindowDesc desc)
    : title_(std::move(desc.title)), extent_(desc.extent), position_(desc.position) {
    const auto plan = plan_win32_window_creation(WindowDesc{.title = title_, .extent = extent_, .position = position_});
    if (!plan.succeeded()) {
        throw std::invalid_argument(plan.diagnostic);
    }
}

std::string_view Win32WindowModel::title() const noexcept {
    return title_;
}

WindowExtent Win32WindowModel::extent() const noexcept {
    return extent_;
}

WindowPosition Win32WindowModel::position() const noexcept {
    return position_;
}

bool Win32WindowModel::is_open() const noexcept {
    return open_;
}

bool Win32WindowModel::focused() const noexcept {
    return focused_;
}

bool Win32WindowModel::minimized() const noexcept {
    return minimized_;
}

void Win32WindowModel::resize(WindowExtent extent) {
    if (!valid_extent(extent)) {
        throw std::invalid_argument("win32 window client extent must be non-zero");
    }
    extent_ = extent;
}

void Win32WindowModel::move(WindowPosition position) {
    position_ = position;
}

void Win32WindowModel::apply_placement(WindowPlacement placement) {
    resize(placement.extent);
    move(placement.position);
}

void Win32WindowModel::request_close() noexcept {
    open_ = false;
}

void Win32WindowModel::handle_event(const Win32WindowEvent& event, VirtualLifecycle* lifecycle) noexcept {
    switch (event.kind) {
    case Win32WindowEventKind::close_requested:
    case Win32WindowEventKind::destroyed:
        open_ = false;
        if (lifecycle != nullptr) {
            lifecycle->push(LifecycleEventKind::quit_requested);
        }
        break;
    case Win32WindowEventKind::resized:
    case Win32WindowEventKind::restored:
    case Win32WindowEventKind::maximized:
        if (valid_extent(event.extent)) {
            extent_ = event.extent;
        }
        minimized_ = false;
        break;
    case Win32WindowEventKind::moved:
        position_ = event.position;
        break;
    case Win32WindowEventKind::minimized:
        minimized_ = true;
        break;
    case Win32WindowEventKind::focus_gained:
        focused_ = true;
        break;
    case Win32WindowEventKind::focus_lost:
        focused_ = false;
        break;
    case Win32WindowEventKind::display_changed:
    case Win32WindowEventKind::dpi_changed:
    case Win32WindowEventKind::unknown:
        break;
    }
}

Win32Window::Win32Window(Win32Runtime& runtime, WindowDesc desc)
    : runtime_(&runtime), title_(std::move(desc.title)), extent_(desc.extent), position_(desc.position) {
    const auto plan = plan_win32_window_creation(WindowDesc{.title = title_, .extent = extent_, .position = position_});
    if (!plan.succeeded()) {
        throw std::invalid_argument(plan.diagnostic);
    }

    const auto wide_class_name = utf8_to_wide(runtime.window_class_name());
    const auto wide_title = utf8_to_wide(title_);
    const auto outer = outer_rect_for_client_extent(extent_);
    auto* const instance = reinterpret_cast<HINSTANCE>(runtime.native_instance_token());
    HWND window =
        CreateWindowExW(0, wide_class_name.c_str(), wide_title.c_str(), WS_OVERLAPPEDWINDOW, position_.x, position_.y,
                        outer_width(outer), outer_height(outer), nullptr, nullptr, instance, nullptr);
    if (window == nullptr) {
        throw std::runtime_error(last_error_message("CreateWindowExW"));
    }
    window_token_ = token_from_window(window);
}

Win32Window::~Win32Window() {
    destroy_window_token(window_token_);
    window_token_ = 0;
}

std::string_view Win32Window::title() const noexcept {
    return title_;
}

WindowExtent Win32Window::extent() const noexcept {
    return extent_;
}

WindowPosition Win32Window::position() const noexcept {
    return position_;
}

bool Win32Window::is_open() const noexcept {
    return open_;
}

WindowDisplayState Win32Window::display_state() const {
    auto* const window = window_from_token(window_token_);
    if (window == nullptr || IsWindow(window) == 0) {
        throw std::runtime_error("win32 window is not valid");
    }

    auto* const monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr) {
        throw std::runtime_error(last_error_message("MonitorFromWindow"));
    }

    MONITORINFO monitor_info{};
    monitor_info.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfoW(monitor, &monitor_info) == 0) {
        throw std::runtime_error(last_error_message("GetMonitorInfoW"));
    }

    RECT client{};
    if (GetClientRect(window, &client) == 0) {
        throw std::runtime_error(last_error_message("GetClientRect"));
    }

    const auto dpi = std::max<UINT>(GetDpiForWindow(window), 96U);
    const auto content_scale = static_cast<float>(dpi) / 96.0F;
    const auto monitor_id = static_cast<DisplayId>(reinterpret_cast<std::uintptr_t>(monitor) & 0xffffffffU);
    return WindowDisplayState{
        .display_id = std::max<DisplayId>(monitor_id, 1U),
        .content_scale = content_scale,
        .pixel_density = content_scale,
        .safe_area =
            DisplayRect{
                .x = 0,
                .y = 0,
                .width = client.right > client.left ? static_cast<std::uint32_t>(client.right - client.left) : 1U,
                .height = client.bottom > client.top ? static_cast<std::uint32_t>(client.bottom - client.top) : 1U,
            },
    };
}

std::uintptr_t Win32Window::native_window_token() const noexcept {
    return window_token_;
}

void Win32Window::resize(WindowExtent extent) {
    if (!valid_extent(extent)) {
        throw std::invalid_argument("win32 window client extent must be non-zero");
    }
    auto* const window = window_from_token(window_token_);
    const auto outer = outer_rect_for_client_extent(extent);
    if (SetWindowPos(window, nullptr, 0, 0, outer_width(outer), outer_height(outer),
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) == 0) {
        throw std::runtime_error(last_error_message("SetWindowPos"));
    }
    extent_ = extent;
}

void Win32Window::move(WindowPosition position) {
    auto* const window = window_from_token(window_token_);
    if (SetWindowPos(window, nullptr, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE) == 0) {
        throw std::runtime_error(last_error_message("SetWindowPos"));
    }
    position_ = position;
}

void Win32Window::apply_placement(WindowPlacement placement) {
    resize(placement.extent);
    move(placement.position);
}

void Win32Window::request_close() noexcept {
    open_ = false;
    destroy_window_token(window_token_);
}

void Win32Window::post_close_request() noexcept {
    auto* const window = window_from_token(window_token_);
    if (window != nullptr && IsWindow(window) != 0) {
        PostMessageW(window, WM_CLOSE, 0, 0);
    }
}

void Win32Window::handle_event(const Win32WindowEvent& event, VirtualLifecycle* lifecycle) noexcept {
    if (event.window_token != 0 && event.window_token != window_token_) {
        return;
    }

    switch (event.kind) {
    case Win32WindowEventKind::close_requested:
    case Win32WindowEventKind::destroyed:
        request_close();
        if (lifecycle != nullptr) {
            lifecycle->push(LifecycleEventKind::quit_requested);
        }
        break;
    case Win32WindowEventKind::resized:
    case Win32WindowEventKind::restored:
    case Win32WindowEventKind::maximized:
        if (valid_extent(event.extent)) {
            extent_ = event.extent;
        }
        break;
    case Win32WindowEventKind::moved:
        position_ = event.position;
        break;
    case Win32WindowEventKind::focus_gained:
    case Win32WindowEventKind::focus_lost:
    case Win32WindowEventKind::minimized:
    case Win32WindowEventKind::display_changed:
    case Win32WindowEventKind::dpi_changed:
    case Win32WindowEventKind::unknown:
        break;
    }
}

} // namespace mirakana::win32
