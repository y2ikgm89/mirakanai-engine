// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace mirakana {

struct Entity {
    std::uint32_t index{0};
    std::uint32_t generation{0};

    friend constexpr bool operator==(Entity lhs, Entity rhs) noexcept {
        return lhs.index == rhs.index && lhs.generation == rhs.generation;
    }

    friend constexpr bool operator!=(Entity lhs, Entity rhs) noexcept {
        return !(lhs == rhs);
    }
};

struct EntityHash {
    [[nodiscard]] std::size_t operator()(Entity entity) const noexcept {
        const auto packed = (static_cast<std::uint64_t>(entity.generation) << 32U) | entity.index;
        return std::hash<std::uint64_t>{}(packed);
    }
};

} // namespace mirakana
