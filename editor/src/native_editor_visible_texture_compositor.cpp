// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_visible_texture_compositor.hpp"

#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] bool valid_extent(ViewportExtent extent) noexcept {
    return extent.width != 0U && extent.height != 0U;
}

[[nodiscard]] bool has_shader(std::string_view entry_point, std::span<const std::uint8_t> bytecode) noexcept {
    return !entry_point.empty() && !bytecode.empty();
}

[[nodiscard]] rhi::Extent2D to_rhi_extent(ViewportExtent extent) noexcept {
    return rhi::Extent2D{.width = extent.width, .height = extent.height};
}

[[nodiscard]] rhi::ViewportDesc to_rhi_viewport(ViewportExtent extent) noexcept {
    return rhi::ViewportDesc{
        .x = 0.0F,
        .y = 0.0F,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .min_depth = 0.0F,
        .max_depth = 1.0F,
    };
}

[[nodiscard]] rhi::ScissorRectDesc to_rhi_scissor(ViewportExtent extent) noexcept {
    return rhi::ScissorRectDesc{.x = 0U, .y = 0U, .width = extent.width, .height = extent.height};
}

} // namespace

struct NativeEditorVisibleTextureCompositor::Impl {
    explicit Impl(NativeEditorVisibleTextureCompositorDesc compositor_desc)
        : device(compositor_desc.device), swapchain(compositor_desc.swapchain), extent(compositor_desc.extent),
          vertex_shader_entry_point(compositor_desc.vertex_shader_entry_point),
          vertex_shader_bytecode(compositor_desc.vertex_shader_bytecode.begin(),
                                 compositor_desc.vertex_shader_bytecode.end()),
          fragment_shader_entry_point(compositor_desc.fragment_shader_entry_point),
          fragment_shader_bytecode(compositor_desc.fragment_shader_bytecode.begin(),
                                   compositor_desc.fragment_shader_bytecode.end()),
          backend_id(compositor_desc.backend_id) {}

    rhi::IRhiDevice* device{nullptr};
    rhi::SwapchainHandle swapchain;
    ViewportExtent extent{.width = 1280, .height = 720};
    std::string vertex_shader_entry_point{"vs_main"};
    std::vector<std::uint8_t> vertex_shader_bytecode;
    std::string fragment_shader_entry_point{"ps_main"};
    std::vector<std::uint8_t> fragment_shader_bytecode;
    std::string backend_id{"d3d12"};
    rhi::DescriptorSetLayoutHandle sampled_texture_layout;
    rhi::DescriptorSetHandle sampled_texture_set;
    rhi::SamplerHandle sampler;
    rhi::PipelineLayoutHandle pipeline_layout;
    rhi::ShaderHandle vertex_shader;
    rhi::ShaderHandle fragment_shader;
    rhi::GraphicsPipelineHandle pipeline;
    NativeEditorVisibleTextureCompositorEvidence evidence;

    void reset_evidence() {
        evidence = NativeEditorVisibleTextureCompositorEvidence{
            .compositor_available = false,
            .visible_panel_available = false,
            .swapchain_frame_acquired = false,
            .sampled_texture_descriptor_bound = false,
            .render_pass_recorded = false,
            .draw_recorded = false,
            .present_recorded = false,
            .fence_waited = false,
            .native_texture_handles_exposed = false,
            .visible_texture_composites = 0U,
            .descriptor_writes = 0U,
            .descriptor_sets_bound = 0U,
            .render_passes = 0U,
            .draw_calls = 0U,
            .present_calls = 0U,
            .fence_waits = 0U,
            .diagnostic = {},
        };
    }

    [[nodiscard]] bool valid_request() {
        if (device == nullptr || swapchain.value == 0U || !valid_extent(extent) ||
            !has_shader(vertex_shader_entry_point, vertex_shader_bytecode) ||
            !has_shader(fragment_shader_entry_point, fragment_shader_bytecode)) {
            evidence.diagnostic =
                "native visible texture compositor requires device, swapchain, extent, and shader bytecode";
            return false;
        }
        return true;
    }

