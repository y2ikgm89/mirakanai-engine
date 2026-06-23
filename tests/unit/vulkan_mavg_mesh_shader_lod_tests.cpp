// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/vulkan/vulkan_mavg_mesh_shader_lod.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
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

[[nodiscard]] const char* counter_bit(bool value) noexcept {
    return value ? "1" : "0";
}

[[nodiscard]] const char*
execution_status(bool ready, const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchResult& result) noexcept {
    if (ready) {
        return "ready";
    }
    return result.host_gated ? "host_gated" : "blocked";
}

void emit_counter(std::string_view name, std::string_view value) {
    std::cout << name << '=' << value << '\n';
}

void emit_counter(std::string_view name, const char* value) {
    emit_counter(name, std::string_view{value});
}

void emit_counter(std::string_view name, std::uint32_t value) {
    std::cout << name << '=' << value << '\n';
}

void emit_counter(std::string_view name, bool value) {
    emit_counter(name, std::string_view{counter_bit(value)});
}

[[nodiscard]] mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc
make_evidence_desc(const SpirvArtifactFromEnvironment& task_artifact, const SpirvArtifactFromEnvironment& mesh_artifact,
                   const SpirvArtifactFromEnvironment& fragment_artifact,
                   std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow> rows) {
    return mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
        .task_shader_spirv = std::span<const std::uint32_t>{task_artifact.words},
        .mesh_shader_spirv = std::span<const std::uint32_t>{mesh_artifact.words},
        .fragment_shader_spirv = std::span<const std::uint32_t>{fragment_artifact.words},
        .task_shader_entry_point = "task_main",
        .mesh_shader_entry_point = "mesh_main",
        .fragment_shader_entry_point = "fragment_main",
        .task_rows = rows,
    };
}

