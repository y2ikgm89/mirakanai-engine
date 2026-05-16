// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/material.hpp"
#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/renderer/rhi_directional_shadow_smoke_frame_renderer.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/renderer/rhi_postprocess_frame_renderer.hpp"
#include "mirakana/renderer/rhi_viewport_surface.hpp"
#include "mirakana/renderer/shadow_map.hpp"
#include "mirakana/renderer/sprite_batch.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class ThrowingSubmitRhiDevice : public mirakana::rhi::IRhiDevice {
  public:
    [[nodiscard]] mirakana::rhi::BackendKind backend_kind() const noexcept override {
        return inner.backend_kind();
    }

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return inner.backend_name();
    }

    [[nodiscard]] mirakana::rhi::RhiStats stats() const noexcept override {
        return inner.stats();
    }

    [[nodiscard]] std::uint64_t gpu_timestamp_ticks_per_second() const noexcept override {
        return inner.gpu_timestamp_ticks_per_second();
    }

    [[nodiscard]] mirakana::rhi::RhiDeviceMemoryDiagnostics memory_diagnostics() const override {
        return inner.memory_diagnostics();
    }

    [[nodiscard]] mirakana::rhi::BufferHandle create_buffer(const mirakana::rhi::BufferDesc& desc) override {
        return inner.create_buffer(desc);
    }

    [[nodiscard]] mirakana::rhi::TextureHandle create_texture(const mirakana::rhi::TextureDesc& desc) override {
        return inner.create_texture(desc);
    }

    [[nodiscard]] mirakana::rhi::SamplerHandle create_sampler(const mirakana::rhi::SamplerDesc& desc) override {
        return inner.create_sampler(desc);
    }

    [[nodiscard]] mirakana::rhi::SwapchainHandle create_swapchain(const mirakana::rhi::SwapchainDesc& desc) override {
        return inner.create_swapchain(desc);
    }

    void resize_swapchain(mirakana::rhi::SwapchainHandle swapchain, mirakana::rhi::Extent2D extent) override {
        inner.resize_swapchain(swapchain, extent);
    }

    [[nodiscard]] mirakana::rhi::SwapchainFrameHandle
    acquire_swapchain_frame(mirakana::rhi::SwapchainHandle swapchain) override {
        return inner.acquire_swapchain_frame(swapchain);
    }

    void release_swapchain_frame(mirakana::rhi::SwapchainFrameHandle frame) override {
        inner.release_swapchain_frame(frame);
    }

    [[nodiscard]] mirakana::rhi::TransientBuffer
    acquire_transient_buffer(const mirakana::rhi::BufferDesc& desc) override {
        return inner.acquire_transient_buffer(desc);
    }

    [[nodiscard]] mirakana::rhi::TransientTexture
    acquire_transient_texture(const mirakana::rhi::TextureDesc& desc) override {
        return inner.acquire_transient_texture(desc);
    }

    void release_transient(mirakana::rhi::TransientResourceHandle lease) override {
        inner.release_transient(lease);
    }

    [[nodiscard]] mirakana::rhi::ShaderHandle create_shader(const mirakana::rhi::ShaderDesc& desc) override {
        return inner.create_shader(desc);
    }

    [[nodiscard]] mirakana::rhi::DescriptorSetLayoutHandle
    create_descriptor_set_layout(const mirakana::rhi::DescriptorSetLayoutDesc& desc) override {
        return inner.create_descriptor_set_layout(desc);
    }

    [[nodiscard]] mirakana::rhi::DescriptorSetHandle
    allocate_descriptor_set(mirakana::rhi::DescriptorSetLayoutHandle layout) override {
        return inner.allocate_descriptor_set(layout);
    }

    void update_descriptor_set(const mirakana::rhi::DescriptorWrite& write) override {
        inner.update_descriptor_set(write);
    }

    [[nodiscard]] mirakana::rhi::PipelineLayoutHandle
    create_pipeline_layout(const mirakana::rhi::PipelineLayoutDesc& desc) override {
        return inner.create_pipeline_layout(desc);
    }

    [[nodiscard]] mirakana::rhi::GraphicsPipelineHandle
    create_graphics_pipeline(const mirakana::rhi::GraphicsPipelineDesc& desc) override {
        return inner.create_graphics_pipeline(desc);
    }

    [[nodiscard]] mirakana::rhi::ComputePipelineHandle
    create_compute_pipeline(const mirakana::rhi::ComputePipelineDesc& desc) override {
        return inner.create_compute_pipeline(desc);
    }

    [[nodiscard]] std::unique_ptr<mirakana::rhi::IRhiCommandList>
    begin_command_list(mirakana::rhi::QueueKind queue) override {
        return inner.begin_command_list(queue);
    }

    [[nodiscard]] mirakana::rhi::FenceValue submit(mirakana::rhi::IRhiCommandList& commands) override {
        if (throw_on_submit) {
            throw std::runtime_error("submit failed");
        }
        return inner.submit(commands);
    }

    void wait(mirakana::rhi::FenceValue fence) override {
        inner.wait(fence);
    }

    void wait_for_queue(mirakana::rhi::QueueKind queue, mirakana::rhi::FenceValue fence) override {
        inner.wait_for_queue(queue, fence);
    }

    void write_buffer(mirakana::rhi::BufferHandle buffer, std::uint64_t offset,
                      std::span<const std::uint8_t> bytes) override {
        inner.write_buffer(buffer, offset, bytes);
    }

    [[nodiscard]] std::vector<std::uint8_t> read_buffer(mirakana::rhi::BufferHandle buffer, std::uint64_t offset,
                                                        std::uint64_t size_bytes) override {
        return inner.read_buffer(buffer, offset, size_bytes);
    }

    mirakana::rhi::NullRhiDevice inner;
    bool throw_on_submit{true};
};

class ThrowingTransitionCommandList final : public mirakana::rhi::IRhiCommandList {
  public:
    ThrowingTransitionCommandList(std::unique_ptr<mirakana::rhi::IRhiCommandList> inner,
                                  std::uint32_t throw_on_transition)
        : inner_(std::move(inner)), throw_on_transition_(throw_on_transition) {}

    [[nodiscard]] mirakana::rhi::QueueKind queue_kind() const noexcept override {
        return inner_->queue_kind();
    }

    [[nodiscard]] bool closed() const noexcept override {
        return inner_->closed();
    }

    [[nodiscard]] mirakana::rhi::IRhiCommandList& inner() noexcept {
        return *inner_;
    }

    void transition_texture(mirakana::rhi::TextureHandle texture, mirakana::rhi::ResourceState before,
                            mirakana::rhi::ResourceState after) override {
        ++transition_count_;
        if (throw_on_transition_ != 0 && transition_count_ == throw_on_transition_) {
            throw std::runtime_error("transition failed");
        }
        inner_->transition_texture(texture, before, after);
    }

    void copy_buffer(mirakana::rhi::BufferHandle source, mirakana::rhi::BufferHandle destination,
                     const mirakana::rhi::BufferCopyRegion& region) override {
        inner_->copy_buffer(source, destination, region);
    }

    void copy_buffer_to_texture(mirakana::rhi::BufferHandle source, mirakana::rhi::TextureHandle destination,
                                const mirakana::rhi::BufferTextureCopyRegion& region) override {
        inner_->copy_buffer_to_texture(source, destination, region);
    }

    void copy_texture_to_buffer(mirakana::rhi::TextureHandle source, mirakana::rhi::BufferHandle destination,
                                const mirakana::rhi::BufferTextureCopyRegion& region) override {
        inner_->copy_texture_to_buffer(source, destination, region);
    }

    void present(mirakana::rhi::SwapchainFrameHandle frame) override {
        inner_->present(frame);
    }

    void begin_render_pass(const mirakana::rhi::RenderPassDesc& desc) override {
        inner_->begin_render_pass(desc);
    }

    void end_render_pass() override {
        inner_->end_render_pass();
    }

    void bind_graphics_pipeline(mirakana::rhi::GraphicsPipelineHandle pipeline) override {
        inner_->bind_graphics_pipeline(pipeline);
    }

    void bind_compute_pipeline(mirakana::rhi::ComputePipelineHandle pipeline) override {
        inner_->bind_compute_pipeline(pipeline);
    }

    void set_viewport(const mirakana::rhi::ViewportDesc& viewport) override {
        inner_->set_viewport(viewport);
    }

    void set_scissor(const mirakana::rhi::ScissorRectDesc& scissor) override {
        inner_->set_scissor(scissor);
    }

    void bind_descriptor_set(mirakana::rhi::PipelineLayoutHandle layout, std::uint32_t set_index,
                             mirakana::rhi::DescriptorSetHandle set) override {
        inner_->bind_descriptor_set(layout, set_index, set);
    }

    void bind_vertex_buffer(const mirakana::rhi::VertexBufferBinding& binding) override {
        inner_->bind_vertex_buffer(binding);
    }

    void bind_index_buffer(const mirakana::rhi::IndexBufferBinding& binding) override {
        inner_->bind_index_buffer(binding);
    }

    void draw(std::uint32_t vertex_count, std::uint32_t instance_count) override {
        inner_->draw(vertex_count, instance_count);
    }

    void draw_indexed(std::uint32_t index_count, std::uint32_t instance_count) override {
        inner_->draw_indexed(index_count, instance_count);
    }

    void dispatch(std::uint32_t group_count_x, std::uint32_t group_count_y, std::uint32_t group_count_z) override {
        inner_->dispatch(group_count_x, group_count_y, group_count_z);
    }

    void begin_gpu_debug_scope(std::string_view name) override {
        inner_->begin_gpu_debug_scope(name);
    }

    void end_gpu_debug_scope() override {
        inner_->end_gpu_debug_scope();
    }

    void insert_gpu_debug_marker(std::string_view name) override {
        inner_->insert_gpu_debug_marker(name);
    }

    void close() override {
        inner_->close();
    }

  private:
    std::unique_ptr<mirakana::rhi::IRhiCommandList> inner_;
    std::uint32_t throw_on_transition_{0};
    std::uint32_t transition_count_{0};
};

class ThrowingTransitionRhiDevice final : public ThrowingSubmitRhiDevice {
  public:
    [[nodiscard]] std::unique_ptr<mirakana::rhi::IRhiCommandList>
    begin_command_list(mirakana::rhi::QueueKind queue) override {
        return std::make_unique<ThrowingTransitionCommandList>(inner.begin_command_list(queue), throw_on_transition);
    }

    [[nodiscard]] mirakana::rhi::FenceValue submit(mirakana::rhi::IRhiCommandList& commands) override {
        if (auto* throwing_commands = dynamic_cast<ThrowingTransitionCommandList*>(&commands);
            throwing_commands != nullptr) {
            return ThrowingSubmitRhiDevice::submit(throwing_commands->inner());
        }
        return ThrowingSubmitRhiDevice::submit(commands);
    }

    std::uint32_t throw_on_transition{0};
};

class ThrowingTransientTextureAcquireRhiDevice final : public ThrowingSubmitRhiDevice {
  public:
    [[nodiscard]] mirakana::rhi::TransientTexture
    acquire_transient_texture(const mirakana::rhi::TextureDesc& desc) override {
        ++acquire_calls;
        if (fail_on_acquire != 0 && acquire_calls == fail_on_acquire) {
            throw std::runtime_error("transient texture allocation failed");
        }
        return inner.acquire_transient_texture(desc);
    }

    std::uint32_t acquire_calls{0};
    std::uint32_t fail_on_acquire{0};
};

[[nodiscard]] std::array<std::uint8_t, mirakana::shadow_receiver_constants_byte_size()>
make_dummy_packed_shadow_receiver_constants() {
    mirakana::DirectionalShadowLightSpacePlan light_space;
    light_space.clip_from_world_cascades.push_back(mirakana::Mat4::identity());
    std::array<std::uint8_t, mirakana::shadow_receiver_constants_byte_size()> packed{};
    mirakana::pack_shadow_receiver_constants(packed, light_space, 1U, mirakana::Mat4::identity());
    return packed;
}

[[nodiscard]] mirakana::rhi::GraphicsPipelineHandle
create_renderer_test_pipeline(mirakana::rhi::IRhiDevice& device, mirakana::rhi::Format color_format,
                              mirakana::rhi::Format depth_format = mirakana::rhi::Format::unknown) {
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    return device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = color_format,
        .depth_format = depth_format,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
}

[[nodiscard]] mirakana::RhiDirectionalShadowSmokeFrameRendererDesc
create_directional_shadow_smoke_test_desc(mirakana::rhi::IRhiDevice& device, mirakana::rhi::SwapchainHandle swapchain) {
    const auto material_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto shadow_receiver_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::shadow_receiver_depth_texture_binding(),
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::shadow_receiver_sampler_binding(),
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::shadow_receiver_constants_binding(),
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto scene_layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, shadow_receiver_layout},
        .push_constant_bytes = 0,
    });
    const auto scene_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto scene_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_shadow_receiver",
        .bytecode_size = 64,
    });
    const auto scene_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = scene_layout,
        .vertex_shader = scene_vertex_shader,
        .fragment_shader = scene_fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                            .depth_write_enabled = true,
                                                            .depth_compare = mirakana::rhi::CompareOp::less_equal},
    });
    const auto shadow_layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto shadow_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_shadow",
        .bytecode_size = 64,
    });
    const auto shadow_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_shadow",
        .bytecode_size = 64,
    });
    const auto shadow_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = shadow_layout,
        .vertex_shader = shadow_vertex_shader,
        .fragment_shader = shadow_fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                            .depth_write_enabled = true,
                                                            .depth_compare = mirakana::rhi::CompareOp::less_equal},
    });
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess_depth",
        .bytecode_size = 64,
    });
    return mirakana::RhiDirectionalShadowSmokeFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .scene_skinned_graphics_pipeline = {},
        .scene_pipeline_layout = scene_layout,
        .shadow_graphics_pipeline = shadow_pipeline,
        .shadow_receiver_descriptor_set_layout = shadow_receiver_layout,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
        .scene_depth_format = mirakana::rhi::Format::depth24_stencil8,
        .shadow_depth_format = mirakana::rhi::Format::depth24_stencil8,
        .shadow_depth_atlas_extent = mirakana::Extent2D{.width = 64, .height = 36},
        .directional_shadow_cascade_count = 1,
        .shadow_receiver_constants_initial = make_dummy_packed_shadow_receiver_constants(),
        .shadow_receiver_descriptor_set_index = 1,
    };
}

[[nodiscard]] std::unique_ptr<mirakana::RhiDirectionalShadowSmokeFrameRenderer>
create_directional_shadow_smoke_test_renderer(mirakana::rhi::IRhiDevice& device,
                                              mirakana::rhi::SwapchainHandle swapchain) {
    return std::make_unique<mirakana::RhiDirectionalShadowSmokeFrameRenderer>(
        create_directional_shadow_smoke_test_desc(device, swapchain));
}

} // namespace

MK_TEST("frame graph v0 orders scene color before postprocess") {
    mirakana::FrameGraphDesc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceDesc{.name = "backbuffer", .imported = true});
    desc.passes.push_back(mirakana::FrameGraphPassDesc{
        .name = "postprocess",
        .reads = {"scene_color"},
        .writes = {"backbuffer"},
    });
    desc.passes.push_back(mirakana::FrameGraphPassDesc{
        .name = "scene_color",
        .writes = {"scene_color"},
    });

    const auto result = mirakana::compile_frame_graph_v0(desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.ordered_passes.size() == 2);
    MK_REQUIRE(result.ordered_passes[0] == "scene_color");
    MK_REQUIRE(result.ordered_passes[1] == "postprocess");
    MK_REQUIRE(result.pass_count == 2);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("frame graph v0 rejects deterministic resource hazards") {
    mirakana::FrameGraphDesc missing_producer;
    missing_producer.passes.push_back(mirakana::FrameGraphPassDesc{
        .name = "postprocess",
        .reads = {"scene_color"},
        .writes = {"backbuffer"},
    });
    const auto missing_result = mirakana::compile_frame_graph_v0(missing_producer);
    MK_REQUIRE(!missing_result.succeeded());
    MK_REQUIRE(!missing_result.diagnostics.empty());
    MK_REQUIRE(missing_result.diagnostics.front().code == mirakana::FrameGraphDiagnosticCode::missing_producer);

    mirakana::FrameGraphDesc write_hazard;
    write_hazard.passes.push_back(mirakana::FrameGraphPassDesc{.name = "first", .writes = {"scene_color"}});
    write_hazard.passes.push_back(mirakana::FrameGraphPassDesc{.name = "second", .writes = {"scene_color"}});
    const auto hazard_result = mirakana::compile_frame_graph_v0(write_hazard);
    MK_REQUIRE(!hazard_result.succeeded());
    MK_REQUIRE(hazard_result.diagnostics.front().code == mirakana::FrameGraphDiagnosticCode::write_write_hazard);

    mirakana::FrameGraphDesc cycle;
    cycle.passes.push_back(mirakana::FrameGraphPassDesc{.name = "a", .reads = {"b_out"}, .writes = {"a_out"}});
    cycle.passes.push_back(mirakana::FrameGraphPassDesc{.name = "b", .reads = {"a_out"}, .writes = {"b_out"}});
    const auto cycle_result = mirakana::compile_frame_graph_v0(cycle);
    MK_REQUIRE(!cycle_result.succeeded());
    MK_REQUIRE(cycle_result.diagnostics.front().code == mirakana::FrameGraphDiagnosticCode::cycle);

    mirakana::FrameGraphDesc missing_consumer;
    missing_consumer.resources.push_back(
        mirakana::FrameGraphResourceDesc{.name = "scene_color", .requires_consumer = true});
    missing_consumer.passes.push_back(mirakana::FrameGraphPassDesc{.name = "scene", .writes = {"scene_color"}});
    const auto consumer_result = mirakana::compile_frame_graph_v0(missing_consumer);
    MK_REQUIRE(!consumer_result.succeeded());
    MK_REQUIRE(consumer_result.diagnostics.front().code == mirakana::FrameGraphDiagnosticCode::missing_consumer);
}

MK_TEST("frame graph v0 rejects duplicate pass and resource declarations") {
    mirakana::FrameGraphDesc dup_pass;
    dup_pass.resources.push_back(mirakana::FrameGraphResourceDesc{.name = "x"});
    dup_pass.passes.push_back(mirakana::FrameGraphPassDesc{.name = "scene", .writes = {"x"}});
    dup_pass.passes.push_back(mirakana::FrameGraphPassDesc{.name = "scene", .writes = {}});

    const auto dup_pass_result = mirakana::compile_frame_graph_v0(dup_pass);
    MK_REQUIRE(!dup_pass_result.succeeded());
    MK_REQUIRE(dup_pass_result.diagnostics.front().code == mirakana::FrameGraphDiagnosticCode::invalid_pass);

    mirakana::FrameGraphDesc dup_resource;
    dup_resource.resources.push_back(mirakana::FrameGraphResourceDesc{.name = "x"});
    dup_resource.resources.push_back(mirakana::FrameGraphResourceDesc{.name = "x"});
    dup_resource.passes.push_back(mirakana::FrameGraphPassDesc{.name = "a", .writes = {"x"}});

    const auto dup_resource_result = mirakana::compile_frame_graph_v0(dup_resource);
    MK_REQUIRE(!dup_resource_result.succeeded());
    MK_REQUIRE(dup_resource_result.diagnostics.front().code == mirakana::FrameGraphDiagnosticCode::invalid_resource);
}

MK_TEST("frame graph v1 plans deterministic barriers between writer and reader") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.ordered_passes.size() == 2);
    MK_REQUIRE(plan.ordered_passes[0] == "scene");
    MK_REQUIRE(plan.ordered_passes[1] == "postprocess");
    MK_REQUIRE(plan.barriers.size() == 1);
    MK_REQUIRE(plan.barriers[0].resource == "scene-color");
    MK_REQUIRE(plan.barriers[0].from_pass == "scene");
    MK_REQUIRE(plan.barriers[0].to_pass == "postprocess");
    MK_REQUIRE(plan.barriers[0].from == mirakana::FrameGraphAccess::color_attachment_write);
    MK_REQUIRE(plan.barriers[0].to == mirakana::FrameGraphAccess::shader_read);
}

