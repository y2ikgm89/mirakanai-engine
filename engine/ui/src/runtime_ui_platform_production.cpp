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
        return true;
    case RuntimeUiPlatformProductionProofKind::first_party_contract:
    case RuntimeUiPlatformProductionProofKind::selected_package_counter:
    case RuntimeUiPlatformProductionProofKind::visible_editor_shell:
    case RuntimeUiPlatformProductionProofKind::host_gate:
    case RuntimeUiPlatformProductionProofKind::dependency_gate:
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
    if (row.proof == RuntimeUiPlatformProductionProofKind::dependency_gate && row.blocker.empty()) {
        append_diagnostic(result.diagnostics, RuntimeUiPlatformProductionDiagnosticCode::dependency_gate_missing,
                          row.id, "runtime UI platform production dependency-gated rows require an explicit blocker");
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
    }
    return "unknown";
}

std::vector<RuntimeUiPlatformAdapterGateRow> make_runtime_ui_platform_adapter_gate_rows() {
    using Feature = RuntimeUiPlatformProductionFeature;
    using Proof = RuntimeUiPlatformProductionProofKind;
    using Status = RuntimeUiPlatformAdapterGateStatus;

    return {
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.windows.directwrite",
            .features = {Feature::production_text_shaping, Feature::real_font_loading, Feature::font_rasterization},
            .proof = Proof::official_sdk_adapter,
            .status = Status::selected_proof,
            .selected = true,
            .ready = true,
            .dependency_recorded = true,
            .host_evidence_available = true,
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.windows.tsf",
            .features = {Feature::native_ime_session},
            .proof = Proof::official_sdk_adapter,
            .status = Status::selected_proof,
            .selected = true,
            .ready = true,
            .dependency_recorded = true,
            .host_evidence_available = true,
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.windows.uia",
            .features = {Feature::os_accessibility_publication},
            .proof = Proof::official_sdk_adapter,
            .status = Status::selected_proof,
            .selected = true,
            .ready = true,
            .dependency_recorded = true,
            .host_evidence_available = true,
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.upload.windows.d3d12",
            .features = {Feature::renderer_texture_upload_execution},
            .proof = Proof::selected_package_counter,
            .status = Status::selected_proof,
            .selected = true,
            .ready = true,
            .dependency_recorded = true,
            .host_evidence_available = true,
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.macos.core_text",
            .features = {Feature::production_text_shaping, Feature::real_font_loading, Feature::font_rasterization},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires macOS/Xcode Core Text and Core Graphics adapter implementation plus Apple-host "
                       "validation; Windows DirectWrite evidence cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.macos.input_method_kit",
            .features = {Feature::native_ime_session},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires macOS/Xcode InputMethodKit adapter implementation and Apple-host validation; "
                       "Windows TSF evidence cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.macos.nsaccessibility",
            .features = {Feature::os_accessibility_publication},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires macOS/Xcode NSAccessibility adapter implementation and Apple-host validation; "
                       "Windows UIA evidence cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.linux.harfbuzz_fontconfig",
            .features = {Feature::production_text_shaping, Feature::real_font_loading},
            .proof = Proof::dependency_gate,
            .status = Status::dependency_gated,
            .blocker = "Requires explicit runtime-ui-harfbuzz and runtime-ui-fontconfig dependency selection, "
                       "vcpkg bootstrap, legal notices, adapter implementation, and Linux host validation.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.linux.freetype",
            .features = {Feature::font_rasterization},
            .proof = Proof::dependency_gate,
            .status = Status::dependency_gated,
            .blocker = "Requires explicit runtime-ui-freetype dependency selection, vcpkg bootstrap, legal notices, "
                       "adapter implementation, and Linux host validation.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.linux.at_spi",
            .features = {Feature::os_accessibility_publication},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires Linux AT-SPI2 adapter implementation and Linux accessibility host validation; "
                       "Windows UIA evidence cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.android.text_input",
            .features = {Feature::native_ime_session},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires Android text-input adapter implementation and Android device or emulator host "
                       "validation; Windows TSF evidence cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.android.accessibility",
            .features = {Feature::os_accessibility_publication},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires Android accessibility adapter implementation and Android device or emulator host "
                       "validation; Windows UIA evidence cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.ios.uitextinput",
            .features = {Feature::native_ime_session},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires iOS UITextInput adapter implementation and Xcode simulator or device validation; "
                       "Windows TSF evidence cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.adapter.ios.uiaccessibility",
            .features = {Feature::os_accessibility_publication},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires iOS UIAccessibility adapter implementation and Xcode simulator or device validation; "
                       "Windows UIA evidence cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.upload.vulkan",
            .features = {Feature::renderer_texture_upload_execution},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker =
                "Requires Vulkan UI atlas upload/readback package proof with DXC SPIR-V, spirv-val, "
                "synchronization, validation-layer, and host runtime evidence; D3D12 proof cannot satisfy this gate.",
        },
        RuntimeUiPlatformAdapterGateRow{
            .id = "runtime_ui.upload.metal",
            .features = {Feature::renderer_texture_upload_execution},
            .proof = Proof::host_gate,
            .status = Status::host_gated,
            .blocker = "Requires Apple-host Metal UI atlas upload/readback proof with Xcode metal/metallib and "
                       "runtime validation; D3D12 or Vulkan proof cannot satisfy this gate.",
        },
    };
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
