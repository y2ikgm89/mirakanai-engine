// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/renderer_commercial_readiness_evidence.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view kEvidenceRecipeId{"renderer-commercial-readiness-evidence"};
constexpr std::string_view kMetalMemoryProfilingRecipeId{"renderer-metal-memory-profiling-host-evidence"};
constexpr std::string_view kEvidenceSchemaVersion{"GameEngine.RendererCommercialReadinessEvidence.v1"};
constexpr std::string_view kMetalMemoryProfilingSchemaVersion{"GameEngine.RendererMetalMemoryProfilingHostEvidence.v1"};

constexpr std::array kRequiredSourceIds{
    std::string_view{"Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25"},
    std::string_view{"Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25"},
    std::string_view{"Apple-Metal-Framework-Memory-Capture-2026-06-25"},
    std::string_view{"Apple-Metal-Shading-Language-Specification-2026-06-25"},
    std::string_view{"Unity-Legal-Terms-2026-06-25"},
    std::string_view{"Epic-Unreal-Engine-EULA-Trademark-2026-06-25"},
    std::string_view{"Godot-Trademark-Licensing-2026-06-25"},
};

constexpr std::array kPackageKinds{
    RendererCommercialReadinessEvidenceKind::visible_3d_package,
    RendererCommercialReadinessEvidenceKind::runtime_ui_package,
    RendererCommercialReadinessEvidenceKind::environment_package,
    RendererCommercialReadinessEvidenceKind::generated_game_package,
};

[[nodiscard]] std::uint8_t evidence_kind_sort_key(RendererCommercialReadinessEvidenceKind kind) noexcept {
    return static_cast<std::uint8_t>(kind);
}

[[nodiscard]] std::uint8_t backend_sort_key(rhi::BackendKind backend) noexcept {
    return static_cast<std::uint8_t>(backend);
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

[[nodiscard]] bool is_lower_hex_sha256(std::string_view value) noexcept {
    return value.size() == 64U &&
           std::ranges::all_of(value, [](char ch) { return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'); });
}

[[nodiscard]] bool is_reviewed_evidence_recipe(std::string_view recipe_id) noexcept {
    return recipe_id == kEvidenceRecipeId || recipe_id == kMetalMemoryProfilingRecipeId;
}

void add_diagnostic(RendererCommercialReadinessPromotionPlan& plan,
                    RendererCommercialReadinessEvidenceDiagnosticCode code,
                    RendererCommercialReadinessEvidenceKind kind, rhi::BackendKind backend, std::string row_id,
                    std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RendererCommercialReadinessEvidenceDiagnostic{
        .code = code,
        .kind = kind,
        .backend = backend,
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_rows(std::vector<RendererCommercialReadinessEvidenceRow>& rows) {
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

void sort_sources(std::vector<RendererCommercialReadinessSourceRow>& rows) {
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) { return lhs.source_id < rhs.source_id; });
}

void sort_diagnostics(RendererCommercialReadinessPromotionPlan& plan) {
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
        return lhs.source_index < rhs.source_index;
    });
}

[[nodiscard]] bool source_ready(const RendererCommercialReadinessPromotionPlan& plan, std::string_view source_id) {
    return std::ranges::any_of(plan.source_rows, [source_id](const auto& row) {
        return row.source_id == source_id && row.reviewed && row.official_or_context7 && !row.stale;
    });
}

void validate_sources(RendererCommercialReadinessPromotionPlan& plan) {
    for (const auto& required_source_id : kRequiredSourceIds) {
        const auto found = std::ranges::find_if(
            plan.source_rows, [required_source_id](const auto& row) { return row.source_id == required_source_id; });
        if (found == plan.source_rows.end()) {
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_source_id,
                           RendererCommercialReadinessEvidenceKind::clean_room, rhi::BackendKind::null,
                           std::string{required_source_id}, "renderer commercial readiness source row is missing", 0U);
            continue;
        }
        if (!found->reviewed || !found->official_or_context7 || found->stale) {
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::stale_source_id,
                           RendererCommercialReadinessEvidenceKind::clean_room, rhi::BackendKind::null,
                           found->source_id, "renderer commercial readiness source row must be reviewed and current",
                           0U);
        }
    }
}

