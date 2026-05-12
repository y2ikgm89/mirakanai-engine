// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/physics/physics3d.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct PhysicsCollisionPackageBodyDesc {
    std::string name;
    PhysicsBody3DDesc body;
    std::string material;
    std::string compound;
};

struct PhysicsCollisionPackageAuthoringDesc {
    AssetId collision_asset;
    std::string output_path;
    std::uint64_t source_revision{1};
    PhysicsWorld3DConfig world_config{};
    std::string native_backend{"unsupported"};
    std::vector<PhysicsCollisionPackageBodyDesc> bodies;
};

struct PhysicsCollisionPackageDiagnostic {
    std::string code;
    std::string message;
    std::string path;
    std::size_t body_index{0};
};

struct PhysicsCollisionPackageAuthoringResult {
    std::string content;
    AssetCookedArtifact artifact;
    std::vector<PhysicsCollisionPackageDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct PhysicsCollisionPackageVerificationResult {
    std::vector<PhysicsCollisionPackageDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct PhysicsCollisionPackageChangedFile {
    std::string path;
    std::string content;
    std::uint64_t content_hash{0};
};

struct PhysicsCollisionPackageUpdateDesc {
    std::string package_index_path;
    std::string package_index_content;
    PhysicsCollisionPackageAuthoringDesc collision;
};

struct PhysicsCollisionPackageApplyDesc {
    std::string package_index_path;
    PhysicsCollisionPackageAuthoringDesc collision;
};

struct PhysicsCollisionPackageUpdateResult {
    std::string collision_content;
    std::string package_index_content;
    std::vector<PhysicsCollisionPackageChangedFile> changed_files;
    std::vector<PhysicsCollisionPackageDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] PhysicsCollisionPackageAuthoringResult
author_physics_collision_package_scene(const PhysicsCollisionPackageAuthoringDesc& desc);
[[nodiscard]] PhysicsCollisionPackageAuthoringResult
write_physics_collision_package_scene(IFileSystem& filesystem, const PhysicsCollisionPackageAuthoringDesc& desc);
[[nodiscard]] PhysicsCollisionPackageVerificationResult
verify_physics_collision_package_scene(const AssetCookedPackageIndex& index, AssetId collision_asset,
                                       std::string_view collision_content);
[[nodiscard]] PhysicsCollisionPackageUpdateResult
plan_physics_collision_package_update(const PhysicsCollisionPackageUpdateDesc& desc);
[[nodiscard]] PhysicsCollisionPackageUpdateResult
apply_physics_collision_package_update(IFileSystem& filesystem, const PhysicsCollisionPackageApplyDesc& desc);

} // namespace mirakana
