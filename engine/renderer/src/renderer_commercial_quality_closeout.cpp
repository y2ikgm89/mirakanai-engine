// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/renderer_commercial_quality_closeout.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::string_view kCommercialQualityRecipeId{"renderer-commercial-quality-closeout"};
constexpr std::string_view kMetalMemoryProfilingRecipeId{"renderer-metal-memory-profiling-host-evidence"};
constexpr std::string_view kAppleMetalEnvironmentRecipeId{"renderer-metal-apple-host-evidence"};

constexpr std::array kRequiredPackageKinds{
    RendererCommercialQualityEvidenceKind::visible_3d_package,
    RendererCommercialQualityEvidenceKind::runtime_ui_package,
    RendererCommercialQualityEvidenceKind::environment_package,
    RendererCommercialQualityEvidenceKind::generated_game_package,
};

[[nodiscard]] std::uint8_t backend_sort_key(rhi::BackendKind backend) noexcept {
    return static_cast<std::uint8_t>(backend);
}

[[nodiscard]] std::uint8_t evidence_kind_sort_key(RendererCommercialQualityEvidenceKind kind) noexcept {
    return static_cast<std::uint8_t>(kind);
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9')) || ch == '_';
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_forbidden_native_token(std::string_view token) {
    return token == "native" || token == "handle" || token == "hwnd" || token == "hinstance" ||
           token.starts_with("id3d12") || token.starts_with("vk") || token.starts_with("mtl") ||
           token.starts_with("sdl") || token == "imgui";
}

[[nodiscard]] bool has_native_token(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (is_forbidden_native_token(token)) {
            return true;
        }
        token.clear();
    }
    return is_forbidden_native_token(token);
}

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool is_reviewed_recipe_id(std::string_view recipe_id) noexcept {
    constexpr std::array kReviewedRecipes{
        kCommercialQualityRecipeId,           kMetalMemoryProfilingRecipeId,
        kAppleMetalEnvironmentRecipeId,       std::string_view{"shader-toolchain"},
        std::string_view{"mobile-packaging"}, std::string_view{"ios-simulator-smoke"},
    };
    return std::ranges::find(kReviewedRecipes, recipe_id) != kReviewedRecipes.end();
}

[[nodiscard]] bool kind_uses_required_package_row(RendererCommercialQualityEvidenceKind kind) noexcept {
    return std::ranges::find(kRequiredPackageKinds, kind) != kRequiredPackageKinds.end();
}

[[nodiscard]] bool row_status_ready(const RendererCommercialQualityEvidenceRow& row) noexcept {
    return row.status == RendererCommercialQualityEvidenceStatus::ready && row.ready && !row.host_gated &&
           !row.dependency_gated;
}

[[nodiscard]] bool row_status_host_gated(const RendererCommercialQualityEvidenceRow& row) noexcept {
    return row.status == RendererCommercialQualityEvidenceStatus::host_gated && !row.ready && row.host_gated &&
           !row.dependency_gated;
}

[[nodiscard]] bool row_status_dependency_gated(const RendererCommercialQualityEvidenceRow& row) noexcept {
    return row.status == RendererCommercialQualityEvidenceStatus::dependency_gated && !row.ready && !row.host_gated &&
           row.dependency_gated;
}

[[nodiscard]] bool row_status_unsupported(const RendererCommercialQualityEvidenceRow& row) noexcept {
    return row.status == RendererCommercialQualityEvidenceStatus::unsupported && !row.ready && !row.host_gated &&
           !row.dependency_gated;
}

[[nodiscard]] bool row_ready_for_kind(const RendererCommercialQualityEvidenceRow& row,
                                      RendererCommercialQualityEvidenceKind kind) noexcept {
    return row.kind == kind && row_status_ready(row) && row.reviewed;
}