[[nodiscard]] bool row_base_shape_ready(const RendererCommercialReadinessPromotionPlan& plan,
                                        const RendererCommercialReadinessEvidenceRow& row) {
    return row.selected && row.ready && is_valid_id(row.row_id) && !has_native_token(row.row_id) &&
           source_ready(plan, row.source_id) && is_valid_id(row.source_id) &&
           is_valid_id(row.host_validation_recipe_id) && is_reviewed_evidence_recipe(row.host_validation_recipe_id) &&
           is_valid_id(row.validation_counter_id) && !has_native_token(row.validation_counter_id) &&
           !row.package_counter_id.empty() && is_valid_id(row.package_counter_id) &&
           !has_native_token(row.package_counter_id) && is_lower_hex_sha256(row.artifact_hash_sha256) &&
           is_valid_id(row.artifact_schema_version);
}

[[nodiscard]] bool is_metal_memory_kind(RendererCommercialReadinessEvidenceKind kind) noexcept {
    return kind == RendererCommercialReadinessEvidenceKind::metal_memory_residency ||
           kind == RendererCommercialReadinessEvidenceKind::metal_profiling_capture;
}

[[nodiscard]] bool row_has_external_material(const RendererCommercialReadinessEvidenceRow& row) noexcept {
    return row.external_engine_code_used || row.external_engine_sample_used || row.external_engine_asset_used ||
           row.external_engine_trademark_used || row.external_engine_ui_expression_used || row.external_engine_api_used;
}

[[nodiscard]] std::size_t external_material_flag_count(const RendererCommercialReadinessEvidenceRow& row) noexcept {
    return (row.external_engine_code_used ? 1U : 0U) + (row.external_engine_sample_used ? 1U : 0U) +
           (row.external_engine_asset_used ? 1U : 0U) + (row.external_engine_trademark_used ? 1U : 0U) +
           (row.external_engine_ui_expression_used ? 1U : 0U) + (row.external_engine_api_used ? 1U : 0U);
}

[[nodiscard]] bool row_has_external_claim(const RendererCommercialReadinessEvidenceRow& row) noexcept {
    return row.external_engine_compatibility_claim || row.external_engine_equivalence_claim ||
           row.external_engine_parity_claim;
}

[[nodiscard]] std::size_t external_claim_flag_count(const RendererCommercialReadinessEvidenceRow& row) noexcept {
    return (row.external_engine_compatibility_claim ? 1U : 0U) + (row.external_engine_equivalence_claim ? 1U : 0U) +
           (row.external_engine_parity_claim ? 1U : 0U);
}

[[nodiscard]] bool row_has_safety_blocker(const RendererCommercialReadinessEvidenceRow& row) noexcept {
    return row.request_native_handle_access || row.request_cross_backend_inference || row_has_external_material(row) ||
           row_has_external_claim(row) || !row.third_party_notices_complete;
}

[[nodiscard]] bool row_has_valid_artifact_schema(const RendererCommercialReadinessEvidenceRow& row) noexcept {
    if (is_metal_memory_kind(row.kind)) {
        return row.artifact_schema_version == kMetalMemoryProfilingSchemaVersion &&
               row.host_validation_recipe_id == kMetalMemoryProfilingRecipeId &&
               row.backend == rhi::BackendKind::metal && row.apple_host_metal_artifact;
    }
    return row.artifact_schema_version == kEvidenceSchemaVersion && row.host_validation_recipe_id == kEvidenceRecipeId;
}

[[nodiscard]] bool row_ready_for_kind(const RendererCommercialReadinessPromotionPlan& plan,
                                      const RendererCommercialReadinessEvidenceRow& row,
                                      RendererCommercialReadinessEvidenceKind kind) {
    return row.kind == kind && row_base_shape_ready(plan, row) && row_has_valid_artifact_schema(row) &&
           !row_has_safety_blocker(row) && !row.forbidden_material_diagnostic && !row.forbidden_material_rejected;
}

