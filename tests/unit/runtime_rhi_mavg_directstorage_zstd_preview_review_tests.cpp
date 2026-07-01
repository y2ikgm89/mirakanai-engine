// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_directstorage_zstd_preview_review.hpp"

#include <vector>

namespace {

using mirakana::runtime_rhi::RuntimeMavgDirectStorageZstdPreviewDiagnosticCode;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageZstdPreviewFeatureKind;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageZstdPreviewRow;
using mirakana::runtime_rhi::RuntimeMavgDirectStorageZstdPreviewStatus;

[[nodiscard]] RuntimeMavgDirectStorageZstdPreviewRow
make_preview_row(RuntimeMavgDirectStorageZstdPreviewFeatureKind feature) {
    return RuntimeMavgDirectStorageZstdPreviewRow{
        .feature = feature,
        .row_id = "directstorage.zstd.preview",
        .official_source_id = "microsoft-directstorage-1.4-zstd-preview",
        .sdk_package_version = "1.4.0-preview1-2603.504",
        .reviewed = true,
        .directstorage_14_preview_selected = true,
        .zstd_compression_format_reviewed = true,
        .cpu_decompression_path_reviewed = true,
        .gpu_decompression_path_reviewed = true,
        .gacl_public_preview_reviewed = true,
        .gacl_shuffle_transform_reviewed = true,
        .gacl_bcn_postprocess_scope_reviewed = true,
        .creator_id_support_reviewed = true,
        .explicit_api_usage_required = true,
        .authored_zstd_assets_required = true,
        .staging_buffer_over_256mb_known_issue_reviewed = true,
        .zstd_gpu_fallback_shader_tdr_known_issue_reviewed = true,
        .preview_no_runtime_promotion_reviewed = true,
    };
}

[[nodiscard]] std::vector<RuntimeMavgDirectStorageZstdPreviewRow> make_ready_rows() {
    auto zstd = make_preview_row(RuntimeMavgDirectStorageZstdPreviewFeatureKind::zstd_codec);
    zstd.row_id = "directstorage.zstd.preview.codec";
    zstd.official_source_id = "microsoft-directstorage-nuget-1.4-preview";

    auto gacl = make_preview_row(RuntimeMavgDirectStorageZstdPreviewFeatureKind::gacl_shuffle_transform);
    gacl.row_id = "directstorage.zstd.preview.gacl";
    gacl.official_source_id = "microsoft-directstorage-1.4-zstd-preview";

    auto creator = make_preview_row(RuntimeMavgDirectStorageZstdPreviewFeatureKind::creator_id);
    creator.row_id = "directstorage.zstd.preview.creator_id";
    creator.official_source_id = "microsoft-directstorage-api-downloads";

    auto known_issue = make_preview_row(RuntimeMavgDirectStorageZstdPreviewFeatureKind::preview_known_issue);
    known_issue.row_id = "directstorage.zstd.preview.known_issue";
    known_issue.official_source_id = "microsoft-directstorage-nuget-1.4-preview";

    return {zstd, gacl, creator, known_issue};
}

[[nodiscard]] bool has_code(const mirakana::runtime_rhi::RuntimeMavgDirectStorageZstdPreviewResult& result,
                            RuntimeMavgDirectStorageZstdPreviewDiagnosticCode code) noexcept {
    return mirakana::runtime_rhi::has_runtime_mavg_directstorage_zstd_preview_diagnostic(result, code);
}

} // namespace

MK_TEST("runtime rhi mavg directstorage zstd preview review accepts reviewed official preview rows") {
    const auto rows = make_ready_rows();
    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_zstd_preview_review({.rows = rows});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageZstdPreviewStatus::preview_review_ready);
    MK_REQUIRE(result.reviewed_rows == 4U);
    MK_REQUIRE(result.ready_rows == 4U);
    MK_REQUIRE(result.zstd_codec_rows == 1U);
    MK_REQUIRE(result.gacl_rows == 1U);
    MK_REQUIRE(result.creator_id_rows == 1U);
    MK_REQUIRE(result.preview_known_issue_rows == 1U);
    MK_REQUIRE(result.mavg_directstorage_zstd_preview_review_ready);
    MK_REQUIRE(result.mavg_directstorage_zstd_preview_selected);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
    MK_REQUIRE(!result.mavg_directstorage_zstd_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_gacl_pipeline_ready);
    MK_REQUIRE(result.mavg_directstorage_creator_id_policy_reviewed);
    MK_REQUIRE(!result.mavg_directstorage_native_handles_exposed);
    MK_REQUIRE(!result.mavg_directstorage_performance_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(!result.mavg_external_engine_compatibility);
}

