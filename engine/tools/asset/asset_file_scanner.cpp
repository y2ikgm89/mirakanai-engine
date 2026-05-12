// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/asset_file_scanner.hpp"

#include <algorithm>
#include <stdexcept>
#include <string_view>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_path(std::string_view path) noexcept {
    return !path.empty() && path.find('\n') == std::string_view::npos && path.find('\r') == std::string_view::npos &&
           path.find('\0') == std::string_view::npos;
}

[[nodiscard]] std::uint64_t revision_for_text(std::string_view text) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char character : text) {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ULL;
    }
    return hash == 0 ? 1 : hash;
}

void sort_snapshots(std::vector<AssetFileSnapshot>& snapshots) {
    std::ranges::sort(snapshots, [](const AssetFileSnapshot& lhs, const AssetFileSnapshot& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
}

void sort_missing(std::vector<AssetFileScanMissing>& missing) {
    std::ranges::sort(missing, [](const AssetFileScanMissing& lhs, const AssetFileScanMissing& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
}

} // namespace

AssetFileScanResult scan_asset_files_for_hot_reload(const IFileSystem& filesystem, const AssetRegistry& assets) {
    AssetFileScanResult result;
    const auto records = assets.records();
    result.snapshots.reserve(records.size());

    for (const auto& record : records) {
        if (record.id.value == 0 || record.kind == AssetKind::unknown || !valid_path(record.path)) {
            throw std::invalid_argument("asset record is invalid for file scanning");
        }
        if (!filesystem.exists(record.path)) {
            result.missing.push_back(AssetFileScanMissing{.asset = record.id, .path = record.path});
            continue;
        }

        const auto text = filesystem.read_text(record.path);
        result.snapshots.push_back(AssetFileSnapshot{
            .asset = record.id,
            .path = record.path,
            .revision = revision_for_text(text),
            .size_bytes = static_cast<std::uint64_t>(text.size()),
        });
    }

    sort_snapshots(result.snapshots);
    sort_missing(result.missing);
    return result;
}

} // namespace mirakana
