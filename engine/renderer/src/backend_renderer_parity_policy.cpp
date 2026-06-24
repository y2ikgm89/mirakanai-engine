// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/backend_renderer_parity_policy.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

constexpr std::string_view kAppleMetalEnvironmentHostValidationRecipeId{"renderer-metal-apple-host-evidence"};

struct AppleMetalEnvironmentProofMapping {
    BackendRendererParityFeatureKind feature{BackendRendererParityFeatureKind::synchronization};
    std::string_view proof_id;
    std::string_view package_counter_id;
};

struct AppleMetalMemoryProfilingProofMapping {
    BackendRendererParityFeatureKind feature{BackendRendererParityFeatureKind::memory_residency};
    std::string_view proof_id;
    std::string_view package_counter_id;
};

constexpr AppleMetalEnvironmentProofMapping kAppleMetalEnvironmentProofMappings[] = {
    {
        .feature = BackendRendererParityFeatureKind::synchronization,
        .proof_id = "apple_metal_environment.synchronization",
        .package_counter_id = "metal_environment_synchronization_evidence_ready",
    },
    {
        .feature = BackendRendererParityFeatureKind::shader_validation,
        .proof_id = "apple_metal_environment.shader_validation",
        .package_counter_id = "metal_environment_metallib_valid",
    },
    {
        .feature = BackendRendererParityFeatureKind::package_evidence,
        .proof_id = "apple_metal_environment.package_evidence",
        .package_counter_id = "metal_environment_ready_rows",
    },
};

constexpr AppleMetalMemoryProfilingProofMapping kAppleMetalMemoryProfilingProofMappings[] = {
    {
        .feature = BackendRendererParityFeatureKind::memory_residency,
        .proof_id = "apple_metal_memory_profiling.memory_residency",
        .package_counter_id = "metal_memory_residency_evidence_ready",
    },
    {
        .feature = BackendRendererParityFeatureKind::profiling_capture,
        .proof_id = "apple_metal_memory_profiling.profiling_capture",
        .package_counter_id = "metal_profiling_capture_evidence_ready",
    },
};

[[nodiscard]] bool is_supported_backend(const rhi::BackendKind backend) noexcept {
    return backend == rhi::BackendKind::d3d12 || backend == rhi::BackendKind::vulkan ||
           backend == rhi::BackendKind::metal;
}

[[nodiscard]] bool is_supported_feature(const BackendRendererParityFeatureKind feature) noexcept {
    switch (feature) {
    case BackendRendererParityFeatureKind::synchronization:
    case BackendRendererParityFeatureKind::shader_validation:
    case BackendRendererParityFeatureKind::memory_residency:
    case BackendRendererParityFeatureKind::profiling_capture:
    case BackendRendererParityFeatureKind::package_evidence:
        return true;
    }
    return false;
}

[[nodiscard]] std::uint8_t backend_sort_key(const rhi::BackendKind backend) noexcept {
    return static_cast<std::uint8_t>(backend);
}

[[nodiscard]] std::uint8_t feature_sort_key(const BackendRendererParityFeatureKind feature) noexcept {
    return static_cast<std::uint8_t>(feature);
}

[[nodiscard]] bool contains_backend(const std::vector<rhi::BackendKind>& backends, const rhi::BackendKind backend) {
    return std::ranges::find(backends, backend) != backends.end();
}

[[nodiscard]] bool contains_feature(const std::vector<BackendRendererParityFeatureKind>& features,
                                    const BackendRendererParityFeatureKind feature) {
    return std::ranges::find(features, feature) != features.end();
}

[[nodiscard]] bool is_id_char(const char c) noexcept {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '-';
}

[[nodiscard]] bool is_valid_id(const std::string& value) noexcept {
    return !value.empty() && std::ranges::all_of(value, is_id_char);
}

[[nodiscard]] bool
apple_metal_environment_evidence_complete(const BackendRendererParityAppleMetalEnvironmentEvidenceDesc& desc) noexcept {
    return desc.runtime_ready && desc.command_queue_ready && desc.shader_library_ready && desc.render_pipeline_ready &&
           desc.compute_pipeline_ready && desc.render_pass_ready && desc.resource_evidence_ready &&
           desc.synchronization_evidence_ready && desc.package_evidence_ready && desc.render_readback_nonzero &&
           desc.compute_readback_nonzero &&
           desc.host_validation_recipe_id == kAppleMetalEnvironmentHostValidationRecipeId;
}

