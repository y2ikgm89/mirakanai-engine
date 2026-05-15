// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

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

[[nodiscard]] bool has_matched_change(const mirakana::runtime::RuntimePackageHotReloadCandidateReviewRowV2& row,
                                      std::string_view path,
                                      mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2 kind) {
    for (const auto& change : row.matched_changes) {
        if (change.path == path && change.kind == kind) {
            return true;
        }
    }
    return false;
}

} // namespace

MK_TEST("runtime package hot reload candidate review maps package index paths in stable order") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_candidate_review_v2(
        mirakana::runtime::RuntimePackageHotReloadCandidateReviewDescV2{
            .changed_paths = {"runtime/packages/characters.geindex", "runtime/packages/ui.geindex"},
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters"),
                },
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageHotReloadCandidateReviewStatusV2::review_ready);
    MK_REQUIRE(result.changed_path_count == 2);
    MK_REQUIRE(result.matched_changed_path_count == 2);
    MK_REQUIRE(result.invalid_changed_path_count == 0);
    MK_REQUIRE(result.unmatched_changed_path_count == 0);
    MK_REQUIRE(result.review_candidate_count == 2);
    MK_REQUIRE(result.rows.size() == 2);
    MK_REQUIRE(result.rows[0].candidate.package_index_path == "runtime/packages/characters.geindex");
    MK_REQUIRE(result.rows[1].candidate.package_index_path == "runtime/packages/ui.geindex");
    MK_REQUIRE(has_matched_change(result.rows[0], "runtime/packages/characters.geindex",
                                  mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::package_index));
    MK_REQUIRE(has_matched_change(result.rows[1], "runtime/packages/ui.geindex",
                                  mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::package_index));
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(!result.invoked_package_load);
    MK_REQUIRE(!result.committed);
}

MK_TEST("runtime package hot reload candidate review maps changed payload paths under candidate content roots") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_candidate_review_v2(
        mirakana::runtime::RuntimePackageHotReloadCandidateReviewDescV2{
            .changed_paths =
                {
                    "runtime/characters/hero.texture",
                    "runtime/ui/hud.material",
                    "runtime/ui_extra/hud.material",
                    "runtime/ui/nested.geindex",
                },
            .candidates =
                {
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters"),
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                    make_candidate("runtime/packages/audio.geindex", "runtime/audio", "audio"),
                },
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.rows.size() == 2);
    MK_REQUIRE(result.rows[0].candidate.label == "characters");
    MK_REQUIRE(result.rows[1].candidate.label == "ui");
    MK_REQUIRE(has_matched_change(result.rows[0], "runtime/characters/hero.texture",
                                  mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::content));
    MK_REQUIRE(has_matched_change(result.rows[1], "runtime/ui/hud.material",
                                  mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::content));
    MK_REQUIRE(result.matched_changed_path_count == 2);
    MK_REQUIRE(result.unmatched_changed_path_count == 2);
    MK_REQUIRE(result.review_candidate_count == 2);
    MK_REQUIRE(result.diagnostics.size() == 2);
    MK_REQUIRE(result.diagnostics[0].path == "runtime/ui_extra/hud.material");
    MK_REQUIRE(result.diagnostics[0].code == "unmatched-changed-path");
    MK_REQUIRE(result.diagnostics[1].path == "runtime/ui/nested.geindex");
    MK_REQUIRE(result.diagnostics[1].code == "unmatched-changed-path");
}

MK_TEST("runtime package hot reload candidate review rejects invalid and reports unmatched changed paths") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_candidate_review_v2(
        mirakana::runtime::RuntimePackageHotReloadCandidateReviewDescV2{
            .changed_paths =
                {
                    "runtime/packages/characters.geindex",
                    "/runtime/packages/escape.geindex",
                    "runtime/../packages/escape.geindex",
                    "runtime\\packages\\escape.geindex",
                    "runtime/packages/missing.geindex",
                },
            .candidates =
                {
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters"),
                },
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.changed_path_count == 5);
    MK_REQUIRE(result.matched_changed_path_count == 1);
    MK_REQUIRE(result.invalid_changed_path_count == 3);
    MK_REQUIRE(result.unmatched_changed_path_count == 1);
    MK_REQUIRE(result.diagnostics.size() == 4);
    MK_REQUIRE(result.diagnostics[0].path == "/runtime/packages/escape.geindex");
    MK_REQUIRE(result.diagnostics[0].code == "invalid-changed-path");
    MK_REQUIRE(result.diagnostics[1].path == "runtime/../packages/escape.geindex");
    MK_REQUIRE(result.diagnostics[1].code == "invalid-changed-path");
    MK_REQUIRE(result.diagnostics[2].path == "runtime\\packages\\escape.geindex");
    MK_REQUIRE(result.diagnostics[2].code == "invalid-changed-path");
    MK_REQUIRE(result.diagnostics[3].path == "runtime/packages/missing.geindex");
    MK_REQUIRE(result.diagnostics[3].code == "unmatched-changed-path");
    MK_REQUIRE(!result.invoked_package_load);
    MK_REQUIRE(!result.committed);
}

