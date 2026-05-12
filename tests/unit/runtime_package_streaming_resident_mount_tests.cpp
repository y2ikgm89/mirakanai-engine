// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord make_record(mirakana::AssetId asset, mirakana::AssetKind kind,
                                                                mirakana::runtime::RuntimeAssetHandle handle,
                                                                std::string content) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = "assets/" + std::to_string(asset.value) + ".geasset",
        .content_hash = asset.value + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = std::move(content),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_package(mirakana::runtime::RuntimeAssetRecord record) {
    return mirakana::runtime::RuntimeAssetPackage({std::move(record)});
}

[[nodiscard]] mirakana::runtime::RuntimePackageStreamingExecutionDesc make_valid_desc(std::uint64_t budget_bytes) {
    return mirakana::runtime::RuntimePackageStreamingExecutionDesc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .content_root = "runtime",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = budget_bytes,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = true,
        .required_preload_assets = {},
        .resident_resource_kinds = {},
        .max_resident_packages = 0,
    };
}

} // namespace

MK_TEST("runtime package streaming resident mount commit mounts package and refreshes resident catalog") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {texture, material};
    desc.resident_resource_kinds = {mirakana::AssetKind::texture, mirakana::AssetKind::material};
    desc.max_resident_packages = 1;

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(texture, mirakana::AssetKind::texture, mirakana::runtime::RuntimeAssetHandle{.value = 1},
                        "replacement texture"),
            make_record(material, mirakana::AssetKind::material, mirakana::runtime::RuntimeAssetHandle{.value = 2},
                        "replacement material"),
        }),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, std::move(loaded_package));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.resident_mount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.resident_catalog_refresh.succeeded());
    MK_REQUIRE(result.resident_catalog_refresh.invoked_catalog_build);
    MK_REQUIRE(result.resident_catalog_refresh.mount_generation == mount_set.generation());
    MK_REQUIRE(result.resident_package_count == 1);
    MK_REQUIRE(result.resident_mount_generation == mount_set.generation());
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime package streaming resident mount commit rejects duplicate mount id before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{};
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = make_package(make_record(material, mirakana::AssetKind::material,
                                            mirakana::runtime::RuntimeAssetHandle{.value = 2}, "material")),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
        std::move(loaded_package));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_mount_failed);
    MK_REQUIRE(result.resident_mount.status ==
               mirakana::runtime::RuntimeResidentPackageMountStatusV2::duplicate_mount_id);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "duplicate-mount-id");
    MK_REQUIRE(result.resident_mount_generation == previous_mount_generation);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime package streaming resident mount commit rejects duplicate records before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{};
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(material, mirakana::AssetKind::material, mirakana::runtime::RuntimeAssetHandle{.value = 2},
                        "material"),
            make_record(material, mirakana::AssetKind::material, mirakana::runtime::RuntimeAssetHandle{.value = 3},
                        "duplicate material"),
        }),
        .failures = {},
    };

    auto desc = make_valid_desc(4096);
    desc.max_resident_packages = 2;

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, std::move(loaded_package));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::catalog_build_failed);
    MK_REQUIRE(result.resident_catalog_refresh.invoked_catalog_build);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "catalog-build-failed");
    MK_REQUIRE(result.resident_mount_generation == previous_mount_generation);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime package streaming resident mount commit preserves catalog on projected budget failure") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto cached_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture);
    MK_REQUIRE(cached_handle.has_value());

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = make_package(make_record(material, mirakana::AssetKind::material,
                                            mirakana::runtime::RuntimeAssetHandle{.value = 2},
                                            "material content that exceeds the projected resident budget")),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(8), std::move(loaded_package));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.resident_package_count == 2);
    MK_REQUIRE(result.estimated_resident_bytes > result.resident_budget_bytes);
    MK_REQUIRE(result.resident_mount_generation == previous_mount_generation);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog_cache.catalog(), *cached_handle));
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

int main() {
    return mirakana::test::run_all();
}