[[nodiscard]] bool apple_metal_memory_profiling_common_evidence_complete(
    const BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc& desc) noexcept {
    return desc.runtime_ready && desc.command_queue_ready &&
           desc.host_validation_recipe_id == kAppleMetalEnvironmentHostValidationRecipeId;
}

[[nodiscard]] bool apple_metal_memory_residency_evidence_complete(
    const BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc& desc) noexcept {
    return apple_metal_memory_profiling_common_evidence_complete(desc) && desc.heap_allocation_ready &&
           desc.residency_set_ready && desc.residency_commit_ready && desc.residency_pressure_evidence_ready;
}

[[nodiscard]] bool apple_metal_profiling_capture_evidence_complete(
    const BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc& desc) noexcept {
    return apple_metal_memory_profiling_common_evidence_complete(desc) && desc.capture_manager_ready &&
           desc.capture_scope_ready && desc.capture_boundary_ready && desc.capture_artifact_ready;
}

[[nodiscard]] BackendRendererParityProofRow
make_apple_metal_environment_proof_row(const AppleMetalEnvironmentProofMapping& mapping, const bool ready,
                                       const std::uint32_t source_index) {
    return BackendRendererParityProofRow{
        .proof_id = std::string{mapping.proof_id},
        .feature = mapping.feature,
        .selected_backend = rhi::BackendKind::metal,
        .proof_backend = rhi::BackendKind::metal,
        .reviewed = true,
        .host_validated = ready,
        .host_gate_required = !ready,
        .host_validation_recipe_id = std::string{kAppleMetalEnvironmentHostValidationRecipeId},
        .request_native_handle_access = false,
        .package_counter_id = ready ? std::string{mapping.package_counter_id} : std::string{},
        .source_index = source_index,
    };
}

[[nodiscard]] BackendRendererParityProofRow
make_apple_metal_memory_profiling_proof_row(const AppleMetalMemoryProfilingProofMapping& mapping, const bool ready,
                                            const std::uint32_t source_index) {
    return BackendRendererParityProofRow{
        .proof_id = std::string{mapping.proof_id},
        .feature = mapping.feature,
        .selected_backend = rhi::BackendKind::metal,
        .proof_backend = rhi::BackendKind::metal,
        .reviewed = true,
        .host_validated = ready,
        .host_gate_required = !ready,
        .host_validation_recipe_id = std::string{kAppleMetalEnvironmentHostValidationRecipeId},
        .request_native_handle_access = false,
        .package_counter_id = ready ? std::string{mapping.package_counter_id} : std::string{},
        .source_index = source_index,
    };
}

[[nodiscard]] bool has_native_token(const std::string_view value) noexcept {
    constexpr std::string_view kNativeTokens[] = {
        "ID3D", "D3D12", "Vk", "vk", "MTL", "metal::", "ComPtr",
    };
    return std::ranges::any_of(kNativeTokens,
                               [value](const auto token) { return value.find(token) != std::string_view::npos; });
}

[[nodiscard]] bool is_reviewed_metal_host_validation_recipe(const std::string& recipe_id) noexcept {
    constexpr std::string_view kReviewedMetalHostValidationRecipes[] = {
        "shader-toolchain",
        "mobile-packaging",
        kAppleMetalEnvironmentHostValidationRecipeId,
        "ios-simulator-smoke",
    };
    return std::ranges::any_of(kReviewedMetalHostValidationRecipes,
                               [&recipe_id](const auto reviewed_id) { return recipe_id == reviewed_id; });
}

void hash_mix(std::uint64_t& hash, const std::uint64_t value) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
}

void hash_mix_string(std::uint64_t& hash, const std::string& value) noexcept {
    for (const auto c : value) {
        hash_mix(hash, static_cast<unsigned char>(c));
    }
}