[[nodiscard]] bool has_ready_kind(const RendererCommercialReadinessPromotionPlan& plan,
                                  RendererCommercialReadinessEvidenceKind kind) {
    return std::ranges::any_of(plan.evidence_rows,
                               [&plan, kind](const auto& row) { return row_ready_for_kind(plan, row, kind); });
}

void append_rows(RendererCommercialReadinessPromotionPlan& plan, const RendererCommercialReadinessPromotionDesc& desc) {
    plan.source_rows = desc.source_rows;
    plan.evidence_rows = desc.evidence_rows;
    sort_sources(plan.source_rows);
    sort_rows(plan.evidence_rows);
    plan.row_count = plan.evidence_rows.size();
    for (const auto& row : plan.evidence_rows) {
        if (row.selected && row.ready) {
            ++plan.ready_row_count;
        }
    }
}

void validate_row_shape(RendererCommercialReadinessPromotionPlan& plan) {
    for (const auto& row : plan.evidence_rows) {
        if (row.kind == RendererCommercialReadinessEvidenceKind::forbidden_material) {
            if (row.forbidden_material_diagnostic && row.forbidden_material_rejected && !row.ready &&
                !row_has_external_material(row) && !row_has_external_claim(row)) {
                ++plan.rejected_forbidden_material_count;
                add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::forbidden_material_rejected,
                               row.kind, row.backend, row.row_id,
                               "forbidden external-engine material was rejected and cannot promote readiness",
                               row.source_index);
                continue;
            }
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::invalid_evidence_row, row.kind,
                           row.backend, row.row_id,
                           "forbidden material rows must be rejected diagnostics and must not be ready evidence",
                           row.source_index);
            continue;
        }

        if (!row_base_shape_ready(plan, row)) {
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::invalid_evidence_row, row.kind,
                           row.backend, row.row_id,
                           "renderer commercial readiness evidence rows require selected reviewed value ids, source "
                           "ids, counters, and SHA-256 artifact hashes",
                           row.source_index);
        }
        if (!row_has_valid_artifact_schema(row)) {
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::cross_backend_inference, row.kind,
                           row.backend, row.row_id,
                           "Metal memory/profiling rows must come from Apple-host Metal artifacts and other rows must "
                           "use the commercial readiness schema",
                           row.source_index);
        }
    }
}

void validate_safety_claims(RendererCommercialReadinessPromotionPlan& plan) {
    for (const auto& row : plan.evidence_rows) {
        if (row.request_native_handle_access) {
            ++plan.native_handle_access_count;
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::native_handle_access, row.kind,
                           row.backend, row.row_id,
                           "renderer commercial readiness promotion must not expose native handles", row.source_index);
        }
        if (row.request_cross_backend_inference) {
            ++plan.cross_backend_inference_count;
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::cross_backend_inference, row.kind,
                           row.backend, row.row_id,
                           "renderer commercial readiness promotion cannot infer proof across backends",
                           row.source_index);
        }
        if (row_has_external_material(row)) {
            plan.external_engine_material_used_count += external_material_flag_count(row);
            add_diagnostic(
                plan, RendererCommercialReadinessEvidenceDiagnosticCode::external_engine_material_used, row.kind,
                row.backend, row.row_id,
                "external engine code, samples, assets, trademarks, UI expression, or APIs are not accepted evidence",
                row.source_index);
        }
        if (row_has_external_claim(row)) {
            plan.external_engine_claim_count += external_claim_flag_count(row);
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::external_engine_claim, row.kind,
                           row.backend, row.row_id,
                           "Unity, Unreal Engine, Godot, compatibility, equivalence, and parity claims cannot promote "
                           "renderer readiness",
                           row.source_index);
        }
        if (!row.third_party_notices_complete) {
            add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_third_party_notice,
                           row.kind, row.backend, row.row_id,
                           "renderer commercial readiness evidence requires complete third-party notices",
                           row.source_index);
        }
    }
}

