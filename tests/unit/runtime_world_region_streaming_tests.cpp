// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/world_region_streaming.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class CountingFileSystem final : public mirakana::IFileSystem {
  public:
    [[nodiscard]] bool exists(std::string_view path) const override {
        return filesystem_.exists(path);
    }

    [[nodiscard]] bool is_directory(std::string_view path) const override {
        return filesystem_.is_directory(path);
    }

    [[nodiscard]] std::string read_text(std::string_view path) const override {
        ++read_text_count_;
        if (fail_read_text_) {
            throw std::runtime_error("read failed");
        }
        return filesystem_.read_text(path);
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view root) const override {
        return filesystem_.list_files(root);
    }

    void write_text(std::string_view path, std::string_view text) override {
        filesystem_.write_text(path, text);
    }

    void remove(std::string_view path) override {
        filesystem_.remove(path);
    }

    void remove_empty_directory(std::string_view path) override {
        filesystem_.remove_empty_directory(path);
    }

    [[nodiscard]] int read_text_count() const noexcept {
        return read_text_count_;
    }

    void fail_read_text(bool value) noexcept {
        fail_read_text_ = value;
    }

  private:
    mirakana::MemoryFileSystem filesystem_;
    mutable int read_text_count_{0};
    bool fail_read_text_{false};
};

[[nodiscard]] mirakana::runtime::RuntimeWorldRegionPackageDesc
make_region(std::string region_id, std::uint32_t mount_id, std::uint64_t resident_bytes, std::size_t asset_records) {
    const auto package_index_path = "runtime/regions/" + region_id + ".geindex";
    const auto scene_asset_id = mirakana::AssetId::from_name(region_id + "/scene");
    return mirakana::runtime::RuntimeWorldRegionPackageDesc{
        .region_id = region_id,
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = package_index_path,
                .content_root = "runtime",
                .label = region_id,
            },
        .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
        .estimated_resident_bytes = resident_bytes,
        .estimated_asset_records = asset_records,
        .required_preload_assets = {scene_asset_id},
        .resident_resource_kinds = {mirakana::AssetKind::scene},
    };
}

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

void write_region_package(CountingFileSystem& filesystem, std::string_view region_id, mirakana::AssetId asset,
                          mirakana::AssetKind kind, std::string_view path, std::string_view payload,
                          std::uint64_t source_revision = 11) {
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = kind,
                                                                      .path = std::string(path),
                                                                      .content = std::string(payload),
                                                                      .source_revision = source_revision,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/regions/" + std::string(region_id) + ".geindex",
                          mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text("runtime/" + std::string(path), payload);
}

[[nodiscard]] mirakana::runtime::RuntimeResidentCatalogCacheV2
make_refreshed_cache(const mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set,
                     mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget = {}) {
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    MK_REQUIRE(catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget)
                   .succeeded());
    return catalog_cache;
}

void mount_region_scene(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set, std::string region_id,
                        std::uint32_t mount_id) {
    const auto scene = mirakana::AssetId::from_name(region_id + "/scene");
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
                       .label = region_id,
                       .package = make_package(make_record(scene, mirakana::AssetKind::scene,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = mount_id},
                                                           region_id + " scene")),
                   })
                   .succeeded());
}

