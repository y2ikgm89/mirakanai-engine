// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/two_d_commercial_release_legal_gate.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::Runtime2DCommercialReleaseDiagnostic;
using mirakana::runtime::Runtime2DCommercialReleaseDiagnosticCode;
using mirakana::runtime::Runtime2DCommercialReleaseEvidenceKind;
using mirakana::runtime::Runtime2DCommercialReleaseEvidenceRow;
using mirakana::runtime::Runtime2DCommercialReleaseGateDesc;
using mirakana::runtime::Runtime2DCommercialReleaseOfficialSourceKind;
using mirakana::runtime::Runtime2DCommercialReleaseOfficialSourceRow;
using mirakana::runtime::Runtime2DCommercialReleasePlatformGateKind;
using mirakana::runtime::Runtime2DCommercialReleasePlatformGateRow;

[[nodiscard]] bool has_diagnostic(const std::vector<Runtime2DCommercialReleaseDiagnostic>& diagnostics,
                                  Runtime2DCommercialReleaseDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] Runtime2DCommercialReleaseEvidenceRow evidence(Runtime2DCommercialReleaseEvidenceKind kind,
                                                             std::string id) {
    return Runtime2DCommercialReleaseEvidenceRow{
        .id = std::move(id),
        .kind = kind,
        .path_or_record_id = "games/sample_2d_desktop_runtime_package/game.agent.json",
        .validation_recipe_id = "installed-2d-commercial-release-legal-gate-smoke",
        .ready = true,
        .package_visible = true,
        .engineering_review_input = true,
        .counsel_review_required = true,
    };
}

[[nodiscard]] std::vector<Runtime2DCommercialReleaseEvidenceRow> make_evidence_rows() {
    return {
        evidence(Runtime2DCommercialReleaseEvidenceKind::package_content_inventory, "package-content-inventory"),
        evidence(Runtime2DCommercialReleaseEvidenceKind::third_party_notice_record, "third-party-notice-record"),
        evidence(Runtime2DCommercialReleaseEvidenceKind::dependency_manifest_record, "dependency-manifest-record"),
        evidence(Runtime2DCommercialReleaseEvidenceKind::source_provenance_summary, "source-provenance-summary"),
        evidence(Runtime2DCommercialReleaseEvidenceKind::clean_room_static_guard, "clean-room-static-guard"),
        evidence(Runtime2DCommercialReleaseEvidenceKind::trademark_surface_guard, "trademark-surface-guard"),
        evidence(Runtime2DCommercialReleaseEvidenceKind::distribution_artifact_inventory,
                 "distribution-artifact-inventory"),
        evidence(Runtime2DCommercialReleaseEvidenceKind::generated_asset_review, "generated-asset-review"),
        evidence(Runtime2DCommercialReleaseEvidenceKind::legal_review_input_record, "legal-review-input-record"),
    };
}

