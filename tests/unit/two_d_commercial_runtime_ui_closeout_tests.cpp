// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/two_d_commercial_runtime_ui_closeout.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace {

using mirakana::ui::RuntimeUiPackageSmokeSceneKind;
using mirakana::ui::RuntimeUiPackageSmokeSceneRow;
using mirakana::ui::RuntimeUiPlatformProductionEvidenceRow;
using mirakana::ui::RuntimeUiPlatformProductionFeature;
using mirakana::ui::RuntimeUiPlatformProductionProofKind;
using mirakana::ui::TwoDCommercialRuntimeUiCloseoutDesc;
using mirakana::ui::TwoDCommercialRuntimeUiCloseoutDiagnostic;
using mirakana::ui::TwoDCommercialRuntimeUiCloseoutDiagnosticCode;
using mirakana::ui::TwoDCommercialRuntimeUiOfficialSourceKind;
using mirakana::ui::TwoDCommercialRuntimeUiOfficialSourceRow;

[[nodiscard]] bool has_diagnostic(const std::vector<TwoDCommercialRuntimeUiCloseoutDiagnostic>& diagnostics,
                                  TwoDCommercialRuntimeUiCloseoutDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] RuntimeUiPlatformProductionEvidenceRow make_platform_row(RuntimeUiPlatformProductionFeature feature,
                                                                       RuntimeUiPlatformProductionProofKind proof,
                                                                       const char* id) {
    return RuntimeUiPlatformProductionEvidenceRow{
        .id = id,
        .feature = feature,
        .proof = proof,
        .selected = true,
        .ready = true,
        .dependency_recorded = true,
        .host_evidence_available = true,
    };
}

[[nodiscard]] std::vector<RuntimeUiPlatformProductionEvidenceRow> make_platform_rows() {
    auto rows = std::vector<RuntimeUiPlatformProductionEvidenceRow>{
        make_platform_row(RuntimeUiPlatformProductionFeature::visible_ui_editor,
                          RuntimeUiPlatformProductionProofKind::visible_editor_shell,
                          "runtime-ui-platform.visible-editor"),
        make_platform_row(RuntimeUiPlatformProductionFeature::production_text_shaping,
                          RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
                          "runtime-ui-platform.text-shaping.win32.directwrite"),
        make_platform_row(RuntimeUiPlatformProductionFeature::real_font_loading,
                          RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
                          "runtime-ui-platform.font-loading.win32.directwrite"),
        make_platform_row(RuntimeUiPlatformProductionFeature::font_rasterization,
                          RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
                          "runtime-ui-platform.font-rasterization.win32.directwrite"),
        make_platform_row(RuntimeUiPlatformProductionFeature::native_ime_session,
                          RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
                          "runtime-ui-platform.native-ime.win32.tsf"),
        make_platform_row(RuntimeUiPlatformProductionFeature::os_accessibility_publication,
                          RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
                          "runtime-ui-platform.accessibility.win32.uia"),
        make_platform_row(RuntimeUiPlatformProductionFeature::renderer_texture_upload_execution,
                          RuntimeUiPlatformProductionProofKind::selected_package_counter,
                          "runtime-ui-platform.renderer-upload.d3d12"),
        make_platform_row(RuntimeUiPlatformProductionFeature::clean_room_provenance,
                          RuntimeUiPlatformProductionProofKind::first_party_contract, "runtime-ui-platform.clean-room"),
        RuntimeUiPlatformProductionEvidenceRow{
            .id = "runtime-ui-platform.external-engine-parity-non-claim",
            .feature = RuntimeUiPlatformProductionFeature::external_engine_parity_non_claim,
            .proof = RuntimeUiPlatformProductionProofKind::unsupported_non_claim,
            .blocker = "Unity, Unreal, and Godot compatibility, parity, and replacement claims remain unsupported.",
        },
    };
    rows[6].renderer_upload_executed = true;
    return rows;
}

[[nodiscard]] std::vector<RuntimeUiPackageSmokeSceneRow> make_package_smoke_rows() {
    return {
        RuntimeUiPackageSmokeSceneRow{
            .id = "runtime-ui-package.multilingual",
            .kind = RuntimeUiPackageSmokeSceneKind::multilingual_glyph_fallback,
            .selected = true,
            .ready = true,
            .language_rows = 4U,
            .glyph_fallback_rows = 3U,
            .supporting_evidence_rows = 3U,
        },
        RuntimeUiPackageSmokeSceneRow{
            .id = "runtime-ui-package.long-label",
            .kind = RuntimeUiPackageSmokeSceneKind::long_label_layout,
            .selected = true,
            .ready = true,
            .long_label_code_units = 96U,
            .wrapped_line_rows = 2U,
            .supporting_evidence_rows = 2U,
        },
        RuntimeUiPackageSmokeSceneRow{
            .id = "runtime-ui-package.controller",
            .kind = RuntimeUiPackageSmokeSceneKind::controller_only_navigation,
            .selected = true,
            .ready = true,
            .controller_navigation_edges = 6U,
            .controller_glyph_rows = 2U,
            .supporting_evidence_rows = 2U,
        },
        RuntimeUiPackageSmokeSceneRow{
            .id = "runtime-ui-package.accessibility",
            .kind = RuntimeUiPackageSmokeSceneKind::accessibility_tree_review,
            .selected = true,
            .ready = true,
            .accessibility_nodes = 4U,
            .accessibility_action_rows = 2U,
            .accessibility_reading_order_rows = 4U,
            .supporting_evidence_rows = 3U,
        },
    };
}

