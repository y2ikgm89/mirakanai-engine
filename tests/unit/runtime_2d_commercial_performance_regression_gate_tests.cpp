// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/two_d_commercial_performance_regression_gate.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::Runtime2DCommercialPerformanceDiagnostic;
using mirakana::runtime::Runtime2DCommercialPerformanceDiagnosticCode;
using mirakana::runtime::Runtime2DCommercialPerformanceMetricKind;
using mirakana::runtime::Runtime2DCommercialPerformanceMetricRow;
using mirakana::runtime::Runtime2DCommercialPerformanceOfficialSourceKind;
using mirakana::runtime::Runtime2DCommercialPerformanceOfficialSourceRow;
using mirakana::runtime::Runtime2DCommercialPerformanceRegressionGateDesc;
using mirakana::runtime::Runtime2DCommercialPerformanceWorkloadKind;
using mirakana::runtime::Runtime2DCommercialPerformanceWorkloadRow;

[[nodiscard]] bool has_diagnostic(const std::vector<Runtime2DCommercialPerformanceDiagnostic>& diagnostics,
                                  Runtime2DCommercialPerformanceDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] std::vector<Runtime2DCommercialPerformanceWorkloadRow> make_workload_rows() {
    return {
        Runtime2DCommercialPerformanceWorkloadRow{
            .id = "2d-dense-sprites",
            .kind = Runtime2DCommercialPerformanceWorkloadKind::dense_sprites,
            .host_class_id = "windows-d3d12-package-smoke",
            .validation_recipe_id = "installed-2d-commercial-performance-regression-smoke",
            .ready = true,
            .package_visible = true,
        },
        Runtime2DCommercialPerformanceWorkloadRow{
            .id = "2d-large-tilemap",
            .kind = Runtime2DCommercialPerformanceWorkloadKind::large_tilemap,
            .host_class_id = "windows-d3d12-package-smoke",
            .validation_recipe_id = "installed-2d-commercial-performance-regression-smoke",
            .ready = true,
            .package_visible = true,
        },
        Runtime2DCommercialPerformanceWorkloadRow{
            .id = "2d-ui-heavy-scene",
            .kind = Runtime2DCommercialPerformanceWorkloadKind::ui_heavy_scene,
            .host_class_id = "windows-d3d12-package-smoke",
            .validation_recipe_id = "installed-2d-commercial-performance-regression-smoke",
            .ready = true,
            .package_visible = true,
        },
        Runtime2DCommercialPerformanceWorkloadRow{
            .id = "2d-animation-heavy-scene",
            .kind = Runtime2DCommercialPerformanceWorkloadKind::animation_heavy_scene,
            .host_class_id = "windows-d3d12-package-smoke",
            .validation_recipe_id = "installed-2d-commercial-performance-regression-smoke",
            .ready = true,
            .package_visible = true,
        },
        Runtime2DCommercialPerformanceWorkloadRow{
            .id = "2d-streaming-stress",
            .kind = Runtime2DCommercialPerformanceWorkloadKind::streaming_stress,
            .host_class_id = "windows-d3d12-package-smoke",
            .validation_recipe_id = "installed-2d-commercial-performance-regression-smoke",
            .ready = true,
            .package_visible = true,
        },
        Runtime2DCommercialPerformanceWorkloadRow{
            .id = "2d-physics-stress",
            .kind = Runtime2DCommercialPerformanceWorkloadKind::physics_stress,
            .host_class_id = "windows-d3d12-package-smoke",
            .validation_recipe_id = "installed-2d-commercial-performance-regression-smoke",
            .ready = true,
            .package_visible = true,
        },
        Runtime2DCommercialPerformanceWorkloadRow{
            .id = "2d-audio-input-stress",
            .kind = Runtime2DCommercialPerformanceWorkloadKind::audio_input_stress,
            .host_class_id = "windows-d3d12-package-smoke",
            .validation_recipe_id = "installed-2d-commercial-performance-regression-smoke",
            .ready = true,
            .package_visible = true,
        },
        Runtime2DCommercialPerformanceWorkloadRow{
            .id = "2d-long-running-playtest",
            .kind = Runtime2DCommercialPerformanceWorkloadKind::long_running_playtest,
            .host_class_id = "windows-d3d12-package-smoke",
            .validation_recipe_id = "installed-2d-commercial-performance-regression-smoke",
            .ready = true,
            .package_visible = true,
        },
    };
}

