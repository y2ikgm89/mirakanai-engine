// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/two_d_commercial_renderer_rhi_quality.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view kRequiredSourceStamp{"2026-07-01"};
constexpr std::string_view kSelectedRecipeId{"2d-commercial-renderer-rhi-quality"};
constexpr std::size_t kOfficialSourceKindCount{4U};

[[nodiscard]] bool string_contains(std::string_view value, std::string_view needle) noexcept {
    return value.find(needle) != std::string_view::npos;
}

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

void add_diagnostic(TwoDCommercialRendererRhiQualityResult& result, TwoDCommercialRendererRhiQualityDiagnosticCode code,
                    TwoDCommercialRendererRhiQualityEvidenceKind kind, rhi::BackendKind backend, std::string row_id,
                    std::string message) {
    result.diagnostics.push_back(TwoDCommercialRendererRhiQualityDiagnostic{
        .code = code,
        .kind = kind,
        .backend = backend,
        .row_id = std::move(row_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool row_status_ready(const TwoDCommercialRendererRhiQualityEvidenceRow& row) noexcept {
    return row.status == TwoDCommercialRendererRhiQualityEvidenceStatus::ready && row.ready && !row.host_gated &&
           !row.dependency_gated;
}

[[nodiscard]] bool row_status_host_gated(const TwoDCommercialRendererRhiQualityEvidenceRow& row) noexcept {
    return row.status == TwoDCommercialRendererRhiQualityEvidenceStatus::host_gated && !row.ready && row.host_gated &&
           !row.dependency_gated;
}

[[nodiscard]] bool row_status_dependency_gated(const TwoDCommercialRendererRhiQualityEvidenceRow& row) noexcept {
    return row.status == TwoDCommercialRendererRhiQualityEvidenceStatus::dependency_gated && !row.ready &&
           !row.host_gated && row.dependency_gated;
}

[[nodiscard]] bool row_status_unsupported(const TwoDCommercialRendererRhiQualityEvidenceRow& row) noexcept {
    return row.status == TwoDCommercialRendererRhiQualityEvidenceStatus::unsupported && !row.ready && !row.host_gated &&
           !row.dependency_gated;
}

[[nodiscard]] bool row_status_valid(const TwoDCommercialRendererRhiQualityEvidenceRow& row) noexcept {
    return row_status_ready(row) || row_status_host_gated(row) || row_status_dependency_gated(row) ||
           row_status_unsupported(row);
}

[[nodiscard]] bool ready_evidence_row(const TwoDCommercialRendererRhiQualityEvidenceRow& row,
                                      TwoDCommercialRendererRhiQualityEvidenceKind kind) noexcept {
    return row.kind == kind && row.selected && row.reviewed && row_status_ready(row) &&
           row.validation_recipe_id == kSelectedRecipeId && !row.package_counter_id.empty();
}

[[nodiscard]] bool sprite_throughput_ready(const SpriteBatchProductionThroughputPlan& plan) noexcept {
    const auto rows_within_budget = std::ranges::none_of(plan.rows, [](const auto& row) { return row.over_budget; });
    return plan.status == SpriteBatchProductionThroughputStatus::ready && plan.succeeded() &&
           plan.total_draw_rows != 0U && plan.total_instance_rows != 0U && plan.total_upload_bytes != 0U &&
           rows_within_budget && !plan.claimed_broad_optimization && !plan.claimed_cross_backend_parity &&
           !plan.claimed_metal_readiness;
}

void evaluate_official_sources(const TwoDCommercialRendererRhiQualityDesc& desc,
                               TwoDCommercialRendererRhiQualityResult& result) {
    result.official_source_rows = desc.official_source_rows.size();
    result.official_source_ledger_ready = true;
    std::array<bool, kOfficialSourceKindCount> seen_kinds{};

    for (const auto& row : desc.official_source_rows) {
        const auto index = static_cast<std::size_t>(row.kind);
        if (index < seen_kinds.size()) {
            seen_kinds[index] = true;
        }

        if (!is_valid_id(row.id) || !row.ready || !row.official || !row.public_docs_only) {
            result.official_source_ledger_ready = false;
            add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::invalid_official_source,
                           TwoDCommercialRendererRhiQualityEvidenceKind::claim_control, rhi::BackendKind::null, row.id,
                           "2D commercial renderer/RHI official source rows must be ready, official, and "
                           "public-docs-only");
        }
        if (!string_contains(row.id, kRequiredSourceStamp)) {
            result.official_source_ledger_ready = false;
            add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::stale_official_source,
                           TwoDCommercialRendererRhiQualityEvidenceKind::claim_control, rhi::BackendKind::null, row.id,
                           "2D commercial renderer/RHI official source rows must match the reviewed source date");
        }
    }

    for (std::size_t index = 0U; index < seen_kinds.size(); ++index) {
        if (seen_kinds[index]) {
            continue;
        }
        result.official_source_ledger_ready = false;
        const auto kind = static_cast<TwoDCommercialRendererRhiQualityOfficialSourceKind>(index);
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::missing_official_source,
                       TwoDCommercialRendererRhiQualityEvidenceKind::claim_control, rhi::BackendKind::null, {},
                       "2D commercial renderer/RHI is missing official source row " +
                           std::string{two_d_commercial_renderer_rhi_quality_official_source_kind_name(kind)});
    }
}

void summarize_evidence_rows(const TwoDCommercialRendererRhiQualityDesc& desc,
                             TwoDCommercialRendererRhiQualityResult& result) {
    result.evidence_row_count = desc.evidence_rows.size();
    for (const auto& row : desc.evidence_rows) {
        if (row_status_ready(row)) {
            ++result.ready_row_count;
        } else if (row_status_host_gated(row)) {
            ++result.host_gated_row_count;
        } else if (row_status_dependency_gated(row)) {
            ++result.dependency_gated_row_count;
        } else if (row_status_unsupported(row)) {
            ++result.unsupported_row_count;
        }
    }
}

void validate_evidence_rows(const TwoDCommercialRendererRhiQualityDesc& desc,
                            TwoDCommercialRendererRhiQualityResult& result) {
    for (const auto& row : desc.evidence_rows) {
        if (!is_valid_id(row.row_id) || !row.reviewed || !row_status_valid(row) ||
            (row_status_ready(row) &&
             (row.validation_recipe_id != kSelectedRecipeId || row.package_counter_id.empty()))) {
            add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::invalid_evidence_row, row.kind,
                           row.backend, row.row_id,
                           "2D commercial renderer/RHI rows require reviewed ids, exact status taxonomy, and "
                           "selected validation/package counters");
        }
        if (row.public_native_handles_exposed) {
            ++result.public_native_handle_rows;
            add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::public_native_handle_access,
                           row.kind, row.backend, row.row_id,
                           "2D commercial renderer/RHI evidence must not expose backend native handles");
        }
        if (row.request_cross_backend_inference) {
            ++result.cross_backend_inference_rows;
            add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::cross_backend_inference, row.kind,
                           row.backend, row.row_id,
                           "2D commercial renderer/RHI evidence cannot infer Vulkan or Metal readiness from D3D12");
        }
        auto add_external_engine_claim = [&](bool claim) {
            if (!claim) {
                return;
            }
            ++result.external_engine_claim_rows;
            add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::external_engine_claim, row.kind,
                           row.backend, row.row_id,
                           "2D commercial renderer/RHI evidence must remain first-party and clean-room");
        };
        add_external_engine_claim(row.external_engine_code_used);
        add_external_engine_claim(row.external_engine_sample_used);
        add_external_engine_claim(row.external_engine_asset_used);
        add_external_engine_claim(row.external_engine_trademark_used);
        add_external_engine_claim(row.external_engine_compatibility_claim);
        if (row.legal_approval_claim) {
            ++result.legal_approval_claim_rows;
            add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::legal_approval_claim, row.kind,
                           row.backend, row.row_id,
                           "2D commercial renderer/RHI can retain engineering evidence but cannot claim legal "
                           "approval");
        }
    }
}