MK_TEST("frame graph v1 schedule pass_invoke order matches ordered_passes") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.barriers.size() == 1);

    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);
    std::vector<std::string> pass_invokes;
    pass_invokes.reserve(plan.ordered_passes.size());
    for (const auto& step : schedule) {
        if (step.kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke) {
            pass_invokes.push_back(step.pass_name);
        }
    }
    MK_REQUIRE(pass_invokes == plan.ordered_passes);
}

MK_TEST("frame graph v1 orders directional shadow scene and postprocess for a minimal desktop-style stack") {
    // Compile-only proof for Phase 4 milestone scheduling: shadow map depth before lit scene, scene color before post.
    // Does not invoke SDL/RHI or claim production render-graph ownership.
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "shadow-depth", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});

    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "directional_shadow",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "shadow-depth",
                                                      .access = mirakana::FrameGraphAccess::depth_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "shadow-depth",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.ordered_passes.size() == 3);
    MK_REQUIRE(plan.ordered_passes[0] == "directional_shadow");
    MK_REQUIRE(plan.ordered_passes[1] == "scene");
    MK_REQUIRE(plan.ordered_passes[2] == "postprocess");
    MK_REQUIRE(plan.barriers.size() == 2);

    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);
    MK_REQUIRE(schedule.size() == 5);
    MK_REQUIRE(schedule[0].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(schedule[0].pass_name == "directional_shadow");
    MK_REQUIRE(schedule[1].kind == mirakana::FrameGraphExecutionStep::Kind::barrier);
    MK_REQUIRE(schedule[1].barrier.resource == "shadow-depth");
    MK_REQUIRE(schedule[2].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(schedule[2].pass_name == "scene");
    MK_REQUIRE(schedule[3].kind == mirakana::FrameGraphExecutionStep::Kind::barrier);
    MK_REQUIRE(schedule[3].barrier.resource == "scene-color");
    MK_REQUIRE(schedule[4].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(schedule[4].pass_name == "postprocess");
}

MK_TEST("frame graph v1 breaks parallel-ready tie by pass declaration order") {
    // When two passes have indegree zero, compile_frame_graph_v1 picks the smallest pass index first.
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "effect-a-out", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "effect-b-out", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "combine-out", .lifetime = mirakana::FrameGraphResourceLifetime::transient});

    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "effect_a",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "effect-a-out",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "effect_b",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "effect-b-out",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "combine",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "effect-a-out",
                                                     .access = mirakana::FrameGraphAccess::shader_read},
                  mirakana::FrameGraphResourceAccess{.resource = "effect-b-out",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "combine-out",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.ordered_passes.size() == 3);
    MK_REQUIRE(plan.ordered_passes[0] == "effect_a");
    MK_REQUIRE(plan.ordered_passes[1] == "effect_b");
    MK_REQUIRE(plan.ordered_passes[2] == "combine");

    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);
    MK_REQUIRE(schedule.size() == 5);
    MK_REQUIRE(schedule[0].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(schedule[0].pass_name == "effect_a");
    MK_REQUIRE(schedule[1].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(schedule[1].pass_name == "effect_b");
    MK_REQUIRE(schedule[2].kind == mirakana::FrameGraphExecutionStep::Kind::barrier);
    MK_REQUIRE(schedule[2].barrier.resource == "effect-a-out");
    MK_REQUIRE(schedule[2].barrier.to_pass == "combine");
    MK_REQUIRE(schedule[3].kind == mirakana::FrameGraphExecutionStep::Kind::barrier);
    MK_REQUIRE(schedule[3].barrier.resource == "effect-b-out");
    MK_REQUIRE(schedule[3].barrier.to_pass == "combine");
    MK_REQUIRE(schedule[4].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(schedule[4].pass_name == "combine");
}

MK_TEST("frame graph v1 diagnoses dependency cycle for ping-pong resources") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "a-out", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "b-out", .lifetime = mirakana::FrameGraphResourceLifetime::transient});

    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "pass-a",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "b-out",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "a-out",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "pass-b",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "a-out",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "b-out",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.ordered_passes.empty());
    MK_REQUIRE(plan.diagnostics.size() == 1);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::cycle);
}

MK_TEST("frame graph v1 diagnoses transient reads without producers and same-pass hazards") {
    mirakana::FrameGraphV1Desc transient_read;
    transient_read.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    transient_read.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto transient_result = mirakana::compile_frame_graph_v1(transient_read);
    MK_REQUIRE(!transient_result.succeeded());
    MK_REQUIRE(transient_result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::missing_producer);

    mirakana::FrameGraphV1Desc same_pass;
    same_pass.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "history", .lifetime = mirakana::FrameGraphResourceLifetime::imported});
    same_pass.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "feedback",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "history",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "history",
                                                      .access = mirakana::FrameGraphAccess::copy_destination}},
    });

    const auto hazard_result = mirakana::compile_frame_graph_v1(same_pass);
    MK_REQUIRE(!hazard_result.succeeded());
    MK_REQUIRE(hazard_result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::read_write_hazard);
}

MK_TEST("frame graph v1 allows imported reads and rejects duplicate writers") {
    mirakana::FrameGraphV1Desc imported_read;
    imported_read.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "backbuffer", .lifetime = mirakana::FrameGraphResourceLifetime::imported});
    imported_read.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "present",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "backbuffer",
                                                     .access = mirakana::FrameGraphAccess::present}},
        .writes = {},
    });

    const auto imported_result = mirakana::compile_frame_graph_v1(imported_read);
    MK_REQUIRE(imported_result.succeeded());
    MK_REQUIRE(imported_result.ordered_passes.size() == 1);
    MK_REQUIRE(imported_result.barriers.empty());

    mirakana::FrameGraphV1Desc duplicate_write;
    duplicate_write.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    duplicate_write.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene-a",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    duplicate_write.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene-b",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::copy_destination}},
    });

    const auto duplicate_result = mirakana::compile_frame_graph_v1(duplicate_write);
    MK_REQUIRE(!duplicate_result.succeeded());
    MK_REQUIRE(duplicate_result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::write_write_hazard);
}

MK_TEST("frame graph v1 rejects duplicate pass and resource declarations") {
    mirakana::FrameGraphV1Desc dup_pass;
    dup_pass.resources.push_back(
        mirakana::FrameGraphResourceV1Desc{.name = "x", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    dup_pass.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "x",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    dup_pass.passes.push_back(mirakana::FrameGraphPassV1Desc{.name = "scene", .reads = {}, .writes = {}});

    const auto dup_pass_result = mirakana::compile_frame_graph_v1(dup_pass);
    MK_REQUIRE(!dup_pass_result.succeeded());
    MK_REQUIRE(dup_pass_result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::invalid_pass);

    mirakana::FrameGraphV1Desc dup_resource;
    dup_resource.resources.push_back(
        mirakana::FrameGraphResourceV1Desc{.name = "x", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    dup_resource.resources.push_back(
        mirakana::FrameGraphResourceV1Desc{.name = "x", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    dup_resource.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "a",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "x",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });

    const auto dup_resource_result = mirakana::compile_frame_graph_v1(dup_resource);
    MK_REQUIRE(!dup_resource_result.succeeded());
    MK_REQUIRE(dup_resource_result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::invalid_resource);
}

MK_TEST("frame graph v1 rejects unknown resource access kinds") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(
        mirakana::FrameGraphResourceV1Desc{.name = "x", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "w",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "x", .access = mirakana::FrameGraphAccess::unknown}},
    });

    const auto result = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::invalid_resource);
}

MK_TEST("frame graph rhi transient texture alias planner reuses exact non overlapping descriptors") {
    const auto color_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    };

    mirakana::FrameGraphV1Desc desc;
    for (const std::string_view name : {"early", "late", "overlap-a", "overlap-b"}) {
        desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
            .name = std::string(name), .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    }
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_early",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "early",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});
    desc.passes.push_back(
        mirakana::FrameGraphPassV1Desc{.name = "read_early",
                                       .reads = {mirakana::FrameGraphResourceAccess{
                                           .resource = "early", .access = mirakana::FrameGraphAccess::shader_read}},
                                       .writes = {}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_late",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "late",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});
    desc.passes.push_back(
        mirakana::FrameGraphPassV1Desc{.name = "read_late",
                                       .reads = {mirakana::FrameGraphResourceAccess{
                                           .resource = "late", .access = mirakana::FrameGraphAccess::shader_read}},
                                       .writes = {}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_overlap_a",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "overlap-a",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_overlap_b",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "overlap-b",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "read_overlap",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "overlap-a",
                                                     .access = mirakana::FrameGraphAccess::shader_read},
                  mirakana::FrameGraphResourceAccess{.resource = "overlap-b",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {}});

    const auto plan = mirakana::plan_frame_graph_transient_texture_aliases(
        desc, std::vector<mirakana::FrameGraphTransientTextureDesc>{
                  mirakana::FrameGraphTransientTextureDesc{.resource = "early", .desc = color_desc},
                  mirakana::FrameGraphTransientTextureDesc{.resource = "late", .desc = color_desc},
                  mirakana::FrameGraphTransientTextureDesc{.resource = "overlap-a", .desc = color_desc},
                  mirakana::FrameGraphTransientTextureDesc{.resource = "overlap-b", .desc = color_desc},
              });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.lifetimes.size() == 4);
    MK_REQUIRE(plan.alias_groups.size() == 2);
    MK_REQUIRE(plan.lifetimes[0].resource == "early");
    MK_REQUIRE(plan.lifetimes[1].resource == "late");
    MK_REQUIRE(plan.lifetimes[2].resource == "overlap-a");
    MK_REQUIRE(plan.lifetimes[3].resource == "overlap-b");
    MK_REQUIRE(plan.lifetimes[0].alias_group == plan.lifetimes[1].alias_group);
    MK_REQUIRE(plan.lifetimes[1].alias_group == plan.lifetimes[2].alias_group);
    MK_REQUIRE(plan.lifetimes[2].alias_group != plan.lifetimes[3].alias_group);
    MK_REQUIRE(plan.lifetimes[0].first_pass_index == 0);
    MK_REQUIRE(plan.lifetimes[0].last_pass_index == 1);
    MK_REQUIRE(plan.lifetimes[3].first_pass_index == 5);
    MK_REQUIRE(plan.lifetimes[3].last_pass_index == 6);
    MK_REQUIRE(plan.estimated_unaliased_bytes == 64ULL * 64ULL * 4ULL * 4ULL);
    MK_REQUIRE(plan.estimated_aliased_bytes == 64ULL * 64ULL * 4ULL * 2ULL);
}

MK_TEST("frame graph rhi transient texture alias planner keeps incompatible descriptors separate") {
    const auto color_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    };
    const auto larger_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 128, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    };

    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "small", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "large", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_small",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "small",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});
    desc.passes.push_back(
        mirakana::FrameGraphPassV1Desc{.name = "read_small",
                                       .reads = {mirakana::FrameGraphResourceAccess{
                                           .resource = "small", .access = mirakana::FrameGraphAccess::shader_read}},
                                       .writes = {}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_large",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "large",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});
    desc.passes.push_back(
        mirakana::FrameGraphPassV1Desc{.name = "read_large",
                                       .reads = {mirakana::FrameGraphResourceAccess{
                                           .resource = "large", .access = mirakana::FrameGraphAccess::shader_read}},
                                       .writes = {}});

    const auto plan = mirakana::plan_frame_graph_transient_texture_aliases(
        desc, std::vector<mirakana::FrameGraphTransientTextureDesc>{
                  mirakana::FrameGraphTransientTextureDesc{.resource = "small", .desc = color_desc},
                  mirakana::FrameGraphTransientTextureDesc{.resource = "large", .desc = larger_desc},
              });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.lifetimes.size() == 2);
    MK_REQUIRE(plan.alias_groups.size() == 2);
    MK_REQUIRE(plan.lifetimes[0].alias_group != plan.lifetimes[1].alias_group);
    MK_REQUIRE(plan.estimated_unaliased_bytes == 64ULL * 64ULL * 4ULL + 128ULL * 64ULL * 4ULL);
    MK_REQUIRE(plan.estimated_aliased_bytes == plan.estimated_unaliased_bytes);
}

MK_TEST("frame graph rhi transient texture alias planner rejects unsafe descriptors") {
    const auto color_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    };

    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "backbuffer", .lifetime = mirakana::FrameGraphResourceLifetime::imported});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "post-work", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "post-work",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});

    const auto plan = mirakana::plan_frame_graph_transient_texture_aliases(
        desc, std::vector<mirakana::FrameGraphTransientTextureDesc>{
                  mirakana::FrameGraphTransientTextureDesc{.resource = "scene-color", .desc = color_desc},
                  mirakana::FrameGraphTransientTextureDesc{.resource = "backbuffer", .desc = color_desc},
                  mirakana::FrameGraphTransientTextureDesc{.resource = "ghost", .desc = color_desc},
              });

    MK_REQUIRE(!plan.succeeded());
    const auto has_diagnostic = [&plan](std::string_view resource, std::string_view message) {
        return std::ranges::any_of(plan.diagnostics, [resource, message](const mirakana::FrameGraphDiagnostic& diag) {
            return diag.code == mirakana::FrameGraphDiagnosticCode::invalid_resource && diag.resource == resource &&
                   diag.message == message;
        });
    };
    MK_REQUIRE(has_diagnostic("scene-color", "transient texture usage is missing render_target"));
    MK_REQUIRE(has_diagnostic("post-work", "used transient texture resource has no descriptor"));
    MK_REQUIRE(has_diagnostic("backbuffer", "transient texture descriptor targets an imported resource"));
    MK_REQUIRE(has_diagnostic("ghost", "transient texture descriptor targets an undeclared resource"));
}

MK_TEST("frame graph rhi transient texture alias planner rejects backend incompatible depth descriptors") {
    const auto sampled_depth_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::shader_resource,
    };
    const auto volume_depth_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 2},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    };
    const auto copy_depth_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::copy_source,
    };

    mirakana::FrameGraphV1Desc desc;
    for (const std::string_view name : {"sampled-depth", "volume-depth", "copy-depth"}) {
        desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
            .name = std::string(name), .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    }
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "upload_sampled_depth",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "sampled-depth",
                                                      .access = mirakana::FrameGraphAccess::copy_destination}}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "read_sampled_depth",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "sampled-depth",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_volume_depth",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "volume-depth",
                                                      .access = mirakana::FrameGraphAccess::depth_attachment_write}}});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_copy_depth",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "copy-depth",
                                                      .access = mirakana::FrameGraphAccess::depth_attachment_write}}});

    const auto plan = mirakana::plan_frame_graph_transient_texture_aliases(
        desc, std::vector<mirakana::FrameGraphTransientTextureDesc>{
                  mirakana::FrameGraphTransientTextureDesc{.resource = "sampled-depth", .desc = sampled_depth_desc},
                  mirakana::FrameGraphTransientTextureDesc{.resource = "volume-depth", .desc = volume_depth_desc},
                  mirakana::FrameGraphTransientTextureDesc{.resource = "copy-depth", .desc = copy_depth_desc},
              });

    MK_REQUIRE(!plan.succeeded());
    const auto has_diagnostic = [&plan](std::string_view resource, std::string_view message) {
        return std::ranges::any_of(plan.diagnostics, [resource, message](const mirakana::FrameGraphDiagnostic& diag) {
            return diag.code == mirakana::FrameGraphDiagnosticCode::invalid_resource && diag.resource == resource &&
                   diag.message == message;
        });
    };
    MK_REQUIRE(
        has_diagnostic("sampled-depth", "transient depth texture usage supports only depth_stencil or sampled depth"));
    MK_REQUIRE(has_diagnostic("volume-depth", "transient depth texture extent must be 2D"));
    MK_REQUIRE(
        has_diagnostic("copy-depth", "transient depth texture usage supports only depth_stencil or sampled depth"));
}

MK_TEST("frame graph rhi transient texture alias planner rejects byte estimate overflow") {
    const auto huge = std::numeric_limits<std::uint32_t>::max();
    const auto huge_color_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = huge, .height = huge, .depth = huge},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    };

    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "huge-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "write_huge_color",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "huge-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}}});

    const auto plan = mirakana::plan_frame_graph_transient_texture_aliases(
        desc, std::vector<mirakana::FrameGraphTransientTextureDesc>{
                  mirakana::FrameGraphTransientTextureDesc{.resource = "huge-color", .desc = huge_color_desc},
              });

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(std::ranges::any_of(plan.diagnostics, [](const mirakana::FrameGraphDiagnostic& diag) {
        return diag.code == mirakana::FrameGraphDiagnosticCode::invalid_resource && diag.resource == "huge-color" &&
               diag.message == "transient texture byte estimate overflowed";
    }));
}

MK_TEST("frame graph rhi transient texture lease binding accepts empty alias plans without acquiring") {
    const mirakana::FrameGraphTransientTextureAliasPlan plan;

    mirakana::rhi::NullRhiDevice device;
    const auto result = mirakana::acquire_frame_graph_transient_texture_lease_bindings(device, plan);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.leases.empty());
    MK_REQUIRE(result.texture_bindings.empty());
    MK_REQUIRE(result.diagnostics.empty());

    const auto stats = device.stats();
    MK_REQUIRE(stats.transient_resources_acquired == 0);
    MK_REQUIRE(stats.transient_resources_active == 0);
}

MK_TEST("frame graph rhi transient texture lease binding acquires one lease per alias group") {
    const auto color_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    };

    mirakana::FrameGraphTransientTextureAliasPlan plan;
    plan.alias_groups.push_back(mirakana::FrameGraphTransientTextureAliasGroup{
        .index = 0,
        .desc = color_desc,
        .estimated_bytes = 64ULL * 64ULL * 4ULL,
        .resources = {"early", "late"},
    });
    plan.alias_groups.push_back(mirakana::FrameGraphTransientTextureAliasGroup{
        .index = 1,
        .desc = color_desc,
        .estimated_bytes = 64ULL * 64ULL * 4ULL,
        .resources = {"overlap"},
    });

    mirakana::rhi::NullRhiDevice device;
    const auto result = mirakana::acquire_frame_graph_transient_texture_lease_bindings(device, plan);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.leases.size() == 2);
    MK_REQUIRE(result.texture_bindings.size() == 3);
    MK_REQUIRE(result.leases[0].alias_group == 0);
    MK_REQUIRE(result.leases[1].alias_group == 1);
    MK_REQUIRE(result.texture_bindings[0].resource == "early");
    MK_REQUIRE(result.texture_bindings[1].resource == "late");
    MK_REQUIRE(result.texture_bindings[2].resource == "overlap");
    MK_REQUIRE(result.texture_bindings[0].texture.value == result.texture_bindings[1].texture.value);
    MK_REQUIRE(result.texture_bindings[0].texture.value != result.texture_bindings[2].texture.value);
    MK_REQUIRE(result.texture_bindings[0].current_state == mirakana::rhi::ResourceState::undefined);
    MK_REQUIRE(result.texture_bindings[1].current_state == mirakana::rhi::ResourceState::undefined);
    MK_REQUIRE(result.texture_bindings[2].current_state == mirakana::rhi::ResourceState::undefined);

    auto stats = device.stats();
    MK_REQUIRE(stats.transient_resources_acquired == 2);
    MK_REQUIRE(stats.transient_resources_active == 2);

    mirakana::release_frame_graph_transient_texture_lease_bindings(device, result.leases);

    stats = device.stats();
    MK_REQUIRE(stats.transient_resources_released == 2);
    MK_REQUIRE(stats.transient_resources_active == 0);
}

