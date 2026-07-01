// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/two_d_commercial_release_legal_gate.hpp"

#include <array>
#include <string>
#include <utility>

namespace mirakana::runtime {
namespace {

constexpr std::array<Runtime2DCommercialReleaseEvidenceKind, 9U> kRequiredEvidenceKinds{
    Runtime2DCommercialReleaseEvidenceKind::package_content_inventory,
    Runtime2DCommercialReleaseEvidenceKind::third_party_notice_record,
    Runtime2DCommercialReleaseEvidenceKind::dependency_manifest_record,
    Runtime2DCommercialReleaseEvidenceKind::source_provenance_summary,
    Runtime2DCommercialReleaseEvidenceKind::clean_room_static_guard,
    Runtime2DCommercialReleaseEvidenceKind::trademark_surface_guard,
    Runtime2DCommercialReleaseEvidenceKind::distribution_artifact_inventory,
    Runtime2DCommercialReleaseEvidenceKind::generated_asset_review,
    Runtime2DCommercialReleaseEvidenceKind::legal_review_input_record,
};

constexpr std::array<Runtime2DCommercialReleaseOfficialSourceKind, 5U> kRequiredOfficialSourceKinds{
    Runtime2DCommercialReleaseOfficialSourceKind::microsoft_msix_signing,
    Runtime2DCommercialReleaseOfficialSourceKind::apple_notarization_distribution,
    Runtime2DCommercialReleaseOfficialSourceKind::android_app_signing,
    Runtime2DCommercialReleaseOfficialSourceKind::repository_clean_room_ledger,
    Runtime2DCommercialReleaseOfficialSourceKind::repository_legal_policy,
};

constexpr std::array<Runtime2DCommercialReleasePlatformGateKind, 3U> kRequiredPlatformGateKinds{
    Runtime2DCommercialReleasePlatformGateKind::windows_msix_signing,
    Runtime2DCommercialReleasePlatformGateKind::macos_notarization,
    Runtime2DCommercialReleasePlatformGateKind::android_play_signing,
};

void append_diagnostic(std::vector<Runtime2DCommercialReleaseDiagnostic>& diagnostics,
                       Runtime2DCommercialReleaseDiagnosticCode code, std::string row_id, std::string message) {
    diagnostics.push_back(Runtime2DCommercialReleaseDiagnostic{
        .code = code,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool empty_required(const std::string& value) noexcept {
    return value.empty();
}

[[nodiscard]] bool evidence_row_ready(const Runtime2DCommercialReleaseEvidenceRow& row) noexcept {
    return row.ready && row.package_visible && row.engineering_review_input && row.counsel_review_required &&
           !empty_required(row.id) && !empty_required(row.path_or_record_id) &&
           !empty_required(row.validation_recipe_id);
}

[[nodiscard]] bool official_source_row_ready(const Runtime2DCommercialReleaseOfficialSourceRow& row) noexcept {
    return row.ready && row.official && row.public_docs_only && !empty_required(row.id) && !empty_required(row.url);
}

[[nodiscard]] bool platform_gate_row_ready(const Runtime2DCommercialReleasePlatformGateRow& row) noexcept {
    return row.separate_platform_gate && (row.host_gated || row.ready) && !empty_required(row.id) &&
           !empty_required(row.official_source_id) && !empty_required(row.host_class_id);
}

void evaluate_evidence_rows(const Runtime2DCommercialReleaseGateDesc& desc,
                            Runtime2DCommercialReleaseGateResult& result) {
    bool engineering_review_input_ready = true;
    bool counsel_review_required = true;
    for (const auto& row : desc.evidence_rows) {
        if (evidence_row_ready(row)) {
            ++result.evidence_rows;
        } else {
            append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::evidence_not_ready, row.id,
                              "2D commercial release evidence rows must be package-visible counsel-ready "
                              "engineering review inputs");
            engineering_review_input_ready = false;
            counsel_review_required = false;
        }
        engineering_review_input_ready = engineering_review_input_ready && row.engineering_review_input;
        counsel_review_required = counsel_review_required && row.counsel_review_required;
    }

    for (const auto kind : kRequiredEvidenceKinds) {
        bool found = false;
        for (const auto& row : desc.evidence_rows) {
            if (row.kind == kind && evidence_row_ready(row)) {
                found = true;
                break;
            }
        }
        if (!found) {
            append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::evidence_not_ready,
                              std::string{runtime_2d_commercial_release_evidence_kind_name(kind)},
                              "2D commercial release gate is missing a required engineering evidence row");
        }
    }

    result.evidence_gate_ready = result.evidence_rows == kRequiredEvidenceKinds.size();
    result.engineering_review_input_ready = result.evidence_gate_ready && engineering_review_input_ready;
    result.counsel_review_required = result.evidence_gate_ready && counsel_review_required;
}

void evaluate_official_sources(const Runtime2DCommercialReleaseGateDesc& desc,
                               Runtime2DCommercialReleaseGateResult& result) {
    for (const auto& row : desc.official_source_rows) {
        if (official_source_row_ready(row)) {
            ++result.official_source_rows;
        } else {
            append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::official_source_not_ready,
                              row.id,
                              "2D commercial release official-source rows must be official public documentation rows");
        }
    }

    for (const auto kind : kRequiredOfficialSourceKinds) {
        bool found = false;
        for (const auto& row : desc.official_source_rows) {
            if (row.kind == kind && official_source_row_ready(row)) {
                found = true;
                break;
            }
        }
        if (!found) {
            append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::official_source_not_ready,
                              std::string{runtime_2d_commercial_release_official_source_kind_name(kind)},
                              "2D commercial release gate is missing a required official-source row");
        }
    }