[[nodiscard]] int emit_mavg_vulkan_mesh_shader_lod_evidence() {
    const auto task_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_TASK_SPV");
    const auto mesh_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_MESH_SPV");
    const auto fragment_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_FRAGMENT_SPV");
    const auto row = make_valid_task_row();
    constexpr auto command_size = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes;

    auto direct_desc =
        make_evidence_desc(task_artifact, mesh_artifact, fragment_artifact,
                           std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1});
    const auto direct_result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(direct_desc);

    auto indirect_desc = direct_desc;
    indirect_desc.request_indirect_draw = true;
    indirect_desc.indirect_buffer_size_bytes = command_size;
    indirect_desc.indirect_draw_count = 1U;
    indirect_desc.indirect_stride_bytes = command_size;
    const auto indirect_result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(indirect_desc);

    auto count_desc = direct_desc;
    count_desc.request_indirect_count_draw = true;
    count_desc.indirect_buffer_size_bytes = command_size;
    count_desc.indirect_draw_count = 1U;
    count_desc.indirect_stride_bytes = command_size;
    count_desc.count_buffer_size_bytes = sizeof(std::uint32_t);
    count_desc.max_indirect_count_draws = 1U;
    const auto count_result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(count_desc);

    const auto max_draw_indirect_barriers = std::max(indirect_result.draw_indirect_stage_barriers_recorded,
                                                     count_result.draw_indirect_stage_barriers_recorded);
    const auto max_task_barriers = std::max({direct_result.task_shader_stage_barriers_recorded,
                                             indirect_result.task_shader_stage_barriers_recorded,
                                             count_result.task_shader_stage_barriers_recorded});
    const auto max_mesh_barriers = std::max({direct_result.mesh_shader_stage_barriers_recorded,
                                             indirect_result.mesh_shader_stage_barriers_recorded,
                                             count_result.mesh_shader_stage_barriers_recorded});
    const auto payload_ready = direct_result.shader_payload_consumed_by_task_or_mesh ||
                               indirect_result.shader_payload_consumed_by_task_or_mesh ||
                               count_result.shader_payload_consumed_by_task_or_mesh;

    emit_counter("validation_recipe", "mavg-vulkan-mesh-shader-indirect-dispatch");
    emit_counter("mavg_vulkan_mesh_shader_spirv_artifacts_configured",
                 task_artifact.configured && mesh_artifact.configured && fragment_artifact.configured);
    emit_counter("mavg_vulkan_mesh_shader_indirect_dispatch_status",
                 execution_status(indirect_result.mavg_mesh_shader_lod_vulkan_indirect_ready, indirect_result));
    emit_counter("mavg_vulkan_mesh_shader_indirect_dispatch_ready",
                 indirect_result.mavg_mesh_shader_lod_vulkan_indirect_ready);
    emit_counter("mavg_vulkan_mesh_shader_indirect_count_status",
                 execution_status(count_result.mavg_mesh_shader_lod_vulkan_indirect_count_ready, count_result));
    emit_counter("mavg_vulkan_mesh_shader_indirect_count_ready",
                 count_result.mavg_mesh_shader_lod_vulkan_indirect_count_ready);
    emit_counter("mavg_vulkan_mesh_shader_indirect_count_host_gated",
                 count_result.mavg_mesh_shader_lod_vulkan_indirect_count_host_gated || count_result.host_gated);
    emit_counter("mavg_vulkan_mesh_shader_indirect_argument_buffer_usage_ready",
                 indirect_result.indirect_argument_buffer_usage_ready &&
                     count_result.indirect_argument_buffer_usage_ready);
    emit_counter("mavg_vulkan_mesh_shader_indirect_count_buffer_usage_ready",
                 count_result.indirect_count_buffer_usage_ready);
    emit_counter("mavg_vulkan_mesh_shader_indirect_stride_ready",
                 indirect_result.indirect_stride_valid && count_result.indirect_stride_valid);
    emit_counter("mavg_vulkan_mesh_shader_indirect_range_ready", indirect_result.indirect_argument_range_valid &&
                                                                     count_result.indirect_argument_range_valid &&
                                                                     count_result.indirect_count_range_valid);
    emit_counter("mavg_vulkan_mesh_shader_draw_indirect_stage_barriers", max_draw_indirect_barriers);
    emit_counter("mavg_vulkan_mesh_shader_task_stage_barriers", max_task_barriers);
    emit_counter("mavg_vulkan_mesh_shader_mesh_stage_barriers", max_mesh_barriers);
    emit_counter("mavg_vulkan_mesh_shader_payload_consumption_ready", payload_ready);
    emit_counter("mavg_vulkan_mesh_shader_indirect_count_value", count_result.last_indirect_count_buffer_value);
    emit_counter("mavg_vulkan_mesh_shader_indirect_executed_draw_count",
                 count_result.last_indirect_executed_draw_count);
    emit_counter("mavg_mesh_shader_lod_vulkan_ready", direct_result.mavg_mesh_shader_lod_vulkan_ready);
    emit_counter("mavg_mesh_shader_lod_ready", false);
    emit_counter("mavg_nanite_compatible", false);
    emit_counter("mavg_nanite_equivalent", false);
    emit_counter("mavg_nanite_superior", false);
    emit_counter("mavg_vulkan_native_handles_exposed", false);
    return 0;
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

MK_TEST("vulkan mavg mesh shader lod indirect command mirrors Vulkan xyz layout") {
    using Command = mirakana::rhi::vulkan::VulkanMavgMeshShaderLodIndirectCommand;
    static_assert(sizeof(Command) == mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes);
    static_assert(alignof(Command) == alignof(std::uint32_t));

    constexpr Command command{.group_count_x = 3, .group_count_y = 5, .group_count_z = 7};
    const auto words = std::bit_cast<std::array<std::uint32_t, 3>>(command);

    MK_REQUIRE(words[0] == command.group_count_x);
    MK_REQUIRE(words[1] == command.group_count_y);
    MK_REQUIRE(words[2] == command.group_count_z);
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

MK_TEST("vulkan mavg mesh shader lod rejects indirect argument buffer without indirect usage") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_draw = true,
            .indirect_buffer_usage = mirakana::rhi::BufferUsage::copy_source,
            .indirect_buffer_size_bytes =
                mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.diagnostic_text == "vulkan_mavg_mesh_shader_lod_indirect_argument_usage_invalid");
    MK_REQUIRE(!result.indirect_argument_buffer_usage_ready);
    MK_REQUIRE(result.indirect_argument_offset_aligned);
    MK_REQUIRE(result.indirect_stride_valid);
    MK_REQUIRE(!result.indirect_argument_range_valid);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_eligible);
}

