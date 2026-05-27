// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class AssetImportProductionStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    dependency_evidence_required,
    no_rows,
    invalid_request,
};

enum class AssetImportProductionExecutionReadiness : std::uint8_t {
    reviewed_execution = 0,
    dependency_evidence_required,
    host_evidence_required,
    package_mutation_required,
    unsupported_claim,
};

enum class AssetImportProductionFeatureKind : std::uint8_t {
    source_root_policy = 0,
    gltf_geometry,
    gltf_animation,
    ktx_texture,
    source_image,
    source_audio,
    material_source,
    shader_offline_compile_request,
    package_cook_output,
};

enum class AssetImportProductionProofKind : std::uint8_t {
    first_party_source = 0,
    official_validator,
    reviewed_dependency_adapter,
    offline_command_plan,
    legal_provenance,
    host_gate,
};

enum class AssetImportProductionDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_feature,
    duplicate_required_feature,
    missing_required_feature_row,
    duplicate_feature_row,
    invalid_evidence_row,
    missing_review_evidence,
    missing_host_validation_evidence,
    missing_source_root_evidence,
    missing_importer_id,
    missing_extension_evidence,
    missing_output_package_row,
    missing_license_provenance,
    missing_deterministic_hash,
    missing_validator_evidence,
    missing_dependency_legal_record,
    missing_command_review_evidence,
    unsupported_arbitrary_importer_plugin,
    unsupported_external_download,
    unsupported_live_shader_generation,
    unsupported_source_mutation_outside_roots,
    unsupported_package_mutation,
    unsupported_native_handle_claim,
    unsupported_unreviewed_compiler_execution,
    unsupported_runtime_source_parsing,
    unsupported_broad_codec_claim,
    row_budget_exceeded,
};

struct AssetImportProductionEvidenceRow {
    std::string capability_id;
    AssetImportProductionFeatureKind feature{AssetImportProductionFeatureKind::source_root_policy};
    AssetImportProductionProofKind proof{AssetImportProductionProofKind::first_party_source};
    std::string importer_id;
    std::string source_root;
    std::vector<std::string> declared_extensions;
    std::vector<std::string> output_package_rows;
    std::vector<std::string> validator_ids;
    std::vector<std::string> reviewed_command_ids;
    std::vector<std::string> dependency_ids;
    std::vector<std::string> license_ids;
    std::vector<std::string> provenance_ids;
    std::string deterministic_content_hash;
    bool reviewed{false};
    bool source_root_evidence{false};
    bool importer_declared{false};
    bool extension_evidence{false};
    bool package_handoff_evidence{false};
    bool license_provenance_evidence{false};
    bool deterministic_hash_evidence{false};
    bool validator_evidence{false};
    bool dependency_legal_evidence{false};
    bool dependency_gate_required{false};
    bool command_review_evidence{false};
    bool host_validated{false};
    bool host_gate_required{false};
    bool request_arbitrary_importer_plugin{false};
    bool request_external_download{false};
    bool request_live_shader_generation{false};
    bool request_source_mutation_outside_roots{false};
    bool request_package_mutation{false};
    bool request_native_handle_access{false};
    bool request_unreviewed_compiler_execution{false};
    bool request_runtime_source_parsing{false};
    bool request_broad_codec_claim{false};
    std::uint32_t source_index{0U};
};

struct AssetImportProductionReviewRequest {
    std::vector<AssetImportProductionFeatureKind> required_features;
    std::vector<AssetImportProductionEvidenceRow> rows;
    std::size_t row_budget{512U};
    std::uint64_t seed{0U};
};

struct AssetImportProductionExecutionReadinessRow {
    AssetImportProductionFeatureKind feature{AssetImportProductionFeatureKind::source_root_policy};
    AssetImportProductionExecutionReadiness readiness{AssetImportProductionExecutionReadiness::reviewed_execution};
    std::string capability_id;
    std::uint32_t source_index{0U};
};

struct AssetImportProductionDiagnostic {
    AssetImportProductionDiagnosticCode code{AssetImportProductionDiagnosticCode::none};
    AssetImportProductionFeatureKind feature{AssetImportProductionFeatureKind::source_root_policy};
    std::string capability_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct AssetImportProductionReview {
    AssetImportProductionStatus status{AssetImportProductionStatus::invalid_request};
    std::vector<AssetImportProductionDiagnostic> diagnostics;
    std::vector<AssetImportProductionFeatureKind> required_features;
    std::vector<AssetImportProductionEvidenceRow> rows;
    std::vector<AssetImportProductionExecutionReadinessRow> execution_readiness;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t dependency_gated_row_count{0U};
    std::size_t package_mutation_request_count{0U};
    std::size_t unsupported_claim_row_count{0U};
    std::size_t reviewed_importer_count{0U};
    std::size_t supported_source_format_count{0U};
    std::uint64_t replay_hash{0U};
    bool reviewed_source_roots_ready{false};
    bool gltf_ktx_import_review_ready{false};
    bool image_audio_import_review_ready{false};
    bool shader_offline_compile_review_ready{false};
    bool package_cook_outputs_ready{false};
    bool dependency_legal_records_ready{false};
    bool deterministic_cook_ready{false};
    bool broad_asset_import_ready{false};
    bool invoked_importer_plugin{false};
    bool invoked_external_download{false};
    bool invoked_shader_compiler{false};
    bool invoked_source_mutation{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews broad source-import and cook evidence without executing importer plugins, downloading assets,
/// running shader compilers, mutating sources, parsing runtime source data, or exposing native/middleware handles.
[[nodiscard]] AssetImportProductionReview
review_asset_import_production_readiness(const AssetImportProductionReviewRequest& request);

} // namespace mirakana
