// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_directstorage_zstd_preview_review.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {

std::string_view
runtime_mavg_directstorage_zstd_preview_status_label(RuntimeMavgDirectStorageZstdPreviewStatus status) noexcept {
    switch (status) {
    case RuntimeMavgDirectStorageZstdPreviewStatus::review_required:
        return "review_required";
    case RuntimeMavgDirectStorageZstdPreviewStatus::blocked:
        return "blocked";
    case RuntimeMavgDirectStorageZstdPreviewStatus::preview_review_ready:
        return "preview_review_ready";
    }
    return "unknown";
}

namespace {

void add_diagnostic(RuntimeMavgDirectStorageZstdPreviewResult& result,
                    RuntimeMavgDirectStorageZstdPreviewDiagnosticCode code, std::string message) {
    result.diagnostics.push_back(
        RuntimeMavgDirectStorageZstdPreviewDiagnostic{.code = code, .message = std::move(message)});
}

[[nodiscard]] bool blank(std::string_view value) noexcept {
    return value.empty();
}

[[nodiscard]] bool is_preview_sdk_version(std::string_view value) noexcept {
    return value.starts_with("1.4.0-preview");
}

[[nodiscard]] bool has_blocked_claims(const RuntimeMavgDirectStorageZstdPreviewRow& row) noexcept {
    return row.dll_replacement_enables_features_claimed || row.native_handles_exposed ||
           row.claims_zstd_preview_runtime_ready || row.claims_zstd_execution_ready || row.claims_gacl_pipeline_ready ||
           row.claims_performance_gain || row.claims_package_visible_backend_readiness ||
           row.claims_broad_mavg_backend_ready || row.claims_broad_optimization_ready ||
           row.claims_nanite_compatibility || row.claims_nanite_equivalence || row.claims_nanite_superiority ||
           row.claims_unity_unreal_godot_compatibility || row.gacl_bc7_postprocess_claimed;
}

void count_feature(RuntimeMavgDirectStorageZstdPreviewResult& result,
                   RuntimeMavgDirectStorageZstdPreviewFeatureKind feature) {
    switch (feature) {
    case RuntimeMavgDirectStorageZstdPreviewFeatureKind::zstd_codec:
        ++result.zstd_codec_rows;
        break;
    case RuntimeMavgDirectStorageZstdPreviewFeatureKind::gacl_shuffle_transform:
        ++result.gacl_rows;
        break;
    case RuntimeMavgDirectStorageZstdPreviewFeatureKind::creator_id:
        ++result.creator_id_rows;
        break;
    case RuntimeMavgDirectStorageZstdPreviewFeatureKind::preview_known_issue:
        ++result.preview_known_issue_rows;
        break;
    }
}

void validate_blocked_claims(RuntimeMavgDirectStorageZstdPreviewResult& result,
                             const RuntimeMavgDirectStorageZstdPreviewRow& row) {
    if (row.gacl_bc7_postprocess_claimed) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::bc7_postprocess_claim_not_allowed,
                       "DirectStorage 1.4/GACL preview review must not claim BC7 post-processing support before a "
                       "future Microsoft DirectStorage update is reviewed");
    }
    if (row.dll_replacement_enables_features_claimed) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::dll_replacement_claim_not_allowed,
                       "DirectStorage 1.4/Zstd preview features require authored Zstd/GACL assets and explicit API "
                       "usage; replacing redistributable DLLs is not review evidence");
    }
    if (row.native_handles_exposed) {
        result.mavg_directstorage_native_handles_exposed = true;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::native_handle_exposure,
                       "MAVG DirectStorage Zstd preview review rows must not expose DirectStorage, D3D12, or native "
                       "SDK handles");
    }
    if (row.claims_zstd_preview_runtime_ready || row.claims_zstd_execution_ready || row.claims_gacl_pipeline_ready) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::execution_claim_not_allowed,
                       "MAVG DirectStorage Zstd preview review is policy evidence only and must not claim runtime, "
                       "execution, or GACL pipeline readiness");
    }
    if (row.claims_performance_gain) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::performance_claim_not_allowed,
                       "MAVG DirectStorage Zstd preview review must not claim performance readiness or speedup");
    }
    if (row.claims_package_visible_backend_readiness) {
        add_diagnostic(result,
                       RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::package_backend_readiness_claim_not_allowed,
                       "MAVG DirectStorage Zstd preview review must not promote package-visible backend readiness");
    }
    if (row.claims_broad_mavg_backend_ready) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::broad_backend_claim_not_allowed,
                       "MAVG DirectStorage Zstd preview review must not claim broad MAVG backend readiness");
    }
    if (row.claims_broad_optimization_ready) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::broad_optimization_claim_not_allowed,
                       "MAVG DirectStorage Zstd preview review must not claim broad CPU/GPU/memory optimization");
    }
    if (row.claims_nanite_compatibility || row.claims_nanite_equivalence || row.claims_nanite_superiority) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::nanite_claim_not_allowed,
                       "MAVG DirectStorage Zstd preview review must not claim Nanite compatibility, equivalence, or "
                       "superiority");
    }
    if (row.claims_unity_unreal_godot_compatibility) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::external_engine_claim_not_allowed,
                       "MAVG DirectStorage Zstd preview review must not claim Unity, Unreal, or Godot compatibility");
    }
}

