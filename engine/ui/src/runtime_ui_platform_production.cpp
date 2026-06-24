// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/runtime_ui_platform_production.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

constexpr std::size_t kRequiredFeatureCount{9U};
constexpr std::array<RuntimeUiPlatformAdapterGateRow, 16U> kRuntimeUiPlatformAdapterGateRows{{
    {.id = "runtime_ui.adapter.windows.directwrite",
     .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
     .status = RuntimeUiPlatformAdapterGateStatus::selected_proof,
     .selected = true,
     .ready = true},
    {.id = "runtime_ui.adapter.windows.tsf",
     .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
     .status = RuntimeUiPlatformAdapterGateStatus::selected_proof,
     .selected = true,
     .ready = true},
    {.id = "runtime_ui.adapter.windows.uia",
     .proof = RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
     .status = RuntimeUiPlatformAdapterGateStatus::selected_proof,
     .selected = true,
     .ready = true},
    {.id = "runtime_ui.upload.windows.d3d12",
     .proof = RuntimeUiPlatformProductionProofKind::selected_package_counter,
     .status = RuntimeUiPlatformAdapterGateStatus::selected_proof,
     .selected = true,
     .ready = true},
    {.id = "runtime_ui.adapter.macos.core_text",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected macOS Core Text runtime UI text/font adapter evidence on an Apple host"},
    {.id = "runtime_ui.adapter.macos.input_method_kit",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected macOS InputMethodKit runtime UI IME session evidence on an Apple host"},
    {.id = "runtime_ui.adapter.macos.nsaccessibility",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected macOS NSAccessibility runtime UI publication evidence on an Apple host"},
    {.id = "runtime_ui.adapter.linux.harfbuzz_fontconfig",
     .proof = RuntimeUiPlatformProductionProofKind::dependency_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::dependency_gated,
     .blocker =
         "requires explicit non-default audited HarfBuzz/Fontconfig dependency selection, vcpkg feature, and legal "
         "notices"},
    {.id = "runtime_ui.adapter.linux.freetype",
     .proof = RuntimeUiPlatformProductionProofKind::dependency_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::dependency_gated,
     .blocker =
         "requires explicit non-default audited FreeType dependency selection, vcpkg feature, and legal notices"},
    {.id = "runtime_ui.adapter.linux.at_spi",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected Linux AT-SPI2 runtime UI accessibility publication evidence on a Linux host"},
    {.id = "runtime_ui.adapter.android.text_input",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected Android text input runtime UI adapter evidence on an Android host/device lane"},
    {.id = "runtime_ui.adapter.android.accessibility",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker =
         "requires selected Android accessibility runtime UI publication evidence on an Android host/device lane"},
    {.id = "runtime_ui.adapter.ios.uitextinput",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected iOS UITextInput runtime UI adapter evidence on an Apple iOS host lane"},
    {.id = "runtime_ui.adapter.ios.uiaccessibility",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected iOS UIAccessibility runtime UI publication evidence on an Apple iOS host lane"},
    {.id = "runtime_ui.upload.vulkan",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected Vulkan runtime UI atlas upload/readback execution evidence on a ready Vulkan host"},
    {.id = "runtime_ui.upload.metal",
     .proof = RuntimeUiPlatformProductionProofKind::host_gate,
     .status = RuntimeUiPlatformAdapterGateStatus::host_gated,
     .blocker = "requires selected Metal runtime UI atlas upload/readback execution evidence on an Apple host"},
}};

void append_diagnostic(std::vector<RuntimeUiPlatformProductionDiagnostic>& diagnostics,
                       RuntimeUiPlatformProductionDiagnosticCode code, std::string row_id, std::string message) {
    diagnostics.push_back(RuntimeUiPlatformProductionDiagnostic{
        .code = code,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) noexcept {
    return std::ranges::find(ids, id) != ids.end();
}

[[nodiscard]] bool is_valid_feature(RuntimeUiPlatformProductionFeature feature) noexcept {
    switch (feature) {
    case RuntimeUiPlatformProductionFeature::visible_ui_editor:
    case RuntimeUiPlatformProductionFeature::production_text_shaping:
    case RuntimeUiPlatformProductionFeature::real_font_loading:
    case RuntimeUiPlatformProductionFeature::font_rasterization:
    case RuntimeUiPlatformProductionFeature::native_ime_session:
    case RuntimeUiPlatformProductionFeature::os_accessibility_publication:
    case RuntimeUiPlatformProductionFeature::renderer_texture_upload_execution:
    case RuntimeUiPlatformProductionFeature::clean_room_provenance:
    case RuntimeUiPlatformProductionFeature::external_engine_parity_non_claim:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_valid_proof(RuntimeUiPlatformProductionProofKind proof) noexcept {
    switch (proof) {
    case RuntimeUiPlatformProductionProofKind::first_party_contract:
    case RuntimeUiPlatformProductionProofKind::official_sdk_adapter:
    case RuntimeUiPlatformProductionProofKind::audited_dependency_adapter:
    case RuntimeUiPlatformProductionProofKind::selected_package_counter:
    case RuntimeUiPlatformProductionProofKind::visible_editor_shell:
    case RuntimeUiPlatformProductionProofKind::host_gate:
    case RuntimeUiPlatformProductionProofKind::dependency_gate:
    case RuntimeUiPlatformProductionProofKind::unsupported_non_claim:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_dependency_proof(RuntimeUiPlatformProductionProofKind proof) noexcept {
    switch (proof) {
    case RuntimeUiPlatformProductionProofKind::official_sdk_adapter:
    case RuntimeUiPlatformProductionProofKind::audited_dependency_adapter:
    case RuntimeUiPlatformProductionProofKind::dependency_gate:
        return true;
    case RuntimeUiPlatformProductionProofKind::first_party_contract:
    case RuntimeUiPlatformProductionProofKind::selected_package_counter:
    case RuntimeUiPlatformProductionProofKind::visible_editor_shell:
    case RuntimeUiPlatformProductionProofKind::host_gate:
    case RuntimeUiPlatformProductionProofKind::unsupported_non_claim:
        return false;
    }
    return false;
}

[[nodiscard]] bool is_production_feature(RuntimeUiPlatformProductionFeature feature) noexcept {
    return feature != RuntimeUiPlatformProductionFeature::external_engine_parity_non_claim;
}

[[nodiscard]] bool is_selected_ready_production_row(const RuntimeUiPlatformProductionEvidenceRow& row) noexcept {
    return row.selected && row.ready && row.proof != RuntimeUiPlatformProductionProofKind::host_gate &&
           row.proof != RuntimeUiPlatformProductionProofKind::dependency_gate &&
           row.proof != RuntimeUiPlatformProductionProofKind::unsupported_non_claim;
}

void append_missing_feature_diagnostics(RuntimeUiPlatformProductionResult& result,
                                        const std::array<bool, kRequiredFeatureCount>& seen_features) {
    for (std::size_t index = 0U; index < seen_features.size(); ++index) {
        if (seen_features[index]) {
            continue;
        }
        const auto feature = static_cast<RuntimeUiPlatformProductionFeature>(index);
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::missing_feature_row, {},
                          "runtime UI platform production requires a row for " +
                              std::string{runtime_ui_platform_production_feature_name(feature)});
    }
}

void update_result_counters(RuntimeUiPlatformProductionResult& result,
                            const RuntimeUiPlatformProductionEvidenceRow& row) noexcept {
    if (row.selected) {
        ++result.selected_rows;
        if (row.ready) {
            ++result.ready_rows;
        }
    }
    if (row.proof == RuntimeUiPlatformProductionProofKind::host_gate) {
        ++result.host_gated_rows;
    }
    if (row.proof == RuntimeUiPlatformProductionProofKind::dependency_gate) {
        ++result.dependency_gated_rows;
    }
    if (row.proof == RuntimeUiPlatformProductionProofKind::unsupported_non_claim) {
        ++result.unsupported_non_claim_rows;
    }
    if (row.public_native_handles || row.external_engine_parity_claim || row.middleware_api_exposure ||
        row.copied_external_source || row.copied_external_asset) {
        ++result.unsupported_claim_rows;
    }
}

void validate_common_row(RuntimeUiPlatformProductionResult& result, const RuntimeUiPlatformProductionEvidenceRow& row) {
    if (row.public_native_handles) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::public_native_handles, row.id,
                          "runtime UI platform production evidence must not expose native, backend, GPU, OS, or RHI "
                          "handles in public contracts");
    }
    if (row.external_engine_parity_claim) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::external_engine_parity_claim,
                          row.id,
                          "runtime UI platform production evidence must not claim Unity, Unreal, Godot, visual, "
                          "workflow, or API parity");
    }
    if (row.middleware_api_exposure) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::middleware_api_exposure,
                          row.id, "runtime UI platform production evidence must not expose UI middleware APIs");
    }
    if (row.copied_external_source) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::copied_external_source, row.id,
                          "runtime UI platform production evidence must not copy external engine source");
    }
    if (row.copied_external_asset) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::copied_external_asset, row.id,
                          "runtime UI platform production evidence must not copy external engine assets");
    }
    if (row.selected && !row.ready) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::selected_row_not_ready, row.id,
                          "selected runtime UI platform production evidence rows must be ready");
    }
    if (is_dependency_proof(row.proof) && !row.dependency_recorded) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::dependency_not_recorded,
                          row.id,
                          "official SDK and audited dependency runtime UI platform production proof requires "
                          "dependency and license records");
    }
    if (row.proof == RuntimeUiPlatformProductionProofKind::host_gate && !row.host_evidence_available &&
        row.blocker.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::host_evidence_missing, row.id,
                          "runtime UI platform production host-gated rows require an explicit blocker");
    }
}

