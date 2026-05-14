// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/assets/asset_import_metadata.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class AssetImportActionKind : std::uint8_t {
    unknown,
    texture,
    mesh,
    morph_mesh_cpu,
    animation_float_clip,
    animation_quaternion_clip,
    material,
    scene,
    audio
};

struct AssetImportAction {
    AssetId id;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string output_path;
    std::vector<AssetId> dependencies;
};

struct AssetImportPlan {
    std::vector<AssetImportAction> actions;
    std::vector<AssetDependencyEdge> dependencies;
};

[[nodiscard]] bool is_valid_asset_import_action(const AssetImportAction& action) noexcept;
[[nodiscard]] AssetImportPlan build_asset_import_plan(const AssetImportMetadataRegistry& imports);
[[nodiscard]] AssetImportPlan build_asset_recook_plan(const AssetImportPlan& import_plan,
                                                      const std::vector<AssetHotReloadRecookRequest>& requests);

} // namespace mirakana