[[nodiscard]] mirakana::runtime::RuntimeWorldRegionStreamingPlanRequest streaming_request() {
    return mirakana::runtime::RuntimeWorldRegionStreamingPlanRequest{
        .regions =
            {
                make_region("town", 1U, 32U, 2U),
                make_region("forest", 2U, 48U, 3U),
                make_region("dungeon", 3U, 64U, 4U),
            },
        .active_region_ids = {"town"},
        .desired_region_ids = {"forest", "town"},
        .protected_region_ids = {"town"},
        .budget =
            mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                .max_resident_content_bytes = 96U,
                .max_resident_asset_records = 8U,
            },
        .max_resident_regions = 2U,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeWorldRegionStreamingSafePointDesc
make_safe_point_desc(mirakana::runtime::RuntimeWorldRegionStreamingPlan plan,
                     std::vector<mirakana::runtime::RuntimeWorldRegionPackageDesc> regions) {
    return mirakana::runtime::RuntimeWorldRegionStreamingSafePointDesc{
        .plan = std::move(plan),
        .regions = std::move(regions),
        .target_id = "world-region-streaming",
        .runtime_scene_validation_target_id = "runtime-world-region-scene",
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .budget =
            mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                .max_resident_content_bytes = 96U,
                .max_resident_asset_records = 8U,
            },
        .max_resident_packages = 2U,
        .runtime_scene_validation_succeeded = true,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeWorldRegionStreamingPlan& plan,
                                           mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode code) {
    std::size_t count{0};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime world region streaming plan diffs active and desired regions deterministically") {
    using Action = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;

    const auto request = streaming_request();
    const auto plan = mirakana::runtime::plan_runtime_world_region_streaming(request);

    MK_REQUIRE(plan.status == Status::planned);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.rows.size() == 2U);
    MK_REQUIRE(plan.rows[0].action == Action::load_region);
    MK_REQUIRE(plan.rows[0].region_id == "forest");
    MK_REQUIRE(plan.rows[0].package_index_path == "runtime/regions/forest.geindex");
    MK_REQUIRE(plan.rows[0].required_preload_asset_count == 1U);
    MK_REQUIRE(plan.rows[0].resident_resource_kind_count == 1U);
    MK_REQUIRE(plan.rows[1].action == Action::keep_resident);
    MK_REQUIRE(plan.rows[1].region_id == "town");
    MK_REQUIRE(plan.projected_resident_region_count == 2U);
    MK_REQUIRE(plan.projected_resident_bytes == 80U);
    MK_REQUIRE(plan.projected_resident_asset_records == 5U);
    MK_REQUIRE(plan.load_count == 1U);
    MK_REQUIRE(plan.keep_count == 1U);
    MK_REQUIRE(plan.unload_count == 0U);
    MK_REQUIRE(!plan.committed);
}

MK_TEST("runtime world region streaming plan rejects missing duplicates protected unloads and over budget") {
    using Code = mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode;
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;

    auto invalid = streaming_request();
    invalid.regions.push_back(make_region("forest", 4U, 8U, 1U));
    invalid.active_region_ids = {"town", "town"};
    invalid.desired_region_ids = {"missing", "forest", "forest"};
    invalid.protected_region_ids = {"town", "town"};

    const auto invalid_plan = mirakana::runtime::plan_runtime_world_region_streaming(invalid);

    MK_REQUIRE(invalid_plan.status == Status::invalid_request);
    MK_REQUIRE(invalid_plan.rows.empty());
    MK_REQUIRE(invalid_plan.diagnostics.size() == 5U);
    MK_REQUIRE(diagnostic_count(invalid_plan, Code::duplicate_region) == 1U);
    MK_REQUIRE(diagnostic_count(invalid_plan, Code::duplicate_active_region) == 1U);
    MK_REQUIRE(diagnostic_count(invalid_plan, Code::missing_desired_region) == 1U);
    MK_REQUIRE(diagnostic_count(invalid_plan, Code::duplicate_desired_region) == 1U);
    MK_REQUIRE(diagnostic_count(invalid_plan, Code::duplicate_protected_region) == 1U);

    auto protected_unload = streaming_request();
    protected_unload.desired_region_ids = {"forest"};
    const auto protected_plan = mirakana::runtime::plan_runtime_world_region_streaming(protected_unload);

    MK_REQUIRE(protected_plan.status == Status::invalid_request);
    MK_REQUIRE(protected_plan.rows.empty());
    MK_REQUIRE(protected_plan.diagnostics.size() == 1U);
    MK_REQUIRE(protected_plan.diagnostics[0].code == Code::protected_active_region_unload_requested);

    auto over_budget = streaming_request();
    over_budget.budget.max_resident_content_bytes = 64U;
    const auto over_budget_plan = mirakana::runtime::plan_runtime_world_region_streaming(over_budget);

    MK_REQUIRE(over_budget_plan.status == Status::budget_exceeded);
    MK_REQUIRE(over_budget_plan.rows.empty());
    MK_REQUIRE(over_budget_plan.projected_resident_bytes == 80U);
    MK_REQUIRE(over_budget_plan.diagnostics.size() == 1U);
    MK_REQUIRE(over_budget_plan.diagnostics[0].code == Code::resident_content_budget_exceeded);
}

