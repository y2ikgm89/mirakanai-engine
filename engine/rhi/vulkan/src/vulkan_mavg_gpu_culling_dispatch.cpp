// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/vulkan/vulkan_mavg_gpu_culling_dispatch.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <vector>

namespace mirakana::rhi::vulkan {
namespace {

constexpr std::uint32_t k_indirect_count_buffer_size_bytes = 4U;
constexpr std::uint64_t k_uniform_buffer_alignment_bytes = 256U;

[[nodiscard]] bool valid_dispatch_desc(const VulkanMavgGpuCullingDispatchDesc& desc) noexcept {
    return desc.max_command_count > 0U && desc.record_stride_bytes >= 20U && (desc.record_stride_bytes % 4U) == 0U &&
           !desc.cluster_rows.empty() && !desc.compute_shader_spirv.empty();
}

[[nodiscard]] std::uint32_t
visible_cluster_count(std::span<const VulkanMavgGpuCullingDispatchClusterRow> rows) noexcept {
    return static_cast<std::uint32_t>(std::ranges::count_if(
        rows, [](const VulkanMavgGpuCullingDispatchClusterRow& row) { return row.visible != 0U; }));
}

[[nodiscard]] std::uint32_t
culled_cluster_count(std::span<const VulkanMavgGpuCullingDispatchClusterRow> rows) noexcept {
    return static_cast<std::uint32_t>(rows.size()) - visible_cluster_count(rows);
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

} // namespace

VulkanMavgGpuCullingDispatchResult
dispatch_mavg_gpu_culling_indirect(const VulkanMavgGpuCullingDispatchDesc& desc) noexcept {
    VulkanMavgGpuCullingDispatchResult result{};
    result.visible_cluster_count = visible_cluster_count(desc.cluster_rows);
    result.culled_cluster_count = culled_cluster_count(desc.cluster_rows);
    const auto argument_allocation_bytes =
        static_cast<std::uint64_t>(desc.max_command_count) * static_cast<std::uint64_t>(desc.record_stride_bytes);
    result.argument_buffer_size_bytes =
        static_cast<std::uint64_t>(result.visible_cluster_count) * static_cast<std::uint64_t>(desc.record_stride_bytes);

    if (!valid_dispatch_desc(desc) || result.visible_cluster_count == 0U) {
        result.failure_stage = 1U;
        return result;
    }
    if (result.visible_cluster_count > desc.max_command_count) {
        result.failure_stage = 2U;
        return result;
    }

    const auto spirv_byte_size = desc.compute_shader_spirv.size_bytes();
    const auto validation = validate_spirv_shader_artifact(VulkanSpirvShaderArtifactDesc{
        .stage = ShaderStage::compute,
        .bytecode = desc.compute_shader_spirv.data(),
        .bytecode_size = spirv_byte_size,
    });
    if (!validation.valid) {
        result.failure_stage = 3U;
        return result;
    }

    VulkanInstanceCreateDesc instance_desc;
    instance_desc.application_name = "GameEngineVulkanMavgGpuCullingDispatch";
    instance_desc.api_version = make_vulkan_api_version(1, 3);

    VulkanLogicalDeviceCreateDesc device_desc;
    device_desc.require_synchronization2 = true;
    device_desc.require_dynamic_rendering = true;
    device_desc.require_present_queue = false;

    auto device_result =
        create_runtime_device(VulkanLoaderProbeDesc{.host = current_rhi_host_platform()}, instance_desc, device_desc);
    if (!device_result.created) {
        return result;
    }

    auto& device = device_result.device;
    const auto cluster_count = static_cast<std::uint32_t>(desc.cluster_rows.size());
    const auto cluster_buffer_size =
        static_cast<std::uint64_t>(cluster_count) * vulkan_mavg_gpu_culling_dispatch_cluster_row_stride_bytes;

    auto cluster_device_buffer =
        create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                          .buffer =
                                              BufferDesc{
                                                  .size_bytes = cluster_buffer_size,
                                                  .usage = BufferUsage::storage | BufferUsage::copy_destination,
                                              },
                                          .memory_domain = VulkanBufferMemoryDomain::device_local,
                                      });
    auto cluster_upload_buffer = create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                                                   .buffer =
                                                                       BufferDesc{
                                                                           .size_bytes = cluster_buffer_size,
                                                                           .usage = BufferUsage::copy_source,
                                                                       },
                                                                   .memory_domain = VulkanBufferMemoryDomain::upload,
                                                               });
    auto argument_buffer = create_runtime_buffer(
        device, VulkanRuntimeBufferDesc{
                    .buffer =
                        BufferDesc{
                            .size_bytes = argument_allocation_bytes,
                            .usage = BufferUsage::storage | BufferUsage::indirect | BufferUsage::copy_source,
                        },
                    .memory_domain = VulkanBufferMemoryDomain::device_local,
                });
    auto count_buffer = create_runtime_buffer(
        device, VulkanRuntimeBufferDesc{
                    .buffer =
                        BufferDesc{
                            .size_bytes = k_indirect_count_buffer_size_bytes,
                            .usage = BufferUsage::storage | BufferUsage::indirect | BufferUsage::copy_source,
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
    auto argument_readback = create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                                               .buffer =
                                                                   BufferDesc{
                                                                       .size_bytes = argument_allocation_bytes,
                                                                       .usage = BufferUsage::copy_destination,
                                                                   },
                                                               .memory_domain = VulkanBufferMemoryDomain::readback,
                                                           });
    auto count_readback = create_runtime_buffer(device, VulkanRuntimeBufferDesc{
                                                            .buffer =
                                                                BufferDesc{
                                                                    .size_bytes = k_indirect_count_buffer_size_bytes,
                                                                    .usage = BufferUsage::copy_destination,
                                                                },
                                                            .memory_domain = VulkanBufferMemoryDomain::readback,
                                                        });

    if (!cluster_device_buffer.created || !cluster_upload_buffer.created || !argument_buffer.created ||
        !count_buffer.created || !constants_buffer.created || !argument_readback.created || !count_readback.created) {
        result.failure_stage = 4U;
        return result;
    }

    const auto cluster_bytes = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(desc.cluster_rows.data()), static_cast<std::size_t>(cluster_buffer_size));
    if (!write_runtime_buffer(device, cluster_upload_buffer.buffer,
                              VulkanRuntimeBufferWriteDesc{.byte_offset = 0, .bytes = cluster_bytes})
             .written) {
        result.failure_stage = 5U;
        return result;
    }

    struct Constants {
        std::uint32_t cluster_count;
        std::uint32_t max_command_count;
        std::uint32_t record_stride_bytes;
        std::uint32_t pad;
    };
    Constants constants{
        .cluster_count = cluster_count,
        .max_command_count = desc.max_command_count,
        .record_stride_bytes = desc.record_stride_bytes,
        .pad = 0U,
    };
    const auto constants_bytes =
        std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(&constants), sizeof(Constants));
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
            .type = DescriptorType::storage_buffer,
            .count = 1,
            .stages = ShaderStageVisibility::compute,
        },
        DescriptorBindingDesc{
            .binding = 3,
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
                                  cluster_device_buffer.buffer) ||
        !update_buffer_descriptor(device, descriptor_set.set, 1U, DescriptorType::storage_buffer,
                                  argument_buffer.buffer) ||
        !update_buffer_descriptor(device, descriptor_set.set, 2U, DescriptorType::storage_buffer,
                                  count_buffer.buffer) ||
        !update_buffer_descriptor(device, descriptor_set.set, 3U, DescriptorType::uniform_buffer,
                                  constants_buffer.buffer)) {
        result.failure_stage = 9U;
        return result;
    }

    auto command_pool = create_runtime_command_pool(device);
    if (!command_pool.created || !command_pool.pool.begin_primary_command_buffer()) {
        result.failure_stage = 10U;
        return result;
    }

    if (!record_runtime_buffer_copy(device, command_pool.pool, cluster_upload_buffer.buffer,
                                    cluster_device_buffer.buffer,
                                    VulkanRuntimeBufferCopyDesc{
                                        .region =
                                            BufferCopyRegion{
                                                .source_offset = 0,
                                                .destination_offset = 0,
                                                .size_bytes = cluster_buffer_size,
                                            },
                                    })
             .recorded) {
        result.failure_stage = 11U;
        return result;
    }

    constexpr std::uint32_t k_compute_pipeline_bind_point = 1U;
    if (!record_runtime_compute_pipeline_binding(device, command_pool.pool, compute_pipeline.pipeline).recorded ||
        !record_runtime_descriptor_set_binding(
             device, command_pool.pool, pipeline_layout.layout, descriptor_set.set,
             VulkanRuntimeDescriptorSetBindDesc{.first_set = 0U, .pipeline_bind_point = k_compute_pipeline_bind_point})
             .recorded) {
        result.failure_stage = 12U;
        return result;
    }

    if (!record_runtime_compute_dispatch(device, command_pool.pool,
                                         VulkanRuntimeComputeDispatchDesc{.group_count_x = 1})
             .recorded) {
        result.failure_stage = 13U;
        return result;
    }
    result.compute_dispatches = 1U;
    ++result.resource_barriers_recorded;

    // MavgGpuCullingSyncRequirement Vulkan row:
    // VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT + VK_ACCESS_2_SHADER_WRITE_BIT ->
    // VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT + VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT.
    constexpr std::uint64_t k_compute_shader_stage = 0x0000000000000800ULL;
    constexpr std::uint64_t k_shader_write_access = 0x0000000000000040ULL;
    constexpr std::uint64_t k_draw_indirect_stage = 0x0000000000002000ULL;
    constexpr std::uint64_t k_indirect_command_read_access = 0x0000000000000400ULL;
    constexpr std::uint64_t k_transfer_stage = 0x0000000000001000ULL;
    constexpr std::uint64_t k_transfer_read_access = 0x0000000000000800ULL;

    if (!record_buffer_barrier(device, command_pool.pool, argument_buffer.buffer, k_compute_shader_stage,
                               k_shader_write_access, k_draw_indirect_stage, k_indirect_command_read_access,
                               result.resource_barriers_recorded) ||
        !record_buffer_barrier(device, command_pool.pool, count_buffer.buffer, k_compute_shader_stage,
                               k_shader_write_access, k_draw_indirect_stage, k_indirect_command_read_access,
                               result.resource_barriers_recorded)) {
        result.failure_stage = 14U;
        return result;
    }

    if (!record_buffer_barrier(device, command_pool.pool, argument_buffer.buffer, k_draw_indirect_stage,
                               k_indirect_command_read_access, k_transfer_stage, k_transfer_read_access,
                               result.resource_barriers_recorded) ||
        !record_buffer_barrier(device, command_pool.pool, count_buffer.buffer, k_draw_indirect_stage,
                               k_indirect_command_read_access, k_transfer_stage, k_transfer_read_access,
                               result.resource_barriers_recorded)) {
        result.failure_stage = 15U;
        return result;
    }

    if (!record_runtime_buffer_copy(device, command_pool.pool, argument_buffer.buffer, argument_readback.buffer,
                                    VulkanRuntimeBufferCopyDesc{
                                        .region =
                                            BufferCopyRegion{
                                                .source_offset = 0,
                                                .destination_offset = 0,
                                                .size_bytes = argument_allocation_bytes,
                                            },
                                    })
             .recorded ||
        !record_runtime_buffer_copy(device, command_pool.pool, count_buffer.buffer, count_readback.buffer,
                                    VulkanRuntimeBufferCopyDesc{
                                        .region =
                                            BufferCopyRegion{
                                                .source_offset = 0,
                                                .destination_offset = 0,
                                                .size_bytes = k_indirect_count_buffer_size_bytes,
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

    const auto count_bytes = readback_bytes(device, count_readback.buffer, k_indirect_count_buffer_size_bytes);
    if (count_bytes.size() != k_indirect_count_buffer_size_bytes) {
        result.failure_stage = 20U;
        return result;
    }

    result.count_readback_bytes.resize(count_bytes.size());
    for (std::size_t index = 0; index < count_bytes.size(); ++index) {
        result.count_readback_bytes[index] = static_cast<std::uint8_t>(count_bytes[index]);
    }
    result.count_buffer_value =
        static_cast<std::uint32_t>(static_cast<unsigned char>(result.count_readback_bytes[0])) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(result.count_readback_bytes[1])) << 8U) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(result.count_readback_bytes[2])) << 16U) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(result.count_readback_bytes[3])) << 24U);
    result.argument_buffer_size_bytes =
        static_cast<std::uint64_t>(result.count_buffer_value) * static_cast<std::uint64_t>(desc.record_stride_bytes);

    const auto argument_bytes = readback_bytes(device, argument_readback.buffer, result.argument_buffer_size_bytes);
    if (argument_bytes.size() != result.argument_buffer_size_bytes) {
        result.failure_stage = 21U;
        return result;
    }

    result.argument_readback_bytes.resize(argument_bytes.size());
    for (std::size_t index = 0; index < argument_bytes.size(); ++index) {
        result.argument_readback_bytes[index] = static_cast<std::uint8_t>(argument_bytes[index]);
    }
    result.executed_gpu_culling = true;
    result.succeeded = true;
    return result;
}

} // namespace mirakana::rhi::vulkan
