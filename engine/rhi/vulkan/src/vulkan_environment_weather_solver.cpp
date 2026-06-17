// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/vulkan/vulkan_environment_weather_solver.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <vector>

namespace mirakana::rhi::vulkan {
namespace {

constexpr std::uint64_t fnv_offset_basis = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;
constexpr std::uint64_t k_uniform_buffer_alignment_bytes = 256U;
constexpr std::uint32_t k_descriptor_binding_count = 3U;

// Vulkan 1.3 synchronization2 bit values used by the backend-private barrier wrapper.
constexpr std::uint64_t k_compute_shader_stage = 0x0000000000000800ULL;
constexpr std::uint64_t k_transfer_stage = 0x0000000000001000ULL;
constexpr std::uint64_t k_shader_read_access = 0x0000000000000020ULL;
constexpr std::uint64_t k_shader_write_access = 0x0000000000000040ULL;
constexpr std::uint64_t k_transfer_read_access = 0x0000000000000800ULL;
constexpr std::uint64_t k_transfer_write_access = 0x0000000000001000ULL;

static_assert(sizeof(VulkanEnvironmentWeatherSolverCellRow) == vulkan_environment_weather_solver_cell_row_stride_bytes);
static_assert(sizeof(VulkanEnvironmentWeatherSolverOutputRow) ==
              vulkan_environment_weather_solver_output_row_stride_bytes);

[[nodiscard]] bool valid_desc(const VulkanEnvironmentWeatherSolverDesc& desc) noexcept {
    return !desc.compute_shader_spirv.empty() && !desc.compute_shader_entry_point.empty() && !desc.cell_rows.empty() &&
           desc.effective_timestep_s > 0.0F && desc.air_pressure_hpa > 0.0F && desc.mixing_height_m > 0.0F &&
           desc.cell_rows.size() <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max());
}

[[nodiscard]] bool record_buffer_barrier(VulkanRuntimeDevice& device, VulkanRuntimeCommandPool& command_pool,
                                         VulkanRuntimeBuffer& buffer, std::uint64_t src_stage, std::uint64_t src_access,
                                         std::uint64_t dst_stage, std::uint64_t dst_access,
                                         std::uint32_t& barrier_count) noexcept {
    const auto barrier = record_runtime_buffer_memory_barrier2(device, command_pool,
                                                               VulkanRuntimeBufferMemoryBarrierDesc{
                                                                   .buffer = &buffer,
                                                                   .src_stage_mask = src_stage,
                                                                   .src_access_mask = src_access,
                                                                   .dst_stage_mask = dst_stage,
                                                                   .dst_access_mask = dst_access,
                                                               });
    if (!barrier.recorded) {
        return false;
    }
    barrier_count += barrier.barrier_count;
    return true;
}

[[nodiscard]] std::vector<std::byte> readback_bytes(VulkanRuntimeDevice& device, VulkanRuntimeBuffer& readback,
                                                    std::uint64_t byte_count) {
    const auto read =
        read_runtime_buffer(device, readback, VulkanRuntimeBufferReadDesc{.byte_offset = 0, .byte_count = byte_count});
    if (!read.read) {
        return {};
    }
    return read.bytes;
}

[[nodiscard]] bool update_buffer_descriptor(VulkanRuntimeDevice& device, VulkanRuntimeDescriptorSet& descriptor_set,
                                            std::uint32_t binding, DescriptorType type,
                                            VulkanRuntimeBuffer& buffer) noexcept {
    const auto write = update_runtime_descriptor_set(device, descriptor_set,
                                                     VulkanRuntimeDescriptorWriteDesc{
                                                         .binding = binding,
                                                         .array_element = 0,
                                                         .buffers =
                                                             {
                                                                 VulkanRuntimeDescriptorBufferResource{
                                                                     .type = type,
                                                                     .buffer = &buffer,
                                                                 },
                                                             },
                                                     });
    return write.updated;
}

[[nodiscard]] std::uint64_t hash_bytes(std::span<const std::byte> bytes) noexcept {
    std::uint64_t hash = fnv_offset_basis;
    for (const auto byte : bytes) {
        hash ^= static_cast<std::uint8_t>(byte);
        hash *= fnv_prime;
    }
    return hash == 0U ? fnv_offset_basis : hash;
}

[[nodiscard]] bool any_nonzero(std::span<const std::byte> bytes) noexcept {
    return std::ranges::any_of(bytes, [](const std::byte byte) { return byte != std::byte{0}; });
}

[[nodiscard]] std::vector<VulkanEnvironmentWeatherSolverOutputRow>
decode_output_rows(std::span<const std::byte> bytes) {
    std::vector<VulkanEnvironmentWeatherSolverOutputRow> rows;
    if (bytes.empty() || (bytes.size() % sizeof(VulkanEnvironmentWeatherSolverOutputRow)) != 0U) {
        return rows;
    }
    rows.resize(bytes.size() / sizeof(VulkanEnvironmentWeatherSolverOutputRow));
    std::memcpy(rows.data(), bytes.data(), bytes.size());
    return rows;
}

} // namespace