    result.official_source_ready = result.official_source_rows == kRequiredOfficialSourceKinds.size();
}

void evaluate_platform_gates(const Runtime2DCommercialReleaseGateDesc& desc,
                             Runtime2DCommercialReleaseGateResult& result) {
    for (const auto& row : desc.platform_gate_rows) {
        if (platform_gate_row_ready(row)) {
            ++result.platform_gate_rows;
        } else {
            append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::platform_gate_not_ready,
                              row.id,
                              "2D commercial release signing, notarization, and store rows must remain separate "
                              "platform gates backed by official documentation");
        }
        if (row.host_gated) {
            ++result.host_gated_platform_rows;
        }
    }

    for (const auto kind : kRequiredPlatformGateKinds) {
        bool found = false;
        for (const auto& row : desc.platform_gate_rows) {
            if (row.kind == kind && platform_gate_row_ready(row)) {
                found = true;
                break;
            }
        }
        if (!found) {
            append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::platform_gate_not_ready,
                              std::string{runtime_2d_commercial_release_platform_gate_kind_name(kind)},
                              "2D commercial release gate is missing a required platform release gate row");
        }
    }

    result.platform_gate_ready = result.platform_gate_rows == kRequiredPlatformGateKinds.size();
}

void evaluate_blocker_claims(const Runtime2DCommercialReleaseGateDesc& desc,
                             Runtime2DCommercialReleaseGateResult& result) {
    if (!desc.selected_package_release_gate_claim) {
        append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::selected_package_claim_missing,
                          {}, "2D commercial release gate must be scoped to the selected package lane");
    }
    if (desc.missing_notice_claim) {
        ++result.missing_notice_rows;
        ++result.release_blocker_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::missing_notice, {},
                          "Missing third-party notice rows block 2D commercial release handoff");
    }
    if (desc.unknown_license_claim) {
        ++result.unknown_license_rows;
        ++result.release_blocker_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::unknown_license, {},
                          "Unknown license rows block 2D commercial release handoff");
    }
    if (desc.unapproved_dependency_claim) {
        ++result.unapproved_dependency_rows;
        ++result.release_blocker_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::unapproved_dependency, {},
                          "Unapproved dependencies block 2D commercial release handoff");
    }
    if (desc.external_engine_mark_claim) {
        ++result.external_engine_mark_rows;
        ++result.release_blocker_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::external_engine_mark, {},
                          "External commercial engine marks block 2D commercial release handoff");
    }
    if (desc.copied_asset_claim) {
        ++result.copied_asset_rows;
        ++result.release_blocker_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::copied_asset, {},
                          "Copied or unproven assets block 2D commercial release handoff");
    }
    if (desc.unreviewed_generated_asset_claim) {
        ++result.unreviewed_generated_asset_rows;
        ++result.release_blocker_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::unreviewed_generated_asset, {},
                          "Unreviewed generated assets block 2D commercial release handoff");
    }
    if (desc.external_engine_compatibility_claim) {
        ++result.external_engine_claim_rows;
        ++result.release_blocker_rows;
        append_diagnostic(result.diagnostics,
                          Runtime2DCommercialReleaseDiagnosticCode::external_engine_compatibility_claim, {},
                          "2D commercial release gate must not claim external commercial engine compatibility");
    }
    if (desc.legal_approval_claim) {
        ++result.legal_approval_claim_rows;
        append_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::legal_approval_claim, {},
                          "2D commercial release gate can produce engineering input but not legal approval");
    }
}

} // namespace

