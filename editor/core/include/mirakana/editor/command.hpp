// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct Command {
    std::string id;
    std::string label;
    std::function<void()> action;
    bool enabled{true};
};

class CommandRegistry {
  public:
    [[nodiscard]] bool try_add(Command command);
    [[nodiscard]] bool contains(std::string_view id) const noexcept;
    [[nodiscard]] bool execute(std::string_view id) const;
    [[nodiscard]] const std::vector<Command>& commands() const noexcept;

  private:
    [[nodiscard]] Command* find(std::string_view id) noexcept;
    [[nodiscard]] const Command* find(std::string_view id) const noexcept;

    std::vector<Command> commands_;
};

} // namespace mirakana::editor