MK_TEST("frame graph rhi transient texture lease binding preserves plan diagnostics without acquiring") {
    mirakana::FrameGraphTransientTextureAliasPlan plan;
    plan.diagnostics.push_back(mirakana::FrameGraphDiagnostic{
        .code = mirakana::FrameGraphDiagnosticCode::invalid_resource,
        .pass = {},
        .resource = "scene-color",
        .message = "used transient texture resource has no descriptor",
    });

    mirakana::rhi::NullRhiDevice device;
    const auto result = mirakana::acquire_frame_graph_transient_texture_lease_bindings(device, plan);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.leases.empty());
    MK_REQUIRE(result.texture_bindings.empty());

    const auto stats = device.stats();
    MK_REQUIRE(stats.transient_resources_acquired == 0);
    MK_REQUIRE(stats.transient_resources_active == 0);
}

MK_TEST("frame graph rhi transient texture lease binding rejects empty alias groups before acquiring") {
    const auto color_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    };

    mirakana::FrameGraphTransientTextureAliasPlan plan;
    plan.alias_groups.push_back(mirakana::FrameGraphTransientTextureAliasGroup{
        .index = 7,
        .desc = color_desc,
        .estimated_bytes = 16ULL * 16ULL * 4ULL,
        .resources = {},
    });

    mirakana::rhi::NullRhiDevice device;
    const auto result = mirakana::acquire_frame_graph_transient_texture_lease_bindings(device, plan);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.leases.empty());
    MK_REQUIRE(result.texture_bindings.empty());
    MK_REQUIRE(std::ranges::any_of(result.diagnostics, [](const mirakana::FrameGraphDiagnostic& diag) {
        return diag.code == mirakana::FrameGraphDiagnosticCode::invalid_resource && diag.resource == "alias-group-7" &&
               diag.message == "frame graph transient texture alias group has no resources";
    }));

    const auto stats = device.stats();
    MK_REQUIRE(stats.transient_resources_acquired == 0);
    MK_REQUIRE(stats.transient_resources_active == 0);
}

MK_TEST("frame graph rhi transient texture lease binding releases acquired leases after later failure") {
    const auto color_desc = mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    };

    mirakana::FrameGraphTransientTextureAliasPlan plan;
    plan.alias_groups.push_back(mirakana::FrameGraphTransientTextureAliasGroup{
        .index = 0,
        .desc = color_desc,
        .estimated_bytes = 32ULL * 32ULL * 4ULL,
        .resources = {"first"},
    });
    plan.alias_groups.push_back(mirakana::FrameGraphTransientTextureAliasGroup{
        .index = 1,
        .desc = color_desc,
        .estimated_bytes = 32ULL * 32ULL * 4ULL,
        .resources = {"second"},
    });

    ThrowingTransientTextureAcquireRhiDevice device;
    device.fail_on_acquire = 2;
    const auto result = mirakana::acquire_frame_graph_transient_texture_lease_bindings(device, plan);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.leases.empty());
    MK_REQUIRE(result.texture_bindings.empty());
    MK_REQUIRE(std::ranges::any_of(result.diagnostics, [](const mirakana::FrameGraphDiagnostic& diag) {
        return diag.code == mirakana::FrameGraphDiagnosticCode::invalid_resource && diag.resource == "alias-group-1" &&
               diag.message.find("transient texture allocation failed") != std::string::npos;
    }));

    const auto stats = device.stats();
    MK_REQUIRE(stats.transient_resources_acquired == 1);
    MK_REQUIRE(stats.transient_resources_released == 1);
    MK_REQUIRE(stats.transient_resources_active == 0);
}

MK_TEST("frame graph v1 execution schedule interleaves sorted barriers before each pass") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(
        mirakana::FrameGraphResourceV1Desc{.name = "a", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.resources.push_back(
        mirakana::FrameGraphResourceV1Desc{.name = "b", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "w",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "a",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write},
                   mirakana::FrameGraphResourceAccess{.resource = "b",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "r",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "a",
                                                     .access = mirakana::FrameGraphAccess::shader_read},
                  mirakana::FrameGraphResourceAccess{.resource = "b",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);
    MK_REQUIRE(schedule.size() == 4);
    MK_REQUIRE(schedule[0].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(schedule[0].pass_name == "w");
    MK_REQUIRE(schedule[1].kind == mirakana::FrameGraphExecutionStep::Kind::barrier);
    MK_REQUIRE(schedule[1].barrier.resource == "a");
    MK_REQUIRE(schedule[2].kind == mirakana::FrameGraphExecutionStep::Kind::barrier);
    MK_REQUIRE(schedule[2].barrier.resource == "b");
    MK_REQUIRE(schedule[3].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(schedule[3].pass_name == "r");
}

MK_TEST("frame graph v1 execution schedule is empty when compilation failed") {
    mirakana::FrameGraphV1Desc bad;
    bad.resources.push_back(
        mirakana::FrameGraphResourceV1Desc{.name = "x", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    bad.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "lonely",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "x",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });
    const auto plan = mirakana::compile_frame_graph_v1(bad);
    MK_REQUIRE(!plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);
    MK_REQUIRE(schedule.empty());
}

MK_TEST("frame graph v1 dispatches barrier and pass callbacks in schedule order") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("postprocess"),
    };
    std::vector<std::string> events;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> passes{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&events](std::string_view pass_name) {
                    events.emplace_back(pass_name);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback =
                [&events](std::string_view pass_name) {
                    events.emplace_back(pass_name);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_v1_schedule(
        schedule, mirakana::FrameGraphExecutionCallbacks{
                      .pass_callbacks = passes,
                      .barrier_callback =
                          [&events](const mirakana::FrameGraphBarrier& barrier) {
                              events.push_back("barrier:" + barrier.resource);
                              return mirakana::FrameGraphExecutionCallbackResult{};
                          },
                  });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.pass_callbacks_invoked == 2);
    MK_REQUIRE(result.barrier_callbacks_invoked == 1);
    MK_REQUIRE(events.size() == 3);
    MK_REQUIRE(events[0] == "scene");
    MK_REQUIRE(events[1] == "barrier:scene-color");
    MK_REQUIRE(events[2] == "postprocess");
}

MK_TEST("frame graph v1 callback execution diagnoses missing callbacks before later passes") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("postprocess"),
    };
    std::vector<std::string> events;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> passes{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&events](std::string_view pass_name) {
                    events.emplace_back(pass_name);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_v1_schedule(
        schedule, mirakana::FrameGraphExecutionCallbacks{.pass_callbacks = passes});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.pass_callbacks_invoked == 1);
    MK_REQUIRE(result.barrier_callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::invalid_pass);
    MK_REQUIRE(result.diagnostics[0].pass == "postprocess");
    MK_REQUIRE(events.size() == 1);
    MK_REQUIRE(events[0] == "scene");
}

MK_TEST("frame graph v1 callback execution converts thrown callbacks to diagnostics") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("postprocess"),
    };
    const std::vector<mirakana::FrameGraphPassExecutionBinding> passes{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback = [](std::string_view) -> mirakana::FrameGraphExecutionCallbackResult {
                throw std::runtime_error("renderer pass failed");
            },
        },
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback = [](std::string_view) { return mirakana::FrameGraphExecutionCallbackResult{}; },
        },
    };

    const auto result = mirakana::execute_frame_graph_v1_schedule(
        schedule, mirakana::FrameGraphExecutionCallbacks{.pass_callbacks = passes});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::invalid_pass);
    MK_REQUIRE(result.diagnostics[0].pass == "scene");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph pass callback threw an exception: renderer pass failed");
}

MK_TEST("frame graph v1 callback execution copies pass bindings before dispatch") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("postprocess"),
    };
    std::vector<std::string> events;
    std::vector<mirakana::FrameGraphPassExecutionBinding> passes;
    passes.push_back(mirakana::FrameGraphPassExecutionBinding{
        .pass_name = "scene",
        .callback =
            [&events, &passes](std::string_view pass_name) {
                events.emplace_back(pass_name);
                passes.clear();
                return mirakana::FrameGraphExecutionCallbackResult{};
            },
    });
    passes.push_back(mirakana::FrameGraphPassExecutionBinding{
        .pass_name = "postprocess",
        .callback =
            [&events](std::string_view pass_name) {
                events.emplace_back(pass_name);
                return mirakana::FrameGraphExecutionCallbackResult{};
            },
    });

    const auto result = mirakana::execute_frame_graph_v1_schedule(
        schedule, mirakana::FrameGraphExecutionCallbacks{.pass_callbacks = passes});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.pass_callbacks_invoked == 2);
    MK_REQUIRE(events.size() == 2);
    MK_REQUIRE(events[0] == "scene");
    MK_REQUIRE(events[1] == "postprocess");
}

MK_TEST("frame graph v1 callback execution reports returned callback failures") {
    const std::vector<mirakana::FrameGraphExecutionStep> pass_schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
    };
    const std::vector<mirakana::FrameGraphPassExecutionBinding> failing_passes{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [](std::string_view) {
                    return mirakana::FrameGraphExecutionCallbackResult{.ok = false, .message = "pass declined"};
                },
        },
    };
    const auto pass_result = mirakana::execute_frame_graph_v1_schedule(
        pass_schedule, mirakana::FrameGraphExecutionCallbacks{.pass_callbacks = failing_passes});

    MK_REQUIRE(!pass_result.succeeded());
    MK_REQUIRE(pass_result.pass_callbacks_invoked == 0);
    MK_REQUIRE(pass_result.diagnostics.size() == 1);
    MK_REQUIRE(pass_result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::invalid_pass);
    MK_REQUIRE(pass_result.diagnostics[0].pass == "scene");
    MK_REQUIRE(pass_result.diagnostics[0].message == "pass declined");

    const std::vector<mirakana::FrameGraphExecutionStep> barrier_schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
    };
    const auto barrier_result = mirakana::execute_frame_graph_v1_schedule(
        barrier_schedule, mirakana::FrameGraphExecutionCallbacks{
                              .pass_callbacks = {},
                              .barrier_callback =
                                  [](const mirakana::FrameGraphBarrier&) {
                                      return mirakana::FrameGraphExecutionCallbackResult{.ok = false,
                                                                                         .message = "barrier declined"};
                                  },
                          });

    MK_REQUIRE(!barrier_result.succeeded());
    MK_REQUIRE(barrier_result.barrier_callbacks_invoked == 0);
    MK_REQUIRE(barrier_result.diagnostics.size() == 1);
    MK_REQUIRE(barrier_result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::invalid_resource);
    MK_REQUIRE(barrier_result.diagnostics[0].pass == "postprocess");
    MK_REQUIRE(barrier_result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(barrier_result.diagnostics[0].message == "barrier declined");
}

MK_TEST("frame graph v1 callback execution converts thrown barrier callbacks to diagnostics") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
    };

    const auto result = mirakana::execute_frame_graph_v1_schedule(
        schedule,
        mirakana::FrameGraphExecutionCallbacks{
            .pass_callbacks = {},
            .barrier_callback = [](const mirakana::FrameGraphBarrier&) -> mirakana::FrameGraphExecutionCallbackResult {
                throw std::runtime_error("barrier failed");
            },
        });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barrier_callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::FrameGraphDiagnosticCode::invalid_resource);
    MK_REQUIRE(result.diagnostics[0].pass == "postprocess");
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph barrier callback threw an exception");
}

MK_TEST("frame graph v1 maps texture barrier accesses to rhi resource states") {
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::color_attachment_write) ==
               mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::depth_attachment_write) ==
               mirakana::rhi::ResourceState::depth_write);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::shader_read) ==
               mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::copy_source) ==
               mirakana::rhi::ResourceState::copy_source);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::copy_destination) ==
               mirakana::rhi::ResourceState::copy_destination);
    MK_REQUIRE(mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::present) ==
               mirakana::rhi::ResourceState::present);
    MK_REQUIRE(!mirakana::frame_graph_texture_state_for_access(mirakana::FrameGraphAccess::unknown).has_value());
}

MK_TEST("frame graph v1 records scheduled texture barriers through rhi command lists") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};
    const auto result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, bindings);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 1);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(device.stats().resource_transitions == 2);
}

MK_TEST("frame graph v1 records sequential texture barriers for the same binding") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("postprocess"),
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "postprocess",
            .to_pass = "readback",
            .from = mirakana::FrameGraphAccess::shader_read,
            .to = mirakana::FrameGraphAccess::copy_source,
        }),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource |
                 mirakana::rhi::TextureUsage::copy_source,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::copy_source,
                                 mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};

    const auto result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, bindings);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 2);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::copy_source);
    MK_REQUIRE(device.stats().resource_transitions == 3);
}

MK_TEST("frame graph v1 skips duplicate texture barriers already satisfied by an earlier barrier") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess-a",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess-b",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};

    const auto result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, bindings);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 1);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(device.stats().resource_transitions == 2);
}

MK_TEST("frame graph v1 texture barrier recording propagates shared texture handle state") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "early-color",
            .from_pass = "early",
            .to_pass = "sample",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{
        mirakana::FrameGraphTextureBinding{
            .resource = "early-color",
            .texture = texture,
            .current_state = mirakana::rhi::ResourceState::render_target,
        },
        mirakana::FrameGraphTextureBinding{
            .resource = "late-color",
            .texture = texture,
            .current_state = mirakana::rhi::ResourceState::render_target,
        },
    };

    const auto result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, bindings);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 1);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(bindings[1].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(device.stats().resource_transitions == 2);
}

MK_TEST("frame graph v1 texture barrier recording rejects conflicting shared texture handle states") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "early-color",
            .from_pass = "early",
            .to_pass = "sample",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{
        mirakana::FrameGraphTextureBinding{
            .resource = "early-color",
            .texture = texture,
            .current_state = mirakana::rhi::ResourceState::render_target,
        },
        mirakana::FrameGraphTextureBinding{
            .resource = "late-color",
            .texture = texture,
            .current_state = mirakana::rhi::ResourceState::shader_read,
        },
    };

    const auto result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, bindings);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].resource == "late-color");
    MK_REQUIRE(result.diagnostics[0].message ==
               "frame graph texture bindings sharing a handle disagree on current state");
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(bindings[1].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(device.stats().resource_transitions == 0);
}

MK_TEST("frame graph v1 texture barrier recording returns stable diagnostics for rhi failures") {
    ThrowingTransitionRhiDevice device;
    device.throw_on_submit = false;
    device.throw_on_transition = 2;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);

    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
    };
    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};

    const auto result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, bindings);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture barrier recording failed");
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::render_target);
}

MK_TEST("frame graph v1 texture barrier recording diagnoses missing and stale bindings") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
    };

    mirakana::rhi::NullRhiDevice device;
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    std::vector<mirakana::FrameGraphTextureBinding> no_bindings;
    const auto missing = mirakana::record_frame_graph_texture_barriers(*commands, schedule, no_bindings);
    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.diagnostics.size() == 1);
    MK_REQUIRE(missing.diagnostics[0].resource == "scene-color");

    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    std::vector<mirakana::FrameGraphTextureBinding> stale{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::undefined,
    }};
    const auto stale_result = mirakana::record_frame_graph_texture_barriers(*commands, schedule, stale);
    MK_REQUIRE(!stale_result.succeeded());
    MK_REQUIRE(stale_result.diagnostics.size() == 1);
    MK_REQUIRE(stale_result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(stale[0].current_state == mirakana::rhi::ResourceState::undefined);
}

MK_TEST("frame graph rhi texture schedule execution interleaves barriers and pass callbacks") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};
    std::vector<mirakana::rhi::ResourceState> observed_states;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&bindings, &observed_states](std::string_view) {
                    observed_states.push_back(bindings[0].current_state);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback =
                [&bindings, &observed_states](std::string_view) {
                    observed_states.push_back(bindings[0].current_state);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = {},
        .final_states = {},
    });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 1);
    MK_REQUIRE(result.pass_callbacks_invoked == 2);
    MK_REQUIRE(observed_states.size() == 2);
    MK_REQUIRE(observed_states[0] == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(observed_states[1] == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(device.stats().resource_transitions == 2);
}

MK_TEST("frame graph rhi texture schedule execution rejects undeclared writer-updated states before callbacks") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::undefined,
    }};
    std::vector<mirakana::rhi::ResourceState> observed_states;
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&bindings, &callbacks_invoked, &observed_states](std::string_view) {
                    ++callbacks_invoked;
                    observed_states.push_back(bindings[0].current_state);
                    bindings[0].current_state = mirakana::rhi::ResourceState::render_target;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback =
                [&bindings, &callbacks_invoked, &observed_states](std::string_view) {
                    ++callbacks_invoked;
                    observed_states.push_back(bindings[0].current_state);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = {},
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(observed_states.empty());
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::undefined);
    MK_REQUIRE(device.stats().resource_transitions == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].pass == "postprocess");
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message ==
               "frame graph texture binding current state does not match barrier before state");
}

