// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2
make_candidate(std::string index_path, std::string content_root, std::string label) {
    return mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
        .package_index_path = std::move(index_path),
        .content_root = std::move(content_root),
        .label = std::move(label),
    };
}

[[nodiscard]] mirakana::AssetHotReloadApplyResult make_recook_result(mirakana::AssetHotReloadApplyResultKind kind,
                                                                     std::uint64_t asset, std::string path,
                                                                     std::string diagnostic = {}) {
    return mirakana::AssetHotReloadApplyResult{
        .kind = kind,
        .asset = mirakana::AssetId{.value = asset},
        .path = std::move(path),
        .requested_revision = 3,
        .active_revision = kind == mirakana::AssetHotReloadApplyResultKind::failed_rolled_back ? 2U : 4U,
        .diagnostic = std::move(diagnostic),
    };
}

[[nodiscard]] bool has_matched_change(const mirakana::runtime::RuntimePackageHotReloadCandidateReviewResultV2& result,
                                      std::string_view path,
                                      mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2 kind) {
    for (const auto& row : result.rows) {
        for (const auto& change : row.matched_changes) {
            if (change.path == path && change.kind == kind) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime package hot reload recook change review maps staged and applied recook outputs") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results =
                {
                    make_recook_result(mirakana::AssetHotReloadApplyResultKind::staged, 11, "runtime/ui/hud.material"),
                    make_recook_result(mirakana::AssetHotReloadApplyResultKind::applied, 12,
                                       "runtime/packages/characters.geindex"),
                    make_recook_result(mirakana::AssetHotReloadApplyResultKind::staged, 11, "runtime/ui/hud.material"),
                },
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters"),
                },
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::review_ready);
    MK_REQUIRE(result.recook_apply_result_count == 3);
    MK_REQUIRE(result.staged_recook_change_count == 2);
    MK_REQUIRE(result.applied_recook_change_count == 1);
    MK_REQUIRE(result.failed_recook_apply_result_count == 0);
    MK_REQUIRE(result.accepted_recook_change_count == 3);
    MK_REQUIRE(result.candidate_review.succeeded());
    MK_REQUIRE(result.candidate_review.rows.size() == 2);
    MK_REQUIRE(has_matched_change(result.candidate_review, "runtime/ui/hud.material",
                                  mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::content));
    MK_REQUIRE(has_matched_change(result.candidate_review, "runtime/packages/characters.geindex",
                                  mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::package_index));
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.invoked_candidate_review);
    MK_REQUIRE(!result.invoked_file_watch);
    MK_REQUIRE(!result.invoked_recook);
    MK_REQUIRE(!result.invoked_package_load);
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(!result.committed);
}

MK_TEST("runtime package hot reload recook change review blocks failed recook rows before candidate review") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results =
                {
                    make_recook_result(mirakana::AssetHotReloadApplyResultKind::failed_rolled_back, 11,
                                       "runtime/ui/hud.material", "texture import failed"),
                },
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::failed_recook_apply_result);
    MK_REQUIRE(result.recook_apply_result_count == 1);
    MK_REQUIRE(result.failed_recook_apply_result_count == 1);
    MK_REQUIRE(result.accepted_recook_change_count == 0);
    MK_REQUIRE(result.candidate_review.rows.empty());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDiagnosticPhaseV2::recook_apply_result);
    MK_REQUIRE(result.diagnostics[0].asset.value == 11);
    MK_REQUIRE(result.diagnostics[0].path == "runtime/ui/hud.material");
    MK_REQUIRE(result.diagnostics[0].code == "recook-failed");
    MK_REQUIRE(!result.invoked_candidate_review);
    MK_REQUIRE(!result.invoked_package_load);
    MK_REQUIRE(!result.invoked_resident_commit);
}

