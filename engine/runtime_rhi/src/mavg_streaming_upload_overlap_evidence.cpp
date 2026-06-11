// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_rhi/mavg_streaming_upload_overlap_evidence.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace mirakana::runtime_rhi {
namespace {

void add_diagnostic(RuntimeMavgStreamingUploadOverlapEvidenceResult& result,
                    RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode code, AssetId graph_asset,
                    std::string message) {
    result.diagnostics.push_back(RuntimeMavgStreamingUploadOverlapEvidenceDiagnostic{
        .code = code,
        .graph_asset = graph_asset,
        .message = std::move(message),
    });
}

[[nodiscard]] bool valid_window(const RuntimeMavgStreamingUploadEvidenceWindow& window) noexcept {
    return window.begin_tick < window.end_tick;
}

[[nodiscard]] bool valid_clock_domain(const RuntimeMavgStreamingUploadEvidenceWindow& window) noexcept {
    return window.clock_domain != RuntimeMavgStreamingUploadEvidenceClockDomain::none && window.timeline_id != 0U;
}

[[nodiscard]] bool same_clock_domain(const RuntimeMavgStreamingUploadEvidenceWindow& lhs,
                                     const RuntimeMavgStreamingUploadEvidenceWindow& rhs) noexcept {
    return lhs.clock_domain == rhs.clock_domain && lhs.timeline_id == rhs.timeline_id;
}

[[nodiscard]] std::uint64_t overlap_ticks(const RuntimeMavgStreamingUploadEvidenceWindow& lhs,
                                          const RuntimeMavgStreamingUploadEvidenceWindow& rhs) noexcept {
    const auto begin = std::max(lhs.begin_tick, rhs.begin_tick);
    const auto end = std::min(lhs.end_tick, rhs.end_tick);
    if (begin >= end) {
        return 0;
    }
    return end - begin;
}

} // namespace