MK_TEST("runtime world region streaming plan rejects invalid and duplicate mount ids before rows") {
    using Code = mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode;
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;

    auto invalid_mount = streaming_request();
    invalid_mount.regions[1].mount_id = {};
    const auto invalid_mount_plan = mirakana::runtime::plan_runtime_world_region_streaming(invalid_mount);

    MK_REQUIRE(invalid_mount_plan.status == Status::invalid_request);
    MK_REQUIRE(invalid_mount_plan.rows.empty());
    MK_REQUIRE(invalid_mount_plan.diagnostics.size() == 1U);
    MK_REQUIRE(invalid_mount_plan.diagnostics[0].code == Code::invalid_mount_id);
    MK_REQUIRE(invalid_mount_plan.diagnostics[0].region_id == "forest");

    auto duplicate_mount = streaming_request();
    duplicate_mount.regions[2].mount_id = duplicate_mount.regions[1].mount_id;
    duplicate_mount.desired_region_ids = {"dungeon", "forest", "town"};
    duplicate_mount.budget.max_resident_content_bytes = 160U;
    duplicate_mount.budget.max_resident_asset_records = 12U;
    duplicate_mount.max_resident_regions = 3U;
    const auto duplicate_mount_plan = mirakana::runtime::plan_runtime_world_region_streaming(duplicate_mount);

    MK_REQUIRE(duplicate_mount_plan.status == Status::invalid_request);
    MK_REQUIRE(duplicate_mount_plan.rows.empty());
    MK_REQUIRE(duplicate_mount_plan.diagnostics.size() == 1U);
    MK_REQUIRE(duplicate_mount_plan.diagnostics[0].code == Code::duplicate_mount_id);
    MK_REQUIRE(duplicate_mount_plan.diagnostics[0].region_id == "dungeon");
}

MK_TEST("runtime world region streaming plan reports no changes and unload rows") {
    using Action = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;

    auto no_changes = streaming_request();
    no_changes.desired_region_ids = no_changes.active_region_ids;
    const auto no_changes_plan = mirakana::runtime::plan_runtime_world_region_streaming(no_changes);

    MK_REQUIRE(no_changes_plan.status == Status::planned);
    MK_REQUIRE(no_changes_plan.diagnostics.empty());
    MK_REQUIRE(no_changes_plan.rows.size() == 1U);
    MK_REQUIRE(no_changes_plan.rows[0].action == Action::keep_resident);
    MK_REQUIRE(no_changes_plan.rows[0].region_id == "town");

    auto unload = streaming_request();
    unload.active_region_ids = {"forest", "town"};
    unload.desired_region_ids = {"town"};
    const auto unload_plan = mirakana::runtime::plan_runtime_world_region_streaming(unload);

    MK_REQUIRE(unload_plan.status == Status::planned);
    MK_REQUIRE(unload_plan.diagnostics.empty());
    MK_REQUIRE(unload_plan.rows.size() == 2U);
    MK_REQUIRE(unload_plan.rows[0].action == Action::unload_region);
    MK_REQUIRE(unload_plan.rows[0].region_id == "forest");
    MK_REQUIRE(unload_plan.rows[1].action == Action::keep_resident);
    MK_REQUIRE(unload_plan.rows[1].region_id == "town");
    MK_REQUIRE(unload_plan.unload_count == 1U);
    MK_REQUIRE(unload_plan.keep_count == 1U);
    MK_REQUIRE(unload_plan.load_count == 0U);

    auto empty = streaming_request();
    empty.active_region_ids.clear();
    empty.desired_region_ids.clear();
    empty.protected_region_ids.clear();
    const auto empty_plan = mirakana::runtime::plan_runtime_world_region_streaming(empty);

    MK_REQUIRE(empty_plan.status == Status::no_changes);
    MK_REQUIRE(empty_plan.succeeded());
    MK_REQUIRE(empty_plan.rows.empty());
}