[[nodiscard]] bool row_has_reviewed_recipe(const RendererCommercialQualityEvidenceRow& row) noexcept {
    return !row.host_validation_recipe_id.empty() && is_valid_id(row.host_validation_recipe_id) &&
           !has_native_token(row.host_validation_recipe_id) && is_reviewed_recipe_id(row.host_validation_recipe_id);
}

[[nodiscard]] bool row_has_valid_counter(const RendererCommercialQualityEvidenceRow& row) noexcept {
    return !row.package_counter_id.empty() && is_valid_id(row.package_counter_id) &&
           !has_native_token(row.package_counter_id);
}

[[nodiscard]] bool row_satisfies_required_kind(const RendererCommercialQualityEvidenceRow& row,
                                               RendererCommercialQualityEvidenceKind kind) noexcept {
    if (!row_ready_for_kind(row, kind) || !row_has_reviewed_recipe(row) || !row_has_valid_counter(row)) {
        return false;
    }
    if (kind == RendererCommercialQualityEvidenceKind::metal_memory_profiling) {
        return row.backend == rhi::BackendKind::metal && row.host_validation_recipe_id == kMetalMemoryProfilingRecipeId;
    }
    return true;
}

void add_diagnostic(RendererCommercialQualityCloseoutPlan& plan, RendererCommercialQualityCloseoutDiagnosticCode code,
                    RendererCommercialQualityEvidenceKind kind, rhi::BackendKind backend, std::string row_id,
                    std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RendererCommercialQualityCloseoutDiagnostic{
        .code = code,
        .kind = kind,
        .backend = backend,
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_rows(std::vector<RendererCommercialQualityEvidenceRow>& rows) {
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.kind != rhs.kind) {
            return evidence_kind_sort_key(lhs.kind) < evidence_kind_sort_key(rhs.kind);
        }
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(RendererCommercialQualityCloseoutPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.kind != rhs.kind) {
            return evidence_kind_sort_key(lhs.kind) < evidence_kind_sort_key(rhs.kind);
        }
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] std::string duplicate_key(const RendererCommercialQualityEvidenceRow& row) {
    std::string key;
    key.append(std::to_string(static_cast<std::uint8_t>(row.kind)));
    key.push_back('\n');
    key.append(std::to_string(static_cast<std::uint8_t>(row.backend)));
    key.push_back('\n');
    key.append(row.row_id);
    return key;
}

void validate_duplicate_rows(RendererCommercialQualityCloseoutPlan& plan,
                             const RendererCommercialQualityCloseoutDesc& desc) {
    std::vector<std::string> seen;
    seen.reserve(desc.evidence_rows.size());
    for (const auto& row : desc.evidence_rows) {
        auto key = duplicate_key(row);
        if (std::ranges::find(seen, key) != seen.end()) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::duplicate_evidence_row, row.kind,
                           row.backend, row.row_id,
                           "renderer commercial quality evidence rows must use stable unique row ids",
                           row.source_index);
            continue;
        }
        seen.push_back(std::move(key));
    }
}

void validate_row_shape(RendererCommercialQualityCloseoutPlan& plan,
                        const RendererCommercialQualityCloseoutDesc& desc) {
    for (const auto& row : desc.evidence_rows) {
        if (!is_valid_id(row.row_id) || has_native_token(row.row_id) || !row.reviewed ||
            (!row.package_counter_id.empty() &&
             (!is_valid_id(row.package_counter_id) || has_native_token(row.package_counter_id)))) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::invalid_evidence_row, row.kind,
                           row.backend, row.row_id,
                           "renderer commercial quality rows require reviewed backend-neutral ids and counters",
                           row.source_index);
        }

        if (!row_status_ready(row) && !row_status_host_gated(row) && !row_status_dependency_gated(row) &&
            !row_status_unsupported(row)) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::invalid_evidence_row, row.kind,
                           row.backend, row.row_id,
                           "renderer commercial quality rows require one explicit status taxonomy", row.source_index);
        }

        if (row.status == RendererCommercialQualityEvidenceStatus::ready && !row_has_valid_counter(row)) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::invalid_evidence_row, row.kind,
                           row.backend, row.row_id, "ready evidence rows require a package-visible counter id",
                           row.source_index);
        }

        if (row.status == RendererCommercialQualityEvidenceStatus::ready && !row_has_reviewed_recipe(row)) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::unreviewed_host_validation_recipe,
                           row.kind, row.backend, row.row_id,
                           "ready evidence rows require a reviewed host validation recipe id", row.source_index);
        }

        if (row.kind == RendererCommercialQualityEvidenceKind::metal_memory_profiling &&
            row.backend != rhi::BackendKind::metal) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::cross_backend_inference, row.kind,
                           row.backend, row.row_id,
                           "Metal memory/profiling evidence must be produced by Apple-host Metal evidence",
                           row.source_index);
        }
    }
}

