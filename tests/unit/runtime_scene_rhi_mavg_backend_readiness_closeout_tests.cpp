// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_scene_rhi/mavg_backend_readiness_closeout.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace {

using mirakana::runtime_scene_rhi::MavgBackendReadinessDiagnosticCode;
using mirakana::runtime_scene_rhi::MavgBackendReadinessEvidenceKind;
using mirakana::runtime_scene_rhi::MavgBackendReadinessEvidenceRow;
using mirakana::runtime_scene_rhi::MavgBackendReadinessEvidenceStatus;

constexpr std::array<MavgBackendReadinessEvidenceKind, 9> kRequiredRows{
    MavgBackendReadinessEvidenceKind::graph_cook_package,
    MavgBackendReadinessEvidenceKind::runtime_lod_selection,
    MavgBackendReadinessEvidenceKind::resident_page,
    MavgBackendReadinessEvidenceKind::safe_point_adoption,
    MavgBackendReadinessEvidenceKind::streamed_gpu_upload,
    MavgBackendReadinessEvidenceKind::streamed_backend_draw,
    MavgBackendReadinessEvidenceKind::d3d12_compute_generated_indirect_consumption,
    MavgBackendReadinessEvidenceKind::vulkan_compute_generated_indirect_consumption,
    MavgBackendReadinessEvidenceKind::package_smoke_counter,
};

[[nodiscard]] constexpr bool requires_execution(const MavgBackendReadinessEvidenceKind kind) noexcept {
    return kind == MavgBackendReadinessEvidenceKind::safe_point_adoption ||
           kind == MavgBackendReadinessEvidenceKind::streamed_gpu_upload ||
           kind == MavgBackendReadinessEvidenceKind::streamed_backend_draw ||
           kind == MavgBackendReadinessEvidenceKind::d3d12_compute_generated_indirect_consumption ||
           kind == MavgBackendReadinessEvidenceKind::vulkan_compute_generated_indirect_consumption;
}

[[nodiscard]] constexpr std::string_view row_id_for_kind(const MavgBackendReadinessEvidenceKind kind) noexcept {
    switch (kind) {
    case MavgBackendReadinessEvidenceKind::graph_cook_package:
        return "mavg.backend.graph_cook_package";
    case MavgBackendReadinessEvidenceKind::runtime_lod_selection:
        return "mavg.backend.runtime_lod_selection";
    case MavgBackendReadinessEvidenceKind::resident_page:
        return "mavg.backend.resident_page";
    case MavgBackendReadinessEvidenceKind::safe_point_adoption:
        return "mavg.backend.safe_point_adoption";
    case MavgBackendReadinessEvidenceKind::streamed_gpu_upload:
        return "mavg.backend.streamed_gpu_upload";
    case MavgBackendReadinessEvidenceKind::streamed_backend_draw:
        return "mavg.backend.streamed_backend_draw";
    case MavgBackendReadinessEvidenceKind::d3d12_compute_generated_indirect_consumption:
        return "mavg.backend.d3d12_compute_generated_indirect_consumption";
    case MavgBackendReadinessEvidenceKind::vulkan_compute_generated_indirect_consumption:
        return "mavg.backend.vulkan_compute_generated_indirect_consumption";
    case MavgBackendReadinessEvidenceKind::package_smoke_counter:
        return "mavg.backend.package_smoke_counter";
    case MavgBackendReadinessEvidenceKind::metal_host_gate:
        return "mavg.backend.metal_host_gate";
    }
    return "mavg.backend.unknown";
}

[[nodiscard]] MavgBackendReadinessEvidenceRow make_ready_row(const MavgBackendReadinessEvidenceKind kind) {
    return MavgBackendReadinessEvidenceRow{
        .kind = kind,
        .row_id = row_id_for_kind(kind),
        .status = MavgBackendReadinessEvidenceStatus::ready,
        .package_visible = true,
        .execution_evidence = requires_execution(kind),
    };
}

[[nodiscard]] std::vector<MavgBackendReadinessEvidenceRow> make_ready_rows() {
    std::vector<MavgBackendReadinessEvidenceRow> rows;
    rows.reserve(kRequiredRows.size());
    for (const auto kind : kRequiredRows) {
        rows.push_back(make_ready_row(kind));
    }
    return rows;
}

[[nodiscard]] auto evaluate(std::span<const MavgBackendReadinessEvidenceRow> rows) {
    return mirakana::runtime_scene_rhi::evaluate_mavg_backend_readiness_closeout(
        mirakana::runtime_scene_rhi::MavgBackendReadinessCloseoutDesc{.rows = rows});
}

[[nodiscard]] bool has_code(const mirakana::runtime_scene_rhi::MavgBackendReadinessCloseoutResult& result,
                            const MavgBackendReadinessDiagnosticCode code) {
    return mirakana::runtime_scene_rhi::has_mavg_backend_readiness_closeout_diagnostic(result, code);
}

[[nodiscard]] bool has_row_code(const mirakana::runtime_scene_rhi::MavgBackendReadinessCloseoutResult& result,
                                const MavgBackendReadinessEvidenceKind kind,
                                const MavgBackendReadinessDiagnosticCode code) {
    return mirakana::runtime_scene_rhi::has_mavg_backend_readiness_closeout_row_diagnostic(result,
                                                                                           row_id_for_kind(kind), code);
}

} // namespace