MK_TEST("vulkan mavg mesh shader lod rejects unaligned indirect argument offset") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_draw = true,
            .indirect_buffer_size_bytes = 64,
            .indirect_buffer_offset_bytes = 2,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.diagnostic_text == "vulkan_mavg_mesh_shader_lod_indirect_argument_offset_unaligned");
    MK_REQUIRE(result.indirect_argument_buffer_usage_ready);
    MK_REQUIRE(!result.indirect_argument_offset_aligned);
    MK_REQUIRE(result.indirect_stride_valid);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_eligible);
}

MK_TEST("vulkan mavg mesh shader lod rejects indirect stride smaller than command size") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_draw = true,
            .indirect_buffer_size_bytes = 64,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 2,
            .indirect_stride_bytes =
                mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes - 4U,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.diagnostic_text == "vulkan_mavg_mesh_shader_lod_indirect_stride_invalid");
    MK_REQUIRE(result.indirect_argument_buffer_usage_ready);
    MK_REQUIRE(result.indirect_argument_offset_aligned);
    MK_REQUIRE(!result.indirect_stride_valid);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_eligible);
}

MK_TEST("vulkan mavg mesh shader lod rejects indirect count range overflow before host probing") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_count_draw = true,
            .indirect_buffer_size_bytes =
                mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .count_buffer_size_bytes = 4,
            .count_buffer_offset_bytes = 4,
            .max_indirect_count_draws = 1,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.diagnostic_count == 1U);
    MK_REQUIRE(!result.indirect_count_range_valid);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_count_eligible);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_indirect_count_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready_promoted);
}

MK_TEST("vulkan mavg mesh shader lod rejects indirect count buffer without indirect usage") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_count_draw = true,
            .indirect_buffer_size_bytes =
                mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .count_buffer_usage = mirakana::rhi::BufferUsage::copy_source,
            .count_buffer_size_bytes = 4,
            .count_buffer_offset_bytes = 0,
            .max_indirect_count_draws = 1,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.diagnostic_text == "vulkan_mavg_mesh_shader_lod_indirect_count_usage_invalid");
    MK_REQUIRE(result.draw_mesh_tasks_indirect_eligible);
    MK_REQUIRE(!result.indirect_count_buffer_usage_ready);
    MK_REQUIRE(result.indirect_count_offset_aligned);
    MK_REQUIRE(!result.indirect_count_range_valid);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_count_eligible);
}

MK_TEST("vulkan mavg mesh shader lod rejects unaligned indirect count buffer offset") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_count_draw = true,
            .indirect_buffer_size_bytes =
                mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .count_buffer_size_bytes = 8,
            .count_buffer_offset_bytes = 2,
            .max_indirect_count_draws = 1,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.diagnostic_text == "vulkan_mavg_mesh_shader_lod_indirect_count_offset_unaligned");
    MK_REQUIRE(result.draw_mesh_tasks_indirect_eligible);
    MK_REQUIRE(result.indirect_count_buffer_usage_ready);
    MK_REQUIRE(!result.indirect_count_offset_aligned);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_count_eligible);
}

MK_TEST("vulkan mavg mesh shader lod rejects zero indirect count max draws") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_count_draw = true,
            .indirect_buffer_size_bytes =
                mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes,
            .count_buffer_size_bytes = 4,
            .count_buffer_offset_bytes = 0,
            .max_indirect_count_draws = 0,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.diagnostic_text == "vulkan_mavg_mesh_shader_lod_indirect_count_max_draws_invalid");
    MK_REQUIRE(!result.indirect_argument_range_valid);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_eligible);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_count_eligible);
}

MK_TEST("vulkan mavg mesh shader lod records indirect validation evidence for invalid stride") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_draw = true,
            .indirect_buffer_size_bytes = 64,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 2,
            .indirect_stride_bytes = 10,
        });

    MK_REQUIRE(!result.succeeded);
    MK_REQUIRE(!result.host_gated);
    MK_REQUIRE(!result.feature_query_executed);
    MK_REQUIRE(result.failure_stage == 3U);
    MK_REQUIRE(result.indirect_argument_buffer_usage_ready);
    MK_REQUIRE(result.indirect_argument_offset_aligned);
    MK_REQUIRE(!result.indirect_stride_valid);
    MK_REQUIRE(!result.indirect_argument_range_valid);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_indirect_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready_promoted);
}