void validate_safety_claims(RendererCommercialQualityCloseoutPlan& plan,
                            const RendererCommercialQualityCloseoutDesc& desc) {
    for (const auto& row : desc.evidence_rows) {
        if (row.request_native_handle_access) {
            ++plan.native_handle_access_count;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::unsupported_native_handle_claim,
                           row.kind, row.backend, row.row_id,
                           "renderer commercial quality evidence must not expose native renderer or RHI handles",
                           row.source_index);
        }
        if (row.request_cross_backend_inference) {
            ++plan.cross_backend_inference_count;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::cross_backend_inference, row.kind,
                           row.backend, row.row_id,
                           "renderer commercial quality evidence cannot transfer proof across backends",
                           row.source_index);
        }
        if (row.request_external_engine_parity) {
            ++plan.external_engine_parity_count;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_parity_claim,
                           row.kind, row.backend, row.row_id,
                           "external-engine parity claims require a separate legal and technical plan",
                           row.source_index);
        }
        if (row.request_subjective_quality_claim) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::unsupported_subjective_quality_claim,
                           row.kind, row.backend, row.row_id,
                           "commercial renderer quality must be supported by selected evidence rows, not opinion",
                           row.source_index);
        }
        if (row.request_crash_upload) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::unsupported_crash_upload, row.kind,
                           row.backend, row.row_id,
                           "automatic crash upload execution remains outside this value-only closeout",
                           row.source_index);
        }
    }
}

void validate_clean_room(RendererCommercialQualityCloseoutPlan& plan,
                         const RendererCommercialQualityCloseoutDesc& desc) {
    for (const auto& row : desc.evidence_rows) {
        if (row.external_engine_code_used) {
            ++plan.external_engine_code_used_count;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_code_used, row.kind,
                           row.backend, row.row_id, "external engine source code is not accepted as renderer evidence",
                           row.source_index);
        }
        if (row.external_engine_sample_used) {
            ++plan.external_engine_sample_used_count;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_sample_used, row.kind,
                           row.backend, row.row_id, "external engine sample code is not accepted as renderer evidence",
                           row.source_index);
        }
        if (row.external_engine_asset_used) {
            ++plan.external_engine_asset_used_count;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_asset_used, row.kind,
                           row.backend, row.row_id, "external engine assets require a separate license audit",
                           row.source_index);
        }
        if (row.external_engine_trademark_used) {
            ++plan.external_engine_trademark_used_count;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_trademark_used,
                           row.kind, row.backend, row.row_id,
                           "external engine marks must not appear in product renderer readiness evidence",
                           row.source_index);
        }
        if (row.external_engine_compatibility_claim) {
            ++plan.external_engine_compatibility_claim_count;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::external_engine_compatibility_claim,
                           row.kind, row.backend, row.row_id,
                           "external-engine compatibility claims require separate legal approval", row.source_index);
        }
        if (row.approved_external_material && !row.third_party_notices_complete) {
            plan.third_party_notices_complete = false;
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_third_party_notices, row.kind,
                           row.backend, row.row_id,
                           "approved external material requires complete third-party notices before readiness",
                           row.source_index);
        }
    }
}