void add_diagnostic(BackendRendererParityPolicyPlan& plan, BackendRendererParityDiagnosticCode code,
                    rhi::BackendKind backend, BackendRendererParityFeatureKind feature, std::string row_id,
                    std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(BackendRendererParityDiagnostic{
        .code = code,
        .backend = backend,
        .feature = feature,
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_backends(std::vector<rhi::BackendKind>& backends) {
    std::ranges::sort(backends,
                      [](const auto lhs, const auto rhs) { return backend_sort_key(lhs) < backend_sort_key(rhs); });
}

void sort_features(std::vector<BackendRendererParityFeatureKind>& features) {
    std::ranges::sort(features,
                      [](const auto lhs, const auto rhs) { return feature_sort_key(lhs) < feature_sort_key(rhs); });
}

void sort_proofs(std::vector<BackendRendererParityProofRow>& proofs) {
    std::ranges::sort(proofs, [](const auto& lhs, const auto& rhs) {
        if (lhs.selected_backend != rhs.selected_backend) {
            return backend_sort_key(lhs.selected_backend) < backend_sort_key(rhs.selected_backend);
        }
        if (lhs.feature != rhs.feature) {
            return feature_sort_key(lhs.feature) < feature_sort_key(rhs.feature);
        }
        if (lhs.proof_id != rhs.proof_id) {
            return lhs.proof_id < rhs.proof_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void sort_diagnostics(BackendRendererParityPolicyPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.backend != rhs.backend) {
            return backend_sort_key(lhs.backend) < backend_sort_key(rhs.backend);
        }
        if (lhs.feature != rhs.feature) {
            return feature_sort_key(lhs.feature) < feature_sort_key(rhs.feature);
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

[[nodiscard]] bool row_backend_allowed(const BackendRendererParityPolicyRequest& request,
                                       const rhi::BackendKind backend) {
    return is_supported_backend(backend) && contains_backend(request.required_backends, backend);
}

[[nodiscard]] bool row_feature_allowed(const BackendRendererParityPolicyRequest& request,
                                       const BackendRendererParityFeatureKind feature) {
    return is_supported_feature(feature) && contains_feature(request.required_features, feature);
}

[[nodiscard]] bool proof_ids_valid(const BackendRendererParityProofRow& row) noexcept {
    if (!is_valid_id(row.proof_id) || has_native_token(row.proof_id)) {
        return false;
    }
    if (!row.host_validation_recipe_id.empty() &&
        (!is_valid_id(row.host_validation_recipe_id) || has_native_token(row.host_validation_recipe_id))) {
        return false;
    }
    if (row.package_counter_id.empty()) {
        return row.host_gate_required;
    }
    return is_valid_id(row.package_counter_id) && !has_native_token(row.package_counter_id);
}

[[nodiscard]] bool host_validation_recipe_ready(const BackendRendererParityProofRow& row) noexcept {
    return row.selected_backend != rhi::BackendKind::metal ||
           (!row.host_validation_recipe_id.empty() && is_valid_id(row.host_validation_recipe_id) &&
            !has_native_token(row.host_validation_recipe_id) &&
            is_reviewed_metal_host_validation_recipe(row.host_validation_recipe_id));
}

[[nodiscard]] bool is_host_gate_row(const BackendRendererParityProofRow& row) noexcept {
    return row.selected_backend == rhi::BackendKind::metal && row.proof_backend == rhi::BackendKind::metal &&
           row.host_gate_required && !row.host_validated && row.reviewed && !row.request_native_handle_access &&
           host_validation_recipe_ready(row) && proof_ids_valid(row);
}

[[nodiscard]] bool is_ready_row(const BackendRendererParityProofRow& row) noexcept {
    return backend_renderer_parity_proof_matches_selected_backend(BackendRendererParityProofDesc{
               .selected_backend = row.selected_backend,
               .proof_backend = row.proof_backend,
           }) &&
           row.reviewed && row.host_validated && !row.host_gate_required && !row.request_native_handle_access &&
           host_validation_recipe_ready(row) && proof_ids_valid(row);
}

void validate_budget(BackendRendererParityPolicyPlan& plan, const BackendRendererParityPolicyRequest& request) {
    const auto row_count = request.required_backends.size() + request.required_features.size() + request.proofs.size();
    if (row_count > request.row_budget) {
        add_diagnostic(plan, BackendRendererParityDiagnosticCode::row_budget_exceeded, rhi::BackendKind::null,
                       BackendRendererParityFeatureKind::synchronization, {},
                       "backend renderer parity policy row budget exceeded", 0U);
    }
}

void validate_required_backends(BackendRendererParityPolicyPlan& plan,
                                const BackendRendererParityPolicyRequest& request) {
    std::vector<rhi::BackendKind> seen;
    seen.reserve(request.required_backends.size());
    for (const auto backend : request.required_backends) {
        if (!is_supported_backend(backend)) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::invalid_required_backend, backend,
                           BackendRendererParityFeatureKind::synchronization, {},
                           "backend renderer parity supports d3d12, vulkan, and metal backends only", 0U);
            continue;
        }
        if (contains_backend(seen, backend)) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::duplicate_required_backend, backend,
                           BackendRendererParityFeatureKind::synchronization, {},
                           "backend renderer parity required backends must be unique", 0U);
            continue;
        }
        seen.push_back(backend);
    }
}

void validate_required_features(BackendRendererParityPolicyPlan& plan,
                                const BackendRendererParityPolicyRequest& request) {
    std::vector<BackendRendererParityFeatureKind> seen;
    seen.reserve(request.required_features.size());
    for (const auto feature : request.required_features) {
        if (!is_supported_feature(feature)) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::invalid_required_feature, rhi::BackendKind::null,
                           feature, {}, "backend renderer parity required features must use supported proof kinds", 0U);
            continue;
        }
        if (contains_feature(seen, feature)) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::duplicate_required_feature,
                           rhi::BackendKind::null, feature, {},
                           "backend renderer parity required features must be unique", 0U);
            continue;
        }
        seen.push_back(feature);
    }
}

