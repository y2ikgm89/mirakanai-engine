// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/palette.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace mirakana::editor {
namespace {

std::string lowercase(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const unsigned char character : text) {
        result.push_back(static_cast<char>(std::tolower(character)));
    }
    return result;
}

bool contains_case_insensitive(std::string_view haystack, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }

    const auto lower_haystack = lowercase(haystack);
    const auto lower_needle = lowercase(needle);
    return lower_haystack.find(lower_needle) != std::string::npos;
}

bool matches_query(const Command& command, std::string_view query) {
    return contains_case_insensitive(command.id, query) || contains_case_insensitive(command.label, query);
}

} // namespace

std::vector<CommandPaletteEntry> query_command_palette(const CommandRegistry& registry, std::string_view query) {
    std::vector<CommandPaletteEntry> matches;
    for (const auto& command : registry.commands()) {
        if (!matches_query(command, query)) {
            continue;
        }

        matches.push_back(CommandPaletteEntry{
            .id = command.id,
            .label = command.label,
            .enabled = command.enabled,
        });
    }
    return matches;
}

bool execute_palette_command(const CommandRegistry& registry, std::string_view id) {
    return registry.execute(id);
}

} // namespace mirakana::editor