MK_TEST("runtime world region streaming safe point loads reviewed region packages atomically") {
    using Action = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus;

    CountingFileSystem filesystem;
    const auto town_scene = mirakana::AssetId::from_name("town/scene");
    const auto forest_scene = mirakana::AssetId::from_name("forest/scene");
    write_region_package(filesystem, "forest", forest_scene, mirakana::AssetKind::scene, "regions/forest/scene.scene",
                         "forest scene");

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                .label = "town",
                .package = make_package(make_record(town_scene, mirakana::AssetKind::scene,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1U}, "town scene")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    const auto request = streaming_request();
    const auto plan = mirakana::runtime::plan_runtime_world_region_streaming(request);
    auto desc = make_safe_point_desc(plan, request.regions);

    const auto result = mirakana::runtime::execute_runtime_world_region_streaming_safe_point(filesystem, mount_set,
                                                                                             catalog_cache, desc);

    MK_REQUIRE(result.status == Status::completed);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.rows.size() == 2U);
    MK_REQUIRE(result.load_count == 1U);
    MK_REQUIRE(result.keep_count == 1U);
    MK_REQUIRE(result.unload_count == 0U);
    MK_REQUIRE(result.committed_count == 1U);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(result.rows[0].action == Action::load_region);
    MK_REQUIRE(result.rows[0].region_id == "forest");
    MK_REQUIRE(result.rows[0].streaming.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.rows[0].streaming.committed);
    MK_REQUIRE(result.rows[1].action == Action::keep_resident);
    MK_REQUIRE(mount_set.generation() != previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2U);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), town_scene).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), forest_scene).has_value());
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime world region streaming safe point fails preflight without package reads or live mutation") {
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus;

    CountingFileSystem filesystem;
    const auto town_scene = mirakana::AssetId::from_name("town/scene");
    const auto forest_scene = mirakana::AssetId::from_name("forest/scene");
    write_region_package(filesystem, "forest", forest_scene, mirakana::AssetKind::scene, "regions/forest/scene.scene",
                         "forest scene");

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                .label = "town",
                .package = make_package(make_record(town_scene, mirakana::AssetKind::scene,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1U}, "town scene")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto request = streaming_request();
    auto desc = make_safe_point_desc(mirakana::runtime::plan_runtime_world_region_streaming(request), request.regions);
    desc.runtime_scene_validation_succeeded = false;

    const auto result = mirakana::runtime::execute_runtime_world_region_streaming_safe_point(filesystem, mount_set,
                                                                                             catalog_cache, desc);

    MK_REQUIRE(result.status == Status::failed);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.rows.size() == 1U);
    MK_REQUIRE(result.rows[0].streaming.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime world region streaming safe point rejects missing package candidates before reads") {
    using Code = mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode;
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus;

    CountingFileSystem filesystem;
    const auto town_scene = mirakana::AssetId::from_name("town/scene");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                .label = "town",
                .package = make_package(make_record(town_scene, mirakana::AssetKind::scene,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1U}, "town scene")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    const auto request = streaming_request();
    auto desc =
        make_safe_point_desc(mirakana::runtime::plan_runtime_world_region_streaming(request), {request.regions[0]});

    const auto result = mirakana::runtime::execute_runtime_world_region_streaming_safe_point(filesystem, mount_set,
                                                                                             catalog_cache, desc);

    MK_REQUIRE(result.status == Status::invalid_plan);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostics.size() == 1U);
    MK_REQUIRE(result.diagnostics[0].code == Code::missing_package_candidate);
    MK_REQUIRE(result.diagnostics[0].region_id == "forest");
    MK_REQUIRE(result.rows.empty());
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
}

