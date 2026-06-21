// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_async_overlap_performance_proof.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgAsyncOverlapPerformanceProofResult& result,
                    RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode code, AssetId graph_asset,
                    std::string message) {
    result.diagnostics.push_back(RuntimeMavgAsyncOverlapPerformanceProofDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .message = std::move(message),
    });
}

[[nodiscard]] bool blank(std::string_view value) noexcept {
    return value.empty();
}

[[nodiscard]] std::uint64_t p95_ticks(std::span<const RuntimeMavgAsyncOverlapPerformanceSample> samples) {
    std::vector<std::uint64_t> ticks;
    ticks.reserve(samples.size());
    for (const auto& sample : samples) {
        ticks.push_back(sample.total_ticks);
    }
    std::ranges::sort(ticks);
    const auto index = ticks.empty() ? 0U : ((ticks.size() * 95U) + 99U) / 100U - 1U;
    return ticks[index];
}

[[nodiscard]] std::uint32_t speedup_basis_points(std::uint64_t serial_ticks, std::uint64_t overlapped_ticks) noexcept {
    if (serial_ticks == 0U || overlapped_ticks >= serial_ticks) {
        return 0U;
    }
    return static_cast<std::uint32_t>(((serial_ticks - overlapped_ticks) * 10000U) / serial_ticks);
}

void validate_samples(RuntimeMavgAsyncOverlapPerformanceProofResult& result,
                      const RuntimeMavgAsyncOverlapPerformanceProofDesc& desc,
                      std::span<const RuntimeMavgAsyncOverlapPerformanceSample> samples, bool require_overlap) {
    const auto expected_rows = desc.overlap_evidence == nullptr ? 0U : desc.overlap_evidence->uploaded_page_count;
    for (const auto& sample : samples) {
        if (sample.workload_id != desc.workload_id) {
            add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_workload_mismatch,
                           desc.graph_asset, "MAVG async overlap performance sample workload id must match");
        }
        if (sample.measurement_artifact_id != desc.measurement_artifact_id) {
            add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_artifact_mismatch,
                           desc.graph_asset, "MAVG async overlap performance sample artifact id must match");
        }
        if (sample.completed_row_count == 0U || (expected_rows != 0U && sample.completed_row_count != expected_rows)) {
            add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_row_count_mismatch,
                           desc.graph_asset,
                           "MAVG async overlap performance sample completed row count must match overlap evidence");
        }
        if (sample.background_load_ticks == 0U || sample.gpu_upload_ticks == 0U || sample.total_ticks == 0U ||
            sample.overlap_ticks > sample.background_load_ticks || sample.overlap_ticks > sample.gpu_upload_ticks ||
            sample.overlap_ticks >= sample.total_ticks) {
            add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::invalid_sample_ticks,
                           desc.graph_asset,
                           "MAVG async overlap performance sample ticks must be positive and bounded");
        }
        if (require_overlap && sample.overlap_ticks == 0U) {
            add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_missing_overlap,
                           desc.graph_asset, "MAVG overlapped performance samples must include positive overlap ticks");
        }
        if (!require_overlap && sample.overlap_ticks != 0U) {
            add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::sample_missing_overlap,
                           desc.graph_asset, "MAVG serial performance samples must not include overlap ticks");
        }
    }
}

} // namespace

RuntimeMavgAsyncOverlapPerformanceProofResult
prove_runtime_mavg_async_overlap_performance(const RuntimeMavgAsyncOverlapPerformanceProofDesc& desc) {
    RuntimeMavgAsyncOverlapPerformanceProofResult result;
    result.workload_id = std::string(desc.workload_id);
    result.measurement_artifact_id = std::string(desc.measurement_artifact_id);
    result.sample_count = desc.serial_samples.size() + desc.overlapped_samples.size();
    result.serial_sample_count = desc.serial_samples.size();
    result.overlapped_sample_count = desc.overlapped_samples.size();

    if (desc.graph_asset.value == 0U) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::invalid_graph_asset,
                       desc.graph_asset, "MAVG async overlap performance proof graph asset id must be non-zero");
    }
    if (desc.overlap_evidence == nullptr) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::missing_overlap_evidence,
                       desc.graph_asset, "MAVG async overlap performance proof requires overlap evidence");
    } else {
        result.recorded_temporal_overlap_evidence = desc.overlap_evidence->recorded_temporal_overlap_evidence;
        result.overlap_tick_count = desc.overlap_evidence->overlap_tick_count;
        result.completed_row_count = desc.overlap_evidence->uploaded_page_count;
        if (!desc.overlap_evidence->succeeded() || desc.overlap_evidence->graph_asset != desc.graph_asset ||
            desc.overlap_evidence->timeline_id == 0U) {
            add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::overlap_evidence_not_ready,
                           desc.graph_asset,
                           "MAVG async overlap performance proof requires ready overlap evidence for the same graph");
        }
    }
    if (blank(desc.workload_id)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::invalid_workload_id,
                       desc.graph_asset, "MAVG async overlap performance proof workload id must be non-empty");
    }
    if (blank(desc.measurement_artifact_id)) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::missing_measurement_artifact_id,
                       desc.graph_asset,
                       "MAVG async overlap performance proof measurement artifact id must be non-empty");
    }
    if (desc.serial_samples.size() < desc.minimum_sample_count) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::insufficient_serial_samples,
                       desc.graph_asset, "MAVG async overlap performance proof requires enough serial samples");
    }
    if (desc.overlapped_samples.size() < desc.minimum_sample_count) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::insufficient_overlapped_samples,
                       desc.graph_asset, "MAVG async overlap performance proof requires enough overlapped samples");
    }

    validate_samples(result, desc, desc.serial_samples, false);
    validate_samples(result, desc, desc.overlapped_samples, true);
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.serial_p95_tick_count = p95_ticks(desc.serial_samples);
    result.overlapped_p95_tick_count = p95_ticks(desc.overlapped_samples);
    result.speedup_basis_points = speedup_basis_points(result.serial_p95_tick_count, result.overlapped_p95_tick_count);
    if (result.speedup_basis_points == 0U || result.speedup_basis_points < desc.minimum_speedup_basis_points) {
        add_diagnostic(result, RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode::insufficient_speedup,
                       desc.graph_asset,
                       "MAVG async overlap performance proof requires selected p95 speedup over threshold");
        return result;
    }

    result.claimed_speedup = true;
    result.proved_async_overlap_performance = true;
    return result;
}

bool has_runtime_mavg_async_overlap_performance_proof_diagnostic(
    const RuntimeMavgAsyncOverlapPerformanceProofResult& result,
    RuntimeMavgAsyncOverlapPerformanceProofDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [code](const RuntimeMavgAsyncOverlapPerformanceProofDiagnostic& diagnostic) {
                                   return diagnostic.code == code;
                               });
}

} // namespace mirakana::runtime_rhi
