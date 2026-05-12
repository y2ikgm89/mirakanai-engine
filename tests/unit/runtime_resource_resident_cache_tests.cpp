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

MK_TEST("runtime resident catalog cache reuses catalog for unchanged mount generation and budget") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "abcd"),
                   })
                   .succeeded());

    mirakana::runtime::RuntimeResidentCatalogCacheV2 cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = 16,
        .max_resident_asset_records = 1,
    };
    const auto first = cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(first.invoked_catalog_build);
    MK_REQUIRE(!first.reused_cache);
    MK_REQUIRE(first.mounted_package_count == 1);
    MK_REQUIRE(first.mount_generation == mount_set.generation());
    MK_REQUIRE(cache.has_value());
    const auto first_generation = cache.catalog().generation();
    const auto handle = mirakana::runtime::find_runtime_resource_v2(cache.catalog(), texture);
    MK_REQUIRE(handle.has_value());

    const auto second =
        cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(second.succeeded());
    MK_REQUIRE(second.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::cache_hit);
    MK_REQUIRE(!second.invoked_catalog_build);
    MK_REQUIRE(second.reused_cache);
    MK_REQUIRE(second.catalog_generation == first_generation);
    MK_REQUIRE(cache.catalog().generation() == first_generation);
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(cache.catalog(), *handle));
}

MK_TEST("runtime resident catalog cache rebuilds when mount set generation changes") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "tex"),
                   })
                   .succeeded());

    mirakana::runtime::RuntimeResidentCatalogCacheV2 cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{};
    MK_REQUIRE(
        cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget).succeeded());
    const auto stale = mirakana::runtime::find_runtime_resource_v2(cache.catalog(), texture);
    MK_REQUIRE(stale.has_value());

    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "overlay",
                       .package = make_runtime_package(material, mirakana::AssetKind::material,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 2}, "mat"),
                   })
                   .succeeded());
    const auto rebuild =
        cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(rebuild.succeeded());
    MK_REQUIRE(rebuild.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(rebuild.invoked_catalog_build);
    MK_REQUIRE(rebuild.previous_catalog_generation != rebuild.catalog_generation);
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(cache.catalog(), *stale));
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(cache.catalog(), texture).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(cache.catalog(), material).has_value());
}

MK_TEST("runtime resident catalog cache rejects budget changes without replacing cached catalog") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "abcd"),
                   })
                   .succeeded());

    mirakana::runtime::RuntimeResidentCatalogCacheV2 cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 generous{
        .max_resident_content_bytes = 16,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(
        cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, generous).succeeded());
    const auto cached_generation = cache.catalog().generation();
    const auto cached_handle = mirakana::runtime::find_runtime_resource_v2(cache.catalog(), texture);
    MK_REQUIRE(cached_handle.has_value());

    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 tight{
        .max_resident_content_bytes = 2,
        .max_resident_asset_records = 1,
    };
    const auto failed = cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, tight);

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failed.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(!failed.invoked_catalog_build);
    MK_REQUIRE(!failed.reused_cache);
    MK_REQUIRE(!failed.budget_execution.within_budget);
    MK_REQUIRE(failed.budget_execution.diagnostics.size() == 1);
    MK_REQUIRE(cache.catalog().generation() == cached_generation);
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(cache.catalog(), *cached_handle));
}

int main() {
    return mirakana::test::run_all();
}
