// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/world_region_streaming.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace {

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

int main() {
    return mirakana::test::run_all();
}