    void ensure_pipeline() {
        if (pipeline.value != 0U) {
            return;
        }
        sampled_texture_layout = device->create_descriptor_set_layout(rhi::DescriptorSetLayoutDesc{{
            rhi::DescriptorBindingDesc{
                .binding = 0,
                .type = rhi::DescriptorType::sampled_texture,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
            rhi::DescriptorBindingDesc{
                .binding = 1,
                .type = rhi::DescriptorType::sampler,
                .count = 1,
                .stages = rhi::ShaderStageVisibility::fragment,
            },
        }});
        sampled_texture_set = device->allocate_descriptor_set(sampled_texture_layout);
        sampler = device->create_sampler(rhi::SamplerDesc{
            .min_filter = rhi::SamplerFilter::linear,
            .mag_filter = rhi::SamplerFilter::linear,
            .address_u = rhi::SamplerAddressMode::clamp_to_edge,
            .address_v = rhi::SamplerAddressMode::clamp_to_edge,
            .address_w = rhi::SamplerAddressMode::clamp_to_edge,
        });
        pipeline_layout = device->create_pipeline_layout(rhi::PipelineLayoutDesc{
            .descriptor_sets = {sampled_texture_layout},
            .push_constant_bytes = 0U,
        });
        vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = vertex_shader_entry_point,
            .bytecode_size = vertex_shader_bytecode.size(),
            .bytecode = vertex_shader_bytecode.data(),
        });
        fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = fragment_shader_entry_point,
            .bytecode_size = fragment_shader_bytecode.size(),
            .bytecode = fragment_shader_bytecode.data(),
        });
        pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = rhi::PrimitiveTopology::triangle_list,
            .vertex_buffers = {},
            .vertex_attributes = {},
            .depth_state = {},
        });
    }

    [[nodiscard]] bool composite(NativeTextureDisplayFrame frame) {
        if (!valid_request()) {
            return false;
        }
        if (!frame.available || frame.texture.value == 0U || !valid_extent(frame.extent)) {
            evidence.diagnostic = "native visible texture compositor requires a prepared sampled texture frame";
            return false;
        }

        const auto before = device->stats();
        rhi::SwapchainFrameHandle swapchain_frame;
        bool frame_acquired = false;
        bool present_recorded = false;
        try {
            ensure_pipeline();
            device->update_descriptor_set(rhi::DescriptorWrite{
                .set = sampled_texture_set,
                .binding = 0,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::texture(rhi::DescriptorType::sampled_texture, frame.texture)},
            });
            device->update_descriptor_set(rhi::DescriptorWrite{
                .set = sampled_texture_set,
                .binding = 1,
                .array_element = 0,
                .resources = {rhi::DescriptorResource::sampler(sampler)},
            });

            swapchain_frame = device->acquire_swapchain_frame(swapchain);
            frame_acquired = true;
            auto commands = device->begin_command_list(rhi::QueueKind::graphics);
            commands->begin_render_pass(rhi::RenderPassDesc{
                .color =
                    rhi::RenderPassColorAttachment{
                        .texture = {},
                        .load_action = rhi::LoadAction::clear,
                        .store_action = rhi::StoreAction::store,
                        .swapchain_frame = swapchain_frame,
                        .clear_color =
                            rhi::ClearColorValue{.red = 0.03F, .green = 0.035F, .blue = 0.04F, .alpha = 1.0F},
                    },
                .depth = {},
            });
            commands->set_viewport(to_rhi_viewport(extent));
            commands->set_scissor(to_rhi_scissor(extent));
            commands->bind_graphics_pipeline(pipeline);
            commands->bind_descriptor_set(pipeline_layout, 0U, sampled_texture_set);
            commands->draw(3U, 1U);
            commands->end_render_pass();
            commands->present(swapchain_frame);
            present_recorded = true;
            commands->close();
            const auto fence = device->submit(*commands);
            device->wait(fence);
        } catch (const std::exception& error) {
            auto diagnostic = std::string{error.what()};
            if (frame_acquired && !present_recorded) {
                try {
                    device->release_swapchain_frame(swapchain_frame);
                } catch (const std::exception& release_error) {
                    diagnostic += "; swapchain frame release failed: ";
                    diagnostic += release_error.what();
                }
            }
            evidence.diagnostic = diagnostic;
            return false;
        }

        const auto after = device->stats();
        evidence.compositor_available = true;
        evidence.visible_panel_available = true;
        evidence.swapchain_frame_acquired = after.swapchain_frames_acquired > before.swapchain_frames_acquired;
        evidence.sampled_texture_descriptor_bound = after.descriptor_sets_bound > before.descriptor_sets_bound;
        evidence.render_pass_recorded = after.render_passes_begun > before.render_passes_begun;
        evidence.draw_recorded = after.draw_calls > before.draw_calls;
        evidence.present_recorded = after.present_calls > before.present_calls;
        evidence.fence_waited = after.fence_waits > before.fence_waits;
        evidence.visible_texture_composites = evidence.present_recorded ? 1U : 0U;
        evidence.descriptor_writes = after.descriptor_writes - before.descriptor_writes;
        evidence.descriptor_sets_bound = after.descriptor_sets_bound - before.descriptor_sets_bound;
        evidence.render_passes = after.render_passes_begun - before.render_passes_begun;
        evidence.draw_calls = after.draw_calls - before.draw_calls;
        evidence.present_calls = after.present_calls - before.present_calls;
        evidence.fence_waits = after.fence_waits - before.fence_waits;
        evidence.diagnostic = evidence.present_recorded
                                  ? "native visible compositor sampled private texture into the editor swapchain"
                                  : "native visible compositor did not present a sampled texture";
        return evidence.swapchain_frame_acquired && evidence.sampled_texture_descriptor_bound &&
               evidence.render_pass_recorded && evidence.draw_recorded && evidence.present_recorded &&
               evidence.fence_waited;
    }

    [[nodiscard]] NativeViewportDisplayPlan render_viewport_frame(NativeTextureDisplayAdapter& adapter,
                                                                  std::uint64_t frame_index) {
        reset_evidence();
        const auto prepared = adapter.render_viewport_frame(frame_index);
        const auto& adapter_evidence = adapter.evidence();
        const bool composited = prepared.accepted && composite(adapter.display_frame());
        return plan_native_viewport_display(NativeViewportDisplayDesc{
            .d3d12_host_available = prepared.d3d12_host_available,
            .renderer_output_available = prepared.renderer_output_available,
            .texture_display_requested = prepared.texture_display_requested,
            .texture_adapter_available = adapter_evidence.texture_adapter_available,
            .offscreen_target_available = adapter_evidence.offscreen_target_available,
            .descriptor_lease_available = adapter_evidence.descriptor_lease_available,
            .resource_barriers_recorded = adapter_evidence.resource_barriers_recorded,
            .fence_lifecycle_ready = adapter_evidence.fence_lifecycle_ready,
            .resize_safe_teardown_completed = adapter_evidence.resize_safe_teardown_completed,
            .resize_recreate_required = adapter_evidence.resize_recreate_required,
            .visible_panel_available = evidence.visible_panel_available,
            .visible_texture_composite_recorded = composited,
            .visible_texture_composites = evidence.visible_texture_composites,
            .extent = adapter_evidence.extent,
            .frame_index = frame_index,
            .backend_id = backend_id,
        });
    }

    [[nodiscard]] NativeMaterialPreviewDisplayPlan render_material_preview_frame(NativeTextureDisplayAdapter& adapter,
                                                                                 std::uint64_t frame_index) {
        reset_evidence();
        const auto prepared = adapter.render_material_preview_frame(frame_index);
        const auto& adapter_evidence = adapter.evidence();
        const bool composited = prepared.accepted && composite(adapter.display_frame());
        return plan_native_material_preview_display(NativeMaterialPreviewDisplayDesc{
            .d3d12_host_available = prepared.d3d12_host_available,
            .shader_artifacts_available = prepared.shader_artifacts_available,
            .gpu_payload_available = prepared.gpu_payload_available,
            .texture_display_requested = prepared.texture_display_requested,
            .texture_adapter_available = adapter_evidence.texture_adapter_available,
            .offscreen_target_available = adapter_evidence.offscreen_target_available,
            .descriptor_lease_available = adapter_evidence.descriptor_lease_available,
            .resource_barriers_recorded = adapter_evidence.resource_barriers_recorded,
            .fence_lifecycle_ready = adapter_evidence.fence_lifecycle_ready,
            .visible_panel_available = evidence.visible_panel_available,
            .visible_texture_composite_recorded = composited,
            .visible_texture_composites = evidence.visible_texture_composites,
            .frame_index = frame_index,
            .backend_id = backend_id,
            .frames_rendered = adapter_evidence.frames_rendered,
            .executes = composited,
        });
    }
};

