// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_import_production_review.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::AssetImportProductionDiagnosticCode;
using mirakana::AssetImportProductionFeatureKind;
using mirakana::AssetImportProductionProofKind;
using mirakana::AssetImportProductionStatus;
using mirakana::KtxBasisTextureReviewDiagnosticCode;
using mirakana::KtxBasisTextureReviewFeature;
using mirakana::KtxBasisTextureReviewStatus;

constexpr AssetImportProductionFeatureKind kRequiredFeatures[] = {
    AssetImportProductionFeatureKind::source_root_policy,
    AssetImportProductionFeatureKind::gltf_geometry,
    AssetImportProductionFeatureKind::gltf_animation,
    AssetImportProductionFeatureKind::ktx_texture,
    AssetImportProductionFeatureKind::source_image,
    AssetImportProductionFeatureKind::source_audio,
    AssetImportProductionFeatureKind::material_source,
    AssetImportProductionFeatureKind::shader_offline_compile_request,
    AssetImportProductionFeatureKind::package_cook_output,
};

constexpr KtxBasisTextureReviewFeature kKtxRequiredFeatures[] = {
    KtxBasisTextureReviewFeature::container_validation,    KtxBasisTextureReviewFeature::supercompression_policy,
    KtxBasisTextureReviewFeature::transcode_target_policy, KtxBasisTextureReviewFeature::gpu_target_compatibility,
    KtxBasisTextureReviewFeature::source_provenance,       KtxBasisTextureReviewFeature::package_output,
    KtxBasisTextureReviewFeature::host_tool_gate,
};

[[nodiscard]] std::string capability_id(AssetImportProductionFeatureKind feature) {
    switch (feature) {
    case AssetImportProductionFeatureKind::source_root_policy:
        return "source-root-policy";
    case AssetImportProductionFeatureKind::gltf_geometry:
        return "gltf-geometry-import";
    case AssetImportProductionFeatureKind::gltf_animation:
        return "gltf-animation-import";
    case AssetImportProductionFeatureKind::ktx_texture:
        return "ktx2-texture-import";
    case AssetImportProductionFeatureKind::source_image:
        return "source-image-import";
    case AssetImportProductionFeatureKind::source_audio:
        return "source-audio-import";
    case AssetImportProductionFeatureKind::material_source:
        return "material-source-import";
    case AssetImportProductionFeatureKind::shader_offline_compile_request:
        return "shader-offline-compile-request";
    case AssetImportProductionFeatureKind::package_cook_output:
        return "package-cook-output";
    }
    return "unknown";
}

[[nodiscard]] std::vector<std::string> extensions_for(AssetImportProductionFeatureKind feature) {
    switch (feature) {
    case AssetImportProductionFeatureKind::source_root_policy:
        return {".geassets"};
    case AssetImportProductionFeatureKind::gltf_geometry:
    case AssetImportProductionFeatureKind::gltf_animation:
        return {".gltf", ".glb"};
    case AssetImportProductionFeatureKind::ktx_texture:
        return {".ktx2"};
    case AssetImportProductionFeatureKind::source_image:
        return {".png"};
    case AssetImportProductionFeatureKind::source_audio:
        return {".wav", ".flac", ".mp3"};
    case AssetImportProductionFeatureKind::material_source:
        return {".material", ".materialgraph"};
    case AssetImportProductionFeatureKind::shader_offline_compile_request:
        return {".hlsl"};
    case AssetImportProductionFeatureKind::package_cook_output:
        return {".geindex", ".geasset"};
    }
    return {};
}

