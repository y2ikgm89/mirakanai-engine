// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/mavg_metal_capability_policy.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool is_supported_capability(const MavgMetalCapabilityKind capability) noexcept {
    switch (capability) {
    case MavgMetalCapabilityKind::streamed_cluster_draw:
    case MavgMetalCapabilityKind::mesh_shader_execution:
    case MavgMetalCapabilityKind::gpu_memory_residency:
    case MavgMetalCapabilityKind::deformation_tier:
    case MavgMetalCapabilityKind::ray_tracing_consistency:
    case MavgMetalCapabilityKind::benchmark_evidence:
        return true;
    }
    return false;
}

[[nodiscard]] std::uint8_t capability_sort_key(const MavgMetalCapabilityKind capability) noexcept {
    return static_cast<std::uint8_t>(capability);
}

[[nodiscard]] bool contains_capability(const std::vector<MavgMetalCapabilityKind>& capabilities,
                                       const MavgMetalCapabilityKind capability) {
    return std::ranges::find(capabilities, capability) != capabilities.end();
}

[[nodiscard]] bool is_id_char(const char value) noexcept {
    return (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') || value == '_' || value == '-' ||
           value == '.';
}

[[nodiscard]] bool is_valid_id(const std::string& value) noexcept {
    return !value.empty() && std::ranges::all_of(value, is_id_char);
}

[[nodiscard]] bool has_native_token(const std::string_view value) noexcept {
    constexpr std::string_view kNativeTokens[] = {
        "id3d", "d3d12", "vk", "mtl", "comptr", "native",
    };
    return std::ranges::any_of(kNativeTokens,
                               [value](const auto token) { return value.find(token) != std::string_view::npos; });
}

[[nodiscard]] bool is_reviewed_metal_host_validation_recipe(const std::string& recipe_id) noexcept {
    constexpr std::string_view kReviewedRecipes[] = {
        "renderer-metal-apple-host-evidence",
        "mobile-packaging",
        "ios-simulator-smoke",
        "shader-toolchain",
    };
    return std::ranges::any_of(kReviewedRecipes,
                               [&recipe_id](const auto reviewed_recipe) { return recipe_id == reviewed_recipe; });
}

void hash_mix(std::uint64_t& hash, const std::uint64_t value) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
}

void hash_mix_string(std::uint64_t& hash, const std::string& value) noexcept {
    for (const auto character : value) {
        hash_mix(hash, static_cast<unsigned char>(character));
    }
}