MK_TEST("vulkan mavg mesh shader lod direct path keeps indirect evidence zero") {
    const auto row = make_valid_task_row();
    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
        });

    MK_REQUIRE(!result.draw_mesh_tasks_indirect_eligible);
    MK_REQUIRE(!result.draw_mesh_tasks_indirect_count_eligible);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_indirect_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_indirect_count_ready);
    MK_REQUIRE(result.last_indirect_count_buffer_value == 0U);
    MK_REQUIRE(result.last_indirect_executed_draw_count == 0U);
    MK_REQUIRE(result.draw_indirect_stage_barriers_recorded == 0U);
    MK_REQUIRE(result.task_shader_stage_barriers_recorded == 0U);
    MK_REQUIRE(result.mesh_shader_stage_barriers_recorded == 0U);
    MK_REQUIRE(!result.shader_payload_consumed_by_task_or_mesh);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready_promoted);
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
            .task_shader_entry_point = "task_main",
            .mesh_shader_entry_point = "mesh_main",
            .fragment_shader_entry_point = "fragment_main",
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
        });

    if (result.host_gated) {
        MK_REQUIRE(!result.succeeded);
        MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
        MK_REQUIRE(!result.diagnostic_text.empty());
        return;
    }

    if (!result.succeeded) {
        std::cerr << "vulkan_mavg_mesh_shader_lod diagnostic=" << result.diagnostic_text
                  << " stage=" << result.failure_stage << " created_pipeline=" << result.created_mesh_pipeline_state
                  << " used_mesh=" << result.used_mesh_shader_stage << " used_task=" << result.used_task_shader_stage
                  << " direct_calls=" << result.draw_mesh_tasks_direct_calls
                  << " executed=" << result.executed_mesh_shader << " readback_nonzero=" << result.readback_nonzero
                  << " readback_hash=" << result.readback_hash << '\n';
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
    MK_REQUIRE(result.task_shader_stage_barriers_recorded == 1U);
    MK_REQUIRE(result.mesh_shader_stage_barriers_recorded == 1U);
    MK_REQUIRE(result.shader_payload_consumed_by_task_or_mesh);
    MK_REQUIRE(result.readback_nonzero);
    MK_REQUIRE(result.readback_hash != 0U);
    MK_REQUIRE(result.mavg_mesh_shader_lod_vulkan_ready);
    MK_REQUIRE(!result.claimed_nanite_equivalence);
    MK_REQUIRE(!result.claimed_metal_readiness);
}

MK_TEST("vulkan mavg mesh shader lod executes indirect mesh tasks when host supports it") {
    const auto task_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_TASK_SPV");
    const auto mesh_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_MESH_SPV");
    const auto fragment_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_FRAGMENT_SPV");
    const auto row = make_valid_task_row();
    constexpr auto command_size = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes;

    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_shader_spirv = std::span<const std::uint32_t>{task_artifact.words},
            .mesh_shader_spirv = std::span<const std::uint32_t>{mesh_artifact.words},
            .fragment_shader_spirv = std::span<const std::uint32_t>{fragment_artifact.words},
            .task_shader_entry_point = "task_main",
            .mesh_shader_entry_point = "mesh_main",
            .fragment_shader_entry_point = "fragment_main",
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_draw = true,
            .indirect_buffer_size_bytes = command_size,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = command_size,
        });

    if (!task_artifact.configured || !mesh_artifact.configured || !fragment_artifact.configured || result.host_gated) {
        MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_indirect_ready);
        MK_REQUIRE(!result.mavg_mesh_shader_lod_ready_promoted);
        MK_REQUIRE(!result.diagnostic_text.empty());
        return;
    }

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.created_mesh_pipeline_state);
    MK_REQUIRE(result.draw_mesh_tasks_direct_calls == 0U);
    MK_REQUIRE(result.draw_mesh_tasks_indirect_calls == 1U);
    MK_REQUIRE(result.draw_mesh_tasks_indirect_count_calls == 0U);
    MK_REQUIRE(result.draw_indirect_stage_barriers_recorded == 1U);
    MK_REQUIRE(result.task_shader_stage_barriers_recorded == 1U);
    MK_REQUIRE(result.mesh_shader_stage_barriers_recorded == 1U);
    MK_REQUIRE(result.shader_payload_consumed_by_task_or_mesh);
    MK_REQUIRE(result.readback_nonzero);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
    MK_REQUIRE(result.mavg_mesh_shader_lod_vulkan_indirect_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_indirect_count_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready_promoted);
    MK_REQUIRE(!result.claimed_nanite_equivalence);
    MK_REQUIRE(!result.claimed_metal_readiness);
}