[[nodiscard]] const TwoDCommercialRendererRhiQualityEvidenceRow*
find_ready_row(const TwoDCommercialRendererRhiQualityDesc& desc, TwoDCommercialRendererRhiQualityEvidenceKind kind,
               rhi::BackendKind backend) noexcept {
    const auto found = std::ranges::find_if(desc.evidence_rows, [kind, backend](const auto& row) {
        return row.backend == backend && ready_evidence_row(row, kind);
    });
    return found == desc.evidence_rows.end() ? nullptr : &*found;
}

void evaluate_d3d12_evidence(const TwoDCommercialRendererRhiQualityDesc& desc,
                             TwoDCommercialRendererRhiQualityResult& result) {
    const auto* row = find_ready_row(desc, TwoDCommercialRendererRhiQualityEvidenceKind::backend_host_evidence,
                                     rhi::BackendKind::d3d12);
    if (row != nullptr) {
        result.d3d12_command_allocator_fence_ready =
            row->command_allocator_fence_ready && row->command_list_close_execute_reset_ready;
        result.d3d12_command_list_close_execute_reset_ready = row->command_list_close_execute_reset_ready;
        result.d3d12_descriptor_heap_binding_ready = row->descriptor_heap_binding_ready;
        result.d3d12_pipeline_state_reuse_ready = row->pipeline_state_reuse_ready;
        result.d3d12_resource_barrier_ready = row->resource_barrier_ready;
        result.d3d12_debug_validation_clean = row->debug_validation_clean;
        result.d3d12_timestamp_or_pix_evidence_ready = row->timestamp_or_pix_evidence_ready;
        result.package_visible_readback_ready = row->package_visible_readback_ready;
    }

    if (!desc.selected_d3d12_ready_claim) {
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::selected_d3d12_claim_missing,
                       TwoDCommercialRendererRhiQualityEvidenceKind::backend_host_evidence, rhi::BackendKind::d3d12, {},
                       "2D commercial renderer/RHI readiness must be explicitly scoped to selected D3D12 proof");
    }

    const auto d3d12_ready = row != nullptr && result.d3d12_command_allocator_fence_ready &&
                             result.d3d12_command_list_close_execute_reset_ready &&
                             result.d3d12_descriptor_heap_binding_ready && result.d3d12_pipeline_state_reuse_ready &&
                             result.d3d12_resource_barrier_ready && result.d3d12_debug_validation_clean &&
                             result.d3d12_timestamp_or_pix_evidence_ready && result.package_visible_readback_ready;
    if (!d3d12_ready) {
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::missing_d3d12_evidence,
                       TwoDCommercialRendererRhiQualityEvidenceKind::backend_host_evidence, rhi::BackendKind::d3d12,
                       row == nullptr ? std::string{} : row->row_id,
                       "D3D12 proof requires command allocator/list fence discipline, descriptor heaps, PSO reuse, "
                       "resource barriers, debug validation, timestamp/PIX evidence, and package readback");
    }
}