bool Runtime2DCommercialReleaseGateResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

std::string_view
runtime_2d_commercial_release_evidence_kind_name(Runtime2DCommercialReleaseEvidenceKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialReleaseEvidenceKind::package_content_inventory:
        return "package_content_inventory";
    case Runtime2DCommercialReleaseEvidenceKind::third_party_notice_record:
        return "third_party_notice_record";
    case Runtime2DCommercialReleaseEvidenceKind::dependency_manifest_record:
        return "dependency_manifest_record";
    case Runtime2DCommercialReleaseEvidenceKind::source_provenance_summary:
        return "source_provenance_summary";
    case Runtime2DCommercialReleaseEvidenceKind::clean_room_static_guard:
        return "clean_room_static_guard";
    case Runtime2DCommercialReleaseEvidenceKind::trademark_surface_guard:
        return "trademark_surface_guard";
    case Runtime2DCommercialReleaseEvidenceKind::distribution_artifact_inventory:
        return "distribution_artifact_inventory";
    case Runtime2DCommercialReleaseEvidenceKind::generated_asset_review:
        return "generated_asset_review";
    case Runtime2DCommercialReleaseEvidenceKind::legal_review_input_record:
        return "legal_review_input_record";
    }
    return "unknown";
}

std::string_view
runtime_2d_commercial_release_official_source_kind_name(Runtime2DCommercialReleaseOfficialSourceKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialReleaseOfficialSourceKind::microsoft_msix_signing:
        return "microsoft_msix_signing";
    case Runtime2DCommercialReleaseOfficialSourceKind::apple_notarization_distribution:
        return "apple_notarization_distribution";
    case Runtime2DCommercialReleaseOfficialSourceKind::android_app_signing:
        return "android_app_signing";
    case Runtime2DCommercialReleaseOfficialSourceKind::repository_clean_room_ledger:
        return "repository_clean_room_ledger";
    case Runtime2DCommercialReleaseOfficialSourceKind::repository_legal_policy:
        return "repository_legal_policy";
    }
    return "unknown";
}

std::string_view
runtime_2d_commercial_release_platform_gate_kind_name(Runtime2DCommercialReleasePlatformGateKind kind) noexcept {
    switch (kind) {
    case Runtime2DCommercialReleasePlatformGateKind::windows_msix_signing:
        return "windows_msix_signing";
    case Runtime2DCommercialReleasePlatformGateKind::macos_notarization:
        return "macos_notarization";
    case Runtime2DCommercialReleasePlatformGateKind::android_play_signing:
        return "android_play_signing";
    }
    return "unknown";
}

Runtime2DCommercialReleaseGateResult
evaluate_runtime_2d_commercial_release_legal_gate(const Runtime2DCommercialReleaseGateDesc& desc) {
    Runtime2DCommercialReleaseGateResult result;

    evaluate_evidence_rows(desc, result);
    evaluate_official_sources(desc, result);
    evaluate_platform_gates(desc, result);
    evaluate_blocker_claims(desc, result);

    result.ready = desc.selected_package_release_gate_claim && result.evidence_gate_ready &&
                   result.official_source_ready && result.platform_gate_ready &&
                   result.engineering_review_input_ready && result.counsel_review_required &&
                   result.release_blocker_rows == 0U && result.legal_approval_claim_rows == 0U;
    return result;
}

} // namespace mirakana::runtime