[[nodiscard]] std::array<Runtime2DCommercialReleaseOfficialSourceRow, 5U> make_official_source_rows() {
    return {
        Runtime2DCommercialReleaseOfficialSourceRow{
            .id = "microsoft.msix.signing",
            .kind = Runtime2DCommercialReleaseOfficialSourceKind::microsoft_msix_signing,
            .url = "https://learn.microsoft.com/en-us/windows/msix/package/signing-package-overview",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialReleaseOfficialSourceRow{
            .id = "apple.notarization.distribution",
            .kind = Runtime2DCommercialReleaseOfficialSourceKind::apple_notarization_distribution,
            .url = "https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialReleaseOfficialSourceRow{
            .id = "android.app.signing",
            .kind = Runtime2DCommercialReleaseOfficialSourceKind::android_app_signing,
            .url = "https://developer.android.com/studio/publish/app-signing",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialReleaseOfficialSourceRow{
            .id = "repository.2d-clean-room-ledger",
            .kind = Runtime2DCommercialReleaseOfficialSourceKind::repository_clean_room_ledger,
            .url = "docs/specs/2026-06-30-2d-commercial-clean-room-source-ledger-v1.md",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialReleaseOfficialSourceRow{
            .id = "repository.legal-policy",
            .kind = Runtime2DCommercialReleaseOfficialSourceKind::repository_legal_policy,
            .url = "docs/legal-and-licensing.md",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
    };
}

[[nodiscard]] std::vector<Runtime2DCommercialReleasePlatformGateRow> make_platform_gate_rows() {
    return {
        Runtime2DCommercialReleasePlatformGateRow{
            .id = "windows-msix-signing",
            .kind = Runtime2DCommercialReleasePlatformGateKind::windows_msix_signing,
            .official_source_id = "microsoft.msix.signing",
            .host_class_id = "windows-release-signing-host",
            .host_gated = true,
            .ready = false,
            .separate_platform_gate = true,
        },
        Runtime2DCommercialReleasePlatformGateRow{
            .id = "macos-notarization",
            .kind = Runtime2DCommercialReleasePlatformGateKind::macos_notarization,
            .official_source_id = "apple.notarization.distribution",
            .host_class_id = "macos-xcode-notarization-host",
            .host_gated = true,
            .ready = false,
            .separate_platform_gate = true,
        },
        Runtime2DCommercialReleasePlatformGateRow{
            .id = "android-play-signing",
            .kind = Runtime2DCommercialReleasePlatformGateKind::android_play_signing,
            .official_source_id = "android.app.signing",
            .host_class_id = "android-release-signing-host",
            .host_gated = true,
            .ready = false,
            .separate_platform_gate = true,
        },
    };
}

[[nodiscard]] Runtime2DCommercialReleaseGateDesc make_ready_desc() {
    return Runtime2DCommercialReleaseGateDesc{
        .evidence_rows = make_evidence_rows(),
        .official_source_rows = make_official_source_rows(),
        .platform_gate_rows = make_platform_gate_rows(),
        .selected_package_release_gate_claim = true,
    };
}

} // namespace

MK_TEST("runtime 2d commercial release legal gate accepts counsel ready engineering input") {
    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_release_legal_gate(make_ready_desc());

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.evidence_gate_ready);
    MK_REQUIRE(result.official_source_ready);
    MK_REQUIRE(result.platform_gate_ready);
    MK_REQUIRE(result.engineering_review_input_ready);
    MK_REQUIRE(result.counsel_review_required);
    MK_REQUIRE(result.evidence_rows == 9U);
    MK_REQUIRE(result.official_source_rows == 5U);
    MK_REQUIRE(result.platform_gate_rows == 3U);
    MK_REQUIRE(result.host_gated_platform_rows == 3U);
    MK_REQUIRE(result.release_blocker_rows == 0U);
    MK_REQUIRE(result.missing_notice_rows == 0U);
    MK_REQUIRE(result.unknown_license_rows == 0U);
    MK_REQUIRE(result.unapproved_dependency_rows == 0U);
    MK_REQUIRE(result.external_engine_claim_rows == 0U);
    MK_REQUIRE(result.legal_approval_claim_rows == 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime 2d commercial release legal gate rejects missing notices and unknown licenses") {
    auto desc = make_ready_desc();
    desc.evidence_rows[1].ready = false;
    desc.missing_notice_claim = true;
    desc.unknown_license_claim = true;
    desc.unreviewed_generated_asset_claim = true;

    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_release_legal_gate(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(result.release_blocker_rows == 3U);
    MK_REQUIRE(result.missing_notice_rows == 1U);
    MK_REQUIRE(result.unknown_license_rows == 1U);
    MK_REQUIRE(result.unreviewed_generated_asset_rows == 1U);
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::evidence_not_ready));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::missing_notice));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::unknown_license));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::unreviewed_generated_asset));
}

MK_TEST("runtime 2d commercial release legal gate rejects compatibility and approval claims") {
    auto desc = make_ready_desc();
    desc.external_engine_compatibility_claim = true;
    desc.copied_asset_claim = true;
    desc.external_engine_mark_claim = true;
    desc.legal_approval_claim = true;

    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_release_legal_gate(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(result.release_blocker_rows == 3U);
    MK_REQUIRE(result.external_engine_claim_rows == 1U);
    MK_REQUIRE(result.copied_asset_rows == 1U);
    MK_REQUIRE(result.external_engine_mark_rows == 1U);
    MK_REQUIRE(result.legal_approval_claim_rows == 1U);
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              Runtime2DCommercialReleaseDiagnosticCode::external_engine_compatibility_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::copied_asset));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::external_engine_mark));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialReleaseDiagnosticCode::legal_approval_claim));
}

int main() {
    return mirakana::test::run_all();
}
