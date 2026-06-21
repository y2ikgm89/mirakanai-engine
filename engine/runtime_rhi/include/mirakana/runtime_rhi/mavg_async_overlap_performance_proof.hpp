// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/runtime_rhi/mavg_streaming_upload_overlap_evidence.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_rhi {

enum class RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_graph_asset,
    missing_overlap_evidence,
    overlap_evidence_not_ready,
    invalid_workload_id,
    missing_measurement_artifact_id,
    insufficient_serial_samples,
    insufficient_overlapped_samples,
    sample_workload_mismatch,
    sample_artifact_mismatch,
    sample_row_count_mismatch,
    invalid_sample_ticks,
    sample_missing_overlap,
    insufficient_speedup,
};

struct RuntimeMavgAsyncOverlapPerformanceProofDiagnostic {
    RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode code{
        RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::none};
    AssetId graph_asset;
    std::string message;
};

struct RuntimeMavgAsyncOverlapPerformanceSample {
    std::string_view workload_id;
    std::string_view measurement_artifact_id;
    std::size_t completed_row_count{0};
    std::uint64_t background_load_ticks{0};
    std::uint64_t gpu_upload_ticks{0};
    std::uint64_t total_ticks{0};
    std::uint64_t overlap_ticks{0};
};

struct RuntimeMavgAsyncOverlapPerformanceProofDesc {
    AssetId graph_asset;
    const RuntimeMavgStreamingUploadOverlapEvidenceResult* overlap_evidence{nullptr};
    std::string_view workload_id;
    std::string_view measurement_artifact_id;
    std::span<const RuntimeMavgAsyncOverlapPerformanceSample> serial_samples;
    std::span<const RuntimeMavgAsyncOverlapPerformanceSample> overlapped_samples;
    std::size_t minimum_sample_count{5};
    std::uint32_t minimum_speedup_basis_points{1000};
};

struct RuntimeMavgAsyncOverlapPerformanceProofResult {
    std::vector<RuntimeMavgAsyncOverlapPerformanceProofDiagnostic> diagnostics;
    std::string workload_id;
    std::string measurement_artifact_id;
    std::size_t sample_count{0};
    std::size_t serial_sample_count{0};
    std::size_t overlapped_sample_count{0};
    std::size_t completed_row_count{0};
    std::uint64_t serial_p95_tick_count{0};
    std::uint64_t overlapped_p95_tick_count{0};
    std::uint64_t overlap_tick_count{0};
    std::uint32_t speedup_basis_points{0};
    bool recorded_temporal_overlap_evidence{false};
    bool claimed_speedup{false};
    bool proved_async_overlap_performance{false};
    bool touched_native_handles{false};
    bool used_gpu_directstorage_destination{false};
    bool used_gdeflate{false};
    bool executed_mesh_shader{false};
    bool claimed_metal_readiness{false};
    bool claimed_nanite_equivalence{false};
    bool claimed_broad_optimization{false};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty() && proved_async_overlap_performance;
    }
};

/// Evaluates caller-supplied selected workload timing rows after MAVG overlap evidence exists. This helper validates
/// evidence rows and computes deterministic p95 serial/overlapped ticks; it does not collect live timings, expose
/// native handles, execute backend work, use GPU DirectStorage destinations, use GDeflate, run mesh shaders, or claim
/// broad optimization.
[[nodiscard]] RuntimeMavgAsyncOverlapPerformanceProofResult
prove_runtime_mavg_async_overlap_performance(const RuntimeMavgAsyncOverlapPerformanceProofDesc& desc);

[[nodiscard]] bool has_runtime_mavg_async_overlap_performance_proof_diagnostic(
    const RuntimeMavgAsyncOverlapPerformanceProofResult& result,
    RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode code) noexcept;

} // namespace mirakana::runtime_rhi
