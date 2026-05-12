// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <string>
#include <string_view>

namespace mirakana {

class IClipboard {
  public:
    virtual ~IClipboard() = default;

    [[nodiscard]] virtual bool has_text() const = 0;
    [[nodiscard]] virtual std::string text() const = 0;

    virtual void set_text(std::string_view text) = 0;
    virtual void clear() = 0;
};

class MemoryClipboard final : public IClipboard {
  public:
    [[nodiscard]] bool has_text() const override;
    [[nodiscard]] std::string text() const override;

    void set_text(std::string_view text) override;
    void clear() override;

  private:
    std::string text_;
};

} // namespace mirakana
