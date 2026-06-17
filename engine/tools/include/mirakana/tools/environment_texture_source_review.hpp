// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_source_format.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mirakana {

enum class OpenExrTextureSourceReviewDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    asset_importers_disabled,
    openexr_read_failed,
    unsupported_openexr_layout,
    unsupported_openexr_channels,
    invalid_openexr_metadata,
};

enum class Ktx2BasisTextureSourceReviewDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    asset_importers_disabled,
    ktx_read_failed,
    unsupported_ktx_layout,
    unsupported_ktx_format,
    invalid_ktx_metadata,
};

enum class TextureBackendFormatPolicyDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    unsupported_source,
    missing_backend_evidence,
    unsupported_backend_format,
    invalid_backend_evidence,
};

enum class EnvironmentTextureGeassetMetadataDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    invalid_cook_metadata,
};

enum class EnvironmentTextureGeassetPayloadDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    invalid_cook_metadata,
    invalid_payload,
    unsupported_claim,
};

enum class OpenExrEnvironmentTexturePayloadDecodeDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    asset_importers_disabled,
    openexr_read_failed,
    unsupported_openexr_layout,
    unsupported_openexr_channels,
    invalid_openexr_metadata,
    invalid_cook_metadata,
    invalid_payload,
    unsupported_claim,
};

enum class Ktx2BasisEnvironmentTexturePayloadTranscodeDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    asset_importers_disabled,
    ktx_read_failed,
    ktx_transcode_failed,
    unsupported_ktx_layout,
    unsupported_ktx_format,
    invalid_ktx_metadata,
    invalid_cook_metadata,
    invalid_payload,
    unsupported_claim,
};

enum class EnvironmentTexturePayloadCookDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    asset_importers_disabled,
    openexr_read_failed,
    ktx_read_failed,
    ktx_transcode_failed,
    source_review_failed,
    invalid_cook_metadata,
    invalid_payload,
    unsupported_claim,
};

struct OpenExrTextureSourceReviewDiagnostic {
    OpenExrTextureSourceReviewDiagnosticCode code{OpenExrTextureSourceReviewDiagnosticCode::none};
    std::string message;
    std::string path;
};

struct Ktx2BasisTextureSourceReviewDiagnostic {
    Ktx2BasisTextureSourceReviewDiagnosticCode code{Ktx2BasisTextureSourceReviewDiagnosticCode::none};
    std::string message;
    std::string path;
};

struct TextureBackendFormatPolicyDiagnostic {
    TextureBackendFormatPolicyDiagnosticCode code{TextureBackendFormatPolicyDiagnosticCode::none};
    std::string message;
    std::string backend;
};

struct EnvironmentTextureGeassetMetadataDiagnostic {
    EnvironmentTextureGeassetMetadataDiagnosticCode code{EnvironmentTextureGeassetMetadataDiagnosticCode::none};
    std::string message;
    std::string path;
};

struct EnvironmentTextureGeassetPayloadDiagnostic {
    EnvironmentTextureGeassetPayloadDiagnosticCode code{EnvironmentTextureGeassetPayloadDiagnosticCode::none};
    std::string message;
    std::string path;
};

struct OpenExrEnvironmentTexturePayloadDecodeDiagnostic {
    OpenExrEnvironmentTexturePayloadDecodeDiagnosticCode code{
        OpenExrEnvironmentTexturePayloadDecodeDiagnosticCode::none};
    std::string message;
    std::string path;
};

struct Ktx2BasisEnvironmentTexturePayloadTranscodeDiagnostic {
    Ktx2BasisEnvironmentTexturePayloadTranscodeDiagnosticCode code{
        Ktx2BasisEnvironmentTexturePayloadTranscodeDiagnosticCode::none};
    std::string message;
    std::string path;
};

struct EnvironmentTexturePayloadCookDiagnostic {
    EnvironmentTexturePayloadCookDiagnosticCode code{EnvironmentTexturePayloadCookDiagnosticCode::none};
    std::string message;
    std::string path;
};

struct TextureBackendFormatEvidenceRowV1 {
    TextureCookBackendV1 backend{TextureCookBackendV1::unknown};
    std::string evidence_id;
    std::string official_api;
    bool host_validated{false};
    bool sampled_2d{false};
    bool transfer_dst{false};
    bool texture_cube{false};
    bool rgba16_float{false};
    bool bc7_rgba{false};
    bool astc_4x4_rgba{false};
};

