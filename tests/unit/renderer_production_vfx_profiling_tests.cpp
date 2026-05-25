// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/production_vfx_profiling.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::RendererProductionVfxFeatureKind;
using mirakana::RendererProductionVfxProfilingDiagnosticCode;
using mirakana::RendererProductionVfxProfilingStatus;

[[nodiscard]] mirakana::RendererProductionVfxFeatureRow make_feature(std::string feature_id,
                                                                     RendererProductionVfxFeatureKind kind,
                                                                     mirakana::rhi::BackendKind backend,
                                                                     std::uint32_t source_index) {
    return mirakana::RendererProductionVfxFeatureRow{
        .feature_id = std::move(feature_id),
        .kind = kind,
        .backend = backend,
        .reviewed = true,
        .request_broad_performance_claim = false,
        .request_native_handle_access = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererProductionGpuParticleBudgetRow
make_particle_budget(std::string effect_id, mirakana::rhi::BackendKind backend, std::uint32_t source_index) {
    return mirakana::RendererProductionGpuParticleBudgetRow{
        .effect_id = std::move(effect_id),
        .backend = backend,
        .max_particles = 4096U,
        .max_emitters = 16U,
        .max_spawn_per_frame = 256U,
        .simulation_budget_us = 700U,
        .submission_budget_us = 300U,
        .requires_compute_simulation = true,
        .requires_gpu_sort = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererProductionPostprocessRow
make_postprocess(std::string chain_id, mirakana::rhi::BackendKind backend, std::uint32_t source_index) {
    return mirakana::RendererProductionPostprocessRow{
        .chain_id = std::move(chain_id),
        .backend = backend,
        .pass_count = 3U,
        .scene_color_available = true,
        .scene_depth_available = true,
        .hdr_available = true,
        .history_available = true,
        .backend_shader_evidence_ready = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererProductionBackendTimingRow
make_timing(mirakana::rhi::BackendKind backend, bool host_validated, std::uint32_t source_index) {
    return mirakana::RendererProductionBackendTimingRow{
        .backend = backend,
        .profile_zone_id = "frame.main",
        .gpu_timestamp_frequency_hz = 1000000000ULL,
        .begin_tick = 1000ULL,
        .end_tick = 1900ULL,
        .calibrated_cpu_begin_tick = 2000ULL,
        .calibrated_cpu_end_tick = 2900ULL,
        .max_clock_deviation_ns = 250ULL,
        .debug_scope_count = 2U,
        .debug_marker_count = 1U,
        .host_validated = host_validated,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererProductionCrashTelemetryHandoffRow
make_crash_handoff(std::string handoff_id, mirakana::rhi::BackendKind backend, std::uint32_t source_index) {
    return mirakana::RendererProductionCrashTelemetryHandoffRow{
        .handoff_id = std::move(handoff_id),
        .backend = backend,
        .trace_event_count = 8U,
        .crash_dump_reviewed = true,
        .symbolication_ready = true,
        .telemetry_schema_reviewed = true,
        .operator_handoff_ready = true,
        .request_crash_upload = false,
        .request_native_capture = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererProductionVfxProfilingRequest make_valid_request(bool include_metal_host_evidence) {
    return mirakana::RendererProductionVfxProfilingRequest{
        .required_backends =
            {
                mirakana::rhi::BackendKind::d3d12,
                mirakana::rhi::BackendKind::vulkan,
                mirakana::rhi::BackendKind::metal,
            },
        .feature_rows =
            {
                make_feature("particles.spark", RendererProductionVfxFeatureKind::gpu_particles,
                             mirakana::rhi::BackendKind::d3d12, 1U),
                make_feature("particles.spark", RendererProductionVfxFeatureKind::gpu_particles,
                             mirakana::rhi::BackendKind::vulkan, 2U),
                make_feature("particles.spark", RendererProductionVfxFeatureKind::gpu_particles,
                             mirakana::rhi::BackendKind::metal, 3U),
            },
        .gpu_particle_budget_rows =
            {
                make_particle_budget("particles.spark", mirakana::rhi::BackendKind::d3d12, 4U),
                make_particle_budget("particles.spark", mirakana::rhi::BackendKind::vulkan, 5U),
                make_particle_budget("particles.spark", mirakana::rhi::BackendKind::metal, 6U),
            },
        .postprocess_rows =
            {
                make_postprocess("post.fx", mirakana::rhi::BackendKind::d3d12, 7U),
                make_postprocess("post.fx", mirakana::rhi::BackendKind::vulkan, 8U),
                make_postprocess("post.fx", mirakana::rhi::BackendKind::metal, 9U),
            },
        .backend_timing_rows =
            {
                make_timing(mirakana::rhi::BackendKind::d3d12, true, 10U),
                make_timing(mirakana::rhi::BackendKind::vulkan, true, 11U),
                make_timing(mirakana::rhi::BackendKind::metal, include_metal_host_evidence, 12U),
            },
        .crash_telemetry_handoff_rows =
            {
                make_crash_handoff("crash.d3d12", mirakana::rhi::BackendKind::d3d12, 13U),
                make_crash_handoff("crash.vulkan", mirakana::rhi::BackendKind::vulkan, 14U),
                make_crash_handoff("crash.metal", mirakana::rhi::BackendKind::metal, 15U),
            },
        .row_budget = 32U,
        .seed = 77U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::RendererProductionVfxProfilingPlan& plan,
                                           RendererProductionVfxProfilingDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("production renderer VFX profiling keeps Metal host evidence gated") {
    const auto plan = mirakana::plan_renderer_production_vfx_profiling(make_valid_request(false));

    MK_REQUIRE(plan.status == RendererProductionVfxProfilingStatus::host_evidence_required);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.feature_rows.size() == 3U);
    MK_REQUIRE(plan.gpu_particle_budget_rows.size() == 3U);
    MK_REQUIRE(plan.postprocess_rows.size() == 3U);
    MK_REQUIRE(plan.backend_timing_rows.size() == 3U);
    MK_REQUIRE(plan.crash_telemetry_handoff_rows.size() == 3U);
    MK_REQUIRE(plan.feature_row_count == 3U);
    MK_REQUIRE(plan.gpu_particle_budget_row_count == 3U);
    MK_REQUIRE(plan.postprocess_row_count == 3U);
    MK_REQUIRE(plan.backend_timing_row_count == 3U);
    MK_REQUIRE(plan.crash_telemetry_handoff_row_count == 3U);
    MK_REQUIRE(plan.host_validated_backend_count == 2U);
    MK_REQUIRE(plan.requires_metal_host_evidence);
    MK_REQUIRE(!plan.has_metal_host_evidence);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_gpu_commands);
    MK_REQUIRE(!plan.invoked_native_capture);
    MK_REQUIRE(!plan.invoked_crash_upload);
}

MK_TEST("production renderer VFX profiling is ready with per-backend host timing evidence") {
    const auto plan = mirakana::plan_renderer_production_vfx_profiling(make_valid_request(true));

    MK_REQUIRE(plan.status == RendererProductionVfxProfilingStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.host_validated_backend_count == 3U);
    MK_REQUIRE(plan.requires_metal_host_evidence);
    MK_REQUIRE(plan.has_metal_host_evidence);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("production renderer VFX profiling replay hash includes accepted row details") {
    auto base_request = make_valid_request(true);
    const auto base_plan = mirakana::plan_renderer_production_vfx_profiling(base_request);

    auto particle_variant = base_request;
    particle_variant.gpu_particle_budget_rows[0].requires_gpu_sort =
        !particle_variant.gpu_particle_budget_rows[0].requires_gpu_sort;
    const auto particle_plan = mirakana::plan_renderer_production_vfx_profiling(particle_variant);

    auto timing_variant = base_request;
    timing_variant.backend_timing_rows[1].calibrated_cpu_end_tick += 1U;
    timing_variant.backend_timing_rows[1].max_clock_deviation_ns += 1U;
    const auto timing_plan = mirakana::plan_renderer_production_vfx_profiling(timing_variant);

    MK_REQUIRE(base_plan.status == RendererProductionVfxProfilingStatus::ready);
    MK_REQUIRE(particle_plan.status == RendererProductionVfxProfilingStatus::ready);
    MK_REQUIRE(timing_plan.status == RendererProductionVfxProfilingStatus::ready);
    MK_REQUIRE(base_plan.replay_hash != 0U);
    MK_REQUIRE(particle_plan.replay_hash != base_plan.replay_hash);
    MK_REQUIRE(timing_plan.replay_hash != base_plan.replay_hash);
}

MK_TEST("production renderer VFX profiling rejects cross-backend proof transfer") {
    auto request = make_valid_request(true);
    request.backend_timing_rows.erase(request.backend_timing_rows.begin() + 1);
    request.postprocess_rows.erase(request.postprocess_rows.begin() + 1);

    const auto plan = mirakana::plan_renderer_production_vfx_profiling(request);

    MK_REQUIRE(plan.status == RendererProductionVfxProfilingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.feature_rows.empty());
    MK_REQUIRE(plan.backend_timing_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_parity) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::missing_backend_timing) == 1U);
}

MK_TEST("production renderer VFX profiling rejects unsafe rows and broad claims") {
    auto request = make_valid_request(true);
    request.feature_rows[0].feature_id = "native/d3d12";
    request.feature_rows[0].request_broad_performance_claim = true;
    request.feature_rows[0].request_native_handle_access = true;
    request.gpu_particle_budget_rows[0].max_particles = 128U;
    request.gpu_particle_budget_rows[0].max_spawn_per_frame = 256U;
    request.postprocess_rows[0].scene_color_available = false;
    request.backend_timing_rows[0].end_tick = request.backend_timing_rows[0].begin_tick;
    request.crash_telemetry_handoff_rows[0].request_crash_upload = true;
    request.crash_telemetry_handoff_rows[0].request_native_capture = true;
    request.row_budget = 2U;

    const auto plan = mirakana::plan_renderer_production_vfx_profiling(request);

    MK_REQUIRE(plan.status == RendererProductionVfxProfilingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_feature_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::broad_performance_claim) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_native_handle_claim) ==
               2U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_gpu_particle_budget) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_postprocess_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::invalid_backend_timing) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::unsupported_crash_upload) == 1U);
    MK_REQUIRE(diagnostic_count(plan, RendererProductionVfxProfilingDiagnosticCode::row_budget_exceeded) == 1U);
    MK_REQUIRE(plan.rejected_unsafe_row_count >= 5U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("production renderer VFX profiling reports no rows without backend claims") {
    const auto plan =
        mirakana::plan_renderer_production_vfx_profiling(mirakana::RendererProductionVfxProfilingRequest{});

    MK_REQUIRE(plan.status == RendererProductionVfxProfilingStatus::no_rows);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.feature_row_count == 0U);
    MK_REQUIRE(plan.replay_hash == 0U);
}

int main() {
    return mirakana::test::run_all();
}
