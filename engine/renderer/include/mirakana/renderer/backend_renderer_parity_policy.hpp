// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class BackendRendererParityPolicyStatus : std::uint8_t {
    ready = 0,
    host_evidence_required,
    no_rows,
    invalid_request,
};

enum class BackendRendererParityFeatureKind : std::uint8_t {
    synchronization = 0,
    shader_validation,
    memory_residency,
    profiling_capture,
    package_evidence,
};

enum class BackendRendererParityDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_required_backend,
    duplicate_required_backend,
    invalid_required_feature,
    duplicate_required_feature,
    unsupported_backend,
    missing_required_proof,
    duplicate_proof,
    invalid_proof,
    cross_backend_proof_transfer,
    missing_metal_host_evidence,
    missing_host_validation_recipe,
    unreviewed_host_validation_recipe,
    unsupported_native_handle_claim,
    row_budget_exceeded,
};

struct BackendRendererParityProofDesc {
    rhi::BackendKind selected_backend{rhi::BackendKind::null};
    rhi::BackendKind proof_backend{rhi::BackendKind::null};
};

struct BackendRendererParityProofRow {
    std::string proof_id;
    BackendRendererParityFeatureKind feature{BackendRendererParityFeatureKind::synchronization};
    rhi::BackendKind selected_backend{rhi::BackendKind::null};
    rhi::BackendKind proof_backend{rhi::BackendKind::null};
    bool reviewed{false};
    bool host_validated{false};
    bool host_gate_required{false};
    std::string host_validation_recipe_id;
    bool request_native_handle_access{false};
    std::string package_counter_id;
    std::uint32_t source_index{0U};
};

struct BackendRendererParityAppleMetalEnvironmentEvidenceDesc {
    bool runtime_ready{false};
    bool command_queue_ready{false};
    bool shader_library_ready{false};
    bool render_pipeline_ready{false};
    bool compute_pipeline_ready{false};
    bool render_pass_ready{false};
    bool resource_evidence_ready{false};
    bool synchronization_evidence_ready{false};
    bool package_evidence_ready{false};
    bool render_readback_nonzero{false};
    bool compute_readback_nonzero{false};
    bool native_handle_access{false};
    std::string host_validation_recipe_id{"renderer-metal-apple-host-evidence"};
};

struct BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc {
    bool runtime_ready{false};
    bool command_queue_ready{false};
    bool heap_allocation_ready{false};
    bool residency_set_ready{false};
    bool residency_commit_ready{false};
    bool residency_pressure_evidence_ready{false};
    bool capture_manager_ready{false};
    bool capture_scope_ready{false};
    bool capture_boundary_ready{false};
    bool capture_artifact_ready{false};
    bool native_handle_access{false};
    std::string host_validation_recipe_id{"renderer-metal-apple-host-evidence"};
};

struct BackendRendererParityPolicyRequest {
    std::vector<rhi::BackendKind> required_backends;
    std::vector<BackendRendererParityFeatureKind> required_features;
    std::vector<BackendRendererParityProofRow> proofs;
    std::size_t row_budget{128U};
    std::uint64_t seed{0U};
};

struct BackendRendererParityDiagnostic {
    BackendRendererParityDiagnosticCode code{BackendRendererParityDiagnosticCode::none};
    rhi::BackendKind backend{rhi::BackendKind::null};
    BackendRendererParityFeatureKind feature{BackendRendererParityFeatureKind::synchronization};
    std::string row_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct BackendRendererParityPolicyPlan {
    BackendRendererParityPolicyStatus status{BackendRendererParityPolicyStatus::invalid_request};
    std::vector<BackendRendererParityDiagnostic> diagnostics;
    std::vector<rhi::BackendKind> required_backends;
    std::vector<BackendRendererParityFeatureKind> required_features;
    std::vector<BackendRendererParityProofRow> proofs;
    std::size_t row_count{0U};
    std::size_t ready_row_count{0U};
    std::size_t host_gated_row_count{0U};
    std::size_t host_validated_backend_count{0U};
    std::uint64_t replay_hash{0U};
    bool d3d12_parity_ready{false};
    bool vulkan_parity_ready{false};
    bool metal_parity_ready{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] bool
backend_renderer_parity_proof_matches_selected_backend(const BackendRendererParityProofDesc& desc) noexcept;

[[nodiscard]] std::vector<BackendRendererParityProofRow> make_backend_renderer_parity_apple_metal_environment_proofs(
    const BackendRendererParityAppleMetalEnvironmentEvidenceDesc& desc);

[[nodiscard]] std::vector<BackendRendererParityProofRow>
make_backend_renderer_parity_apple_metal_memory_profiling_proofs(
    const BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc& desc);

[[nodiscard]] BackendRendererParityPolicyPlan
plan_backend_renderer_parity_policy(const BackendRendererParityPolicyRequest& request);

[[nodiscard]] const char* backend_renderer_parity_diagnostic_message(BackendRendererParityDiagnosticCode code) noexcept;

} // namespace mirakana