void validate_duplicate_proofs(BackendRendererParityPolicyPlan& plan,
                               const BackendRendererParityPolicyRequest& request) {
    std::vector<std::string> seen;
    seen.reserve(request.proofs.size());
    for (const auto& row : request.proofs) {
        std::string key;
        key.append(std::to_string(static_cast<std::uint8_t>(row.selected_backend)));
        key.push_back('\n');
        key.append(std::to_string(static_cast<std::uint8_t>(row.feature)));
        if (std::ranges::find(seen, key) != seen.end()) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::duplicate_proof, row.selected_backend,
                           row.feature, row.proof_id,
                           "backend renderer parity allows one proof per backend and required feature",
                           row.source_index);
            continue;
        }
        seen.push_back(std::move(key));
    }
}

void validate_required_proofs(BackendRendererParityPolicyPlan& plan,
                              const BackendRendererParityPolicyRequest& request) {
    for (const auto backend : request.required_backends) {
        if (!is_supported_backend(backend)) {
            continue;
        }
        for (const auto feature : request.required_features) {
            if (!is_supported_feature(feature)) {
                continue;
            }
            const auto has_proof = std::ranges::any_of(request.proofs, [backend, feature](const auto& row) {
                return row.selected_backend == backend && row.feature == feature;
            });
            if (!has_proof) {
                add_diagnostic(plan, BackendRendererParityDiagnosticCode::missing_required_proof, backend, feature, {},
                               "each required backend needs backend-local renderer parity proof for every feature", 0U);
            }
        }
    }
}

void validate_proof_rows(BackendRendererParityPolicyPlan& plan, const BackendRendererParityPolicyRequest& request) {
    for (const auto& row : request.proofs) {
        if (!row_backend_allowed(request, row.selected_backend) || !is_supported_backend(row.proof_backend)) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::unsupported_backend, row.selected_backend,
                           row.feature, row.proof_id,
                           "backend renderer parity proof backend must be required and supported", row.source_index);
        }
        if (!row_feature_allowed(request, row.feature) || !row.reviewed || !proof_ids_valid(row) ||
            (row.host_gate_required && row.selected_backend != rhi::BackendKind::metal) ||
            (row.host_gate_required && row.host_validated)) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::invalid_proof, row.selected_backend, row.feature,
                           row.proof_id,
                           "backend renderer parity proof rows require reviewed ids, counters, and explicit Metal host "
                           "gates",
                           row.source_index);
        }
        if (!backend_renderer_parity_proof_matches_selected_backend(BackendRendererParityProofDesc{
                .selected_backend = row.selected_backend,
                .proof_backend = row.proof_backend,
            })) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::cross_backend_proof_transfer,
                           row.selected_backend, row.feature, row.proof_id,
                           "renderer backend parity proof must come from the selected backend", row.source_index);
        }
        if (row.selected_backend == rhi::BackendKind::metal && !row.host_validated && !row.host_gate_required) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::missing_metal_host_evidence, row.selected_backend,
                           row.feature, row.proof_id,
                           "Metal renderer parity needs Apple host evidence or an explicit host gate",
                           row.source_index);
        }
        if (row.selected_backend == rhi::BackendKind::metal && row.host_validation_recipe_id.empty()) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::missing_host_validation_recipe,
                           row.selected_backend, row.feature, row.proof_id,
                           "Metal renderer parity host evidence needs an explicit validation recipe id",
                           row.source_index);
        } else if (row.selected_backend == rhi::BackendKind::metal && is_valid_id(row.host_validation_recipe_id) &&
                   !has_native_token(row.host_validation_recipe_id) &&
                   !is_reviewed_metal_host_validation_recipe(row.host_validation_recipe_id)) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::unreviewed_host_validation_recipe,
                           row.selected_backend, row.feature, row.proof_id,
                           "Metal renderer parity host evidence must name a reviewed validation recipe id",
                           row.source_index);
        }
        if (row.request_native_handle_access) {
            add_diagnostic(plan, BackendRendererParityDiagnosticCode::unsupported_native_handle_claim,
                           row.selected_backend, row.feature, row.proof_id,
                           "backend renderer parity proof must not expose native renderer or RHI handles",
                           row.source_index);
        }
    }
}