struct OpenExrTextureSourceReviewRequest {
    std::filesystem::path source_file_path;
    std::string source_path;
    std::string source_hash;
    std::string provenance_id;
    std::string license_id;
    TextureSamplerClassV2 sampler_class{TextureSamplerClassV2::environment_radiance};
    bool scene_linear_intent{true};
};

struct Ktx2BasisTextureSourceReviewRequest {
    std::filesystem::path source_file_path;
    std::string source_path;
    std::string source_hash;
    std::string provenance_id;
    std::string license_id;
    TextureColorSpaceV2 color_space{TextureColorSpaceV2::srgb};
    TextureSamplerClassV2 sampler_class{TextureSamplerClassV2::color};
    bool basis_required{true};
};

struct TextureBackendFormatPolicyRequestV1 {
    TextureSourceDocumentV2 source;
    std::vector<TextureBackendFormatEvidenceRowV1> backend_evidence;
    bool require_all_backends{true};
};

struct EnvironmentTextureGeassetMetadataRequestV1 {
    std::string geasset_path;
    TextureCookMetadataDocumentV1 cook_metadata;
    bool metadata_only{true};
};

struct EnvironmentTextureGeassetPayloadRequestV1 {
    AssetId asset;
    std::string geasset_path;
    std::uint64_t source_revision{1};
    TextureCookMetadataDocumentV1 cook_metadata;
    std::vector<std::uint8_t> payload_bytes;
    std::string decode_stage;
    std::string transcode_stage{"not_required"};
    bool pixel_decode_invoked{false};
    bool basis_transcode_invoked{false};
    bool gpu_upload_invoked{false};
    bool broad_asset_pipeline_ready{false};
};

struct OpenExrEnvironmentTexturePayloadDecodeRequestV1 {
    AssetId asset;
    std::string geasset_path;
    std::uint64_t source_revision{1};
    OpenExrTextureSourceReviewRequest source_review;
    std::vector<TextureBackendFormatEvidenceRowV1> backend_evidence;
    bool require_all_backends{true};
    bool gpu_upload_invoked{false};
    bool broad_asset_pipeline_ready{false};
};

struct Ktx2BasisEnvironmentTexturePayloadTranscodeRequestV1 {
    AssetId asset;
    std::string geasset_path;
    std::uint64_t source_revision{1};
    Ktx2BasisTextureSourceReviewRequest source_review;
    std::vector<TextureBackendFormatEvidenceRowV1> backend_evidence;
    bool require_all_backends{true};
    bool gpu_upload_invoked{false};
    bool broad_asset_pipeline_ready{false};
};

struct EnvironmentTexturePayloadCookRequestV1 {
    AssetId asset;
    std::string geasset_path;
    std::uint64_t source_revision{1};
    std::optional<OpenExrTextureSourceReviewRequest> openexr_source_review;
    std::optional<Ktx2BasisTextureSourceReviewRequest> ktx2_basis_source_review;
    std::vector<TextureBackendFormatEvidenceRowV1> backend_evidence;
    bool require_all_backends{true};
    bool gpu_upload_invoked{false};
    bool broad_asset_pipeline_ready{false};
};

struct OpenExrTextureSourceReviewResult {
    std::optional<TextureSourceDocumentV2> source;
    std::vector<OpenExrTextureSourceReviewDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return source.has_value() && diagnostics.empty();
    }
};

struct Ktx2BasisTextureSourceReviewResult {
    std::optional<TextureSourceDocumentV2> source;
    std::vector<Ktx2BasisTextureSourceReviewDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return source.has_value() && diagnostics.empty();
    }
};

struct TextureBackendFormatPolicyResultV1 {
    std::optional<TextureCookMetadataDocumentV1> metadata;
    std::vector<TextureBackendFormatPolicyDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return metadata.has_value() && diagnostics.empty();
    }
};

struct EnvironmentTextureGeassetMetadataResultV1 {
    std::optional<EnvironmentTextureGeassetMetadataDocumentV1> metadata;
    std::vector<EnvironmentTextureGeassetMetadataDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return metadata.has_value() && diagnostics.empty();
    }
};

