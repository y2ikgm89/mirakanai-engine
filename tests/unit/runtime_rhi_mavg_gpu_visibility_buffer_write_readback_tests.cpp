// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp"
#include "mirakana/runtime_rhi/mavg_gpu_visibility_buffer_write_readback.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgGpuVisibilityBufferLayoutPlan make_ready_layout_plan() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/visibility-buffer-rhi-write-readback");

    return mirakana::MavgGpuVisibilityBufferLayoutPlan{
        .slots =
            {
                mirakana::MavgGpuVisibilityBufferSlotRow{
                    .graph_asset = graph_asset,
                    .slot_index = 0,
                    .packet_index = 0,
                    .indirect_command_index = 4,
                    .cluster_index = 3,
                    .page_index = 1,
                    .lod_level = 0,
                    .material_partition = 2,
                    .fallback_substitution = false,
                    .meshlet_ready = true,
                    .payload_page_byte_offset = 128,
                    .payload_page_byte_size = 64,
                    .slot_byte_offset = 0,
                    .slot_byte_size = 64,
                    .cluster_key = 0x100,
                },
                mirakana::MavgGpuVisibilityBufferSlotRow{
                    .graph_asset = graph_asset,
                    .slot_index = 1,
                    .packet_index = 1,
                    .indirect_command_index = 7,
                    .cluster_index = 5,
                    .page_index = 2,
                    .lod_level = 1,
                    .material_partition = 2,
                    .fallback_substitution = true,
                    .meshlet_ready = true,
                    .payload_page_byte_offset = 192,
                    .payload_page_byte_size = 64,
                    .slot_byte_offset = 64,
                    .slot_byte_size = 64,
                    .cluster_key = 0x101,
                },
            },
        .byte_ranges =
            {
                mirakana::MavgGpuVisibilityBufferByteRangeRow{.slot_index = 0, .byte_offset = 0, .byte_size = 64},
                mirakana::MavgGpuVisibilityBufferByteRangeRow{.slot_index = 1, .byte_offset = 64, .byte_size = 64},
            },
        .layout_rows = {mirakana::MavgGpuVisibilityBufferLayoutRow{}},
        .sync_intents = {mirakana::MavgGpuVisibilityBufferSyncIntentRow{}},
        .diagnostics = {},
        .source_packet_count = 2,
        .slot_count = 2,
        .byte_range_count = 2,
        .layout_row_count = 1,
        .sync_intent_row_count = 1,
        .meshlet_ready_slot_count = 2,
        .fallback_slot_count = 1,
        .slot_record_stride_bytes = 64,
        .slot_record_alignment_bytes = 16,
        .slot_buffer_size_bytes = 128,
        .payload_byte_span_offset = 128,
        .payload_byte_span_size = 128,
    };
}

[[nodiscard]] mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceResult
make_ready_resource_result(mirakana::rhi::IRhiDevice& device, const mirakana::MavgGpuVisibilityBufferLayoutPlan& layout,
                           mirakana::rhi::BufferUsage usage = mirakana::rhi::BufferUsage::storage |
                                                              mirakana::rhi::BufferUsage::copy_source |
                                                              mirakana::rhi::BufferUsage::copy_destination) {
    return mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &layout,
            .visibility_buffer_usage = usage,
        });
}

[[nodiscard]] bool
has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackResult& result,
               mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode code) noexcept {
    return mirakana::runtime_rhi::has_runtime_mavg_gpu_visibility_buffer_write_readback_diagnostic(result, code);
}

[[nodiscard]] std::uint32_t read_u32_le(const std::vector<std::uint8_t>& bytes, std::uint64_t offset) noexcept {
    return static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(offset + 0U)]) |
           (static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(offset + 1U)]) << 8U) |
           (static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(offset + 2U)]) << 16U) |
           (static_cast<std::uint32_t>(bytes[static_cast<std::size_t>(offset + 3U)]) << 24U);
}