void evaluate_package_evidence(const TwoDCommercialRendererRhiQualityDesc& desc,
                               TwoDCommercialRendererRhiQualityResult& result) {
    const auto* sprite_row = find_ready_row(desc, TwoDCommercialRendererRhiQualityEvidenceKind::sprite_batch_package,
                                            rhi::BackendKind::null);
    result.sprite_batch_budget_ready = sprite_row != nullptr && sprite_row->sprite_batch_budget_ready &&
                                       sprite_throughput_ready(desc.sprite_throughput);
    if (!result.sprite_batch_budget_ready) {
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::missing_sprite_batch_budget,
                       TwoDCommercialRendererRhiQualityEvidenceKind::sprite_batch_package, rhi::BackendKind::null,
                       sprite_row == nullptr ? std::string{} : sprite_row->row_id,
                       "2D commercial renderer/RHI requires selected sprite batch throughput rows within budget");
    }

    const auto* atlas_row = find_ready_row(desc, TwoDCommercialRendererRhiQualityEvidenceKind::atlas_residency_upload,
                                           rhi::BackendKind::null);
    result.atlas_residency_upload_ready =
        atlas_row != nullptr && atlas_row->atlas_residency_budget_ready && atlas_row->texture_upload_scheduling_ready;
    if (!result.atlas_residency_upload_ready) {
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::missing_atlas_upload_evidence,
                       TwoDCommercialRendererRhiQualityEvidenceKind::atlas_residency_upload, rhi::BackendKind::null,
                       atlas_row == nullptr ? std::string{} : atlas_row->row_id,
                       "2D commercial renderer/RHI requires atlas residency and texture upload scheduling evidence");
    }

    const auto* frame_row =
        find_ready_row(desc, TwoDCommercialRendererRhiQualityEvidenceKind::frame_pacing_budget, rhi::BackendKind::null);
    result.frame_pacing_budget_ready = frame_row != nullptr && frame_row->frame_pacing_budget_ready;
    if (!result.frame_pacing_budget_ready) {
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::missing_frame_pacing_budget,
                       TwoDCommercialRendererRhiQualityEvidenceKind::frame_pacing_budget, rhi::BackendKind::null,
                       frame_row == nullptr ? std::string{} : frame_row->row_id,
                       "2D commercial renderer/RHI requires package-visible frame pacing evidence");
    }

    result.claim_control_ready = find_ready_row(desc, TwoDCommercialRendererRhiQualityEvidenceKind::claim_control,
                                                rhi::BackendKind::null) != nullptr;
    if (!result.claim_control_ready) {
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::missing_claim_control_evidence,
                       TwoDCommercialRendererRhiQualityEvidenceKind::claim_control, rhi::BackendKind::null, {},
                       "2D commercial renderer/RHI requires explicit claim-control evidence");
    }

    result.clean_room_source_review_ready =
        find_ready_row(desc, TwoDCommercialRendererRhiQualityEvidenceKind::clean_room, rhi::BackendKind::null) !=
        nullptr;
    if (!result.clean_room_source_review_ready) {
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::missing_clean_room_evidence,
                       TwoDCommercialRendererRhiQualityEvidenceKind::clean_room, rhi::BackendKind::null, {},
                       "2D commercial renderer/RHI requires first-party clean-room review evidence");
    }
}

