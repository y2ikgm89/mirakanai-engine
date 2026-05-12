// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/command.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct CommandPaletteEntry {
    std::string id;
    std::string label;
    bool enabled{true};
};

[[nodiscard]] std::vector<CommandPaletteEntry> query_command_palette(const CommandRegistry& registry,
                                                                     std::string_view query);
[[nodiscard]] bool execute_palette_command(const CommandRegistry& registry, std::string_view id);

} // namespace mirakana::editor
