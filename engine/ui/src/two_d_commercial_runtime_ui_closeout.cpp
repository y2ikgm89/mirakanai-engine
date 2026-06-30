// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/two_d_commercial_runtime_ui_closeout.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

constexpr std::array<std::string_view, 4U> kSelectedWindowsGateIds{
    "runtime_ui.adapter.windows.directwrite",
    "runtime_ui.adapter.windows.tsf",
    "runtime_ui.adapter.windows.uia",
    "runtime_ui.upload.windows.d3d12",
};

constexpr std::size_t kOfficialSourceKindCount{4U};

void append_diagnostic(std::vector<TwoDCommercialRuntimeUiCloseoutDiagnostic>& diagnostics,
                       TwoDCommercialRuntimeUiCloseoutDiagnosticCode code, std::string row_id, std::string message) {
    diagnostics.push_back(TwoDCommercialRuntimeUiCloseoutDiagnostic{
        .code = code,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool is_selected_windows_gate(std::string_view id) noexcept {
    return std::ranges::find(kSelectedWindowsGateIds, id) != kSelectedWindowsGateIds.end();
}

[[nodiscard]] std::span<const RuntimeUiPlatformAdapterGateRow>
resolve_adapter_gate_rows(const TwoDCommercialRuntimeUiCloseoutDesc& desc) noexcept {
    if (!desc.adapter_gate_rows.empty()) {
        return desc.adapter_gate_rows;
    }
    return runtime_ui_platform_adapter_gate_rows();
}

void evaluate_platform_inputs(const TwoDCommercialRuntimeUiCloseoutDesc& desc,
                              TwoDCommercialRuntimeUiCloseoutResult& result) {
    result.platform_ready = desc.platform_result.ready && desc.platform_result.diagnostics.empty();
    if (!result.platform_ready) {
        append_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::platform_not_ready, {},
                          "2D commercial runtime UI closeout requires the selected runtime UI platform production "
                          "gate to be ready");
    }

    result.package_smoke_ready = desc.package_smoke_review.ready && desc.package_smoke_review.diagnostics.empty();
    if (!result.package_smoke_ready) {
        append_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::package_smoke_not_ready,
                          {},
                          "2D commercial runtime UI closeout requires package-visible multilingual, long-label, "
                          "controller-only, and accessibility smoke scenes");
    }
}

void evaluate_official_sources(const TwoDCommercialRuntimeUiCloseoutDesc& desc,
                               TwoDCommercialRuntimeUiCloseoutResult& result) {
    result.official_source_rows = desc.official_source_rows.size();
    std::array<bool, kOfficialSourceKindCount> seen_kinds{};
    result.official_source_ready = true;

    for (const auto& row : desc.official_source_rows) {
        const auto index = static_cast<std::size_t>(row.kind);
        if (index < seen_kinds.size()) {
            seen_kinds[index] = true;
        }
        if (row.id.empty() || !row.ready || !row.official || !row.public_docs_only) {
            result.official_source_ready = false;
            append_diagnostic(result.diagnostics,
                              TwoDCommercialRuntimeUiCloseoutDiagnosticCode::official_source_not_ready, row.id,
                              "2D commercial runtime UI closeout official source rows must be ready, official, and "
                              "public-docs-only");
        }
    }

    for (std::size_t index = 0U; index < seen_kinds.size(); ++index) {
        if (seen_kinds[index]) {
            continue;
        }
        result.official_source_ready = false;
        const auto kind = static_cast<TwoDCommercialRuntimeUiOfficialSourceKind>(index);
        append_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::official_source_not_ready,
                          {},
                          "2D commercial runtime UI closeout is missing official source row " +
                              std::string{two_d_commercial_runtime_ui_official_source_kind_name(kind)});
    }
}

void evaluate_adapter_gates(const TwoDCommercialRuntimeUiCloseoutDesc& desc,
                            TwoDCommercialRuntimeUiCloseoutResult& result) {
    const auto rows = resolve_adapter_gate_rows(desc);
    std::array<bool, kSelectedWindowsGateIds.size()> selected_seen{};

    for (const auto& row : rows) {
        if (row.status == RuntimeUiPlatformAdapterGateStatus::host_gated) {
            ++result.host_gated_rows;
        }
        if (row.status == RuntimeUiPlatformAdapterGateStatus::dependency_gated) {
            ++result.dependency_gated_rows;
        }

        const auto selected_index = std::ranges::find(kSelectedWindowsGateIds, row.id);
        const auto is_windows_selected = selected_index != kSelectedWindowsGateIds.end();
        if (is_windows_selected) {
            const auto index = static_cast<std::size_t>(selected_index - kSelectedWindowsGateIds.begin());
            selected_seen[index] = row.status == RuntimeUiPlatformAdapterGateStatus::selected_proof && row.selected &&
                                   row.ready && row.blocker.empty();
            if (selected_seen[index]) {
                ++result.selected_windows_ready_rows;
            } else {
                append_diagnostic(result.diagnostics,
                                  TwoDCommercialRuntimeUiCloseoutDiagnosticCode::selected_adapter_gate_drift,
                                  std::string{row.id},
                                  "selected Windows DirectWrite, TSF, UIA, and D3D12 adapter gates must stay ready "
                                  "and blocker-free");
            }
            continue;
        }

        if (row.selected || row.ready || row.status == RuntimeUiPlatformAdapterGateStatus::selected_proof) {
            ++result.non_windows_promoted_rows;
            append_diagnostic(result.diagnostics,
                              TwoDCommercialRuntimeUiCloseoutDiagnosticCode::non_windows_adapter_promoted,
                              std::string{row.id},
                              "2D commercial runtime UI closeout must not promote Linux, macOS, iOS, Android, "
                              "Vulkan, or Metal UI readiness from the Windows proof");
        }
        if (row.blocker.empty()) {
            append_diagnostic(result.diagnostics,
                              TwoDCommercialRuntimeUiCloseoutDiagnosticCode::selected_adapter_gate_drift,
                              std::string{row.id},
                              "non-selected runtime UI adapter gates must retain explicit host or dependency blockers");
        }
    }

    for (std::size_t index = 0U; index < selected_seen.size(); ++index) {
        if (!selected_seen[index]) {
            append_diagnostic(result.diagnostics,
                              TwoDCommercialRuntimeUiCloseoutDiagnosticCode::selected_adapter_gate_drift,
                              std::string{kSelectedWindowsGateIds[index]},
                              "2D commercial runtime UI closeout is missing a selected Windows adapter gate");
        }
    }
}