void NativeEditorVisibleTextureCompositor::resize(ViewportExtent extent) {
    impl_->extent = extent;
}

NativeEditorVisibleTextureCompositor::NativeEditorVisibleTextureCompositor(
    NativeEditorVisibleTextureCompositorDesc desc)
    : impl_(std::make_unique<Impl>(desc)) {}

NativeEditorVisibleTextureCompositor::~NativeEditorVisibleTextureCompositor() = default;

NativeEditorVisibleTextureCompositor::NativeEditorVisibleTextureCompositor(
    NativeEditorVisibleTextureCompositor&&) noexcept = default;

NativeEditorVisibleTextureCompositor&
NativeEditorVisibleTextureCompositor::operator=(NativeEditorVisibleTextureCompositor&&) noexcept = default;

NativeViewportDisplayPlan
NativeEditorVisibleTextureCompositor::render_viewport_frame(NativeTextureDisplayAdapter& adapter,
                                                            std::uint64_t frame_index) {
    return impl_->render_viewport_frame(adapter, frame_index);
}

NativeMaterialPreviewDisplayPlan
NativeEditorVisibleTextureCompositor::render_material_preview_frame(NativeTextureDisplayAdapter& adapter,
                                                                    std::uint64_t frame_index) {
    return impl_->render_material_preview_frame(adapter, frame_index);
}

const NativeEditorVisibleTextureCompositorEvidence& NativeEditorVisibleTextureCompositor::evidence() const noexcept {
    return impl_->evidence;
}

} // namespace mirakana::editor
