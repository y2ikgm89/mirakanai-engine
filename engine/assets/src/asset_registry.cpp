// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_registry.hpp"

#include <stdexcept>
#include <utility>

namespace mirakana {

AssetId AssetId::from_name(std::string_view name) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char character : name) {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ULL;
    }
    return AssetId{hash};
}

bool AssetRegistry::try_add(AssetRecord record) {
    if (record.id.value == 0 || record.path.empty() || record.kind == AssetKind::unknown) {
        return false;
    }

    auto [_, inserted] = records_.emplace(record.id, std::move(record));
    return inserted;
}

void AssetRegistry::add(AssetRecord record) {
    if (!try_add(std::move(record))) {
        throw std::logic_error("asset record could not be added");
    }
}

const AssetRecord* AssetRegistry::find(AssetId id) const noexcept {
    const auto it = records_.find(id);
    return it == records_.end() ? nullptr : &it->second;
}

std::size_t AssetRegistry::count() const noexcept {
    return records_.size();
}

std::vector<AssetRecord> AssetRegistry::records() const {
    std::vector<AssetRecord> result;
    result.reserve(records_.size());
    for (const auto& [_, record] : records_) {
        result.push_back(record);
    }
    return result;
}

} // namespace mirakana
