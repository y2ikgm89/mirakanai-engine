// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/material.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

struct MaterialInstancePackageUpdateFailure {
    std::string diagnostic;
};

struct MaterialInstancePackageChangedFile {
    std::string path;
    std::string content;
    std::uint64_t content_hash{0};
};

struct MaterialInstancePackageUpdateDesc {
    std::string package_index_path;
    std::string package_index_content;
    std::string output_path;
    std::uint64_t source_revision{0};
    MaterialInstanceDefinition instance;
    std::string material_graph{"unsupported"};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
};

struct MaterialInstancePackageApplyDesc {
    std::string package_index_path;
    std::string output_path;
    std::uint64_t source_revision{0};
    MaterialInstanceDefinition instance;
    std::string material_graph{"unsupported"};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
};

struct MaterialInstancePackageUpdateResult {
    std::string material_content;
    std::string package_index_content;
    std::vector<MaterialInstancePackageChangedFile> changed_files;
    std::vector<MaterialInstancePackageUpdateFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct MaterialGraphPackageUpdateDesc {
    std::string package_index_path;
    std::string package_index_content;
    std::string material_graph_content;
    std::string output_path;
    std::uint64_t source_revision{0};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
};

struct MaterialGraphPackageApplyDesc {
    std::string package_index_path;
    std::string material_graph_path;
    std::string output_path;
    std::uint64_t source_revision{0};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
};

struct MaterialGraphPackageUpdateResult {
    std::string material_content;
    std::string package_index_content;
    std::vector<MaterialInstancePackageChangedFile> changed_files;
    std::vector<MaterialInstancePackageUpdateFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

[[nodiscard]] MaterialInstancePackageUpdateResult
plan_material_instance_package_update(const MaterialInstancePackageUpdateDesc& desc);
[[nodiscard]] MaterialInstancePackageUpdateResult
apply_material_instance_package_update(IFileSystem& filesystem, const MaterialInstancePackageApplyDesc& desc);

[[nodiscard]] MaterialGraphPackageUpdateResult
plan_material_graph_package_update(const MaterialGraphPackageUpdateDesc& desc);
[[nodiscard]] MaterialGraphPackageUpdateResult
apply_material_graph_package_update(IFileSystem& filesystem, const MaterialGraphPackageApplyDesc& desc);

} // namespace mirakana
