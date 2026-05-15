// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/resource_runtime.hpp"

#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimePackageHotReloadCandidateReviewRowV2
make_review_row(std::string index_path = "runtime/packages/ui.geindex", std::string content_root = "runtime/ui",
                std::string label = "ui") {
    return mirakana::runtime::RuntimePackageHotReloadCandidateReviewRowV2{
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = std::move(index_path),
                .content_root = std::move(content_root),
                .label = std::move(label),
            },
        .matched_changes =
            {
                mirakana::runtime::RuntimePackageHotReloadCandidateReviewChangeV2{
                    .path = "runtime/ui/hud.material",
                    .kind = mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::content,
                },
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewDescV2 make_valid_desc() {
    return mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewDescV2{
        .selected_candidate = make_review_row(),
        .discovery =
            mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{
                .root = "runtime/packages/",
                .content_root = "runtime/ui/",
            },
        .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
        .reviewed_existing_mount_ids =
            {
                mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
                mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11},
                mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12},
            },
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::first_mount_wins,
        .budget =
            mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                .max_resident_content_bytes = 4096U,
                .max_resident_asset_records = 12U,
            },
        .eviction_candidate_unmount_order =
            {
                mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11},
                mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12},
            },
        .protected_mount_ids =
            {
                mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
            },
    };
}

} // namespace

MK_TEST("runtime package hot reload replacement intent review builds safe point descriptor") {
    const auto result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(make_valid_desc());

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::review_ready);
    MK_REQUIRE(result.replacement_desc.discovery.root == "runtime/packages");
    MK_REQUIRE(result.replacement_desc.discovery.content_root == "runtime/ui");
    MK_REQUIRE(result.replacement_desc.selected_package_index_path == "runtime/packages/ui.geindex");
    MK_REQUIRE(result.replacement_desc.mount_id.value == 7);
    MK_REQUIRE(result.replacement_desc.overlay == mirakana::runtime::RuntimePackageMountOverlay::first_mount_wins);
    MK_REQUIRE(result.replacement_desc.budget.max_resident_content_bytes.value() == 4096U);
    MK_REQUIRE(result.replacement_desc.budget.max_resident_asset_records.value() == 12U);
    MK_REQUIRE(result.replacement_desc.eviction_candidate_unmount_order.size() == 2);
    MK_REQUIRE(result.replacement_desc.eviction_candidate_unmount_order[0].value == 11);
    MK_REQUIRE(result.replacement_desc.eviction_candidate_unmount_order[1].value == 12);
    MK_REQUIRE(result.replacement_desc.protected_mount_ids.size() == 1);
    MK_REQUIRE(result.replacement_desc.protected_mount_ids[0].value == 7);
    MK_REQUIRE(result.matched_change_count == 1);
    MK_REQUIRE(result.eviction_candidate_count == 2);
    MK_REQUIRE(result.protected_mount_count == 1);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(!result.invoked_discovery);
    MK_REQUIRE(!result.invoked_package_load);
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(!result.committed);
}

MK_TEST("runtime package hot reload replacement intent review rejects invalid candidate rows") {
    auto invalid_candidate = make_valid_desc();
    invalid_candidate.selected_candidate = make_review_row("runtime/packages/ui.txt", "runtime/ui", "ui");

    const auto invalid_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(invalid_candidate);

    MK_REQUIRE(!invalid_result.succeeded());
    MK_REQUIRE(invalid_result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::invalid_candidate);
    MK_REQUIRE(invalid_result.diagnostics.size() == 1);
    MK_REQUIRE(invalid_result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhaseV2::candidate);
    MK_REQUIRE(invalid_result.diagnostics[0].code == "invalid-selected-candidate");
    MK_REQUIRE(!invalid_result.invoked_package_load);
    MK_REQUIRE(!invalid_result.committed);

    auto missing_match = make_valid_desc();
    missing_match.selected_candidate.matched_changes.clear();

    const auto missing_match_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(missing_match);

    MK_REQUIRE(!missing_match_result.succeeded());
    MK_REQUIRE(missing_match_result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::missing_matched_change);
    MK_REQUIRE(missing_match_result.diagnostics.size() == 1);
    MK_REQUIRE(missing_match_result.diagnostics[0].code == "missing-matched-change");
    MK_REQUIRE(!missing_match_result.invoked_package_load);
    MK_REQUIRE(!missing_match_result.committed);
}

MK_TEST("runtime package hot reload replacement intent review rejects invalid and missing mount ids") {
    auto invalid_mount = make_valid_desc();
    invalid_mount.mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{};

    const auto invalid_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(invalid_mount);

    MK_REQUIRE(!invalid_result.succeeded());
    MK_REQUIRE(invalid_result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::invalid_mount_id);
    MK_REQUIRE(invalid_result.diagnostics.size() == 1);
    MK_REQUIRE(invalid_result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhaseV2::resident_replace);
    MK_REQUIRE(invalid_result.diagnostics[0].code == "invalid-mount-id");
    MK_REQUIRE(!invalid_result.invoked_discovery);
    MK_REQUIRE(!invalid_result.invoked_package_load);

    auto missing_mount = make_valid_desc();
    missing_mount.mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99};

    const auto missing_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(missing_mount);

    MK_REQUIRE(!missing_result.succeeded());
    MK_REQUIRE(missing_result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::missing_mount_id);
    MK_REQUIRE(missing_result.diagnostics.size() == 1);
    MK_REQUIRE(missing_result.diagnostics[0].code == "missing-mount-id");
    MK_REQUIRE(!missing_result.invoked_discovery);
    MK_REQUIRE(!missing_result.invoked_package_load);
}