[[nodiscard]] std::uint64_t read_u64_le(const std::vector<std::uint8_t>& bytes, std::uint64_t offset) noexcept {
    std::uint64_t value = 0;
    for (std::uint32_t byte_index = 0; byte_index < 8U; ++byte_index) {
        value |= static_cast<std::uint64_t>(bytes[static_cast<std::size_t>(offset + byte_index)]) << (byte_index * 8U);
    }
    return value;
}

class FaultingRhiDevice final : public mirakana::rhi::IRhiDevice {
  public:
    enum class Mode { submit_fails, readback_fails, readback_mismatch };

    explicit FaultingRhiDevice(Mode mode) : mode_{mode} {}

    [[nodiscard]] mirakana::rhi::BackendKind backend_kind() const noexcept override {
        return delegate_.backend_kind();
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return "faulting-null-rhi";
    }

    [[nodiscard]] mirakana::rhi::RhiStats stats() const noexcept override {
        return delegate_.stats();
    }

    [[nodiscard]] std::uint64_t gpu_timestamp_ticks_per_second() const noexcept override {
        return delegate_.gpu_timestamp_ticks_per_second();
    }

    [[nodiscard]] mirakana::rhi::RhiDeviceMemoryDiagnostics memory_diagnostics() const override {
        return delegate_.memory_diagnostics();
    }

    [[nodiscard]] mirakana::rhi::RhiResidencyActionResult
    execute_residency_action(const mirakana::rhi::RhiResidencyActionDesc& desc) override {
        return delegate_.execute_residency_action(desc);
    }

    [[nodiscard]] const mirakana::rhi::RhiResourceLifetimeRegistry*
    resource_lifetime_registry() const noexcept override {
        return delegate_.resource_lifetime_registry();
    }

    [[nodiscard]] mirakana::rhi::BufferHandle create_buffer(const mirakana::rhi::BufferDesc& desc) override {
        return delegate_.create_buffer(desc);
    }

    [[nodiscard]] mirakana::rhi::TextureHandle create_texture(const mirakana::rhi::TextureDesc& desc) override {
        return delegate_.create_texture(desc);
    }

    [[nodiscard]] mirakana::rhi::SamplerHandle create_sampler(const mirakana::rhi::SamplerDesc& desc) override {
        return delegate_.create_sampler(desc);
    }

    [[nodiscard]] mirakana::rhi::SwapchainHandle create_swapchain(const mirakana::rhi::SwapchainDesc& desc) override {
        return delegate_.create_swapchain(desc);
    }

    void resize_swapchain(mirakana::rhi::SwapchainHandle swapchain, mirakana::rhi::Extent2D extent) override {
        delegate_.resize_swapchain(swapchain, extent);
    }

    [[nodiscard]] mirakana::rhi::SwapchainFrameHandle
    acquire_swapchain_frame(mirakana::rhi::SwapchainHandle swapchain) override {
        return delegate_.acquire_swapchain_frame(swapchain);
    }

    void release_swapchain_frame(mirakana::rhi::SwapchainFrameHandle frame) override {
        delegate_.release_swapchain_frame(frame);
    }

    [[nodiscard]] mirakana::rhi::TransientBuffer
    acquire_transient_buffer(const mirakana::rhi::BufferDesc& desc) override {
        return delegate_.acquire_transient_buffer(desc);
    }

    [[nodiscard]] mirakana::rhi::TransientTexture
    acquire_transient_texture(const mirakana::rhi::TextureDesc& desc) override {
        return delegate_.acquire_transient_texture(desc);
    }

    [[nodiscard]] mirakana::rhi::TransientTextureAliasGroup
    acquire_transient_texture_alias_group(const mirakana::rhi::TextureDesc& desc, std::size_t texture_count) override {
        return delegate_.acquire_transient_texture_alias_group(desc, texture_count);
    }

    void release_transient(mirakana::rhi::TransientResourceHandle lease) override {
        delegate_.release_transient(lease);
    }

    [[nodiscard]] mirakana::rhi::ShaderHandle create_shader(const mirakana::rhi::ShaderDesc& desc) override {
        return delegate_.create_shader(desc);
    }