void add_diagnostic(MavgMetalCapabilityPlan& plan, MavgMetalCapabilityDiagnosticCode code,
                    MavgMetalCapabilityKind capability, std::string row_id, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(MavgMetalCapabilityDiagnostic{
        .code = code,
        .capability = capability,
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_required_capabilities(std::vector<MavgMetalCapabilityKind>& capabilities) {
    std::ranges::sort(capabilities, [](const auto lhs, const auto rhs) {
        return capability_sort_key(lhs) < capability_sort_key(rhs);
    });
}

void sort_rows(std::vector<MavgMetalCapabilityRow>& rows) {
    std::ranges::sort(rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.capability != rhs.capability) {
            return capability_sort_key(lhs.capability) < capability_sort_key(rhs.capability);
        }
        if (lhs.capability_id != rhs.capability_id) {
            return lhs.capability_id < rhs.capability_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(MavgMetalCapabilityPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.capability != rhs.capability) {
            return capability_sort_key(lhs.capability) < capability_sort_key(rhs.capability);
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

[[nodiscard]] bool row_ids_valid(const MavgMetalCapabilityRow& row) noexcept {
    if (!is_valid_id(row.capability_id) || has_native_token(row.capability_id)) {
        return false;
    }
    if (row.host_validation_recipe_id.empty() || !is_valid_id(row.host_validation_recipe_id) ||
        has_native_token(row.host_validation_recipe_id)) {
        return false;
    }
    if (row.package_counter_id.empty()) {
        return row.status == MavgMetalCapabilityRowStatus::host_gated;
    }
    return is_valid_id(row.package_counter_id) && !has_native_token(row.package_counter_id);
}

[[nodiscard]] bool row_ids_have_native_token(const MavgMetalCapabilityRow& row) noexcept {
    return has_native_token(row.capability_id) || has_native_token(row.host_validation_recipe_id) ||
           (!row.package_counter_id.empty() && has_native_token(row.package_counter_id));
}

[[nodiscard]] bool host_recipe_reviewed(const MavgMetalCapabilityRow& row) noexcept {
    return is_valid_id(row.host_validation_recipe_id) && !has_native_token(row.host_validation_recipe_id) &&
           is_reviewed_metal_host_validation_recipe(row.host_validation_recipe_id);
}

[[nodiscard]] bool row_has_unsupported_claim(const MavgMetalCapabilityRow& row) noexcept {
    return row.request_cross_backend_inference || row.request_native_handle_access || row.request_gpu_execution ||
           row.request_nanite_claim;
}

[[nodiscard]] bool is_host_gated_row(const MavgMetalCapabilityRow& row) noexcept {
    return row.backend == rhi::BackendKind::metal && row.status == MavgMetalCapabilityRowStatus::host_gated &&
           row.reviewed && row.backend_local_evidence && !row.apple_host_validated && row.host_gate_required &&
           row_ids_valid(row) && host_recipe_reviewed(row) && !row_has_unsupported_claim(row);
}

[[nodiscard]] bool is_ready_row(const MavgMetalCapabilityRow& row) noexcept {
    return row.backend == rhi::BackendKind::metal && row.status == MavgMetalCapabilityRowStatus::ready &&
           row.reviewed && row.backend_local_evidence && row.apple_host_validated && !row.host_gate_required &&
           !row.package_counter_id.empty() && row_ids_valid(row) && host_recipe_reviewed(row) &&
           !row_has_unsupported_claim(row);
}

void validate_budget(MavgMetalCapabilityPlan& plan, const MavgMetalCapabilityRequest& request) {
    const auto row_count = request.required_capabilities.size() + request.rows.size();
    if (row_count > request.row_budget) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::row_budget_exceeded,
                       MavgMetalCapabilityKind::streamed_cluster_draw, {}, "MAVG Metal capability row budget exceeded",
                       0U);
    }
}

void validate_required_capabilities(MavgMetalCapabilityPlan& plan, const MavgMetalCapabilityRequest& request) {
    std::vector<MavgMetalCapabilityKind> seen;
    seen.reserve(request.required_capabilities.size());
    for (const auto capability : request.required_capabilities) {
        if (!is_supported_capability(capability)) {
            add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::invalid_required_capability, capability, {},
                           "MAVG Metal policy supports reviewed MAVG capability kinds only", 0U);
            continue;
        }
        if (contains_capability(seen, capability)) {
            add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::duplicate_required_capability, capability, {},
                           "MAVG Metal required capabilities must be unique", 0U);
            continue;
        }
        seen.push_back(capability);
    }
}

void validate_duplicate_rows(MavgMetalCapabilityPlan& plan, const MavgMetalCapabilityRequest& request) {
    std::vector<MavgMetalCapabilityKind> seen;
    seen.reserve(request.rows.size());
    for (const auto& row : request.rows) {
        if (contains_capability(seen, row.capability)) {
            add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::duplicate_capability_row, row.capability,
                           row.capability_id, "MAVG Metal policy allows one row per capability", row.source_index);
            continue;
        }
        seen.push_back(row.capability);
    }
}

void validate_required_rows(MavgMetalCapabilityPlan& plan, const MavgMetalCapabilityRequest& request) {
    for (const auto capability : request.required_capabilities) {
        if (!is_supported_capability(capability)) {
            continue;
        }
        const auto has_row =
            std::ranges::any_of(request.rows, [capability](const auto& row) { return row.capability == capability; });
        if (!has_row) {
            add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::missing_required_capability_row, capability, {},
                           "each required MAVG Metal capability needs a reviewed row", 0U);
        }
    }
}

