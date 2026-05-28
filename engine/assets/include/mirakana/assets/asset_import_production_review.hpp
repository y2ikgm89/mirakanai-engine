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

enum class KtxBasisTextureReviewStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    dependency_evidence_required,
    no_rows,
    invalid_request,
};

enum class KtxBasisTextureReviewFeature : std::uint8_t {
    container_validation = 0,
    supercompression_policy,
    transcode_target_policy,
    gpu_target_compatibility,
    source_provenance,
    package_output,
    host_tool_gate,
};

enum class KtxBasisTextureReviewDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_feature,
    duplicate_required_feature,
    missing_required_feature_row,
    duplicate_feature_row,
    invalid_row,
    missing_container_validation,
    missing_supercompression_policy,
    missing_transcode_target,
    missing_gpu_target_compatibility,
    missing_source_provenance,
    missing_package_output,
    missing_dependency_legal_record,
    missing_host_tool_evidence,
    unsupported_runtime_transcoding,
    unsupported_gpu_upload,
    unsupported_compression_execution,
    unsupported_native_handle_claim,
    unsupported_broad_texture_codec_claim,
    row_budget_exceeded,
};

enum class GltfSceneImportReviewStatus : std::uint8_t {
    ready = 0,
    dependency_evidence_required,
    no_rows,
    invalid_request,
};

enum class GltfSceneImportReviewFeature : std::uint8_t {
    source_root_policy = 0,
    parser_validation,
    geometry_payload,
    material_payload,
    animation_payload,
    external_resource_policy,
    source_provenance,
    package_output,
};

enum class GltfSceneImportReviewDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_feature,
    duplicate_required_feature,
    missing_required_feature_row,
    duplicate_feature_row,
    invalid_row,
    missing_source_root,
    missing_parser_validation,
    missing_geometry_payload,
    missing_material_payload,
    missing_animation_payload,
    missing_external_resource_policy,
    missing_source_provenance,
    missing_package_output,
    missing_dependency_legal_record,
    unsupported_arbitrary_extension,
    unsupported_external_network_fetch,
    unsupported_runtime_source_parsing,
    unsupported_parser_type_leakage,
    unsupported_native_handle_claim,
    unsupported_broad_scene_import_claim,
    unsupported_package_mutation,
    row_budget_exceeded,
};

enum class SourceImageAudioCodecReviewStatus : std::uint8_t {
    ready = 0,
    dependency_evidence_required,
    no_rows,
    invalid_request,
};

enum class SourceImageAudioCodecReviewFeature : std::uint8_t {
    png_decode_adapter = 0,
    png_pixel_format,
    audio_decode_adapter,
    audio_sample_format,
    source_provenance,
    package_output,
};

enum class SourceImageAudioCodecReviewDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_feature,
    duplicate_required_feature,
    missing_required_feature_row,
    duplicate_feature_row,
    invalid_row,
    missing_source_root,
    missing_decode_adapter,
    missing_pixel_format_diagnostics,
    missing_sample_format_diagnostics,
    missing_source_provenance,
    missing_package_output,
    missing_dependency_legal_record,
    unsupported_svg_vector_codec,
    unsupported_broad_image_codec,
    unsupported_broad_audio_codec,
    unsupported_background_decode_streaming,
    unsupported_hrtf_middleware,
    unsupported_runtime_source_parsing,
    unsupported_native_handle_claim,
    unsupported_package_mutation,
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

struct KtxBasisTextureReviewRow {
    std::string row_id;
    KtxBasisTextureReviewFeature feature{KtxBasisTextureReviewFeature::container_validation};
    std::vector<std::string> container_validator_ids;
    std::string supercompression_scheme;
    std::string transcode_policy;
    std::string transcode_target;
    std::string gpu_target;
    std::vector<std::string> source_provenance_ids;
    std::vector<std::string> package_output_rows;
    std::vector<std::string> dependency_ids;
    std::vector<std::string> license_ids;
    std::string deterministic_content_hash;
    bool reviewed{false};
    bool container_validation_evidence{false};
    bool supercompression_policy_evidence{false};
    bool transcode_target_evidence{false};
    bool gpu_target_compatibility_evidence{false};
    bool source_provenance_evidence{false};
    bool package_output_evidence{false};
    bool dependency_legal_evidence{false};
    bool dependency_gate_required{false};
    bool host_tool_validated{false};
    bool host_tool_gate_required{false};
    bool request_runtime_transcoding{false};
    bool request_gpu_upload{false};
    bool request_compression_execution{false};
    bool request_native_handle_access{false};
    bool request_broad_texture_codec_claim{false};
    std::uint32_t source_index{0U};
};

struct KtxBasisTextureReviewRequest {
    std::vector<KtxBasisTextureReviewFeature> required_features;
    std::vector<KtxBasisTextureReviewRow> rows;
    std::size_t row_budget{64U};
    std::uint64_t seed{0U};
};