[[nodiscard]] bool backend_all_features_ready(const BackendRendererParityPolicyPlan& plan,
                                              const rhi::BackendKind backend) {
    return std::ranges::all_of(plan.required_features, [&plan, backend](const auto feature) {
        const auto row_it = std::ranges::find_if(plan.proofs, [backend, feature](const auto& row) {
            return row.selected_backend == backend && row.feature == feature;
        });
        return row_it != plan.proofs.end() && is_ready_row(*row_it);
    });
}

void summarize(BackendRendererParityPolicyPlan& plan) {
    std::vector<rhi::BackendKind> ready_backends;
    for (const auto& row : plan.proofs) {
        if (is_ready_row(row)) {
            ++plan.ready_row_count;
        } else if (is_host_gate_row(row)) {
            ++plan.host_gated_row_count;
        }
    }

    for (const auto backend : plan.required_backends) {
        const auto backend_ready = backend_all_features_ready(plan, backend);
        if (backend_ready && !contains_backend(ready_backends, backend)) {
            ready_backends.push_back(backend);
        }
        if (backend == rhi::BackendKind::d3d12) {
            plan.d3d12_parity_ready = backend_ready;
        } else if (backend == rhi::BackendKind::vulkan) {
            plan.vulkan_parity_ready = backend_ready;
        } else if (backend == rhi::BackendKind::metal) {
            plan.metal_parity_ready = backend_ready;
        }
    }
    plan.host_validated_backend_count = ready_backends.size();
}

void compute_replay_hash(BackendRendererParityPolicyPlan& plan, const BackendRendererParityPolicyRequest& request) {
    if (!plan.diagnostics.empty() || plan.proofs.empty()) {
        return;
    }
    std::uint64_t hash = 1469598103934665603ULL ^ request.seed;
    for (const auto backend : plan.required_backends) {
        hash_mix(hash, static_cast<std::uint8_t>(backend));
    }
    for (const auto feature : plan.required_features) {
        hash_mix(hash, static_cast<std::uint8_t>(feature));
    }
    for (const auto& row : plan.proofs) {
        hash_mix_string(hash, row.proof_id);
        hash_mix(hash, static_cast<std::uint8_t>(row.feature));
        hash_mix(hash, static_cast<std::uint8_t>(row.selected_backend));
        hash_mix(hash, static_cast<std::uint8_t>(row.proof_backend));
        hash_mix(hash, row.reviewed ? 1U : 0U);
        hash_mix(hash, row.host_validated ? 1U : 0U);
        hash_mix(hash, row.host_gate_required ? 1U : 0U);
        hash_mix_string(hash, row.host_validation_recipe_id);
        hash_mix(hash, row.request_native_handle_access ? 1U : 0U);
        hash_mix_string(hash, row.package_counter_id);
        hash_mix(hash, row.source_index);
    }
    plan.replay_hash = hash;
}

} // namespace

bool BackendRendererParityPolicyPlan::succeeded() const noexcept {
    return status == BackendRendererParityPolicyStatus::ready && diagnostics.empty();
}

bool backend_renderer_parity_proof_matches_selected_backend(const BackendRendererParityProofDesc& desc) noexcept {
    return desc.selected_backend != rhi::BackendKind::null && desc.selected_backend == desc.proof_backend;
}

std::vector<BackendRendererParityProofRow> make_backend_renderer_parity_apple_metal_environment_proofs(
    const BackendRendererParityAppleMetalEnvironmentEvidenceDesc& desc) {
    if (desc.native_handle_access) {
        return {};
    }

    const auto ready = apple_metal_environment_evidence_complete(desc);
    std::vector<BackendRendererParityProofRow> proofs;
    proofs.reserve(std::size(kAppleMetalEnvironmentProofMappings));
    std::uint32_t source_index{1U};
    for (const auto& mapping : kAppleMetalEnvironmentProofMappings) {
        proofs.push_back(make_apple_metal_environment_proof_row(mapping, ready, source_index++));
    }
    return proofs;
}