[[nodiscard]] mirakana::AssetImportProductionEvidenceRow make_ready_row(AssetImportProductionFeatureKind feature,
                                                                        std::uint32_t source_index) {
    const auto is_gltf = feature == AssetImportProductionFeatureKind::gltf_geometry ||
                         feature == AssetImportProductionFeatureKind::gltf_animation;
    const auto is_ktx = feature == AssetImportProductionFeatureKind::ktx_texture;
    const auto is_shader = feature == AssetImportProductionFeatureKind::shader_offline_compile_request;
    const auto uses_dependency_adapter = is_gltf || is_ktx ||
                                         feature == AssetImportProductionFeatureKind::source_image ||
                                         feature == AssetImportProductionFeatureKind::source_audio;
    const auto requires_dependency_evidence = uses_dependency_adapter || is_shader;

    return mirakana::AssetImportProductionEvidenceRow{
        .capability_id = capability_id(feature),
        .feature = feature,
        .proof = uses_dependency_adapter ? AssetImportProductionProofKind::reviewed_dependency_adapter
                                         : AssetImportProductionProofKind::first_party_source,
        .importer_id = std::string{"reviewed."} + capability_id(feature),
        .source_root = "source/assets",
        .declared_extensions = extensions_for(feature),
        .output_package_rows = {std::string{"runtime/"} + capability_id(feature) + ".geasset"},
        .validator_ids = is_gltf ? std::vector<std::string>{"gltf-validator"}
                                 : (is_ktx ? std::vector<std::string>{"ktx-validate"}
                                           : std::vector<std::string>{"first-party-schema"}),
        .reviewed_command_ids =
            is_shader ? std::vector<std::string>{"dxc-offline-plan", "spirv-val-plan"} : std::vector<std::string>{},
        .dependency_ids = is_ktx                    ? std::vector<std::string>{"vcpkg.ktx"}
                          : is_shader               ? std::vector<std::string>{"toolchain.dxc", "toolchain.spirv-tools"}
                          : uses_dependency_adapter ? std::vector<std::string>{"vcpkg.asset-importers"}
                                                    : std::vector<std::string>{},
        .license_ids = {"LicenseRef-Proprietary", "third-party-notice-reviewed"},
        .provenance_ids = {std::string{"provenance."} + capability_id(feature)},
        .deterministic_content_hash = std::string{"sha256:"} + capability_id(feature),
        .reviewed = true,
        .source_root_evidence = true,
        .importer_declared = true,
        .extension_evidence = true,
        .package_handoff_evidence = true,
        .license_provenance_evidence = true,
        .deterministic_hash_evidence = true,
        .validator_evidence = is_gltf || is_ktx || is_shader,
        .dependency_legal_evidence = requires_dependency_evidence,
        .dependency_gate_required = false,
        .command_review_evidence = is_shader,
        .host_validated = true,
        .host_gate_required = false,
        .request_arbitrary_importer_plugin = false,
        .request_external_download = false,
        .request_live_shader_generation = false,
        .request_source_mutation_outside_roots = false,
        .request_package_mutation = false,
        .request_native_handle_access = false,
        .request_unreviewed_compiler_execution = false,
        .request_runtime_source_parsing = false,
        .request_broad_codec_claim = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::KtxBasisTextureReviewRow make_ktx_row(KtxBasisTextureReviewFeature feature,
                                                              std::uint32_t source_index) {
    const auto has_ktx_dependency = feature != KtxBasisTextureReviewFeature::host_tool_gate;
    const auto is_host_tool_gate = feature == KtxBasisTextureReviewFeature::host_tool_gate;

    return mirakana::KtxBasisTextureReviewRow{
        .row_id = std::string{"ktx2-basis."} + std::to_string(source_index),
        .feature = feature,
        .container_validator_ids = feature == KtxBasisTextureReviewFeature::container_validation
                                       ? std::vector<std::string>{"ktx2check-review"}
                                       : std::vector<std::string>{},
        .supercompression_scheme =
            feature == KtxBasisTextureReviewFeature::supercompression_policy ? "basis-uastc" : "",
        .transcode_policy =
            feature == KtxBasisTextureReviewFeature::transcode_target_policy ? "offline-reviewed-target" : "",
        .transcode_target = feature == KtxBasisTextureReviewFeature::transcode_target_policy ? "bc7-rgba" : "",
        .gpu_target = feature == KtxBasisTextureReviewFeature::gpu_target_compatibility ? "d3d12-bc7-rgba" : "",
        .source_provenance_ids = feature == KtxBasisTextureReviewFeature::source_provenance
                                     ? std::vector<std::string>{"provenance.ktx2-basis-source"}
                                     : std::vector<std::string>{},
        .package_output_rows = feature == KtxBasisTextureReviewFeature::package_output
                                   ? std::vector<std::string>{"runtime/assets/3d/ktx2_basis_texture.geasset"}
                                   : std::vector<std::string>{},
        .dependency_ids = has_ktx_dependency ? std::vector<std::string>{"vcpkg.ktx"} : std::vector<std::string>{},
        .license_ids = has_ktx_dependency || feature == KtxBasisTextureReviewFeature::source_provenance
                           ? std::vector<std::string>{"LicenseRef-KTX-Software"}
                           : std::vector<std::string>{},
        .deterministic_content_hash = std::string{"sha256:ktx2-basis-row-"} + std::to_string(source_index),
        .reviewed = true,
        .container_validation_evidence = feature == KtxBasisTextureReviewFeature::container_validation,
        .supercompression_policy_evidence = feature == KtxBasisTextureReviewFeature::supercompression_policy,
        .transcode_target_evidence = feature == KtxBasisTextureReviewFeature::transcode_target_policy,
        .gpu_target_compatibility_evidence = feature == KtxBasisTextureReviewFeature::gpu_target_compatibility,
        .source_provenance_evidence = feature == KtxBasisTextureReviewFeature::source_provenance,
        .package_output_evidence = feature == KtxBasisTextureReviewFeature::package_output,
        .dependency_legal_evidence = has_ktx_dependency,
        .dependency_gate_required = false,
        .host_tool_validated = false,
        .host_tool_gate_required = is_host_tool_gate,
        .request_runtime_transcoding = false,
        .request_gpu_upload = false,
        .request_compression_execution = false,
        .request_native_handle_access = false,
        .request_broad_texture_codec_claim = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::KtxBasisTextureReviewRequest make_ktx_request() {
    std::vector<mirakana::KtxBasisTextureReviewRow> rows;
    rows.reserve(std::size(kKtxRequiredFeatures));
    std::uint32_t source_index{1U};
    for (const auto feature : kKtxRequiredFeatures) {
        rows.push_back(make_ktx_row(feature, source_index++));
    }

    return mirakana::KtxBasisTextureReviewRequest{
        .required_features = {std::begin(kKtxRequiredFeatures), std::end(kKtxRequiredFeatures)},
        .rows = std::move(rows),
        .row_budget = 16U,
        .seed = 379U,
    };
}

[[nodiscard]] mirakana::AssetImportProductionReviewRequest make_request() {
    std::vector<mirakana::AssetImportProductionEvidenceRow> rows;
    rows.reserve(std::size(kRequiredFeatures));
    std::uint32_t source_index{1U};
    for (const auto feature : kRequiredFeatures) {
        rows.push_back(make_ready_row(feature, source_index++));
    }

    return mirakana::AssetImportProductionReviewRequest{
        .required_features = {std::begin(kRequiredFeatures), std::end(kRequiredFeatures)},
        .rows = std::move(rows),
        .row_budget = 64U,
        .seed = 789U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::AssetImportProductionReview& review,
                                           AssetImportProductionDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : review.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] bool has_diagnostic_for_feature(const mirakana::AssetImportProductionReview& review,
                                              AssetImportProductionDiagnosticCode code,
                                              AssetImportProductionFeatureKind feature) {
    for (const auto& diagnostic : review.diagnostics) {
        if (diagnostic.code == code && diagnostic.feature == feature) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::size_t ktx_diagnostic_count(const mirakana::KtxBasisTextureReview& review,
                                               KtxBasisTextureReviewDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : review.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("asset import production review accepts explicit broad source and cook evidence") {
    const auto review = mirakana::review_asset_import_production_readiness(make_request());

    MK_REQUIRE(review.status == AssetImportProductionStatus::ready);
    MK_REQUIRE(review.succeeded());
    MK_REQUIRE(review.diagnostics.empty());
    MK_REQUIRE(review.row_count == std::size(kRequiredFeatures));
    MK_REQUIRE(review.ready_row_count == std::size(kRequiredFeatures));
    MK_REQUIRE(review.host_gated_row_count == 0U);
    MK_REQUIRE(review.dependency_gated_row_count == 0U);
    MK_REQUIRE(review.package_mutation_request_count == 0U);
    MK_REQUIRE(review.unsupported_claim_row_count == 0U);
    MK_REQUIRE(review.reviewed_importer_count == std::size(kRequiredFeatures));
    MK_REQUIRE(review.supported_source_format_count == 13U);
    MK_REQUIRE(review.execution_readiness.size() == std::size(kRequiredFeatures));
    for (const auto& readiness_row : review.execution_readiness) {
        MK_REQUIRE(readiness_row.readiness == mirakana::AssetImportProductionExecutionReadiness::reviewed_execution);
    }
    MK_REQUIRE(review.reviewed_source_roots_ready);
    MK_REQUIRE(review.gltf_ktx_import_review_ready);
    MK_REQUIRE(review.image_audio_import_review_ready);
    MK_REQUIRE(review.shader_offline_compile_review_ready);
    MK_REQUIRE(review.package_cook_outputs_ready);
    MK_REQUIRE(review.dependency_legal_records_ready);
    MK_REQUIRE(review.deterministic_cook_ready);
    MK_REQUIRE(review.broad_asset_import_ready);
    MK_REQUIRE(review.replay_hash != 0U);
    MK_REQUIRE(!review.invoked_importer_plugin);
    MK_REQUIRE(!review.invoked_external_download);
    MK_REQUIRE(!review.invoked_shader_compiler);
    MK_REQUIRE(!review.invoked_source_mutation);
}

MK_TEST("asset import production review reports host gated rows without broad readiness") {
    auto request = make_request();
    request.rows[3].host_validated = false;
    request.rows[3].host_gate_required = true;
    request.rows[3].validator_evidence = false;
    request.rows[3].package_handoff_evidence = false;

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::host_evidence_required);
    MK_REQUIRE(!review.succeeded());
    MK_REQUIRE(review.diagnostics.empty());
    MK_REQUIRE(review.ready_row_count == std::size(kRequiredFeatures) - 1U);
    MK_REQUIRE(review.host_gated_row_count == 1U);
    MK_REQUIRE(review.execution_readiness[3].feature == AssetImportProductionFeatureKind::ktx_texture);
    MK_REQUIRE(review.execution_readiness[3].readiness ==
               mirakana::AssetImportProductionExecutionReadiness::host_evidence_required);
    MK_REQUIRE(!review.gltf_ktx_import_review_ready);
    MK_REQUIRE(!review.broad_asset_import_ready);
    MK_REQUIRE(review.replay_hash != 0U);
}

MK_TEST("asset import production review reports dependency gated execution matrix without broad readiness") {
    auto request = make_request();
    request.rows[4].dependency_legal_evidence = false;
    request.rows[4].dependency_gate_required = true;

    const auto review = mirakana::review_asset_import_production_readiness(request);
    const auto ready_review = mirakana::review_asset_import_production_readiness(make_request());

    MK_REQUIRE(review.status == AssetImportProductionStatus::dependency_evidence_required);
    MK_REQUIRE(!review.succeeded());
    MK_REQUIRE(review.diagnostics.empty());
    MK_REQUIRE(review.ready_row_count == std::size(kRequiredFeatures) - 1U);
    MK_REQUIRE(review.dependency_gated_row_count == 1U);
    MK_REQUIRE(review.host_gated_row_count == 0U);
    MK_REQUIRE(!review.image_audio_import_review_ready);
    MK_REQUIRE(!review.dependency_legal_records_ready);
    MK_REQUIRE(!review.broad_asset_import_ready);
    MK_REQUIRE(review.execution_readiness.size() == std::size(kRequiredFeatures));
    MK_REQUIRE(review.execution_readiness[4].feature == AssetImportProductionFeatureKind::source_image);
    MK_REQUIRE(review.execution_readiness[4].readiness ==
               mirakana::AssetImportProductionExecutionReadiness::dependency_evidence_required);
    MK_REQUIRE(review.replay_hash != 0U);
    MK_REQUIRE(review.replay_hash != ready_review.replay_hash);
}

MK_TEST("asset import production review rejects ktx2 basis readiness without selected dependency evidence") {
    auto request = make_request();
    request.rows[3].dependency_ids = {"vcpkg.ktx", "custom-wrapper"};
    request.rows[3].license_ids = {"LicenseRef-Proprietary"};
    request.rows[3].provenance_ids = {"provenance.generic-texture-import"};

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(has_diagnostic_for_feature(review, AssetImportProductionDiagnosticCode::missing_dependency_legal_record,
                                          AssetImportProductionFeatureKind::ktx_texture));
    MK_REQUIRE(!review.gltf_ktx_import_review_ready);
    MK_REQUIRE(!review.broad_asset_import_ready);
}

MK_TEST("ktx2 basis texture review accepts selected dependency and host gated offline tool policy") {
    const auto review = mirakana::review_ktx_basis_texture_readiness(make_ktx_request());

    MK_REQUIRE(review.status == KtxBasisTextureReviewStatus::host_evidence_required);
    MK_REQUIRE(!review.succeeded());
    MK_REQUIRE(review.diagnostics.empty());
    MK_REQUIRE(review.row_count == std::size(kKtxRequiredFeatures));
    MK_REQUIRE(review.ready_row_count == std::size(kKtxRequiredFeatures) - 1U);
    MK_REQUIRE(review.host_gated_row_count == 1U);
    MK_REQUIRE(review.dependency_gated_row_count == 0U);
    MK_REQUIRE(review.unsupported_claim_row_count == 0U);
    MK_REQUIRE(review.container_validation_rows == 1U);
    MK_REQUIRE(review.supercompression_policy_rows == 1U);
    MK_REQUIRE(review.transcode_target_policy_rows == 1U);
    MK_REQUIRE(review.gpu_target_compatibility_rows == 1U);
    MK_REQUIRE(review.source_provenance_rows == 1U);
    MK_REQUIRE(review.package_output_rows == 1U);
    MK_REQUIRE(review.host_tool_gate_rows == 1U);
    MK_REQUIRE(review.container_validation_ready);
    MK_REQUIRE(review.supercompression_policy_ready);
    MK_REQUIRE(review.transcode_target_policy_ready);
    MK_REQUIRE(review.gpu_target_compatibility_ready);
    MK_REQUIRE(review.source_provenance_ready);
    MK_REQUIRE(review.package_output_ready);
    MK_REQUIRE(review.dependency_legal_records_ready);
    MK_REQUIRE(review.selected_package_evidence_ready);
    MK_REQUIRE(review.ktx_basis_review_ready);
    MK_REQUIRE(!review.broad_texture_codec_ready);
    MK_REQUIRE(!review.invoked_runtime_transcoding);
    MK_REQUIRE(!review.invoked_gpu_upload);
    MK_REQUIRE(!review.invoked_compression_tool);
    MK_REQUIRE(review.replay_hash != 0U);
}

MK_TEST("ktx2 basis texture review reports dependency gated selected package evidence") {
    auto request = make_ktx_request();
    request.rows[0].dependency_legal_evidence = false;
    request.rows[0].dependency_gate_required = true;
    request.rows[6].host_tool_validated = true;
    request.rows[6].host_tool_gate_required = false;

    const auto review = mirakana::review_ktx_basis_texture_readiness(request);

    MK_REQUIRE(review.status == KtxBasisTextureReviewStatus::dependency_evidence_required);
    MK_REQUIRE(!review.succeeded());
    MK_REQUIRE(review.diagnostics.empty());
    MK_REQUIRE(review.ready_row_count == std::size(kKtxRequiredFeatures) - 1U);
    MK_REQUIRE(review.host_gated_row_count == 0U);
    MK_REQUIRE(review.dependency_gated_row_count == 1U);
    MK_REQUIRE(!review.container_validation_ready);
    MK_REQUIRE(!review.dependency_legal_records_ready);
    MK_REQUIRE(!review.selected_package_evidence_ready);
    MK_REQUIRE(!review.ktx_basis_review_ready);
    MK_REQUIRE(!review.broad_texture_codec_ready);
    MK_REQUIRE(review.replay_hash != 0U);
}

MK_TEST("ktx2 basis texture review rejects runtime transcode upload and missing target policy") {
    auto request = make_ktx_request();
    request.rows[2].transcode_target.clear();
    request.rows[2].transcode_target_evidence = false;
    request.rows[2].request_runtime_transcoding = true;
    request.rows[2].request_gpu_upload = true;
    request.rows[2].request_compression_execution = true;
    request.rows[2].request_native_handle_access = true;
    request.rows[2].request_broad_texture_codec_claim = true;

    const auto review = mirakana::review_ktx_basis_texture_readiness(request);

    MK_REQUIRE(review.status == KtxBasisTextureReviewStatus::invalid_request);
    MK_REQUIRE(ktx_diagnostic_count(review, KtxBasisTextureReviewDiagnosticCode::missing_transcode_target) == 1U);
    MK_REQUIRE(ktx_diagnostic_count(review, KtxBasisTextureReviewDiagnosticCode::unsupported_runtime_transcoding) ==
               1U);
    MK_REQUIRE(ktx_diagnostic_count(review, KtxBasisTextureReviewDiagnosticCode::unsupported_gpu_upload) == 1U);
    MK_REQUIRE(ktx_diagnostic_count(review, KtxBasisTextureReviewDiagnosticCode::unsupported_compression_execution) ==
               1U);
    MK_REQUIRE(ktx_diagnostic_count(review, KtxBasisTextureReviewDiagnosticCode::unsupported_native_handle_claim) ==
               1U);
    MK_REQUIRE(
        ktx_diagnostic_count(review, KtxBasisTextureReviewDiagnosticCode::unsupported_broad_texture_codec_claim) == 1U);
    MK_REQUIRE(!review.selected_package_evidence_ready);
    MK_REQUIRE(!review.ktx_basis_review_ready);
    MK_REQUIRE(!review.broad_texture_codec_ready);
    MK_REQUIRE(review.replay_hash == 0U);
}

MK_TEST("asset import production review rejects gltf scene import without package hash and source root evidence") {
    auto request = make_request();
    request.rows[1].source_root_evidence = false;
    request.rows[1].output_package_rows.clear();
    request.rows[1].deterministic_hash_evidence = false;

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(has_diagnostic_for_feature(review, AssetImportProductionDiagnosticCode::missing_source_root_evidence,
                                          AssetImportProductionFeatureKind::gltf_geometry));
    MK_REQUIRE(has_diagnostic_for_feature(review, AssetImportProductionDiagnosticCode::missing_output_package_row,
                                          AssetImportProductionFeatureKind::gltf_geometry));
    MK_REQUIRE(has_diagnostic_for_feature(review, AssetImportProductionDiagnosticCode::missing_deterministic_hash,
                                          AssetImportProductionFeatureKind::gltf_geometry));
    MK_REQUIRE(!review.gltf_ktx_import_review_ready);
    MK_REQUIRE(!review.broad_asset_import_ready);
}

MK_TEST("asset import production review rejects broad source image and audio codec extension claims") {
    auto request = make_request();
    request.rows[4].declared_extensions = {".png", ".jpg", ".webp"};
    request.rows[5].declared_extensions = {".wav", ".flac", ".mp3", ".ogg"};

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(has_diagnostic_for_feature(review, AssetImportProductionDiagnosticCode::unsupported_broad_codec_claim,
                                          AssetImportProductionFeatureKind::source_image));
    MK_REQUIRE(has_diagnostic_for_feature(review, AssetImportProductionDiagnosticCode::unsupported_broad_codec_claim,
                                          AssetImportProductionFeatureKind::source_audio));
    MK_REQUIRE(!review.image_audio_import_review_ready);
    MK_REQUIRE(!review.broad_asset_import_ready);
}

MK_TEST("asset import production review requires shader toolchain dependency and legal evidence") {
    auto request = make_request();
    request.rows[7].dependency_ids = {"toolchain.dxc", "toolchain.spirv-tools", "extra-tool"};

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(has_diagnostic_for_feature(review, AssetImportProductionDiagnosticCode::missing_dependency_legal_record,
                                          AssetImportProductionFeatureKind::shader_offline_compile_request));
    MK_REQUIRE(!review.shader_offline_compile_review_ready);
    MK_REQUIRE(!review.broad_asset_import_ready);
}

MK_TEST("asset import production review keeps shader compiler rows host gated without host validation") {
    auto request = make_request();
    request.rows[7].host_validated = false;
    request.rows[7].host_gate_required = true;
    request.rows[7].command_review_evidence = false;

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::host_evidence_required);
    MK_REQUIRE(review.diagnostics.empty());
    MK_REQUIRE(review.host_gated_row_count == 1U);
    MK_REQUIRE(review.execution_readiness[7].feature ==
               AssetImportProductionFeatureKind::shader_offline_compile_request);
    MK_REQUIRE(review.execution_readiness[7].readiness ==
               mirakana::AssetImportProductionExecutionReadiness::host_evidence_required);
    MK_REQUIRE(!review.shader_offline_compile_review_ready);
    MK_REQUIRE(!review.broad_asset_import_ready);
}

MK_TEST("asset import production review rejects parser compiler and native handle leakage tokens") {
    auto request = make_request();
    request.rows[1].source_root = "source/assets/FastGltf::Parser";
    request.rows[7].reviewed_command_ids = {"idxccompiler3"};
    request.rows[8].output_package_rows = {"runtime/Native_Handle_package.geasset"};

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::invalid_evidence_row) == 3U);
    MK_REQUIRE(review.replay_hash == 0U);
}

MK_TEST("asset import production review rejects missing manifest and package evidence") {
    auto request = make_request();
    request.rows[1].reviewed = false;
    request.rows[1].host_validated = false;
    request.rows[1].source_root_evidence = false;
    request.rows[1].importer_declared = false;
    request.rows[1].declared_extensions.clear();
    request.rows[1].output_package_rows.clear();
    request.rows[1].license_provenance_evidence = false;
    request.rows[1].deterministic_hash_evidence = false;
    request.rows[1].dependency_legal_evidence = false;

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(!review.succeeded());
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_review_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_host_validation_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_source_root_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_importer_id) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_extension_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_output_package_row) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_license_provenance) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_deterministic_hash) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_dependency_legal_record) == 1U);
    MK_REQUIRE(review.rows.empty());
    MK_REQUIRE(review.replay_hash == 0U);
}

