// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/resource_runtime.hpp"

#include <algorithm>
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

MK_TEST("runtime 2D package playtest productization composes reviewed recipe and evidence rows") {
    using mirakana::runtime::Runtime2DPackagePlaytestEvidenceStatus;
    using mirakana::runtime::Runtime2DPackagePlaytestFailureClassification;

    const auto
        result =
            mirakana::runtime::plan_runtime_2d_package_playtest_productization(
                mirakana::runtime::Runtime2DPackagePlaytestDesc{
                    .manifest_validation_recipe_ids =
                        {
                            "installed-2d-package-smoke",
                            "installed-2d-input-device-production-ux-smoke",
                        },
                    .generated_playtest_recipe_ids =
                        {
                            "installed-2d-package-smoke-playtest",
                        },
                    .runtime_host_launch_recipe_ids =
                        {
                            "desktop-game-runtime-playtest",
                        },
                    .hot_reload_safe_point_evidence_ids =
                        {
                            "hot-reload-package-playtest-evidence",
                        },
                    .recipe_rows =
                        {
                            mirakana::runtime::Runtime2DPackagePlaytestRecipeRow{
                                .id = "installed-2d-package-smoke-playtest",
                                .validation_recipe_id = "installed-2d-package-smoke",
                                .reviewed_recipe_surface_id = "package-smoke-evidence-review",
                                .evidence_kind = "package-smoke-log",
                                .expected_signals = {"package_smoke_status=passed"},
                                .failure_classifications =
                                    {
                                        Runtime2DPackagePlaytestFailureClassification::missing_package_file,
                                        Runtime2DPackagePlaytestFailureClassification::invalid_scene_binding,
                                        Runtime2DPackagePlaytestFailureClassification::package_load_failure,
                                        Runtime2DPackagePlaytestFailureClassification::shader_tool_gap,
                                        Runtime2DPackagePlaytestFailureClassification::counter_mismatch,
                                        Runtime2DPackagePlaytestFailureClassification::hot_reload_recook_failure,
                                        Runtime2DPackagePlaytestFailureClassification::runtime_replacement_failure,
                                        Runtime2DPackagePlaytestFailureClassification::host_gated_backend,
                                        Runtime2DPackagePlaytestFailureClassification::long_run_budget_exceeded,
                                        Runtime2DPackagePlaytestFailureClassification::retained_artifact_missing,
                                    },
                                .runtime_host_launch_row_id = "desktop-game-runtime-playtest",
                                .hot_reload_safe_point_evidence_id = "hot-reload-package-playtest-evidence",
                                .required_frame_count = 3U,
                                .require_zero_over_budget_frames = true,
                                .require_memory_high_water = true,
                                .require_retained_profile_artifact = true,
                            },
                        },
                    .evidence_rows =
                        {
                            mirakana::runtime::Runtime2DPackagePlaytestEvidenceRow{
                                .recipe_id = "installed-2d-package-smoke-playtest",
                                .status = Runtime2DPackagePlaytestEvidenceStatus::passed,
                                .stdout_summary = "package_smoke_status=passed; package_smoke_counter_rows=8",
                                .stderr_summary = "",
                                .package_smoke_counters =
                                    {"package_smoke_status=passed", "2d_input_device_production_ux_ready=1"},
                                .profile_artifacts =
                                    {"reports/playtest/latest/installed-2d-package-smoke.profile.json"},
                                .remediation_handoff_ids = {"mutation-ledger-remediation"},
                                .frame_count = 3U,
                                .over_budget_frame_count = 0U,
                                .memory_high_water_bytes = 4096U,
                                .retained_artifact_hash = 9157U,
                                .externally_supplied = true,
                            },
                        },
                });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::Runtime2DPackagePlaytestStatus::ready);
    MK_REQUIRE(result.recipe_rows.size() == 1);
    MK_REQUIRE(result.recipe_rows[0].id == "installed-2d-package-smoke-playtest");
    MK_REQUIRE(result.recipe_rows[0].validation_recipe_declared);
    MK_REQUIRE(result.recipe_rows[0].generated_playtest_declared);
    MK_REQUIRE(result.recipe_rows[0].runtime_host_launch_declared);
    MK_REQUIRE(result.recipe_rows[0].hot_reload_safe_point_review_declared);
    MK_REQUIRE(result.evidence_rows.size() == 1);
    MK_REQUIRE(result.imported_evidence_count == 1);
    MK_REQUIRE(result.package_smoke_counter_count == 2);
    MK_REQUIRE(result.profile_artifact_count == 1);
    MK_REQUIRE(result.remediation_handoff_count == 1);
    MK_REQUIRE(result.failure_classification_count == 10);
    MK_REQUIRE(result.long_run_frame_count == 3U);
    MK_REQUIRE(result.long_run_over_budget_frame_count == 0U);
    MK_REQUIRE(result.long_run_memory_high_water_bytes == 4096U);
    MK_REQUIRE(result.retained_profile_artifact_hash_count == 1U);
    MK_REQUIRE(result.retained_profile_artifact_hash == 9157U);
    MK_REQUIRE(!result.invoked_editor_core_execution);
    MK_REQUIRE(!result.invoked_validation_recipe_execution);
    MK_REQUIRE(!result.invoked_arbitrary_shell);
    MK_REQUIRE(!result.invoked_active_session_hot_reload);
    MK_REQUIRE(!result.exposed_native_handles);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime 2D package playtest productization requires all failure classifications") {
    using mirakana::runtime::Runtime2DPackagePlaytestEvidenceStatus;
    using mirakana::runtime::Runtime2DPackagePlaytestFailureClassification;

    const auto result = mirakana::runtime::plan_runtime_2d_package_playtest_productization(
        mirakana::runtime::Runtime2DPackagePlaytestDesc{
            .manifest_validation_recipe_ids = {"installed-2d-package-smoke"},
            .generated_playtest_recipe_ids = {"installed-2d-package-smoke-playtest"},
            .recipe_rows =
                {
                    mirakana::runtime::Runtime2DPackagePlaytestRecipeRow{
                        .id = "installed-2d-package-smoke-playtest",
                        .validation_recipe_id = "installed-2d-package-smoke",
                        .reviewed_recipe_surface_id = "package-smoke-evidence-review",
                        .evidence_kind = "package-smoke-log",
                        .failure_classifications =
                            {
                                Runtime2DPackagePlaytestFailureClassification::missing_package_file,
                                Runtime2DPackagePlaytestFailureClassification::counter_mismatch,
                            },
                    },
                },
            .evidence_rows =
                {
                    mirakana::runtime::Runtime2DPackagePlaytestEvidenceRow{
                        .recipe_id = "installed-2d-package-smoke-playtest",
                        .status = Runtime2DPackagePlaytestEvidenceStatus::passed,
                        .stdout_summary = "package_smoke_status=passed",
                        .package_smoke_counters = {"package_smoke_status=passed"},
                        .externally_supplied = true,
                    },
                },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::Runtime2DPackagePlaytestStatus::blocked);
    MK_REQUIRE(result.diagnostics.size() == 3);
    MK_REQUIRE(std::ranges::find(result.diagnostics, "missing-runtime-host-launch-row") != result.diagnostics.end());
    MK_REQUIRE(std::ranges::find(result.diagnostics, "missing-hot-reload-safe-point-row") != result.diagnostics.end());
    MK_REQUIRE(std::ranges::find(result.diagnostics, "missing-required-failure-classification") !=
               result.diagnostics.end());
}

