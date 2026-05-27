// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/cursor.hpp"

namespace mirakana::win32 {

struct Win32CursorModePlan {
    CursorState state{};
    bool show_cursor{true};
    bool clip_to_window{false};
    bool capture_mouse{false};
    bool request_raw_relative_motion{false};
};

[[nodiscard]] Win32CursorModePlan plan_win32_cursor_mode(CursorMode mode) noexcept;

} // namespace mirakana::win32