struct EnvironmentTextureGeassetPayloadResultV1 {
    std::string content;
    AssetCookedArtifact artifact;
    std::vector<EnvironmentTextureGeassetPayloadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return !content.empty() && is_valid_asset_cooked_artifact(artifact) && diagnostics.empty();
    }
};

struct OpenExrEnvironmentTexturePayloadDecodeResultV1 {
    std::optional<TextureSourceDocumentV2> source;
    std::optional<TextureCookMetadataDocumentV1> cook_metadata;
    std::vector<std::uint8_t> payload_bytes;
    std::string decode_stage;
    EnvironmentTextureGeassetPayloadResultV1 payload;
    std::vector<OpenExrEnvironmentTexturePayloadDecodeDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return source.has_value() && cook_metadata.has_value() && !payload_bytes.empty() && !decode_stage.empty() &&
               payload.succeeded() && diagnostics.empty();
    }
};

struct Ktx2BasisEnvironmentTexturePayloadTranscodeResultV1 {
    std::optional<TextureSourceDocumentV2> source;
    std::optional<TextureCookMetadataDocumentV1> cook_metadata;
    std::vector<std::uint8_t> payload_bytes;
    std::string decode_stage;
    std::string transcode_stage;
    EnvironmentTextureGeassetPayloadResultV1 payload;
    std::vector<Ktx2BasisEnvironmentTexturePayloadTranscodeDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return source.has_value() && cook_metadata.has_value() && !payload_bytes.empty() && !decode_stage.empty() &&
               !transcode_stage.empty() && payload.succeeded() && diagnostics.empty();
    }
};

struct EnvironmentTexturePayloadCookResultV1 {
    std::optional<TextureSourceDocumentV2> source;
    std::optional<TextureCookMetadataDocumentV1> cook_metadata;
    std::vector<std::uint8_t> payload_bytes;
    std::string decode_stage;
    std::string transcode_stage;
    EnvironmentTextureGeassetPayloadResultV1 payload;
    std::vector<EnvironmentTexturePayloadCookDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return source.has_value() && cook_metadata.has_value() && !payload_bytes.empty() && !decode_stage.empty() &&
               !transcode_stage.empty() && payload.succeeded() && diagnostics.empty();
    }
};

[[nodiscard]] bool has_openexr_texture_source_review() noexcept;
[[nodiscard]] OpenExrTextureSourceReviewResult
review_openexr_texture_source_metadata(const OpenExrTextureSourceReviewRequest& request);
[[nodiscard]] bool has_openexr_environment_texture_payload_decode() noexcept;
[[nodiscard]] OpenExrEnvironmentTexturePayloadDecodeResultV1
decode_openexr_environment_texture_payload_v1(const OpenExrEnvironmentTexturePayloadDecodeRequestV1& request);
[[nodiscard]] bool has_ktx2_basis_texture_source_review() noexcept;
[[nodiscard]] Ktx2BasisTextureSourceReviewResult
review_ktx2_basis_texture_source_metadata(const Ktx2BasisTextureSourceReviewRequest& request);
[[nodiscard]] bool has_ktx2_basis_environment_texture_payload_transcode() noexcept;
[[nodiscard]] Ktx2BasisEnvironmentTexturePayloadTranscodeResultV1 transcode_ktx2_basis_environment_texture_payload_v1(
    const Ktx2BasisEnvironmentTexturePayloadTranscodeRequestV1& request);
[[nodiscard]] bool has_environment_texture_payload_cook() noexcept;
[[nodiscard]] bool
has_environment_texture_payload_cook_diagnostic(const EnvironmentTexturePayloadCookResultV1& result,
                                                EnvironmentTexturePayloadCookDiagnosticCode code) noexcept;
[[nodiscard]] EnvironmentTexturePayloadCookResultV1
cook_environment_texture_payload_v1(const EnvironmentTexturePayloadCookRequestV1& request);
[[nodiscard]] TextureBackendFormatPolicyResultV1
plan_texture_backend_format_policy_v1(const TextureBackendFormatPolicyRequestV1& request);
[[nodiscard]] EnvironmentTextureGeassetMetadataResultV1
plan_environment_texture_geasset_metadata_v1(const EnvironmentTextureGeassetMetadataRequestV1& request);
[[nodiscard]] EnvironmentTextureGeassetPayloadResultV1
plan_environment_texture_geasset_payload_v1(const EnvironmentTextureGeassetPayloadRequestV1& request);

} // namespace mirakana
