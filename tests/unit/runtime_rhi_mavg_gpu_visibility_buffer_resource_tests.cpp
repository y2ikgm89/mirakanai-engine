// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime_rhi/mavg_gpu_visibility_buffer_resource.hpp"

#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] mirakana::MavgGpuVisibilityBufferLayoutPlan make_ready_layout_plan() {
    const auto graph_asset = mirakana::AssetId::from_name("mavg/visibility-buffer-rhi-allocation");

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

[[nodiscard]] mirakana::MavgGpuVisibilityBufferLayoutPlan make_zero_slot_layout_plan() {
    return mirakana::MavgGpuVisibilityBufferLayoutPlan{
        .layout_rows = {mirakana::MavgGpuVisibilityBufferLayoutRow{}},
        .source_packet_count = 0,
        .slot_count = 0,
        .byte_range_count = 0,
        .layout_row_count = 1,
        .sync_intent_row_count = 0,
        .slot_record_stride_bytes = 64,
        .slot_record_alignment_bytes = 16,
        .slot_buffer_size_bytes = 0,
    };
}

class ZeroHandleRhiDevice final : public mirakana::rhi::IRhiDevice {
  public:
    [[nodiscard]] mirakana::rhi::BackendKind backend_kind() const noexcept override {
        return mirakana::rhi::BackendKind::null;
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return "zero-handle-test";
    }

    [[nodiscard]] mirakana::rhi::RhiStats stats() const noexcept override {
        return stats_;
    }

    [[nodiscard]] std::uint64_t gpu_timestamp_ticks_per_second() const noexcept override {
        return 0;
    }

    [[nodiscard]] mirakana::rhi::RhiDeviceMemoryDiagnostics memory_diagnostics() const override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::BufferHandle create_buffer(const mirakana::rhi::BufferDesc& /*desc*/) override {
        ++stats_.buffers_created;
        return {};
    }