std::vector<BackendRendererParityProofRow> make_backend_renderer_parity_apple_metal_memory_profiling_proofs(
    const BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc& desc) {
    if (desc.native_handle_access) {
        return {};
    }

    std::vector<BackendRendererParityProofRow> proofs;
    proofs.reserve(std::size(kAppleMetalMemoryProfilingProofMappings));
    proofs.push_back(make_apple_metal_memory_profiling_proof_row(
        kAppleMetalMemoryProfilingProofMappings[0], apple_metal_memory_residency_evidence_complete(desc), 1U));
    proofs.push_back(make_apple_metal_memory_profiling_proof_row(
        kAppleMetalMemoryProfilingProofMappings[1], apple_metal_profiling_capture_evidence_complete(desc), 2U));
    return proofs;
}

BackendRendererParityPolicyPlan plan_backend_renderer_parity_policy(const BackendRendererParityPolicyRequest& request) {
    BackendRendererParityPolicyPlan plan;
    plan.required_backends = request.required_backends;
    plan.required_features = request.required_features;
    plan.proofs = request.proofs;
    plan.row_count = request.proofs.size();
    sort_backends(plan.required_backends);
    sort_features(plan.required_features);
    sort_proofs(plan.proofs);

    if (request.required_backends.empty() || request.required_features.empty() || request.proofs.empty()) {
        plan.status = BackendRendererParityPolicyStatus::no_rows;
        return plan;
    }

    validate_budget(plan, request);
    validate_required_backends(plan, request);
    validate_required_features(plan, request);
    validate_duplicate_proofs(plan, request);
    validate_required_proofs(plan, request);
    validate_proof_rows(plan, request);
    sort_diagnostics(plan);
    summarize(plan);
    compute_replay_hash(plan, request);

    if (!plan.diagnostics.empty()) {
        plan.status = BackendRendererParityPolicyStatus::invalid_request;
        plan.replay_hash = 0U;
        return plan;
    }

    const auto all_required_ready = std::ranges::all_of(
        plan.required_backends, [&plan](const auto backend) { return backend_all_features_ready(plan, backend); });
    if (all_required_ready) {
        plan.status = BackendRendererParityPolicyStatus::ready;
    } else {
        plan.status = BackendRendererParityPolicyStatus::host_evidence_required;
    }
    return plan;
}

const char* backend_renderer_parity_diagnostic_message(const BackendRendererParityDiagnosticCode code) noexcept {
    switch (code) {
    case BackendRendererParityDiagnosticCode::none:
        return "none";
    case BackendRendererParityDiagnosticCode::invalid_required_backend:
        return "invalid_required_backend";
    case BackendRendererParityDiagnosticCode::duplicate_required_backend:
        return "duplicate_required_backend";
    case BackendRendererParityDiagnosticCode::invalid_required_feature:
        return "invalid_required_feature";
    case BackendRendererParityDiagnosticCode::duplicate_required_feature:
        return "duplicate_required_feature";
    case BackendRendererParityDiagnosticCode::unsupported_backend:
        return "unsupported_backend";
    case BackendRendererParityDiagnosticCode::missing_required_proof:
        return "missing_required_proof";
    case BackendRendererParityDiagnosticCode::duplicate_proof:
        return "duplicate_proof";
    case BackendRendererParityDiagnosticCode::invalid_proof:
        return "invalid_proof";
    case BackendRendererParityDiagnosticCode::cross_backend_proof_transfer:
        return "cross_backend_proof_transfer";
    case BackendRendererParityDiagnosticCode::missing_metal_host_evidence:
        return "missing_metal_host_evidence";
    case BackendRendererParityDiagnosticCode::missing_host_validation_recipe:
        return "missing_host_validation_recipe";
    case BackendRendererParityDiagnosticCode::unreviewed_host_validation_recipe:
        return "unreviewed_host_validation_recipe";
    case BackendRendererParityDiagnosticCode::unsupported_native_handle_claim:
        return "unsupported_native_handle_claim";
    case BackendRendererParityDiagnosticCode::row_budget_exceeded:
        return "row_budget_exceeded";
    }
    return "unknown";
}

} // namespace mirakana