void validate_budget(RendererCommercialQualityCloseoutPlan& plan, const RendererCommercialQualityCloseoutDesc& desc) {
    if (desc.evidence_rows.size() > desc.row_budget) {
        add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::row_budget_exceeded,
                       RendererCommercialQualityEvidenceKind::backend_parity, rhi::BackendKind::null, {},
                       "renderer commercial quality closeout evidence rows exceed the aggregate row budget", 0U);
    }
}

[[nodiscard]] bool backend_parity_ready(const BackendRendererParityPolicyPlan& plan) noexcept {
    return plan.status == BackendRendererParityPolicyStatus::ready && plan.succeeded() && plan.d3d12_parity_ready &&
           plan.vulkan_parity_ready && plan.metal_parity_ready;
}

[[nodiscard]] bool quality_matrix_ready(const RendererQualityMatrixPlan& plan) noexcept {
    return plan.status == RendererQualityMatrixStatus::ready && plan.succeeded() && plan.d3d12_quality_matrix_ready &&
           plan.vulkan_strict_quality_matrix_ready && plan.metal_quality_matrix_ready &&
           plan.general_renderer_quality_ready && !plan.invoked_native_capture && !plan.invoked_crash_upload;
}

[[nodiscard]] bool vfx_profiling_ready(const RendererProductionVfxProfilingPlan& plan) noexcept {
    return plan.status == RendererProductionVfxProfilingStatus::ready && plan.succeeded() &&
           plan.d3d12_host_evidence_ready && plan.vulkan_strict_host_evidence_ready && plan.metal_host_evidence_ready &&
           !plan.invoked_native_capture && !plan.invoked_crash_upload;
}

[[nodiscard]] bool has_ready_kind(const std::vector<RendererCommercialQualityEvidenceRow>& rows,
                                  RendererCommercialQualityEvidenceKind kind) noexcept {
    return std::ranges::any_of(rows, [kind](const auto& row) { return row_satisfies_required_kind(row, kind); });
}

void append_rows(RendererCommercialQualityCloseoutPlan& plan, const RendererCommercialQualityCloseoutDesc& desc) {
    plan.evidence_rows = desc.evidence_rows;
    sort_rows(plan.evidence_rows);
    plan.row_count = plan.evidence_rows.size();
}

void summarize_rows(RendererCommercialQualityCloseoutPlan& plan) {
    for (const auto& row : plan.evidence_rows) {
        if (row_status_ready(row)) {
            ++plan.ready_row_count;
        } else if (row_status_host_gated(row)) {
            ++plan.host_gated_row_count;
        } else if (row_status_dependency_gated(row)) {
            ++plan.dependency_gated_row_count;
        } else if (row_status_unsupported(row)) {
            ++plan.unsupported_row_count;
        }
    }
}