    [[nodiscard]] mirakana::rhi::DescriptorSetLayoutHandle
    create_descriptor_set_layout(const mirakana::rhi::DescriptorSetLayoutDesc& desc) override {
        return delegate_.create_descriptor_set_layout(desc);
    }

    [[nodiscard]] mirakana::rhi::DescriptorSetHandle
    allocate_descriptor_set(mirakana::rhi::DescriptorSetLayoutHandle layout) override {
        return delegate_.allocate_descriptor_set(layout);
    }

    void update_descriptor_set(const mirakana::rhi::DescriptorWrite& write) override {
        delegate_.update_descriptor_set(write);
    }

    [[nodiscard]] mirakana::rhi::PipelineLayoutHandle
    create_pipeline_layout(const mirakana::rhi::PipelineLayoutDesc& desc) override {
        return delegate_.create_pipeline_layout(desc);
    }

    [[nodiscard]] mirakana::rhi::GraphicsPipelineHandle
    create_graphics_pipeline(const mirakana::rhi::GraphicsPipelineDesc& desc) override {
        return delegate_.create_graphics_pipeline(desc);
    }

    [[nodiscard]] mirakana::rhi::ComputePipelineHandle
    create_compute_pipeline(const mirakana::rhi::ComputePipelineDesc& desc) override {
        return delegate_.create_compute_pipeline(desc);
    }

    [[nodiscard]] std::unique_ptr<mirakana::rhi::IRhiCommandList>
    begin_command_list(mirakana::rhi::QueueKind queue) override {
        return delegate_.begin_command_list(queue);
    }

    [[nodiscard]] mirakana::rhi::FenceValue submit(mirakana::rhi::IRhiCommandList& commands) override {
        if (mode_ == Mode::submit_fails) {
            throw std::logic_error("faulting rhi submit failed");
        }
        return delegate_.submit(commands);
    }

    void wait(mirakana::rhi::FenceValue fence) override {
        delegate_.wait(fence);
    }

    void wait_for_queue(mirakana::rhi::QueueKind queue, mirakana::rhi::FenceValue fence) override {
        delegate_.wait_for_queue(queue, fence);
    }

    void write_buffer(mirakana::rhi::BufferHandle buffer, std::uint64_t offset,
                      std::span<const std::uint8_t> bytes) override {
        delegate_.write_buffer(buffer, offset, bytes);
    }

    [[nodiscard]] std::vector<std::uint8_t> read_buffer(mirakana::rhi::BufferHandle buffer, std::uint64_t offset,
                                                        std::uint64_t size_bytes) override {
        if (mode_ == Mode::readback_fails) {
            throw std::logic_error("faulting rhi readback failed");
        }
        auto bytes = delegate_.read_buffer(buffer, offset, size_bytes);
        if (mode_ == Mode::readback_mismatch && !bytes.empty()) {
            bytes.front() ^= 0xffU;
        }
        return bytes;
    }

  private:
    Mode mode_{Mode::submit_fails};
    mirakana::rhi::NullRhiDevice delegate_;
};

} // namespace

