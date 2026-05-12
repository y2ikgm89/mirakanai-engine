// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <string>
#include <utility>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_runtime_package(mirakana::AssetId asset,
                                                                          mirakana::AssetKind kind,
                                                                          mirakana::runtime::RuntimeAssetHandle handle,
                                                                          std::string content) {
    return mirakana::runtime::RuntimeAssetPackage({
        mirakana::runtime::RuntimeAssetRecord{
            .handle = handle,
            .asset = asset,
            .kind = kind,
            .path = "assets/" + std::to_string(asset.value) + ".geasset",
            .content_hash = asset.value + handle.value,
            .source_revision = handle.value,
            .dependencies = {},
            .content = std::move(content),
        },
    });
}

} // namespace

MK_TEST("runtime resident package unmount commit refreshes catalog cache") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "texture"),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "overlay",
                       .package = make_runtime_package(material, mirakana::AssetKind::material,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 2}, "material"),
                   })
                   .succeeded());

    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 2,
    };
    MK_REQUIRE(catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto stale_texture_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture);
    MK_REQUIRE(stale_texture_handle.has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());

    const auto result = mirakana::runtime::commit_runtime_resident_package_unmount_v2(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeResidentPackageUnmountCommitStatusV2::unmounted);
    MK_REQUIRE(result.unmount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::unmounted);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.previous_mount_generation == previous_mount_generation);
    MK_REQUIRE(result.mount_generation == mount_set.generation());
    MK_REQUIRE(result.mount_generation != previous_mount_generation);
    MK_REQUIRE(result.previous_mount_count == 2);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() != previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog_cache.catalog(), *stale_texture_handle));
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime resident package unmount commit rejects missing id before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "texture"),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{};
    MK_REQUIRE(catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_resident_package_unmount_v2(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeResidentPackageUnmountCommitStatusV2::missing_mount_id);
    MK_REQUIRE(result.unmount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::missing_mount_id);
    MK_REQUIRE(result.previous_mount_generation == previous_mount_generation);
    MK_REQUIRE(result.mount_generation == previous_mount_generation);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime resident package unmount commit preserves state on projected remaining budget failure") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                .label = "base",
                .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                mirakana::runtime::RuntimeAssetHandle{.value = 1}, "texture content"),
            })
            .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "overlay",
                       .package = make_runtime_package(material, mirakana::AssetKind::material,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 2}, "material"),
                   })
                   .succeeded());

    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 generous_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 2,
    };
    MK_REQUIRE(catalog_cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, generous_budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 tight_remaining_budget{
        .max_resident_content_bytes = 2,
        .max_resident_asset_records = 1,
    };
    const auto result = mirakana::runtime::commit_runtime_resident_package_unmount_v2(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, tight_remaining_budget);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeResidentPackageUnmountCommitStatusV2::budget_failed);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(!result.catalog_refresh.invoked_catalog_build);
    MK_REQUIRE(result.previous_mount_generation == previous_mount_generation);
    MK_REQUIRE(result.mount_generation == previous_mount_generation);
    MK_REQUIRE(result.mounted_package_count == 2);
    MK_REQUIRE(result.catalog_refresh.mounted_package_count == 1);
    MK_REQUIRE(result.catalog_refresh.mount_generation != previous_mount_generation);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

int main() {
    return mirakana::test::run_all();
}