struct KtxBasisTextureReviewDiagnostic {
    KtxBasisTextureReviewDiagnosticCode code{KtxBasisTextureReviewDiagnosticCode::none};
    KtxBasisTextureReviewFeature feature{KtxBasisTextureReviewFeature::container_validation};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct KtxBasisTextureReview {
    KtxBasisTextureReviewStatus status{KtxBasisTextureReviewStatus::invalid_request};
    std::vector<KtxBasisTextureReviewDiagnostic> diagnostics;
    std::vector<KtxBasisTextureReviewFeature> required_features;
    std::vector<KtxBasisTextureReviewRow> rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t dependency_gated_row_count{0U};
    std::size_t unsupported_claim_row_count{0U};
    std::size_t container_validation_rows{0U};
    std::size_t supercompression_policy_rows{0U};
    std::size_t transcode_target_policy_rows{0U};
    std::size_t gpu_target_compatibility_rows{0U};
    std::size_t source_provenance_rows{0U};
    std::size_t package_output_rows{0U};
    std::size_t host_tool_gate_rows{0U};
    std::uint64_t replay_hash{0U};
    bool container_validation_ready{false};
    bool supercompression_policy_ready{false};
    bool transcode_target_policy_ready{false};
    bool gpu_target_compatibility_ready{false};
    bool source_provenance_ready{false};
    bool package_output_ready{false};
    bool dependency_legal_records_ready{false};
    bool selected_package_evidence_ready{false};
    bool ktx_basis_review_ready{false};
    bool broad_texture_codec_ready{false};
    bool invoked_runtime_transcoding{false};
    bool invoked_gpu_upload{false};
    bool invoked_compression_tool{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

struct SourceImageAudioCodecReviewRow {
    std::string row_id;
    SourceImageAudioCodecReviewFeature feature{SourceImageAudioCodecReviewFeature::png_decode_adapter};
    std::string source_root;
    std::string importer_id;
    std::vector<std::string> declared_extensions;
    std::vector<std::string> dependency_ids;
    std::vector<std::string> license_ids;
    std::vector<std::string> provenance_ids;
    std::vector<std::string> package_output_rows;
    std::string deterministic_content_hash;
    std::string image_pixel_format;
    std::uint32_t image_width{0U};
    std::uint32_t image_height{0U};
    std::string audio_sample_format;
    std::uint32_t audio_channels{0U};
    std::uint32_t audio_sample_rate{0U};
    std::uint32_t audio_frame_count{0U};
    bool reviewed{false};
    bool source_root_evidence{false};
    bool decode_adapter_evidence{false};
    bool pixel_format_evidence{false};
    bool sample_format_evidence{false};
    bool source_provenance_evidence{false};
    bool package_output_evidence{false};
    bool deterministic_hash_evidence{false};
    bool dependency_legal_evidence{false};
    bool dependency_gate_required{false};
    bool request_svg_vector_codec{false};
    bool request_broad_image_codec{false};
    bool request_broad_audio_codec{false};
    bool request_background_decode_streaming{false};
    bool request_hrtf_middleware{false};
    bool request_runtime_source_parsing{false};
    bool request_native_handle_access{false};
    bool request_package_mutation{false};
    std::uint32_t source_index{0U};
};

struct SourceImageAudioCodecReviewRequest {
    std::vector<SourceImageAudioCodecReviewFeature> required_features;
    std::vector<SourceImageAudioCodecReviewRow> rows;
    std::size_t row_budget{64U};
    std::uint64_t seed{0U};
};

struct SourceImageAudioCodecReviewDiagnostic {
    SourceImageAudioCodecReviewDiagnosticCode code{SourceImageAudioCodecReviewDiagnosticCode::none};
    SourceImageAudioCodecReviewFeature feature{SourceImageAudioCodecReviewFeature::png_decode_adapter};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct SourceImageAudioCodecReview {
    SourceImageAudioCodecReviewStatus status{SourceImageAudioCodecReviewStatus::invalid_request};
    std::vector<SourceImageAudioCodecReviewDiagnostic> diagnostics;
    std::vector<SourceImageAudioCodecReviewFeature> required_features;
    std::vector<SourceImageAudioCodecReviewRow> rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t dependency_gated_row_count{0U};
    std::size_t unsupported_claim_row_count{0U};
    std::size_t png_decode_rows{0U};
    std::size_t png_pixel_format_rows{0U};
    std::size_t audio_decode_rows{0U};
    std::size_t audio_sample_format_rows{0U};
    std::size_t source_provenance_rows{0U};
    std::size_t package_output_rows{0U};
    std::uint64_t replay_hash{0U};
    bool png_decode_ready{false};
    bool png_rgba8_pixel_format_ready{false};
    bool audio_decode_ready{false};
    bool audio_sample_format_ready{false};
    bool source_provenance_ready{false};
    bool package_output_ready{false};
    bool dependency_legal_records_ready{false};
    bool selected_package_evidence_ready{false};
    bool source_image_audio_codec_ready{false};
    bool broad_image_codec_ready{false};
    bool broad_audio_codec_ready{false};
    bool invoked_svg_vector_decode{false};
    bool invoked_background_decode_streaming{false};
    bool invoked_hrtf_middleware{false};
    bool invoked_runtime_source_parsing{false};
    bool exposed_native_handle{false};
    bool mutated_packages{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

struct GltfSceneImportReviewRow {
    std::string row_id;
    GltfSceneImportReviewFeature feature{GltfSceneImportReviewFeature::source_root_policy};
    std::string source_root;
    std::string importer_id;
    std::vector<std::string> declared_extensions;
    std::vector<std::string> validator_ids;
    std::vector<std::string> dependency_ids;
    std::vector<std::string> license_ids;
    std::vector<std::string> provenance_ids;
    std::vector<std::string> package_output_rows;
    std::string deterministic_content_hash;
    std::string external_resource_policy;
    bool reviewed{false};
    bool source_root_evidence{false};
    bool parser_validation_evidence{false};
    bool geometry_payload_evidence{false};
    bool material_payload_evidence{false};
    bool animation_payload_evidence{false};
    bool external_resource_policy_evidence{false};
    bool source_provenance_evidence{false};
    bool package_output_evidence{false};
    bool dependency_legal_evidence{false};
    bool dependency_gate_required{false};
    bool request_arbitrary_extension{false};
    bool request_external_network_fetch{false};
    bool request_runtime_source_parsing{false};
    bool request_parser_type_access{false};
    bool request_native_handle_access{false};
    bool request_broad_scene_import_claim{false};
    bool request_package_mutation{false};
    std::uint32_t source_index{0U};
};

struct GltfSceneImportReviewRequest {
    std::vector<GltfSceneImportReviewFeature> required_features;
    std::vector<GltfSceneImportReviewRow> rows;
    std::size_t row_budget{64U};
    std::uint64_t seed{0U};
};

struct GltfSceneImportReviewDiagnostic {
    GltfSceneImportReviewDiagnosticCode code{GltfSceneImportReviewDiagnosticCode::none};
    GltfSceneImportReviewFeature feature{GltfSceneImportReviewFeature::source_root_policy};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct GltfSceneImportReview {
    GltfSceneImportReviewStatus status{GltfSceneImportReviewStatus::invalid_request};
    std::vector<GltfSceneImportReviewDiagnostic> diagnostics;
    std::vector<GltfSceneImportReviewFeature> required_features;
    std::vector<GltfSceneImportReviewRow> rows;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t dependency_gated_row_count{0U};
    std::size_t unsupported_claim_row_count{0U};
    std::size_t source_root_rows{0U};
    std::size_t parser_validation_rows{0U};
    std::size_t geometry_payload_rows{0U};
    std::size_t material_payload_rows{0U};
    std::size_t animation_payload_rows{0U};
    std::size_t external_resource_policy_rows{0U};
    std::size_t source_provenance_rows{0U};
    std::size_t package_output_rows{0U};
    std::uint64_t replay_hash{0U};
    bool source_root_ready{false};
    bool parser_validation_ready{false};
    bool geometry_payload_ready{false};
    bool material_payload_ready{false};
    bool animation_payload_ready{false};
    bool external_resource_policy_ready{false};
    bool source_provenance_ready{false};
    bool package_output_ready{false};
    bool dependency_legal_records_ready{false};
    bool selected_package_evidence_ready{false};
    bool gltf_scene_import_ready{false};
    bool broad_scene_import_ready{false};
    bool invoked_external_network_fetch{false};
    bool invoked_runtime_source_parsing{false};
    bool leaked_parser_type{false};
    bool exposed_native_handle{false};
    bool mutated_packages{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews broad source-import and cook evidence without executing importer plugins, downloading assets,
/// running shader compilers, mutating sources, parsing runtime source data, or exposing native/middleware handles.
[[nodiscard]] AssetImportProductionReview
review_asset_import_production_readiness(const AssetImportProductionReviewRequest& request);

/// Reviews KTX2/Basis texture import and offline transcode planning evidence without loading KTX files,
/// transcoding textures, uploading GPU resources, running compression tools, or exposing native handles.
[[nodiscard]] KtxBasisTextureReview review_ktx_basis_texture_readiness(const KtxBasisTextureReviewRequest& request);

/// Reviews selected glTF scene import evidence without parsing runtime source data, fetching network resources,
/// exposing fastgltf/native handles, mutating packages, or claiming broad scene import readiness.
[[nodiscard]] GltfSceneImportReview review_gltf_scene_import_readiness(const GltfSceneImportReviewRequest& request);

/// Reviews selected PNG and source-audio codec evidence without decoding source files, streaming in the
/// background, invoking HRTF/middleware, parsing runtime source data, mutating packages, or exposing native handles.
[[nodiscard]] SourceImageAudioCodecReview
review_source_image_audio_codec_readiness(const SourceImageAudioCodecReviewRequest& request);

} // namespace mirakana