MK_TEST("runtime rhi mavg visibility buffer write readback copies deterministic records through null rhi") {
    auto layout = make_ready_layout_plan();
    mirakana::rhi::NullRhiDevice device;
    auto resource = make_ready_resource_result(device, layout);
    MK_REQUIRE(resource.succeeded());

    const auto stats_before = device.stats();
    const auto result = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .layout_plan = &layout,
            .resource_result = &resource,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.rows.size() == 1U);
    MK_REQUIRE(result.encoded_record_count == 2U);
    MK_REQUIRE(result.encoded_record_stride_bytes == 64U);
    MK_REQUIRE(result.encoded_byte_count == 128U);
    MK_REQUIRE(result.uploaded_byte_count == 128U);
    MK_REQUIRE(result.readback_byte_count == 128U);
    MK_REQUIRE(result.copy_command_count == 2U);
    MK_REQUIRE(result.submitted_copy_work);
    MK_REQUIRE(result.submitted_copy_fence.value != 0U);
    MK_REQUIRE(result.readback_bytes_match_encoded_records);
    MK_REQUIRE(result.wrote_gpu_visibility_buffer);
    MK_REQUIRE(result.rows[0].upload_buffer.value != 0U);
    MK_REQUIRE(result.rows[0].visibility_buffer.value == resource.resource_rows[0].visibility_buffer.value);
    MK_REQUIRE(result.rows[0].readback_buffer.value != 0U);
    MK_REQUIRE(result.rows[0].encoded_byte_count == 128U);
    MK_REQUIRE(result.rows[0].copy_command_count == 2U);
    MK_REQUIRE(result.rows[0].readback_bytes_match_encoded_records);
    MK_REQUIRE(result.encoded_bytes == result.readback_bytes);
    MK_REQUIRE(result.encoded_bytes.size() == 128U);
    MK_REQUIRE(read_u32_le(result.encoded_bytes, 0) == 0U);
    MK_REQUIRE(read_u32_le(result.encoded_bytes, 4) == 0U);
    MK_REQUIRE(read_u64_le(result.encoded_bytes, 8) == 0x100U);
    MK_REQUIRE(read_u64_le(result.encoded_bytes, 16) == 128U);
    MK_REQUIRE(read_u64_le(result.encoded_bytes, 24) == 64U);
    MK_REQUIRE(read_u32_le(result.encoded_bytes, 32) == 2U);
    MK_REQUIRE(read_u32_le(result.encoded_bytes, 64) == 1U);
    MK_REQUIRE(read_u32_le(result.encoded_bytes, 68) == 1U);
    MK_REQUIRE(read_u64_le(result.encoded_bytes, 72) == 0x101U);
    MK_REQUIRE(read_u64_le(result.encoded_bytes, 80) == 192U);
    MK_REQUIRE(read_u64_le(result.encoded_bytes, 88) == 64U);
    MK_REQUIRE(read_u32_le(result.encoded_bytes, 96) == 3U);
    for (std::uint64_t offset = 36; offset < 64; ++offset) {
        MK_REQUIRE(result.encoded_bytes[static_cast<std::size_t>(offset)] == 0U);
    }
    for (std::uint64_t offset = 100; offset < 128; ++offset) {
        MK_REQUIRE(result.encoded_bytes[static_cast<std::size_t>(offset)] == 0U);
    }
    MK_REQUIRE(device.stats().buffers_created == stats_before.buffers_created + 2U);
    MK_REQUIRE(device.stats().buffer_writes == stats_before.buffer_writes + 1U);
    MK_REQUIRE(device.stats().buffer_reads == stats_before.buffer_reads + 1U);
    MK_REQUIRE(device.stats().buffer_copies == stats_before.buffer_copies + 2U);
    MK_REQUIRE(device.stats().command_lists_submitted == stats_before.command_lists_submitted + 1U);
    MK_REQUIRE(!result.created_descriptor_bindings);
    MK_REQUIRE(!result.created_compute_pipeline);
    MK_REQUIRE(!result.dispatched_compute_shader);
    MK_REQUIRE(!result.executed_gpu_traversal);
    MK_REQUIRE(!result.executed_mesh_shader);
    MK_REQUIRE(!result.executed_indirect_draw);
    MK_REQUIRE(!result.used_gpu_decompression);
    MK_REQUIRE(!result.enforced_allocator_budget);
    MK_REQUIRE(!result.exposed_native_handles);
    MK_REQUIRE(!result.claimed_vulkan_parity);
    MK_REQUIRE(!result.claimed_metal_parity);
    MK_REQUIRE(!result.claimed_nanite_compatibility);
    MK_REQUIRE(!result.claimed_benchmark_superiority);
    MK_REQUIRE(!result.claimed_async_overlap_or_performance);
}

