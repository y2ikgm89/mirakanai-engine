// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/cursor.hpp"
#include "mirakana/platform/sdl3/sdl_window.hpp"

namespace mirakana {

class SdlCursor final : public ICursor {
  public:
    explicit SdlCursor(SdlWindow& window) noexcept;

    [[nodiscard]] CursorState state() const override;
    void set_mode(CursorMode mode) override;

  private:
    SdlWindow* window_{nullptr};
};

} // namespace mirakana