[[nodiscard]] Runtime2DCommercialPerformanceMetricRow metric_row(Runtime2DCommercialPerformanceMetricKind kind,
                                                                 std::string id, std::uint64_t p50, std::uint64_t p95,
                                                                 std::uint64_t p99, bool host_gated_artifact = false) {
    return Runtime2DCommercialPerformanceMetricRow{
        .id = std::move(id),
        .kind = kind,
        .host_class_id = "windows-d3d12-package-smoke",
        .counter_name = "sample_2d.commercial_performance",
        .p50_value = p50,
        .p95_value = p95,
        .p99_value = p99,
        .p50_limit = 16'670U,
        .p95_limit = 16'670U,
        .p99_limit = 16'670U,
        .ready = true,
        .package_visible = !host_gated_artifact,
        .host_gated_artifact = host_gated_artifact,
        .threshold_host_class_specific = true,
    };
}

[[nodiscard]] std::vector<Runtime2DCommercialPerformanceMetricRow> make_metric_rows() {
    return {
        metric_row(Runtime2DCommercialPerformanceMetricKind::cpu_frame_time, "cpu-frame-time", 16'000U, 16'000U,
                   16'000U),
        metric_row(Runtime2DCommercialPerformanceMetricKind::gpu_frame_time, "gpu-frame-time", 0U, 0U, 0U, true),
        metric_row(Runtime2DCommercialPerformanceMetricKind::input_to_present_latency, "input-to-present-latency", 0U,
                   0U, 0U, true),
        metric_row(Runtime2DCommercialPerformanceMetricKind::present_pacing, "present-pacing", 0U, 0U, 0U, true),
        metric_row(Runtime2DCommercialPerformanceMetricKind::io_decompression_upload_overlap,
                   "io-decompression-upload-overlap", 1U, 1U, 1U),
        metric_row(Runtime2DCommercialPerformanceMetricKind::memory_high_water, "memory-high-water", 4096U, 8192U,
                   8192U),
        metric_row(Runtime2DCommercialPerformanceMetricKind::gpu_residency_pressure, "gpu-residency-pressure", 0U, 0U,
                   0U, true),
        metric_row(Runtime2DCommercialPerformanceMetricKind::allocator_churn, "allocator-churn", 0U, 0U, 0U),
        metric_row(Runtime2DCommercialPerformanceMetricKind::job_queue_depth, "job-queue-depth", 1U, 1U, 1U),
        metric_row(Runtime2DCommercialPerformanceMetricKind::cache_misses, "cache-misses", 0U, 0U, 0U, true),
        metric_row(Runtime2DCommercialPerformanceMetricKind::package_miss_pop_in, "package-miss-pop-in", 0U, 0U, 0U),
    };
}

[[nodiscard]] std::array<Runtime2DCommercialPerformanceOfficialSourceRow, 7U> make_official_source_rows() {
    return {
        Runtime2DCommercialPerformanceOfficialSourceRow{
            .id = "microsoft.learn.d3d12",
            .kind = Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_d3d12,
            .url = "https://learn.microsoft.com/en-us/windows/win32/direct3d12/direct3d-12-graphics",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialPerformanceOfficialSourceRow{
            .id = "microsoft.learn.windows-performance-toolkit",
            .kind = Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_wpt,
            .url = "https://learn.microsoft.com/en-us/windows-hardware/test/wpt/",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialPerformanceOfficialSourceRow{
            .id = "microsoft.pix-on-windows",
            .kind = Runtime2DCommercialPerformanceOfficialSourceKind::microsoft_pix,
            .url = "https://learn.microsoft.com/en-us/windows/win32/direct3dtools/pix/articles/general/pix-overview",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialPerformanceOfficialSourceRow{
            .id = "khronos.vulkan-timestamps-debug-utils",
            .kind = Runtime2DCommercialPerformanceOfficialSourceKind::khronos_vulkan_timestamp_debug_utils,
            .url = "https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialPerformanceOfficialSourceRow{
            .id = "apple.xctrace-metal-capture",
            .kind = Runtime2DCommercialPerformanceOfficialSourceKind::apple_xctrace_metal_capture,
            .url = "https://developer.apple.com/metal/tools/",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialPerformanceOfficialSourceRow{
            .id = "linux.perf",
            .kind = Runtime2DCommercialPerformanceOfficialSourceKind::linux_perf,
            .url = "https://perf.wiki.kernel.org/index.php/Main_Page",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
        Runtime2DCommercialPerformanceOfficialSourceRow{
            .id = "repository.legal-policy",
            .kind = Runtime2DCommercialPerformanceOfficialSourceKind::repository_legal_policy,
            .url = "docs/legal-and-licensing.md",
            .ready = true,
            .official = true,
            .public_docs_only = true,
        },
    };
}

[[nodiscard]] Runtime2DCommercialPerformanceRegressionGateDesc make_ready_desc() {
    return Runtime2DCommercialPerformanceRegressionGateDesc{
        .workload_rows = make_workload_rows(),
        .metric_rows = make_metric_rows(),
        .official_source_rows = make_official_source_rows(),
        .selected_package_regression_gate_claim = true,
    };
}

} // namespace

MK_TEST("runtime 2d commercial performance regression gate accepts selected package evidence") {
    const auto result =
        mirakana::runtime::evaluate_runtime_2d_commercial_performance_regression_gate(make_ready_desc());

    MK_REQUIRE(result.ready);
    MK_REQUIRE(result.workload_gate_ready);
    MK_REQUIRE(result.metric_gate_ready);
    MK_REQUIRE(result.official_source_ready);
    MK_REQUIRE(result.host_threshold_gate_ready);
    MK_REQUIRE(result.workload_rows == 8U);
    MK_REQUIRE(result.metric_rows == 11U);
    MK_REQUIRE(result.host_class_threshold_rows == 11U);
    MK_REQUIRE(result.package_visible_metric_rows == 6U);
    MK_REQUIRE(result.host_gated_profiler_artifact_rows == 5U);
    MK_REQUIRE(result.official_source_rows == 7U);
    MK_REQUIRE(result.cpu_frame_p50_us == 16'000U);
    MK_REQUIRE(result.cpu_frame_p95_us == 16'000U);
    MK_REQUIRE(result.cpu_frame_p99_us == 16'000U);
    MK_REQUIRE(result.over_budget_rows == 0U);
    MK_REQUIRE(result.broad_optimization_claim_rows == 0U);
    MK_REQUIRE(result.cross_backend_parity_claim_rows == 0U);
    MK_REQUIRE(result.external_engine_claim_rows == 0U);
    MK_REQUIRE(result.legal_approval_claim_rows == 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("runtime 2d commercial performance regression gate rejects missing workload and source rows") {
    auto desc = make_ready_desc();
    desc.workload_rows.pop_back();
    desc.official_source_rows[1].ready = false;

    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_performance_regression_gate(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::workload_not_ready));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::official_source_not_ready));
}

MK_TEST("runtime 2d commercial performance regression gate rejects over budget and broad claims") {
    auto desc = make_ready_desc();
    desc.metric_rows[0].p99_value = 20'000U;
    desc.broad_optimization_claim = true;
    desc.cross_backend_parity_claim = true;
    desc.external_engine_compatibility_claim = true;
    desc.legal_approval_claim = true;

    const auto result = mirakana::runtime::evaluate_runtime_2d_commercial_performance_regression_gate(desc);

    MK_REQUIRE(!result.ready);
    MK_REQUIRE(result.over_budget_rows == 1U);
    MK_REQUIRE(result.broad_optimization_claim_rows == 1U);
    MK_REQUIRE(result.cross_backend_parity_claim_rows == 1U);
    MK_REQUIRE(result.external_engine_claim_rows == 1U);
    MK_REQUIRE(result.legal_approval_claim_rows == 1U);
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::over_budget));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::broad_optimization_claim));
    MK_REQUIRE(
        has_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::cross_backend_parity_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics,
                              Runtime2DCommercialPerformanceDiagnosticCode::external_engine_compatibility_claim));
    MK_REQUIRE(has_diagnostic(result.diagnostics, Runtime2DCommercialPerformanceDiagnosticCode::legal_approval_claim));
}

int main() {
    return mirakana::test::run_all();
}
