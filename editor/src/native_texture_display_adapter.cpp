// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_texture_display_adapter.hpp"

#include "mirakana/renderer/rhi_viewport_surface.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <exception>
#include <memory>
#include <string>

namespace mirakana::editor {
namespace {

[[nodiscard]] Extent2D to_rhi_extent(ViewportExtent extent) noexcept {
    return Extent2D{.width = extent.width, .height = extent.height};
}

[[nodiscard]] bool valid_extent(ViewportExtent extent) noexcept {
    return extent.width != 0U && extent.height != 0U;
}

[[nodiscard]] rhi::FenceValue last_submitted_fence(const rhi::RhiStats& stats) noexcept {
    return rhi::FenceValue{.value = stats.last_submitted_fence_value, .queue = stats.last_submitted_fence_queue};
}

[[nodiscard]] bool vulkan_backend(std::string_view backend_id) noexcept {
    return backend_id == "vulkan";
}

} // namespace

struct NativeTextureDisplayAdapter::Impl {
    explicit Impl(NativeTextureDisplayAdapterDesc adapter_desc)
        : device(adapter_desc.device), extent(adapter_desc.extent),
          d3d12_host_available(adapter_desc.d3d12_host_available),
          vulkan_host_available(adapter_desc.vulkan_host_available),
          vulkan_validation_layer_ready(adapter_desc.vulkan_validation_layer_ready),
          vulkan_spirv_artifacts_available(adapter_desc.vulkan_spirv_artifacts_available),
          vulkan_synchronization2_ready(adapter_desc.vulkan_synchronization2_ready),
          renderer_output_available(adapter_desc.renderer_output_available),
          shader_artifacts_available(adapter_desc.shader_artifacts_available),
          gpu_payload_available(adapter_desc.gpu_payload_available), backend_id(adapter_desc.backend_id) {}

    rhi::IRhiDevice* device{nullptr};
    ViewportExtent extent{.width = 1280, .height = 720};
    bool d3d12_host_available{false};
    bool vulkan_host_available{false};
    bool vulkan_validation_layer_ready{false};
    bool vulkan_spirv_artifacts_available{false};
    bool vulkan_synchronization2_ready{false};
    bool renderer_output_available{true};
    bool shader_artifacts_available{true};
    bool gpu_payload_available{true};
    std::string backend_id{"d3d12"};
    bool resize_recreate_required{false};
    bool resize_safe_teardown_completed{true};
    std::unique_ptr<RhiViewportSurface> surface;
    rhi::DescriptorSetLayoutHandle sampled_texture_layout;
    rhi::DescriptorSetHandle sampled_texture_set;
    NativeTextureDisplayAdapterEvidence evidence;
    NativeTextureDisplayFrame display_frame;

    void resize(ViewportExtent next_extent) {
        if (next_extent.width == extent.width && next_extent.height == extent.height) {
            return;
        }
        extent = next_extent;
        resize_recreate_required = true;
        resize_safe_teardown_completed = false;
        if (device == nullptr || surface == nullptr) {
            resize_safe_teardown_completed = true;
            return;
        }

        const auto stats = device->stats();
        const auto fence = last_submitted_fence(stats);
        if (fence.value != 0U) {
            device->wait(fence);
        }
        surface->resize(to_rhi_extent(extent));
        resize_safe_teardown_completed = true;
    }

    void ensure_surface() {
        if (surface != nullptr) {
            return;
        }
        surface = std::make_unique<RhiViewportSurface>(RhiViewportSurfaceDesc{
            .device = device,
            .extent = to_rhi_extent(extent),
            .color_format = rhi::Format::rgba8_unorm,
            .wait_for_completion = true,
            .allow_native_display_interop = true,
        });
    }

