// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord make_record(mirakana::AssetId asset, mirakana::AssetKind kind,
                                                                mirakana::runtime::RuntimeAssetHandle handle,
                                                                std::uint64_t content_hash,
                                                                std::uint64_t source_revision, std::string content) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = "assets/" + std::to_string(asset.value) + ".geasset",
        .content_hash = content_hash,
        .source_revision = source_revision,
        .dependencies = {},
        .content = std::move(content),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_package(mirakana::runtime::RuntimeAssetRecord record) {
    return mirakana::runtime::RuntimeAssetPackage({std::move(record)});
}

} // namespace

MK_TEST("runtime resident package replacement commit preserves mount slot and refreshes catalog cache") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, 10, 1,
                                                           "base v1")),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "overlay",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 2}, 20, 1,
                                                           "overlay")),
                   })
                   .succeeded());

    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::first_mount_wins, budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto stale_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture);
    MK_REQUIRE(stale_handle.has_value());

    const auto result = mirakana::runtime::commit_runtime_resident_package_replace_v2(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
        make_package(make_record(texture, mirakana::AssetKind::texture,
                                 mirakana::runtime::RuntimeAssetHandle{.value = 3}, 100, 2, "base v2")),
        mirakana::runtime::RuntimePackageMountOverlay::first_mount_wins, budget);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::replaced);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.previous_mount_generation == previous_mount_generation);
    MK_REQUIRE(result.mount_generation == mount_set.generation());
    MK_REQUIRE(result.mount_generation != previous_mount_generation);
    MK_REQUIRE(result.mounted_package_count == 2);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(mount_set.mounts()[0].label == "base");
    MK_REQUIRE(mount_set.mounts()[1].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(catalog_cache.catalog().generation() != previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog_cache.catalog(), *stale_handle));

    const auto refreshed_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture);
    MK_REQUIRE(refreshed_handle.has_value());
    const auto* refreshed_record =
        mirakana::runtime::runtime_resource_record_v2(catalog_cache.catalog(), *refreshed_handle);
    MK_REQUIRE(refreshed_record != nullptr);
    MK_REQUIRE(refreshed_record->content_hash == 100);
    MK_REQUIRE(refreshed_record->source_revision == 2);
}

MK_TEST("runtime resident package replacement commit rejects invalid and missing ids before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, 10, 1, "base")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{};
    MK_REQUIRE(catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid = mirakana::runtime::commit_runtime_resident_package_replace_v2(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{},
        make_package(make_record(texture, mirakana::AssetKind::texture,
                                 mirakana::runtime::RuntimeAssetHandle{.value = 2}, 20, 2, "invalid")),
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id);
    MK_REQUIRE(!invalid.invoked_candidate_catalog_build);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);

    const auto missing = mirakana::runtime::commit_runtime_resident_package_replace_v2(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9},
        make_package(make_record(texture, mirakana::AssetKind::texture,
                                 mirakana::runtime::RuntimeAssetHandle{.value = 3}, 30, 3, "missing")),
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id);
    MK_REQUIRE(!missing.invoked_candidate_catalog_build);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime resident package replacement commit rejects duplicate candidate records before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, 10, 1, "base")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{};
    MK_REQUIRE(catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_resident_package_replace_v2(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
        mirakana::runtime::RuntimeAssetPackage({
            make_record(texture, mirakana::AssetKind::texture, mirakana::runtime::RuntimeAssetHandle{.value = 2}, 20, 2,
                        "replacement"),
            make_record(texture, mirakana::AssetKind::texture, mirakana::runtime::RuntimeAssetHandle{.value = 3}, 30, 3,
                        "duplicate"),
        }),
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::catalog_build_failed);
    MK_REQUIRE(result.invoked_candidate_catalog_build);
    MK_REQUIRE(result.candidate_catalog_build.diagnostics.size() == 1);
    MK_REQUIRE(!result.catalog_refresh.invoked_catalog_build);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime resident package replacement commit preserves state on projected budget failure") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, 10, 1, "base")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 generous_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(catalog_cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, generous_budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 tight_budget{
        .max_resident_content_bytes = 4,
        .max_resident_asset_records = 1,
    };
    const auto result = mirakana::runtime::commit_runtime_resident_package_replace_v2(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
        make_package(make_record(texture, mirakana::AssetKind::texture,
                                 mirakana::runtime::RuntimeAssetHandle{.value = 2}, 20, 2,
                                 "replacement that exceeds budget")),
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, tight_budget);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::budget_failed);
    MK_REQUIRE(result.invoked_candidate_catalog_build);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(!result.catalog_refresh.invoked_catalog_build);
    MK_REQUIRE(result.previous_mount_generation == previous_mount_generation);
    MK_REQUIRE(result.mount_generation == previous_mount_generation);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content_hash == 10);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

int main() {
    return mirakana::test::run_all();
}
