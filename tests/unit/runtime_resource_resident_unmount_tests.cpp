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

MK_TEST("runtime resident package eviction plan is no op when current view is within budget") {
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
    const auto previous_mount_generation = mount_set.generation();

    const mirakana::runtime::RuntimeResidentPackageEvictionPlanDescV2 desc{
        .target_budget = {.max_resident_content_bytes = 64, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}},
        .protected_mount_ids = {},
    };

    const auto plan = mirakana::runtime::plan_runtime_resident_package_evictions_v2(mount_set, desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required);
    MK_REQUIRE(plan.steps.empty());
    MK_REQUIRE(plan.current_refresh.succeeded());
    MK_REQUIRE(plan.projected_refresh.succeeded());
    MK_REQUIRE(plan.previous_mount_count == 1);
    MK_REQUIRE(plan.projected_mount_count == 1);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
}

MK_TEST("runtime resident package eviction plan returns reviewed candidate order until budget passes") {
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto overlay = mirakana::AssetId::from_name("materials/overlay");
    const auto transient = mirakana::AssetId::from_name("meshes/transient");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(base, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "aaaaaa"),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "overlay",
                       .package = make_runtime_package(overlay, mirakana::AssetKind::material,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 2}, "bbbb"),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                       .label = "transient",
                       .package = make_runtime_package(transient, mirakana::AssetKind::mesh,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 3}, "cc"),
                   })
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();

    const mirakana::runtime::RuntimeResidentPackageEvictionPlanDescV2 desc{
        .target_budget = {.max_resident_content_bytes = 6, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                                    mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}},
        .protected_mount_ids = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}},
    };

    const auto plan = mirakana::runtime::plan_runtime_resident_package_evictions_v2(mount_set, desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned);
    MK_REQUIRE(plan.current_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(plan.steps.size() == 2);
    MK_REQUIRE(plan.steps[0].mount_id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3});
    MK_REQUIRE(plan.steps[0].catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(plan.steps[1].mount_id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(plan.steps[1].catalog_refresh.succeeded());
    MK_REQUIRE(plan.projected_refresh.succeeded());
    MK_REQUIRE(plan.projected_refresh.budget_execution.estimated_resident_content_bytes == 6);
    MK_REQUIRE(plan.projected_refresh.budget_execution.resident_asset_record_count == 1);
    MK_REQUIRE(plan.previous_mount_count == 3);
    MK_REQUIRE(plan.projected_mount_count == 1);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 3);
}

MK_TEST("runtime resident package eviction plan rejects protected candidates before partial planning") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "protected",
                       .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "texture"),
                   })
                   .succeeded());

    const mirakana::runtime::RuntimeResidentPackageEvictionPlanDescV2 desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}},
        .protected_mount_ids = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}},
    };

    const auto plan = mirakana::runtime::plan_runtime_resident_package_evictions_v2(mount_set, desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id);
    MK_REQUIRE(plan.steps.empty());
    MK_REQUIRE(plan.diagnostics.size() == 1);
    MK_REQUIRE(plan.diagnostics[0].code == "protected-candidate-mount-id");
    MK_REQUIRE(plan.diagnostics[0].mount == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(mount_set.mounts().size() == 1);
}

MK_TEST("runtime resident package eviction plan rejects duplicate and missing candidates before partial planning") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                       .label = "base",
                       .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "texture"),
                   })
                   .succeeded());

    const mirakana::runtime::RuntimeResidentPackageEvictionPlanDescV2 duplicate_desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                                    mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}},
        .protected_mount_ids = {},
    };
    const auto duplicate = mirakana::runtime::plan_runtime_resident_package_evictions_v2(mount_set, duplicate_desc);
    MK_REQUIRE(!duplicate.succeeded());
    MK_REQUIRE(duplicate.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::duplicate_candidate_mount_id);
    MK_REQUIRE(duplicate.steps.empty());
    MK_REQUIRE(duplicate.diagnostics[0].code == "duplicate-candidate-mount-id");

    const mirakana::runtime::RuntimeResidentPackageEvictionPlanDescV2 missing_desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99}},
        .protected_mount_ids = {},
    };
    const auto missing = mirakana::runtime::plan_runtime_resident_package_evictions_v2(mount_set, missing_desc);
    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::missing_candidate_mount_id);
    MK_REQUIRE(missing.steps.empty());
    MK_REQUIRE(missing.diagnostics[0].code == "missing-candidate-mount-id");
    MK_REQUIRE(mount_set.mounts().size() == 1);
}