MK_TEST("runtime rhi mavg visibility buffer write readback rejects missing invalid and mismatched inputs") {
    auto layout = make_ready_layout_plan();
    mirakana::rhi::NullRhiDevice device;
    auto resource = make_ready_resource_result(device, layout);
    MK_REQUIRE(resource.succeeded());

    const auto missing_device = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .layout_plan = &layout,
            .resource_result = &resource,
        });
    MK_REQUIRE(!missing_device.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing_device,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_rhi_device));

    const auto missing_layout = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .resource_result = &resource,
        });
    MK_REQUIRE(!missing_layout.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing_layout,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_layout_plan));

    auto invalid_layout = layout;
    invalid_layout.slot_count = 1;
    const auto invalid_layout_result = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .layout_plan = &invalid_layout,
            .resource_result = &resource,
        });
    MK_REQUIRE(!invalid_layout_result.succeeded());
    MK_REQUIRE(has_diagnostic(
        invalid_layout_result,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_layout_plan));

    const auto missing_resource = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .layout_plan = &layout,
        });
    MK_REQUIRE(!missing_resource.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing_resource,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_resource_result));

    auto invalid_resource = resource;
    invalid_resource.diagnostics.push_back(mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnostic{
        .code = mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_layout_plan,
        .graph_asset = layout.slots.front().graph_asset,
        .message = "invalid resource",
    });
    const auto invalid_resource_result =
        mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
            mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
                .device = &device,
                .layout_plan = &layout,
                .resource_result = &invalid_resource,
            });
    MK_REQUIRE(!invalid_resource_result.succeeded());
    MK_REQUIRE(has_diagnostic(
        invalid_resource_result,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::invalid_resource_result));

    auto missing_row_resource = resource;
    missing_row_resource.resource_rows.clear();
    const auto missing_row = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .layout_plan = &layout,
            .resource_result = &missing_row_resource,
        });
    MK_REQUIRE(!missing_row.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing_row,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::missing_resource_row));

    auto zero_handle_resource = resource;
    zero_handle_resource.resource_rows[0].visibility_buffer = {};
    const auto zero_handle = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .layout_plan = &layout,
            .resource_result = &zero_handle_resource,
        });
    MK_REQUIRE(!zero_handle.succeeded());
    MK_REQUIRE(has_diagnostic(zero_handle,
                              mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::
                                  invalid_visibility_buffer_handle));

    mirakana::rhi::NullRhiDevice other_device;
    const auto owner_mismatch = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &other_device,
            .layout_plan = &layout,
            .resource_result = &resource,
        });
    MK_REQUIRE(!owner_mismatch.succeeded());
    MK_REQUIRE(has_diagnostic(
        owner_mismatch,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::resource_device_mismatch));
}