void evaluate_unsafe_claims(const TwoDCommercialRuntimeUiCloseoutDesc& desc,
                            TwoDCommercialRuntimeUiCloseoutResult& result) {
    if (!desc.selected_windows_d3d12_ready_claim) {
        append_diagnostic(result.diagnostics,
                          TwoDCommercialRuntimeUiCloseoutDiagnosticCode::selected_windows_claim_missing, {},
                          "2D commercial runtime UI closeout must explicitly scope the ready claim to selected "
                          "Windows/D3D12 proof");
    }
    if (desc.linux_ready_claim || desc.macos_ready_claim || desc.ios_ready_claim || desc.android_ready_claim ||
        desc.vulkan_ready_claim || desc.metal_ready_claim) {
        append_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::non_windows_ready_claim,
                          {},
                          "2D commercial runtime UI closeout must keep non-Windows, Vulkan, and Metal UI readiness "
                          "host-gated or unclaimed");
    }
    if (desc.public_native_handles) {
        append_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::public_native_handles, {},
                          "2D commercial runtime UI closeout must not expose Win32, D3D12, TSF, UIA, RHI, or native "
                          "handles in public game/runtime contracts");
    }
    if (desc.ui_middleware_claim) {
        append_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::ui_middleware_claim, {},
                          "2D commercial runtime UI closeout must not vendor or claim UI middleware APIs");
    }
    if (desc.external_engine_compatibility_claim) {
        append_diagnostic(result.diagnostics,
                          TwoDCommercialRuntimeUiCloseoutDiagnosticCode::external_engine_compatibility_claim, {},
                          "2D commercial runtime UI closeout must not claim Unity, Unreal, Godot, or third-party "
                          "engine compatibility");
    }
    if (desc.cross_platform_parity_claim) {
        append_diagnostic(result.diagnostics,
                          TwoDCommercialRuntimeUiCloseoutDiagnosticCode::cross_platform_parity_claim, {},
                          "2D commercial runtime UI closeout must not claim cross-platform UI parity from selected "
                          "Windows proof");
    }
    if (desc.legal_approval_claim) {
        append_diagnostic(result.diagnostics, TwoDCommercialRuntimeUiCloseoutDiagnosticCode::legal_approval_claim, {},
                          "2D commercial runtime UI closeout can provide engineering review input but not legal "
                          "approval");
    }
}

} // namespace

bool TwoDCommercialRuntimeUiCloseoutResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

std::string_view
two_d_commercial_runtime_ui_official_source_kind_name(TwoDCommercialRuntimeUiOfficialSourceKind kind) noexcept {
    switch (kind) {
    case TwoDCommercialRuntimeUiOfficialSourceKind::context7_directwrite:
        return "context7_directwrite";
    case TwoDCommercialRuntimeUiOfficialSourceKind::context7_win32_tsf_uia:
        return "context7_win32_tsf_uia";
    case TwoDCommercialRuntimeUiOfficialSourceKind::context7_direct3d12:
        return "context7_direct3d12";
    case TwoDCommercialRuntimeUiOfficialSourceKind::repository_legal_policy:
        return "repository_legal_policy";
    }
    return "unknown";
}

TwoDCommercialRuntimeUiCloseoutResult
evaluate_2d_commercial_runtime_ui_closeout(const TwoDCommercialRuntimeUiCloseoutDesc& desc) {
    TwoDCommercialRuntimeUiCloseoutResult result;
    evaluate_platform_inputs(desc, result);
    evaluate_official_sources(desc, result);
    evaluate_adapter_gates(desc, result);
    evaluate_unsafe_claims(desc, result);
    result.ready = result.platform_ready && result.package_smoke_ready && result.official_source_ready &&
                   desc.selected_windows_d3d12_ready_claim && result.selected_windows_ready_rows == 4U &&
                   result.non_windows_promoted_rows == 0U && result.diagnostics.empty();
    return result;
}

} // namespace mirakana::ui