[[nodiscard]] std::array<TwoDCommercialRuntimeUiOfficialSourceRow, 4U> make_official_source_rows() {
    return {
        TwoDCommercialRuntimeUiOfficialSourceRow{
            .id = "context7.microsoft.directwrite",
            .kind = TwoDCommercialRuntimeUiOfficialSourceKind::context7_directwrite,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        TwoDCommercialRuntimeUiOfficialSourceRow{
            .id = "context7.microsoft.win32-tsf-uia",
            .kind = TwoDCommercialRuntimeUiOfficialSourceKind::context7_win32_tsf_uia,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        TwoDCommercialRuntimeUiOfficialSourceRow{
            .id = "context7.microsoft.direct3d12",
            .kind = TwoDCommercialRuntimeUiOfficialSourceKind::context7_direct3d12,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        TwoDCommercialRuntimeUiOfficialSourceRow{
            .id = "repository.legal-policy",
            .kind = TwoDCommercialRuntimeUiOfficialSourceKind::repository_legal_policy,
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
    };
}

[[nodiscard]] TwoDCommercialRuntimeUiCloseoutDesc make_ready_desc() {
    return TwoDCommercialRuntimeUiCloseoutDesc{
        .platform_result = mirakana::ui::evaluate_runtime_ui_platform_production(make_platform_rows()),
        .package_smoke_review = mirakana::ui::review_runtime_ui_package_smoke_scenes(make_package_smoke_rows()),
        .official_source_rows = make_official_source_rows(),
        .selected_windows_d3d12_ready_claim = true,
    };
}

} // namespace

MK_TEST("2d commercial runtime ui closeout accepts selected Windows D3D12 evidence only") {
    const auto result = mirakana::ui::evaluate_2d_commercial_runtime_ui_closeout(make_ready_desc());

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.platform_ready);
    MK_REQUIRE(result.package_smoke_ready);
    MK_REQUIRE(result.official_source_ready);
    MK_REQUIRE(result.selected_windows_ready_rows == 4U);
    MK_REQUIRE(result.host_gated_rows == 10U);
    MK_REQUIRE(result.dependency_gated_rows == 2U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("2d commercial runtime ui closeout rejects missing package smoke and official docs") {
    auto desc = make_ready_desc();
    desc.package_smoke_review.ready = false;
    desc.official_source_rows[1].ready = false;

    const auto result = mirakana::ui::evaluate_2d_commercial_runtime_ui_closeout(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::package_smoke_not_ready));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::official_source_not_ready));
}

MK_TEST("2d commercial runtime ui closeout rejects broad platform parity and native handles") {
    auto desc = make_ready_desc();
    desc.public_native_handles = true;
    desc.cross_platform_parity_claim = true;
    desc.linux_ready_claim = true;
    desc.legal_approval_claim = true;

    const auto result = mirakana::ui::evaluate_2d_commercial_runtime_ui_closeout(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::public_native_handles));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::cross_platform_parity_claim));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::non_windows_ready_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::legal_approval_claim));
}

MK_TEST("2d commercial runtime ui closeout rejects unsafe adapter gate drift") {
    auto desc = make_ready_desc();
    desc.adapter_gate_rows = {
        mirakana::ui::RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.windows.directwrite",
            .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
            .status = mirakana::ui::RuntimeUiPlatformAdapterGateStatus::selected_proof,
            .selected = true,
            .ready = true,
        },
        mirakana::ui::RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.linux.harfbuzz_fontconfig",
            .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
            .status = mirakana::ui::RuntimeUiPlatformAdapterGateStatus::selected_proof,
            .selected = true,
            .ready = true,
        },
    };

    const auto result = mirakana::ui::evaluate_2d_commercial_runtime_ui_closeout(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::selected_adapter_gate_drift));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              TwoDCommercialRuntimeUiCloseoutDiagnosticCode::non_windows_adapter_promoted));
}

int main() {
    return mirakana::test::run_all();
}