void validate_budget(RendererCommercialReadinessPromotionPlan& plan,
                     const RendererCommercialReadinessPromotionDesc& desc) {
    if (desc.evidence_rows.size() > desc.row_budget) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::row_budget_exceeded,
                       RendererCommercialReadinessEvidenceKind::claim_control, rhi::BackendKind::null, {},
                       "renderer commercial readiness evidence rows exceed the aggregate row budget", 0U);
    }
}

void compute_required_evidence(RendererCommercialReadinessPromotionPlan& plan,
                               const RendererCommercialReadinessPromotionDesc& desc) {
    if (!desc.quality_closeout.succeeded()) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_quality_closeout,
                       RendererCommercialReadinessEvidenceKind::renderer_quality_matrix, rhi::BackendKind::null, {},
                       "renderer commercial readiness promotion requires a ready quality closeout value plan", 0U);
    }

    plan.d3d12_evidence_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::d3d12);
    plan.vulkan_strict_evidence_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::vulkan_strict);
    plan.apple_metal_evidence_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::apple_metal);
    if (!plan.d3d12_evidence_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_d3d12_evidence,
                       RendererCommercialReadinessEvidenceKind::d3d12, rhi::BackendKind::d3d12, {},
                       "renderer commercial readiness requires selected D3D12 evidence", 0U);
    }
    if (!plan.vulkan_strict_evidence_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_vulkan_strict_evidence,
                       RendererCommercialReadinessEvidenceKind::vulkan_strict, rhi::BackendKind::vulkan, {},
                       "renderer commercial readiness requires selected strict Vulkan evidence", 0U);
    }
    if (!plan.apple_metal_evidence_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_apple_metal_evidence,
                       RendererCommercialReadinessEvidenceKind::apple_metal, rhi::BackendKind::metal, {},
                       "renderer commercial readiness requires selected Apple-host Metal evidence", 0U);
    }

    plan.renderer_quality_matrix_ready =
        has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::renderer_quality_matrix);
    if (!plan.renderer_quality_matrix_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_renderer_quality_matrix,
                       RendererCommercialReadinessEvidenceKind::renderer_quality_matrix, rhi::BackendKind::null, {},
                       "renderer commercial readiness requires selected renderer quality matrix evidence", 0U);
    }

    plan.production_vfx_profiling_ready =
        has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::production_vfx_profiling);
    if (!plan.production_vfx_profiling_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_production_vfx_profiling,
                       RendererCommercialReadinessEvidenceKind::production_vfx_profiling, rhi::BackendKind::null, {},
                       "renderer commercial readiness requires selected production VFX/profiling evidence", 0U);
    }

    plan.metal_memory_residency_ready =
        has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::metal_memory_residency);
    plan.metal_profiling_capture_ready =
        has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::metal_profiling_capture);
    if (!plan.metal_memory_residency_ready || !plan.metal_profiling_capture_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_metal_memory_profiling,
                       RendererCommercialReadinessEvidenceKind::metal_memory_residency, rhi::BackendKind::metal, {},
                       "broad Metal readiness requires Apple-host memory residency and profiling capture evidence", 0U);
    }

    plan.visible_3d_package_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::visible_3d_package);
    plan.runtime_ui_package_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::runtime_ui_package);
    plan.environment_package_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::environment_package);
    plan.generated_game_package_ready =
        has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::generated_game_package);
    for (const auto kind : kPackageKinds) {
        if (!has_ready_kind(plan, kind)) {
            add_diagnostic(
                plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_required_package_row, kind,
                rhi::BackendKind::null, {},
                "renderer commercial readiness requires selected 3D/UI/environment/generated-game package evidence",
                0U);
        }
    }

    plan.claim_control_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::claim_control);
    if (!plan.claim_control_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_claim_control,
                       RendererCommercialReadinessEvidenceKind::claim_control, rhi::BackendKind::null, {},
                       "renderer commercial readiness requires explicit claim-control evidence", 0U);
    }

    plan.clean_room_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::clean_room);
    if (!plan.clean_room_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_clean_room,
                       RendererCommercialReadinessEvidenceKind::clean_room, rhi::BackendKind::null, {},
                       "renderer commercial readiness requires clean-room legal/source review evidence", 0U);
    }

    plan.third_party_notice_ready = has_ready_kind(plan, RendererCommercialReadinessEvidenceKind::third_party_notice);
    if (!plan.third_party_notice_ready) {
        add_diagnostic(plan, RendererCommercialReadinessEvidenceDiagnosticCode::missing_third_party_notice,
                       RendererCommercialReadinessEvidenceKind::third_party_notice, rhi::BackendKind::null, {},
                       "renderer commercial readiness requires complete third-party notice evidence", 0U);
    }
}

