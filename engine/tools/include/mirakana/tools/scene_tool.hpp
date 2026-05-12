// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/scene/scene.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

struct ScenePackageUpdateFailure {
    AssetId asset;
    std::string path;
    std::string diagnostic;
};

struct ScenePackageChangedFile {
    std::string path;
    std::string content;
    std::uint64_t content_hash{0};
};

struct ScenePackageUpdateDesc {
    std::string package_index_path;
    std::string package_index_content;
    std::string output_path;
    std::uint64_t source_revision{0};
    AssetId scene_asset;
    Scene scene{"Scene"};
    std::vector<AssetId> mesh_dependencies;
    std::vector<AssetId> material_dependencies;
    std::vector<AssetId> sprite_dependencies;
    std::string editor_productization{"unsupported"};
    std::string prefab_mutation{"unsupported"};
    std::string runtime_source_import{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
    std::string material_graph{"unsupported"};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
};

struct ScenePackageApplyDesc {
    std::string package_index_path;
    std::string output_path;
    std::uint64_t source_revision{0};
    AssetId scene_asset;
    Scene scene{"Scene"};
    std::vector<AssetId> mesh_dependencies;
    std::vector<AssetId> material_dependencies;
    std::vector<AssetId> sprite_dependencies;
    std::string editor_productization{"unsupported"};
    std::string prefab_mutation{"unsupported"};
    std::string runtime_source_import{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
    std::string material_graph{"unsupported"};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
};

struct ScenePackageUpdateResult {
    std::string scene_content;
    std::string package_index_content;
    std::vector<ScenePackageChangedFile> changed_files;
    std::vector<ScenePackageUpdateFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

[[nodiscard]] ScenePackageUpdateResult plan_scene_package_update(const ScenePackageUpdateDesc& desc);
[[nodiscard]] ScenePackageUpdateResult apply_scene_package_update(IFileSystem& filesystem,
                                                                  const ScenePackageApplyDesc& desc);

} // namespace mirakana