    [[nodiscard]] mirakana::rhi::TextureHandle create_texture(const mirakana::rhi::TextureDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::SamplerHandle create_sampler(const mirakana::rhi::SamplerDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::SwapchainHandle
    create_swapchain(const mirakana::rhi::SwapchainDesc& /*desc*/) override {
        return {};
    }

    void resize_swapchain(mirakana::rhi::SwapchainHandle /*swapchain*/, mirakana::rhi::Extent2D /*extent*/) override {}

    [[nodiscard]] mirakana::rhi::SwapchainFrameHandle
    acquire_swapchain_frame(mirakana::rhi::SwapchainHandle /*swapchain*/) override {
        return {};
    }

    void release_swapchain_frame(mirakana::rhi::SwapchainFrameHandle /*frame*/) override {}

    [[nodiscard]] mirakana::rhi::TransientBuffer
    acquire_transient_buffer(const mirakana::rhi::BufferDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::TransientTexture
    acquire_transient_texture(const mirakana::rhi::TextureDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::TransientTextureAliasGroup
    acquire_transient_texture_alias_group(const mirakana::rhi::TextureDesc& /*desc*/,
                                          std::size_t /*texture_count*/) override {
        return {};
    }

    void release_transient(mirakana::rhi::TransientResourceHandle /*lease*/) override {}

    [[nodiscard]] mirakana::rhi::ShaderHandle create_shader(const mirakana::rhi::ShaderDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::DescriptorSetLayoutHandle
    create_descriptor_set_layout(const mirakana::rhi::DescriptorSetLayoutDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::DescriptorSetHandle
    allocate_descriptor_set(mirakana::rhi::DescriptorSetLayoutHandle /*layout*/) override {
        return {};
    }

    void update_descriptor_set(const mirakana::rhi::DescriptorWrite& /*write*/) override {}

    [[nodiscard]] mirakana::rhi::PipelineLayoutHandle
    create_pipeline_layout(const mirakana::rhi::PipelineLayoutDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::GraphicsPipelineHandle
    create_graphics_pipeline(const mirakana::rhi::GraphicsPipelineDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] mirakana::rhi::ComputePipelineHandle
    create_compute_pipeline(const mirakana::rhi::ComputePipelineDesc& /*desc*/) override {
        return {};
    }

    [[nodiscard]] std::unique_ptr<mirakana::rhi::IRhiCommandList>
    begin_command_list(mirakana::rhi::QueueKind /*queue*/) override {
        return nullptr;
    }

    [[nodiscard]] mirakana::rhi::FenceValue submit(mirakana::rhi::IRhiCommandList& /*commands*/) override {
        return {};
    }

    void wait(mirakana::rhi::FenceValue /*fence*/) override {}

    void wait_for_queue(mirakana::rhi::QueueKind /*queue*/, mirakana::rhi::FenceValue /*fence*/) override {}

    void write_buffer(mirakana::rhi::BufferHandle /*buffer*/, std::uint64_t /*offset*/,
                      std::span<const std::uint8_t> /*bytes*/) override {}

    [[nodiscard]] std::vector<std::uint8_t> read_buffer(mirakana::rhi::BufferHandle /*buffer*/,
                                                        std::uint64_t /*offset*/,
                                                        std::uint64_t /*size_bytes*/) override {
        return {};
    }

  private:
    mirakana::rhi::RhiStats stats_{};
};

[[nodiscard]] bool
has_diagnostic(const mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceResult& result,
               mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode code) noexcept {
    return mirakana::runtime_rhi::has_runtime_mavg_gpu_visibility_buffer_resource_diagnostic(result, code);
}

} // namespace

MK_TEST("runtime rhi mavg visibility buffer resource allocates null rhi storage readback buffer") {
    const auto layout = make_ready_layout_plan();
    mirakana::rhi::NullRhiDevice device;
    const auto buffers_before = device.stats().buffers_created;

    const auto result = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &layout,
        });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.resource_rows.size() == 1U);
    MK_REQUIRE(result.layout_slot_count == 2U);
    MK_REQUIRE(result.allocated_buffer_count == 1U);
    MK_REQUIRE(result.allocated_buffer_size_bytes == 128U);
    MK_REQUIRE(result.device_buffers_created_before == buffers_before);
    MK_REQUIRE(result.device_buffers_created_after == buffers_before + 1U);
    MK_REQUIRE(result.resource_rows[0].visibility_buffer.value != 0U);
    MK_REQUIRE(result.resource_rows[0].buffer_desc.size_bytes == layout.slot_buffer_size_bytes);
    MK_REQUIRE(result.resource_rows[0].owner_device == &device);
    MK_REQUIRE(mirakana::rhi::has_flag(result.resource_rows[0].buffer_desc.usage, mirakana::rhi::BufferUsage::storage));
    MK_REQUIRE(
        mirakana::rhi::has_flag(result.resource_rows[0].buffer_desc.usage, mirakana::rhi::BufferUsage::copy_source));
    MK_REQUIRE(result.resource_rows[0].slot_count == 2U);
    MK_REQUIRE(result.resource_rows[0].slot_buffer_size_bytes == 128U);
    MK_REQUIRE(result.resource_rows[0].ready_for_readback_proof);
    MK_REQUIRE(result.resource_rows[0].storage_usage_ready);
    MK_REQUIRE(result.resource_rows[0].copy_source_usage_ready);
    MK_REQUIRE(result.allocated_rhi_resources);
    MK_REQUIRE(result.owner_device == &device);
    MK_REQUIRE(result.ready_for_readback_proof);
    MK_REQUIRE(!result.wrote_gpu_visibility_buffer);
    MK_REQUIRE(!result.created_descriptor_bindings);
    MK_REQUIRE(!result.recorded_resource_barriers);
    MK_REQUIRE(!result.submitted_gpu_work);
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
}

MK_TEST("runtime rhi mavg visibility buffer resource handles missing invalid and zero slot layouts") {
    mirakana::rhi::NullRhiDevice device;
    const auto missing = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{.device = &device});

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing, mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_layout_plan));
    MK_REQUIRE(!missing.allocated_rhi_resources);
    MK_REQUIRE(device.stats().buffers_created == 0U);