[[nodiscard]] bool has_invalid_request_diagnostic(const RendererCommercialReadinessPromotionPlan& plan) {
    constexpr std::array kInvalidDiagnostics{
        RendererCommercialReadinessEvidenceDiagnosticCode::missing_source_id,
        RendererCommercialReadinessEvidenceDiagnosticCode::stale_source_id,
        RendererCommercialReadinessEvidenceDiagnosticCode::invalid_evidence_row,
        RendererCommercialReadinessEvidenceDiagnosticCode::native_handle_access,
        RendererCommercialReadinessEvidenceDiagnosticCode::cross_backend_inference,
        RendererCommercialReadinessEvidenceDiagnosticCode::external_engine_material_used,
        RendererCommercialReadinessEvidenceDiagnosticCode::external_engine_claim,
        RendererCommercialReadinessEvidenceDiagnosticCode::row_budget_exceeded,
    };
    return std::ranges::any_of(plan.diagnostics, [&kInvalidDiagnostics](const auto& diagnostic) {
        return std::ranges::find(kInvalidDiagnostics, diagnostic.code) != kInvalidDiagnostics.end();
    });
}

void compute_final_readiness(RendererCommercialReadinessPromotionPlan& plan,
                             const RendererCommercialReadinessPromotionDesc& desc) {
    const auto package_rows_ready = plan.visible_3d_package_ready && plan.runtime_ui_package_ready &&
                                    plan.environment_package_ready && plan.generated_game_package_ready;
    plan.renderer_backend_parity_ready = desc.quality_closeout.renderer_backend_parity_ready &&
                                         plan.d3d12_evidence_ready && plan.vulkan_strict_evidence_ready &&
                                         plan.apple_metal_evidence_ready;
    plan.renderer_metal_broad_readiness =
        plan.renderer_backend_parity_ready && desc.quality_closeout.renderer_metal_broad_readiness &&
        plan.apple_metal_evidence_ready && plan.metal_memory_residency_ready && plan.metal_profiling_capture_ready;
    plan.renderer_broad_quality_ready =
        plan.renderer_backend_parity_ready && desc.quality_closeout.renderer_broad_quality_ready &&
        plan.renderer_quality_matrix_ready && plan.production_vfx_profiling_ready &&
        plan.metal_memory_residency_ready && plan.metal_profiling_capture_ready && package_rows_ready;
    plan.renderer_commercial_readiness =
        desc.quality_closeout.succeeded() && plan.renderer_backend_parity_ready &&
        plan.renderer_metal_broad_readiness && plan.renderer_broad_quality_ready && plan.claim_control_ready &&
        plan.clean_room_ready && plan.third_party_notice_ready && plan.rejected_forbidden_material_count == 0U &&
        plan.native_handle_access_count == 0U && plan.cross_backend_inference_count == 0U &&
        plan.external_engine_material_used_count == 0U && plan.external_engine_claim_count == 0U;
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

[[nodiscard]] std::uint64_t compute_replay_hash(const RendererCommercialReadinessPromotionPlan& plan,
                                                const RendererCommercialReadinessPromotionDesc& desc) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, desc.seed);
    hash_mix(hash, desc.quality_closeout.replay_hash);
    hash_mix(hash, desc.quality_closeout.renderer_backend_parity_ready ? 1U : 0U);
    hash_mix(hash, desc.quality_closeout.renderer_metal_broad_readiness ? 1U : 0U);
    hash_mix(hash, desc.quality_closeout.renderer_broad_quality_ready ? 1U : 0U);
    hash_mix(hash, desc.quality_closeout.renderer_commercial_readiness ? 1U : 0U);
    for (const auto& source : plan.source_rows) {
        hash_string(hash, source.source_id);
        hash_mix(hash, source.reviewed ? 1U : 0U);
        hash_mix(hash, source.official_or_context7 ? 1U : 0U);
        hash_mix(hash, source.stale ? 1U : 0U);
    }
    for (const auto& row : plan.evidence_rows) {
        hash_string(hash, row.row_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.kind));
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, row.selected ? 1U : 0U);
        hash_mix(hash, row.ready ? 1U : 0U);
        hash_string(hash, row.source_id);
        hash_string(hash, row.host_validation_recipe_id);
        hash_string(hash, row.validation_counter_id);
        hash_string(hash, row.package_counter_id);
        hash_string(hash, row.artifact_hash_sha256);
        hash_string(hash, row.artifact_schema_version);
        hash_mix(hash, row.apple_host_metal_artifact ? 1U : 0U);
        hash_mix(hash, row.request_native_handle_access ? 1U : 0U);
        hash_mix(hash, row.request_cross_backend_inference ? 1U : 0U);
        hash_mix(hash, row.external_engine_code_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_sample_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_asset_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_trademark_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_ui_expression_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_api_used ? 1U : 0U);
        hash_mix(hash, row.external_engine_compatibility_claim ? 1U : 0U);
        hash_mix(hash, row.external_engine_equivalence_claim ? 1U : 0U);
        hash_mix(hash, row.external_engine_parity_claim ? 1U : 0U);
        hash_mix(hash, row.forbidden_material_diagnostic ? 1U : 0U);
        hash_mix(hash, row.forbidden_material_rejected ? 1U : 0U);
        hash_mix(hash, row.third_party_notices_complete ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] bool request_has_any_rows(const RendererCommercialReadinessPromotionDesc& desc) noexcept {
    return !desc.source_rows.empty() || !desc.evidence_rows.empty() ||
           desc.quality_closeout.status != RendererCommercialQualityCloseoutStatus::invalid_request;
}

} // namespace