MK_TEST("runtime package hot reload replacement intent review rejects unsafe discovery descriptors") {
    auto invalid_root = make_valid_desc();
    invalid_root.discovery.root = "runtime/../packages";

    const auto invalid_root_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(invalid_root);

    MK_REQUIRE(!invalid_root_result.succeeded());
    MK_REQUIRE(invalid_root_result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::invalid_descriptor);
    MK_REQUIRE(invalid_root_result.diagnostics.size() == 1);
    MK_REQUIRE(invalid_root_result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhaseV2::descriptor);
    MK_REQUIRE(invalid_root_result.diagnostics[0].code == "invalid-discovery-root");
    MK_REQUIRE(!invalid_root_result.invoked_discovery);
    MK_REQUIRE(!invalid_root_result.invoked_package_load);

    auto mismatched_root = make_valid_desc();
    mismatched_root.discovery.root = "runtime/characters";

    const auto mismatched_root_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(mismatched_root);

    MK_REQUIRE(!mismatched_root_result.succeeded());
    MK_REQUIRE(mismatched_root_result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::invalid_descriptor);
    MK_REQUIRE(mismatched_root_result.diagnostics[0].code == "candidate-outside-discovery-root");
    MK_REQUIRE(!mismatched_root_result.invoked_discovery);
    MK_REQUIRE(!mismatched_root_result.invoked_package_load);

    auto mismatched_content_root = make_valid_desc();
    mismatched_content_root.discovery.content_root = "runtime/characters";

    const auto mismatched_content_root_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(mismatched_content_root);

    MK_REQUIRE(!mismatched_content_root_result.succeeded());
    MK_REQUIRE(mismatched_content_root_result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::invalid_descriptor);
    MK_REQUIRE(mismatched_content_root_result.diagnostics[0].code == "candidate-content-root-mismatch");
    MK_REQUIRE(!mismatched_content_root_result.invoked_discovery);
    MK_REQUIRE(!mismatched_content_root_result.invoked_package_load);
}

MK_TEST("runtime package hot reload replacement intent review rejects invalid overlay intent") {
    auto invalid_overlay = make_valid_desc();
    invalid_overlay.overlay = static_cast<mirakana::runtime::RuntimePackageMountOverlay>(255);

    const auto invalid_overlay_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(invalid_overlay);

    MK_REQUIRE(!invalid_overlay_result.succeeded());
    MK_REQUIRE(invalid_overlay_result.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::invalid_overlay);
    MK_REQUIRE(invalid_overlay_result.diagnostics.size() == 1);
    MK_REQUIRE(invalid_overlay_result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhaseV2::descriptor);
    MK_REQUIRE(invalid_overlay_result.diagnostics[0].code == "invalid-overlay");
    MK_REQUIRE(!invalid_overlay_result.invoked_discovery);
    MK_REQUIRE(!invalid_overlay_result.invoked_package_load);
}

MK_TEST("runtime package hot reload replacement intent review validates reviewed eviction candidates") {
    auto invalid_eviction = make_valid_desc();
    invalid_eviction.eviction_candidate_unmount_order[0] = mirakana::runtime::RuntimeResidentPackageMountIdV2{};

    const auto invalid_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(invalid_eviction);

    MK_REQUIRE(!invalid_result.succeeded());
    MK_REQUIRE(
        invalid_result.status ==
        mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::invalid_eviction_candidate_mount_id);
    MK_REQUIRE(invalid_result.diagnostics.size() == 1);
    MK_REQUIRE(invalid_result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewDiagnosticPhaseV2::eviction_plan);
    MK_REQUIRE(invalid_result.diagnostics[0].code == "invalid-eviction-candidate-mount-id");

    auto duplicate_eviction = make_valid_desc();
    duplicate_eviction.eviction_candidate_unmount_order[1] =
        mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11};

    const auto duplicate_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(duplicate_eviction);

    MK_REQUIRE(!duplicate_result.succeeded());
    MK_REQUIRE(duplicate_result.status == mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::
                                              duplicate_eviction_candidate_mount_id);
    MK_REQUIRE(duplicate_result.diagnostics[0].code == "duplicate-eviction-candidate-mount-id");

    auto missing_eviction = make_valid_desc();
    missing_eviction.eviction_candidate_unmount_order[1] =
        mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99};

    const auto missing_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(missing_eviction);

    MK_REQUIRE(!missing_result.succeeded());
    MK_REQUIRE(
        missing_result.status ==
        mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::missing_eviction_candidate_mount_id);
    MK_REQUIRE(missing_result.diagnostics[0].code == "missing-eviction-candidate-mount-id");

    auto protected_eviction = make_valid_desc();
    protected_eviction.eviction_candidate_unmount_order[0] =
        mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7};

    const auto protected_result =
        mirakana::runtime::plan_runtime_package_hot_reload_replacement_intent_review_v2(protected_eviction);

    MK_REQUIRE(!protected_result.succeeded());
    MK_REQUIRE(protected_result.status == mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::
                                              protected_eviction_candidate_mount_id);
    MK_REQUIRE(protected_result.diagnostics[0].code == "protected-eviction-candidate-mount-id");
}

int main() {
    return mirakana::test::run_all();
}