MK_TEST("frame graph rhi texture schedule execution records pass target states before callbacks") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::undefined,
    }};
    const std::vector<mirakana::FrameGraphTexturePassTargetState> pass_target_states{
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "scene",
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };
    const auto pass_target_accesses = mirakana::build_frame_graph_texture_pass_target_accesses(desc);
    std::vector<mirakana::rhi::ResourceState> observed_states;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&bindings, &observed_states](std::string_view) {
                    observed_states.push_back(bindings[0].current_state);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback =
                [&bindings, &observed_states](std::string_view) {
                    observed_states.push_back(bindings[0].current_state);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = pass_target_accesses,
        .pass_target_states = pass_target_states,
        .final_states = {},
    });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 2);
    MK_REQUIRE(result.pass_target_state_barriers_recorded == 1);
    MK_REQUIRE(result.pass_callbacks_invoked == 2);
    MK_REQUIRE(observed_states.size() == 2);
    MK_REQUIRE(observed_states[0] == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(observed_states[1] == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(device.stats().resource_transitions == 2);
}

MK_TEST("frame graph rhi texture schedule execution hands off shared texture handle state between aliases") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("early.write"),
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "early-color",
            .from_pass = "early.write",
            .to_pass = "early.read",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("early.read"),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("late.write"),
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "late-color",
            .from_pass = "late.write",
            .to_pass = "late.read",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("late.read"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{
        mirakana::FrameGraphTextureBinding{
            .resource = "early-color",
            .texture = texture,
            .current_state = mirakana::rhi::ResourceState::undefined,
        },
        mirakana::FrameGraphTextureBinding{
            .resource = "late-color",
            .texture = texture,
            .current_state = mirakana::rhi::ResourceState::undefined,
        },
    };
    const std::vector<mirakana::FrameGraphTexturePassTargetAccess> pass_target_accesses{
        mirakana::FrameGraphTexturePassTargetAccess{
            .pass_name = "early.write",
            .resource = "early-color",
            .access = mirakana::FrameGraphAccess::color_attachment_write,
        },
        mirakana::FrameGraphTexturePassTargetAccess{
            .pass_name = "late.write",
            .resource = "late-color",
            .access = mirakana::FrameGraphAccess::color_attachment_write,
        },
    };
    const std::vector<mirakana::FrameGraphTexturePassTargetState> pass_target_states{
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "early.write",
            .resource = "early-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "late.write",
            .resource = "late-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };
    const std::vector<mirakana::FrameGraphTextureFinalState> final_states{
        mirakana::FrameGraphTextureFinalState{
            .resource = "early-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };

    std::vector<std::pair<mirakana::rhi::ResourceState, mirakana::rhi::ResourceState>> observed_states;
    const auto record_states = [&bindings, &observed_states](std::string_view) {
        observed_states.emplace_back(bindings[0].current_state, bindings[1].current_state);
        return mirakana::FrameGraphExecutionCallbackResult{};
    };
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{.pass_name = "early.write", .callback = record_states},
        mirakana::FrameGraphPassExecutionBinding{.pass_name = "early.read", .callback = record_states},
        mirakana::FrameGraphPassExecutionBinding{.pass_name = "late.write", .callback = record_states},
        mirakana::FrameGraphPassExecutionBinding{.pass_name = "late.read", .callback = record_states},
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = pass_target_accesses,
        .pass_target_states = pass_target_states,
        .final_states = final_states,
    });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 5);
    MK_REQUIRE(result.pass_target_state_barriers_recorded == 2);
    MK_REQUIRE(result.final_state_barriers_recorded == 1);
    MK_REQUIRE(result.pass_callbacks_invoked == 4);
    MK_REQUIRE(observed_states.size() == 4);
    MK_REQUIRE(observed_states[0].first == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(observed_states[0].second == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(observed_states[1].first == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(observed_states[1].second == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(observed_states[2].first == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(observed_states[2].second == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(observed_states[3].first == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(observed_states[3].second == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(bindings[1].current_state == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(device.stats().resource_transitions == 5);
}

MK_TEST("frame graph rhi texture target access helper derives concrete writer rows") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-depth", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write},
                   mirakana::FrameGraphResourceAccess{.resource = "scene-depth",
                                                      .access = mirakana::FrameGraphAccess::depth_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto target_accesses = mirakana::build_frame_graph_texture_pass_target_accesses(desc);

    MK_REQUIRE(target_accesses.size() == 2);
    MK_REQUIRE(target_accesses[0].pass_name == "scene");
    MK_REQUIRE(target_accesses[0].resource == "scene-color");
    MK_REQUIRE(target_accesses[0].access == mirakana::FrameGraphAccess::color_attachment_write);
    MK_REQUIRE(target_accesses[1].pass_name == "scene");
    MK_REQUIRE(target_accesses[1].resource == "scene-depth");
    MK_REQUIRE(target_accesses[1].access == mirakana::FrameGraphAccess::depth_attachment_write);
}

MK_TEST("frame graph rhi texture schedule execution rejects pass target state without writer access") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("postprocess"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::shader_read,
    }};
    const std::vector<mirakana::FrameGraphTexturePassTargetAccess> pass_target_accesses;
    const std::vector<mirakana::FrameGraphTexturePassTargetState> pass_target_states{
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "postprocess",
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = pass_target_accesses,
        .pass_target_states = pass_target_states,
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_target_state_barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].pass == "postprocess");
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture pass target state has no declared writer access");
}

MK_TEST("frame graph rhi texture schedule execution rejects pass target state access mismatch") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::undefined,
    }};
    const std::vector<mirakana::FrameGraphTexturePassTargetAccess> pass_target_accesses{
        mirakana::FrameGraphTexturePassTargetAccess{
            .pass_name = "scene",
            .resource = "scene-color",
            .access = mirakana::FrameGraphAccess::color_attachment_write,
        },
    };
    const std::vector<mirakana::FrameGraphTexturePassTargetState> pass_target_states{
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "scene",
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::shader_read,
        },
    };
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback = [](std::string_view) { return mirakana::FrameGraphExecutionCallbackResult{}; },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = pass_target_accesses,
        .pass_target_states = pass_target_states,
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_target_state_barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].pass == "scene");
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture pass target state disagrees with writer access");
}

MK_TEST("frame graph rhi texture schedule execution rejects duplicate pass target accesses before callbacks") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::undefined,
    }};
    const std::vector<mirakana::FrameGraphTexturePassTargetAccess> pass_target_accesses{
        mirakana::FrameGraphTexturePassTargetAccess{
            .pass_name = "scene",
            .resource = "scene-color",
            .access = mirakana::FrameGraphAccess::color_attachment_write,
        },
        mirakana::FrameGraphTexturePassTargetAccess{
            .pass_name = "scene",
            .resource = "scene-color",
            .access = mirakana::FrameGraphAccess::color_attachment_write,
        },
    };
    const std::vector<mirakana::FrameGraphTexturePassTargetState> pass_target_states{
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "scene",
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = pass_target_accesses,
        .pass_target_states = pass_target_states,
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_target_state_barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].pass == "scene");
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture pass target access is declared more than once");
}

MK_TEST("frame graph rhi texture schedule execution validates duplicate pass target states before callbacks") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::undefined,
    }};
    const std::vector<mirakana::FrameGraphTexturePassTargetState> pass_target_states{
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "scene",
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "scene",
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };
    const std::vector<mirakana::FrameGraphTexturePassTargetAccess> pass_target_accesses{
        mirakana::FrameGraphTexturePassTargetAccess{
            .pass_name = "scene",
            .resource = "scene-color",
            .access = mirakana::FrameGraphAccess::color_attachment_write,
        },
    };
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = pass_target_accesses,
        .pass_target_states = pass_target_states,
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_target_state_barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].pass == "scene");
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture pass target state is declared more than once");
}

MK_TEST("frame graph rhi texture schedule execution rejects unscheduled pass target states") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::undefined,
    }};
    const std::vector<mirakana::FrameGraphTexturePassTargetState> pass_target_states{
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "postprocess",
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = pass_target_states,
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_target_state_barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].pass == "postprocess");
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture pass target state targets an unscheduled pass");
}

MK_TEST("frame graph rhi texture schedule execution rejects undefined pass target states") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::undefined,
    }};
    const std::vector<mirakana::FrameGraphTexturePassTargetState> pass_target_states{
        mirakana::FrameGraphTexturePassTargetState{
            .pass_name = "scene",
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::undefined,
        },
    };
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = pass_target_states,
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_target_state_barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].pass == "scene");
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture pass target state cannot be undefined");
}

MK_TEST("frame graph rhi texture schedule execution records final texture states after callbacks") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};
    const std::vector<mirakana::FrameGraphTextureFinalState> final_states{
        mirakana::FrameGraphTextureFinalState{
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };
    std::vector<mirakana::rhi::ResourceState> observed_states;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&bindings, &observed_states](std::string_view) {
                    observed_states.push_back(bindings[0].current_state);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback =
                [&bindings, &observed_states](std::string_view) {
                    observed_states.push_back(bindings[0].current_state);
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = {},
        .final_states = final_states,
    });

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 2);
    MK_REQUIRE(result.final_state_barriers_recorded == 1);
    MK_REQUIRE(result.pass_callbacks_invoked == 2);
    MK_REQUIRE(observed_states.size() == 2);
    MK_REQUIRE(observed_states[0] == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(observed_states[1] == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(device.stats().resource_transitions == 3);
}

MK_TEST("frame graph rhi texture schedule execution validates final states before callbacks") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};
    const std::vector<mirakana::FrameGraphTextureFinalState> final_states{
        mirakana::FrameGraphTextureFinalState{
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
        mirakana::FrameGraphTextureFinalState{
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::shader_read,
        },
    };
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = {},
        .final_states = final_states,
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.final_state_barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture final state is declared more than once");
}

MK_TEST("frame graph rhi texture schedule execution reports final-state transition failures") {
    mirakana::FrameGraphV1Desc desc;
    desc.resources.push_back(mirakana::FrameGraphResourceV1Desc{
        .name = "scene-color", .lifetime = mirakana::FrameGraphResourceLifetime::transient});
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "scene",
        .reads = {},
        .writes = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                      .access = mirakana::FrameGraphAccess::color_attachment_write}},
    });
    desc.passes.push_back(mirakana::FrameGraphPassV1Desc{
        .name = "postprocess",
        .reads = {mirakana::FrameGraphResourceAccess{.resource = "scene-color",
                                                     .access = mirakana::FrameGraphAccess::shader_read}},
        .writes = {},
    });

    const auto plan = mirakana::compile_frame_graph_v1(desc);
    MK_REQUIRE(plan.succeeded());
    const auto schedule = mirakana::schedule_frame_graph_v1_execution(plan);

    ThrowingTransitionRhiDevice device;
    device.throw_on_submit = false;
    device.throw_on_transition = 3;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};
    const std::vector<mirakana::FrameGraphTextureFinalState> final_states{
        mirakana::FrameGraphTextureFinalState{
            .resource = "scene-color",
            .state = mirakana::rhi::ResourceState::render_target,
        },
    };
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback = [](std::string_view) { return mirakana::FrameGraphExecutionCallbackResult{}; },
        },
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback = [](std::string_view) { return mirakana::FrameGraphExecutionCallbackResult{}; },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = {},
        .final_states = final_states,
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 1);
    MK_REQUIRE(result.final_state_barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 2);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::shader_read);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
    MK_REQUIRE(result.diagnostics[0].message == "frame graph texture final-state barrier recording failed");
}

MK_TEST("frame graph rhi texture schedule execution validates barriers before pass callbacks") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("postprocess"),
    };

    mirakana::rhi::NullRhiDevice device;
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    std::vector<mirakana::FrameGraphTextureBinding> no_bindings;
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "postprocess",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = no_bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = {},
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].resource == "scene-color");
}

MK_TEST("frame graph rhi texture schedule execution validates pass callbacks before barriers") {
    const std::vector<mirakana::FrameGraphExecutionStep> schedule{
        mirakana::FrameGraphExecutionStep::make_pass_invoke("scene"),
        mirakana::FrameGraphExecutionStep::make_barrier(mirakana::FrameGraphBarrier{
            .resource = "scene-color",
            .from_pass = "scene",
            .to_pass = "postprocess",
            .from = mirakana::FrameGraphAccess::color_attachment_write,
            .to = mirakana::FrameGraphAccess::shader_read,
        }),
        mirakana::FrameGraphExecutionStep::make_pass_invoke("postprocess"),
    };

    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(texture, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);

    std::vector<mirakana::FrameGraphTextureBinding> bindings{mirakana::FrameGraphTextureBinding{
        .resource = "scene-color",
        .texture = texture,
        .current_state = mirakana::rhi::ResourceState::render_target,
    }};
    std::size_t callbacks_invoked = 0;
    const std::vector<mirakana::FrameGraphPassExecutionBinding> pass_callbacks{
        mirakana::FrameGraphPassExecutionBinding{
            .pass_name = "scene",
            .callback =
                [&callbacks_invoked](std::string_view) {
                    ++callbacks_invoked;
                    return mirakana::FrameGraphExecutionCallbackResult{};
                },
        },
    };

    const auto result = mirakana::execute_frame_graph_rhi_texture_schedule(mirakana::FrameGraphRhiTextureExecutionDesc{
        .commands = commands.get(),
        .schedule = schedule,
        .texture_bindings = bindings,
        .pass_callbacks = pass_callbacks,
        .pass_target_accesses = {},
        .pass_target_states = {},
        .final_states = {},
    });

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.barriers_recorded == 0);
    MK_REQUIRE(result.pass_callbacks_invoked == 0);
    MK_REQUIRE(callbacks_invoked == 0);
    MK_REQUIRE(bindings[0].current_state == mirakana::rhi::ResourceState::render_target);
    MK_REQUIRE(device.stats().resource_transitions == 1);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].pass == "postprocess");
}

MK_TEST("sprite batch planner preserves order and coalesces adjacent compatible runs") {
    const auto atlas_a = mirakana::AssetId::from_name("textures/atlas_a");
    const auto atlas_b = mirakana::AssetId::from_name("textures/atlas_b");
    const auto textured_a = mirakana::SpriteTextureRegion{
        .enabled = true,
        .atlas_page = atlas_a,
        .uv_rect = mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 0.5F, .v1 = 1.0F},
    };
    const auto textured_b = mirakana::SpriteTextureRegion{
        .enabled = true,
        .atlas_page = atlas_b,
        .uv_rect = mirakana::SpriteUvRect{.u0 = 0.5F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F},
    };

    const std::vector<mirakana::SpriteCommand> sprites{
        mirakana::SpriteCommand{},
        mirakana::SpriteCommand{},
        mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{}, .color = mirakana::Color{}, .texture = textured_a},
        mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{}, .color = mirakana::Color{}, .texture = textured_a},
        mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{}, .color = mirakana::Color{}, .texture = textured_b},
        mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{}, .color = mirakana::Color{}, .texture = textured_a},
    };

    const auto plan = mirakana::plan_sprite_batches(sprites);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.sprite_count == 6);
    MK_REQUIRE(plan.textured_sprite_count == 4);
    MK_REQUIRE(plan.draw_count == 4);
    MK_REQUIRE(plan.texture_bind_count == 3);
    MK_REQUIRE(plan.batches.size() == 4);
    MK_REQUIRE(plan.batches[0].kind == mirakana::SpriteBatchKind::solid_color);
    MK_REQUIRE(plan.batches[0].first_sprite == 0);
    MK_REQUIRE(plan.batches[0].sprite_count == 2);
    MK_REQUIRE(plan.batches[1].kind == mirakana::SpriteBatchKind::textured);
    MK_REQUIRE(plan.batches[1].atlas_page == atlas_a);
    MK_REQUIRE(plan.batches[1].first_sprite == 2);
    MK_REQUIRE(plan.batches[1].sprite_count == 2);
    MK_REQUIRE(plan.batches[2].kind == mirakana::SpriteBatchKind::textured);
    MK_REQUIRE(plan.batches[2].atlas_page == atlas_b);
    MK_REQUIRE(plan.batches[2].first_sprite == 4);
    MK_REQUIRE(plan.batches[2].sprite_count == 1);
    MK_REQUIRE(plan.batches[3].kind == mirakana::SpriteBatchKind::textured);
    MK_REQUIRE(plan.batches[3].atlas_page == atlas_a);
    MK_REQUIRE(plan.batches[3].first_sprite == 5);
    MK_REQUIRE(plan.batches[3].sprite_count == 1);
}

MK_TEST("sprite batch planner diagnoses invalid texture metadata as untextured fallback") {
    const auto atlas = mirakana::AssetId::from_name("textures/atlas");
    const std::vector<mirakana::SpriteCommand> sprites{
        mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{},
            .color = mirakana::Color{},
            .texture =
                mirakana::SpriteTextureRegion{
                    .enabled = true,
                    .atlas_page = mirakana::AssetId{},
                    .uv_rect = mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}},
        },
        mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{},
            .color = mirakana::Color{},
            .texture =
                mirakana::SpriteTextureRegion{
                    .enabled = true,
                    .atlas_page = atlas,
                    .uv_rect = mirakana::SpriteUvRect{.u0 = 0.7F, .v0 = 0.0F, .u1 = 0.6F, .v1 = 1.0F}},
        },
        mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{},
            .color = mirakana::Color{},
            .texture =
                mirakana::SpriteTextureRegion{
                    .enabled = true,
                    .atlas_page = atlas,
                    .uv_rect = mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}},
        },
    };

    const auto plan = mirakana::plan_sprite_batches(sprites);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.sprite_count == 3);
    MK_REQUIRE(plan.textured_sprite_count == 1);
    MK_REQUIRE(plan.draw_count == 2);
    MK_REQUIRE(plan.texture_bind_count == 1);
    MK_REQUIRE(plan.batches.size() == 2);
    MK_REQUIRE(plan.batches[0].kind == mirakana::SpriteBatchKind::solid_color);
    MK_REQUIRE(plan.batches[0].first_sprite == 0);
    MK_REQUIRE(plan.batches[0].sprite_count == 2);
    MK_REQUIRE(plan.batches[1].kind == mirakana::SpriteBatchKind::textured);
    MK_REQUIRE(plan.batches[1].first_sprite == 2);
    MK_REQUIRE(plan.batches[1].sprite_count == 1);
    MK_REQUIRE(plan.diagnostics.size() == 2);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::SpriteBatchDiagnosticCode::missing_texture_atlas);
    MK_REQUIRE(plan.diagnostics[0].sprite_index == 0);
    MK_REQUIRE(plan.diagnostics[1].code == mirakana::SpriteBatchDiagnosticCode::invalid_uv_rect);
    MK_REQUIRE(plan.diagnostics[1].sprite_index == 1);
}

MK_TEST("shadow map foundation builds a deterministic directional plan") {
    const auto plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 256, .height = 256},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 3,
        .receiver_count = 2,
        .directional_cascade_count = 3,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.directional_cascade_count == 3);
    MK_REQUIRE(plan.cascade_tile_extent.width == 256);
    MK_REQUIRE(plan.cascade_tile_extent.height == 256);
    MK_REQUIRE(plan.depth_texture.extent.width == 768);
    MK_REQUIRE(plan.depth_texture.extent.height == 256);
    MK_REQUIRE(plan.depth_texture.extent.depth == 1);
    MK_REQUIRE(plan.depth_texture.format == mirakana::rhi::Format::depth24_stencil8);
    MK_REQUIRE(plan.depth_texture.usage ==
               (mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource));
    MK_REQUIRE(plan.caster_count == 3);
    MK_REQUIRE(plan.receiver_count == 2);
    MK_REQUIRE(plan.frame_graph_plan.succeeded());
    MK_REQUIRE(plan.frame_graph_plan.ordered_passes.size() == 2);
    MK_REQUIRE(plan.frame_graph_plan.ordered_passes[0] == "shadow.directional.depth");
    MK_REQUIRE(plan.frame_graph_plan.ordered_passes[1] == "shadow.receiver.resolve");
    MK_REQUIRE(plan.frame_graph_plan.barriers.size() == 1);
    MK_REQUIRE(plan.frame_graph_execution.size() == 3);
    MK_REQUIRE(plan.frame_graph_execution[0].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(plan.frame_graph_execution[0].pass_name == "shadow.directional.depth");
    MK_REQUIRE(plan.frame_graph_execution[1].kind == mirakana::FrameGraphExecutionStep::Kind::barrier);
    MK_REQUIRE(plan.frame_graph_execution[1].barrier.resource == "shadow.depth");
    MK_REQUIRE(plan.frame_graph_execution[2].kind == mirakana::FrameGraphExecutionStep::Kind::pass_invoke);
    MK_REQUIRE(plan.frame_graph_execution[2].pass_name == "shadow.receiver.resolve");
}

MK_TEST("practical shadow cascade distances bracket near plane and far plane with strict monotonicity") {
    const auto distances = mirakana::compute_practical_shadow_cascade_distances(4, 0.5F, 128.0F, 0.75F);
    MK_REQUIRE(distances.size() == 5);
    MK_REQUIRE(distances.front() == 0.5F);
    MK_REQUIRE(distances.back() == 128.0F);
    for (std::size_t i = 0; i + 1 < distances.size(); ++i) {
        MK_REQUIRE(distances[i] < distances[i + 1]);
    }
}

MK_TEST("practical shadow cascade distances with single cascade returns near and far only") {
    const auto distances = mirakana::compute_practical_shadow_cascade_distances(1, 1.0F, 1000.0F, 0.5F);
    MK_REQUIRE(distances.size() == 2);
    MK_REQUIRE(distances[0] == 1.0F);
    MK_REQUIRE(distances[1] == 1000.0F);
}

MK_TEST("practical shadow cascade distances reject invalid arguments") {
    bool threw_zero_cascades = false;
    try {
        (void)mirakana::compute_practical_shadow_cascade_distances(0, 1.0F, 100.0F);
    } catch (const std::invalid_argument&) {
        threw_zero_cascades = true;
    }
    MK_REQUIRE(threw_zero_cascades);

    bool threw_near = false;
    try {
        (void)mirakana::compute_practical_shadow_cascade_distances(2, 0.0F, 100.0F);
    } catch (const std::invalid_argument&) {
        threw_near = true;
    }
    MK_REQUIRE(threw_near);

    bool threw_far = false;
    try {
        (void)mirakana::compute_practical_shadow_cascade_distances(2, 10.0F, 5.0F);
    } catch (const std::invalid_argument&) {
        threw_far = true;
    }
    MK_REQUIRE(threw_far);

    bool threw_lambda = false;
    try {
        (void)mirakana::compute_practical_shadow_cascade_distances(2, 0.1F, 50.0F, 1.5F);
    } catch (const std::invalid_argument&) {
        threw_lambda = true;
    }
    MK_REQUIRE(threw_lambda);
}