MK_TEST("runtime 2D package playtest productization fails closed on long run evidence gaps") {
    using mirakana::runtime::Runtime2DPackagePlaytestEvidenceStatus;
    using mirakana::runtime::Runtime2DPackagePlaytestFailureClassification;

    const auto
        result =
            mirakana::runtime::plan_runtime_2d_package_playtest_productization(
                mirakana::runtime::Runtime2DPackagePlaytestDesc{
                    .manifest_validation_recipe_ids = {"installed-2d-package-playtest-productization-smoke"},
                    .generated_playtest_recipe_ids = {"installed-2d-package-playtest-productization-smoke-playtest"},
                    .runtime_host_launch_recipe_ids = {"desktop-game-runtime-playtest"},
                    .hot_reload_safe_point_evidence_ids = {"packaged-2d-residency-budget"},
                    .recipe_rows =
                        {
                            mirakana::runtime::Runtime2DPackagePlaytestRecipeRow{
                                .id = "installed-2d-package-playtest-productization-smoke-playtest",
                                .validation_recipe_id = "installed-2d-package-playtest-productization-smoke",
                                .reviewed_recipe_surface_id = "package-smoke-evidence-review",
                                .evidence_kind = "package-smoke-counter-import",
                                .expected_signals = {"2d_package_playtest_productization_status=ready"},
                                .failure_classifications =
                                    {
                                        Runtime2DPackagePlaytestFailureClassification::missing_package_file,
                                        Runtime2DPackagePlaytestFailureClassification::invalid_scene_binding,
                                        Runtime2DPackagePlaytestFailureClassification::package_load_failure,
                                        Runtime2DPackagePlaytestFailureClassification::shader_tool_gap,
                                        Runtime2DPackagePlaytestFailureClassification::counter_mismatch,
                                        Runtime2DPackagePlaytestFailureClassification::hot_reload_recook_failure,
                                        Runtime2DPackagePlaytestFailureClassification::runtime_replacement_failure,
                                        Runtime2DPackagePlaytestFailureClassification::host_gated_backend,
                                        Runtime2DPackagePlaytestFailureClassification::long_run_budget_exceeded,
                                        Runtime2DPackagePlaytestFailureClassification::retained_artifact_missing,
                                    },
                                .runtime_host_launch_row_id = "desktop-game-runtime-playtest",
                                .hot_reload_safe_point_evidence_id = "packaged-2d-residency-budget",
                                .required_frame_count = 3U,
                                .require_zero_over_budget_frames = true,
                                .require_memory_high_water = true,
                                .require_retained_profile_artifact = true,
                            },
                        },
                    .evidence_rows =
                        {
                            mirakana::runtime::Runtime2DPackagePlaytestEvidenceRow{
                                .recipe_id = "installed-2d-package-playtest-productization-smoke-playtest",
                                .status = Runtime2DPackagePlaytestEvidenceStatus::passed,
                                .stdout_summary = "package_smoke_status=passed",
                                .package_smoke_counters = {"package_smoke_status=passed"},
                                .profile_artifacts = {"reports/playtest/latest/package-smoke.summary.txt"},
                                .remediation_handoff_ids = {"counter-mismatch-remediation"},
                                .frame_count = 2U,
                                .over_budget_frame_count = 1U,
                                .memory_high_water_bytes = 0U,
                                .retained_artifact_hash = 0U,
                                .externally_supplied = true,
                            },
                        },
                });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::Runtime2DPackagePlaytestStatus::blocked);
    MK_REQUIRE(result.long_run_frame_count == 2U);
    MK_REQUIRE(result.long_run_over_budget_frame_count == 1U);
    MK_REQUIRE(result.long_run_memory_high_water_bytes == 0U);
    MK_REQUIRE(result.retained_profile_artifact_hash_count == 0U);
    MK_REQUIRE(std::ranges::find(result.diagnostics, "long-run-frame-count-below-required") !=
               result.diagnostics.end());
    MK_REQUIRE(std::ranges::find(result.diagnostics, "long-run-over-budget-frames") != result.diagnostics.end());
    MK_REQUIRE(std::ranges::find(result.diagnostics, "missing-long-run-memory-high-water") != result.diagnostics.end());
    MK_REQUIRE(std::ranges::find(result.diagnostics, "missing-retained-profile-artifact-hash") !=
               result.diagnostics.end());
}

