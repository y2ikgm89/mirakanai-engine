// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgDirectStorageZstdPreviewFeatureKind : std::uint8_t {
    zstd_codec,
    gacl_shuffle_transform,
    creator_id,
    preview_known_issue,
};

enum class RuntimeMavgDirectStorageZstdPreviewStatus : std::uint8_t {
    review_required,
    blocked,
    preview_review_ready,
};

enum class RuntimeMavgDirectStorageZstdPreviewDiagnosticCode : std::uint8_t {
    none,
    missing_official_source,
    missing_preview_sdk_version,
    stable_sdk_selected_for_preview,
    missing_directstorage_14_preview_selection,
    missing_zstd_format_review,
    missing_cpu_gpu_decompression_review,
    missing_gacl_review,
    missing_gacl_shuffle_transform_review,
    missing_gacl_postprocess_scope_review,
    bc7_postprocess_claim_not_allowed,
    missing_creator_id_review,
    missing_explicit_api_usage_requirement,
    missing_authored_zstd_asset_requirement,
    missing_staging_buffer_known_issue_review,
    missing_zstd_fallback_shader_known_issue_review,
    missing_preview_no_runtime_promotion_review,
    dll_replacement_claim_not_allowed,
    native_handle_exposure,
    execution_claim_not_allowed,
    performance_claim_not_allowed,
    package_backend_readiness_claim_not_allowed,
    broad_backend_claim_not_allowed,
    broad_optimization_claim_not_allowed,
    nanite_claim_not_allowed,
    external_engine_claim_not_allowed,
};

struct RuntimeMavgDirectStorageZstdPreviewDiagnostic {
    RuntimeMavgDirectStorageZstdPreviewDiagnosticCode code{RuntimeMavgDirectStorageZstdPreviewDiagnosticCode::none};
    std::string message;
};

struct RuntimeMavgDirectStorageZstdPreviewRow {
    RuntimeMavgDirectStorageZstdPreviewFeatureKind feature{RuntimeMavgDirectStorageZstdPreviewFeatureKind::zstd_codec};
    std::string_view row_id;
    std::string_view official_source_id;
    std::string_view sdk_package_version;
    bool reviewed{false};
    bool directstorage_14_preview_selected{false};
    bool zstd_compression_format_reviewed{false};
    bool cpu_decompression_path_reviewed{false};
    bool gpu_decompression_path_reviewed{false};
    bool gacl_public_preview_reviewed{false};
    bool gacl_shuffle_transform_reviewed{false};
    bool gacl_bcn_postprocess_scope_reviewed{false};
    bool gacl_bc7_postprocess_claimed{false};
    bool creator_id_support_reviewed{false};
    bool explicit_api_usage_required{false};
    bool authored_zstd_assets_required{false};
    bool staging_buffer_over_256mb_known_issue_reviewed{false};
    bool zstd_gpu_fallback_shader_tdr_known_issue_reviewed{false};
    bool preview_no_runtime_promotion_reviewed{false};
    bool dll_replacement_enables_features_claimed{false};
    bool native_handles_exposed{false};
    bool claims_zstd_preview_runtime_ready{false};
    bool claims_zstd_execution_ready{false};
    bool claims_gacl_pipeline_ready{false};
    bool claims_performance_gain{false};
    bool claims_package_visible_backend_readiness{false};
    bool claims_broad_mavg_backend_ready{false};
    bool claims_broad_optimization_ready{false};
    bool claims_nanite_compatibility{false};
    bool claims_nanite_equivalence{false};
    bool claims_nanite_superiority{false};
    bool claims_unity_unreal_godot_compatibility{false};
};

struct RuntimeMavgDirectStorageZstdPreviewDesc {
    std::span<const RuntimeMavgDirectStorageZstdPreviewRow> rows;
};

struct RuntimeMavgDirectStorageZstdPreviewResult {
    RuntimeMavgDirectStorageZstdPreviewStatus status{RuntimeMavgDirectStorageZstdPreviewStatus::review_required};
    std::vector<RuntimeMavgDirectStorageZstdPreviewDiagnostic> diagnostics;
    std::size_t reviewed_rows{0};
    std::size_t ready_rows{0};
    std::size_t zstd_codec_rows{0};
    std::size_t gacl_rows{0};
    std::size_t creator_id_rows{0};
    std::size_t preview_known_issue_rows{0};
    std::size_t blocked_rows{0};
    std::size_t review_required_rows{0};
    bool mavg_directstorage_zstd_preview_review_ready{false};
    bool mavg_directstorage_zstd_preview_selected{false};
    bool mavg_directstorage_zstd_preview_ready{false};
    bool mavg_directstorage_zstd_execution_ready{false};
    bool mavg_directstorage_gacl_pipeline_ready{false};
    bool mavg_directstorage_creator_id_policy_reviewed{false};
    bool mavg_directstorage_native_handles_exposed{false};
    bool mavg_directstorage_performance_ready{false};
    bool mavg_package_visible_backend_readiness_ready{false};
    bool mavg_broad_cpu_gpu_memory_optimization_ready{false};
    bool mavg_nanite_compatible{false};
    bool mavg_nanite_equivalent{false};
    bool mavg_nanite_superior{false};
    bool mavg_external_engine_compatibility{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == RuntimeMavgDirectStorageZstdPreviewStatus::preview_review_ready &&
               mavg_directstorage_zstd_preview_review_ready;
    }
};

[[nodiscard]] std::string_view
runtime_mavg_directstorage_zstd_preview_status_label(RuntimeMavgDirectStorageZstdPreviewStatus status) noexcept;
[[nodiscard]] RuntimeMavgDirectStorageZstdPreviewResult
evaluate_runtime_mavg_directstorage_zstd_preview_review(const RuntimeMavgDirectStorageZstdPreviewDesc& desc);
[[nodiscard]] bool
has_runtime_mavg_directstorage_zstd_preview_diagnostic(const RuntimeMavgDirectStorageZstdPreviewResult& result,
                                                       RuntimeMavgDirectStorageZstdPreviewDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