RuntimeMavgStreamingUploadOverlapEvidenceResult
plan_runtime_mavg_streaming_upload_overlap_evidence(const RuntimeMavgStreamingUploadOverlapEvidenceDesc& desc) {
    RuntimeMavgStreamingUploadOverlapEvidenceResult result;
    const bool missing_required_input =
        desc.closeout == nullptr || desc.safe_point_adoption == nullptr || desc.gpu_upload == nullptr;

    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::invalid_graph_asset,
                       desc.graph_asset, "MAVG streaming upload overlap evidence graph asset id must be non-zero");
    }
    if (desc.closeout == nullptr) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_closeout,
                       desc.graph_asset, "MAVG streaming upload overlap evidence requires a closeout result");
    }
    if (desc.safe_point_adoption == nullptr) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_safe_point_adoption,
                       desc.graph_asset,
                       "MAVG streaming upload overlap evidence requires safe-point adoption evidence");
    }
    if (desc.gpu_upload == nullptr) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_gpu_upload,
                       desc.graph_asset,
                       "MAVG streaming upload overlap evidence requires streamed GPU upload evidence");
    }
    if (!valid_window(desc.background_load_window)) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::invalid_background_window,
                       desc.graph_asset, "MAVG background load evidence window must have begin_tick < end_tick");
    }
    if (!valid_window(desc.gpu_upload_window)) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::invalid_gpu_upload_window,
                       desc.graph_asset, "MAVG GPU upload evidence window must have begin_tick < end_tick");
    }
    if (!valid_clock_domain(desc.background_load_window) || !valid_clock_domain(desc.gpu_upload_window)) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::invalid_window_clock_domain,
                       desc.graph_asset,
                       "MAVG streaming/upload evidence windows require a non-zero shared clock domain");
    } else if (!same_clock_domain(desc.background_load_window, desc.gpu_upload_window)) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::window_clock_domain_mismatch,
                       desc.graph_asset,
                       "MAVG streaming/upload evidence windows must use the same clock domain and timeline id");
    }
    if (desc.require_measured_budget_evidence && !desc.measured_budget_evidence_ready) {
        add_diagnostic(result,
                       RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_measured_budget_evidence,
                       desc.graph_asset,
                       "MAVG streaming upload overlap evidence cannot claim measured speedup without budget evidence");
    }
    if (missing_required_input || !result.diagnostics.empty()) {
        return result;
    }

    const auto& closeout = *desc.closeout;
    const auto& adoption = *desc.safe_point_adoption;
    const auto& upload = *desc.gpu_upload;

    result.background_loaded_row_count = closeout.background_loaded_row_count;
    result.adopted_page_count = adoption.adopted_page_count;
    result.uploaded_page_count = upload.uploaded_page_count;
    result.uploaded_cluster_count = upload.uploaded_cluster_count;
    result.uploaded_bytes = upload.uploaded_bytes;
    result.background_load_tick_count = desc.background_load_window.duration_ticks();
    result.gpu_upload_tick_count = desc.gpu_upload_window.duration_ticks();
    result.executed_background_worker =
        closeout.executed_background_worker || closeout.background_load.executed_background_worker;
    result.invoked_gpu_upload = upload.invoked_gpu_upload;

    if (!closeout.succeeded()) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::closeout_failed,
                       desc.graph_asset, "MAVG streaming upload overlap evidence requires a successful closeout");
    }
    if (closeout.background_loaded_row_count == 0U || closeout.background_load.loaded_rows.empty() ||
        !closeout.background_load.succeeded()) {
        add_diagnostic(
            result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::closeout_missing_background_load_evidence,
            desc.graph_asset, "MAVG streaming upload overlap evidence requires successful background loaded rows");
    }
    if (!adoption.succeeded()) {
        add_diagnostic(
            result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::safe_point_adoption_not_committed,
            desc.graph_asset, "MAVG streaming upload overlap evidence requires committed safe-point adoption");
    }
    if (!upload.succeeded()) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::gpu_upload_not_ready,
                       desc.graph_asset,
                       "MAVG streaming upload overlap evidence requires ready streamed GPU upload rows");
    }
    if (adoption.adopted_rows.empty() || upload.page_bindings.empty()) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_source_row_evidence,
                       desc.graph_asset,
                       "MAVG streaming upload overlap evidence requires adoption and upload source rows");
    }
    const auto source_graph_matches =
        std::ranges::all_of(closeout.background_load.loaded_rows,
                            [&desc](const runtime::RuntimeMavgPageStreamingBackgroundLoadedRow& row) {
                                return row.row.graph_asset == desc.graph_asset;
                            }) &&
        std::ranges::all_of(adoption.adopted_rows,
                            [&desc](const RuntimeMavgClusterStreamingSafePointAdoptionRow& row) {
                                return row.graph_asset == desc.graph_asset;
                            }) &&
        std::ranges::all_of(upload.page_bindings, [&desc](const RuntimeMavgStreamedClusterPageBindingRow& row) {
            return row.graph_asset == desc.graph_asset;
        });
    if (!source_graph_matches) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::source_row_graph_asset_mismatch,
                       desc.graph_asset,
                       "MAVG streaming upload overlap evidence source rows must match the requested graph asset");
    }
    if (desc.background_load_window.completed_row_count == 0U || desc.gpu_upload_window.completed_row_count == 0U ||
        desc.background_load_window.completed_row_count != closeout.background_load.loaded_rows.size() ||
        desc.gpu_upload_window.completed_row_count != upload.page_bindings.size()) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::missing_window_row_evidence,
                       desc.graph_asset, "MAVG streaming/upload evidence windows require completed row counters");
    }

    result.overlap_tick_count = overlap_ticks(desc.background_load_window, desc.gpu_upload_window);
    if (result.overlap_tick_count == 0U) {
        add_diagnostic(result, RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode::no_temporal_overlap,
                       desc.graph_asset,
                       "MAVG streaming upload overlap evidence requires intersecting background and upload windows");
    }
    if (!result.diagnostics.empty()) {
        return result;
    }

    result.recorded_temporal_overlap_evidence = true;
    return result;
}

bool has_runtime_mavg_streaming_upload_overlap_evidence_diagnostic(
    const RuntimeMavgStreamingUploadOverlapEvidenceResult& result,
    RuntimeMavgStreamingUploadOverlapEvidenceDiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics,
                               [code](const RuntimeMavgStreamingUploadOverlapEvidenceDiagnostic& diagnostic) {
                                   return diagnostic.code == code;
                               });
}

} // namespace mirakana::runtime_rhi
