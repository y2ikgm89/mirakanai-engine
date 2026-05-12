// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <string>
#include <vector>

namespace mirakana {

struct AssetFileScanMissing {
    AssetId asset;
    std::string path;
};

struct AssetFileScanResult {
    std::vector<AssetFileSnapshot> snapshots;
    std::vector<AssetFileScanMissing> missing;

    [[nodiscard]] bool complete() const noexcept {
        return missing.empty();
    }
};

[[nodiscard]] AssetFileScanResult scan_asset_files_for_hot_reload(const IFileSystem& filesystem,
                                                                  const AssetRegistry& assets);

} // namespace mirakana
