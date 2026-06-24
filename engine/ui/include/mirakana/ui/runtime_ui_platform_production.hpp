// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::ui {

enum class RuntimeUiPlatformProductionFeature : std::uint8_t {
    visible_ui_editor,
    production_text_shaping,
    real_font_loading,
    font_rasterization,
    native_ime_session,
    os_accessibility_publication,
    renderer_texture_upload_execution,
    clean_room_provenance,
    external_engine_parity_non_claim,
};

enum class RuntimeUiPlatformProductionProofKind : std::uint8_t {
    first_party_contract,
    official_sdk_adapter,
    audited_dependency_adapter,
    selected_package_counter,
    visible_editor_shell,
    host_gate,
    dependency_gate,
    unsupported_non_claim,
};

enum class RuntimeUiPlatformProductionDiagnosticCode : std::uint8_t {
    no_rows,
    missing_row_id,
    duplicate_row_id,
    missing_feature_row,
    unsupported_feature,
    unsupported_proof_kind,
    public_native_handles,
    dependency_not_recorded,
    host_evidence_missing,
    dependency_gate_missing,
    renderer_upload_missing,
    external_engine_parity_claim,
    middleware_api_exposure,
    copied_external_source,
    copied_external_asset,
    row_budget_overflow,
    selected_row_not_ready,
    missing_non_claim_blocker,
    invalid_non_claim_proof,
};

enum class RuntimeUiPlatformAdapterGateStatus : std::uint8_t {
    selected_proof,
    host_gated,
    dependency_gated,
    unsupported,
};

struct RuntimeUiPlatformProductionEvidenceRow {
    std::string id;
    RuntimeUiPlatformProductionFeature feature{RuntimeUiPlatformProductionFeature::visible_ui_editor};
    RuntimeUiPlatformProductionProofKind proof{RuntimeUiPlatformProductionProofKind::first_party_contract};
    bool selected{false};
    bool ready{false};
    bool dependency_recorded{false};
    bool host_evidence_available{false};
    bool renderer_upload_executed{false};
    bool public_native_handles{false};
    bool external_engine_parity_claim{false};
    bool middleware_api_exposure{false};
    bool copied_external_source{false};
    bool copied_external_asset{false};
    std::string blocker;
};

struct RuntimeUiPlatformProductionDiagnostic {
    RuntimeUiPlatformProductionDiagnosticCode code{RuntimeUiPlatformProductionDiagnosticCode::no_rows};
    std::string row_id;
    std::string message;
};

struct RuntimeUiPlatformProductionResult {
    bool ready{false};
    std::vector<RuntimeUiPlatformProductionEvidenceRow> rows;
    std::vector<RuntimeUiPlatformProductionDiagnostic> diagnostics;
    std::size_t selected_rows{0U};
    std::size_t ready_rows{0U};
    std::size_t host_gated_rows{0U};
    std::size_t dependency_gated_rows{0U};
    std::size_t unsupported_non_claim_rows{0U};
    std::size_t unsupported_claim_rows{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeUiPlatformAdapterGateRow {
    std::string_view id;
    RuntimeUiPlatformProductionProofKind proof{RuntimeUiPlatformProductionProofKind::host_gate};
    RuntimeUiPlatformAdapterGateStatus status{RuntimeUiPlatformAdapterGateStatus::host_gated};
    bool selected{false};
    bool ready{false};
    std::string_view blocker;
};

[[nodiscard]] std::string_view
runtime_ui_platform_production_feature_name(RuntimeUiPlatformProductionFeature feature) noexcept;
[[nodiscard]] std::string_view
runtime_ui_platform_production_proof_name(RuntimeUiPlatformProductionProofKind proof) noexcept;
[[nodiscard]] std::string_view
runtime_ui_platform_adapter_gate_status_name(RuntimeUiPlatformAdapterGateStatus status) noexcept;
[[nodiscard]] std::span<const RuntimeUiPlatformAdapterGateRow> runtime_ui_platform_adapter_gate_rows() noexcept;
[[nodiscard]] RuntimeUiPlatformProductionResult
evaluate_runtime_ui_platform_production(std::span<const RuntimeUiPlatformProductionEvidenceRow> rows,
                                        std::size_t row_budget = 64U);

} // namespace mirakana::ui
