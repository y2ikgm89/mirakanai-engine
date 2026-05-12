// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/registry.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace mirakana {

Entity Registry::create() {
    if (!free_indices_.empty()) {
        const auto index = free_indices_.back();
        free_indices_.pop_back();
        auto& slot = slots_[index];
        slot.alive = true;
        ++living_count_;
        return Entity{.index = index, .generation = slot.generation};
    }

    const auto index = static_cast<std::uint32_t>(slots_.size());
    slots_.push_back(Slot{.generation = 1, .alive = true});
    ++living_count_;
    return Entity{.index = index, .generation = 1};
}

void Registry::destroy(Entity entity) {
    if (!alive(entity)) {
        return;
    }

    for (auto& [_, storage] : component_storages_) {
        storage->erase(entity);
    }

    auto& slot = slots_[entity.index];
    slot.alive = false;
    ++slot.generation;
    free_indices_.push_back(entity.index);
    --living_count_;
}

bool Registry::alive(Entity entity) const {
    if (entity.index >= slots_.size()) {
        return false;
    }
    const auto& slot = slots_[entity.index];
    return slot.alive && slot.generation == entity.generation;
}

std::size_t Registry::living_count() const noexcept {
    return living_count_;
}

void Registry::ensure_alive(Entity entity) const {
    if (!alive(entity)) {
        throw std::logic_error("entity is not alive");
    }
}

} // namespace mirakana
