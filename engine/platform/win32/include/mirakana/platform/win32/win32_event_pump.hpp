// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/window.hpp"

#include <cstdint>
#include <vector>

namespace mirakana::win32 {

enum class Win32MessageId : std::uint8_t {
    unknown = 0,
    close,
    destroy,
    size,
    move,
    set_focus,
    kill_focus,
    display_change,
    dpi_changed,
};

inline constexpr std::uint64_t win32_size_restored = 0;
inline constexpr std::uint64_t win32_size_minimized = 1;
inline constexpr std::uint64_t win32_size_maximized = 2;

struct Win32CopiedMessage {
    Win32MessageId message{Win32MessageId::unknown};
    std::uint64_t wparam{0};
    std::uint16_t low_word{0};
    std::uint16_t high_word{0};
    std::uintptr_t window_token{0};
};

enum class Win32WindowEventKind : std::uint8_t {
    unknown = 0,
    close_requested,
    destroyed,
    resized,
    moved,
    minimized,
    maximized,
    restored,
    focus_gained,
    focus_lost,
    display_changed,
    dpi_changed,
};

struct Win32WindowEvent {
    Win32WindowEventKind kind{Win32WindowEventKind::unknown};
    WindowExtent extent{};
    WindowPosition position{};
    WindowExtent display_pixel_extent{};
    float content_scale{1.0F};
    std::uintptr_t window_token{0};
};

[[nodiscard]] Win32WindowEvent translate_win32_window_message(const Win32CopiedMessage& message) noexcept;

class Win32EventPump final {
  public:
    [[nodiscard]] std::vector<Win32WindowEvent> poll();
};

} // namespace mirakana::win32