MK_TEST("shadow map foundation reports deterministic invalid plan diagnostics") {
    const auto missing_light = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::unknown, .casts_shadows = false},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(!missing_light.succeeded());
    MK_REQUIRE(mirakana::has_shadow_map_diagnostic(missing_light, mirakana::ShadowMapDiagnosticCode::missing_light));

    const auto unsupported_light = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::point, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(!unsupported_light.succeeded());
    MK_REQUIRE(mirakana::has_shadow_map_diagnostic(unsupported_light,
                                                   mirakana::ShadowMapDiagnosticCode::unsupported_light_type));

    const auto disabled_shadow = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light =
            mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = false},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(!disabled_shadow.succeeded());
    MK_REQUIRE(mirakana::has_shadow_map_diagnostic(disabled_shadow,
                                                   mirakana::ShadowMapDiagnosticCode::light_does_not_cast_shadows));

    const auto invalid_cascade_count = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 256, .height = 256},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
        .directional_cascade_count = 9,
    });
    MK_REQUIRE(!invalid_cascade_count.succeeded());
    MK_REQUIRE(mirakana::has_shadow_map_diagnostic(
        invalid_cascade_count, mirakana::ShadowMapDiagnosticCode::invalid_directional_cascade_count));

    const auto invalid_extent = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 0, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(!invalid_extent.succeeded());
    MK_REQUIRE(mirakana::has_shadow_map_diagnostic(invalid_extent, mirakana::ShadowMapDiagnosticCode::invalid_extent));

    const auto unsupported_format = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::rgba8_unorm,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(!unsupported_format.succeeded());
    MK_REQUIRE(mirakana::has_shadow_map_diagnostic(unsupported_format,
                                                   mirakana::ShadowMapDiagnosticCode::unsupported_depth_format));

    const auto missing_casters = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 0,
        .receiver_count = 1,
    });
    MK_REQUIRE(!missing_casters.succeeded());
    MK_REQUIRE(
        mirakana::has_shadow_map_diagnostic(missing_casters, mirakana::ShadowMapDiagnosticCode::missing_casters));

    const auto missing_receivers = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 0,
    });
    MK_REQUIRE(!missing_receivers.succeeded());
    MK_REQUIRE(
        mirakana::has_shadow_map_diagnostic(missing_receivers, mirakana::ShadowMapDiagnosticCode::missing_receivers));
}

MK_TEST("shadow receiver foundation builds deterministic sampled depth descriptor policy") {
    const auto shadow_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional,
                                              .casts_shadows = true,
                                              .source_index = 2},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 3,
        .receiver_count = 2,
    });
    MK_REQUIRE(shadow_plan.succeeded());

    const auto light_space =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
            .focus_center = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
            .focus_radius = 40.0F,
            .depth_radius = 80.0F,
        });
    MK_REQUIRE(light_space.succeeded());

    const auto receiver_plan = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &shadow_plan,
        .light_space = &light_space,
        .depth_bias = 0.002F,
        .lit_intensity = 1.0F,
        .shadow_intensity = 0.3F,
    });

    MK_REQUIRE(receiver_plan.succeeded());
    MK_REQUIRE(receiver_plan.receiver_count == 2);
    MK_REQUIRE(receiver_plan.directional_cascade_count == shadow_plan.directional_cascade_count);
    MK_REQUIRE(receiver_plan.cascade_tile_extent.width == shadow_plan.cascade_tile_extent.width);
    MK_REQUIRE(receiver_plan.cascade_tile_extent.height == shadow_plan.cascade_tile_extent.height);
    MK_REQUIRE(receiver_plan.clip_from_world_cascades.size() == shadow_plan.directional_cascade_count);
    MK_REQUIRE(receiver_plan.depth_texture.extent.width == shadow_plan.depth_texture.extent.width);
    MK_REQUIRE(receiver_plan.depth_texture.extent.height == shadow_plan.depth_texture.extent.height);
    MK_REQUIRE(receiver_plan.depth_texture.extent.depth == shadow_plan.depth_texture.extent.depth);
    MK_REQUIRE(receiver_plan.depth_texture.format == shadow_plan.depth_texture.format);
    MK_REQUIRE(receiver_plan.depth_texture.usage == shadow_plan.depth_texture.usage);
    MK_REQUIRE(receiver_plan.depth_bias == 0.002F);
    MK_REQUIRE(receiver_plan.lit_intensity == 1.0F);
    MK_REQUIRE(receiver_plan.shadow_intensity == 0.3F);
    MK_REQUIRE(receiver_plan.filter_mode == mirakana::ShadowReceiverFilterMode::fixed_pcf_3x3);
    MK_REQUIRE(receiver_plan.filter_radius_texels == 1.0F);
    MK_REQUIRE(receiver_plan.filter_tap_count == 9);
    MK_REQUIRE(receiver_plan.sampler.min_filter == mirakana::rhi::SamplerFilter::nearest);
    MK_REQUIRE(receiver_plan.sampler.mag_filter == mirakana::rhi::SamplerFilter::nearest);
    MK_REQUIRE(receiver_plan.sampler.address_u == mirakana::rhi::SamplerAddressMode::clamp_to_edge);
    MK_REQUIRE(receiver_plan.sampler.address_v == mirakana::rhi::SamplerAddressMode::clamp_to_edge);
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings.size() == 3);
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[0].binding ==
               mirakana::shadow_receiver_depth_texture_binding());
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[0].type == mirakana::rhi::DescriptorType::sampled_texture);
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[0].stages ==
               mirakana::rhi::ShaderStageVisibility::fragment);
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[1].binding == mirakana::shadow_receiver_sampler_binding());
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[1].type == mirakana::rhi::DescriptorType::sampler);
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[1].stages ==
               mirakana::rhi::ShaderStageVisibility::fragment);
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[2].binding ==
               mirakana::shadow_receiver_constants_binding());
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[2].type == mirakana::rhi::DescriptorType::uniform_buffer);
    MK_REQUIRE(receiver_plan.descriptor_set_layout.bindings[2].stages ==
               mirakana::rhi::ShaderStageVisibility::fragment);
}

MK_TEST("shadow receiver foundation reports deterministic invalid policy diagnostics") {
    const auto failed_shadow_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::unknown, .casts_shadows = false},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(!failed_shadow_plan.succeeded());

    const auto null_plan = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{});
    MK_REQUIRE(!null_plan.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(
        null_plan, mirakana::ShadowReceiverDiagnosticCode::invalid_shadow_map_plan));

    const auto failed_input = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &failed_shadow_plan,
    });
    MK_REQUIRE(!failed_input.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(
        failed_input, mirakana::ShadowReceiverDiagnosticCode::invalid_shadow_map_plan));

    const mirakana::ShadowMapPlan forged_shadow_plan;
    const auto forged_input = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &forged_shadow_plan,
    });
    MK_REQUIRE(!forged_input.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(
        forged_input, mirakana::ShadowReceiverDiagnosticCode::invalid_shadow_map_plan));

    const auto valid_shadow_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional, .casts_shadows = true},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(valid_shadow_plan.succeeded());

    const auto valid_light_space =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &valid_shadow_plan,
            .light_direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
            .focus_center = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .focus_radius = 50.0F,
            .depth_radius = 100.0F,
        });
    MK_REQUIRE(valid_light_space.succeeded());

    const auto missing_light_space = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &valid_shadow_plan,
        .light_space = nullptr,
    });
    MK_REQUIRE(!missing_light_space.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(
        missing_light_space, mirakana::ShadowReceiverDiagnosticCode::invalid_light_space_plan));

    mirakana::DirectionalShadowLightSpacePlan failed_light_space;
    failed_light_space.diagnostics.push_back(mirakana::DirectionalShadowLightSpaceDiagnostic{
        .code = mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_light_direction,
        .message = "forced diagnostic"});
    const auto invalid_light_space_receiver = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &valid_shadow_plan,
        .light_space = &failed_light_space,
    });
    MK_REQUIRE(!invalid_light_space_receiver.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(
        invalid_light_space_receiver, mirakana::ShadowReceiverDiagnosticCode::invalid_light_space_plan));

    const auto invalid_bias = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &valid_shadow_plan,
        .light_space = &valid_light_space,
        .depth_bias = -0.001F,
    });
    MK_REQUIRE(!invalid_bias.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(invalid_bias,
                                                        mirakana::ShadowReceiverDiagnosticCode::invalid_depth_bias));

    const auto invalid_lit = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &valid_shadow_plan,
        .light_space = &valid_light_space,
        .lit_intensity = 1.2F,
    });
    MK_REQUIRE(!invalid_lit.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(invalid_lit,
                                                        mirakana::ShadowReceiverDiagnosticCode::invalid_lit_intensity));

    const auto invalid_shadow = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &valid_shadow_plan,
        .light_space = &valid_light_space,
        .shadow_intensity = -0.1F,
    });
    MK_REQUIRE(!invalid_shadow.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(
        invalid_shadow, mirakana::ShadowReceiverDiagnosticCode::invalid_shadow_intensity));

    const auto inverted_intensity = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &valid_shadow_plan,
        .light_space = &valid_light_space,
        .lit_intensity = 0.2F,
        .shadow_intensity = 0.3F,
    });
    MK_REQUIRE(!inverted_intensity.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(
        inverted_intensity, mirakana::ShadowReceiverDiagnosticCode::shadow_intensity_exceeds_lit_intensity));

    const auto invalid_filter_mode = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &valid_shadow_plan,
        .light_space = &valid_light_space,
        .filter_mode = static_cast<mirakana::ShadowReceiverFilterMode>(99),
    });
    MK_REQUIRE(!invalid_filter_mode.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(invalid_filter_mode,
                                                        mirakana::ShadowReceiverDiagnosticCode::invalid_filter_mode));

    const auto invalid_filter_radius = mirakana::build_shadow_receiver_plan(mirakana::ShadowReceiverDesc{
        .shadow_map = &valid_shadow_plan,
        .light_space = &valid_light_space,
        .filter_radius_texels = -1.0F,
    });
    MK_REQUIRE(!invalid_filter_radius.succeeded());
    MK_REQUIRE(mirakana::has_shadow_receiver_diagnostic(
        invalid_filter_radius, mirakana::ShadowReceiverDiagnosticCode::invalid_filter_radius_texels));
}

MK_TEST("stable directional shadow light-space policy builds a texel-snapped orthographic plan") {
    const auto shadow_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional,
                                              .casts_shadows = true,
                                              .source_index = 0},
        .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 3,
        .receiver_count = 3,
    });
    MK_REQUIRE(shadow_plan.succeeded());

    const auto plan = mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
        .shadow_map = &shadow_plan,
        .light_direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
        .focus_center = mirakana::Vec3{.x = 1.13F, .y = 2.0F, .z = -3.27F},
        .focus_radius = 8.0F,
        .depth_radius = 16.0F,
        .texel_snap = mirakana::ShadowLightSpaceTexelSnap::enabled,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.light_direction == (mirakana::Vec3{0.0F, -1.0F, 0.0F}));
    MK_REQUIRE(plan.focus_radius == 8.0F);
    MK_REQUIRE(plan.depth_radius == 16.0F);
    MK_REQUIRE(plan.ortho_width == 16.0F);
    MK_REQUIRE(plan.ortho_height == 16.0F);
    MK_REQUIRE(plan.texel_world_size == 16.0F / 1024.0F);
    MK_REQUIRE(std::abs(plan.snapped_focus_center.x - 1.125F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.snapped_focus_center.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.snapped_focus_center.z + 3.265625F) < 0.0001F);

    const auto snapped_light_space = mirakana::transform_point(plan.view_from_world, plan.snapped_focus_center);
    MK_REQUIRE(std::abs(snapped_light_space.x) < 0.0001F);
    MK_REQUIRE(std::abs(snapped_light_space.y) < 0.0001F);
    MK_REQUIRE(std::abs(snapped_light_space.z) < 0.0001F);
    MK_REQUIRE(std::abs(plan.clip_from_view_cascades[0].at(0, 0) - 0.125F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.clip_from_view_cascades[0].at(1, 1) - 0.125F) < 0.0001F);
}

MK_TEST("stable directional shadow light-space policy reports deterministic diagnostics") {
    const auto shadow_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        .light = mirakana::ShadowMapLightDesc{.type = mirakana::ShadowMapLightType::directional,
                                              .casts_shadows = true,
                                              .source_index = 0},
        .extent = mirakana::rhi::Extent2D{.width = 512, .height = 512},
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .caster_count = 1,
        .receiver_count = 1,
    });
    MK_REQUIRE(shadow_plan.succeeded());

    const auto null_shadow =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{});
    MK_REQUIRE(!null_shadow.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        null_shadow, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_shadow_map_plan));

    const auto invalid_direction =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .focus_radius = 1.0F,
            .depth_radius = 1.0F,
        });
    MK_REQUIRE(!invalid_direction.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        invalid_direction, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_light_direction));

    const auto invalid_center =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
            .focus_center = mirakana::Vec3{.x = std::numeric_limits<float>::infinity(), .y = 0.0F, .z = 0.0F},
            .focus_radius = 1.0F,
            .depth_radius = 1.0F,
        });
    MK_REQUIRE(!invalid_center.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        invalid_center, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_focus_center));

    const auto invalid_radius =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
            .focus_radius = 0.0F,
            .depth_radius = 1.0F,
        });
    MK_REQUIRE(!invalid_radius.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        invalid_radius, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_focus_radius));

    const auto invalid_depth =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
            .focus_radius = 1.0F,
            .depth_radius = -1.0F,
        });
    MK_REQUIRE(!invalid_depth.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        invalid_depth, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_depth_radius));
}

MK_TEST("rhi postprocess frame renderer records scene color and postprocess passes") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
    });
    MK_REQUIRE(renderer.frame_graph_pass_count() == 2);
    MK_REQUIRE(renderer.postprocess_ready());

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.frames_started == 1);
    MK_REQUIRE(renderer_stats.frames_finished == 1);
    MK_REQUIRE(renderer_stats.meshes_submitted == 1);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 2);
    MK_REQUIRE(renderer_stats.framegraph_barrier_steps_executed == 2);
    MK_REQUIRE(renderer_stats.postprocess_passes_executed == 1);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.textures_created == 1);
    MK_REQUIRE(rhi_stats.samplers_created == 1);
    MK_REQUIRE(rhi_stats.descriptor_set_layouts_created == 1);
    MK_REQUIRE(rhi_stats.descriptor_sets_allocated == 1);
    MK_REQUIRE(rhi_stats.descriptor_writes == 2);
    MK_REQUIRE(rhi_stats.render_passes_begun == 2);
    MK_REQUIRE(rhi_stats.graphics_pipelines_bound == 2);
    MK_REQUIRE(rhi_stats.descriptor_sets_bound == 1);
    MK_REQUIRE(rhi_stats.resource_transitions == 2);
    MK_REQUIRE(rhi_stats.draw_calls == 2);
    MK_REQUIRE(rhi_stats.present_calls == 1);
}

MK_TEST("rhi postprocess frame renderer records morph scene draws") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    const auto material_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{});
    const auto morph_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::storage_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex,
        },
    }});
    const auto material_set = device.allocate_descriptor_set(material_layout);
    const auto morph_set = device.allocate_descriptor_set(morph_layout);
    const auto morph_pipeline_layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, morph_layout},
        .push_constant_bytes = 0,
    });
    const auto morph_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_morph",
        .bytecode_size = 64,
    });
    const auto morph_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_morph",
        .bytecode_size = 64,
    });
    const auto morph_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = morph_pipeline_layout,
        .vertex_shader = morph_vertex_shader,
        .fragment_shader = morph_fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });
    const auto vertex_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto index_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 12, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});
    const auto morph_delta_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_destination});
    const auto morph_weight_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform | mirakana::rhi::BufferUsage::copy_destination});

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .scene_morph_graphics_pipeline = morph_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .mesh_binding =
            mirakana::MeshGpuBinding{
                .vertex_buffer = vertex_buffer,
                .index_buffer = index_buffer,
                .vertex_count = 3,
                .index_count = 3,
                .vertex_offset = 0,
                .index_offset = 0,
                .vertex_stride = 12,
                .index_format = mirakana::rhi::IndexFormat::uint32,
                .owner_device = &device,
            },
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = morph_pipeline_layout,
                                                         .descriptor_set = material_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = &device},
        .gpu_morphing = true,
        .morph_mesh =
            mirakana::MorphMeshGpuBinding{
                .position_delta_buffer = morph_delta_buffer,
                .morph_weight_buffer = morph_weight_buffer,
                .morph_descriptor_set = morph_set,
                .vertex_count = 3,
                .target_count = 1,
                .position_delta_bytes = 36,
                .morph_weight_uniform_allocation_bytes = 256,
                .owner_device = &device,
            },
    });
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.meshes_submitted == 1);
    MK_REQUIRE(renderer_stats.gpu_morph_draws == 1);
    MK_REQUIRE(renderer_stats.morph_descriptor_binds == 1);
}

MK_TEST("rhi postprocess frame renderer two-stage chain uses three frame graph passes and two postprocess draws") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess_chain",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_stage0 = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess_chain_0",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_stage1 = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess_chain_1",
        .bytecode_size = 64,
    });

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages =
            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_stage0, postprocess_fragment_stage1},
        .wait_for_completion = true,
    });
    MK_REQUIRE(renderer.frame_graph_pass_count() == 3);
    MK_REQUIRE(renderer.postprocess_ready());

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.frames_started == 1);
    MK_REQUIRE(renderer_stats.frames_finished == 1);
    MK_REQUIRE(renderer_stats.meshes_submitted == 1);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 3);
    MK_REQUIRE(renderer_stats.framegraph_barrier_steps_executed == 4);
    MK_REQUIRE(renderer_stats.postprocess_passes_executed == 2);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.textures_created == 2);
    MK_REQUIRE(rhi_stats.samplers_created == 1);
    MK_REQUIRE(rhi_stats.descriptor_set_layouts_created == 2);
    MK_REQUIRE(rhi_stats.descriptor_sets_allocated == 2);
    MK_REQUIRE(rhi_stats.descriptor_writes == 4);
    MK_REQUIRE(rhi_stats.render_passes_begun == 3);
    MK_REQUIRE(rhi_stats.graphics_pipelines_bound == 3);
    MK_REQUIRE(rhi_stats.descriptor_sets_bound == 2);
    MK_REQUIRE(rhi_stats.resource_transitions == 4);
    MK_REQUIRE(rhi_stats.draw_calls == 3);
    MK_REQUIRE(rhi_stats.present_calls == 1);
}

MK_TEST("rhi postprocess frame renderer can bind scene depth as a postprocess input") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm,
                                                              mirakana::rhi::Format::depth24_stencil8);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess_depth",
        .bytecode_size = 64,
    });

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
        .enable_depth_input = true,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
    });
    MK_REQUIRE(renderer.frame_graph_pass_count() == 2);
    MK_REQUIRE(renderer.postprocess_ready());

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.end_frame();
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.end_frame();
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.frames_started == 3);
    MK_REQUIRE(renderer_stats.frames_finished == 3);
    MK_REQUIRE(renderer_stats.meshes_submitted == 3);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 6);
    MK_REQUIRE(renderer_stats.framegraph_barrier_steps_executed == 13);
    MK_REQUIRE(renderer_stats.postprocess_passes_executed == 3);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.textures_created == 2);
    MK_REQUIRE(rhi_stats.samplers_created == 2);
    MK_REQUIRE(rhi_stats.descriptor_set_layouts_created == 1);
    MK_REQUIRE(rhi_stats.descriptor_sets_allocated == 1);
    MK_REQUIRE(rhi_stats.descriptor_writes == 4);
    MK_REQUIRE(rhi_stats.render_passes_begun == 6);
    MK_REQUIRE(rhi_stats.graphics_pipelines_bound == 6);
    MK_REQUIRE(rhi_stats.descriptor_sets_bound == 3);
    MK_REQUIRE(rhi_stats.resource_transitions == 13);
    MK_REQUIRE(rhi_stats.draw_calls == 6);
    MK_REQUIRE(rhi_stats.present_calls == 3);
}