void validate_feature_row(RuntimeUiPlatformProductionResult& result,
                          const RuntimeUiPlatformProductionEvidenceRow& row) {
    if (row.feature == RuntimeUiPlatformProductionFeature::renderer_texture_upload_execution && row.selected &&
        row.ready && !row.renderer_upload_executed) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::renderer_upload_missing,
                          row.id,
                          "renderer texture upload execution evidence must include selected upload execution proof");
    }
    if (row.feature != RuntimeUiPlatformProductionFeature::external_engine_parity_non_claim) {
        return;
    }
    if (row.proof != RuntimeUiPlatformProductionProofKind::unsupported_non_claim) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::invalid_non_claim_proof,
                          row.id,
                          "external engine parity evidence must be an unsupported non-claim row, not production proof");
    }
    if (row.blocker.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::missing_non_claim_blocker,
                          row.id, "external engine parity non-claim rows require explicit non-claim text");
    }
}

} // namespace

bool RuntimeUiPlatformProductionResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

std::string_view runtime_ui_platform_production_feature_name(RuntimeUiPlatformProductionFeature feature) noexcept {
    switch (feature) {
    case RuntimeUiPlatformProductionFeature::visible_ui_editor:
        return "visible_ui_editor";
    case RuntimeUiPlatformProductionFeature::production_text_shaping:
        return "production_text_shaping";
    case RuntimeUiPlatformProductionFeature::real_font_loading:
        return "real_font_loading";
    case RuntimeUiPlatformProductionFeature::font_rasterization:
        return "font_rasterization";
    case RuntimeUiPlatformProductionFeature::native_ime_session:
        return "native_ime_session";
    case RuntimeUiPlatformProductionFeature::os_accessibility_publication:
        return "os_accessibility_publication";
    case RuntimeUiPlatformProductionFeature::renderer_texture_upload_execution:
        return "renderer_texture_upload_execution";
    case RuntimeUiPlatformProductionFeature::clean_room_provenance:
        return "clean_room_provenance";
    case RuntimeUiPlatformProductionFeature::external_engine_parity_non_claim:
        return "external_engine_parity_non_claim";
    }
    return "unknown";
}

