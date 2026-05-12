// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <string_view>

namespace mirakana {

struct Version {
    int major;
    int minor;
    int patch;
    std::string_view name;
};

[[nodiscard]] constexpr Version engine_version() noexcept {
    return Version{.major = 0, .minor = 1, .patch = 0, .name = "GameEngine"};
}

} // namespace mirakana
