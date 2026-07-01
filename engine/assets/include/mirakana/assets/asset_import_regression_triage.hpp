// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_regression_corpus.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class AssetImportRegressionTriageSeverity : std::uint8_t {
    info,
    action_required,
    blocked,
};

enum class AssetImportRegressionRecommendedAction : std::uint8_t {
    none,
    fix_notice_or_remove_asset,
    refresh_corpus_manifest,
    inspect_source_asset,
    record_unsupported_or_reduce_source,
    open_axis_unit_preview,
    inspect_codec_dependency,
    rerun_isolated_and_compare_hashes,
    split_corpus_or_raise_reviewed_budget,
};

enum class AssetImportRegressionReimportDecision : std::uint8_t {
    not_needed,
    dry_run_allowed,
    blocked,
};

struct AssetImportRegressionTriageRowV1 {
    std::string asset_id;
    AssetImportRegressionCorpusAssetKind kind{AssetImportRegressionCorpusAssetKind::gltf_mesh};
    AssetId asset;
    std::string source_path;
    std::string source_sha256;
    std::string preset_sha256;
    std::string importer_id;
    std::string importer_version;
    std::string phase;
    AssetImportRegressionDiagnosticCode code{AssetImportRegressionDiagnosticCode::none};
    AssetImportRegressionTriageSeverity severity{AssetImportRegressionTriageSeverity::info};
    AssetImportRegressionRecommendedAction recommended_action{AssetImportRegressionRecommendedAction::none};
    AssetImportRegressionReimportDecision reimport_decision{AssetImportRegressionReimportDecision::not_needed};
    std::string repro_command_id;
    std::string repro_command;
    std::string source_excerpt_hash;
    bool preset_diff_required{false};
    bool axis_unit_preview_required{false};
    bool legal_blocked{false};
    bool nondeterministic{false};
};

struct AssetImportRegressionTriageDocumentV1 {
    std::string format{"GameEngine.AssetImportRegressionTriage.v1"};
    std::string corpus_id;
    std::string run_id;
    std::vector<AssetImportRegressionTriageRowV1> rows;
    std::size_t row_count{0U};
    std::size_t failed_count{0U};
    std::size_t blocked_count{0U};
    std::size_t reimport_candidate_count{0U};
    std::size_t preset_diff_required_count{0U};
    std::size_t axis_unit_preview_required_count{0U};
    std::size_t legal_blocked_count{0U};
    std::size_t nondeterministic_count{0U};
    bool ready_for_operator_review{false};
};

[[nodiscard]] std::string_view
asset_import_regression_triage_severity_label(AssetImportRegressionTriageSeverity value) noexcept;
[[nodiscard]] std::string_view
asset_import_regression_recommended_action_label(AssetImportRegressionRecommendedAction value) noexcept;
[[nodiscard]] std::string_view
asset_import_regression_reimport_decision_label(AssetImportRegressionReimportDecision value) noexcept;

[[nodiscard]] AssetImportRegressionRecommendedAction
recommended_action_for_asset_import_regression_code(AssetImportRegressionDiagnosticCode code) noexcept;
[[nodiscard]] AssetImportRegressionTriageDocumentV1
make_asset_import_regression_triage_v1(const AssetImportRegressionReportV1& report);
[[nodiscard]] std::string
serialize_asset_import_regression_triage_v1(const AssetImportRegressionTriageDocumentV1& document);
[[nodiscard]] AssetImportRegressionTriageDocumentV1
deserialize_asset_import_regression_triage_v1(std::string_view text);

} // namespace mirakana
