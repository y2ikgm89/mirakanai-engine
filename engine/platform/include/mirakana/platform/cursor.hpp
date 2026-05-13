// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
namespace mirakana {

enum class CursorMode : std::uint8_t {
    normal,
    hidden,
    confined,
    relative,
};

struct CursorState {
    CursorMode mode{CursorMode::normal};
    bool visible{true};
    bool grabbed{false};
    bool relative{false};
};

[[nodiscard]] CursorState make_cursor_state(CursorMode mode) noexcept;

class ICursor {
  public:
    virtual ~ICursor() = default;

    [[nodiscard]] virtual CursorState state() const = 0;
    virtual void set_mode(CursorMode mode) = 0;
};

class MemoryCursor final : public ICursor {
  public:
    [[nodiscard]] CursorState state() const override;
    void set_mode(CursorMode mode) override;

  private:
    CursorMode mode_{CursorMode::normal};
};

} // namespace mirakana
