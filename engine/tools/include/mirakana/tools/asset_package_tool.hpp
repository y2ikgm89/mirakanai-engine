// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/asset_import_tool.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct AssetCookedPackageAssemblyDesc {
    std::uint64_t source_revision{1};
};

struct AssetCookedPackageAssemblyFailure {
    AssetId asset;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string path;
    std::string diagnostic;
};

struct AssetCookedPackageAssemblyResult {
    AssetCookedPackageIndex index;
    std::vector<AssetCookedPackageAssemblyFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

[[nodiscard]] AssetCookedPackageAssemblyResult
assemble_asset_cooked_package(IFileSystem& filesystem, const AssetImportPlan& plan,
                              const AssetImportExecutionResult& import_result,
                              AssetCookedPackageAssemblyDesc desc = {});

void write_asset_cooked_package_index(IFileSystem& filesystem, std::string_view path,
                                      const AssetCookedPackageIndex& index);

} // namespace mirakana