MK_TEST("vulkan mavg mesh shader lod executes indirect count mesh tasks when host supports it") {
    const auto task_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_TASK_SPV");
    const auto mesh_artifact = load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_MESH_SPV");
    const auto fragment_artifact =
        load_spirv_artifact_from_environment("MK_VULKAN_TEST_MAVG_MESH_SHADER_LOD_FRAGMENT_SPV");
    const auto row = make_valid_task_row();
    constexpr auto command_size = mirakana::rhi::vulkan::vulkan_mavg_mesh_shader_lod_indirect_command_size_bytes;

    const auto result = mirakana::rhi::vulkan::execute_vulkan_mavg_mesh_shader_lod(
        mirakana::rhi::vulkan::VulkanMavgMeshShaderLodDispatchDesc{
            .task_shader_spirv = std::span<const std::uint32_t>{task_artifact.words},
            .mesh_shader_spirv = std::span<const std::uint32_t>{mesh_artifact.words},
            .fragment_shader_spirv = std::span<const std::uint32_t>{fragment_artifact.words},
            .task_shader_entry_point = "task_main",
            .mesh_shader_entry_point = "mesh_main",
            .fragment_shader_entry_point = "fragment_main",
            .task_rows = std::span<const mirakana::rhi::vulkan::VulkanMavgMeshShaderLodTaskRow>{&row, 1},
            .request_indirect_count_draw = true,
            .indirect_buffer_size_bytes = command_size,
            .indirect_buffer_offset_bytes = 0,
            .indirect_draw_count = 1,
            .indirect_stride_bytes = command_size,
            .count_buffer_size_bytes = 4,
            .count_buffer_offset_bytes = 0,
            .max_indirect_count_draws = 1,
        });

    if (!task_artifact.configured || !mesh_artifact.configured || !fragment_artifact.configured || result.host_gated) {
        MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_indirect_count_ready);
        MK_REQUIRE(!result.mavg_mesh_shader_lod_ready_promoted);
        MK_REQUIRE(!result.diagnostic_text.empty());
        return;
    }

    MK_REQUIRE(result.succeeded);
    MK_REQUIRE(result.draw_mesh_tasks_direct_calls == 0U);
    MK_REQUIRE(result.draw_mesh_tasks_indirect_calls == 0U);
    MK_REQUIRE(result.draw_mesh_tasks_indirect_count_calls == 1U);
    MK_REQUIRE(result.draw_indirect_stage_barriers_recorded == 2U);
    MK_REQUIRE(result.task_shader_stage_barriers_recorded == 1U);
    MK_REQUIRE(result.mesh_shader_stage_barriers_recorded == 1U);
    MK_REQUIRE(result.shader_payload_consumed_by_task_or_mesh);
    MK_REQUIRE(result.last_indirect_count_buffer_value == 1U);
    MK_REQUIRE(result.last_indirect_executed_draw_count == 1U);
    MK_REQUIRE(result.readback_nonzero);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_vulkan_indirect_ready);
    MK_REQUIRE(result.mavg_mesh_shader_lod_vulkan_indirect_count_ready);
    MK_REQUIRE(!result.mavg_mesh_shader_lod_ready_promoted);
    MK_REQUIRE(!result.claimed_nanite_equivalence);
    MK_REQUIRE(!result.claimed_metal_readiness);
}

int main() {
    if (environment_variable_value("MK_VULKAN_MAVG_MESH_SHADER_LOD_PRINT_EVIDENCE") == "1") {
        return emit_mavg_vulkan_mesh_shader_lod_evidence();
    }
    return mirakana::test::run_all();
}