MK_TEST("runtime resident package eviction plan reports unreachable budget without mutating mounts") {
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto transient = mirakana::AssetId::from_name("meshes/transient");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(base, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "aaaaaa"),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "transient",
                       .package = make_runtime_package(transient, mirakana::AssetKind::mesh,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 2}, "bb"),
                   })
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();

    const mirakana::runtime::RuntimeResidentPackageEvictionPlanDescV2 desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}},
        .protected_mount_ids = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}},
    };

    const auto plan = mirakana::runtime::plan_runtime_resident_package_evictions_v2(mount_set, desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable);
    MK_REQUIRE(plan.steps.size() == 1);
    MK_REQUIRE(plan.steps[0].mount_id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(plan.projected_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(plan.projected_mount_count == 1);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
}

MK_TEST("runtime resident package reviewed eviction commit applies reviewed candidates atomically") {
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto overlay = mirakana::AssetId::from_name("materials/overlay");
    const auto transient = mirakana::AssetId::from_name("meshes/transient");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(base, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "aaaaaa"),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "overlay",
                       .package = make_runtime_package(overlay, mirakana::AssetKind::material,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 2}, "bbbb"),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                       .label = "transient",
                       .package = make_runtime_package(transient, mirakana::AssetKind::mesh,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 3}, "cc"),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 previous_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 3,
    };
    MK_REQUIRE(catalog_cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, previous_budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();

    const mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitDescV2 desc{
        .target_budget = {.max_resident_content_bytes = 6, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                                    mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}},
        .protected_mount_ids = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}},
    };

    const auto result =
        mirakana::runtime::commit_runtime_resident_package_reviewed_evictions_v2(mount_set, catalog_cache, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitStatusV2::committed);
    MK_REQUIRE(result.eviction_plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.previous_mount_generation == previous_mount_generation);
    MK_REQUIRE(result.mount_generation == mount_set.generation());
    MK_REQUIRE(result.mount_generation != previous_mount_generation);
    MK_REQUIRE(result.previous_mount_count == 3);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(result.evicted_mount_count == 2);
    MK_REQUIRE(result.projected_resident_bytes == 6);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), overlay).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), transient).has_value());
}

MK_TEST("runtime resident package reviewed eviction commit succeeds as no op when current view fits") {
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
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget)
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();

    const mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitDescV2 desc{
        .target_budget = budget,
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}},
        .protected_mount_ids = {},
    };

    const auto result =
        mirakana::runtime::commit_runtime_resident_package_reviewed_evictions_v2(mount_set, catalog_cache, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitStatusV2::no_eviction_required);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::cache_hit);
    MK_REQUIRE(result.evicted_mount_count == 0);
    MK_REQUIRE(result.previous_mount_count == 1);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(result.mount_generation == previous_mount_generation);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
}

MK_TEST("runtime resident package reviewed eviction commit rejects reviewed candidates before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                       .label = "base",
                       .package = make_runtime_package(texture, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "texture"),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitDescV2 invalid_desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{}},
        .protected_mount_ids = {},
    };
    const auto invalid = mirakana::runtime::commit_runtime_resident_package_reviewed_evictions_v2(
        mount_set, catalog_cache, invalid_desc);

    const mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitDescV2 duplicate_desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                                    mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}},
        .protected_mount_ids = {},
    };
    const auto duplicate = mirakana::runtime::commit_runtime_resident_package_reviewed_evictions_v2(
        mount_set, catalog_cache, duplicate_desc);

    const mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitDescV2 missing_desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99}},
        .protected_mount_ids = {},
    };
    const auto missing = mirakana::runtime::commit_runtime_resident_package_reviewed_evictions_v2(
        mount_set, catalog_cache, missing_desc);

    const mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitDescV2 protected_desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}},
        .protected_mount_ids = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}},
    };
    const auto protected_result = mirakana::runtime::commit_runtime_resident_package_reviewed_evictions_v2(
        mount_set, catalog_cache, protected_desc);

    MK_REQUIRE(invalid.status ==
               mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitStatusV2::invalid_candidate_mount_id);
    MK_REQUIRE(duplicate.status ==
               mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitStatusV2::duplicate_candidate_mount_id);
    MK_REQUIRE(missing.status ==
               mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitStatusV2::missing_candidate_mount_id);
    MK_REQUIRE(protected_result.status ==
               mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitStatusV2::protected_candidate_mount_id);
    MK_REQUIRE(!invalid.committed);
    MK_REQUIRE(!duplicate.committed);
    MK_REQUIRE(!missing.committed);
    MK_REQUIRE(!protected_result.committed);
    MK_REQUIRE(invalid.diagnostics[0].code == "invalid-candidate-mount-id");
    MK_REQUIRE(duplicate.diagnostics[0].code == "duplicate-candidate-mount-id");
    MK_REQUIRE(missing.diagnostics[0].code == "missing-candidate-mount-id");
    MK_REQUIRE(protected_result.diagnostics[0].code == "protected-candidate-mount-id");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime resident package reviewed eviction commit preserves state when candidates are insufficient") {
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto transient = mirakana::AssetId::from_name("meshes/transient");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_runtime_package(base, mirakana::AssetKind::texture,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 1}, "aaaaaa"),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "transient",
                       .package = make_runtime_package(transient, mirakana::AssetKind::mesh,
                                                       mirakana::runtime::RuntimeAssetHandle{.value = 2}, "bb"),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitDescV2 desc{
        .target_budget = {.max_resident_content_bytes = 1, .max_resident_asset_records = 1},
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .candidate_unmount_order = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}},
        .protected_mount_ids = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}},
    };

    const auto result =
        mirakana::runtime::commit_runtime_resident_package_reviewed_evictions_v2(mount_set, catalog_cache, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimeResidentPackageReviewedEvictionCommitStatusV2::budget_failed);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1);
    MK_REQUIRE(result.evicted_mount_count == 0);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

int main() {
    return mirakana::test::run_all();
}
