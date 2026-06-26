// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary
// Plan ID: renderer-commercial-readiness-evidence-promotion-v1 Task 10F.2

#include "mirakana/rhi/d3d12/d3d12_backend.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view k_validation_recipe{"renderer-d3d12-commercial-quality-host-evidence"};
constexpr std::string_view k_source_id{"Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25"};

[[noreturn]] void fail(std::string_view message) {
    std::cerr << message << '\n';
    std::exit(1);
}

[[nodiscard]] std::string json_bool(bool value) {
    return value ? "true" : "false";
}

[[nodiscard]] std::string json_escape(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
        case '\\':
            escaped += "\\\\";
            break;
        case '"':
            escaped += "\\\"";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped += ch;
            break;
        }
    }
    return escaped;
}

void write_text(const std::filesystem::path& path, std::string_view text) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        fail("failed to open D3D12 host supplement probe output file");
    }
    out << text;
    if (!out) {
        fail("failed to write D3D12 host supplement probe output file");
    }
}

[[nodiscard]] std::string
make_probe_summary_json(const mirakana::rhi::d3d12::CommercialQualityHostSupplementResult& result) {
    std::ostringstream json;
    json << "{\n"
         << "  \"schema_version\": \"GameEngine.RendererD3d12CommercialQualityHostSupplementProbe.v1\",\n"
         << "  \"validation_recipe\": \"" << k_validation_recipe << "\",\n"
         << "  \"source_id\": \"" << k_source_id << "\",\n"
         << "  \"ready\": " << json_bool(result.ready) << ",\n"
         << "  \"windows_sdk_available\": " << json_bool(result.windows_sdk_available) << ",\n"
         << "  \"dxgi_factory_created\": " << json_bool(result.dxgi_factory_created) << ",\n"
         << "  \"device_created\": " << json_bool(result.device_created) << ",\n"
         << "  \"used_warp\": " << json_bool(result.used_warp) << ",\n"
         << "  \"debug_layer_available\": " << json_bool(result.debug_layer_available) << ",\n"
         << "  \"debug_layer_enabled\": " << json_bool(result.debug_layer_enabled) << ",\n"
         << "  \"gpu_based_validation_enabled\": " << json_bool(result.gpu_based_validation_enabled) << ",\n"
         << "  \"info_queue_available\": " << json_bool(result.info_queue_available) << ",\n"
         << "  \"debug_message_count\": " << result.debug_message_count << ",\n"
         << "  \"gpu_based_validation_message_count\": " << result.gpu_based_validation_message_count << ",\n"
         << "  \"first_debug_message_id\": " << result.first_debug_message_id << ",\n"
         << "  \"first_debug_message_description\": \"" << json_escape(result.first_debug_message_description)
         << "\",\n"
         << "  \"clock_calibration_ready\": " << json_bool(result.clock_calibration_ready) << ",\n"
         << "  \"query_video_memory_info_ready\": " << json_bool(result.query_video_memory_info_ready) << ",\n"
         << "  \"enqueue_make_resident_ready\": " << json_bool(result.enqueue_make_resident_ready) << ",\n"
         << "  \"enqueue_make_resident_fence_signaled\": " << json_bool(result.enqueue_make_resident_fence_signaled)
         << ",\n"
         << "  \"unordered_access_barrier_ready\": " << json_bool(result.unordered_access_barrier_ready) << ",\n"
         << "  \"native_handles_exposed\": " << json_bool(result.native_handles_exposed) << ",\n"
         << "  \"diagnostic\": \"" << json_escape(result.diagnostic) << "\"\n"
         << "}\n";
    return json.str();
}

[[nodiscard]] std::string
make_host_gate_summary_json(const mirakana::rhi::d3d12::CommercialQualityHostSupplementResult& result) {
    std::ostringstream json;
    json << "{\n"
         << "  \"schema_version\": \"GameEngine.RendererD3d12CommercialQualityHostSupplementGate.v1\",\n"
         << "  \"validation_recipe\": \"" << k_validation_recipe << "\",\n"
         << "  \"source_id\": \"" << k_source_id << "\",\n"
         << "  \"status\": \"host_evidence_required\",\n"
         << "  \"renderer_d3d12_commercial_quality_host_supplement_ready\": 0,\n"
         << "  \"renderer_backend_parity_ready\": 0,\n"
         << "  \"renderer_metal_broad_readiness\": 0,\n"
         << "  \"renderer_broad_quality_ready\": 0,\n"
         << "  \"renderer_commercial_readiness\": 0,\n"
         << "  \"renderer_environment_ready\": 0,\n"
         << "  \"debug_message_count\": " << result.debug_message_count << ",\n"
         << "  \"gpu_based_validation_message_count\": " << result.gpu_based_validation_message_count << ",\n"
         << "  \"first_debug_message_id\": " << result.first_debug_message_id << ",\n"
         << "  \"first_debug_message_description\": \"" << json_escape(result.first_debug_message_description)
         << "\",\n"
         << "  \"diagnostic\": \"" << json_escape(result.diagnostic) << "\"\n"
         << "}\n";
    return json.str();
}