void evaluate_scope_claims(const TwoDCommercialRendererRhiQualityDesc& desc,
                           TwoDCommercialRendererRhiQualityResult& result) {
    if (desc.vulkan_ready_claim || desc.metal_ready_claim || desc.broad_backend_parity_claim ||
        desc.broad_renderer_rhi_quality_claim) {
        add_diagnostic(result, TwoDCommercialRendererRhiQualityDiagnosticCode::broad_backend_claim,
                       TwoDCommercialRendererRhiQualityEvidenceKind::backend_host_evidence, rhi::BackendKind::null, {},
                       "2D commercial renderer/RHI Phase 6 selected proof must not promote Vulkan, Metal, or broad "
                       "backend parity");
    }
}

[[nodiscard]] bool has_invalid_request_diagnostic(const TwoDCommercialRendererRhiQualityResult& result) noexcept {
    constexpr std::array kInvalidDiagnostics{
        TwoDCommercialRendererRhiQualityDiagnosticCode::invalid_official_source,
        TwoDCommercialRendererRhiQualityDiagnosticCode::stale_official_source,
        TwoDCommercialRendererRhiQualityDiagnosticCode::invalid_evidence_row,
        TwoDCommercialRendererRhiQualityDiagnosticCode::public_native_handle_access,
        TwoDCommercialRendererRhiQualityDiagnosticCode::cross_backend_inference,
        TwoDCommercialRendererRhiQualityDiagnosticCode::external_engine_claim,
        TwoDCommercialRendererRhiQualityDiagnosticCode::legal_approval_claim,
        TwoDCommercialRendererRhiQualityDiagnosticCode::broad_backend_claim,
    };
    return std::ranges::any_of(result.diagnostics, [&kInvalidDiagnostics](const auto& diagnostic) {
        return std::ranges::find(kInvalidDiagnostics, diagnostic.code) != kInvalidDiagnostics.end();
    });
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

[[nodiscard]] std::uint64_t compute_replay_hash(const TwoDCommercialRendererRhiQualityDesc& desc,
                                                const TwoDCommercialRendererRhiQualityResult& result) {
    std::uint64_t hash{1469598103934665603ULL};
    hash_mix(hash, desc.seed);
    hash_mix(hash, desc.sprite_throughput.total_draw_rows);
    hash_mix(hash, desc.sprite_throughput.total_instance_rows);
    hash_mix(hash, desc.sprite_throughput.total_upload_bytes);
    for (const auto& source : desc.official_source_rows) {
        hash_string(hash, source.id);
        hash_mix(hash, static_cast<std::uint8_t>(source.kind));
        hash_mix(hash, source.ready ? 1U : 0U);
        hash_mix(hash, source.official ? 1U : 0U);
        hash_mix(hash, source.public_docs_only ? 1U : 0U);
    }
    for (const auto& row : desc.evidence_rows) {
        hash_string(hash, row.row_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.kind));
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.status));
        hash_mix(hash, row.selected ? 1U : 0U);
        hash_mix(hash, row.reviewed ? 1U : 0U);
        hash_mix(hash, row.ready ? 1U : 0U);
        hash_mix(hash, row.host_gated ? 1U : 0U);
        hash_mix(hash, row.dependency_gated ? 1U : 0U);
        hash_string(hash, row.validation_recipe_id);
        hash_string(hash, row.package_counter_id);
        hash_mix(hash, row.command_allocator_fence_ready ? 1U : 0U);
        hash_mix(hash, row.command_list_close_execute_reset_ready ? 1U : 0U);
        hash_mix(hash, row.descriptor_heap_binding_ready ? 1U : 0U);
        hash_mix(hash, row.pipeline_state_reuse_ready ? 1U : 0U);
        hash_mix(hash, row.resource_barrier_ready ? 1U : 0U);
        hash_mix(hash, row.debug_validation_clean ? 1U : 0U);
        hash_mix(hash, row.timestamp_or_pix_evidence_ready ? 1U : 0U);
        hash_mix(hash, row.package_visible_readback_ready ? 1U : 0U);
        hash_mix(hash, row.sprite_batch_budget_ready ? 1U : 0U);
        hash_mix(hash, row.atlas_residency_budget_ready ? 1U : 0U);
        hash_mix(hash, row.texture_upload_scheduling_ready ? 1U : 0U);
        hash_mix(hash, row.frame_pacing_budget_ready ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    hash_mix(hash, result.selected_d3d12_renderer_rhi_quality_ready ? 1U : 0U);
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] bool request_has_any_rows(const TwoDCommercialRendererRhiQualityDesc& desc) noexcept {
    return !desc.evidence_rows.empty() ||
           std::ranges::any_of(desc.official_source_rows, [](const auto& row) { return !row.id.empty(); }) ||
           desc.sprite_throughput.status != SpriteBatchProductionThroughputStatus::invalid_request;
}