bool RendererCommercialReadinessPromotionPlan::succeeded() const noexcept {
    return status == RendererCommercialReadinessPromotionStatus::ready && renderer_commercial_readiness;
}

RendererCommercialReadinessPromotionPlan
plan_renderer_commercial_readiness_promotion(const RendererCommercialReadinessPromotionDesc& desc) {
    RendererCommercialReadinessPromotionPlan plan;

    if (!request_has_any_rows(desc)) {
        plan.status = RendererCommercialReadinessPromotionStatus::no_rows;
        return plan;
    }

    append_rows(plan, desc);
    validate_sources(plan);
    validate_row_shape(plan);
    validate_safety_claims(plan);
    validate_budget(plan, desc);
    compute_required_evidence(plan, desc);
    compute_final_readiness(plan, desc);
    sort_diagnostics(plan);

    if (has_invalid_request_diagnostic(plan)) {
        plan.status = RendererCommercialReadinessPromotionStatus::invalid_request;
        plan.replay_hash = 0U;
        return plan;
    }

    plan.replay_hash = compute_replay_hash(plan, desc);
    plan.status = plan.renderer_commercial_readiness ? RendererCommercialReadinessPromotionStatus::ready
                                                     : RendererCommercialReadinessPromotionStatus::evidence_required;
    return plan;
}

} // namespace mirakana