MK_TEST("rhi postprocess frame renderer records native ui overlay after postprocess") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });
    const auto overlay_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_native_ui_overlay",
        .bytecode_size = 64,
    });
    const auto overlay_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_native_ui_overlay",
        .bytecode_size = 64,
    });

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
        .native_ui_overlay_vertex_shader = overlay_vertex_shader,
        .native_ui_overlay_fragment_shader = overlay_fragment_shader,
        .enable_native_ui_overlay = true,
    });
    MK_REQUIRE(renderer.frame_graph_pass_count() == 2);
    MK_REQUIRE(renderer.postprocess_ready());
    MK_REQUIRE(renderer.native_ui_overlay_ready());

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.draw_sprite(mirakana::SpriteCommand{
        .transform = mirakana::Transform2D{.position = mirakana::Vec2{.x = 16.0F, .y = 12.0F},
                                           .scale = mirakana::Vec2{.x = 24.0F, .y = 10.0F},
                                           .rotation_radians = 0.0F},
        .color = mirakana::Color{.r = 1.0F, .g = 0.2F, .b = 0.1F, .a = 1.0F},
    });
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_draws == 1);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 2);
    MK_REQUIRE(renderer_stats.framegraph_barrier_steps_executed == 2);
    MK_REQUIRE(renderer_stats.postprocess_passes_executed == 1);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.render_passes_begun == 2);
    MK_REQUIRE(rhi_stats.graphics_pipelines_bound == 3);
    MK_REQUIRE(rhi_stats.vertex_buffer_bindings == 1);
    MK_REQUIRE(rhi_stats.buffer_writes == 1);
    MK_REQUIRE(rhi_stats.draw_calls == 3);
    MK_REQUIRE(rhi_stats.present_calls == 1);
}

MK_TEST("rhi postprocess frame renderer binds optional tangent frame mesh vertex streams") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });
    const auto position_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto normal_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto tangent_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto index_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 12, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .mesh_binding =
            mirakana::MeshGpuBinding{
                .vertex_buffer = position_buffer,
                .index_buffer = index_buffer,
                .vertex_count = 3,
                .index_count = 3,
                .vertex_stride = 12,
                .index_format = mirakana::rhi::IndexFormat::uint32,
                .owner_device = &device,
                .normal_vertex_buffer = normal_buffer,
                .tangent_vertex_buffer = tangent_buffer,
                .normal_vertex_stride = 12,
                .tangent_vertex_stride = 12,
            },
    });
    renderer.end_frame();

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.vertex_buffer_bindings == 3);
    MK_REQUIRE(rhi_stats.index_buffer_bindings == 1);
    MK_REQUIRE(rhi_stats.draw_calls == 2);
}

MK_TEST("rhi postprocess frame renderer binds a native ui texture atlas page for textured sprites") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });
    const auto overlay_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_native_ui_overlay",
        .bytecode_size = 64,
    });
    const auto overlay_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_native_ui_overlay",
        .bytecode_size = 64,
    });
    const auto atlas_texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto atlas_sampler = device.create_sampler(mirakana::rhi::SamplerDesc{});
    const auto atlas_page = mirakana::AssetId::from_name("sample/ui/atlas");

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
        .native_ui_overlay_vertex_shader = overlay_vertex_shader,
        .native_ui_overlay_fragment_shader = overlay_fragment_shader,
        .native_ui_overlay_atlas =
            mirakana::NativeUiOverlayAtlasBinding{
                .atlas_page = atlas_page, .texture = atlas_texture, .sampler = atlas_sampler, .owner_device = &device},
        .enable_native_ui_overlay = true,
        .enable_native_ui_overlay_textures = true,
    });
    MK_REQUIRE(renderer.native_ui_overlay_ready());
    MK_REQUIRE(renderer.native_ui_overlay_atlas_ready());

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.draw_sprite(mirakana::SpriteCommand{
        .transform = mirakana::Transform2D{.position = mirakana::Vec2{.x = 16.0F, .y = 12.0F},
                                           .scale = mirakana::Vec2{.x = 24.0F, .y = 10.0F},
                                           .rotation_radians = 0.0F},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .texture =
            mirakana::SpriteTextureRegion{.enabled = true,
                                          .atlas_page = atlas_page,
                                          .uv_rect =
                                              mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}},
    });
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.native_ui_overlay_sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_textured_sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_texture_binds == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_draws == 1);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.descriptor_set_layouts_created == 2);
    MK_REQUIRE(rhi_stats.descriptor_sets_allocated == 2);
    MK_REQUIRE(rhi_stats.descriptor_writes == 4);
    MK_REQUIRE(rhi_stats.descriptor_sets_bound == 2);
}

MK_TEST("rhi directional shadow smoke frame renderer records a shadow receiver smoke pass") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto material_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto shadow_receiver_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::shadow_receiver_depth_texture_binding(),
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::shadow_receiver_sampler_binding(),
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = mirakana::shadow_receiver_constants_binding(),
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto scene_layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, shadow_receiver_layout},
        .push_constant_bytes = 0,
    });
    const auto material_set = device.allocate_descriptor_set(material_layout);
    const auto material_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform,
    });
    device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = material_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                material_buffer)},
    });
    const auto scene_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto scene_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_shadow_receiver",
        .bytecode_size = 64,
    });
    const auto scene_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = scene_layout,
        .vertex_shader = scene_vertex_shader,
        .fragment_shader = scene_fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                            .depth_write_enabled = true,
                                                            .depth_compare = mirakana::rhi::CompareOp::less_equal},
    });
    const auto shadow_layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto shadow_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_shadow",
        .bytecode_size = 64,
    });
    const auto shadow_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_shadow",
        .bytecode_size = 64,
    });
    const auto shadow_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = shadow_layout,
        .vertex_shader = shadow_vertex_shader,
        .fragment_shader = shadow_fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                            .depth_write_enabled = true,
                                                            .depth_compare = mirakana::rhi::CompareOp::less_equal},
    });
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess_depth",
        .bytecode_size = 64,
    });

    mirakana::RhiDirectionalShadowSmokeFrameRenderer renderer(mirakana::RhiDirectionalShadowSmokeFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .scene_skinned_graphics_pipeline = {},
        .scene_pipeline_layout = scene_layout,
        .shadow_graphics_pipeline = shadow_pipeline,
        .shadow_receiver_descriptor_set_layout = shadow_receiver_layout,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
        .scene_depth_format = mirakana::rhi::Format::depth24_stencil8,
        .shadow_depth_format = mirakana::rhi::Format::depth24_stencil8,
        .shadow_depth_atlas_extent = mirakana::Extent2D{.width = 64, .height = 36},
        .directional_shadow_cascade_count = 1,
        .shadow_receiver_constants_initial = make_dummy_packed_shadow_receiver_constants(),
        .shadow_receiver_descriptor_set_index = 1,
    });
    MK_REQUIRE(renderer.frame_graph_pass_count() == 3);
    MK_REQUIRE(renderer.directional_shadow_ready());

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .material_binding =
            mirakana::MaterialGpuBinding{
                .pipeline_layout = scene_layout,
                .descriptor_set = material_set,
                .descriptor_set_index = 0,
                .owner_device = &device,
            },
    });
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.frames_started == 1);
    MK_REQUIRE(renderer_stats.frames_finished == 1);
    MK_REQUIRE(renderer_stats.meshes_submitted == 1);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 3);
    MK_REQUIRE(renderer_stats.framegraph_barrier_steps_executed == 9);
    MK_REQUIRE(renderer_stats.postprocess_passes_executed == 1);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.render_passes_begun == 3);
    MK_REQUIRE(rhi_stats.graphics_pipelines_bound == 3);
    MK_REQUIRE(rhi_stats.descriptor_sets_bound == 3);
    MK_REQUIRE(rhi_stats.draw_calls == 3);
    MK_REQUIRE(rhi_stats.present_calls == 1);
}

MK_TEST("rhi directional shadow smoke frame renderer records morph scene draws") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto material_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                             .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::fragment},
    }});
    const auto morph_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                             .type = mirakana::rhi::DescriptorType::storage_buffer,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::vertex},
        mirakana::rhi::DescriptorBindingDesc{.binding = 1,
                                             .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::vertex},
    }});
    const auto shadow_receiver_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{.binding = mirakana::shadow_receiver_depth_texture_binding(),
                                             .type = mirakana::rhi::DescriptorType::sampled_texture,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::fragment},
        mirakana::rhi::DescriptorBindingDesc{.binding = mirakana::shadow_receiver_sampler_binding(),
                                             .type = mirakana::rhi::DescriptorType::sampler,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::fragment},
        mirakana::rhi::DescriptorBindingDesc{.binding = mirakana::shadow_receiver_constants_binding(),
                                             .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::fragment},
    }});
    const auto scene_layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, morph_layout, shadow_receiver_layout},
        .push_constant_bytes = 0,
    });
    const auto material_set = device.allocate_descriptor_set(material_layout);
    const auto morph_set = device.allocate_descriptor_set(morph_layout);
    const auto material_buffer = device.create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 256, .usage = mirakana::rhi::BufferUsage::uniform});
    device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = material_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                material_buffer)},
    });

    const auto scene_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto scene_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "ps_shadow_receiver", .bytecode_size = 64});
    const auto morph_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_morph", .bytecode_size = 64});
    const auto scene_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = scene_layout,
        .vertex_shader = scene_vertex_shader,
        .fragment_shader = scene_fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                            .depth_write_enabled = true,
                                                            .depth_compare = mirakana::rhi::CompareOp::less_equal},
    });
    const auto morph_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = scene_layout,
        .vertex_shader = morph_vertex_shader,
        .fragment_shader = scene_fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                            .depth_write_enabled = true,
                                                            .depth_compare = mirakana::rhi::CompareOp::less_equal},
    });
    const auto shadow_layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto shadow_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_shadow", .bytecode_size = 64});
    const auto shadow_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "ps_shadow", .bytecode_size = 64});
    const auto shadow_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = shadow_layout,
        .vertex_shader = shadow_vertex_shader,
        .fragment_shader = shadow_fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {},
        .vertex_attributes = {},
        .depth_state = mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                            .depth_write_enabled = true,
                                                            .depth_compare = mirakana::rhi::CompareOp::less_equal},
    });
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_postprocess", .bytecode_size = 64});
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "ps_postprocess_depth", .bytecode_size = 64});
    const auto vertex_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto index_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 12, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});
    const auto morph_delta_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_destination});
    const auto morph_weight_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform | mirakana::rhi::BufferUsage::copy_destination});

    const auto make_renderer_desc = [&]() {
        return mirakana::RhiDirectionalShadowSmokeFrameRendererDesc{
            .device = &device,
            .extent = mirakana::Extent2D{.width = 64, .height = 36},
            .swapchain = swapchain,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .scene_graphics_pipeline = scene_pipeline,
            .scene_morph_graphics_pipeline = morph_pipeline,
            .scene_pipeline_layout = scene_layout,
            .shadow_graphics_pipeline = shadow_pipeline,
            .shadow_receiver_descriptor_set_layout = shadow_receiver_layout,
            .postprocess_vertex_shader = postprocess_vertex_shader,
            .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
            .wait_for_completion = true,
            .scene_depth_format = mirakana::rhi::Format::depth24_stencil8,
            .shadow_depth_format = mirakana::rhi::Format::depth24_stencil8,
            .shadow_depth_atlas_extent = mirakana::Extent2D{.width = 64, .height = 36},
            .directional_shadow_cascade_count = 1,
            .shadow_receiver_constants_initial = make_dummy_packed_shadow_receiver_constants(),
            .shadow_receiver_descriptor_set_index = 2,
        };
    };

    mirakana::RhiDirectionalShadowSmokeFrameRenderer renderer(make_renderer_desc());

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .mesh_binding = mirakana::MeshGpuBinding{.vertex_buffer = vertex_buffer,
                                                 .index_buffer = index_buffer,
                                                 .vertex_count = 3,
                                                 .index_count = 3,
                                                 .vertex_offset = 0,
                                                 .index_offset = 0,
                                                 .vertex_stride = 12,
                                                 .index_format = mirakana::rhi::IndexFormat::uint32,
                                                 .owner_device = &device},
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = scene_layout,
                                                         .descriptor_set = material_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = &device},
        .gpu_morphing = true,
        .morph_mesh =
            mirakana::MorphMeshGpuBinding{
                .position_delta_buffer = morph_delta_buffer,
                .morph_weight_buffer = morph_weight_buffer,
                .morph_descriptor_set = morph_set,
                .vertex_count = 3,
                .target_count = 1,
                .position_delta_bytes = 36,
                .morph_weight_uniform_allocation_bytes = 256,
                .owner_device = &device,
            },
    });
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 3);
    MK_REQUIRE(renderer_stats.gpu_morph_draws == 1);
    MK_REQUIRE(renderer_stats.morph_descriptor_binds == 1);

    bool rejected_missing_material = false;
    try {
        mirakana::RhiDirectionalShadowSmokeFrameRenderer rejecting_renderer(make_renderer_desc());
        rejecting_renderer.begin_frame();
        rejecting_renderer.draw_mesh(mirakana::MeshCommand{
            .mesh_binding = mirakana::MeshGpuBinding{.vertex_buffer = vertex_buffer,
                                                     .index_buffer = index_buffer,
                                                     .vertex_count = 3,
                                                     .index_count = 3,
                                                     .vertex_offset = 0,
                                                     .index_offset = 0,
                                                     .vertex_stride = 12,
                                                     .index_format = mirakana::rhi::IndexFormat::uint32,
                                                     .owner_device = &device},
            .gpu_morphing = true,
            .morph_mesh =
                mirakana::MorphMeshGpuBinding{
                    .position_delta_buffer = morph_delta_buffer,
                    .morph_weight_buffer = morph_weight_buffer,
                    .morph_descriptor_set = morph_set,
                    .vertex_count = 3,
                    .target_count = 1,
                    .position_delta_bytes = 36,
                    .morph_weight_uniform_allocation_bytes = 256,
                    .owner_device = &device,
                },
        });
    } catch (const std::invalid_argument&) {
        rejected_missing_material = true;
    }
    MK_REQUIRE(rejected_missing_material);
}

MK_TEST("rhi directional shadow smoke frame renderer records native ui overlay after postprocess") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    auto desc = create_directional_shadow_smoke_test_desc(device, swapchain);
    desc.native_ui_overlay_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_native_ui_overlay",
        .bytecode_size = 64,
    });
    desc.native_ui_overlay_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_native_ui_overlay",
        .bytecode_size = 64,
    });
    desc.enable_native_ui_overlay = true;

    mirakana::RhiDirectionalShadowSmokeFrameRenderer renderer(desc);
    MK_REQUIRE(renderer.frame_graph_pass_count() == 3);
    MK_REQUIRE(renderer.directional_shadow_ready());
    MK_REQUIRE(renderer.native_ui_overlay_ready());

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.draw_sprite(mirakana::SpriteCommand{
        .transform = mirakana::Transform2D{.position = mirakana::Vec2{.x = 28.0F, .y = 18.0F},
                                           .scale = mirakana::Vec2{.x = 20.0F, .y = 8.0F},
                                           .rotation_radians = 0.0F},
        .color = mirakana::Color{.r = 0.1F, .g = 0.8F, .b = 0.25F, .a = 1.0F},
    });
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_draws == 1);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 3);
    MK_REQUIRE(renderer_stats.framegraph_barrier_steps_executed == 9);
    MK_REQUIRE(renderer_stats.postprocess_passes_executed == 1);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.render_passes_begun == 3);
    MK_REQUIRE(rhi_stats.graphics_pipelines_bound == 4);
    MK_REQUIRE(rhi_stats.vertex_buffer_bindings == 1);
    MK_REQUIRE(rhi_stats.buffer_writes == 2);
    MK_REQUIRE(rhi_stats.draw_calls == 4);
    MK_REQUIRE(rhi_stats.present_calls == 1);
}

MK_TEST("rhi directional shadow smoke frame renderer reports shadow color target prep failure from executor") {
    ThrowingTransitionRhiDevice device;
    device.throw_on_submit = false;
    device.throw_on_transition = 1;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    auto renderer = create_directional_shadow_smoke_test_renderer(device, swapchain);

    bool begin_succeeded = false;
    try {
        renderer->begin_frame();
        begin_succeeded = true;
    } catch (...) {
        begin_succeeded = false;
    }
    MK_REQUIRE(begin_succeeded);

    bool execution_failed = false;
    try {
        renderer->end_frame();
    } catch (const std::runtime_error& ex) {
        execution_failed =
            std::string_view{ex.what()} == "rhi shadow smoke renderer frame graph rhi texture execution failed";
    }

    MK_REQUIRE(execution_failed);
    MK_REQUIRE(!renderer->frame_active());
    MK_REQUIRE(renderer->stats().frames_finished == 0);
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);
}

MK_TEST("rhi directional shadow smoke frame renderer preserves target states when submit fails") {
    ThrowingSubmitRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    auto renderer = create_directional_shadow_smoke_test_renderer(device, swapchain);

    renderer->begin_frame();

    bool submit_failed = false;
    try {
        renderer->end_frame();
    } catch (const std::runtime_error& ex) {
        submit_failed = std::string_view{ex.what()} == "submit failed";
    }

    MK_REQUIRE(submit_failed);
    MK_REQUIRE(!renderer->frame_active());
    MK_REQUIRE(renderer->stats().frames_finished == 0);
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);

    device.throw_on_submit = false;
    renderer->begin_frame();
    renderer->end_frame();

    const auto stats = renderer->stats();
    MK_REQUIRE(stats.frames_finished == 1);
    MK_REQUIRE(stats.framegraph_passes_executed == 3);
    MK_REQUIRE(stats.framegraph_barrier_steps_executed == 9);
}

MK_TEST("rhi directional shadow smoke frame renderer reports frame graph final-state transition failure") {
    ThrowingTransitionRhiDevice device;
    device.throw_on_submit = false;
    device.throw_on_transition = 8;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    auto renderer = create_directional_shadow_smoke_test_renderer(device, swapchain);

    renderer->begin_frame();

    bool execution_failed = false;
    try {
        renderer->end_frame();
    } catch (const std::runtime_error& ex) {
        execution_failed =
            std::string_view{ex.what()} == "rhi shadow smoke renderer frame graph rhi texture execution failed";
    }

    MK_REQUIRE(execution_failed);
    MK_REQUIRE(!renderer->frame_active());
    MK_REQUIRE(renderer->stats().frames_finished == 0);
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);
}

MK_TEST("rhi directional shadow smoke frame renderer rejects inconsistent shadow filter metadata") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });

    auto invalid_taps = create_directional_shadow_smoke_test_desc(device, swapchain);
    invalid_taps.shadow_filter_tap_count = 4;

    bool rejected_taps = false;
    try {
        mirakana::RhiDirectionalShadowSmokeFrameRenderer renderer(invalid_taps);
        static_cast<void>(renderer);
    } catch (const std::invalid_argument&) {
        rejected_taps = true;
    }

    auto invalid_radius = create_directional_shadow_smoke_test_desc(device, swapchain);
    invalid_radius.shadow_filter_radius_texels = 2.0F;

    bool rejected_radius = false;
    try {
        mirakana::RhiDirectionalShadowSmokeFrameRenderer renderer(invalid_radius);
        static_cast<void>(renderer);
    } catch (const std::invalid_argument&) {
        rejected_radius = true;
    }

    MK_REQUIRE(rejected_taps);
    MK_REQUIRE(rejected_radius);
}

MK_TEST("rhi directional shadow smoke frame renderer releases acquired swapchain frame on destruction") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });

    {
        auto renderer = create_directional_shadow_smoke_test_renderer(device, swapchain);
        renderer->begin_frame();
        MK_REQUIRE(renderer->frame_active());
    }

    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);

    device.resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 80, .height = 45});
    const auto frame = device.acquire_swapchain_frame(swapchain);
    MK_REQUIRE(frame.value != 0);
    device.release_swapchain_frame(frame);
}