void compute_final_readiness(const TwoDCommercialRendererRhiQualityDesc& desc,
                             TwoDCommercialRendererRhiQualityResult& result) {
    result.vulkan_strict_renderer_rhi_quality_ready = false;
    result.metal_renderer_rhi_quality_ready = false;
    result.broad_backend_parity_ready = false;
    result.broad_renderer_rhi_quality_ready = false;
    result.selected_d3d12_renderer_rhi_quality_ready =
        result.official_source_ledger_ready && desc.selected_d3d12_ready_claim &&
        result.d3d12_command_allocator_fence_ready && result.d3d12_command_list_close_execute_reset_ready &&
        result.d3d12_descriptor_heap_binding_ready && result.d3d12_pipeline_state_reuse_ready &&
        result.d3d12_resource_barrier_ready && result.d3d12_debug_validation_clean &&
        result.d3d12_timestamp_or_pix_evidence_ready && result.package_visible_readback_ready &&
        result.sprite_batch_budget_ready && result.atlas_residency_upload_ready && result.frame_pacing_budget_ready &&
        result.claim_control_ready && result.clean_room_source_review_ready && result.public_native_handle_rows == 0U &&
        result.cross_backend_inference_rows == 0U && result.external_engine_claim_rows == 0U &&
        result.legal_approval_claim_rows == 0U;
}