MK_TEST("runtime world region streaming safe point preserves live state when reviewed evictions are insufficient") {
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus;

    CountingFileSystem filesystem;
    const auto town_scene = mirakana::AssetId::from_name("town/scene");
    const auto forest_scene = mirakana::AssetId::from_name("forest/scene");
    write_region_package(filesystem, "forest", forest_scene, mirakana::AssetKind::scene, "regions/forest/scene.scene",
                         "forest scene");

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                       .label = "town",
                       .package = make_package(make_record(town_scene, mirakana::AssetKind::scene,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1U},
                                                           "town scene payload")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto request = streaming_request();
    auto desc = make_safe_point_desc(mirakana::runtime::plan_runtime_world_region_streaming(request), request.regions);
    desc.budget.max_resident_content_bytes = 12U;
    desc.max_resident_packages = 2U;

    const auto result = mirakana::runtime::execute_runtime_world_region_streaming_safe_point(filesystem, mount_set,
                                                                                             catalog_cache, desc);

    MK_REQUIRE(result.status == Status::failed);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.rows.size() == 1U);
    MK_REQUIRE(result.rows[0].streaming.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.rows[0].streaming.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable);
    MK_REQUIRE(!result.rows[0].streaming.committed);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "town scene payload");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), forest_scene).has_value());
}

MK_TEST("runtime world region streaming safe point unloads reviewed inactive regions") {
    using Action = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;
    using Status = mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus;

    CountingFileSystem filesystem;
    const auto town_scene = mirakana::AssetId::from_name("town/scene");
    const auto forest_scene = mirakana::AssetId::from_name("forest/scene");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                .label = "town",
                .package = make_package(make_record(town_scene, mirakana::AssetKind::scene,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1U}, "town scene")),
            })
            .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2U},
                       .label = "forest",
                       .package = make_package(make_record(forest_scene, mirakana::AssetKind::scene,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 2U},
                                                           "forest scene")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    auto request = streaming_request();
    request.active_region_ids = {"forest", "town"};
    request.desired_region_ids = {"town"};
    const auto plan = mirakana::runtime::plan_runtime_world_region_streaming(request);
    auto desc = make_safe_point_desc(plan, request.regions);

    const auto result = mirakana::runtime::execute_runtime_world_region_streaming_safe_point(filesystem, mount_set,
                                                                                             catalog_cache, desc);

    MK_REQUIRE(result.status == Status::completed);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.rows.size() == 2U);
    MK_REQUIRE(result.rows[0].action == Action::unload_region);
    MK_REQUIRE(result.rows[0].region_id == "forest");
    MK_REQUIRE(result.rows[0].streaming.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.rows[1].action == Action::keep_resident);
    MK_REQUIRE(result.unload_count == 1U);
    MK_REQUIRE(result.keep_count == 1U);
    MK_REQUIRE(result.committed_count == 1U);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() != previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1U);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U});
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), town_scene).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), forest_scene).has_value());
}