void compute_required_evidence(RendererCommercialQualityCloseoutPlan& plan,
                               const RendererCommercialQualityCloseoutDesc& desc) {
    plan.renderer_backend_parity_ready = backend_parity_ready(desc.backend_parity);
    if (!plan.renderer_backend_parity_ready) {
        add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_backend_parity,
                       RendererCommercialQualityEvidenceKind::backend_parity, rhi::BackendKind::null, {},
                       "renderer backend parity requires ready D3D12, strict Vulkan, and Apple-host Metal flags", 0U);
    }

    plan.d3d12_renderer_quality_ready = desc.quality_matrix.d3d12_quality_matrix_ready;
    plan.vulkan_strict_renderer_quality_ready = desc.quality_matrix.vulkan_strict_quality_matrix_ready;
    plan.metal_renderer_quality_ready = desc.quality_matrix.metal_quality_matrix_ready;
    if (!quality_matrix_ready(desc.quality_matrix)) {
        add_diagnostic(
            plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_quality_matrix,
            RendererCommercialQualityEvidenceKind::renderer_quality_matrix, rhi::BackendKind::null, {},
            "renderer quality matrix requires ready D3D12, strict Vulkan, Apple-host Metal, and package rows", 0U);
    }

    plan.production_vfx_profiling_ready = vfx_profiling_ready(desc.vfx_profiling);
    if (!plan.production_vfx_profiling_ready) {
        add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_production_vfx_profiling,
                       RendererCommercialQualityEvidenceKind::production_vfx_profiling, rhi::BackendKind::null, {},
                       "production VFX/profiling requires ready D3D12, strict Vulkan, and Apple-host Metal evidence",
                       0U);
    }

    plan.metal_memory_profiling_ready =
        has_ready_kind(plan.evidence_rows, RendererCommercialQualityEvidenceKind::metal_memory_profiling);
    if (!plan.metal_memory_profiling_ready) {
        add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_metal_memory_profiling_evidence,
                       RendererCommercialQualityEvidenceKind::metal_memory_profiling, rhi::BackendKind::metal, {},
                       "Metal broad readiness requires dedicated memory/profiling host evidence", 0U);
    }

    plan.visible_3d_package_ready =
        has_ready_kind(plan.evidence_rows, RendererCommercialQualityEvidenceKind::visible_3d_package);
    plan.runtime_ui_package_ready =
        has_ready_kind(plan.evidence_rows, RendererCommercialQualityEvidenceKind::runtime_ui_package);
    plan.environment_package_ready =
        has_ready_kind(plan.evidence_rows, RendererCommercialQualityEvidenceKind::environment_package);
    plan.generated_game_package_ready =
        has_ready_kind(plan.evidence_rows, RendererCommercialQualityEvidenceKind::generated_game_package);
    for (const auto kind : kRequiredPackageKinds) {
        if (!has_ready_kind(plan.evidence_rows, kind)) {
            add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_required_package_row, kind,
                           rhi::BackendKind::null, {},
                           "renderer commercial closeout requires selected visible package evidence rows", 0U);
        }
    }

    plan.claim_control_ready = has_ready_kind(plan.evidence_rows, RendererCommercialQualityEvidenceKind::claim_control);
    if (!plan.claim_control_ready) {
        add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_claim_control_evidence,
                       RendererCommercialQualityEvidenceKind::claim_control, rhi::BackendKind::null, {},
                       "renderer commercial closeout requires explicit claim-control evidence", 0U);
    }

    plan.clean_room_source_review_ready =
        has_ready_kind(plan.evidence_rows, RendererCommercialQualityEvidenceKind::clean_room);
    if (!plan.clean_room_source_review_ready) {
        add_diagnostic(plan, RendererCommercialQualityCloseoutDiagnosticCode::missing_clean_room_evidence,
                       RendererCommercialQualityEvidenceKind::clean_room, rhi::BackendKind::null, {},
                       "renderer commercial closeout requires clean-room source review evidence", 0U);
    }
}