MK_TEST("asset import production review rejects unsupported execution and unsafe claims") {
    auto request = make_request();
    request.rows[7].request_arbitrary_importer_plugin = true;
    request.rows[7].request_external_download = true;
    request.rows[7].request_live_shader_generation = true;
    request.rows[7].request_source_mutation_outside_roots = true;
    request.rows[7].request_native_handle_access = true;
    request.rows[7].request_unreviewed_compiler_execution = true;
    request.rows[7].request_runtime_source_parsing = true;
    request.rows[7].request_broad_codec_claim = true;
    request.rows[7].command_review_evidence = false;
    request.rows[7].validator_evidence = false;

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_arbitrary_importer_plugin) ==
               1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_external_download) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_live_shader_generation) == 1U);
    MK_REQUIRE(
        diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_source_mutation_outside_roots) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_native_handle_claim) == 1U);
    MK_REQUIRE(
        diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_unreviewed_compiler_execution) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_runtime_source_parsing) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_broad_codec_claim) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_command_review_evidence) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_validator_evidence) == 1U);
    MK_REQUIRE(review.unsupported_claim_row_count == 1U);
    MK_REQUIRE(review.package_mutation_request_count == 0U);
    MK_REQUIRE(review.execution_readiness[7].readiness ==
               mirakana::AssetImportProductionExecutionReadiness::unsupported_claim);
    MK_REQUIRE(!review.invoked_importer_plugin);
    MK_REQUIRE(!review.invoked_external_download);
    MK_REQUIRE(!review.invoked_shader_compiler);
    MK_REQUIRE(!review.invoked_source_mutation);
}