void validate_row(MavgMetalCapabilityPlan& plan, const MavgMetalCapabilityRequest& request,
                  const MavgMetalCapabilityRow& row) {
    if (row.backend != rhi::BackendKind::metal) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::unsupported_backend, row.capability, row.capability_id,
                       "MAVG Metal policy cannot infer readiness from non-Metal backends", row.source_index);
    }
    if (!is_supported_capability(row.capability) ||
        !contains_capability(request.required_capabilities, row.capability) || !row.reviewed || !row_ids_valid(row) ||
        (row.status == MavgMetalCapabilityRowStatus::ready && row.host_gate_required) ||
        (row.status == MavgMetalCapabilityRowStatus::host_gated && row.apple_host_validated) ||
        row.status == MavgMetalCapabilityRowStatus::unsupported) {
        add_diagnostic(
            plan, MavgMetalCapabilityDiagnosticCode::invalid_capability_row, row.capability, row.capability_id,
            "MAVG Metal capability rows require reviewed Metal-local ready or host-gated evidence", row.source_index);
    }
    if (!row.backend_local_evidence) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::missing_backend_local_evidence, row.capability,
                       row.capability_id, "MAVG Metal capability rows must be backend-local", row.source_index);
    }
    if (row.status == MavgMetalCapabilityRowStatus::ready && !row.apple_host_validated) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::missing_apple_host_evidence, row.capability,
                       row.capability_id, "ready MAVG Metal capability rows require Apple-host evidence",
                       row.source_index);
    }
    if (row.host_validation_recipe_id.empty()) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::missing_host_validation_recipe, row.capability,
                       row.capability_id, "MAVG Metal capability rows require a validation recipe id",
                       row.source_index);
    } else if (is_valid_id(row.host_validation_recipe_id) && !has_native_token(row.host_validation_recipe_id) &&
               !is_reviewed_metal_host_validation_recipe(row.host_validation_recipe_id)) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::unreviewed_host_validation_recipe, row.capability,
                       row.capability_id, "MAVG Metal capability rows require a reviewed host validation recipe",
                       row.source_index);
    }
    if (row.status == MavgMetalCapabilityRowStatus::ready && row.package_counter_id.empty()) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::missing_package_counter_evidence, row.capability,
                       row.capability_id, "ready MAVG Metal capability rows require package counter evidence",
                       row.source_index);
    }
    if (row.request_cross_backend_inference) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::unsupported_cross_backend_inference, row.capability,
                       row.capability_id, "MAVG Metal readiness cannot be inferred from D3D12 or Vulkan evidence",
                       row.source_index);
    }
    if (row.request_native_handle_access || row_ids_have_native_token(row)) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::unsupported_native_handle_claim, row.capability,
                       row.capability_id, "MAVG Metal capability rows must not expose native handles",
                       row.source_index);
    }
    if (row.request_gpu_execution) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::unsupported_gpu_execution_claim, row.capability,
                       row.capability_id, "MAVG Metal capability policy does not execute GPU commands",
                       row.source_index);
    }
    if (row.request_nanite_claim) {
        add_diagnostic(plan, MavgMetalCapabilityDiagnosticCode::unsupported_nanite_claim, row.capability,
                       row.capability_id, "MAVG Metal capability policy does not claim Nanite equivalence",
                       row.source_index);
    }
}

void validate_rows(MavgMetalCapabilityPlan& plan, const MavgMetalCapabilityRequest& request) {
    for (const auto& row : request.rows) {
        validate_row(plan, request, row);
    }
}

void summarize(MavgMetalCapabilityPlan& plan) {
    for (const auto& row : plan.rows) {
        if (is_ready_row(row)) {
            ++plan.ready_row_count;
            ++plan.host_validated_capability_count;
        } else if (is_host_gated_row(row)) {
            ++plan.host_gated_row_count;
        } else if (row.status == MavgMetalCapabilityRowStatus::unsupported) {
            ++plan.unsupported_row_count;
        }
    }

    plan.requires_apple_host_evidence = !plan.required_capabilities.empty();
    plan.has_apple_host_evidence =
        plan.ready_row_count == plan.required_capabilities.size() && !plan.required_capabilities.empty();
    plan.metal_mavg_ready = plan.has_apple_host_evidence && plan.host_gated_row_count == 0U;
}

void compute_replay_hash(MavgMetalCapabilityPlan& plan, const MavgMetalCapabilityRequest& request) {
    if (!plan.diagnostics.empty() || plan.rows.empty()) {
        return;
    }

    std::uint64_t hash = 1469598103934665603ULL ^ request.seed;
    hash_mix(hash, request.row_budget);
    for (const auto capability : plan.required_capabilities) {
        hash_mix(hash, capability_sort_key(capability));
    }
    for (const auto& row : plan.rows) {
        hash_mix_string(hash, row.capability_id);
        hash_mix(hash, capability_sort_key(row.capability));
        hash_mix(hash, static_cast<std::uint8_t>(row.backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.status));
        hash_mix(hash, row.reviewed ? 1U : 0U);
        hash_mix(hash, row.backend_local_evidence ? 1U : 0U);
        hash_mix(hash, row.apple_host_validated ? 1U : 0U);
        hash_mix(hash, row.host_gate_required ? 1U : 0U);
        hash_mix_string(hash, row.host_validation_recipe_id);
        hash_mix_string(hash, row.package_counter_id);
        hash_mix(hash, row.request_cross_backend_inference ? 1U : 0U);
        hash_mix(hash, row.request_native_handle_access ? 1U : 0U);
        hash_mix(hash, row.request_gpu_execution ? 1U : 0U);
        hash_mix(hash, row.request_nanite_claim ? 1U : 0U);
        hash_mix(hash, row.source_index);
    }
    plan.replay_hash = hash;
}

} // namespace