[[nodiscard]] bool validate_required_evidence(RuntimeMavgDirectStorageZstdPreviewResult& result,
                                              const RuntimeMavgDirectStorageZstdPreviewRow& row) {
    bool valid = true;

    if (blank(row.official_source_id)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_official_source,
                       "MAVG DirectStorage Zstd preview rows require Microsoft official source ids");
    }
    if (!is_preview_sdk_version(row.sdk_package_version)) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_preview_sdk_version,
                       "MAVG DirectStorage Zstd preview rows require the reviewed DirectStorage 1.4 preview SDK "
                       "package version");
    }
    if (!blank(row.sdk_package_version) && !is_preview_sdk_version(row.sdk_package_version) &&
        !row.directstorage_14_preview_selected) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::stable_sdk_selected_for_preview,
                       "Stable DirectStorage SDK rows cannot satisfy the Zstd/GACL preview review gate");
    }
    if (!row.reviewed || !row.directstorage_14_preview_selected) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_directstorage_14_preview_selection,
                       "MAVG DirectStorage Zstd preview rows require reviewed DirectStorage 1.4 public-preview "
                       "selection");
    }
    if (!row.zstd_compression_format_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_zstd_format_review,
                       "MAVG DirectStorage Zstd preview rows require DSTORAGE_COMPRESSION_FORMAT_ZSTD review");
    }
    if (!row.cpu_decompression_path_reviewed || !row.gpu_decompression_path_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_cpu_gpu_decompression_review,
                       "MAVG DirectStorage Zstd preview rows require both CPU and GPU decompression path review");
    }
    if (!row.gacl_public_preview_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_gacl_review,
                       "MAVG DirectStorage Zstd preview rows require Game Asset Conditioning Library preview review");
    }
    if (!row.gacl_shuffle_transform_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_gacl_shuffle_transform_review,
                       "MAVG DirectStorage Zstd preview rows require GACL shuffle transform review");
    }
    if (!row.gacl_bcn_postprocess_scope_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_gacl_postprocess_scope_review,
                       "MAVG DirectStorage Zstd preview rows require BC1/BC3/BC4/BC5 post-processing scope review");
    }
    if (!row.creator_id_support_reviewed) {
        valid = false;
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_creator_id_review,
                       "MAVG DirectStorage Zstd preview rows require D3D12 CreatorID support review");
    }
    if (!row.explicit_api_usage_required) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_explicit_api_usage_requirement,
                       "MAVG DirectStorage Zstd preview rows require explicit API usage before future promotion");
    }
    if (!row.authored_zstd_assets_required) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_authored_zstd_asset_requirement,
                       "MAVG DirectStorage Zstd preview rows require explicitly authored and compressed Zstd/GACL "
                       "assets before future promotion");
    }
    if (!row.staging_buffer_over_256mb_known_issue_reviewed) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_staging_buffer_known_issue_review,
                       "MAVG DirectStorage Zstd preview rows require the preview staging-buffer known issue review");
    }
    if (!row.zstd_gpu_fallback_shader_tdr_known_issue_reviewed) {
        valid = false;
        add_diagnostic(
            result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_zstd_fallback_shader_known_issue_review,
            "MAVG DirectStorage Zstd preview rows require the preview Zstd GPU fallback shader TDR known issue review");
    }
    if (!row.preview_no_runtime_promotion_reviewed) {
        valid = false;
        add_diagnostic(result,
                       RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_preview_no_runtime_promotion_review,
                       "MAVG DirectStorage Zstd preview rows must record that preview review does not promote runtime "
                       "execution, package, or performance readiness");
    }

    return valid;
}

} // namespace

