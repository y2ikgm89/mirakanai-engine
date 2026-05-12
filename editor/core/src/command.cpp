// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/command.hpp"

#include <algorithm>
#include <utility>

namespace mirakana::editor {

bool CommandRegistry::try_add(Command command) {
    if (command.id.empty() || command.label.empty() || !command.action || contains(command.id)) {
        return false;
    }

    commands_.push_back(std::move(command));
    return true;
}

bool CommandRegistry::contains(std::string_view id) const noexcept {
    return find(id) != nullptr;
}

bool CommandRegistry::execute(std::string_view id) const {
    const auto* command = find(id);
    if (command == nullptr || !command->enabled) {
        return false;
    }

    command->action();
    return true;
}

const std::vector<Command>& CommandRegistry::commands() const noexcept {
    return commands_;
}

Command* CommandRegistry::find(std::string_view id) noexcept {
    const auto it = std::ranges::find_if(commands_, [id](const Command& command) { return command.id == id; });
    return it == commands_.end() ? nullptr : &(*it);
}

const Command* CommandRegistry::find(std::string_view id) const noexcept {
    const auto it = std::ranges::find_if(commands_, [id](const Command& command) { return command.id == id; });
    return it == commands_.end() ? nullptr : &(*it);
}

} // namespace mirakana::editor