std::string_view runtime_ui_platform_production_proof_name(RuntimeUiPlatformProductionProofKind proof) noexcept {
    switch (proof) {
    case RuntimeUiPlatformProductionProofKind::first_party_contract:
        return "first_party_contract";
    case RuntimeUiPlatformProductionProofKind::official_sdk_adapter:
        return "official_sdk_adapter";
    case RuntimeUiPlatformProductionProofKind::audited_dependency_adapter:
        return "audited_dependency_adapter";
    case RuntimeUiPlatformProductionProofKind::selected_package_counter:
        return "selected_package_counter";
    case RuntimeUiPlatformProductionProofKind::visible_editor_shell:
        return "visible_editor_shell";
    case RuntimeUiPlatformProductionProofKind::host_gate:
        return "host_gate";
    case RuntimeUiPlatformProductionProofKind::dependency_gate:
        return "dependency_gate";
    case RuntimeUiPlatformProductionProofKind::unsupported_non_claim:
        return "unsupported_non_claim";
    }
    return "unknown";
}

std::string_view runtime_ui_platform_adapter_gate_status_name(RuntimeUiPlatformAdapterGateStatus status) noexcept {
    switch (status) {
    case RuntimeUiPlatformAdapterGateStatus::selected_proof:
        return "selected_proof";
    case RuntimeUiPlatformAdapterGateStatus::host_gated:
        return "host_gated";
    case RuntimeUiPlatformAdapterGateStatus::dependency_gated:
        return "dependency_gated";
    case RuntimeUiPlatformAdapterGateStatus::unsupported:
        return "unsupported";
    }
    return "unknown";
}