[[nodiscard]] bool has_invalid_request_diagnostic(const RendererCommercialQualityCloseoutPlan& plan) noexcept {
    constexpr std::array kInvalidDiagnostics{
        RendererCommercialQualityCloseoutDiagnosticCode::duplicate_evidence_row,
        RendererCommercialQualityCloseoutDiagnosticCode::invalid_evidence_row,
        RendererCommercialQualityCloseoutDiagnosticCode::unreviewed_host_validation_recipe,
        RendererCommercialQualityCloseoutDiagnosticCode::unsupported_native_handle_claim,
        RendererCommercialQualityCloseoutDiagnosticCode::cross_backend_inference,
        RendererCommercialQualityCloseoutDiagnosticCode::external_engine_parity_claim,
        RendererCommercialQualityCloseoutDiagnosticCode::unsupported_subjective_quality_claim,
        RendererCommercialQualityCloseoutDiagnosticCode::unsupported_crash_upload,
        RendererCommercialQualityCloseoutDiagnosticCode::external_engine_code_used,
        RendererCommercialQualityCloseoutDiagnosticCode::external_engine_sample_used,
        RendererCommercialQualityCloseoutDiagnosticCode::external_engine_asset_used,
        RendererCommercialQualityCloseoutDiagnosticCode::external_engine_trademark_used,
        RendererCommercialQualityCloseoutDiagnosticCode::external_engine_compatibility_claim,
        RendererCommercialQualityCloseoutDiagnosticCode::missing_third_party_notices,
        RendererCommercialQualityCloseoutDiagnosticCode::row_budget_exceeded,
    };
    return std::ranges::any_of(plan.diagnostics, [&kInvalidDiagnostics](const auto& diagnostic) {
        return std::ranges::find(kInvalidDiagnostics, diagnostic.code) != kInvalidDiagnostics.end();
    });
}

[[nodiscard]] std::size_t count_rejected_unsafe_rows(const RendererCommercialQualityCloseoutPlan& plan) {
    std::vector<std::string> keys;
    keys.reserve(plan.diagnostics.size());
    for (const auto& diagnostic : plan.diagnostics) {
        const auto invalid =
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::invalid_evidence_row ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::unreviewed_host_validation_recipe ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::unsupported_native_handle_claim ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::cross_backend_inference ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::external_engine_parity_claim ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::unsupported_subjective_quality_claim ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::unsupported_crash_upload ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::external_engine_code_used ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::external_engine_sample_used ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::external_engine_asset_used ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::external_engine_trademark_used ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::external_engine_compatibility_claim ||
            diagnostic.code == RendererCommercialQualityCloseoutDiagnosticCode::missing_third_party_notices;
        if (!invalid) {
            continue;
        }
        std::string key;
        key.append(std::to_string(static_cast<std::uint8_t>(diagnostic.kind)));
        key.push_back('\n');
        key.append(diagnostic.row_id);
        key.push_back('\n');
        key.append(std::to_string(diagnostic.source_index));
        if (std::ranges::find(keys, key) == keys.end()) {
            keys.push_back(std::move(key));
        }
    }
    return keys.size();
}

void compute_final_readiness(RendererCommercialQualityCloseoutPlan& plan) {
    const auto packages_ready = plan.visible_3d_package_ready && plan.runtime_ui_package_ready &&
                                plan.environment_package_ready && plan.generated_game_package_ready;
    plan.renderer_metal_broad_readiness = plan.renderer_backend_parity_ready && plan.metal_renderer_quality_ready &&
                                          plan.production_vfx_profiling_ready && plan.metal_memory_profiling_ready;
    plan.renderer_broad_quality_ready = plan.renderer_backend_parity_ready && plan.d3d12_renderer_quality_ready &&
                                        plan.vulkan_strict_renderer_quality_ready &&
                                        plan.metal_renderer_quality_ready && plan.production_vfx_profiling_ready &&
                                        plan.metal_memory_profiling_ready && packages_ready;
    plan.renderer_commercial_readiness =
        plan.renderer_broad_quality_ready && plan.renderer_metal_broad_readiness && plan.claim_control_ready &&
        plan.clean_room_source_review_ready && plan.third_party_notices_complete &&
        plan.native_handle_access_count == 0U && plan.cross_backend_inference_count == 0U &&
        plan.external_engine_parity_count == 0U && plan.external_engine_code_used_count == 0U &&
        plan.external_engine_sample_used_count == 0U && plan.external_engine_asset_used_count == 0U &&
        plan.external_engine_trademark_used_count == 0U && plan.external_engine_compatibility_claim_count == 0U;
}

void hash_mix(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value;
    hash *= 1099511628211ULL;
}

void hash_string(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto ch : value) {
        hash_mix(hash, static_cast<unsigned char>(ch));
    }
    hash_mix(hash, 0xffU);
}