MK_TEST("runtime package hot reload recook change review rejects invalid recook rows before candidate review") {
    const auto unknown_kind = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results =
                {
                    make_recook_result(mirakana::AssetHotReloadApplyResultKind::unknown, 11, "runtime/ui/hud.material"),
                },
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                },
        });

    MK_REQUIRE(!unknown_kind.succeeded());
    MK_REQUIRE(unknown_kind.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::invalid_recook_apply_result);
    MK_REQUIRE(unknown_kind.diagnostics.size() == 1);
    MK_REQUIRE(unknown_kind.diagnostics[0].code == "invalid-recook-result-kind");
    MK_REQUIRE(!unknown_kind.invoked_candidate_review);

    const auto out_of_range_kind = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results =
                {
                    make_recook_result(static_cast<mirakana::AssetHotReloadApplyResultKind>(255), 11,
                                       "runtime/ui/hud.material"),
                },
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                },
        });

    MK_REQUIRE(!out_of_range_kind.succeeded());
    MK_REQUIRE(out_of_range_kind.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::invalid_recook_apply_result);
    MK_REQUIRE(out_of_range_kind.diagnostics.size() == 1);
    MK_REQUIRE(out_of_range_kind.diagnostics[0].code == "invalid-recook-result-kind");
    MK_REQUIRE(!out_of_range_kind.invoked_candidate_review);

    const auto invalid_path = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results =
                {
                    make_recook_result(mirakana::AssetHotReloadApplyResultKind::staged, 11,
                                       "runtime/../ui/hud.material"),
                },
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                },
        });

    MK_REQUIRE(!invalid_path.succeeded());
    MK_REQUIRE(invalid_path.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::invalid_recook_apply_result);
    MK_REQUIRE(invalid_path.diagnostics.size() == 1);
    MK_REQUIRE(invalid_path.diagnostics[0].code == "invalid-recook-result-path");
    MK_REQUIRE(!invalid_path.invoked_candidate_review);
    MK_REQUIRE(!invalid_path.invoked_package_load);
    MK_REQUIRE(!invalid_path.committed);

    const auto zero_asset = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results =
                {
                    make_recook_result(mirakana::AssetHotReloadApplyResultKind::staged, 0, "runtime/ui/hud.material"),
                },
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                },
        });

    MK_REQUIRE(!zero_asset.succeeded());
    MK_REQUIRE(zero_asset.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::invalid_recook_apply_result);
    MK_REQUIRE(zero_asset.diagnostics.size() == 1);
    MK_REQUIRE(zero_asset.diagnostics[0].code == "invalid-recook-apply-result-asset");
    MK_REQUIRE(!zero_asset.invoked_candidate_review);

    auto applied_without_active_revision =
        make_recook_result(mirakana::AssetHotReloadApplyResultKind::applied, 11, "runtime/ui/hud.material");
    applied_without_active_revision.active_revision = 0;
    const auto invalid_revision = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results =
                {
                    applied_without_active_revision,
                },
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                },
        });

    MK_REQUIRE(!invalid_revision.succeeded());
    MK_REQUIRE(invalid_revision.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::invalid_recook_apply_result);
    MK_REQUIRE(invalid_revision.diagnostics.size() == 1);
    MK_REQUIRE(invalid_revision.diagnostics[0].code == "invalid-recook-apply-result-revision");
    MK_REQUIRE(!invalid_revision.invoked_candidate_review);
}

MK_TEST("runtime package hot reload recook change review reports no recook rows without reading packages") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results = {},
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::no_recook_changes);
    MK_REQUIRE(result.recook_apply_result_count == 0);
    MK_REQUIRE(result.accepted_recook_change_count == 0);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(!result.invoked_candidate_review);
    MK_REQUIRE(!result.invoked_recook);
    MK_REQUIRE(!result.invoked_package_load);
    MK_REQUIRE(!result.committed);
}

MK_TEST("runtime package hot reload recook change review surfaces candidate review failures") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_recook_change_review_v2(
        mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDescV2{
            .recook_apply_results =
                {
                    make_recook_result(mirakana::AssetHotReloadApplyResultKind::staged, 11, "runtime/ui/hud.material"),
                },
            .candidates =
                {
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters"),
                },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewStatusV2::candidate_review_failed);
    MK_REQUIRE(result.accepted_recook_change_count == 1);
    MK_REQUIRE(result.candidate_review.status ==
               mirakana::runtime::RuntimePackageHotReloadCandidateReviewStatusV2::no_matches);
    MK_REQUIRE(result.candidate_review.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadRecookChangeReviewDiagnosticPhaseV2::candidate_review);
    MK_REQUIRE(result.diagnostics[0].path == "runtime/ui/hud.material");
    MK_REQUIRE(result.diagnostics[0].code == "unmatched-changed-path");
    MK_REQUIRE(result.invoked_candidate_review);
    MK_REQUIRE(!result.invoked_package_load);
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(!result.committed);
}

int main() {
    return mirakana::test::run_all();
}