MK_TEST("runtime 2D package playtest productization blocks unsupported execution claims") {
    using mirakana::runtime::Runtime2DPackagePlaytestFailureClassification;

    const auto result = mirakana::runtime::plan_runtime_2d_package_playtest_productization(
        mirakana::runtime::Runtime2DPackagePlaytestDesc{
            .manifest_validation_recipe_ids = {"installed-2d-package-smoke"},
            .generated_playtest_recipe_ids = {"installed-2d-package-smoke-playtest"},
            .runtime_host_launch_recipe_ids = {"desktop-game-runtime-playtest"},
            .hot_reload_safe_point_evidence_ids = {"hot-reload-package-playtest-evidence"},
            .recipe_rows =
                {
                    mirakana::runtime::Runtime2DPackagePlaytestRecipeRow{
                        .id = "installed-2d-package-smoke-playtest",
                        .validation_recipe_id = "installed-2d-package-smoke",
                        .reviewed_recipe_surface_id = "package-smoke-evidence-review",
                        .evidence_kind = "package-smoke-log",
                        .failure_classifications =
                            {
                                Runtime2DPackagePlaytestFailureClassification::missing_package_file,
                                Runtime2DPackagePlaytestFailureClassification::invalid_scene_binding,
                                Runtime2DPackagePlaytestFailureClassification::package_load_failure,
                                Runtime2DPackagePlaytestFailureClassification::shader_tool_gap,
                                Runtime2DPackagePlaytestFailureClassification::counter_mismatch,
                                Runtime2DPackagePlaytestFailureClassification::hot_reload_recook_failure,
                                Runtime2DPackagePlaytestFailureClassification::runtime_replacement_failure,
                                Runtime2DPackagePlaytestFailureClassification::host_gated_backend,
                                Runtime2DPackagePlaytestFailureClassification::long_run_budget_exceeded,
                                Runtime2DPackagePlaytestFailureClassification::retained_artifact_missing,
                            },
                        .runtime_host_launch_row_id = "desktop-game-runtime-playtest",
                        .hot_reload_safe_point_evidence_id = "hot-reload-package-playtest-evidence",
                    },
                },
            .request_editor_core_execution = true,
            .request_validation_recipe_execution = true,
            .request_arbitrary_shell_execution = true,
            .request_active_session_hot_reload = true,
            .request_native_handle_exposure = true,
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::Runtime2DPackagePlaytestStatus::blocked);
    MK_REQUIRE(result.invoked_editor_core_execution);
    MK_REQUIRE(result.invoked_validation_recipe_execution);
    MK_REQUIRE(result.invoked_arbitrary_shell);
    MK_REQUIRE(result.invoked_active_session_hot_reload);
    MK_REQUIRE(result.exposed_native_handles);
    MK_REQUIRE(result.diagnostics.size() == 5);
}

int main() {
    return mirakana::test::run_all();
}