    void ensure_descriptor_lease() {
        if (sampled_texture_layout.value != 0U && sampled_texture_set.value != 0U) {
            return;
        }
        sampled_texture_layout = device->create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
            rhi::DescriptorBindingDesc{
                .binding = 0,
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
        }});
        sampled_texture_set = device->allocate_descriptor_set(sampled_texture_layout);
    }

    [[nodiscard]] bool prepare_frame(std::uint64_t frame_index) {
        evidence = NativeTextureDisplayAdapterEvidence{
            .texture_adapter_available = false,
            .offscreen_target_available = false,
            .descriptor_lease_available = false,
            .resource_barriers_recorded = false,
            .fence_lifecycle_ready = false,
            .resize_safe_teardown_completed = resize_safe_teardown_completed,
            .resize_recreate_required = resize_recreate_required,
            .native_texture_handles_exposed = false,
            .extent = extent,
            .frame_index = frame_index,
            .frames_rendered = surface != nullptr ? surface->frames_rendered() : 0U,
            .descriptor_writes = 0U,
            .resource_transitions = 0U,
            .fence_waits = 0U,
            .diagnostic = {},
        };
        display_frame = NativeTextureDisplayFrame{};

        const bool is_vulkan = vulkan_backend(backend_id);
        const bool backend_host_available = is_vulkan ? vulkan_host_available : d3d12_host_available;
        const bool backend_gates_ready =
            !is_vulkan ||
            (vulkan_validation_layer_ready && vulkan_spirv_artifacts_available && vulkan_synchronization2_ready);
        if (!backend_host_available || !backend_gates_ready || device == nullptr || !valid_extent(extent)) {
            evidence.diagnostic =
                is_vulkan ? "native texture display adapter requires a Vulkan host, validation layer, SPIR-V "
                            "artifacts, synchronization2, RHI device, and non-zero extent"
                          : "native texture display adapter requires a D3D12 host, RHI device, and non-zero extent";
            return false;
        }

        const auto before = device->stats();
        try {
            ensure_surface();
            surface->render_clear_frame();
            const auto display = surface->prepare_display_frame();
            display_frame = NativeTextureDisplayFrame{
                .available = display.texture.value != 0U,
                .texture = display.texture,
                .extent = ViewportExtent{.width = display.extent.width, .height = display.extent.height},
                .format = display.format,
                .frame_index = frame_index,
                .frames_rendered = surface->frames_rendered(),
            };
            ensure_descriptor_lease();
            device->update_descriptor_set(rhi::DescriptorWrite{
                .set = sampled_texture_set,
                .binding = 0,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, display.texture)},
            });
        } catch (const std::exception& error) {
            evidence.diagnostic = error.what();
            return false;
        }

        const auto after = device->stats();
        evidence.texture_adapter_available = true;
        evidence.offscreen_target_available = surface != nullptr && surface->color_texture().value != 0U;
        evidence.descriptor_lease_available =
            sampled_texture_set.value != 0U && after.descriptor_writes > before.descriptor_writes;
        evidence.resource_barriers_recorded = after.resource_transitions > before.resource_transitions;
        evidence.fence_lifecycle_ready = after.fences_signaled > before.fences_signaled &&
                                         after.fence_waits > before.fence_waits &&
                                         after.last_completed_fence_value >= after.last_submitted_fence_value;
        evidence.resize_safe_teardown_completed = resize_safe_teardown_completed;
        evidence.frames_rendered = surface != nullptr ? surface->frames_rendered() : 0U;
        evidence.descriptor_writes = after.descriptor_writes - before.descriptor_writes;
        evidence.resource_transitions = after.resource_transitions - before.resource_transitions;
        evidence.fence_waits = after.fence_waits - before.fence_waits;
        evidence.diagnostic = evidence.offscreen_target_available && evidence.descriptor_lease_available &&
                                      evidence.resource_barriers_recorded && evidence.fence_lifecycle_ready
                                  ? "native private RHI texture display frame prepared"
                                  : "native private RHI texture display evidence is incomplete";
        return evidence.offscreen_target_available && evidence.descriptor_lease_available &&
               evidence.resource_barriers_recorded && evidence.fence_lifecycle_ready;
    }

    [[nodiscard]] NativeViewportDisplayPlan viewport_plan(std::uint64_t frame_index) {
        const bool prepared = prepare_frame(frame_index);
        return plan_native_viewport_display(NativeViewportDisplayDesc{
            .d3d12_host_available = d3d12_host_available,
            .vulkan_host_available = vulkan_host_available,
            .vulkan_validation_layer_ready = vulkan_validation_layer_ready,
            .vulkan_spirv_artifacts_available = vulkan_spirv_artifacts_available,
            .vulkan_synchronization2_ready = vulkan_synchronization2_ready,
            .renderer_output_available = renderer_output_available && prepared,
            .texture_display_requested = true,
            .texture_adapter_available = evidence.texture_adapter_available,
            .offscreen_target_available = evidence.offscreen_target_available,
            .descriptor_lease_available = evidence.descriptor_lease_available,
            .resource_barriers_recorded = evidence.resource_barriers_recorded,
            .fence_lifecycle_ready = evidence.fence_lifecycle_ready,
            .resize_safe_teardown_completed = evidence.resize_safe_teardown_completed,
            .resize_recreate_required = evidence.resize_recreate_required,
            .extent = extent,
            .frame_index = frame_index,
            .backend_id = backend_id,
        });
    }

    [[nodiscard]] NativeMaterialPreviewDisplayPlan material_plan(std::uint64_t frame_index) {
        const bool prepared = prepare_frame(frame_index);
        return plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = d3d12_host_available,
            .vulkan_host_available = vulkan_host_available,
            .vulkan_validation_layer_ready = vulkan_validation_layer_ready,
            .vulkan_spirv_artifacts_available = vulkan_spirv_artifacts_available,
            .vulkan_synchronization2_ready = vulkan_synchronization2_ready,
            .shader_artifacts_available = shader_artifacts_available,
            .gpu_payload_available = gpu_payload_available,
            .texture_display_requested = true,
            .texture_adapter_available = evidence.texture_adapter_available,
            .offscreen_target_available = evidence.offscreen_target_available,
            .descriptor_lease_available = evidence.descriptor_lease_available,
            .resource_barriers_recorded = evidence.resource_barriers_recorded,
            .fence_lifecycle_ready = evidence.fence_lifecycle_ready,
            .frame_index = frame_index,
            .backend_id = backend_id,
            .frames_rendered = prepared ? evidence.frames_rendered : 0U,
            .executes = prepared,
        });
    }
};