[[nodiscard]] std::uint64_t compute_replay_hash(const RendererCommercialQualityCloseoutPlan& plan,
                                                const RendererCommercialQualityCloseoutDesc& desc) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, desc.seed);
    hash_mix(hash, desc.backend_parity.replay_hash);
    hash_mix(hash, desc.quality_matrix.replay_hash);
    hash_mix(hash, desc.vfx_profiling.replay_hash);
    for (const auto& row : plan.evidence_rows) {
        hash_string(hash, row.row_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.kind));
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.status));
        hash_mix(hash, row.reviewed ? 1U : 0U);
        hash_mix(hash, row.ready ? 1U : 0U);
        hash_mix(hash, row.host_gated ? 1U : 0U);
        hash_mix(hash, row.dependency_gated ? 1U : 0U);
        hash_string(hash, row.host_validation_recipe_id);
        hash_string(hash, row.package_counter_id);
        hash_mix(hash, row.request_native_handle_access ? 1U : 0U);
        hash_mix(hash, row.request_cross_backend_inference ? 1U : 0U);
        hash_mix(hash, row.request_external_engine_parity ? 1U : 0U);
        hash_mix(hash, row.request_subjective_quality_claim ? 1U : 0U);
        hash_mix(hash, row.request_crash_upload ? 1U : 0U);
        hash_mix(hash, row.external_engine_code_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_sample_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_asset_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_trademark_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_compatibility_claim ? 1U : 0U);
        hash_mix(hash, row.approved_external_material ? 1U : 0U);
        hash_mix(hash, row.third_party_notices_complete ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] bool request_has_any_rows(const RendererCommercialQualityCloseoutDesc& desc) noexcept {
    return !desc.evidence_rows.empty() ||
           desc.backend_parity.status != BackendRendererParityPolicyStatus::invalid_request ||
           desc.quality_matrix.status != RendererQualityMatrixStatus::invalid_request ||
           desc.vfx_profiling.status != RendererProductionVfxProfilingStatus::invalid_request;
}

} // namespace

bool RendererCommercialQualityCloseoutPlan::succeeded() const noexcept {
    return status == RendererCommercialQualityCloseoutStatus::ready && renderer_commercial_readiness;
}

RendererCommercialQualityCloseoutPlan
plan_renderer_commercial_quality_closeout(const RendererCommercialQualityCloseoutDesc& desc) {
    RendererCommercialQualityCloseoutPlan plan;

    if (!request_has_any_rows(desc)) {
        plan.status = RendererCommercialQualityCloseoutStatus::no_rows;
        return plan;
    }

    append_rows(plan, desc);
    validate_duplicate_rows(plan, desc);
    validate_row_shape(plan, desc);
    validate_safety_claims(plan, desc);
    validate_clean_room(plan, desc);
    validate_budget(plan, desc);
    summarize_rows(plan);
    compute_required_evidence(plan, desc);
    compute_final_readiness(plan);
    plan.rejected_unsafe_row_count = count_rejected_unsafe_rows(plan);
    sort_diagnostics(plan);

    if (has_invalid_request_diagnostic(plan)) {
        plan.status = RendererCommercialQualityCloseoutStatus::invalid_request;
        plan.replay_hash = 0U;
        return plan;
    }

    plan.replay_hash = compute_replay_hash(plan, desc);
    if (plan.renderer_commercial_readiness) {
        plan.status = RendererCommercialQualityCloseoutStatus::ready;
    } else if (plan.unsupported_row_count > 0U) {
        plan.status = RendererCommercialQualityCloseoutStatus::unsupported;
    } else if (plan.dependency_gated_row_count > 0U) {
        plan.status = RendererCommercialQualityCloseoutStatus::dependency_evidence_required;
    } else {
        plan.status = RendererCommercialQualityCloseoutStatus::host_evidence_required;
    }
    return plan;
}

} // namespace mirakana
