// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/vulkan/vulkan_mavg_mesh_shader_lod.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace {

struct SpirvArtifactFromEnvironment {
    bool configured{false};
    std::vector<std::uint32_t> words;
    std::string diagnostic;
};

[[nodiscard]] std::string environment_variable_value(const char* name) {
#if defined(_WIN32)
    char* value{nullptr};
    std::size_t value_size{0};
    if (_dupenv_s(&value, &value_size, name) != 0 || value == nullptr) {
        return {};
    }

    std::string result{value};
    std::free(value);
    return result;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
#endif
}

[[nodiscard]] SpirvArtifactFromEnvironment load_spirv_artifact_from_environment(const char* environment_variable) {
    const auto path = environment_variable_value(environment_variable);
    if (path.empty()) {
        return SpirvArtifactFromEnvironment{.configured = false, .words = {}, .diagnostic = "not configured"};
    }

    std::ifstream input{path, std::ios::binary | std::ios::ate};
    if (!input) {
        return SpirvArtifactFromEnvironment{
            .configured = true, .words = {}, .diagnostic = std::string{"unable to open "} + environment_variable};
    }

    const auto size = input.tellg();
    if (size <= 0 || size % static_cast<std::streamoff>(sizeof(std::uint32_t)) != 0) {
        return SpirvArtifactFromEnvironment{.configured = true,
                                            .words = {},
                                            .diagnostic =
                                                std::string{"invalid SPIR-V byte size in "} + environment_variable};
    }

    input.seekg(0, std::ios::beg);
    std::vector<std::uint32_t> words(
        static_cast<std::size_t>(size / static_cast<std::streamoff>(sizeof(std::uint32_t))));
    input.read(reinterpret_cast<char*>(words.data()), size);
    if (!input) {
        return SpirvArtifactFromEnvironment{
            .configured = true, .words = {}, .diagnostic = std::string{"failed to read "} + environment_variable};
    }

    return SpirvArtifactFromEnvironment{.configured = true, .words = std::move(words), .diagnostic = "loaded"};
}

[[nodiscard]] mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow make_valid_task_row() noexcept {
    return mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow{
        .cluster_index = 7,
        .meshlet_index = 11,
        .output_vertex_count = 3,
        .output_primitive_count = 1,
        .task_group_count_x = 1,
        .task_group_count_y = 1,
        .task_group_count_z = 1,
        .mesh_group_thread_count = 1,
        .fallback_index_count = 3,
    };
}

} // namespace

MK_TEST("vulkan mavg mesh shader lod rejects empty task rows") {
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{});

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 1U);
    MK_REQUIRE(result.diagnostic_count == 1U);
    MK_REQUIRE(!result.diagnostic_text.empty());
    MK_REQUIRE(!result.created_mesh_pipeline_state);
    MK_REQUIRE(!result.executed_mesh_shader);
    MK_REQUIRE(!result.used_input_layout);
    MK_REQUIRE(!result.used_index_buffer);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
}

MK_TEST("vulkan mavg mesh shader lod probe records mesh shader feature state") {
    const auto capability = mirakana::rhi::vulkan::probe_vulkan_mavg_mesh_shader_lod_capability();

    if (!capability.loader_available) {
        MK_REQUIRE(!capability.instance_created);
        MK_REQUIRE(!capability.feature_query_executed);
        MK_REQUIRE(!capability.diagnostic_text.empty());
    }
    if (capability.feature_query_executed) {
        MK_REQUIRE(capability.physical_device_selected);
        MK_REQUIRE(capability.device_extension_supported || !capability.mesh_shader_supported);
        MK_REQUIRE(!capability.mesh_shader_enabled || capability.mesh_shader_supported);
        MK_REQUIRE(!capability.task_shader_enabled || capability.task_shader_supported);
        MK_REQUIRE(!capability.draw_indirect_count_enabled || capability.draw_indirect_count_supported);
        MK_REQUIRE(capability.max_mesh_work_group_total_count == 0U || capability.max_mesh_output_vertices > 0U);
        if (capability.mesh_shader_supported) {
            MK_REQUIRE(capability.max_mesh_work_group_count_x > 0U);
            MK_REQUIRE(capability.max_mesh_work_group_count_y > 0U);
            MK_REQUIRE(capability.max_mesh_work_group_count_z > 0U);
            MK_REQUIRE(capability.max_mesh_work_group_total_count > 0U);
        }
    }
    MK_REQUIRE(!capability.exposed_native_handles);
    MK_REQUIRE(!capability.claimed_nanite_equivalence);
    MK_REQUIRE(!capability.claimed_metal_readiness);
}