MK_TEST("runtime rhi mavg visibility buffer write readback rejects usage size and byte range gaps") {
    auto layout = make_ready_layout_plan();
    mirakana::rhi::NullRhiDevice device;

    auto missing_copy_source_resource = make_ready_resource_result(
        device, layout, mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_destination);
    MK_REQUIRE(!missing_copy_source_resource.succeeded());
    missing_copy_source_resource = mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceResult{
        .resource_rows = {mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceRow{
            .graph_asset = layout.slots.front().graph_asset,
            .visibility_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
                .size_bytes = layout.slot_buffer_size_bytes,
                .usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_destination,
            }),
            .buffer_desc =
                mirakana::rhi::BufferDesc{
                    .size_bytes = layout.slot_buffer_size_bytes,
                    .usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_destination,
                },
            .owner_device = &device,
            .slot_count = layout.slot_count,
            .byte_range_count = layout.byte_range_count,
            .slot_buffer_size_bytes = layout.slot_buffer_size_bytes,
            .storage_usage_ready = true,
            .ready_for_readback_proof = false,
        }},
        .layout_slot_count = layout.slot_count,
        .layout_byte_range_count = layout.byte_range_count,
        .allocated_buffer_count = 1,
        .allocated_buffer_size_bytes = layout.slot_buffer_size_bytes,
        .owner_device = &device,
        .allocated_rhi_resources = true,
    };

    const auto missing_copy_source = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .layout_plan = &layout,
            .resource_result = &missing_copy_source_resource,
        });
    MK_REQUIRE(!missing_copy_source.succeeded());
    MK_REQUIRE(has_diagnostic(missing_copy_source,
                              mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::
                                  missing_copy_source_usage_for_readback_copy));

    auto missing_copy_destination_resource = make_ready_resource_result(
        device, layout, mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_source);
    MK_REQUIRE(missing_copy_destination_resource.succeeded());
    const auto missing_copy_destination =
        mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
            mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
                .device = &device,
                .layout_plan = &layout,
                .resource_result = &missing_copy_destination_resource,
            });
    MK_REQUIRE(!missing_copy_destination.succeeded());
    MK_REQUIRE(has_diagnostic(missing_copy_destination,
                              mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::
                                  missing_copy_destination_usage_for_staged_write));

    auto resource = make_ready_resource_result(device, layout);
    MK_REQUIRE(resource.succeeded());
    const auto buffers_before_exceeded = device.stats().buffers_created;
    const auto exceeded = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .layout_plan = &layout,
            .resource_result = &resource,
            .max_write_size_bytes = 64,
        });
    MK_REQUIRE(!exceeded.succeeded());
    MK_REQUIRE(has_diagnostic(
        exceeded,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::max_write_size_exceeded));
    MK_REQUIRE(device.stats().buffers_created == buffers_before_exceeded);

    auto overflow_layout = layout;
    overflow_layout.byte_ranges[1].byte_offset = 96;
    overflow_layout.slots[1].slot_byte_offset = 96;
    const auto overflow = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &device,
            .layout_plan = &overflow_layout,
            .resource_result = &resource,
        });
    MK_REQUIRE(!overflow.succeeded());
    MK_REQUIRE(has_diagnostic(
        overflow,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::slot_byte_range_overflow));
}

MK_TEST("runtime rhi mavg visibility buffer write readback reports rhi operation failures") {
    auto layout = make_ready_layout_plan();

    FaultingRhiDevice submit_device{FaultingRhiDevice::Mode::submit_fails};
    auto submit_resource = make_ready_resource_result(submit_device, layout);
    MK_REQUIRE(submit_resource.succeeded());
    const auto submit_failed = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &submit_device,
            .layout_plan = &layout,
            .resource_result = &submit_resource,
        });
    MK_REQUIRE(!submit_failed.succeeded());
    MK_REQUIRE(has_diagnostic(
        submit_failed,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_submit_failed));

    FaultingRhiDevice readback_device{FaultingRhiDevice::Mode::readback_fails};
    auto readback_resource = make_ready_resource_result(readback_device, layout);
    MK_REQUIRE(readback_resource.succeeded());
    const auto readback_failed = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &readback_device,
            .layout_plan = &layout,
            .resource_result = &readback_resource,
        });
    MK_REQUIRE(!readback_failed.succeeded());
    MK_REQUIRE(has_diagnostic(
        readback_failed,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::rhi_readback_failed));

    FaultingRhiDevice mismatch_device{FaultingRhiDevice::Mode::readback_mismatch};
    auto mismatch_resource = make_ready_resource_result(mismatch_device, layout);
    MK_REQUIRE(mismatch_resource.succeeded());
    const auto mismatch = mirakana::runtime_rhi::execute_runtime_mavg_gpu_visibility_buffer_write_readback(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDesc{
            .device = &mismatch_device,
            .layout_plan = &layout,
            .resource_result = &mismatch_resource,
        });
    MK_REQUIRE(!mismatch.succeeded());
    MK_REQUIRE(has_diagnostic(
        mismatch, mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferWriteReadbackDiagnosticCode::readback_mismatch));
    MK_REQUIRE(mismatch.encoded_bytes.size() == 128U);
    MK_REQUIRE(mismatch.readback_bytes.size() == 128U);
    MK_REQUIRE(mismatch.encoded_bytes != mismatch.readback_bytes);
}

int main() {
    return mirakana::test::run_all();
}
