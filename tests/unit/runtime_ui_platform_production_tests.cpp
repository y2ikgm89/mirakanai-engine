// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_platform_production.hpp"

#include <algorithm>
#include <vector>

namespace {

using mirakana::ui::RuntimeUiPlatformProductionDiagnostic;
using mirakana::ui::RuntimeUiPlatformProductionDiagnosticCode;
using mirakana::ui::RuntimeUiPlatformProductionEvidenceRow;
using mirakana::ui::RuntimeUiPlatformProductionFeature;
using mirakana::ui::RuntimeUiPlatformProductionProofKind;

[[nodiscard]] bool has_diagnostic(const std::vector<RuntimeUiPlatformProductionDiagnostic>& diagnostics,
                                  RuntimeUiPlatformProductionDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] RuntimeUiPlatformProductionEvidenceRow
make_row(RuntimeUiPlatformProductionFeature feature, RuntimeUiPlatformProductionProofKind proof, const char* id) {
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

[[nodiscard]] std::vector<RuntimeUiPlatformProductionEvidenceRow> make_complete_rows() {
    auto rows = std::vector<RuntimeUiPlatformProductionEvidenceRow>{
        make_row(RuntimeUiPlatformProductionFeature::visible_ui_editor,
                 RuntimeUiPlatformProductionProofKind::visible_editor_shell, "runtime-ui-platform.visible-editor"),
        make_row(RuntimeUiPlatformProductionFeature::production_text_shaping,
                 RuntimeUiPlatformProductionProofKind::official_sdk_adapter, "runtime-ui-platform.text-shaping"),
        make_row(RuntimeUiPlatformProductionFeature::real_font_loading,
                 RuntimeUiPlatformProductionProofKind::official_sdk_adapter, "runtime-ui-platform.font-loading"),
        make_row(RuntimeUiPlatformProductionFeature::font_rasterization,
                 RuntimeUiPlatformProductionProofKind::audited_dependency_adapter,
                 "runtime-ui-platform.font-rasterization"),
        make_row(RuntimeUiPlatformProductionFeature::native_ime_session,
                 RuntimeUiPlatformProductionProofKind::official_sdk_adapter, "runtime-ui-platform.native-ime"),
        make_row(RuntimeUiPlatformProductionFeature::os_accessibility_publication,
                 RuntimeUiPlatformProductionProofKind::official_sdk_adapter, "runtime-ui-platform.accessibility"),
        make_row(RuntimeUiPlatformProductionFeature::renderer_texture_upload_execution,
                 RuntimeUiPlatformProductionProofKind::selected_package_counter, "runtime-ui-platform.renderer-upload"),
        make_row(RuntimeUiPlatformProductionFeature::clean_room_provenance,
                 RuntimeUiPlatformProductionProofKind::first_party_contract, "runtime-ui-platform.clean-room"),
        RuntimeUiPlatformProductionEvidenceRow{
            .id = "runtime-ui-platform.external-engine-parity-non-claim",
            .feature = RuntimeUiPlatformProductionFeature::external_engine_parity_non_claim,
            .proof = RuntimeUiPlatformProductionProofKind::unsupported_non_claim,
            .selected = false,
            .ready = false,
            .dependency_recorded = true,
            .host_evidence_available = true,
            .blocker =
                "Unity, Unreal, and Godot compatibility, visual parity, and API parity are intentionally not claimed.",
        },
    };
    rows[6].renderer_upload_executed = true;
    return rows;
}

} // namespace

MK_TEST("runtime ui platform production gate requires clean-room and external parity non-claim rows") {
    auto rows = make_complete_rows();
    rows.erase(rows.begin() + 8);
    rows.erase(rows.begin() + 7);

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::missing_feature_row));
}

MK_TEST("runtime ui platform production gate requires renderer upload execution evidence") {
    auto rows = make_complete_rows();
    rows[6].renderer_upload_executed = false;

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::renderer_upload_missing));
}

MK_TEST("runtime ui platform production gate rejects native handles and copied sources") {
    auto rows = make_complete_rows();
    rows[0].public_native_handles = true;
    rows[1].copied_external_source = true;
    rows[2].copied_external_asset = true;

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::public_native_handles));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::copied_external_source));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::copied_external_asset));
}

MK_TEST("runtime ui platform production gate rejects duplicate dependency host middleware and row budget issues") {
    auto rows = make_complete_rows();
    rows.push_back(rows[0]);
    rows.back().id = rows[1].id;
    rows[1].dependency_recorded = false;
    rows[4].proof = RuntimeUiPlatformProductionProofKind::host_gate;
    rows[4].host_evidence_available = false;
    rows[4].blocker.clear();
    rows[5].middleware_api_exposure = true;

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows, 4U);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::duplicate_row_id));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::dependency_not_recorded));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::host_evidence_missing));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::middleware_api_exposure));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::row_budget_overflow));
}

MK_TEST("runtime ui platform production gate rejects broad production claims without every selected row") {
    auto rows = make_complete_rows();
    rows[1].external_engine_parity_claim = true;
    rows[3].ready = false;

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::external_engine_parity_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::selected_row_not_ready));
}

MK_TEST("runtime ui platform production gate becomes ready only with all selected evidence and non-claims") {
    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(make_complete_rows());

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.selected_rows == 8U);
    MK_REQUIRE(result.ready_rows == 8U);
    MK_REQUIRE(result.unsupported_non_claim_rows == 1U);
    MK_REQUIRE(result.diagnostics.empty());
}

int main() {
    return mirakana::test::run_all();
}