bool MavgMetalCapabilityPlan::succeeded() const noexcept {
    return status == MavgMetalCapabilityStatus::ready && diagnostics.empty();
}

MavgMetalCapabilityPlan plan_mavg_metal_capabilities(const MavgMetalCapabilityRequest& request) {
    MavgMetalCapabilityPlan plan;
    plan.required_capabilities = request.required_capabilities;
    plan.rows = request.rows;
    plan.row_count = request.rows.size();
    sort_required_capabilities(plan.required_capabilities);
    sort_rows(plan.rows);

    if (request.required_capabilities.empty() && request.rows.empty()) {
        plan.status = MavgMetalCapabilityStatus::no_rows;
        return plan;
    }

    validate_budget(plan, request);
    validate_required_capabilities(plan, request);
    validate_duplicate_rows(plan, request);
    validate_required_rows(plan, request);
    validate_rows(plan, request);
    sort_diagnostics(plan);
    if (!plan.diagnostics.empty()) {
        plan.status = MavgMetalCapabilityStatus::invalid_request;
        plan.replay_hash = 0U;
        return plan;
    }

    summarize(plan);
    compute_replay_hash(plan, request);

    plan.status =
        plan.metal_mavg_ready ? MavgMetalCapabilityStatus::ready : MavgMetalCapabilityStatus::host_evidence_required;
    return plan;
}

const char* mavg_metal_capability_diagnostic_message(const MavgMetalCapabilityDiagnosticCode code) noexcept {
    switch (code) {
    case MavgMetalCapabilityDiagnosticCode::none:
        return "none";
    case MavgMetalCapabilityDiagnosticCode::invalid_required_capability:
        return "invalid_required_capability";
    case MavgMetalCapabilityDiagnosticCode::duplicate_required_capability:
        return "duplicate_required_capability";
    case MavgMetalCapabilityDiagnosticCode::unsupported_backend:
        return "unsupported_backend";
    case MavgMetalCapabilityDiagnosticCode::missing_required_capability_row:
        return "missing_required_capability_row";
    case MavgMetalCapabilityDiagnosticCode::duplicate_capability_row:
        return "duplicate_capability_row";
    case MavgMetalCapabilityDiagnosticCode::invalid_capability_row:
        return "invalid_capability_row";
    case MavgMetalCapabilityDiagnosticCode::missing_backend_local_evidence:
        return "missing_backend_local_evidence";
    case MavgMetalCapabilityDiagnosticCode::missing_apple_host_evidence:
        return "missing_apple_host_evidence";
    case MavgMetalCapabilityDiagnosticCode::missing_host_validation_recipe:
        return "missing_host_validation_recipe";
    case MavgMetalCapabilityDiagnosticCode::unreviewed_host_validation_recipe:
        return "unreviewed_host_validation_recipe";
    case MavgMetalCapabilityDiagnosticCode::missing_package_counter_evidence:
        return "missing_package_counter_evidence";
    case MavgMetalCapabilityDiagnosticCode::unsupported_cross_backend_inference:
        return "unsupported_cross_backend_inference";
    case MavgMetalCapabilityDiagnosticCode::unsupported_native_handle_claim:
        return "unsupported_native_handle_claim";
    case MavgMetalCapabilityDiagnosticCode::unsupported_gpu_execution_claim:
        return "unsupported_gpu_execution_claim";
    case MavgMetalCapabilityDiagnosticCode::unsupported_nanite_claim:
        return "unsupported_nanite_claim";
    case MavgMetalCapabilityDiagnosticCode::row_budget_exceeded:
        return "row_budget_exceeded";
    }
    return "unknown";
}

} // namespace mirakana