MK_TEST("rhi postprocess frame renderer releases acquired swapchain frame on destruction") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });

    {
        mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
            .device = &device,
            .extent = mirakana::Extent2D{.width = 64, .height = 36},
            .swapchain = swapchain,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .scene_graphics_pipeline = scene_pipeline,
            .postprocess_vertex_shader = postprocess_vertex_shader,
            .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
            .wait_for_completion = true,
        });
        renderer.begin_frame();
        MK_REQUIRE(renderer.frame_active());
    }

    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);

    device.resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 80, .height = 45});
    const auto frame = device.acquire_swapchain_frame(swapchain);
    MK_REQUIRE(frame.value != 0);
    device.release_swapchain_frame(frame);
}

MK_TEST("rhi postprocess frame renderer rejects unsupported depth input formats") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm,
                                                              mirakana::rhi::Format::depth24_stencil8);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess_depth",
        .bytecode_size = 64,
    });

    bool rejected = false;
    try {
        mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
            .device = &device,
            .extent = mirakana::Extent2D{.width = 64, .height = 36},
            .swapchain = swapchain,
            .color_format = mirakana::rhi::Format::bgra8_unorm,
            .scene_graphics_pipeline = scene_pipeline,
            .postprocess_vertex_shader = postprocess_vertex_shader,
            .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
            .wait_for_completion = true,
            .enable_depth_input = true,
            .depth_format = mirakana::rhi::Format::rgba8_unorm,
        });
        static_cast<void>(renderer);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }

    MK_REQUIRE(rejected);
}

MK_TEST("rhi postprocess frame renderer releases pre-present swapchain frame when scene callback fails") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = mirakana::rhi::GraphicsPipelineHandle{999},
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
    });

    renderer.begin_frame();

    bool execution_failed = false;
    try {
        renderer.end_frame();
    } catch (const std::runtime_error& ex) {
        execution_failed =
            std::string_view{ex.what()} == "rhi postprocess renderer frame graph rhi texture execution failed";
    }

    MK_REQUIRE(execution_failed);
    MK_REQUIRE(!renderer.frame_active());
    MK_REQUIRE(renderer.stats().frames_finished == 0);
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);

    device.resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 80, .height = 45});
    const auto frame = device.acquire_swapchain_frame(swapchain);
    MK_REQUIRE(frame.value != 0);
    device.release_swapchain_frame(frame);
}

MK_TEST("rhi postprocess frame renderer releases pre-present swapchain frame when scene target prep fails") {
    ThrowingTransitionRhiDevice device;
    device.throw_on_submit = false;
    device.throw_on_transition = 1;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
    });

    renderer.begin_frame();

    bool execution_failed = false;
    try {
        renderer.end_frame();
    } catch (const std::runtime_error&) {
        execution_failed = true;
    }

    MK_REQUIRE(execution_failed);
    MK_REQUIRE(!renderer.frame_active());
    MK_REQUIRE(renderer.stats().frames_finished == 0);
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);

    device.resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 80, .height = 45});
    const auto frame = device.acquire_swapchain_frame(swapchain);
    MK_REQUIRE(frame.value != 0);
    device.release_swapchain_frame(frame);
}

MK_TEST("rhi postprocess frame renderer releases pre-present swapchain frame when postprocess depth restore fails") {
    ThrowingTransitionRhiDevice device;
    device.throw_on_submit = false;
    device.throw_on_transition = 5;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto scene_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm,
                                                              mirakana::rhi::Format::depth24_stencil8);
    const auto postprocess_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_postprocess",
        .bytecode_size = 64,
    });
    const auto postprocess_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_postprocess",
        .bytecode_size = 64,
    });

    mirakana::RhiPostprocessFrameRenderer renderer(mirakana::RhiPostprocessFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .swapchain = swapchain,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .scene_graphics_pipeline = scene_pipeline,
        .postprocess_vertex_shader = postprocess_vertex_shader,
        .postprocess_fragment_stages = std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
        .wait_for_completion = true,
        .enable_depth_input = true,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
    });

    renderer.begin_frame();

    bool transition_failed = false;
    try {
        renderer.end_frame();
    } catch (const std::runtime_error&) {
        transition_failed = true;
    }

    MK_REQUIRE(transition_failed);
    MK_REQUIRE(!renderer.frame_active());
    MK_REQUIRE(renderer.stats().frames_finished == 0);
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);

    device.resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 80, .height = 45});
    const auto frame = device.acquire_swapchain_frame(swapchain);
    MK_REQUIRE(frame.value != 0);
    device.release_swapchain_frame(frame);
}

MK_TEST("rhi frame renderer submits a texture frame through an rhi device") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0, .stride = 32, .input_rate = mirakana::rhi::VertexInputRate::vertex}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
            .semantic_index = 0,
        }},
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    MK_REQUIRE(renderer.backend_name() == "null");
    renderer.begin_frame();
    renderer.draw_sprite(mirakana::SpriteCommand{.transform = mirakana::Transform2D{},
                                                 .color = mirakana::Color{.r = 1.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F}});
    renderer.draw_mesh(mirakana::MeshCommand{.transform = mirakana::Transform3D{},
                                             .color = mirakana::Color{.r = 0.0F, .g = 1.0F, .b = 0.0F, .a = 1.0F}});
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.frames_started == 1);
    MK_REQUIRE(renderer_stats.frames_finished == 1);
    MK_REQUIRE(renderer_stats.sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.meshes_submitted == 1);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 1);
    MK_REQUIRE(renderer_stats.framegraph_barrier_steps_executed == 0);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.command_lists_begun == 1);
    MK_REQUIRE(rhi_stats.render_passes_begun == 1);
    MK_REQUIRE(rhi_stats.graphics_pipelines_bound == 1);
    MK_REQUIRE(rhi_stats.draw_calls == 2);
    MK_REQUIRE(rhi_stats.vertices_submitted == 6);
    MK_REQUIRE(rhi_stats.command_lists_submitted == 1);
    MK_REQUIRE(rhi_stats.fences_signaled == 1);
}

MK_TEST("rhi frame renderer records native textured 2d sprites through an overlay pass") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto native_sprite_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_native_sprite_overlay",
        .bytecode_size = 64,
    });
    const auto native_sprite_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_native_sprite_overlay",
        .bytecode_size = 64,
    });
    const auto atlas_texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto atlas_sampler = device.create_sampler(mirakana::rhi::SamplerDesc{});
    const auto atlas_page = mirakana::AssetId::from_name("sample/2d/player-sprite");
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
        .native_sprite_overlay_color_format = mirakana::rhi::Format::rgba8_unorm,
        .native_sprite_overlay_vertex_shader = native_sprite_vertex_shader,
        .native_sprite_overlay_fragment_shader = native_sprite_fragment_shader,
        .native_sprite_overlay_atlas =
            mirakana::NativeUiOverlayAtlasBinding{
                .atlas_page = atlas_page, .texture = atlas_texture, .sampler = atlas_sampler, .owner_device = &device},
        .enable_native_sprite_overlay = true,
        .enable_native_sprite_overlay_textures = true,
    });

    renderer.begin_frame();
    renderer.draw_sprite(mirakana::SpriteCommand{
        .transform = mirakana::Transform2D{.position = mirakana::Vec2{.x = 32.0F, .y = 24.0F},
                                           .scale = mirakana::Vec2{.x = 16.0F, .y = 16.0F},
                                           .rotation_radians = 0.0F},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .texture =
            mirakana::SpriteTextureRegion{.enabled = true,
                                          .atlas_page = atlas_page,
                                          .uv_rect =
                                              mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}},
    });
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_textured_sprites_submitted == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_texture_binds == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_draws == 1);
    MK_REQUIRE(renderer_stats.native_ui_overlay_textured_draws == 1);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 1);
    MK_REQUIRE(renderer_stats.framegraph_barrier_steps_executed == 0);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.render_passes_begun == 1);
    MK_REQUIRE(rhi_stats.descriptor_sets_bound == 1);
    MK_REQUIRE(rhi_stats.vertex_buffer_bindings == 1);
    MK_REQUIRE(rhi_stats.vertices_submitted == 6);
}

MK_TEST("rhi frame renderer reports native sprite batch execution counters from adjacent runs") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto native_sprite_vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_native_sprite_overlay",
        .bytecode_size = 64,
    });
    const auto native_sprite_fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_native_sprite_overlay",
        .bytecode_size = 64,
    });
    const auto atlas_texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto atlas_sampler = device.create_sampler(mirakana::rhi::SamplerDesc{});
    const auto atlas_page = mirakana::AssetId::from_name("sample/2d/player-sprite");
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
        .native_sprite_overlay_color_format = mirakana::rhi::Format::rgba8_unorm,
        .native_sprite_overlay_vertex_shader = native_sprite_vertex_shader,
        .native_sprite_overlay_fragment_shader = native_sprite_fragment_shader,
        .native_sprite_overlay_atlas =
            mirakana::NativeUiOverlayAtlasBinding{
                .atlas_page = atlas_page, .texture = atlas_texture, .sampler = atlas_sampler, .owner_device = &device},
        .enable_native_sprite_overlay = true,
        .enable_native_sprite_overlay_textures = true,
    });

    const auto textured_sprite = [&](float x) {
        return mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{.position = mirakana::Vec2{.x = x, .y = 24.0F},
                                               .scale = mirakana::Vec2{.x = 8.0F, .y = 8.0F},
                                               .rotation_radians = 0.0F},
            .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
            .texture =
                mirakana::SpriteTextureRegion{
                    .enabled = true,
                    .atlas_page = atlas_page,
                    .uv_rect = mirakana::SpriteUvRect{.u0 = 0.0F, .v0 = 0.0F, .u1 = 1.0F, .v1 = 1.0F}},
        };
    };
    const auto solid_sprite = [](float x) {
        return mirakana::SpriteCommand{
            .transform = mirakana::Transform2D{.position = mirakana::Vec2{.x = x, .y = 40.0F},
                                               .scale = mirakana::Vec2{.x = 8.0F, .y = 8.0F},
                                               .rotation_radians = 0.0F},
            .color = mirakana::Color{.r = 0.2F, .g = 0.4F, .b = 1.0F, .a = 1.0F},
        };
    };

    renderer.begin_frame();
    renderer.draw_sprite(textured_sprite(12.0F));
    renderer.draw_sprite(textured_sprite(24.0F));
    renderer.draw_sprite(solid_sprite(36.0F));
    renderer.draw_sprite(textured_sprite(48.0F));
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.native_sprite_batches_executed == 3);
    MK_REQUIRE(renderer_stats.native_sprite_batch_sprites_executed == 4);
    MK_REQUIRE(renderer_stats.native_sprite_batch_textured_sprites_executed == 3);
    MK_REQUIRE(renderer_stats.native_sprite_batch_texture_binds == 2);
    MK_REQUIRE(renderer_stats.native_ui_overlay_draws == 3);
    MK_REQUIRE(renderer_stats.native_ui_overlay_textured_draws == 2);

    const auto rhi_stats = device.stats();
    MK_REQUIRE(rhi_stats.draw_calls == 3);
    MK_REQUIRE(rhi_stats.vertex_buffer_bindings == 3);
    MK_REQUIRE(rhi_stats.vertices_submitted == 24);
}

MK_TEST("rhi frame renderer forwards an optional depth attachment") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto depth = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    mirakana::rhi::GraphicsPipelineDesc pipeline_desc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto pipeline = device.create_graphics_pipeline(pipeline_desc);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
        .depth_texture = depth,
    });

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});

    bool rejected_depth_replace_during_frame = false;
    try {
        renderer.replace_depth_texture(mirakana::rhi::TextureHandle{});
    } catch (const std::logic_error&) {
        rejected_depth_replace_during_frame = true;
    }

    renderer.end_frame();

    renderer.replace_depth_texture(mirakana::rhi::TextureHandle{});
    MK_REQUIRE(renderer.stats().frames_finished == 1);
    MK_REQUIRE(rejected_depth_replace_during_frame);
    MK_REQUIRE(device.stats().render_passes_begun == 1);
    MK_REQUIRE(device.stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(device.stats().draw_calls == 1);
}

MK_TEST("rhi frame renderer replaces host owned depth texture after swapchain resize") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto depth = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 36, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto resized_depth = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 128, .height = 72, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    mirakana::rhi::GraphicsPipelineDesc pipeline_desc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto pipeline = device.create_graphics_pipeline(pipeline_desc);

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .color_texture = mirakana::rhi::TextureHandle{},
        .swapchain = swapchain,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
        .depth_texture = depth,
    });

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.end_frame();

    renderer.resize(mirakana::Extent2D{.width = 128, .height = 72});

    bool rejected_stale_depth = false;
    bool stale_depth_message_preserved = false;
    try {
        renderer.begin_frame();
        renderer.draw_mesh(mirakana::MeshCommand{});
        renderer.end_frame();
    } catch (const std::runtime_error& ex) {
        rejected_stale_depth = true;
        stale_depth_message_preserved =
            std::string_view{ex.what()}.find("rhi render pass depth attachment extent must match the color target") !=
            std::string_view::npos;
    }

    renderer.replace_depth_texture(resized_depth);
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.end_frame();

    MK_REQUIRE(rejected_stale_depth);
    MK_REQUIRE(stale_depth_message_preserved);
    MK_REQUIRE(renderer.stats().frames_finished == 2);
    MK_REQUIRE(device.stats().swapchain_resizes == 1);
    MK_REQUIRE(device.stats().render_passes_begun == 2);
    MK_REQUIRE(device.stats().present_calls == 2);
}

MK_TEST("rhi frame renderer enforces frame lifecycle") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    bool rejected_draw_before_begin = false;
    try {
        renderer.draw_sprite(mirakana::SpriteCommand{});
    } catch (const std::logic_error&) {
        rejected_draw_before_begin = true;
    }

    renderer.begin_frame();

    bool rejected_double_begin = false;
    try {
        renderer.begin_frame();
    } catch (const std::logic_error&) {
        rejected_double_begin = true;
    }

    renderer.end_frame();

    bool rejected_double_end = false;
    try {
        renderer.end_frame();
    } catch (const std::logic_error&) {
        rejected_double_end = true;
    }

    MK_REQUIRE(rejected_draw_before_begin);
    MK_REQUIRE(rejected_double_begin);
    MK_REQUIRE(rejected_double_end);
}

MK_TEST("rhi frame renderer clears active swapchain frame when end frame submit fails") {
    ThrowingSubmitRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 32, .height = 32},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = mirakana::rhi::TextureHandle{},
        .swapchain = swapchain,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();

    bool submit_failed = false;
    try {
        renderer.end_frame();
    } catch (const std::runtime_error&) {
        submit_failed = true;
    }

    MK_REQUIRE(submit_failed);
    MK_REQUIRE(!renderer.frame_active());
    MK_REQUIRE(renderer.stats().frames_finished == 0);
    MK_REQUIRE(renderer.stats().framegraph_passes_executed == 0);

    device.throw_on_submit = false;
    renderer.begin_frame();
    renderer.end_frame();

    MK_REQUIRE(!renderer.frame_active());
    MK_REQUIRE(renderer.stats().frames_finished == 1);
    MK_REQUIRE(renderer.stats().framegraph_passes_executed == 1);
}

MK_TEST("rhi frame renderer accepts recreated graphics pipelines between frames") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto first_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    const auto reloaded_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = first_pipeline,
        .wait_for_completion = true,
    });

    renderer.replace_graphics_pipeline(reloaded_pipeline);
    renderer.begin_frame();

    bool rejected_active_replace = false;
    try {
        renderer.replace_graphics_pipeline(first_pipeline);
    } catch (const std::logic_error&) {
        rejected_active_replace = true;
    }

    renderer.draw_mesh(mirakana::MeshCommand{});
    renderer.end_frame();

    MK_REQUIRE(rejected_active_replace);
    MK_REQUIRE(device.stats().graphics_pipelines_created == 2);
    MK_REQUIRE(device.stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(device.stats().draw_calls == 1);
}

MK_TEST("rhi frame renderer binds mesh buffers and material descriptor sets for mesh commands") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertices =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex});
    const auto indices =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 12, .usage = mirakana::rhi::BufferUsage::index});
    const auto set_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{
        {mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        }},
    });
    const auto descriptor_set = device.allocate_descriptor_set(set_layout);
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {set_layout}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mirakana::AssetId::from_name("meshes/triangle"),
        .material = mirakana::AssetId::from_name("materials/triangle"),
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mirakana::MeshGpuBinding{.vertex_buffer = vertices,
                                                 .index_buffer = indices,
                                                 .vertex_count = 3,
                                                 .index_count = 3,
                                                 .vertex_offset = 0,
                                                 .index_offset = 0,
                                                 .vertex_stride = 12,
                                                 .index_format = mirakana::rhi::IndexFormat::uint32,
                                                 .owner_device = &device},
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = layout,
                                                         .descriptor_set = descriptor_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = &device},
    });
    renderer.end_frame();

    const auto stats = device.stats();
    MK_REQUIRE(stats.descriptor_sets_bound == 1);
    MK_REQUIRE(stats.vertex_buffer_bindings == 1);
    MK_REQUIRE(stats.index_buffer_bindings == 1);
    MK_REQUIRE(stats.indexed_draw_calls == 1);
    MK_REQUIRE(stats.draw_calls == 1);
    MK_REQUIRE(stats.indices_submitted == 3);
}

MK_TEST("rhi frame renderer rejects uploaded mesh buffers from another rhi device") {
    mirakana::rhi::NullRhiDevice upload_device;
    mirakana::rhi::NullRhiDevice render_device;
    const mirakana::runtime::RuntimeMeshPayload payload{
        .asset = mirakana::AssetId::from_name("meshes/uploaded_elsewhere"),
        .handle = mirakana::runtime::RuntimeAssetHandle{9},
        .vertex_count = 3,
        .index_count = 3,
        .has_normals = false,
        .has_uvs = false,
        .has_tangent_frame = false,
        .vertex_bytes = std::vector<std::uint8_t>(36, std::uint8_t{0x40}),
        .index_bytes =
            std::vector<std::uint8_t>{0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00},
    };
    const auto upload = mirakana::runtime_rhi::upload_runtime_mesh(upload_device, payload);
    MK_REQUIRE(upload.succeeded());

    const auto target = render_device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = render_device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = render_device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = render_device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = render_device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &render_device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    bool rejected_cross_device_mesh = false;
    try {
        renderer.draw_mesh(mirakana::MeshCommand{
            .transform = mirakana::Transform3D{},
            .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
            .mesh = payload.asset,
            .material = mirakana::AssetId::from_name("materials/default"),
            .world_from_node = mirakana::Mat4::identity(),
            .mesh_binding = mirakana::runtime_rhi::make_runtime_mesh_gpu_binding(upload),
            .material_binding = mirakana::MaterialGpuBinding{},
        });
    } catch (const std::invalid_argument&) {
        rejected_cross_device_mesh = true;
    }
    renderer.end_frame();

    MK_REQUIRE(rejected_cross_device_mesh);
    MK_REQUIRE(render_device.stats().vertex_buffer_bindings == 0);
    MK_REQUIRE(render_device.stats().indexed_draw_calls == 0);
}