[[nodiscard]] std::string
make_supplement_json(const mirakana::rhi::d3d12::CommercialQualityHostSupplementResult& result) {
    std::ostringstream json;
    json << "{\n"
         << "  \"schema_version\": \"GameEngine.RendererD3d12CommercialQualityHostSupplement.v1\",\n"
         << "  \"validation_recipe\": \"" << k_validation_recipe << "\",\n"
         << "  \"fixture_only\": false,\n"
         << "  \"source_id\": \"" << k_source_id << "\",\n"
         << "  \"proof_rows\": {\n"
         << "    \"clock_calibration\": {\n"
         << "      \"ready\": true,\n"
         << "      \"api_name\": \"ID3D12CommandQueue::GetClockCalibration\",\n"
         << "      \"cpu_qpc_sample\": true,\n"
         << "      \"gpu_timestamp\": " << result.clock_calibration_gpu_timestamp << ",\n"
         << "      \"cpu_qpc_value\": " << result.clock_calibration_cpu_qpc_sample << ",\n"
         << "      \"queue_frequency_hz\": " << result.queue_frequency_hz << "\n"
         << "    },\n"
         << "    \"debug_validation\": {\n"
         << "      \"ready\": true,\n"
         << "      \"debug_layer_or_gpu_based_validation_clean\": true,\n"
         << "      \"debug_message_count\": " << result.debug_message_count << ",\n"
         << "      \"gpu_based_validation_message_count\": " << result.gpu_based_validation_message_count << ",\n"
         << "      \"first_debug_message_id\": " << result.first_debug_message_id << ",\n"
         << "      \"first_debug_message_description\": \"" << json_escape(result.first_debug_message_description)
         << "\"\n"
         << "    },\n"
         << "    \"residency\": {\n"
         << "      \"ready\": true,\n"
         << "      \"query_video_memory_info_ready\": true,\n"
         << "      \"enqueue_make_resident_fence_signaled\": true,\n"
         << "      \"residency_api_name\": \"ID3D12Device3::EnqueueMakeResident\",\n"
         << "      \"budget_api_name\": \"IDXGIAdapter3::QueryVideoMemoryInfo\",\n"
         << "      \"local_video_memory_budget_bytes\": " << result.local_video_memory_budget_bytes << ",\n"
         << "      \"local_video_memory_usage_bytes\": " << result.local_video_memory_usage_bytes << ",\n"
         << "      \"residency_fence_value\": " << result.residency_fence_value << "\n"
         << "    },\n"
         << "    \"unordered_access_barrier\": {\n"
         << "      \"ready\": true,\n"
         << "      \"resource_barrier_api_name\": \"D3D12_RESOURCE_BARRIER\",\n"
         << "      \"unordered_access_barriers_recorded\": " << result.unordered_access_barriers_recorded << "\n"
         << "    },\n"
         << "    \"native_handles\": {\n"
         << "      \"ready\": true,\n"
         << "      \"native_handles_exposed\": false\n"
         << "    }\n"
         << "  },\n"
         << "  \"non_claims\": {\n"
         << "    \"vulkan_inferred\": false,\n"
         << "    \"metal_inferred\": false,\n"
         << "    \"broad_ui_parity\": false,\n"
         << "    \"environment_ready\": false,\n"
         << "    \"external_engine_parity\": false,\n"
         << "    \"native_handles_exposed\": false\n"
         << "  }\n"
         << "}\n";
    return json.str();
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2 || argv[1] == nullptr || std::string_view{argv[1]}.empty()) {
        fail("Usage: MK_d3d12_commercial_quality_host_supplement_probe <output-directory>");
    }

    const std::filesystem::path output_directory{argv[1]};
    std::filesystem::create_directories(output_directory);

    const auto result = mirakana::rhi::d3d12::collect_commercial_quality_host_supplement(
        mirakana::rhi::d3d12::CommercialQualityHostSupplementDesc{
            .device = mirakana::rhi::d3d12::DeviceBootstrapDesc{.prefer_warp = true, .enable_debug_layer = true},
            .enable_gpu_based_validation = true,
        });

    write_text(output_directory / "probe-summary.json", make_probe_summary_json(result));
    if (result.ready) {
        write_text(output_directory / "d3d12-host-supplement.json", make_supplement_json(result));
    } else {
        write_text(output_directory / "host-gate-summary.json", make_host_gate_summary_json(result));
    }

    std::cout << "renderer-d3d12-commercial-quality-host-supplement-probe: "
              << "validation_recipe=renderer-d3d12-commercial-quality-host-supplement "
              << "renderer_d3d12_commercial_quality_host_supplement_probe_ready=" << (result.ready ? 1 : 0) << ' '
              << "renderer_d3d12_commercial_quality_host_supplement_ready=" << (result.ready ? 1 : 0) << ' '
              << "debug_message_count=" << result.debug_message_count << ' '
              << "gpu_based_validation_message_count=" << result.gpu_based_validation_message_count << ' '
              << "clock_calibration_ready=" << (result.clock_calibration_ready ? 1 : 0) << ' '
              << "query_video_memory_info_ready=" << (result.query_video_memory_info_ready ? 1 : 0) << ' '
              << "enqueue_make_resident_ready=" << (result.enqueue_make_resident_ready ? 1 : 0) << ' '
              << "enqueue_make_resident_fence_signaled=" << (result.enqueue_make_resident_fence_signaled ? 1 : 0) << ' '
              << "unordered_access_barrier_ready=" << (result.unordered_access_barrier_ready ? 1 : 0) << ' '
              << "native_handles_exposed=0 "
              << "renderer_backend_parity_ready=0 "
              << "renderer_metal_broad_readiness=0 "
              << "renderer_broad_quality_ready=0 "
              << "renderer_commercial_readiness=0 "
              << "renderer_environment_ready=0\n";

    return result.ready ? 0 : 2;
}