    auto invalid_layout = make_ready_layout_plan();
    invalid_layout.diagnostics.push_back(mirakana::MavgGpuVisibilityBufferLayoutDiagnostic{
        .code = mirakana::MavgGpuVisibilityBufferLayoutDiagnosticCode::invalid_visible_cluster_packet_plan,
        .graph_asset = invalid_layout.slots[0].graph_asset,
        .message = "invalid upstream packet plan",
    });
    const auto invalid = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &invalid_layout,
        });

    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(has_diagnostic(
        invalid, mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_layout_plan));
    MK_REQUIRE(!invalid.allocated_rhi_resources);
    MK_REQUIRE(device.stats().buffers_created == 0U);

    const auto zero_layout = make_zero_slot_layout_plan();
    const auto rejected_zero = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &zero_layout,
        });
    MK_REQUIRE(!rejected_zero.succeeded());
    MK_REQUIRE(has_diagnostic(
        rejected_zero,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::zero_slot_buffer_disallowed));
    MK_REQUIRE(device.stats().buffers_created == 0U);

    const auto allowed_zero = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &zero_layout,
            .allow_zero_slot_buffer = true,
        });
    MK_REQUIRE(allowed_zero.succeeded());
    MK_REQUIRE(allowed_zero.resource_rows.empty());
    MK_REQUIRE(allowed_zero.layout_slot_count == 0U);
    MK_REQUIRE(allowed_zero.allocated_buffer_count == 0U);
    MK_REQUIRE(!allowed_zero.allocated_rhi_resources);
    MK_REQUIRE(device.stats().buffers_created == 0U);
}

MK_TEST("runtime rhi mavg visibility buffer resource rejects invalid size and usage before allocation") {
    mirakana::rhi::NullRhiDevice device;
    auto invalid_size_layout = make_ready_layout_plan();
    invalid_size_layout.slot_buffer_size_bytes = 0;

    const auto invalid_size = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &invalid_size_layout,
        });

    MK_REQUIRE(!invalid_size.succeeded());
    MK_REQUIRE(has_diagnostic(
        invalid_size,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_slot_buffer_size));
    MK_REQUIRE(device.stats().buffers_created == 0U);

    const auto layout = make_ready_layout_plan();
    const auto exceeded = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &layout,
            .max_buffer_size_bytes = 64,
        });

    MK_REQUIRE(!exceeded.succeeded());
    MK_REQUIRE(has_diagnostic(
        exceeded,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::max_buffer_size_exceeded));
    MK_REQUIRE(device.stats().buffers_created == 0U);

    const auto missing_storage = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &layout,
            .visibility_buffer_usage = mirakana::rhi::BufferUsage::copy_source,
        });

    MK_REQUIRE(!missing_storage.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing_storage,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_storage_usage));
    MK_REQUIRE(device.stats().buffers_created == 0U);

    const auto missing_copy_source = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &device,
            .layout_plan = &layout,
            .visibility_buffer_usage = mirakana::rhi::BufferUsage::storage,
        });

    MK_REQUIRE(!missing_copy_source.succeeded());
    MK_REQUIRE(has_diagnostic(missing_copy_source,
                              mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::
                                  missing_copy_source_usage_for_readback));
    MK_REQUIRE(device.stats().buffers_created == 0U);
}

MK_TEST("runtime rhi mavg visibility buffer resource rejects missing device and zero allocated handle") {
    const auto layout = make_ready_layout_plan();
    const auto missing_device = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{.layout_plan = &layout});

    MK_REQUIRE(!missing_device.succeeded());
    MK_REQUIRE(has_diagnostic(
        missing_device,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::missing_rhi_device));
    MK_REQUIRE(!missing_device.allocated_rhi_resources);

    ZeroHandleRhiDevice zero_handle_device;
    const auto zero_handle = mirakana::runtime_rhi::create_runtime_mavg_gpu_visibility_buffer_resource(
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDesc{
            .device = &zero_handle_device,
            .layout_plan = &layout,
        });

    MK_REQUIRE(!zero_handle.succeeded());
    MK_REQUIRE(has_diagnostic(
        zero_handle,
        mirakana::runtime_rhi::RuntimeMavgGpuVisibilityBufferResourceDiagnosticCode::invalid_allocated_buffer_handle));
    MK_REQUIRE(zero_handle.resource_rows.empty());
    MK_REQUIRE(!zero_handle.allocated_rhi_resources);
    MK_REQUIRE(zero_handle_device.stats().buffers_created == 1U);
}

int main() {
    return mirakana::test::run_all();
}