MK_TEST("runtime scene rhi mavg backend readiness fails closed when evidence rows are empty") {
    const auto result = evaluate(std::span<const MavgBackendReadinessEvidenceRow>{});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_backend_readiness_ready);
    MK_REQUIRE(result.required_row_count == kRequiredRows.size());
    MK_REQUIRE(result.ready_required_row_count == 0U);
    MK_REQUIRE(result.missing_required_row_count == kRequiredRows.size());
    MK_REQUIRE(has_code(result, MavgBackendReadinessDiagnosticCode::missing_required_row));
}

MK_TEST("runtime scene rhi mavg backend readiness succeeds only with every required package-visible row") {
    const auto rows = make_ready_rows();
    const auto result = evaluate(rows);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(!result.mavg_broad_backend_readiness_ready);
    MK_REQUIRE(!result.metal_inference_used);
    MK_REQUIRE(!result.native_handles_exposed);
    MK_REQUIRE(result.required_row_count == kRequiredRows.size());
    MK_REQUIRE(result.ready_required_row_count == kRequiredRows.size());
    MK_REQUIRE(result.missing_required_row_count == 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime scene rhi mavg backend readiness rejects missing vulkan row despite metal host gate") {
    auto rows = make_ready_rows();
    rows.erase(
        std::remove_if(rows.begin(), rows.end(),
                       [](const MavgBackendReadinessEvidenceRow& row) {
                           return row.kind ==
                                  MavgBackendReadinessEvidenceKind::vulkan_compute_generated_indirect_consumption;
                       }),
        rows.end());
    rows.push_back(MavgBackendReadinessEvidenceRow{
        .kind = MavgBackendReadinessEvidenceKind::metal_host_gate,
        .row_id = "mavg.backend.metal_host_gate",
        .status = MavgBackendReadinessEvidenceStatus::host_gated,
        .package_visible = true,
    });

    const auto result = mirakana::runtime_scene_rhi::evaluate_mavg_backend_readiness_closeout(
        mirakana::runtime_scene_rhi::MavgBackendReadinessCloseoutDesc{.rows = rows, .request_metal_inference = true});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(result.metal_host_gated_row_count == 1U);
    MK_REQUIRE(!result.metal_inference_used);
    MK_REQUIRE(has_code(result, MavgBackendReadinessDiagnosticCode::metal_inference_rejected));
    MK_REQUIRE(has_row_code(result, MavgBackendReadinessEvidenceKind::vulkan_compute_generated_indirect_consumption,
                            MavgBackendReadinessDiagnosticCode::missing_required_row));
}

MK_TEST("runtime scene rhi mavg backend readiness rejects value-only rows where execution is required") {
    auto rows = make_ready_rows();
    auto& streamed_draw = *std::find_if(rows.begin(), rows.end(), [](const MavgBackendReadinessEvidenceRow& row) {
        return row.kind == MavgBackendReadinessEvidenceKind::streamed_backend_draw;
    });
    streamed_draw.value_only = true;
    streamed_draw.execution_evidence = false;

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(has_row_code(result, MavgBackendReadinessEvidenceKind::streamed_backend_draw,
                            MavgBackendReadinessDiagnosticCode::execution_evidence_required));
}

MK_TEST("runtime scene rhi mavg backend readiness rejects native handle exposure") {
    auto rows = make_ready_rows();
    auto& upload = *std::find_if(rows.begin(), rows.end(), [](const MavgBackendReadinessEvidenceRow& row) {
        return row.kind == MavgBackendReadinessEvidenceKind::streamed_gpu_upload;
    });
    upload.touched_native_handles = true;

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(result.native_handles_exposed);
    MK_REQUIRE(has_row_code(result, MavgBackendReadinessEvidenceKind::streamed_gpu_upload,
                            MavgBackendReadinessDiagnosticCode::native_handle_access));
}

MK_TEST("runtime scene rhi mavg backend readiness rejects package smoke counters that are not ready") {
    auto rows = make_ready_rows();
    auto& package_smoke = *std::find_if(rows.begin(), rows.end(), [](const MavgBackendReadinessEvidenceRow& row) {
        return row.kind == MavgBackendReadinessEvidenceKind::package_smoke_counter;
    });
    package_smoke.status = MavgBackendReadinessEvidenceStatus::blocked;

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(has_row_code(result, MavgBackendReadinessEvidenceKind::package_smoke_counter,
                            MavgBackendReadinessDiagnosticCode::row_not_ready));
}

MK_TEST("runtime scene rhi mavg backend readiness rejects duplicate required rows") {
    auto rows = make_ready_rows();
    rows.push_back(make_ready_row(MavgBackendReadinessEvidenceKind::runtime_lod_selection));

    const auto result = evaluate(rows);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.mavg_package_visible_backend_readiness_ready);
    MK_REQUIRE(has_row_code(result, MavgBackendReadinessEvidenceKind::runtime_lod_selection,
                            MavgBackendReadinessDiagnosticCode::duplicate_required_row));
}

int main() {
    return mirakana::test::run_all();
}