MK_TEST("vulkan mavg mesh shader lod host gates without mesh shader support") {
    const auto capability = mirakana::rhi::vulkan::probe_vulkan_mavg_mesh_shader_lod_capability();
    if (!capability.feature_query_executed || capability.mesh_shader_supported) {
        return;
    }

    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(result.host_gated);
    MK_REQUIRE(result.feature_query_executed);
    MK_REQUIRE(!result.created_mesh_pipeline_state);
    MK_REQUIRE(!result.executed_mesh_shader);
    MK_REQUIRE(!result.mesh_shader_enabled);
    MK_REQUIRE(!result.task_shader_enabled);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
    MK_REQUIRE(!result.used_input_layout);
    MK_REQUIRE(!result.used_index_buffer);
    MK_REQUIRE(result.diagnostic_count > 0U);
}

MK_TEST("vulkan mavg mesh shader lod rejects invalid workgroup counts") {
    auto row = make_valid_task_row();
    row.task_group_count_x = 0;

    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 2U);
    MK_REQUIRE(result.diagnostic_count == 1U);
    MK_REQUIRE(!result.created_mesh_pipeline_state);
    MK_REQUIRE(!result.executed_mesh_shader);
}

MK_TEST("vulkan mavg mesh shader lod rejects indirect range overflow") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_draw = true,
            .indirect_buffer_size_bytes = 12,
            .indirect_buffer_offset_bytes = 8,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = 12,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.diagnostic_count == 1U);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_eligible);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_count_eligible);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
}

MK_TEST("vulkan mavg mesh shader lod does not promote fallback indexed draw") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .allow_conventional_indexed_fallback = true,
        });

    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
    MK_REQUIRE(!result.used_input_layout);
    MK_REQUIRE(!result.used_index_buffer);
    MK_REQUIRE(result.fallback_indexed_draw_preserved);
    MK_REQUIRE(!result.fallback_indexed_draw_promoted_readiness);
    MK_REQUIRE(!result.claimed_nanite_equivalence);
    MK_REQUIRE(!result.claimed_metal_readiness);
}

MK_TEST("vulkan mavg mesh shader lod executes mesh shader when host supports it") {
    const auto task_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_TASK_SPV");
    const auto mesh_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_MESH_SPV");
    const auto fragment_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_FRAGMENT_SPV");
    if (!task_artifact.configured || !mesh_artifact.configured || !fragment_artifact.configured) {
        const auto row = make_valid_task_row();
        const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
            mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
                .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            });

        MK_REQUIRE(!result.succeeded);
        MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
        MK_REQUIRE(!result.diagnostic_text.empty());
        return;
    }
    MK_REQUIRE(task_artifact.diagnostic == "loaded");
    MK_REQUIRE(mesh_artifact.diagnostic == "loaded");
    MK_REQUIRE(fragment_artifact.diagnostic == "loaded");

    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_shader_spirv = std::span<const std::uint32_t>{task_artifact.words},
            .mesh_shader_spirv = std::span<const std::uint32_t>{mesh_artifact.words},
            .fragment_shader_spirv = std::span<const std::uint32_t>{fragment_artifact.words},
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
        });

    if (result.host_gated) {
        MK_REQUIRE(!result.succeeded);
        MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
        MK_REQUIRE(!result.diagnostic_text.empty());
        return;
    }

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.created_mesh_pipeline_state);
    MK_REQUIRE(result.used_mesh_shader_stage);
    MK_REQUIRE(result.used_task_shader_stage);
    MK_REQUIRE(result.mesh_shader_enabled);
    MK_REQUIRE(result.task_shader_enabled);
    MK_REQUIRE(!result.used_input_layout);
    MK_REQUIRE(!result.used_index_buffer);
    MK_REQUIRE(result.draw_mesh_tasks_direct_calls == 1U);
    MK_REQUIRE(result.draw_mesh_tasks_indirect_calls == 0U);
    MK_REQUIRE(result.draw_mesh_tasks_indirect_count_calls == 0U);
    MK_REQUIRE(result.readback_nonzero);
    MK_REQUIRE(result.readback_hash != 0U);
    MK_REQUIRE(result.mavg_mesh_shader_lod_vulkan_ready);
    MK_REQUIRE(!result.claimed_nanite_equivalence);
    MK_REQUIRE(!result.claimed_metal_readiness);
}

int main() {
    return mirakana::test::run_all();
}