MK_TEST("runtime package hot reload candidate review deduplicates repeated matches for one candidate") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_candidate_review_v2(
        mirakana::runtime::RuntimePackageHotReloadCandidateReviewDescV2{
            .changed_paths =
                {
                    "runtime/packages/characters.geindex",
                    "runtime/characters/hero.texture",
                    "runtime/characters/hero.texture",
                },
            .candidates =
                {
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters"),
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters-copy"),
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui", "ui"),
                },
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.rows.size() == 1);
    MK_REQUIRE(result.rows[0].candidate.package_index_path == "runtime/packages/characters.geindex");
    MK_REQUIRE(result.rows[0].matched_changes.size() == 2);
    MK_REQUIRE(has_matched_change(result.rows[0], "runtime/packages/characters.geindex",
                                  mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::package_index));
    MK_REQUIRE(has_matched_change(result.rows[0], "runtime/characters/hero.texture",
                                  mirakana::runtime::RuntimePackageHotReloadCandidateReviewMatchKindV2::content));
    MK_REQUIRE(result.matched_changed_path_count == 3);
    MK_REQUIRE(result.review_candidate_count == 1);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime package hot reload candidate review ignores invalid candidate rows before review") {
    const auto result = mirakana::runtime::plan_runtime_package_hot_reload_candidate_review_v2(
        mirakana::runtime::RuntimePackageHotReloadCandidateReviewDescV2{
            .changed_paths = {"runtime/ui/hud.material", "runtime/packages/ui.geindex"},
            .candidates =
                {
                    make_candidate("runtime/packages/ui.geindex", "runtime/ui/", "ui"),
                    make_candidate("runtime/packages/audio.geindex", "runtime/audio", "audio/"),
                },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageHotReloadCandidateReviewStatusV2::no_matches);
    MK_REQUIRE(result.rows.empty());
    MK_REQUIRE(result.review_candidate_count == 0);
    MK_REQUIRE(result.changed_path_count == 2);
    MK_REQUIRE(result.matched_changed_path_count == 0);
    MK_REQUIRE(result.unmatched_changed_path_count == 2);
    MK_REQUIRE(result.diagnostics.size() == 2);
    MK_REQUIRE(result.diagnostics[0].code == "unmatched-changed-path");
    MK_REQUIRE(result.diagnostics[1].code == "unmatched-changed-path");
    MK_REQUIRE(!result.invoked_package_load);
    MK_REQUIRE(!result.committed);
}

MK_TEST("runtime package hot reload candidate review returns typed no-match statuses without package reads") {
    const auto no_changes = mirakana::runtime::plan_runtime_package_hot_reload_candidate_review_v2(
        mirakana::runtime::RuntimePackageHotReloadCandidateReviewDescV2{
            .changed_paths = {},
            .candidates =
                {
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters"),
                },
        });
    const auto no_candidates = mirakana::runtime::plan_runtime_package_hot_reload_candidate_review_v2(
        mirakana::runtime::RuntimePackageHotReloadCandidateReviewDescV2{
            .changed_paths = {"runtime/packages/characters.geindex"},
            .candidates = {},
        });
    const auto no_matches = mirakana::runtime::plan_runtime_package_hot_reload_candidate_review_v2(
        mirakana::runtime::RuntimePackageHotReloadCandidateReviewDescV2{
            .changed_paths = {"runtime/packages/missing.geindex"},
            .candidates =
                {
                    make_candidate("runtime/packages/characters.geindex", "runtime/characters", "characters"),
                },
        });

    MK_REQUIRE(!no_changes.succeeded());
    MK_REQUIRE(no_changes.status == mirakana::runtime::RuntimePackageHotReloadCandidateReviewStatusV2::no_changes);
    MK_REQUIRE(no_changes.review_candidate_count == 0);
    MK_REQUIRE(!no_changes.invoked_package_load);
    MK_REQUIRE(!no_changes.committed);

    MK_REQUIRE(!no_candidates.succeeded());
    MK_REQUIRE(no_candidates.status ==
               mirakana::runtime::RuntimePackageHotReloadCandidateReviewStatusV2::no_candidates);
    MK_REQUIRE(no_candidates.review_candidate_count == 0);
    MK_REQUIRE(no_candidates.unmatched_changed_path_count == 1);
    MK_REQUIRE(no_candidates.diagnostics.size() == 1);
    MK_REQUIRE(no_candidates.diagnostics[0].code == "unmatched-changed-path");
    MK_REQUIRE(!no_candidates.invoked_package_load);
    MK_REQUIRE(!no_candidates.committed);

    MK_REQUIRE(!no_matches.succeeded());
    MK_REQUIRE(no_matches.status == mirakana::runtime::RuntimePackageHotReloadCandidateReviewStatusV2::no_matches);
    MK_REQUIRE(no_matches.review_candidate_count == 0);
    MK_REQUIRE(no_matches.unmatched_changed_path_count == 1);
    MK_REQUIRE(no_matches.diagnostics.size() == 1);
    MK_REQUIRE(no_matches.diagnostics[0].code == "unmatched-changed-path");
    MK_REQUIRE(!no_matches.invoked_package_load);
    MK_REQUIRE(!no_matches.committed);
}

int main() {
    return mirakana::test::run_all();
}