RuntimeMavgDirectStorageZstdPreviewResult
evaluate_runtime_mavg_directstorage_zstd_preview_review(const RuntimeMavgDirectStorageZstdPreviewDesc& desc) {
    RuntimeMavgDirectStorageZstdPreviewResult result;

    bool has_zstd_codec = false;
    bool has_gacl = false;
    bool has_creator_id = false;
    bool has_known_issue = false;
    bool blocked = false;

    for (const auto& row : desc.rows) {
        if (row.reviewed) {
            ++result.reviewed_rows;
        }
        if (row.directstorage_14_preview_selected) {
            result.mavg_directstorage_zstd_preview_selected = true;
        }
        count_feature(result, row.feature);

        validate_blocked_claims(result, row);
        const bool row_blocked = has_blocked_claims(row);
        const bool row_valid = validate_required_evidence(result, row);

        if (row_blocked) {
            ++result.blocked_rows;
            blocked = true;
            continue;
        }
        if (!row_valid) {
            ++result.review_required_rows;
            continue;
        }

        ++result.ready_rows;
        has_zstd_codec = has_zstd_codec || row.feature == RuntimeMavgDirectStorageZstdPreviewFeatureKind::zstd_codec;
        has_gacl = has_gacl || row.feature == RuntimeMavgDirectStorageZstdPreviewFeatureKind::gacl_shuffle_transform;
        has_creator_id = has_creator_id || row.feature == RuntimeMavgDirectStorageZstdPreviewFeatureKind::creator_id;
        has_known_issue =
            has_known_issue || row.feature == RuntimeMavgDirectStorageZstdPreviewFeatureKind::preview_known_issue;
    }

    result.mavg_directstorage_creator_id_policy_reviewed = has_creator_id;

    if (!has_zstd_codec) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_zstd_format_review,
                       "MAVG DirectStorage Zstd preview review requires one Zstd codec row");
    }
    if (!has_gacl) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_gacl_review,
                       "MAVG DirectStorage Zstd preview review requires one GACL row");
    }
    if (!has_creator_id) {
        add_diagnostic(result, RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_creator_id_review,
                       "MAVG DirectStorage Zstd preview review requires one D3D12 CreatorID row");
    }
    if (!has_known_issue) {
        add_diagnostic(result,
                       RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::missing_staging_buffer_known_issue_review,
                       "MAVG DirectStorage Zstd preview review requires one preview known-issue row");
    }

    if (blocked) {
        result.status = RuntimeMavgDirectStorageZstdPreviewStatus::blocked;
        return result;
    }
    if (!result.diagnostics.empty() || !has_zstd_codec || !has_gacl || !has_creator_id || !has_known_issue) {
        result.status = RuntimeMavgDirectStorageZstdPreviewStatus::review_required;
        return result;
    }

    result.status = RuntimeMavgDirectStorageZstdPreviewStatus::preview_review_ready;
    result.mavg_directstorage_zstd_preview_review_ready = true;
    return result;
}

bool has_runtime_mavg_directstorage_zstd_preview_diagnostic(
    const RuntimeMavgDirectStorageZstdPreviewResult& result,
    RuntimeMavgDirectStorageZstdPreviewDiagnosticCode code) noexcept {
    return std::ranges::any_of(
        result.diagnostics,
        [code](const RuntimeMavgDirectStorageZstdPreviewDiagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana::runtime_rhi
