// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/cursor.hpp"

namespace mirakana {

CursorState make_cursor_state(CursorMode mode) noexcept {
    switch (mode) {
    case CursorMode::normal:
        return CursorState{.mode = CursorMode::normal, .visible = true, .grabbed = false, .relative = false};
    case CursorMode::hidden:
        return CursorState{.mode = CursorMode::hidden, .visible = false, .grabbed = false, .relative = false};
    case CursorMode::confined:
        return CursorState{.mode = CursorMode::confined, .visible = true, .grabbed = true, .relative = false};
    case CursorMode::relative:
        return CursorState{.mode = CursorMode::relative, .visible = false, .grabbed = true, .relative = true};
    }

    return CursorState{};
}

CursorState MemoryCursor::state() const {
    return make_cursor_state(mode_);
}

void MemoryCursor::set_mode(CursorMode mode) {
    mode_ = mode;
}

} // namespace mirakana