MK_TEST("runtime rhi mavg directstorage zstd preview review host gates missing preview obligations") {
    auto rows = make_ready_rows();
    rows[0].sdk_package_version = "1.3.0";
    rows[0].directstorage_14_preview_selected = false;
    rows[0].zstd_compression_format_reviewed = false;
    rows[0].cpu_decompression_path_reviewed = false;
    rows[0].explicit_api_usage_required = false;
    rows[0].authored_zstd_assets_required = false;
    rows[0].preview_no_runtime_promotion_reviewed = false;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_zstd_preview_review({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageZstdPreviewStatus::review_required);
    MK_REQUIRE(result.review_required_rows == 1U);
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_preview_sdk_version));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::stable_sdk_selected_for_preview));
    MK_REQUIRE(has_code(result,
                        RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_directstorage_14_preview_selection));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_zstd_format_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_cpu_gpu_decompression_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_explicit_api_usage_requirement));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_authored_zstd_asset_requirement));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_preview_no_runtime_promotion_review));
}

MK_TEST("runtime rhi mavg directstorage zstd preview review requires gacl and known issue scope") {
    auto rows = make_ready_rows();
    rows[1].gacl_public_preview_reviewed = false;
    rows[1].gacl_shuffle_transform_reviewed = false;
    rows[1].gacl_bcn_postprocess_scope_reviewed = false;
    rows[3].staging_buffer_over_256mb_known_issue_reviewed = false;
    rows[3].zstd_gpu_fallback_shader_tdr_known_issue_reviewed = false;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_zstd_preview_review({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageZstdPreviewStatus::review_required);
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_gacl_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_gacl_shuffle_transform_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_gacl_postprocess_scope_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_staging_buffer_known_issue_review));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_zstd_fallback_shader_known_issue_review));
}

MK_TEST("runtime rhi mavg directstorage zstd preview review blocks execution and external claims") {
    auto rows = make_ready_rows();
    rows[0].claims_zstd_preview_runtime_ready = true;
    rows[0].claims_zstd_execution_ready = true;
    rows[0].claims_gacl_pipeline_ready = true;
    rows[0].claims_performance_gain = true;
    rows[0].claims_package_visible_backend_readiness = true;
    rows[0].claims_broad_mavg_backend_ready = true;
    rows[0].claims_broad_optimization_ready = true;
    rows[0].claims_nanite_compatibility = true;
    rows[0].claims_nanite_equivalence = true;
    rows[0].claims_nanite_superiority = true;
    rows[0].claims_unity_unreal_godot_compatibility = true;
    rows[0].native_handles_exposed = true;
    rows[0].dll_replacement_enables_features_claimed = true;
    rows[0].gacl_bc7_postprocess_claimed = true;

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_zstd_preview_review({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageZstdPreviewStatus::blocked);
    MK_REQUIRE(result.blocked_rows == 1U);
    MK_REQUIRE(result.mavg_directstorage_native_handles_exposed);
    MK_REQUIRE(!result.mavg_directstorage_zstd_preview_ready);
    MK_REQUIRE(!result.mavg_directstorage_zstd_execution_ready);
    MK_REQUIRE(!result.mavg_directstorage_gacl_pipeline_ready);
    MK_REQUIRE(!result.mavg_directstorage_performance_ready);
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_cpu_gpu_memory_optimization_ready);
    MK_REQUIRE(!result.mavg_nanite_compatible);
    MK_REQUIRE(!result.mavg_nanite_equivalent);
    MK_REQUIRE(!result.mavg_nanite_superior);
    MK_REQUIRE(!result.mavg_external_engine_compatibility);
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::dll_replacement_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::native_handle_exposure));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::execution_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::performance_claim_not_allowed));
    MK_REQUIRE(has_code(
        result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::package_backend_readiness_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::broad_backend_claim_not_allowed));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::broad_optimization_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::nanite_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::external_engine_claim_not_allowed));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::bc7_postprocess_claim_not_allowed));
}

MK_TEST("runtime rhi mavg directstorage zstd preview review requires all four feature rows") {
    const std::vector<RuntimeMavgDirectStorageZstdPreviewRow> rows{
        make_preview_row(RuntimeMavgDirectStorageZstdPreviewFeatureKind::zstd_codec),
    };

    const auto result = mirakana::runtime_rhi::evaluate_runtime_mavg_directstorage_zstd_preview_review({.rows = rows});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == RuntimeMavgDirectStorageZstdPreviewStatus::review_required);
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_gacl_review));
    MK_REQUIRE(has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_creator_id_review));
    MK_REQUIRE(
        has_code(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_staging_buffer_known_issue_review));
}

int main() {
    return mirakana::test::run_all();
}
