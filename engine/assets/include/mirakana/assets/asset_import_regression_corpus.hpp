// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_import_provenance.hpp"
#include "mirakana/assets/asset_registry.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class AssetImportRegressionCorpusAssetKind : std::uint8_t {
    gltf_scene,
    gltf_mesh,
    gltf_animation,
    png_texture,
    openexr_texture,
    ktx2_basis_texture,
    material_document,
    audio_source
};

enum class AssetImportRegressionLicensePolicy : std::uint8_t {
    accepted_for_source_tree,
    accepted_for_host_corpus_only,
    rejected
};

enum class AssetImportRegressionDiagnosticCode : std::uint8_t {
    none,
    invalid_manifest,
    duplicate_asset_id,
    unsafe_source_path,
    missing_source_file,
    source_hash_mismatch,
    missing_license_provenance,
    rejected_license,
    external_engine_material,
    unsupported_format,
    parser_error,
    validator_error,
    missing_external_resource,
    unsafe_external_resource_path,
    unsupported_extension,
    unsupported_animation_channel,
    unsupported_skin_or_morph_combination,
    coordinate_normalization_failed,
    material_extraction_failed,
    texture_decode_failed,
    texture_transcode_failed,
    cooked_output_mismatch,
    nondeterministic_output,
    row_budget_exceeded
};

struct AssetImportRegressionCorpusAssetV1 {
    std::string asset_id;
    AssetImportRegressionCorpusAssetKind kind{AssetImportRegressionCorpusAssetKind::gltf_mesh};
    AssetKeyV2 asset_key;
    std::string source_path;
    std::string expected_sha256;
    std::vector<std::string> expected_output_kinds;
    std::vector<std::string> required_features;
    AssetImportMeshPresetV1 mesh_preset;
    std::vector<std::string> preset_metadata;
    AssetImportProvenanceRowV1 provenance;
    AssetImportRegressionLicensePolicy license_policy{AssetImportRegressionLicensePolicy::rejected};
    bool allow_external_resources{false};
    bool allow_checked_in_distribution{false};
};

struct AssetImportRegressionCorpusDocumentV1 {
    std::string corpus_id{"GameEngine.AssetImportRegressionCorpus.v1"};
    std::string corpus_version{"1"};
    std::string root_path;
    std::vector<AssetImportRegressionCorpusAssetV1> assets;
    std::uint64_t row_budget{10000U};
};

struct AssetImportRegressionReportRowV1 {
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
    std::string message;
    std::string deterministic_output_hash;
    bool succeeded{false};
    bool ready_for_commercial_evidence{false};
};

struct AssetImportRegressionReportV1 {
    std::string corpus_id;
    std::string run_id;
    std::vector<AssetImportRegressionReportRowV1> rows;
    std::size_t asset_count{0U};
    std::size_t succeeded_count{0U};
    std::size_t failed_count{0U};
    std::size_t legal_blocked_count{0U};
    std::size_t nondeterministic_count{0U};
    bool ready{false};
};

[[nodiscard]] std::string_view
asset_import_regression_asset_kind_label(AssetImportRegressionCorpusAssetKind value) noexcept;
[[nodiscard]] std::string_view
asset_import_regression_license_policy_label(AssetImportRegressionLicensePolicy value) noexcept;
[[nodiscard]] std::string_view
asset_import_regression_diagnostic_code_label(AssetImportRegressionDiagnosticCode value) noexcept;

[[nodiscard]] std::vector<std::string>
validate_asset_import_regression_corpus_v1(const AssetImportRegressionCorpusDocumentV1& document);
[[nodiscard]] std::string
serialize_asset_import_regression_corpus_v1(const AssetImportRegressionCorpusDocumentV1& document);
[[nodiscard]] AssetImportRegressionCorpusDocumentV1
deserialize_asset_import_regression_corpus_v1(std::string_view text);
[[nodiscard]] std::string serialize_asset_import_regression_report_v1(const AssetImportRegressionReportV1& report);
[[nodiscard]] AssetImportRegressionReportV1 deserialize_asset_import_regression_report_v1(std::string_view text);

} // namespace mirakana