MK_TEST("asset import production review distinguishes package mutation from unsupported importer execution") {
    auto request = make_request();
    request.rows[2].request_arbitrary_importer_plugin = true;
    request.rows[8].request_package_mutation = true;

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_arbitrary_importer_plugin) ==
               1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::unsupported_package_mutation) == 1U);
    MK_REQUIRE(review.unsupported_claim_row_count == 1U);
    MK_REQUIRE(review.package_mutation_request_count == 1U);
    MK_REQUIRE(review.execution_readiness.size() == std::size(kRequiredFeatures));
    MK_REQUIRE(review.execution_readiness[2].readiness ==
               mirakana::AssetImportProductionExecutionReadiness::unsupported_claim);
    MK_REQUIRE(review.execution_readiness[8].feature == AssetImportProductionFeatureKind::package_cook_output);
    MK_REQUIRE(review.execution_readiness[8].readiness ==
               mirakana::AssetImportProductionExecutionReadiness::package_mutation_required);
    MK_REQUIRE(!review.invoked_importer_plugin);
    MK_REQUIRE(!review.invoked_source_mutation);
}

MK_TEST("asset import production review rejects missing required features duplicate rows and unsafe tokens") {
    auto request = make_request();
    request.rows.pop_back();
    request.rows.push_back(request.rows.front());
    request.rows.back().source_index = 99U;
    request.rows[0].capability_id = "VkImage";
    request.rows[0].output_package_rows = {"runtime/ID3D12Resource.geasset"};

    const auto review = mirakana::review_asset_import_production_readiness(request);

    MK_REQUIRE(review.status == AssetImportProductionStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::missing_required_feature_row) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::duplicate_feature_row) == 1U);
    MK_REQUIRE(diagnostic_count(review, AssetImportProductionDiagnosticCode::invalid_evidence_row) == 1U);
    MK_REQUIRE(review.replay_hash == 0U);
}

MK_TEST("asset import production review reports no rows without broad import claims") {
    const auto review =
        mirakana::review_asset_import_production_readiness(mirakana::AssetImportProductionReviewRequest{});

    MK_REQUIRE(review.status == AssetImportProductionStatus::no_rows);
    MK_REQUIRE(review.succeeded());
    MK_REQUIRE(review.diagnostics.empty());
    MK_REQUIRE(review.row_count == 0U);
    MK_REQUIRE(review.replay_hash == 0U);
}

int main() {
    return mirakana::test::run_all();
}