std::span<const RuntimeUiPlatformAdapterGateRow> runtime_ui_platform_adapter_gate_rows() noexcept {
    return kRuntimeUiPlatformAdapterGateRows;
}

RuntimeUiPlatformProductionResult
evaluate_runtime_ui_platform_production(std::span<const RuntimeUiPlatformProductionEvidenceRow> rows,
                                        std::size_t row_budget) {
    RuntimeUiPlatformProductionResult result;
    result.rows.assign(rows.begin(), rows.end());

    if (rows.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::no_rows, {},
                          "runtime UI platform production requires evidence rows");
    }
    if (row_budget == 0U || rows.size() > row_budget) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::row_budget_overflow, {},
                          "runtime UI platform production evidence rows exceed the request budget");
    }

    std::vector<std::string> row_ids;
    row_ids.reserve(rows.size());
    std::array<bool, kRequiredFeatureCount> seen_features{};
    std::array<bool, kRequiredFeatureCount> selected_ready_features{};

    for (const auto& row : rows) {
        update_result_counters(result, row);

        if (row.id.empty()) {
            append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::missing_row_id, {},
                              "runtime UI platform production evidence row id must not be empty");
        } else if (contains_id(row_ids, row.id)) {
            append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::duplicate_row_id, row.id,
                              "runtime UI platform production evidence row ids must be unique");
        } else {
            row_ids.push_back(row.id);
        }

        if (!is_valid_feature(row.feature)) {
            append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::unsupported_feature,
                              row.id, "runtime UI platform production evidence row has an unsupported feature");
            continue;
        }
        if (!is_valid_proof(row.proof)) {
            append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::unsupported_proof_kind,
                              row.id, "runtime UI platform production evidence row has an unsupported proof kind");
            continue;
        }

        const auto feature_index = static_cast<std::size_t>(row.feature);
        seen_features[feature_index] = true;
        if (is_production_feature(row.feature) && is_selected_ready_production_row(row)) {
            selected_ready_features[feature_index] = true;
        }
        if (row.feature == RuntimeUiPlatformProductionFeature::external_engine_parity_non_claim &&
            row.proof == RuntimeUiPlatformProductionProofKind::unsupported_non_claim && !row.blocker.empty() &&
            !row.external_engine_parity_claim) {
            selected_ready_features[feature_index] = true;
        }

        validate_common_row(result, row);
        validate_feature_row(result, row);
    }

    append_missing_feature_diagnostics(result, seen_features);

    const auto all_required_features_satisfied =
        std::ranges::all_of(selected_ready_features, [](bool selected_ready) { return selected_ready; });
    result.ready = result.diagnostics.empty() && all_required_features_satisfied;
    return result;
}

} // namespace mirakana::ui
