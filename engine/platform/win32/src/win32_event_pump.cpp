// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_event_pump.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace mirakana::win32 {
namespace {

[[nodiscard]] Win32MessageId message_id_from_native(UINT message) noexcept {
    switch (message) {
    case WM_CLOSE:
        return Win32MessageId::close;
    case WM_DESTROY:
        return Win32MessageId::destroy;
    case WM_SIZE:
        return Win32MessageId::size;
    case WM_MOVE:
        return Win32MessageId::move;
    case WM_SETFOCUS:
        return Win32MessageId::set_focus;
    case WM_KILLFOCUS:
        return Win32MessageId::kill_focus;
    case WM_DISPLAYCHANGE:
        return Win32MessageId::display_change;
    case WM_DPICHANGED:
        return Win32MessageId::dpi_changed;
    default:
        return Win32MessageId::unknown;
    }
}

[[nodiscard]] Win32CopiedMessage copy_message(const MSG& message) noexcept {
    return Win32CopiedMessage{
        .message = message_id_from_native(message.message),
        .wparam = static_cast<std::uint64_t>(message.wParam),
        .low_word = LOWORD(message.lParam),
        .high_word = HIWORD(message.lParam),
        .window_token = reinterpret_cast<std::uintptr_t>(message.hwnd),
    };
}

} // namespace

Win32WindowEvent translate_win32_window_message(const Win32CopiedMessage& message) noexcept {
    Win32WindowEvent event;
    event.window_token = message.window_token;

    switch (message.message) {
    case Win32MessageId::close:
        event.kind = Win32WindowEventKind::close_requested;
        break;
    case Win32MessageId::destroy:
        event.kind = Win32WindowEventKind::destroyed;
        break;
    case Win32MessageId::size:
        if (message.wparam == win32_size_minimized) {
            event.kind = Win32WindowEventKind::minimized;
        } else if (message.wparam == win32_size_maximized) {
            event.kind = Win32WindowEventKind::maximized;
        } else {
            event.kind = Win32WindowEventKind::resized;
        }
        event.extent = WindowExtent{.width = message.low_word, .height = message.high_word};
        break;
    case Win32MessageId::move:
        event.kind = Win32WindowEventKind::moved;
        event.position = WindowPosition{.x = static_cast<std::int16_t>(message.low_word),
                                        .y = static_cast<std::int16_t>(message.high_word)};
        break;
    case Win32MessageId::set_focus:
        event.kind = Win32WindowEventKind::focus_gained;
        break;
    case Win32MessageId::kill_focus:
        event.kind = Win32WindowEventKind::focus_lost;
        break;
    case Win32MessageId::display_change:
        event.kind = Win32WindowEventKind::display_changed;
        event.display_pixel_extent = WindowExtent{.width = message.low_word, .height = message.high_word};
        break;
    case Win32MessageId::dpi_changed:
        event.kind = Win32WindowEventKind::dpi_changed;
        event.content_scale = static_cast<float>(message.wparam & 0xffffU) / 96.0F;
        break;
    case Win32MessageId::unknown:
        break;
    }

    return event;
}

std::vector<Win32WindowEvent> Win32EventPump::poll() {
    std::vector<Win32WindowEvent> events;
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
        const auto copied = copy_message(message);
        auto event = translate_win32_window_message(copied);
        if (event.kind != Win32WindowEventKind::unknown) {
            events.push_back(event);
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return events;
}

} // namespace mirakana::win32
