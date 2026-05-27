// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_cursor.hpp"

namespace mirakana::win32 {

Win32CursorModePlan plan_win32_cursor_mode(CursorMode mode) noexcept {
    Win32CursorModePlan plan;
    plan.state = make_cursor_state(mode);

    switch (mode) {
    case CursorMode::normal:
        plan.show_cursor = true;
        break;
    case CursorMode::hidden:
        plan.show_cursor = false;
        break;
    case CursorMode::confined:
        plan.show_cursor = true;
        plan.clip_to_window = true;
        plan.capture_mouse = true;
        break;
    case CursorMode::relative:
        plan.show_cursor = false;
        plan.clip_to_window = true;
        plan.capture_mouse = true;
        plan.request_raw_relative_motion = true;
        break;
    }

    return plan;
}

} // namespace mirakana::win32