NativeTextureDisplayAdapter::NativeTextureDisplayAdapter(NativeTextureDisplayAdapterDesc desc)
    : impl_(std::make_unique<Impl>(desc)) {}

NativeTextureDisplayAdapter::~NativeTextureDisplayAdapter() = default;

NativeTextureDisplayAdapter::NativeTextureDisplayAdapter(NativeTextureDisplayAdapter&&) noexcept = default;

NativeTextureDisplayAdapter& NativeTextureDisplayAdapter::operator=(NativeTextureDisplayAdapter&&) noexcept = default;

void NativeTextureDisplayAdapter::resize(ViewportExtent extent) {
    impl_->resize(extent);
}

NativeViewportDisplayPlan NativeTextureDisplayAdapter::render_viewport_frame(std::uint64_t frame_index) {
    return impl_->viewport_plan(frame_index);
}

NativeMaterialPreviewDisplayPlan NativeTextureDisplayAdapter::render_material_preview_frame(std::uint64_t frame_index) {
    return impl_->material_plan(frame_index);
}

const NativeTextureDisplayAdapterEvidence& NativeTextureDisplayAdapter::evidence() const noexcept {
    return impl_->evidence;
}

NativeTextureDisplayFrame NativeTextureDisplayAdapter::display_frame() const noexcept {
    return impl_->display_frame;
}

} // namespace mirakana::editor