MK_TEST("runtime world region navigation refs review reports residency without package reads") {
    using Code = mirakana::runtime::RuntimeWorldRegionNavigationDiagnosticCode;
    using Status = mirakana::runtime::RuntimeWorldRegionNavigationReviewStatus;

    CountingFileSystem filesystem;
    const auto town_scene = mirakana::AssetId::from_name("town/scene");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                .label = "town",
                .package = make_package(make_record(town_scene, mirakana::AssetKind::scene,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1U}, "town scene")),
            })
            .succeeded());

    const auto request = streaming_request();
    const auto review = mirakana::runtime::review_runtime_world_region_navigation_refs(
        mount_set, mirakana::runtime::RuntimeWorldRegionNavigationRefReviewRequest{
                       .regions = request.regions,
                       .route_region_ids = {"town", "forest"},
                   });

    MK_REQUIRE(review.status == Status::not_resident);
    MK_REQUIRE(!review.succeeded());
    MK_REQUIRE(review.rows.size() == 2U);
    MK_REQUIRE(review.rows[0].region_id == "town");
    MK_REQUIRE(review.rows[0].resident);
    MK_REQUIRE(review.rows[1].region_id == "forest");
    MK_REQUIRE(!review.rows[1].resident);
    MK_REQUIRE(review.resident_region_count == 1U);
    MK_REQUIRE(review.missing_resident_region_count == 1U);
    MK_REQUIRE(review.diagnostics.size() == 1U);
    MK_REQUIRE(review.diagnostics[0].code == Code::route_region_not_resident);
    MK_REQUIRE(review.diagnostics[0].region_id == "forest");
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime world region navigation refs review rejects duplicate route and missing catalog regions") {
    using Code = mirakana::runtime::RuntimeWorldRegionNavigationDiagnosticCode;
    using Status = mirakana::runtime::RuntimeWorldRegionNavigationReviewStatus;

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    const auto request = streaming_request();

    const auto review = mirakana::runtime::review_runtime_world_region_navigation_refs(
        mount_set, mirakana::runtime::RuntimeWorldRegionNavigationRefReviewRequest{
                       .regions = request.regions,
                       .route_region_ids = {"town", "missing", "town"},
                   });

    MK_REQUIRE(review.status == Status::invalid_request);
    MK_REQUIRE(!review.succeeded());
    MK_REQUIRE(review.rows.empty());
    MK_REQUIRE(review.diagnostics.size() == 2U);
    MK_REQUIRE(review.diagnostics[0].code == Code::missing_region_package);
    MK_REQUIRE(review.diagnostics[0].region_id == "missing");
    MK_REQUIRE(review.diagnostics[1].code == Code::duplicate_route_region);
    MK_REQUIRE(review.diagnostics[1].region_id == "town");

    const auto empty_route = mirakana::runtime::review_runtime_world_region_navigation_refs(
        mount_set, mirakana::runtime::RuntimeWorldRegionNavigationRefReviewRequest{
                       .regions = request.regions,
                       .route_region_ids = {},
                   });

    MK_REQUIRE(empty_route.status == Status::invalid_request);
    MK_REQUIRE(!empty_route.succeeded());
    MK_REQUIRE(empty_route.rows.empty());
    MK_REQUIRE(empty_route.diagnostics.size() == 1U);
    MK_REQUIRE(empty_route.diagnostics[0].code == Code::empty_route);
}

MK_TEST("runtime world region navigation path cache review rejects malformed routes and stale catalog caches") {
    using Code = mirakana::runtime::RuntimeWorldRegionNavigationDiagnosticCode;
    using Status = mirakana::runtime::RuntimeWorldRegionNavigationReviewStatus;

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_region_scene(mount_set, "town", 1U);
    mount_region_scene(mount_set, "forest", 2U);
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto request = streaming_request();

    const mirakana::runtime::RuntimeWorldRegionNavigationPathCacheEntry matching_cache{
        .region_path = {"town", "forest"},
        .portal_path = {"road"},
        .mount_generation = mount_set.generation(),
        .catalog_generation = catalog_cache.catalog().generation(),
    };
    const auto malformed = mirakana::runtime::review_runtime_world_region_navigation_path_cache(
        mount_set, catalog_cache,
        mirakana::runtime::RuntimeWorldRegionNavigationPathCacheReviewRequest{
            .regions = request.regions,
            .route_region_ids = {"town", "forest"},
            .route_portal_ids = {},
            .cache = matching_cache,
        });

    MK_REQUIRE(malformed.status == Status::invalid_request);
    MK_REQUIRE(!malformed.succeeded());
    MK_REQUIRE(malformed.cache_ready == false);
    MK_REQUIRE(malformed.diagnostics.size() == 1U);
    MK_REQUIRE(malformed.diagnostics[0].code == Code::route_portal_count_mismatch);

    mount_region_scene(mount_set, "dungeon", 3U);
    auto stale_catalog_cache = matching_cache;
    stale_catalog_cache.mount_generation = mount_set.generation();
    const auto stale = mirakana::runtime::review_runtime_world_region_navigation_path_cache(
        mount_set, catalog_cache,
        mirakana::runtime::RuntimeWorldRegionNavigationPathCacheReviewRequest{
            .regions = request.regions,
            .route_region_ids = {"town", "forest"},
            .route_portal_ids = {"road"},
            .cache = stale_catalog_cache,
        });

    MK_REQUIRE(stale.status == Status::stale);
    MK_REQUIRE(!stale.succeeded());
    MK_REQUIRE(stale.cache_ready == false);
    MK_REQUIRE(stale.diagnostics.size() == 1U);
    MK_REQUIRE(stale.diagnostics[0].code == Code::catalog_cache_not_ready);
    MK_REQUIRE(stale.current_mount_generation == mount_set.generation());
    MK_REQUIRE(stale.current_catalog_generation == catalog_cache.catalog().generation());
}

