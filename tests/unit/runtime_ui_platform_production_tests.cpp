// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_platform_production.hpp"

#include <algorithm>
#include <string_view>
#include <vector>

namespace {

using mirakana::ui::RuntimeUiPlatformAdapterGateRow;
using mirakana::ui::RuntimeUiPlatformAdapterGateStatus;
using mirakana::ui::RuntimeUiPlatformProductionDiagnostic;
using mirakana::ui::RuntimeUiPlatformProductionDiagnosticCode;
using mirakana::ui::RuntimeUiPlatformProductionEvidenceRow;
using mirakana::ui::RuntimeUiPlatformProductionFeature;
using mirakana::ui::RuntimeUiPlatformProductionProofKind;

[[nodiscard]] bool has_diagnostic(const std::vector<RuntimeUiPlatformProductionDiagnostic>& diagnostics,
                                  RuntimeUiPlatformProductionDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] const RuntimeUiPlatformAdapterGateRow* find_gate(const std::vector<RuntimeUiPlatformAdapterGateRow>& rows,
                                                               std::string_view id) {
    const auto iter = std::ranges::find_if(rows, [id](const auto& row) { return row.id == id; });
    return iter == rows.end() ? nullptr : &*iter;
}

[[nodiscard]] bool has_feature(const RuntimeUiPlatformAdapterGateRow& row, RuntimeUiPlatformProductionFeature feature) {
    return std::ranges::find(row.features, feature) != row.features.end();
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

MK_TEST("runtime ui platform adapter gate catalog records exact cross platform gate ids") {
    const auto rows = mirakana::ui::make_runtime_ui_platform_adapter_gate_rows();
    const auto expected_ids = std::vector<std::string_view>{
        "runtime_ui.adapter.windows.directwrite",
        "runtime_ui.adapter.windows.tsf",
        "runtime_ui.adapter.windows.uia",
        "runtime_ui.upload.windows.d3d12",
        "runtime_ui.adapter.macos.core_text",
        "runtime_ui.adapter.macos.input_method_kit",
        "runtime_ui.adapter.macos.nsaccessibility",
        "runtime_ui.adapter.linux.harfbuzz_fontconfig",
        "runtime_ui.adapter.linux.freetype",
        "runtime_ui.adapter.linux.at_spi",
        "runtime_ui.adapter.android.text_input",
        "runtime_ui.adapter.android.accessibility",
        "runtime_ui.adapter.ios.uitextinput",
        "runtime_ui.adapter.ios.uiaccessibility",
        "runtime_ui.upload.vulkan",
        "runtime_ui.upload.metal",
    };

    MK_REQUIRE(rows.size() == expected_ids.size());
    for (const auto id : expected_ids) {
        MK_REQUIRE(find_gate(rows, id) != nullptr);
    }

    for (auto outer = rows.begin(); outer != rows.end(); ++outer) {
        for (auto inner = outer + 1; inner != rows.end(); ++inner) {
            MK_REQUIRE(outer->id != inner->id);
        }
    }
}

MK_TEST("runtime ui platform adapter gate catalog selects only implemented Windows and D3D12 proof rows") {
    const auto rows = mirakana::ui::make_runtime_ui_platform_adapter_gate_rows();

    const auto* directwrite = find_gate(rows, "runtime_ui.adapter.windows.directwrite");
    MK_REQUIRE(directwrite != nullptr);
    MK_REQUIRE(directwrite->selected);
    MK_REQUIRE(directwrite->ready);
    MK_REQUIRE(directwrite->dependency_recorded);
    MK_REQUIRE(directwrite->host_evidence_available);
    MK_REQUIRE(directwrite->status == RuntimeUiPlatformAdapterGateStatus::selected_proof);
    MK_REQUIRE(directwrite->proof == RuntimeUiPlatformProductionProofKind::official_sdk_adapter);
    MK_REQUIRE(has_feature(*directwrite, RuntimeUiPlatformProductionFeature::production_text_shaping));
    MK_REQUIRE(has_feature(*directwrite, RuntimeUiPlatformProductionFeature::real_font_loading));
    MK_REQUIRE(has_feature(*directwrite, RuntimeUiPlatformProductionFeature::font_rasterization));

    const auto selected_rows =
        static_cast<std::size_t>(std::ranges::count_if(rows, [](const auto& row) { return row.selected; }));
    const auto ready_rows =
        static_cast<std::size_t>(std::ranges::count_if(rows, [](const auto& row) { return row.ready; }));
    MK_REQUIRE(selected_rows == 4U);
    MK_REQUIRE(ready_rows == 4U);
    MK_REQUIRE(find_gate(rows, "runtime_ui.adapter.windows.tsf")->selected);
    MK_REQUIRE(find_gate(rows, "runtime_ui.adapter.windows.uia")->selected);
    MK_REQUIRE(find_gate(rows, "runtime_ui.upload.windows.d3d12")->selected);
    MK_REQUIRE(!find_gate(rows, "runtime_ui.upload.vulkan")->selected);
    MK_REQUIRE(!find_gate(rows, "runtime_ui.upload.metal")->selected);
}

MK_TEST("runtime ui platform adapter gate catalog keeps unimplemented rows blocked without optional dependencies") {
    const auto rows = mirakana::ui::make_runtime_ui_platform_adapter_gate_rows();

    for (const auto& row : rows) {
        MK_REQUIRE(!row.public_native_handles);
        if (row.selected) {
            MK_REQUIRE(row.blocker.empty());
            continue;
        }
        MK_REQUIRE(!row.ready);
        MK_REQUIRE(!row.optional_dependency_selected);
        MK_REQUIRE(!row.blocker.empty());
        MK_REQUIRE(row.status == RuntimeUiPlatformAdapterGateStatus::host_gated ||
                   row.status == RuntimeUiPlatformAdapterGateStatus::dependency_gated);
        MK_REQUIRE(row.proof == RuntimeUiPlatformProductionProofKind::host_gate ||
                   row.proof == RuntimeUiPlatformProductionProofKind::dependency_gate);
    }

    const auto* harfbuzz = find_gate(rows, "runtime_ui.adapter.linux.harfbuzz_fontconfig");
    MK_REQUIRE(harfbuzz != nullptr);
    MK_REQUIRE(harfbuzz->status == RuntimeUiPlatformAdapterGateStatus::dependency_gated);
    MK_REQUIRE(harfbuzz->proof == RuntimeUiPlatformProductionProofKind::dependency_gate);
    MK_REQUIRE(!harfbuzz->optional_dependency_selected);
    MK_REQUIRE(has_feature(*harfbuzz, RuntimeUiPlatformProductionFeature::production_text_shaping));
    MK_REQUIRE(has_feature(*harfbuzz, RuntimeUiPlatformProductionFeature::real_font_loading));
}

MK_TEST("runtime ui platform adapter gate status names are stable") {
    MK_REQUIRE(mirakana::ui::runtime_ui_platform_adapter_gate_status_name(
                   RuntimeUiPlatformAdapterGateStatus::selected_proof) == "selected_proof");
    MK_REQUIRE(mirakana::ui::runtime_ui_platform_adapter_gate_status_name(
                   RuntimeUiPlatformAdapterGateStatus::host_gated) == "host_gated");
    MK_REQUIRE(mirakana::ui::runtime_ui_platform_adapter_gate_status_name(
                   RuntimeUiPlatformAdapterGateStatus::dependency_gated) == "dependency_gated");
}

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

MK_TEST("runtime ui platform production gate rejects dependency gates without blockers") {
    auto rows = make_complete_rows();
    rows[1].proof = RuntimeUiPlatformProductionProofKind::dependency_gate;
    rows[1].selected = false;
    rows[1].ready = false;
    rows[1].blocker.clear();

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::dependency_gate_missing));
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

MK_TEST("runtime ui platform production gate accepts selected Windows DirectWrite text shaping evidence") {
    auto rows = make_complete_rows();
    rows[1] = RuntimeUiPlatformProductionEvidenceRow{
        .id = "runtime-ui-platform.text-shaping.win32.directwrite",
        .feature = RuntimeUiPlatformProductionFeature::production_text_shaping,
        .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = true,
        .dependency_recorded = true,
        .host_evidence_available = true,
    };

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows);

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime ui platform production gate accepts selected Windows DirectWrite font loading and rasterization "
        "evidence") {
    auto rows = make_complete_rows();
    rows[2] = RuntimeUiPlatformProductionEvidenceRow{
        .id = "runtime-ui-platform.font-loading.win32.directwrite",
        .feature = RuntimeUiPlatformProductionFeature::real_font_loading,
        .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = true,
        .dependency_recorded = true,
        .host_evidence_available = true,
    };
    rows[3] = RuntimeUiPlatformProductionEvidenceRow{
        .id = "runtime-ui-platform.font-rasterization.win32.directwrite",
        .feature = RuntimeUiPlatformProductionFeature::font_rasterization,
        .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = true,
        .dependency_recorded = true,
        .host_evidence_available = true,
    };

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows);

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime ui platform production gate accepts selected Windows TSF native IME evidence") {
    auto rows = make_complete_rows();
    rows[4] = RuntimeUiPlatformProductionEvidenceRow{
        .id = "runtime-ui-platform.native-ime.win32.tsf",
        .feature = RuntimeUiPlatformProductionFeature::native_ime_session,
        .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = true,
        .dependency_recorded = true,
        .host_evidence_available = true,
    };

    const auto result = mirakana::ui::evaluate_runtime_ui_platform_production(rows);

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.diagnostics.empty());
}

int main() {
    return mirakana::test::run_all();
}
