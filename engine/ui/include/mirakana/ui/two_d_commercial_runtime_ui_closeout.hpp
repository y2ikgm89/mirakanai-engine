// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/runtime_ui_package_smoke_scene.hpp"
#include "mirakana/ui/runtime_ui_platform_production.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::ui {

enum class TwoDCommercialRuntimeUiOfficialSourceKind : std::uint8_t {
    context7_directwrite,
    context7_win32_tsf_uia,
    context7_direct3d12,
    repository_legal_policy,
};

enum class TwoDCommercialRuntimeUiCloseoutDiagnosticCode : std::uint8_t {
    platform_not_ready,
    package_smoke_not_ready,
    official_source_not_ready,
    selected_windows_claim_missing,
    selected_adapter_gate_drift,
    non_windows_adapter_promoted,
    non_windows_ready_claim,
    public_native_handles,
    ui_middleware_claim,
    external_engine_compatibility_claim,
    cross_platform_parity_claim,
    legal_approval_claim,
};

struct TwoDCommercialRuntimeUiOfficialSourceRow {
    std::string id;
    TwoDCommercialRuntimeUiOfficialSourceKind kind{TwoDCommercialRuntimeUiOfficialSourceKind::context7_directwrite};
    bool ready{false};
    bool official{false};
    bool public_docs_only{false};
};

struct TwoDCommercialRuntimeUiCloseoutDiagnostic {
    TwoDCommercialRuntimeUiCloseoutDiagnosticCode code{
        TwoDCommercialRuntimeUiCloseoutDiagnosticCode::platform_not_ready};
    std::string row_id;
    std::string message;
};

struct TwoDCommercialRuntimeUiCloseoutDesc {
    RuntimeUiPlatformProductionResult platform_result;
    RuntimeUiPackageSmokeSceneReview package_smoke_review;
    std::array<TwoDCommercialRuntimeUiOfficialSourceRow, 4U> official_source_rows{};
    std::vector<RuntimeUiPlatformAdapterGateRow> adapter_gate_rows;
    bool selected_windows_d3d12_ready_claim{false};
    bool linux_ready_claim{false};
    bool macos_ready_claim{false};
    bool ios_ready_claim{false};
    bool android_ready_claim{false};
    bool vulkan_ready_claim{false};
    bool metal_ready_claim{false};
    bool public_native_handles{false};
    bool ui_middleware_claim{false};
    bool external_engine_compatibility_claim{false};
    bool cross_platform_parity_claim{false};
    bool legal_approval_claim{false};
};

struct TwoDCommercialRuntimeUiCloseoutResult {
    bool ready{false};
    bool platform_ready{false};
    bool package_smoke_ready{false};
    bool official_source_ready{false};
    std::vector<TwoDCommercialRuntimeUiCloseoutDiagnostic> diagnostics;
    std::size_t official_source_rows{0U};
    std::size_t selected_windows_ready_rows{0U};
    std::size_t host_gated_rows{0U};
    std::size_t dependency_gated_rows{0U};
    std::size_t non_windows_promoted_rows{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] std::string_view
two_d_commercial_runtime_ui_official_source_kind_name(TwoDCommercialRuntimeUiOfficialSourceKind kind) noexcept;
[[nodiscard]] TwoDCommercialRuntimeUiCloseoutResult
evaluate_2d_commercial_runtime_ui_closeout(const TwoDCommercialRuntimeUiCloseoutDesc& desc);

} // namespace mirakana::ui