void sort_diagnostics(TwoDCommercialRendererRhiQualityResult& result) {
    std::ranges::sort(result.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.kind != rhs.kind) {
            return static_cast<std::uint8_t>(lhs.kind) < static_cast<std::uint8_t>(rhs.kind);
        }
        if (lhs.backend != rhs.backend) {
            return static_cast<std::uint8_t>(lhs.backend) < static_cast<std::uint8_t>(rhs.backend);
        }
        return lhs.row_id < rhs.row_id;
    });
}

} // namespace

bool TwoDCommercialRendererRhiQualityResult::succeeded() const noexcept {
    return status == TwoDCommercialRendererRhiQualityStatus::ready && selected_d3d12_renderer_rhi_quality_ready &&
           diagnostics.empty();
}

std::string_view two_d_commercial_renderer_rhi_quality_official_source_kind_name(
    TwoDCommercialRendererRhiQualityOfficialSourceKind kind) noexcept {
    switch (kind) {
    case TwoDCommercialRendererRhiQualityOfficialSourceKind::microsoft_direct3d12:
        return "microsoft_direct3d12";
    case TwoDCommercialRendererRhiQualityOfficialSourceKind::khronos_vulkan:
        return "khronos_vulkan";
    case TwoDCommercialRendererRhiQualityOfficialSourceKind::apple_metal:
        return "apple_metal";
    case TwoDCommercialRendererRhiQualityOfficialSourceKind::repository_legal_policy:
        return "repository_legal_policy";
    }
    return "unknown";
}

TwoDCommercialRendererRhiQualityResult
evaluate_2d_commercial_renderer_rhi_quality(const TwoDCommercialRendererRhiQualityDesc& desc) {
    TwoDCommercialRendererRhiQualityResult result;
    if (!request_has_any_rows(desc)) {
        result.status = TwoDCommercialRendererRhiQualityStatus::no_rows;
        return result;
    }

    evaluate_official_sources(desc, result);
    summarize_evidence_rows(desc, result);
    validate_evidence_rows(desc, result);
    evaluate_scope_claims(desc, result);
    evaluate_d3d12_evidence(desc, result);
    evaluate_package_evidence(desc, result);
    compute_final_readiness(desc, result);
    sort_diagnostics(result);

    if (has_invalid_request_diagnostic(result)) {
        result.status = TwoDCommercialRendererRhiQualityStatus::invalid_request;
        result.replay_hash = 0U;
        return result;
    }

    result.replay_hash = compute_replay_hash(desc, result);
    if (result.selected_d3d12_renderer_rhi_quality_ready) {
        result.status = TwoDCommercialRendererRhiQualityStatus::ready;
    } else if (result.unsupported_row_count > 0U) {
        result.status = TwoDCommercialRendererRhiQualityStatus::unsupported;
    } else if (result.dependency_gated_row_count > 0U) {
        result.status = TwoDCommercialRendererRhiQualityStatus::dependency_evidence_required;
    } else {
        result.status = TwoDCommercialRendererRhiQualityStatus::host_evidence_required;
    }
    return result;
}

} // namespace mirakana