MK_TEST("runtime world region navigation path cache review fails closed on stale generations") {
    using Code = mirakana::runtime::RuntimeWorldRegionNavigationDiagnosticCode;
    using Status = mirakana::runtime::RuntimeWorldRegionNavigationReviewStatus;

    const auto town_scene = mirakana::AssetId::from_name("town/scene");
    const auto forest_scene = mirakana::AssetId::from_name("forest/scene");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                .label = "town",
                .package = make_package(make_record(town_scene, mirakana::AssetKind::scene,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1U}, "town scene")),
            })
            .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2U},
                       .label = "forest",
                       .package = make_package(make_record(forest_scene, mirakana::AssetKind::scene,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 2U},
                                                           "forest scene")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto request = streaming_request();
    const mirakana::runtime::RuntimeWorldRegionNavigationPathCacheEntry cache{
        .region_path = {"town", "forest"},
        .portal_path = {"road"},
        .mount_generation = mount_set.generation() - 1U,
        .catalog_generation = catalog_cache.catalog().generation(),
    };

    const auto stale = mirakana::runtime::review_runtime_world_region_navigation_path_cache(
        mount_set, catalog_cache,
        mirakana::runtime::RuntimeWorldRegionNavigationPathCacheReviewRequest{
            .regions = request.regions,
            .route_region_ids = {"town", "forest"},
            .route_portal_ids = {"road"},
            .cache = cache,
        });

    MK_REQUIRE(stale.status == Status::stale);
    MK_REQUIRE(!stale.succeeded());
    MK_REQUIRE(stale.cache_ready == false);
    MK_REQUIRE(stale.diagnostics.size() == 1U);
    MK_REQUIRE(stale.diagnostics[0].code == Code::mount_generation_mismatch);
    MK_REQUIRE(stale.current_mount_generation == mount_set.generation());
    MK_REQUIRE(stale.current_catalog_generation == catalog_cache.catalog().generation());

    auto ready_cache = cache;
    ready_cache.mount_generation = mount_set.generation();
    const auto ready = mirakana::runtime::review_runtime_world_region_navigation_path_cache(
        mount_set, catalog_cache,
        mirakana::runtime::RuntimeWorldRegionNavigationPathCacheReviewRequest{
            .regions = request.regions,
            .route_region_ids = {"town", "forest"},
            .route_portal_ids = {"road"},
            .cache = ready_cache,
        });

    MK_REQUIRE(ready.status == Status::ready);
    MK_REQUIRE(ready.succeeded());
    MK_REQUIRE(ready.cache_ready);
    MK_REQUIRE(ready.diagnostics.empty());
    MK_REQUIRE(ready.resident_region_count == 2U);
}

int main() {
    return mirakana::test::run_all();
}
