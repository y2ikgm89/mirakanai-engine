// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime {

enum class Runtime2DCommercialReleaseEvidenceKind : std::uint8_t {
    package_content_inventory,
    third_party_notice_record,
    dependency_manifest_record,
    source_provenance_summary,
    clean_room_static_guard,
    trademark_surface_guard,
    distribution_artifact_inventory,
    generated_asset_review,
    legal_review_input_record,
};

enum class Runtime2DCommercialReleaseOfficialSourceKind : std::uint8_t {
    microsoft_msix_signing,
    apple_notarization_distribution,
    android_app_signing,
    repository_clean_room_ledger,
    repository_legal_policy,
};

enum class Runtime2DCommercialReleasePlatformGateKind : std::uint8_t {
    windows_msix_signing,
    macos_notarization,
    android_play_signing,
};

enum class Runtime2DCommercialReleaseDiagnosticCode : std::uint8_t {
    evidence_not_ready,
    official_source_not_ready,
    platform_gate_not_ready,
    selected_package_claim_missing,
    missing_notice,
    unknown_license,
    unapproved_dependency,
    external_engine_mark,
    copied_asset,
    unreviewed_generated_asset,
    external_engine_compatibility_claim,
    legal_approval_claim,
};

struct Runtime2DCommercialReleaseEvidenceRow {
    std::string id;
    Runtime2DCommercialReleaseEvidenceKind kind{Runtime2DCommercialReleaseEvidenceKind::package_content_inventory};
    std::string path_or_record_id;
    std::string validation_recipe_id;
    bool ready{false};
    bool package_visible{false};
    bool engineering_review_input{false};
    bool counsel_review_required{false};
};

struct Runtime2DCommercialReleaseOfficialSourceRow {
    std::string id;
    Runtime2DCommercialReleaseOfficialSourceKind kind{
        Runtime2DCommercialReleaseOfficialSourceKind::microsoft_msix_signing};
    std::string url;
    bool ready{false};
    bool official{false};
    bool public_docs_only{false};
};

struct Runtime2DCommercialReleasePlatformGateRow {
    std::string id;
    Runtime2DCommercialReleasePlatformGateKind kind{Runtime2DCommercialReleasePlatformGateKind::windows_msix_signing};
    std::string official_source_id;
    std::string host_class_id;
    bool host_gated{false};
    bool ready{false};
    bool separate_platform_gate{false};
};

struct Runtime2DCommercialReleaseDiagnostic {
    Runtime2DCommercialReleaseDiagnosticCode code{Runtime2DCommercialReleaseDiagnosticCode::evidence_not_ready};
    std::string row_id;
    std::string message;
};

struct Runtime2DCommercialReleaseGateDesc {
    std::vector<Runtime2DCommercialReleaseEvidenceRow> evidence_rows;
    std::array<Runtime2DCommercialReleaseOfficialSourceRow, 5U> official_source_rows{};
    std::vector<Runtime2DCommercialReleasePlatformGateRow> platform_gate_rows;
    bool selected_package_release_gate_claim{false};
    bool missing_notice_claim{false};
    bool unknown_license_claim{false};
    bool unapproved_dependency_claim{false};
    bool external_engine_mark_claim{false};
    bool copied_asset_claim{false};
    bool unreviewed_generated_asset_claim{false};
    bool external_engine_compatibility_claim{false};
    bool legal_approval_claim{false};
};

struct Runtime2DCommercialReleaseGateResult {
    bool ready{false};
    bool evidence_gate_ready{false};
    bool official_source_ready{false};
    bool platform_gate_ready{false};
    bool engineering_review_input_ready{false};
    bool counsel_review_required{false};
    std::vector<Runtime2DCommercialReleaseDiagnostic> diagnostics;
    std::size_t evidence_rows{0U};
    std::size_t official_source_rows{0U};
    std::size_t platform_gate_rows{0U};
    std::size_t host_gated_platform_rows{0U};
    std::size_t release_blocker_rows{0U};
    std::size_t missing_notice_rows{0U};
    std::size_t unknown_license_rows{0U};
    std::size_t unapproved_dependency_rows{0U};
    std::size_t external_engine_mark_rows{0U};
    std::size_t copied_asset_rows{0U};
    std::size_t unreviewed_generated_asset_rows{0U};
    std::size_t external_engine_claim_rows{0U};
    std::size_t legal_approval_claim_rows{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::string_view
runtime_2d_commercial_release_evidence_kind_name(Runtime2DCommercialReleaseEvidenceKind kind) noexcept;
[[nodiscard]] std::string_view
runtime_2d_commercial_release_official_source_kind_name(Runtime2DCommercialReleaseOfficialSourceKind kind) noexcept;
[[nodiscard]] std::string_view
runtime_2d_commercial_release_platform_gate_kind_name(Runtime2DCommercialReleasePlatformGateKind kind) noexcept;
[[nodiscard]] Runtime2DCommercialReleaseGateResult
evaluate_runtime_2d_commercial_release_legal_gate(const Runtime2DCommercialReleaseGateDesc& desc);

} // namespace mirakana::runtime