MK_TEST("rhi frame renderer binds runtime material texture descriptors for mesh commands") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1, .height = 1, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto texture_asset = mirakana::AssetId::from_name("textures/base_color");
    const mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/runtime_bound"),
        .name = "RuntimeBound",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_asset}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);
    const auto binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{
            .slot = mirakana::MaterialTextureSlot::base_color, .texture = texture, .owner_device = &device}});
    MK_REQUIRE(binding.succeeded());

    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {binding.descriptor_set_layout}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .transform = mirakana::Transform3D{},
        .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
        .mesh = mirakana::AssetId::from_name("meshes/procedural_triangle"),
        .material = material.id,
        .world_from_node = mirakana::Mat4::identity(),
        .mesh_binding = mirakana::MeshGpuBinding{},
        .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = layout,
                                                         .descriptor_set = binding.descriptor_set,
                                                         .descriptor_set_index = 0,
                                                         .owner_device = &device},
    });
    renderer.end_frame();

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(renderer_stats.meshes_submitted == 1);

    const auto stats = device.stats();
    MK_REQUIRE(stats.samplers_created == 1);
    MK_REQUIRE(stats.descriptor_writes == 4);
    MK_REQUIRE(stats.descriptor_sets_bound == 1);
    MK_REQUIRE(stats.draw_calls == 1);
}

MK_TEST("rhi frame renderer rejects material descriptor sets from another rhi device") {
    mirakana::rhi::NullRhiDevice material_device;
    mirakana::rhi::NullRhiDevice render_device;
    const auto material_set_layout =
        material_device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{
            {mirakana::rhi::DescriptorBindingDesc{
                .binding = 0,
                .type = mirakana::rhi::DescriptorType::uniform_buffer,
                .count = 1,
                .stages = mirakana::rhi::ShaderStageVisibility::fragment,
            }},
        });
    const auto material_descriptor_set = material_device.allocate_descriptor_set(material_set_layout);

    const auto render_set_layout = render_device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{
        {mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        }},
    });
    const auto target = render_device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = render_device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = render_device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = render_device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {render_set_layout}, .push_constant_bytes = 0});
    const auto pipeline = render_device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &render_device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    bool rejected_cross_device_material = false;
    try {
        renderer.draw_mesh(mirakana::MeshCommand{
            .transform = mirakana::Transform3D{},
            .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
            .mesh = mirakana::AssetId::from_name("meshes/procedural"),
            .material = mirakana::AssetId::from_name("materials/cross_device"),
            .world_from_node = mirakana::Mat4::identity(),
            .mesh_binding = mirakana::MeshGpuBinding{},
            .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = layout,
                                                             .descriptor_set = material_descriptor_set,
                                                             .descriptor_set_index = 0,
                                                             .owner_device = &material_device},
        });
    } catch (const std::invalid_argument&) {
        rejected_cross_device_material = true;
    }
    renderer.end_frame();

    MK_REQUIRE(rejected_cross_device_material);
    MK_REQUIRE(render_device.stats().descriptor_sets_bound == 0);
    MK_REQUIRE(render_device.stats().draw_calls == 0);
}

MK_TEST("rhi frame renderer releases acquired swapchain frames when primary pass execution fails") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 32, .height = 32},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = mirakana::rhi::TextureHandle{},
        .swapchain = swapchain,
        .graphics_pipeline = mirakana::rhi::GraphicsPipelineHandle{999},
        .wait_for_completion = true,
    });

    bool rejected_frame = false;
    bool failure_message_preserved = false;
    try {
        renderer.begin_frame();
        renderer.draw_mesh(mirakana::MeshCommand{});
        renderer.end_frame();
    } catch (const std::runtime_error& ex) {
        rejected_frame = true;
        failure_message_preserved =
            std::string_view{ex.what()}.find("rhi graphics pipeline handle must belong to this device") !=
            std::string_view::npos;
    }

    MK_REQUIRE(rejected_frame);
    MK_REQUIRE(failure_message_preserved);
    MK_REQUIRE(!renderer.frame_active());
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);
    MK_REQUIRE(renderer.stats().frames_finished == 0);
    MK_REQUIRE(renderer.stats().framegraph_passes_executed == 0);

    device.resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 40, .height = 40});
    MK_REQUIRE(device.stats().swapchain_resizes == 1);

    const auto frame = device.acquire_swapchain_frame(swapchain);
    MK_REQUIRE(frame.value != 0);
    device.release_swapchain_frame(frame);
}

MK_TEST("rhi frame renderer records gpu morph counters only after primary pass success") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto primary_pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::rgba8_unorm);
    const auto material_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{});
    const auto morph_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::storage_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex,
        },
    }});
    const auto material_set = device.allocate_descriptor_set(material_layout);
    const auto morph_set = device.allocate_descriptor_set(morph_layout);
    const auto morph_pipeline_layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_layout, morph_layout},
        .push_constant_bytes = 0,
    });
    const auto vertex_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto index_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 12, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});
    const auto morph_delta_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::storage | mirakana::rhi::BufferUsage::copy_destination});
    const auto morph_weight_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform | mirakana::rhi::BufferUsage::copy_destination});

    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 32},
        .color_texture = target,
        .swapchain = mirakana::rhi::SwapchainHandle{},
        .graphics_pipeline = primary_pipeline,
        .wait_for_completion = true,
        .morph_graphics_pipeline = mirakana::rhi::GraphicsPipelineHandle{999},
    });

    bool rejected_frame = false;
    try {
        renderer.begin_frame();
        renderer.draw_mesh(mirakana::MeshCommand{
            .mesh_binding = mirakana::MeshGpuBinding{.vertex_buffer = vertex_buffer,
                                                     .index_buffer = index_buffer,
                                                     .vertex_count = 3,
                                                     .index_count = 3,
                                                     .vertex_offset = 0,
                                                     .index_offset = 0,
                                                     .vertex_stride = 12,
                                                     .index_format = mirakana::rhi::IndexFormat::uint32,
                                                     .owner_device = &device},
            .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = morph_pipeline_layout,
                                                             .descriptor_set = material_set,
                                                             .descriptor_set_index = 0,
                                                             .owner_device = &device},
            .gpu_morphing = true,
            .morph_mesh =
                mirakana::MorphMeshGpuBinding{
                    .position_delta_buffer = morph_delta_buffer,
                    .morph_weight_buffer = morph_weight_buffer,
                    .morph_descriptor_set = morph_set,
                    .vertex_count = 3,
                    .target_count = 1,
                    .position_delta_bytes = 36,
                    .morph_weight_uniform_allocation_bytes = 256,
                    .owner_device = &device,
                },
        });
        renderer.end_frame();
    } catch (const std::runtime_error&) {
        rejected_frame = true;
    }

    const auto renderer_stats = renderer.stats();
    MK_REQUIRE(rejected_frame);
    MK_REQUIRE(renderer_stats.meshes_submitted == 1);
    MK_REQUIRE(renderer_stats.frames_finished == 0);
    MK_REQUIRE(renderer_stats.gpu_morph_draws == 0);
    MK_REQUIRE(renderer_stats.morph_descriptor_binds == 0);
    MK_REQUIRE(renderer_stats.framegraph_passes_executed == 0);
}

MK_TEST("rhi frame renderer releases acquired swapchain frame on destruction") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 32, .height = 32},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto pipeline = create_renderer_test_pipeline(device, mirakana::rhi::Format::bgra8_unorm);

    {
        mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
            .device = &device,
            .extent = mirakana::Extent2D{.width = 32, .height = 32},
            .color_texture = mirakana::rhi::TextureHandle{},
            .swapchain = swapchain,
            .graphics_pipeline = pipeline,
            .wait_for_completion = true,
        });
        renderer.begin_frame();
        MK_REQUIRE(renderer.frame_active());
    }

    MK_REQUIRE(device.stats().swapchain_frames_acquired == 1);
    MK_REQUIRE(device.stats().swapchain_frames_released == 1);

    device.resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 40, .height = 40});
    const auto frame = device.acquire_swapchain_frame(swapchain);
    MK_REQUIRE(frame.value != 0);
    device.release_swapchain_frame(frame);
}

MK_TEST("rhi frame renderer resizes its swapchain color target") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .color_texture = mirakana::rhi::TextureHandle{},
        .swapchain = swapchain,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.resize(mirakana::Extent2D{.width = 128, .height = 72});

    MK_REQUIRE(renderer.backbuffer_extent().width == 128);
    MK_REQUIRE(renderer.backbuffer_extent().height == 72);
    MK_REQUIRE(device.stats().swapchain_resizes == 1);

    renderer.begin_frame();
    renderer.draw_sprite(mirakana::SpriteCommand{.transform = mirakana::Transform2D{},
                                                 .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F}});
    renderer.end_frame();
    MK_REQUIRE(renderer.stats().frames_finished == 1);
    MK_REQUIRE(device.stats().present_calls == 1);
}

MK_TEST("rhi frame renderer rejects resize during an active swapchain frame") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 64, .height = 36},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 36},
        .color_texture = mirakana::rhi::TextureHandle{},
        .swapchain = swapchain,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });

    renderer.begin_frame();
    bool rejected_resize = false;
    bool leaked_resize_to_rhi = false;
    try {
        renderer.resize(mirakana::Extent2D{.width = 128, .height = 72});
    } catch (const std::invalid_argument&) {
        leaked_resize_to_rhi = true;
    } catch (const std::logic_error&) {
        rejected_resize = true;
    }
    renderer.end_frame();

    MK_REQUIRE(rejected_resize);
    MK_REQUIRE(!leaked_resize_to_rhi);
    MK_REQUIRE(renderer.backbuffer_extent().width == 64);
    MK_REQUIRE(renderer.backbuffer_extent().height == 36);
    MK_REQUIRE(device.stats().swapchain_resizes == 0);
}

MK_TEST("rhi viewport surface owns a renderable sampled texture target") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
    });

    MK_REQUIRE(surface.backend_name() == "null");
    MK_REQUIRE(surface.extent().width == 320);
    MK_REQUIRE(surface.color_format() == mirakana::rhi::Format::rgba8_unorm);
    MK_REQUIRE(surface.color_texture().value != 0);
    MK_REQUIRE(surface.frames_rendered() == 0);

    surface.render_clear_frame();

    MK_REQUIRE(surface.frames_rendered() == 1);
    MK_REQUIRE(device.stats().textures_created == 1);
    MK_REQUIRE(device.stats().command_lists_begun == 1);
    MK_REQUIRE(device.stats().render_passes_begun == 1);
    MK_REQUIRE(device.stats().command_lists_submitted == 1);
    MK_REQUIRE(device.stats().fences_signaled == 1);
}

MK_TEST("rhi viewport surface reports frame graph color transition failure") {
    ThrowingTransitionRhiDevice device;
    device.throw_on_submit = false;
    device.throw_on_transition = 1;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 18},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
    });

    bool execution_failed = false;
    try {
        surface.render_clear_frame();
    } catch (const std::runtime_error& ex) {
        execution_failed =
            std::string_view{ex.what()} == "rhi viewport surface frame graph color state execution failed";
    }

    MK_REQUIRE(execution_failed);
    MK_REQUIRE(surface.frames_rendered() == 0);
}

MK_TEST("rhi viewport surface recovers after final frame graph color transition failure") {
    ThrowingTransitionRhiDevice device;
    device.throw_on_submit = false;
    device.throw_on_transition = 2;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 18},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
    });

    bool execution_failed = false;
    try {
        surface.render_clear_frame();
    } catch (const std::runtime_error& ex) {
        execution_failed =
            std::string_view{ex.what()} == "rhi viewport surface frame graph color state execution failed";
    }

    MK_REQUIRE(execution_failed);
    MK_REQUIRE(surface.frames_rendered() == 0);

    surface.render_clear_frame();

    MK_REQUIRE(surface.frames_rendered() == 1);
}

MK_TEST("rhi viewport surface preserves recorded color state after submit failure") {
    ThrowingSubmitRhiDevice device;
    device.throw_on_submit = true;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 32, .height = 18},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
    });

    bool submit_failed = false;
    try {
        (void)surface.prepare_display_frame();
    } catch (const std::runtime_error& ex) {
        submit_failed = std::string_view{ex.what()} == "submit failed";
    }

    MK_REQUIRE(submit_failed);
    MK_REQUIRE(surface.frames_rendered() == 0);

    device.throw_on_submit = false;
    const auto display = surface.prepare_display_frame();

    MK_REQUIRE(display.texture.value == surface.color_texture().value);
    MK_REQUIRE(display.frame_index == 0);
}

MK_TEST("rhi viewport surface reads back rendered color target") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 17, .height = 9},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
    });

    surface.render_clear_frame();
    const auto frame = surface.readback_color_frame();

    MK_REQUIRE(frame.extent.width == 17);
    MK_REQUIRE(frame.extent.height == 9);
    MK_REQUIRE(frame.format == mirakana::rhi::Format::rgba8_unorm);
    MK_REQUIRE(frame.frame_index == 1);
    MK_REQUIRE(frame.row_pitch >= 17U * 4U);
    MK_REQUIRE(frame.row_pitch % 256U == 0);
    MK_REQUIRE(frame.pixels.size() == static_cast<std::size_t>(frame.row_pitch) * frame.extent.height);
    MK_REQUIRE(device.stats().buffers_created == 1);
    MK_REQUIRE(device.stats().texture_buffer_copies == 1);
    MK_REQUIRE(device.stats().buffer_reads == 1);

    const auto second_frame = surface.readback_color_frame();

    MK_REQUIRE(second_frame.pixels.size() == frame.pixels.size());
    MK_REQUIRE(device.stats().buffers_created == 1);
    MK_REQUIRE(device.stats().texture_buffer_copies == 2);
    MK_REQUIRE(device.stats().buffer_reads == 2);
}

MK_TEST("rhi viewport surface prepares native display frame without CPU readback") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 19, .height = 11},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
        .allow_native_display_interop = true,
    });

    surface.render_clear_frame();
    const auto display = surface.prepare_display_frame();

    MK_REQUIRE(display.extent.width == 19);
    MK_REQUIRE(display.extent.height == 11);
    MK_REQUIRE(display.format == mirakana::rhi::Format::rgba8_unorm);
    MK_REQUIRE(display.texture.value == surface.color_texture().value);
    MK_REQUIRE(display.frame_index == 1);
    MK_REQUIRE(device.stats().texture_buffer_copies == 0);
    MK_REQUIRE(device.stats().buffer_reads == 0);
    MK_REQUIRE(device.stats().resource_transitions == 3);
}

MK_TEST("rhi viewport surface readback transitions after native display preparation") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 13, .height = 7},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
        .allow_native_display_interop = true,
    });

    surface.render_clear_frame();
    (void)surface.prepare_display_frame();
    const auto frame = surface.readback_color_frame();

    MK_REQUIRE(frame.extent.width == 13);
    MK_REQUIRE(frame.frame_index == 1);
    MK_REQUIRE(device.stats().texture_buffer_copies == 1);
    MK_REQUIRE(device.stats().buffer_reads == 1);
    MK_REQUIRE(device.stats().resource_transitions == 4);
}

MK_TEST("rhi viewport surface renders submitted renderer commands before readback") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 64,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    bool submitted = false;
    surface.render_frame(
        mirakana::RhiViewportRenderDesc{.graphics_pipeline = pipeline,
                                        .clear_color = mirakana::Color{.r = 0.04F, .g = 0.05F, .b = 0.07F, .a = 1.0F}},
        [&submitted](mirakana::IRenderer& renderer) {
            renderer.draw_mesh(
                mirakana::MeshCommand{.transform = mirakana::Transform3D{},
                                      .color = mirakana::Color{.r = 0.2F, .g = 0.8F, .b = 0.4F, .a = 1.0F}});
            submitted = true;
        });

    const auto frame = surface.readback_color_frame();

    MK_REQUIRE(submitted);
    MK_REQUIRE(surface.frames_rendered() == 1);
    MK_REQUIRE(frame.frame_index == 1);
    MK_REQUIRE(device.stats().render_passes_begun == 1);
    MK_REQUIRE(device.stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(device.stats().draw_calls == 1);
    MK_REQUIRE(device.stats().texture_buffer_copies == 1);
}

MK_TEST("rhi viewport surface renders runtime material descriptor bindings") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 64, .height = 64},
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .wait_for_completion = true,
    });

    const auto texture_asset = mirakana::AssetId::from_name("textures/preview_base_color");
    const mirakana::runtime::RuntimeTexturePayload texture_payload{
        .asset = texture_asset,
        .handle = mirakana::runtime::RuntimeAssetHandle{1},
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::TextureSourcePixelFormat::rgba8_unorm,
        .source_bytes = 4,
        .bytes = std::vector<std::uint8_t>{64, 128, 192, 255},
    };
    const auto texture_upload = mirakana::runtime_rhi::upload_runtime_texture(device, texture_payload);
    MK_REQUIRE(texture_upload.succeeded());

    const mirakana::MaterialDefinition material{
        .id = mirakana::AssetId::from_name("materials/preview_bound"),
        .name = "PreviewBound",
        .shading_model = mirakana::MaterialShadingModel::lit,
        .surface_mode = mirakana::MaterialSurfaceMode::opaque,
        .factors = mirakana::MaterialFactors{},
        .texture_bindings = {mirakana::MaterialTextureBinding{.slot = mirakana::MaterialTextureSlot::base_color,
                                                              .texture = texture_asset}},
        .double_sided = false,
    };
    const auto metadata = mirakana::build_material_pipeline_binding_metadata(material);
    const auto material_binding = mirakana::runtime_rhi::create_runtime_material_gpu_binding(
        device, metadata, material.factors,
        {mirakana::runtime_rhi::RuntimeMaterialTextureResource{.slot = mirakana::MaterialTextureSlot::base_color,
                                                               .texture = texture_upload.texture,
                                                               .owner_device = texture_upload.owner_device}});
    MK_REQUIRE(material_binding.succeeded());

    const auto layout = device.create_pipeline_layout(mirakana::rhi::PipelineLayoutDesc{
        .descriptor_sets = {material_binding.descriptor_set_layout},
        .push_constant_bytes = 0,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 64,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "ps_main",
        .bytecode_size = 64,
    });
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    surface.render_frame(
        mirakana::RhiViewportRenderDesc{.graphics_pipeline = pipeline,
                                        .clear_color = mirakana::Color{.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F}},
        [&](mirakana::IRenderer& renderer) {
            renderer.draw_mesh(mirakana::MeshCommand{
                .transform = mirakana::Transform3D{},
                .color = mirakana::Color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F},
                .mesh = mirakana::AssetId::from_name("meshes/material_preview_triangle"),
                .material = material.id,
                .world_from_node = mirakana::Mat4::identity(),
                .mesh_binding = mirakana::MeshGpuBinding{},
                .material_binding = mirakana::MaterialGpuBinding{.pipeline_layout = layout,
                                                                 .descriptor_set = material_binding.descriptor_set,
                                                                 .descriptor_set_index = 0,
                                                                 .owner_device = &device},
            });
        });

    MK_REQUIRE(surface.frames_rendered() == 1);
    const auto stats = device.stats();
    MK_REQUIRE(stats.buffer_texture_copies == 1);
    MK_REQUIRE(stats.samplers_created == 1);
    MK_REQUIRE(stats.descriptor_writes == 4);
    MK_REQUIRE(stats.descriptor_sets_bound == 1);
    MK_REQUIRE(stats.draw_calls == 1);
    MK_REQUIRE(stats.command_lists_submitted >= 3);
}

MK_TEST("rhi viewport surface recreates its render target on resize") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::RhiViewportSurface surface(mirakana::RhiViewportSurfaceDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 160, .height = 90},
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .wait_for_completion = true,
    });

    const auto first_texture = surface.color_texture();
    surface.resize(mirakana::Extent2D{.width = 640, .height = 360});

    MK_REQUIRE(surface.extent().width == 640);
    MK_REQUIRE(surface.extent().height == 360);
    MK_REQUIRE(surface.color_texture().value != 0);
    MK_REQUIRE(surface.color_texture().value != first_texture.value);
    MK_REQUIRE(device.stats().textures_created == 2);

    bool rejected_zero_resize = false;
    try {
        surface.resize(mirakana::Extent2D{.width = 0, .height = 360});
    } catch (const std::invalid_argument&) {
        rejected_zero_resize = true;
    }
    MK_REQUIRE(rejected_zero_resize);
}

int main() {
    return mirakana::test::run_all();
}