VulkanEnvironmentWeatherSolverResult
dispatch_environment_weather_solver(const VulkanEnvironmentWeatherSolverDesc& desc) noexcept {
    VulkanEnvironmentWeatherSolverResult result{};
    result.cell_count = static_cast<std::uint32_t>(desc.cell_rows.size());
    result.output_buffer_size_bytes =
        static_cast<std::uint64_t>(result.cell_count) * vulkan_environment_weather_solver_output_row_stride_bytes;

    if (!valid_desc(desc)) {
        result.failure_stage = 1U;
        return result;
    }

    const auto spirv_byte_size = desc.compute_shader_spirv.size_bytes();
    const auto validation = validate_spirv_shader_artifact(VulkanSpirvShaderArtifactDesc{
        .stage = ShaderStage::compute,
        .bytecode = desc.compute_shader_spirv.data(),
        .bytecode_size = spirv_byte_size,
    });
    if (!validation.valid) {
        result.failure_stage = 2U;
        return result;
    }

    VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanEnvironmentWeatherSolver";
    instance_desc.api_version = make_vulkan_api_version(1, 3);

    VulkanLogicalDeviceCreateDesc device_desc;
    device_desc.require_synchronization2 = true;
    device_desc.require_dynamic_rendering = true;
    device_desc.require_present_queue = false;

    auto device_result =
        create_runtime_device(VulkanLoaderProbeDesc{.host = current_rhi_host_platform()}, instance_desc, device_desc);
    if (!device_result.created) {
        result.failure_stage = 3U;
        return result;
    }

    auto& device = device_result.device;
    const auto input_buffer_size =
        static_cast<std::uint64_t>(result.cell_count) * vulkan_environment_weather_solver_cell_row_stride_bytes;
    const auto output_buffer_size = result.output_buffer_size_bytes;

    auto input_device_buffer =
        create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                          .buffer =
                                              BufferDesc{
                                                  .size_bytes = input_buffer_size,
                                                  .usage = BufferUsage::storage | BufferUsage::copy_destination,
                                              },
                                          .memory_domain = VulkanBufferMemoryDomain::device_local,
                                      });
    auto input_upload_buffer = create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                                                 .buffer =
                                                                     BufferDesc{
                                                                         .size_bytes = input_buffer_size,
                                                                         .usage = BufferUsage::copy_source,
                                                                     },
                                                                 .memory_domain = VulkanBufferMemoryDomain::upload,
                                                             });
    auto output_buffer =
        create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                          .buffer =
                                              BufferDesc{
                                                  .size_bytes = output_buffer_size,
                                                  .usage = BufferUsage::storage | BufferUsage::copy_source,
                                              },
                                          .memory_domain = VulkanBufferMemoryDomain::device_local,
                                      });
    auto constants_buffer =
        create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                          .buffer =
                                              BufferDesc{
                                                  .size_bytes = k_uniform_buffer_alignment_bytes,
                                                  .usage = BufferUsage::uniform | BufferUsage::copy_source,
                                              },
                                          .memory_domain = VulkanBufferMemoryDomain::upload,
                                      });
    auto output_readback = create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                                             .buffer =
                                                                 BufferDesc{
                                                                     .size_bytes = output_buffer_size,
                                                                     .usage = BufferUsage::copy_destination,
                                                                 },
                                                             .memory_domain = VulkanBufferMemoryDomain::readback,
                                                         });

    if (!input_device_buffer.created || !input_upload_buffer.created || !output_buffer.created ||
        !constants_buffer.created || !output_readback.created) {
        result.failure_stage = 4U;
        return result;
    }

    const auto input_bytes = std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(desc.cell_rows.data()),
                                                           static_cast<std::size_t>(input_buffer_size));
    if (!write_runtime_buffer(device, input_upload_buffer.buffer,
                              VulkanRuntimeBufferWriteDesc{.byte_offset = 0, .bytes = input_bytes})
             .written) {
        result.failure_stage = 5U;
        return result;
    }

    struct Constants {
        std::uint32_t cell_count;
        float effective_timestep_s;
        float air_pressure_hpa;
        float mixing_height_m;
    };
    const Constants constants{
        .cell_count = result.cell_count,
        .effective_timestep_s = desc.effective_timestep_s,
        .air_pressure_hpa = desc.air_pressure_hpa,
        .mixing_height_m = desc.mixing_height_m,
    };
    const auto constants_bytes =
        std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(&constants), sizeof(constants));
    if (!write_runtime_buffer(device, constants_buffer.buffer,
                              VulkanRuntimeBufferWriteDesc{.byte_offset = 0, .bytes = constants_bytes})
             .written) {
        result.failure_stage = 6U;
        return result;
    }

    auto shader_module = create_runtime_shader_module(device, VulkanRuntimeShaderModuleDesc{
                                                                  .stage = ShaderStage::compute,
                                                                  .bytecode = desc.compute_shader_spirv.data(),
                                                                  .bytecode_size = spirv_byte_size,
                                                              });
    if (!shader_module.created) {
        result.failure_stage = 7U;
        return result;
    }

    DescriptorSetLayoutDesc descriptor_layout_desc;
    descriptor_layout_desc.bindings = {
        DescriptorBindingDesc{
            .binding = 0,
            .type = DescriptorType::storage_buffer,
            .count = 1,
            .stages = ShaderStageVisibility::compute,
        },
        DescriptorBindingDesc{
            .binding = 1,
            .type = DescriptorType::storage_buffer,
            .count = 1,
            .stages = ShaderStageVisibility::compute,
        },
        DescriptorBindingDesc{
            .binding = 2,
            .type = DescriptorType::uniform_buffer,
            .count = 1,
            .stages = ShaderStageVisibility::compute,
        },
    };
    auto descriptor_layout = create_runtime_descriptor_set_layout(
        device, VulkanRuntimeDescriptorSetLayoutDesc{.layout = descriptor_layout_desc});
    auto descriptor_set = create_runtime_descriptor_set(device, descriptor_layout.layout);
    auto pipeline_layout = create_runtime_pipeline_layout(
        device, VulkanRuntimePipelineLayoutDesc{.descriptor_set_layouts = {&descriptor_layout.layout}});
    auto compute_pipeline = create_runtime_compute_pipeline(
        device, pipeline_layout.layout, shader_module.module,
        VulkanRuntimeComputePipelineDesc{.entry_point = desc.compute_shader_entry_point});
    if (!descriptor_layout.created || !descriptor_set.created || !pipeline_layout.created ||
        !compute_pipeline.created) {
        result.failure_stage = 8U;
        return result;
    }

    if (!update_buffer_descriptor(device, descriptor_set.set, 0U, DescriptorType::storage_buffer,
                                  input_device_buffer.buffer) ||
        !update_buffer_descriptor(device, descriptor_set.set, 1U, DescriptorType::storage_buffer,
                                  output_buffer.buffer) ||
        !update_buffer_descriptor(device, descriptor_set.set, 2U, DescriptorType::uniform_buffer,
                                  constants_buffer.buffer)) {
        result.failure_stage = 9U;
        return result;
    }
    result.descriptor_set_bindings = k_descriptor_binding_count;

    auto command_pool = create_runtime_command_pool(device);
    if (!command_pool.created || !command_pool.pool.begin_primary_command_buffer()) {
        result.failure_stage = 10U;
        return result;
    }

    if (!record_runtime_buffer_copy(device, command_pool.pool, input_upload_buffer.buffer, input_device_buffer.buffer,
                                    VulkanRuntimeBufferCopyDesc{
                                        .region =
                                            BufferCopyRegion{
                                                .source_offset = 0,
                                                .destination_offset = 0,
                                                .size_bytes = input_buffer_size,
                                            },
                                    })
             .recorded) {
        result.failure_stage = 11U;
        return result;
    }

    if (!record_buffer_barrier(device, command_pool.pool, input_device_buffer.buffer, k_transfer_stage,
                               k_transfer_write_access, k_compute_shader_stage, k_shader_read_access,
                               result.resource_barriers_recorded)) {
        result.failure_stage = 12U;
        return result;
    }

    constexpr std::uint32_t k_compute_pipeline_bind_point = 1U;
    if (!record_runtime_compute_pipeline_binding(device, command_pool.pool, compute_pipeline.pipeline).recorded ||
        !record_runtime_descriptor_set_binding(
             device, command_pool.pool, pipeline_layout.layout, descriptor_set.set,
             VulkanRuntimeDescriptorSetBindDesc{.first_set = 0U, .pipeline_bind_point = k_compute_pipeline_bind_point})
             .recorded) {
        result.failure_stage = 13U;
        return result;
    }

    const auto workgroups = (result.cell_count + 63U) / 64U;
    if (!record_runtime_compute_dispatch(device, command_pool.pool,
                                         VulkanRuntimeComputeDispatchDesc{.group_count_x = workgroups})
             .recorded) {
        result.failure_stage = 14U;
        return result;
    }
    result.compute_dispatches = 1U;

    if (!record_buffer_barrier(device, command_pool.pool, output_buffer.buffer, k_compute_shader_stage,
                               k_shader_write_access, k_transfer_stage, k_transfer_read_access,
                               result.resource_barriers_recorded)) {
        result.failure_stage = 15U;
        return result;
    }

    if (!record_runtime_buffer_copy(device, command_pool.pool, output_buffer.buffer, output_readback.buffer,
                                    VulkanRuntimeBufferCopyDesc{
                                        .region =
                                            BufferCopyRegion{
                                                .source_offset = 0,
                                                .destination_offset = 0,
                                                .size_bytes = output_buffer_size,
                                            },
                                    })
             .recorded) {
        result.failure_stage = 16U;
        return result;
    }

    if (!command_pool.pool.end_primary_command_buffer()) {
        result.failure_stage = 17U;
        return result;
    }

    auto frame_sync =
        create_runtime_frame_sync(device, VulkanRuntimeFrameSyncDesc{.create_image_available_semaphore = false,
                                                                     .create_render_finished_semaphore = false,
                                                                     .create_in_flight_fence = true,
                                                                     .start_in_flight_fence_signaled = false});
    if (!frame_sync.created) {
        result.failure_stage = 18U;
        return result;
    }

    const auto submit =
        submit_runtime_command_buffer(device, command_pool.pool, frame_sync.sync,
                                      VulkanRuntimeCommandBufferSubmitDesc{.wait_image_available_semaphore = false,
                                                                           .signal_render_finished_semaphore = false,
                                                                           .signal_in_flight_fence = true,
                                                                           .wait_for_graphics_queue_idle = true});
    if (!submit.submitted) {
        result.failure_stage = 19U;
        return result;
    }

    const auto bytes = readback_bytes(device, output_readback.buffer, output_buffer_size);
    if (bytes.size() != static_cast<std::size_t>(output_buffer_size)) {
        result.failure_stage = 20U;
        return result;
    }

    result.output_readback_nonzero = any_nonzero(bytes);
    result.output_checksum = hash_bytes(bytes);
    result.output_rows = decode_output_rows(bytes);
    result.executed_gpu_solver = true;
    result.succeeded = result.output_readback_nonzero && result.output_rows.size() == desc.cell_rows.size();
    return result;
}

} // namespace mirakana::rhi::vulkan
