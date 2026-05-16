// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/backend_capabilities.hpp"
#include "mirakana/rhi/gpu_debug.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <string>

MK_TEST("rhi backend capabilities describe shader formats and native presentation support") {
    const auto null_caps = mirakana::rhi::default_backend_capabilities(mirakana::rhi::BackendKind::null);
    const auto d3d12_caps = mirakana::rhi::default_backend_capabilities(mirakana::rhi::BackendKind::d3d12);
    const auto vulkan_caps = mirakana::rhi::default_backend_capabilities(mirakana::rhi::BackendKind::vulkan);
    const auto metal_caps = mirakana::rhi::default_backend_capabilities(mirakana::rhi::BackendKind::metal);

    MK_REQUIRE(mirakana::rhi::backend_kind_id(mirakana::rhi::BackendKind::vulkan) == "vulkan");
    MK_REQUIRE(mirakana::rhi::shader_artifact_format_id(vulkan_caps.shader_format) == "spirv");
    MK_REQUIRE(null_caps.headless);
    MK_REQUIRE(!null_caps.native_device);
    MK_REQUIRE(d3d12_caps.native_device);
    MK_REQUIRE(d3d12_caps.surface_presentation);
    MK_REQUIRE(d3d12_caps.shader_format == mirakana::rhi::ShaderArtifactFormat::dxil);
    MK_REQUIRE(vulkan_caps.explicit_queue_family_selection);
    MK_REQUIRE(vulkan_caps.shader_format == mirakana::rhi::ShaderArtifactFormat::spirv);
    MK_REQUIRE(metal_caps.native_device);
    MK_REQUIRE(metal_caps.surface_presentation);
    MK_REQUIRE(metal_caps.shader_format == mirakana::rhi::ShaderArtifactFormat::metallib);
}

MK_TEST("rhi backend host policy keeps platform specific APIs off unsupported hosts") {
    const auto windows_order = mirakana::rhi::preferred_backend_order(mirakana::rhi::RhiHostPlatform::windows);
    const auto android_order = mirakana::rhi::preferred_backend_order(mirakana::rhi::RhiHostPlatform::android);
    const auto macos_order = mirakana::rhi::preferred_backend_order(mirakana::rhi::RhiHostPlatform::macos);

    MK_REQUIRE(windows_order[0] == mirakana::rhi::BackendKind::d3d12);
    MK_REQUIRE(windows_order[1] == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(windows_order[3] == mirakana::rhi::BackendKind::null);
    MK_REQUIRE(android_order[0] == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(android_order[3] == mirakana::rhi::BackendKind::null);
    MK_REQUIRE(macos_order[0] == mirakana::rhi::BackendKind::metal);
    MK_REQUIRE(macos_order[3] == mirakana::rhi::BackendKind::null);

    MK_REQUIRE(mirakana::rhi::is_backend_supported_on_host(mirakana::rhi::BackendKind::d3d12,
                                                           mirakana::rhi::RhiHostPlatform::windows));
    MK_REQUIRE(!mirakana::rhi::is_backend_supported_on_host(mirakana::rhi::BackendKind::d3d12,
                                                            mirakana::rhi::RhiHostPlatform::linux));
    MK_REQUIRE(mirakana::rhi::is_backend_supported_on_host(mirakana::rhi::BackendKind::vulkan,
                                                           mirakana::rhi::RhiHostPlatform::android));
    MK_REQUIRE(mirakana::rhi::is_backend_supported_on_host(mirakana::rhi::BackendKind::metal,
                                                           mirakana::rhi::RhiHostPlatform::ios));
    MK_REQUIRE(!mirakana::rhi::is_backend_supported_on_host(mirakana::rhi::BackendKind::metal,
                                                            mirakana::rhi::RhiHostPlatform::windows));
}

MK_TEST("rhi backend probe plans encode Vulkan and Metal bootstrap requirements") {
    const auto vulkan_plan = mirakana::rhi::backend_probe_plan(mirakana::rhi::BackendKind::vulkan);
    const auto metal_plan = mirakana::rhi::backend_probe_plan(mirakana::rhi::BackendKind::metal);

    MK_REQUIRE(vulkan_plan.step_count == 6);
    MK_REQUIRE(vulkan_plan.steps[0] == mirakana::rhi::BackendProbeStep::validate_host);
    MK_REQUIRE(vulkan_plan.steps[1] == mirakana::rhi::BackendProbeStep::load_runtime);
    MK_REQUIRE(vulkan_plan.steps[2] == mirakana::rhi::BackendProbeStep::enumerate_physical_devices);
    MK_REQUIRE(vulkan_plan.steps[3] == mirakana::rhi::BackendProbeStep::query_queue_families);
    MK_REQUIRE(vulkan_plan.steps[4] == mirakana::rhi::BackendProbeStep::create_logical_device);
    MK_REQUIRE(vulkan_plan.steps[5] == mirakana::rhi::BackendProbeStep::verify_shader_artifacts);

    MK_REQUIRE(metal_plan.step_count == 5);
    MK_REQUIRE(metal_plan.steps[0] == mirakana::rhi::BackendProbeStep::validate_host);
    MK_REQUIRE(metal_plan.steps[1] == mirakana::rhi::BackendProbeStep::load_runtime);
    MK_REQUIRE(metal_plan.steps[2] == mirakana::rhi::BackendProbeStep::create_default_device);
    MK_REQUIRE(metal_plan.steps[3] == mirakana::rhi::BackendProbeStep::create_command_queue);
    MK_REQUIRE(metal_plan.steps[4] == mirakana::rhi::BackendProbeStep::verify_shader_artifacts);
}

MK_TEST("rhi backend probe results carry deterministic diagnostics") {
    const auto unsupported = mirakana::rhi::make_backend_probe_result(
        mirakana::rhi::BackendKind::metal, mirakana::rhi::RhiHostPlatform::windows,
        mirakana::rhi::BackendProbeStatus::unsupported_host, "Metal is only available on Apple platforms");
    const auto available = mirakana::rhi::make_backend_probe_result(mirakana::rhi::BackendKind::vulkan,
                                                                    mirakana::rhi::RhiHostPlatform::linux,
                                                                    mirakana::rhi::BackendProbeStatus::available);

    MK_REQUIRE(unsupported.backend == mirakana::rhi::BackendKind::metal);
    MK_REQUIRE(unsupported.host == mirakana::rhi::RhiHostPlatform::windows);
    MK_REQUIRE(unsupported.status == mirakana::rhi::BackendProbeStatus::unsupported_host);
    MK_REQUIRE(unsupported.diagnostic == "Metal is only available on Apple platforms");
    MK_REQUIRE(!unsupported.capabilities.native_device);

    MK_REQUIRE(available.capabilities.backend == mirakana::rhi::BackendKind::vulkan);
    MK_REQUIRE(available.capabilities.native_device);
    MK_REQUIRE(available.diagnostic == "available");
    MK_REQUIRE(mirakana::rhi::backend_probe_status_id(available.status) == "available");
}

MK_TEST("null rhi creates buffers and textures with stable handles") {
    mirakana::rhi::NullRhiDevice device;

    const auto vertex_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 1024,
        .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto color_texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 640, .height = 480, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });

    MK_REQUIRE(device.backend_kind() == mirakana::rhi::BackendKind::null);
    MK_REQUIRE(device.backend_name() == "null");
    MK_REQUIRE(vertex_buffer.value == 1);
    MK_REQUIRE(color_texture.value == 1);
    MK_REQUIRE(device.stats().buffers_created == 1);
    MK_REQUIRE(device.stats().textures_created == 1);
}

MK_TEST("null rhi mirrors buffer and texture teardown into resource lifetime registry") {
    mirakana::rhi::NullRhiDevice device;

    const auto vertex_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto color_texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::shader_resource,
    });

    const auto* lifetime = device.resource_lifetime_registry();
    MK_REQUIRE(lifetime != nullptr);
    MK_REQUIRE(lifetime->records().size() == 2);
    MK_REQUIRE(lifetime->records()[0].kind == mirakana::rhi::RhiResourceKind::buffer);
    MK_REQUIRE(lifetime->records()[0].debug_name == "buffer-1");
    MK_REQUIRE(lifetime->records()[1].kind == mirakana::rhi::RhiResourceKind::texture);
    MK_REQUIRE(lifetime->records()[1].debug_name == "texture-1");

    MK_REQUIRE(device.null_mark_buffer_released(vertex_buffer));
    MK_REQUIRE(lifetime->records().size() == 1);
    MK_REQUIRE(device.null_mark_texture_released(color_texture));
    MK_REQUIRE(lifetime->records().empty());
}

MK_TEST("null rhi mirrors sampler shader descriptor and pipeline teardown into resource lifetime registry") {
    mirakana::rhi::NullRhiDevice device;

    const auto sampler = device.create_sampler(mirakana::rhi::SamplerDesc{});
    const auto vs = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 4, .bytecode = "abcd"});
    const auto fs = device.create_shader(mirakana::rhi::ShaderDesc{.stage = mirakana::rhi::ShaderStage::fragment,
                                                                   .entry_point = "fs_main",
                                                                   .bytecode_size = 4,
                                                                   .bytecode = "efgh"});
    const auto dsl = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{});
    const auto ds = device.allocate_descriptor_set(dsl);
    const auto pl = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {dsl}, .push_constant_bytes = 0});
    const auto gp = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = pl,
        .vertex_shader = vs,
        .fragment_shader = fs,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    const auto* lifetime = device.resource_lifetime_registry();
    MK_REQUIRE(lifetime != nullptr);
    MK_REQUIRE(lifetime->records().size() == 7);

    MK_REQUIRE(device.null_mark_graphics_pipeline_released(gp));
    MK_REQUIRE(device.null_mark_pipeline_layout_released(pl));
    MK_REQUIRE(device.null_mark_descriptor_set_released(ds));
    MK_REQUIRE(device.null_mark_descriptor_set_layout_released(dsl));
    MK_REQUIRE(device.null_mark_shader_released(fs));
    MK_REQUIRE(device.null_mark_shader_released(vs));
    MK_REQUIRE(device.null_mark_sampler_released(sampler));
    MK_REQUIRE(lifetime->records().empty());
}

MK_TEST("null rhi creates compute pipelines and records dispatches") {
    mirakana::rhi::NullRhiDevice device;

    const auto storage = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64,
        .usage = mirakana::rhi::BufferUsage::storage,
    });
    const auto set_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::storage_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::compute,
        },
    }});
    const auto descriptor_set = device.allocate_descriptor_set(set_layout);
    device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::storage_buffer,
                                                                storage)},
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {set_layout}, .push_constant_bytes = 0});
    const auto shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute, .entry_point = "cs_main", .bytecode_size = 64});
    const auto pipeline = device.create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = layout,
        .compute_shader = shader,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::compute);
    commands->bind_compute_pipeline(pipeline);
    commands->bind_descriptor_set(layout, 0, descriptor_set);
    commands->dispatch(2, 3, 4);
    commands->close();
    (void)device.submit(*commands);

    MK_REQUIRE(pipeline.value == 1);
    MK_REQUIRE(device.stats().compute_pipelines_created == 1);
    MK_REQUIRE(device.stats().compute_pipelines_bound == 1);
    MK_REQUIRE(device.stats().descriptor_sets_bound == 1);
    MK_REQUIRE(device.stats().compute_dispatches == 1);
    MK_REQUIRE(device.stats().compute_workgroups_x == 2);
    MK_REQUIRE(device.stats().compute_workgroups_y == 3);
    MK_REQUIRE(device.stats().compute_workgroups_z == 4);
}

MK_TEST("null rhi rejects invalid compute pipeline and dispatch ordering") {
    mirakana::rhi::NullRhiDevice device;

    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto compute_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute, .entry_point = "cs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto pipeline = device.create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
        .layout = layout,
        .compute_shader = compute_shader,
    });

    bool rejected_unknown_layout = false;
    try {
        (void)device.create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
            .layout = mirakana::rhi::PipelineLayoutHandle{99},
            .compute_shader = compute_shader,
        });
    } catch (const std::invalid_argument&) {
        rejected_unknown_layout = true;
    }

    bool rejected_wrong_shader_stage = false;
    try {
        (void)device.create_compute_pipeline(mirakana::rhi::ComputePipelineDesc{
            .layout = layout,
            .compute_shader = fragment_shader,
        });
    } catch (const std::invalid_argument&) {
        rejected_wrong_shader_stage = true;
    }

    bool rejected_graphics_queue_bind = false;
    try {
        auto graphics_commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
        graphics_commands->bind_compute_pipeline(pipeline);
    } catch (const std::logic_error&) {
        rejected_graphics_queue_bind = true;
    }

    bool rejected_dispatch_without_pipeline = false;
    try {
        auto compute_commands = device.begin_command_list(mirakana::rhi::QueueKind::compute);
        compute_commands->dispatch(1, 1, 1);
    } catch (const std::logic_error&) {
        rejected_dispatch_without_pipeline = true;
    }

    bool rejected_zero_workgroups = false;
    try {
        auto compute_commands = device.begin_command_list(mirakana::rhi::QueueKind::compute);
        compute_commands->bind_compute_pipeline(pipeline);
        compute_commands->dispatch(1, 0, 1);
    } catch (const std::invalid_argument&) {
        rejected_zero_workgroups = true;
    }

    MK_REQUIRE(rejected_unknown_layout);
    MK_REQUIRE(rejected_wrong_shader_stage);
    MK_REQUIRE(rejected_graphics_queue_bind);
    MK_REQUIRE(rejected_dispatch_without_pipeline);
    MK_REQUIRE(rejected_zero_workgroups);
    MK_REQUIRE(device.stats().compute_pipelines_created == 1);
    MK_REQUIRE(device.stats().compute_pipelines_bound == 1);
    MK_REQUIRE(device.stats().compute_dispatches == 0);
}

/// Validates GPU skinning vertex contract: `uint16x4` joint indices + `float32x4` weights map to D3D12
/// `BLENDINDICES` / `BLENDWEIGHT` semantics via first-party `VertexSemantic` values.
MK_TEST("null rhi accepts skinned tangent-space vertex layout with joint index and weight attributes") {
    mirakana::rhi::NullRhiDevice device;
    const auto vs = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_skin", .bytecode_size = 4, .bytecode = "abcd"});
    const auto fs = device.create_shader(mirakana::rhi::ShaderDesc{.stage = mirakana::rhi::ShaderStage::fragment,
                                                                   .entry_point = "fs_skin",
                                                                   .bytecode_size = 4,
                                                                   .bytecode = "efgh"});
    const auto dsl = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{});
    const auto pl = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {dsl}, .push_constant_bytes = 0});

    constexpr std::uint32_t k_stride = 72U;
    const auto
        pipeline =
            device.create_graphics_pipeline(mirakana::rhi::
                                                GraphicsPipelineDesc{
                                                    .layout = pl,
                                                    .vertex_shader = vs,
                                                    .fragment_shader = fs,
                                                    .color_format = mirakana::rhi::Format::rgba8_unorm,
                                                    .depth_format = mirakana::rhi::Format::unknown,
                                                    .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
                                                    .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{
                                                        .binding = 0,
                                                        .stride = k_stride,
                                                        .input_rate = mirakana::rhi::VertexInputRate::vertex}},
                                                    .vertex_attributes =
                                                        {
                                                            mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                                               .binding = 0,
                                                                                               .offset = 0,
                                                                                               .format =
                                                                                                   mirakana::
                                                                                                       rhi::
                                                                                                           VertexFormat::
                                                                                                               float32x3,
                                                                                               .semantic =
                                                                                                   mirakana::rhi::
                                                                                                       VertexSemantic::
                                                                                                           position,
                                                                                               .semantic_index = 0},
                                                            mirakana::
                                                                rhi::
                                                                    VertexAttributeDesc{
                                                                        .location = 1,
                                                                        .binding = 0,
                                                                        .offset = 12,
                                                                        .format = mirakana::rhi::VertexFormat::
                                                                            float32x3,
                                                                        .semantic =
                                                                            mirakana::rhi::VertexSemantic::normal,
                                                                        .semantic_index = 0},
                                                            mirakana::rhi::
                                                                VertexAttributeDesc{
                                                                    .location = 2,
                                                                    .binding = 0,
                                                                    .offset = 24,
                                                                    .format = mirakana::rhi::VertexFormat::float32x2,
                                                                    .semantic = mirakana::rhi::VertexSemantic::texcoord,
                                                                    .semantic_index = 0},
                                                            mirakana::rhi::
                                                                VertexAttributeDesc{
                                                                    .location = 3,
                                                                    .binding = 0,
                                                                    .offset = 32,
                                                                    .format = mirakana::rhi::VertexFormat::float32x4,
                                                                    .semantic = mirakana::rhi::VertexSemantic::tangent,
                                                                    .semantic_index = 0},
                                                            mirakana::rhi::
                                                                VertexAttributeDesc{
                                                                    .location = 4,
                                                                    .binding = 0,
                                                                    .offset = 48,
                                                                    .format = mirakana::rhi::VertexFormat::uint16x4,
                                                                    .semantic = mirakana::rhi::
                                                                        VertexSemantic::joint_indices,
                                                                    .semantic_index = 0},
                                                            mirakana::rhi::
                                                                VertexAttributeDesc{
                                                                    .location = 5,
                                                                    .binding = 0,
                                                                    .offset = 56,
                                                                    .format = mirakana::rhi::VertexFormat::float32x4,
                                                                    .semantic = mirakana::rhi::
                                                                        VertexSemantic::joint_weights,
                                                                    .semantic_index = 0},
                                                        },
                                                });
    MK_REQUIRE(pipeline.value != 0);
}

MK_TEST("null rhi transient buffer release updates resource lifetime registry") {
    mirakana::rhi::NullRhiDevice device;
    const auto transient = device.acquire_transient_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 128,
        .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination,
    });
    MK_REQUIRE(device.resource_lifetime_registry() != nullptr);
    MK_REQUIRE(device.resource_lifetime_registry()->records().size() == 1);
    device.release_transient(transient.lease);
    MK_REQUIRE(device.resource_lifetime_registry()->records().empty());
}

MK_TEST("null rhi records command lists and signals fences on submit") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 1280, .height = 720, .depth = 1},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::present,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(commands->queue_kind() == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(!commands->closed());

    commands->transition_texture(target, mirakana::rhi::ResourceState::undefined,
                                 mirakana::rhi::ResourceState::render_target);
    commands->transition_texture(target, mirakana::rhi::ResourceState::render_target,
                                 mirakana::rhi::ResourceState::present);
    commands->close();
    MK_REQUIRE(commands->closed());

    const auto fence = device.submit(*commands);
    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(device.completed_fence().value == 1);
    MK_REQUIRE(device.stats().command_lists_begun == 1);
    MK_REQUIRE(device.stats().command_lists_submitted == 1);
    MK_REQUIRE(device.stats().resource_transitions == 2);
    MK_REQUIRE(device.stats().fences_signaled == 1);
    MK_REQUIRE(device.stats().last_submitted_fence_value == 1);

    device.wait(fence);

    MK_REQUIRE(device.stats().fence_waits == 1);
    MK_REQUIRE(device.stats().fence_wait_failures == 0);
    MK_REQUIRE(device.stats().last_completed_fence_value == 1);
}

MK_TEST("null rhi records texture aliasing barriers without changing texture state") {
    mirakana::rhi::NullRhiDevice device;
    const auto first = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::render_target |
                 mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto second = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination | mirakana::rhi::TextureUsage::render_target |
                 mirakana::rhi::TextureUsage::shader_resource,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->texture_aliasing_barrier(first, second);
    commands->transition_texture(first, mirakana::rhi::ResourceState::copy_destination,
                                 mirakana::rhi::ResourceState::render_target);
    commands->transition_texture(second, mirakana::rhi::ResourceState::copy_destination,
                                 mirakana::rhi::ResourceState::shader_read);
    commands->close();

    const auto fence = device.submit(*commands);
    device.wait(fence);

    const auto stats = device.stats();
    MK_REQUIRE(stats.texture_aliasing_barriers == 1);
    MK_REQUIRE(stats.resource_transitions == 2);
}

MK_TEST("null rhi rejects invalid texture aliasing barrier handles") {
    mirakana::rhi::NullRhiDevice device;
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    bool rejected_same = false;
    try {
        commands->texture_aliasing_barrier(texture, texture);
    } catch (const std::invalid_argument&) {
        rejected_same = true;
    }

    bool rejected_empty = false;
    try {
        commands->texture_aliasing_barrier(mirakana::rhi::TextureHandle{}, texture);
    } catch (const std::invalid_argument&) {
        rejected_empty = true;
    }

    bool rejected_unknown = false;
    try {
        commands->texture_aliasing_barrier(texture, mirakana::rhi::TextureHandle{999});
    } catch (const std::invalid_argument&) {
        rejected_unknown = true;
    }
    commands->close();

    MK_REQUIRE(rejected_same);
    MK_REQUIRE(rejected_empty);
    MK_REQUIRE(rejected_unknown);
    MK_REQUIRE(device.stats().texture_aliasing_barriers == 0);
}

MK_TEST("null rhi reports invalid fence wait attempts") {
    mirakana::rhi::NullRhiDevice device;

    bool rejected_unsubmitted_fence = false;
    try {
        device.wait(mirakana::rhi::FenceValue{.value = 1});
    } catch (const std::invalid_argument&) {
        rejected_unsubmitted_fence = true;
    }

    MK_REQUIRE(rejected_unsubmitted_fence);
    MK_REQUIRE(device.stats().fence_waits == 1);
    MK_REQUIRE(device.stats().fence_wait_failures == 1);
    MK_REQUIRE(device.stats().last_submitted_fence_value == 0);
    MK_REQUIRE(device.stats().last_completed_fence_value == 0);
}

MK_TEST("null rhi records queue waits for submitted fences") {
    mirakana::rhi::NullRhiDevice device;

    auto compute_commands = device.begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->close();
    const auto compute_fence = device.submit(*compute_commands);

    device.wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);

    auto graphics_commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->close();
    const auto graphics_fence = device.submit(*graphics_commands);

    MK_REQUIRE(device.stats().queue_waits == 1);
    MK_REQUIRE(device.stats().queue_wait_failures == 0);
    MK_REQUIRE(device.stats().fence_waits == 0);
    MK_REQUIRE(device.stats().compute_queue_submits == 1);
    MK_REQUIRE(device.stats().graphics_queue_submits == 1);
    MK_REQUIRE(device.stats().last_compute_submitted_fence_value == compute_fence.value);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_fence_value == compute_fence.value);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(device.stats().last_queue_wait_fence_value == compute_fence.value);
    MK_REQUIRE(device.stats().last_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(device.stats().last_graphics_submitted_fence_value == graphics_fence.value);
    MK_REQUIRE(device.stats().last_submitted_fence_value == graphics_fence.value);
    MK_REQUIRE(device.stats().last_submitted_fence_queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(device.stats().last_completed_fence_value == graphics_fence.value);
    MK_REQUIRE(device.stats().last_compute_submit_sequence < device.stats().last_graphics_queue_wait_sequence);
    MK_REQUIRE(device.stats().last_graphics_queue_wait_sequence < device.stats().last_graphics_submit_sequence);
}

MK_TEST("null rhi submitted fences carry queue identity and per queue values") {
    mirakana::rhi::NullRhiDevice device;

    auto graphics_commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->close();
    const auto graphics_fence = device.submit(*graphics_commands);

    auto compute_commands = device.begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->close();
    const auto compute_fence = device.submit(*compute_commands);

    auto copy_commands = device.begin_command_list(mirakana::rhi::QueueKind::copy);
    copy_commands->close();
    const auto copy_fence = device.submit(*copy_commands);

    MK_REQUIRE(graphics_fence.queue == mirakana::rhi::QueueKind::graphics);
    MK_REQUIRE(compute_fence.queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(copy_fence.queue == mirakana::rhi::QueueKind::copy);
    MK_REQUIRE(graphics_fence.value == 1);
    MK_REQUIRE(compute_fence.value == 1);
    MK_REQUIRE(copy_fence.value == 1);
}

MK_TEST("rhi async overlap readiness diagnostics classify serial graphics waits") {
    mirakana::rhi::NullRhiDevice device;

    auto compute_commands = device.begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->close();
    const auto compute_fence = device.submit(*compute_commands);

    device.wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);

    auto graphics_commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->close();
    const auto graphics_fence = device.submit(*graphics_commands);

    const auto diagnostics = mirakana::rhi::diagnose_compute_graphics_async_overlap_readiness(device.stats(), 0);

    MK_REQUIRE(diagnostics.status == mirakana::rhi::RhiAsyncOverlapReadinessStatus::not_proven_serial_dependency);
    MK_REQUIRE(diagnostics.compute_queue_submitted);
    MK_REQUIRE(diagnostics.graphics_queue_waited_for_compute);
    MK_REQUIRE(diagnostics.graphics_queue_submitted_after_wait);
    MK_REQUIRE(diagnostics.same_frame_graphics_wait_serializes_compute);
    MK_REQUIRE(!diagnostics.gpu_timestamps_available);
    MK_REQUIRE(diagnostics.last_compute_submitted_fence_value == compute_fence.value);
    MK_REQUIRE(diagnostics.last_graphics_queue_wait_fence_value == compute_fence.value);
    MK_REQUIRE(diagnostics.last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::compute);
    MK_REQUIRE(diagnostics.last_graphics_submitted_fence_value == graphics_fence.value);
    MK_REQUIRE(diagnostics.last_graphics_queue_wait_sequence < diagnostics.last_graphics_submit_sequence);
}

MK_TEST("rhi async overlap readiness diagnostics require timestamp support for timing candidates") {
    mirakana::rhi::RhiStats stats{};
    stats.compute_queue_submits = 1;
    stats.graphics_queue_submits = 1;
    stats.last_compute_submitted_fence_value = 3;
    stats.last_graphics_submitted_fence_value = 4;

    const auto unsupported = mirakana::rhi::diagnose_compute_graphics_async_overlap_readiness(stats, 0);
    const auto ready = mirakana::rhi::diagnose_compute_graphics_async_overlap_readiness(stats, 1'000'000);

    MK_REQUIRE(unsupported.status ==
               mirakana::rhi::RhiAsyncOverlapReadinessStatus::unsupported_missing_timestamp_support);
    MK_REQUIRE(!unsupported.gpu_timestamps_available);
    MK_REQUIRE(ready.status == mirakana::rhi::RhiAsyncOverlapReadinessStatus::ready_for_backend_private_timing);
    MK_REQUIRE(ready.gpu_timestamps_available);
}

MK_TEST("rhi async overlap readiness diagnostics classify pipelined output slot scheduling") {
    mirakana::rhi::RhiStats stats{};
    stats.compute_queue_submits = 2;
    stats.graphics_queue_submits = 1;
    stats.queue_waits = 1;
    stats.last_compute_submitted_fence_value = 5;
    stats.last_graphics_queue_wait_fence_value = 3;
    stats.last_graphics_queue_wait_fence_queue = mirakana::rhi::QueueKind::compute;
    stats.last_graphics_submitted_fence_value = 6;
    stats.last_graphics_queue_wait_sequence = 2;
    stats.last_compute_submit_sequence = 3;
    stats.last_graphics_submit_sequence = 4;

    const mirakana::rhi::RhiPipelinedComputeGraphicsScheduleEvidence schedule{
        .output_slot_count = 2,
        .current_compute_output_slot_index = 1,
        .graphics_consumed_output_slot_index = 0,
        .previous_compute_fence = mirakana::rhi::FenceValue{.value = 3, .queue = mirakana::rhi::QueueKind::compute},
        .current_compute_fence = mirakana::rhi::FenceValue{.value = 5, .queue = mirakana::rhi::QueueKind::compute},
    };
    const auto diagnostics =
        mirakana::rhi::diagnose_pipelined_compute_graphics_async_overlap_readiness(stats, schedule, 1'000'000);

    MK_REQUIRE(diagnostics.status == mirakana::rhi::RhiAsyncOverlapReadinessStatus::ready_for_backend_private_timing);
    MK_REQUIRE(diagnostics.output_ring_available);
    MK_REQUIRE(diagnostics.compute_and_graphics_use_distinct_output_slots);
    MK_REQUIRE(diagnostics.graphics_queue_waited_for_previous_compute);
    MK_REQUIRE(!diagnostics.graphics_queue_waited_for_current_compute);
    MK_REQUIRE(diagnostics.graphics_queue_submitted_after_wait);
    MK_REQUIRE(!diagnostics.same_frame_graphics_wait_serializes_compute);
    MK_REQUIRE(diagnostics.previous_compute_submitted_fence_value == 3);
    MK_REQUIRE(diagnostics.last_compute_submitted_fence_value == 5);
    MK_REQUIRE(diagnostics.last_graphics_queue_wait_fence_value == 3);
    MK_REQUIRE(diagnostics.last_graphics_submitted_fence_value == 6);
}

MK_TEST("null rhi reports invalid queue wait attempts") {
    mirakana::rhi::NullRhiDevice device;

    bool rejected_unsubmitted_fence = false;
    try {
        device.wait_for_queue(mirakana::rhi::QueueKind::graphics, mirakana::rhi::FenceValue{.value = 1});
    } catch (const std::invalid_argument&) {
        rejected_unsubmitted_fence = true;
    }

    MK_REQUIRE(rejected_unsubmitted_fence);
    MK_REQUIRE(device.stats().queue_waits == 1);
    MK_REQUIRE(device.stats().queue_wait_failures == 1);
    MK_REQUIRE(device.stats().fence_waits == 0);
    MK_REQUIRE(device.stats().last_submitted_fence_value == 0);
    MK_REQUIRE(device.stats().last_completed_fence_value == 0);
}

MK_TEST("null rhi records buffer texture readback commands") {
    mirakana::rhi::NullRhiDevice device;
    const auto upload = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 4096,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto readback = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 4096,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_source | mirakana::rhi::TextureUsage::copy_destination,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::copy);
    commands->copy_buffer(
        upload, readback,
        mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 512, .size_bytes = 1024});
    commands->copy_buffer_to_texture(
        upload, texture,
        mirakana::rhi::BufferTextureCopyRegion{
            .buffer_offset = 0,
            .buffer_row_length = 0,
            .buffer_image_height = 0,
            .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
            .texture_extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        });
    commands->copy_texture_to_buffer(
        texture, readback,
        mirakana::rhi::BufferTextureCopyRegion{
            .buffer_offset = 0,
            .buffer_row_length = 0,
            .buffer_image_height = 0,
            .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
            .texture_extent = mirakana::rhi::Extent3D{.width = 16, .height = 16, .depth = 1},
        });
    commands->close();

    (void)device.submit(*commands);
    MK_REQUIRE(device.stats().buffer_copies == 1);
    MK_REQUIRE(device.stats().buffer_texture_copies == 1);
    MK_REQUIRE(device.stats().texture_buffer_copies == 1);
    MK_REQUIRE(device.stats().bytes_copied == 1024);
}

MK_TEST("null rhi reads copy destination buffer ranges") {
    mirakana::rhi::NullRhiDevice device;
    const auto readback = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 64,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });

    const auto bytes = device.read_buffer(readback, 8, 16);

    MK_REQUIRE(bytes.size() == 16);
    MK_REQUIRE(device.stats().buffer_reads == 1);
    MK_REQUIRE(device.stats().bytes_read == 16);

    bool rejected_usage = false;
    try {
        const auto vertex_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
            .size_bytes = 64,
            .usage = mirakana::rhi::BufferUsage::vertex,
        });
        (void)device.read_buffer(vertex_buffer, 0, 16);
    } catch (const std::invalid_argument&) {
        rejected_usage = true;
    }

    bool rejected_range = false;
    try {
        (void)device.read_buffer(readback, 56, 16);
    } catch (const std::invalid_argument&) {
        rejected_range = true;
    }

    MK_REQUIRE(rejected_usage);
    MK_REQUIRE(rejected_range);
}

MK_TEST("null rhi writes upload buffer bytes and copies them to readable destinations") {
    mirakana::rhi::NullRhiDevice device;
    const auto upload = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 8,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto readback = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 8,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });
    const std::vector<std::uint8_t> bytes{0x10, 0x20, 0x30, 0x40};

    device.write_buffer(upload, 2, bytes);
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::copy);
    commands->copy_buffer(
        upload, readback,
        mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = 8});
    commands->close();
    (void)device.submit(*commands);

    const auto copied = device.read_buffer(readback, 0, 8);

    MK_REQUIRE(copied.size() == 8);
    MK_REQUIRE(copied[0] == 0x00);
    MK_REQUIRE(copied[1] == 0x00);
    MK_REQUIRE(copied[2] == 0x10);
    MK_REQUIRE(copied[3] == 0x20);
    MK_REQUIRE(copied[4] == 0x30);
    MK_REQUIRE(copied[5] == 0x40);
    MK_REQUIRE(device.stats().buffer_writes == 1);
    MK_REQUIRE(device.stats().bytes_written == 4);
}

MK_TEST("rhi computes buffer texture copy footprints") {
    const auto tight = mirakana::rhi::buffer_texture_copy_required_bytes(
        mirakana::rhi::Format::rgba8_unorm,
        mirakana::rhi::BufferTextureCopyRegion{
            .buffer_offset = 128,
            .buffer_row_length = 0,
            .buffer_image_height = 0,
            .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
            .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        });
    const auto padded = mirakana::rhi::buffer_texture_copy_required_bytes(
        mirakana::rhi::Format::bgra8_unorm,
        mirakana::rhi::BufferTextureCopyRegion{
            .buffer_offset = 64,
            .buffer_row_length = 8,
            .buffer_image_height = 6,
            .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
            .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 3},
        });

    bool rejected_row_length = false;
    try {
        (void)mirakana::rhi::buffer_texture_copy_required_bytes(
            mirakana::rhi::Format::rgba8_unorm,
            mirakana::rhi::BufferTextureCopyRegion{
                .buffer_offset = 0,
                .buffer_row_length = 3,
                .buffer_image_height = 0,
                .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
                .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
            });
    } catch (const std::invalid_argument&) {
        rejected_row_length = true;
    }

    MK_REQUIRE(tight == 192);
    MK_REQUIRE(padded == 560);
    MK_REQUIRE(rejected_row_length);
}

MK_TEST("null rhi rejects invalid copy and present commands") {
    mirakana::rhi::NullRhiDevice device;
    const auto source = device.create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 64, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto non_copy_source =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 64, .usage = mirakana::rhi::BufferUsage::vertex});
    const auto destination = device.create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 64, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::copy);

    bool rejected_range = false;
    try {
        commands->copy_buffer(
            source, destination,
            mirakana::rhi::BufferCopyRegion{.source_offset = 32, .destination_offset = 0, .size_bytes = 64});
    } catch (const std::invalid_argument&) {
        rejected_range = true;
    }

    bool rejected_usage = false;
    try {
        commands->copy_buffer(
            non_copy_source, destination,
            mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = 16});
    } catch (const std::invalid_argument&) {
        rejected_usage = true;
    }

    bool rejected_extent = false;
    try {
        commands->copy_buffer_to_texture(
            source, texture,
            mirakana::rhi::BufferTextureCopyRegion{
                .buffer_offset = 0,
                .buffer_row_length = 0,
                .buffer_image_height = 0,
                .texture_offset = mirakana::rhi::Offset3D{.x = 4, .y = 4, .z = 0},
                .texture_extent = mirakana::rhi::Extent3D{.width = 8, .height = 8, .depth = 1},
            });
    } catch (const std::invalid_argument&) {
        rejected_extent = true;
    }

    bool rejected_swapchain = false;
    auto present_commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    try {
        present_commands->present(mirakana::rhi::SwapchainFrameHandle{99});
    } catch (const std::invalid_argument&) {
        rejected_swapchain = true;
    }

    MK_REQUIRE(rejected_range);
    MK_REQUIRE(rejected_usage);
    MK_REQUIRE(rejected_extent);
    MK_REQUIRE(rejected_swapchain);
}

MK_TEST("null rhi validates buffer texture copy footprint against buffer size") {
    mirakana::rhi::NullRhiDevice device;
    const auto too_small = device.create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 63, .usage = mirakana::rhi::BufferUsage::copy_source});
    const auto readback = device.create_buffer(
        mirakana::rhi::BufferDesc{.size_bytes = 63, .usage = mirakana::rhi::BufferUsage::copy_destination});
    const auto texture = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_source | mirakana::rhi::TextureUsage::copy_destination,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::copy);

    bool rejected_upload_span = false;
    try {
        commands->copy_buffer_to_texture(
            too_small, texture,
            mirakana::rhi::BufferTextureCopyRegion{
                .buffer_offset = 0,
                .buffer_row_length = 0,
                .buffer_image_height = 0,
                .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
                .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
            });
    } catch (const std::invalid_argument&) {
        rejected_upload_span = true;
    }

    bool rejected_readback_span = false;
    try {
        commands->copy_texture_to_buffer(
            texture, readback,
            mirakana::rhi::BufferTextureCopyRegion{
                .buffer_offset = 0,
                .buffer_row_length = 0,
                .buffer_image_height = 0,
                .texture_offset = mirakana::rhi::Offset3D{.x = 0, .y = 0, .z = 0},
                .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
            });
    } catch (const std::invalid_argument&) {
        rejected_readback_span = true;
    }

    MK_REQUIRE(rejected_upload_span);
    MK_REQUIRE(rejected_readback_span);
}

MK_TEST("null rhi acquires releases and invalidates transient resources") {
    mirakana::rhi::NullRhiDevice device;
    const auto transient_upload = device.acquire_transient_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 128,
        .usage = mirakana::rhi::BufferUsage::copy_source,
    });
    const auto transient_target = device.acquire_transient_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::copy_destination,
    });
    const auto persistent_destination = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 128,
        .usage = mirakana::rhi::BufferUsage::copy_destination,
    });

    MK_REQUIRE(transient_upload.lease.value == 1);
    MK_REQUIRE(transient_upload.buffer.value != 0);
    MK_REQUIRE(transient_target.lease.value == 2);
    MK_REQUIRE(transient_target.texture.value != 0);
    MK_REQUIRE(device.stats().transient_resources_acquired == 2);
    MK_REQUIRE(device.stats().transient_resources_active == 2);

    device.release_transient(transient_upload.lease);
    MK_REQUIRE(device.stats().transient_resources_released == 1);
    MK_REQUIRE(device.stats().transient_resources_active == 1);

    bool rejected_released_buffer = false;
    try {
        auto commands = device.begin_command_list(mirakana::rhi::QueueKind::copy);
        commands->copy_buffer(
            transient_upload.buffer, persistent_destination,
            mirakana::rhi::BufferCopyRegion{.source_offset = 0, .destination_offset = 0, .size_bytes = 16});
    } catch (const std::invalid_argument&) {
        rejected_released_buffer = true;
    }

    bool rejected_double_release = false;
    try {
        device.release_transient(transient_upload.lease);
    } catch (const std::invalid_argument&) {
        rejected_double_release = true;
    }

    device.release_transient(transient_target.lease);
    MK_REQUIRE(device.stats().transient_resources_active == 0);
    MK_REQUIRE(rejected_released_buffer);
    MK_REQUIRE(rejected_double_release);
}

MK_TEST("null rhi rejects invalid resource descriptions") {
    mirakana::rhi::NullRhiDevice device;

    bool rejected_buffer = false;
    try {
        (void)device.create_buffer(
            mirakana::rhi::BufferDesc{.size_bytes = 0, .usage = mirakana::rhi::BufferUsage::vertex});
    } catch (const std::invalid_argument&) {
        rejected_buffer = true;
    }

    bool rejected_texture = false;
    try {
        (void)device.create_texture(mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 0, .height = 480, .depth = 1},
            .format = mirakana::rhi::Format::rgba8_unorm,
            .usage = mirakana::rhi::TextureUsage::render_target,
        });
    } catch (const std::invalid_argument&) {
        rejected_texture = true;
    }

    MK_REQUIRE(rejected_buffer);
    MK_REQUIRE(rejected_texture);
}

MK_TEST("null rhi creates swapchain shader and graphics pipeline contracts") {
    mirakana::rhi::NullRhiDevice device;

    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 1280, .height = 720},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex,
        .entry_point = "vs_main",
        .bytecode_size = 256,
    });
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment,
        .entry_point = "fs_main",
        .bytecode_size = 256,
    });
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 128});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    device.resize_swapchain(swapchain, mirakana::rhi::Extent2D{.width = 1920, .height = 1080});

    MK_REQUIRE(swapchain.value == 1);
    MK_REQUIRE(vertex_shader.value == 1);
    MK_REQUIRE(fragment_shader.value == 2);
    MK_REQUIRE(layout.value == 1);
    MK_REQUIRE(pipeline.value == 1);
    MK_REQUIRE(device.stats().swapchains_created == 1);
    MK_REQUIRE(device.stats().swapchain_resizes == 1);
    MK_REQUIRE(device.stats().shader_modules_created == 2);
    MK_REQUIRE(device.stats().pipeline_layouts_created == 1);
    MK_REQUIRE(device.stats().graphics_pipelines_created == 1);
}

MK_TEST("null rhi enforces swapchain present sequencing") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 800, .height = 600},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
    });

    auto copy_commands = device.begin_command_list(mirakana::rhi::QueueKind::copy);
    const auto frame = device.acquire_swapchain_frame(swapchain);
    bool rejected_copy_queue_present = false;
    try {
        copy_commands->present(frame);
    } catch (const std::logic_error&) {
        rejected_copy_queue_present = true;
    }
    copy_commands->close();

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    bool rejected_present_without_render = false;
    try {
        commands->present(frame);
    } catch (const std::invalid_argument&) {
        rejected_present_without_render = true;
    }
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = frame,
            },
    });

    bool rejected_present_inside_render_pass = false;
    try {
        commands->present(frame);
    } catch (const std::logic_error&) {
        rejected_present_inside_render_pass = true;
    }

    commands->end_render_pass();
    commands->present(frame);

    bool rejected_duplicate_present = false;
    try {
        commands->present(frame);
    } catch (const std::invalid_argument&) {
        rejected_duplicate_present = true;
    }
    commands->close();
    (void)device.submit(*commands);

    MK_REQUIRE(rejected_copy_queue_present);
    MK_REQUIRE(rejected_present_without_render);
    MK_REQUIRE(rejected_present_inside_render_pass);
    MK_REQUIRE(rejected_duplicate_present);
    MK_REQUIRE(device.stats().present_calls == 1);
}

MK_TEST("null rhi rejects multiple pending frames for one swapchain") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 800, .height = 600},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
    });

    auto first = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    const auto first_frame = device.acquire_swapchain_frame(swapchain);
    first->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = first_frame,
            },
    });
    first->end_render_pass();

    auto second = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    bool rejected_second_frame = false;
    mirakana::rhi::SwapchainFrameHandle second_frame;
    try {
        second_frame = device.acquire_swapchain_frame(swapchain);
        second->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = mirakana::rhi::TextureHandle{},
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .swapchain_frame = second_frame,
                },
        });
    } catch (const std::invalid_argument&) {
        rejected_second_frame = true;
    }

    if (!rejected_second_frame) {
        second->end_render_pass();
        second->present(second_frame);
        second->close();
        first->close();
    } else {
        first->present(first_frame);
        first->close();
        (void)device.submit(*first);

        auto third = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
        const auto third_frame = device.acquire_swapchain_frame(swapchain);
        third->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = mirakana::rhi::TextureHandle{},
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .swapchain_frame = third_frame,
                },
        });
        third->end_render_pass();
        third->present(third_frame);
        third->close();
        (void)device.submit(*third);
    }

    MK_REQUIRE(rejected_second_frame);
    MK_REQUIRE(device.stats().present_calls == 2);
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 2);
    MK_REQUIRE(device.stats().swapchain_frames_released == 2);
}

MK_TEST("null rhi releases acquired swapchain frames before submit") {
    mirakana::rhi::NullRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 800, .height = 600},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .buffer_count = 2,
        .vsync = true,
    });

    const auto first_frame = device.acquire_swapchain_frame(swapchain);
    bool rejected_second_acquire = false;
    try {
        (void)device.acquire_swapchain_frame(swapchain);
    } catch (const std::invalid_argument&) {
        rejected_second_acquire = true;
    }

    device.release_swapchain_frame(first_frame);
    const auto second_frame = device.acquire_swapchain_frame(swapchain);
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = mirakana::rhi::TextureHandle{},
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = second_frame,
            },
    });
    commands->end_render_pass();
    commands->present(second_frame);
    commands->close();
    (void)device.submit(*commands);

    MK_REQUIRE(rejected_second_acquire);
    MK_REQUIRE(device.stats().swapchain_frames_acquired == 2);
    MK_REQUIRE(device.stats().swapchain_frames_released == 2);
}

MK_TEST("null rhi creates updates and binds descriptor sets") {
    mirakana::rhi::NullRhiDevice device;

    const auto uniform_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256,
        .usage = mirakana::rhi::BufferUsage::uniform,
    });
    const auto albedo = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 640, .height = 480, .depth = 1},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });

    const auto set_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::vertex | mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto descriptor_set = device.allocate_descriptor_set(set_layout);
    device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                uniform_buffer)},
    });
    device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 1,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 albedo)},
    });

    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto pipeline_layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {set_layout}, .push_constant_bytes = 64});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->bind_descriptor_set(pipeline_layout, 0, descriptor_set);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->close();

    (void)device.submit(*commands);
    MK_REQUIRE(device.stats().descriptor_set_layouts_created == 1);
    MK_REQUIRE(device.stats().descriptor_sets_allocated == 1);
    MK_REQUIRE(device.stats().descriptor_writes == 2);
    MK_REQUIRE(device.stats().descriptor_sets_bound == 1);
}

MK_TEST("null rhi creates sampler descriptors beside sampled texture descriptors") {
    mirakana::rhi::NullRhiDevice device;
    const auto albedo = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto sampler = device.create_sampler(mirakana::rhi::SamplerDesc{
        .min_filter = mirakana::rhi::SamplerFilter::linear,
        .mag_filter = mirakana::rhi::SamplerFilter::linear,
        .address_u = mirakana::rhi::SamplerAddressMode::repeat,
        .address_v = mirakana::rhi::SamplerAddressMode::repeat,
        .address_w = mirakana::rhi::SamplerAddressMode::repeat,
    });
    const auto set_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 1,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 16,
            .type = mirakana::rhi::DescriptorType::sampler,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto descriptor_set = device.allocate_descriptor_set(set_layout);

    device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 1,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 albedo)},
    });
    device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 16,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::sampler(sampler)},
    });

    bool rejected_unknown_sampler = false;
    try {
        device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = 16,
            .array_element = 0,
            .resources = {mirakana::rhi::DescriptorResource::sampler(mirakana::rhi::SamplerHandle{99})},
        });
    } catch (const std::invalid_argument&) {
        rejected_unknown_sampler = true;
    }

    const auto stats = device.stats();
    MK_REQUIRE(sampler.value == 1);
    MK_REQUIRE(stats.samplers_created == 1);
    MK_REQUIRE(stats.descriptor_set_layouts_created == 1);
    MK_REQUIRE(stats.descriptor_sets_allocated == 1);
    MK_REQUIRE(stats.descriptor_writes == 2);
    MK_REQUIRE(rejected_unknown_sampler);
}

MK_TEST("null rhi rejects unsupported depth texture usage combinations at creation") {
    mirakana::rhi::NullRhiDevice device;

    bool rejected_depth_copy_source = false;
    try {
        (void)device.create_texture(mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::depth24_stencil8,
            .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::copy_source,
        });
    } catch (const std::invalid_argument&) {
        rejected_depth_copy_source = true;
    }

    bool rejected_depth_storage = false;
    try {
        (void)device.create_texture(mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::depth24_stencil8,
            .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::storage,
        });
    } catch (const std::invalid_argument&) {
        rejected_depth_storage = true;
    }

    bool rejected_color_depth = false;
    try {
        (void)device.create_texture(mirakana::rhi::TextureDesc{
            .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
            .format = mirakana::rhi::Format::rgba8_unorm,
            .usage = mirakana::rhi::TextureUsage::depth_stencil,
        });
    } catch (const std::invalid_argument&) {
        rejected_color_depth = true;
    }

    MK_REQUIRE(rejected_depth_copy_source);
    MK_REQUIRE(rejected_depth_storage);
    MK_REQUIRE(rejected_color_depth);
}

MK_TEST("null rhi accepts sampled depth textures and preserves depth attachment state requirements") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto sampled_depth = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto depth_only = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto set_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{
            .binding = 0,
            .type = mirakana::rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = mirakana::rhi::ShaderStageVisibility::fragment,
        },
    }});
    const auto descriptor_set = device.allocate_descriptor_set(set_layout);

    device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
        .set = descriptor_set,
        .binding = 0,
        .array_element = 0,
        .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                 sampled_depth)},
    });

    bool rejected_depth_without_shader_resource = false;
    try {
        device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = 0,
            .array_element = 0,
            .resources = {mirakana::rhi::DescriptorResource::texture(mirakana::rhi::DescriptorType::sampled_texture,
                                                                     depth_only)},
        });
    } catch (const std::invalid_argument&) {
        rejected_depth_without_shader_resource = true;
    }

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->transition_texture(sampled_depth, mirakana::rhi::ResourceState::depth_write,
                                 mirakana::rhi::ResourceState::shader_read);

    bool rejected_shader_read_depth_attachment = false;
    try {
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = target,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = sampled_depth},
        });
    } catch (const std::invalid_argument&) {
        rejected_shader_read_depth_attachment = true;
    }
    if (!rejected_shader_read_depth_attachment) {
        commands->end_render_pass();
    }
    MK_REQUIRE(rejected_shader_read_depth_attachment);

    commands->transition_texture(sampled_depth, mirakana::rhi::ResourceState::shader_read,
                                 mirakana::rhi::ResourceState::depth_write);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
        .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = sampled_depth},
    });
    commands->end_render_pass();
    commands->close();
    (void)device.submit(*commands);

    MK_REQUIRE(rejected_depth_without_shader_resource);
    MK_REQUIRE(device.stats().descriptor_writes == 1);
    MK_REQUIRE(device.stats().resource_transitions == 2);
    MK_REQUIRE(device.stats().render_passes_begun == 1);
}

MK_TEST("null rhi rejects invalid descriptor layouts writes and incompatible bindings") {
    mirakana::rhi::NullRhiDevice device;

    bool rejected_duplicate_binding = false;
    try {
        (void)device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
            mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                                 .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                                 .count = 1,
                                                 .stages = mirakana::rhi::ShaderStageVisibility::vertex},
            mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                                 .type = mirakana::rhi::DescriptorType::sampled_texture,
                                                 .count = 1,
                                                 .stages = mirakana::rhi::ShaderStageVisibility::fragment},
        }});
    } catch (const std::invalid_argument&) {
        rejected_duplicate_binding = true;
    }

    bool rejected_empty_visibility = false;
    try {
        (void)device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
            mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                                 .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                                 .count = 1,
                                                 .stages = mirakana::rhi::ShaderStageVisibility::none},
        }});
    } catch (const std::invalid_argument&) {
        rejected_empty_visibility = true;
    }

    const auto uniform_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                             .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::vertex},
    }});
    const auto texture_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                             .type = mirakana::rhi::DescriptorType::sampled_texture,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::fragment},
    }});
    const auto descriptor_set = device.allocate_descriptor_set(uniform_layout);
    const auto vertex_only_buffer =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 128, .usage = mirakana::rhi::BufferUsage::vertex});

    bool rejected_wrong_usage = false;
    try {
        device.update_descriptor_set(mirakana::rhi::DescriptorWrite{
            .set = descriptor_set,
            .binding = 0,
            .array_element = 0,
            .resources = {mirakana::rhi::DescriptorResource::buffer(mirakana::rhi::DescriptorType::uniform_buffer,
                                                                    vertex_only_buffer)},
        });
    } catch (const std::invalid_argument&) {
        rejected_wrong_usage = true;
    }

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 320, .height = 180, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto pipeline_layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {texture_layout}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = pipeline_layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color = mirakana::rhi::RenderPassColorAttachment{.texture = target,
                                                          .load_action = mirakana::rhi::LoadAction::clear,
                                                          .store_action = mirakana::rhi::StoreAction::store},
    });
    commands->bind_graphics_pipeline(pipeline);

    bool rejected_incompatible_set = false;
    try {
        commands->bind_descriptor_set(pipeline_layout, 0, descriptor_set);
    } catch (const std::invalid_argument&) {
        rejected_incompatible_set = true;
    }

    MK_REQUIRE(rejected_duplicate_binding);
    MK_REQUIRE(rejected_empty_visibility);
    MK_REQUIRE(rejected_wrong_usage);
    MK_REQUIRE(rejected_incompatible_set);
}

MK_TEST("null rhi records render passes pipeline binding and draws") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 640, .height = 480, .depth = 1},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target | mirakana::rhi::TextureUsage::present,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->close();

    const auto fence = device.submit(*commands);
    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(device.stats().render_passes_begun == 1);
    MK_REQUIRE(device.stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(device.stats().draw_calls == 1);
    MK_REQUIRE(device.stats().vertices_submitted == 3);
}

MK_TEST("null rhi records render passes with depth attachments") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 640, .height = 480, .depth = 1},
        .format = mirakana::rhi::Format::bgra8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto depth = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 640, .height = 480, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    mirakana::rhi::GraphicsPipelineDesc pipeline_desc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::bgra8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto pipeline = device.create_graphics_pipeline(pipeline_desc);

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
        .depth =
            mirakana::rhi::RenderPassDepthAttachment{
                .texture = depth,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .clear_depth = mirakana::rhi::ClearDepthValue{0.5F},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->draw(3, 1);
    commands->end_render_pass();
    commands->close();

    const auto fence = device.submit(*commands);
    MK_REQUIRE(fence.value == 1);
    MK_REQUIRE(device.stats().render_passes_begun == 1);
    MK_REQUIRE(device.stats().graphics_pipelines_bound == 1);
    MK_REQUIRE(device.stats().draw_calls == 1);
}

MK_TEST("null rhi validates depth attachments and pipeline compatibility") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 320, .height = 180, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto depth = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 320, .height = 180, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto mismatched_extent_depth = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 64, .height = 64, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil,
    });
    const auto sampled_depth = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 320, .height = 180, .depth = 1},
        .format = mirakana::rhi::Format::depth24_stencil8,
        .usage = mirakana::rhi::TextureUsage::depth_stencil | mirakana::rhi::TextureUsage::shader_resource,
    });
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto no_depth_pipeline = device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
    mirakana::rhi::GraphicsPipelineDesc depth_pipeline_desc{
        .layout = layout,
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::depth24_stencil8,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    };
    depth_pipeline_desc.depth_state = mirakana::rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = mirakana::rhi::CompareOp::less_equal};
    const auto depth_pipeline = device.create_graphics_pipeline(depth_pipeline_desc);

    bool rejected_depth_state_without_format = false;
    try {
        mirakana::rhi::GraphicsPipelineDesc invalid_depth_state_desc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::rgba8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        };
        invalid_depth_state_desc.depth_state =
            mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                 .depth_write_enabled = true,
                                                 .depth_compare = mirakana::rhi::CompareOp::less_equal};
        (void)device.create_graphics_pipeline(invalid_depth_state_desc);
    } catch (const std::invalid_argument&) {
        rejected_depth_state_without_format = true;
    }

    bool rejected_depth_write_without_test = false;
    try {
        mirakana::rhi::GraphicsPipelineDesc invalid_depth_write_desc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::rgba8_unorm,
            .depth_format = mirakana::rhi::Format::depth24_stencil8,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        };
        invalid_depth_write_desc.depth_state =
            mirakana::rhi::DepthStencilStateDesc{.depth_test_enabled = false,
                                                 .depth_write_enabled = true,
                                                 .depth_compare = mirakana::rhi::CompareOp::less_equal};
        (void)device.create_graphics_pipeline(invalid_depth_write_desc);
    } catch (const std::invalid_argument&) {
        rejected_depth_write_without_test = true;
    }

    const auto color = mirakana::rhi::RenderPassColorAttachment{
        .texture = target,
        .load_action = mirakana::rhi::LoadAction::clear,
        .store_action = mirakana::rhi::StoreAction::store,
    };

    bool rejected_unknown_depth_handle = false;
    try {
        auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color = color,
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = mirakana::rhi::TextureHandle{999}},
        });
    } catch (const std::invalid_argument&) {
        rejected_unknown_depth_handle = true;
    }

    bool rejected_extent_mismatch = false;
    try {
        auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color = color,
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = mismatched_extent_depth},
        });
    } catch (const std::invalid_argument&) {
        rejected_extent_mismatch = true;
    }

    bool rejected_clear_depth = false;
    try {
        auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color = color,
            .depth =
                mirakana::rhi::RenderPassDepthAttachment{
                    .texture = depth,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                    .clear_depth = mirakana::rhi::ClearDepthValue{1.5F},
                },
        });
    } catch (const std::invalid_argument&) {
        rejected_clear_depth = true;
    }

    bool rejected_no_depth_pipeline = false;
    try {
        auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color = color,
            .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = depth},
        });
        commands->bind_graphics_pipeline(no_depth_pipeline);
    } catch (const std::invalid_argument&) {
        rejected_no_depth_pipeline = true;
    }

    bool rejected_depth_pipeline_without_attachment = false;
    try {
        auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{.color = color});
        commands->bind_graphics_pipeline(depth_pipeline);
    } catch (const std::invalid_argument&) {
        rejected_depth_pipeline_without_attachment = true;
    }

    MK_REQUIRE(rejected_unknown_depth_handle);
    MK_REQUIRE(rejected_extent_mismatch);
    MK_REQUIRE(rejected_clear_depth);
    MK_REQUIRE(rejected_depth_state_without_format);
    MK_REQUIRE(rejected_depth_write_without_test);
    MK_REQUIRE(rejected_no_depth_pipeline);
    MK_REQUIRE(rejected_depth_pipeline_without_attachment);

    const auto render_passes_before_sampled_depth = device.stats().render_passes_begun;
    auto sampled_depth_commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    sampled_depth_commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color = color,
        .depth = mirakana::rhi::RenderPassDepthAttachment{.texture = sampled_depth},
    });
    sampled_depth_commands->bind_graphics_pipeline(depth_pipeline);
    sampled_depth_commands->end_render_pass();
    sampled_depth_commands->close();
    (void)device.submit(*sampled_depth_commands);
    MK_REQUIRE(device.stats().render_passes_begun == render_passes_before_sampled_depth + 1);
}

MK_TEST("null rhi binds vertex and index buffers before indexed draw") {
    mirakana::rhi::NullRhiDevice device;
    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 32, .height = 32, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    const auto vertices =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 96, .usage = mirakana::rhi::BufferUsage::vertex});
    const auto indices =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 12, .usage = mirakana::rhi::BufferUsage::index});
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

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
                .swapchain_frame = {},
                .clear_color = mirakana::rhi::ClearColorValue{},
            },
    });
    commands->bind_graphics_pipeline(pipeline);
    commands->bind_vertex_buffer(mirakana::rhi::VertexBufferBinding{.buffer = vertices, .offset = 0, .stride = 32});
    commands->bind_index_buffer(mirakana::rhi::IndexBufferBinding{
        .buffer = indices, .offset = 0, .format = mirakana::rhi::IndexFormat::uint32});
    commands->draw_indexed(3, 1);
    commands->end_render_pass();
    commands->close();
    (void)device.submit(*commands);

    const auto stats = device.stats();
    MK_REQUIRE(stats.vertex_buffer_bindings == 1);
    MK_REQUIRE(stats.index_buffer_bindings == 1);
    MK_REQUIRE(stats.indexed_draw_calls == 1);
    MK_REQUIRE(stats.draw_calls == 1);
    MK_REQUIRE(stats.indices_submitted == 3);
}

MK_TEST("null rhi rejects invalid shader and pipeline descriptions") {
    mirakana::rhi::NullRhiDevice device;

    bool rejected_shader = false;
    try {
        (void)device.create_shader(mirakana::rhi::ShaderDesc{
            .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "", .bytecode_size = 64});
    } catch (const std::invalid_argument&) {
        rejected_shader = true;
    }

    bool rejected_pipeline = false;
    try {
        (void)device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = mirakana::rhi::PipelineLayoutHandle{},
            .vertex_shader = mirakana::rhi::ShaderHandle{},
            .fragment_shader = mirakana::rhi::ShaderHandle{},
            .color_format = mirakana::rhi::Format::unknown,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    } catch (const std::invalid_argument&) {
        rejected_pipeline = true;
    }

    MK_REQUIRE(rejected_shader);
    MK_REQUIRE(rejected_pipeline);
}

MK_TEST("null rhi rejects unknown texture and pipeline handles during recording") {
    mirakana::rhi::NullRhiDevice device;
    const auto shader_resource = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 320, .height = 180, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::shader_resource,
    });

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    bool rejected_transition = false;
    try {
        commands->transition_texture(mirakana::rhi::TextureHandle{99}, mirakana::rhi::ResourceState::undefined,
                                     mirakana::rhi::ResourceState::render_target);
    } catch (const std::invalid_argument&) {
        rejected_transition = true;
    }

    bool rejected_attachment_usage = false;
    try {
        commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
            .color =
                mirakana::rhi::RenderPassColorAttachment{
                    .texture = shader_resource,
                    .load_action = mirakana::rhi::LoadAction::clear,
                    .store_action = mirakana::rhi::StoreAction::store,
                },
        });
    } catch (const std::invalid_argument&) {
        rejected_attachment_usage = true;
    }

    const auto target = device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 320, .height = 180, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });
    commands->begin_render_pass(mirakana::rhi::RenderPassDesc{
        .color =
            mirakana::rhi::RenderPassColorAttachment{
                .texture = target,
                .load_action = mirakana::rhi::LoadAction::clear,
                .store_action = mirakana::rhi::StoreAction::store,
            },
    });

    bool rejected_pipeline = false;
    try {
        commands->bind_graphics_pipeline(mirakana::rhi::GraphicsPipelineHandle{99});
    } catch (const std::invalid_argument&) {
        rejected_pipeline = true;
    }

    MK_REQUIRE(rejected_transition);
    MK_REQUIRE(rejected_attachment_usage);
    MK_REQUIRE(rejected_pipeline);
}

MK_TEST("null rhi validates graphics pipeline shader stages and handle ownership") {
    mirakana::rhi::NullRhiDevice device;
    const auto layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
    const auto vertex_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::vertex, .entry_point = "vs_main", .bytecode_size = 64});
    const auto fragment_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::fragment, .entry_point = "fs_main", .bytecode_size = 64});
    const auto compute_shader = device.create_shader(mirakana::rhi::ShaderDesc{
        .stage = mirakana::rhi::ShaderStage::compute, .entry_point = "cs_main", .bytecode_size = 64});

    bool rejected_unknown_layout = false;
    try {
        (void)device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = mirakana::rhi::PipelineLayoutHandle{99},
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::rgba8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    } catch (const std::invalid_argument&) {
        rejected_unknown_layout = true;
    }

    bool rejected_unknown_shader = false;
    try {
        (void)device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = layout,
            .vertex_shader = mirakana::rhi::ShaderHandle{99},
            .fragment_shader = fragment_shader,
            .color_format = mirakana::rhi::Format::rgba8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    } catch (const std::invalid_argument&) {
        rejected_unknown_shader = true;
    }

    bool rejected_stage_mismatch = false;
    try {
        (void)device.create_graphics_pipeline(mirakana::rhi::GraphicsPipelineDesc{
            .layout = layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = compute_shader,
            .color_format = mirakana::rhi::Format::rgba8_unorm,
            .depth_format = mirakana::rhi::Format::unknown,
            .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
        });
    } catch (const std::invalid_argument&) {
        rejected_stage_mismatch = true;
    }

    MK_REQUIRE(rejected_unknown_layout);
    MK_REQUIRE(rejected_unknown_shader);
    MK_REQUIRE(rejected_stage_mismatch);
}

MK_TEST("null rhi gpu debug scopes and markers update stats and enforce balance on close") {
    mirakana::rhi::NullRhiDevice device;
    auto list = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    list->begin_gpu_debug_scope("outer");
    list->insert_gpu_debug_marker("mid");
    list->begin_gpu_debug_scope("inner");
    list->end_gpu_debug_scope();
    list->end_gpu_debug_scope();
    list->close();
    [[maybe_unused]] const auto submitted_fence = device.submit(*list);
    (void)submitted_fence;

    const auto stats = device.stats();
    MK_REQUIRE(stats.gpu_debug_scopes_begun == 2);
    MK_REQUIRE(stats.gpu_debug_scopes_ended == 2);
    MK_REQUIRE(stats.gpu_debug_markers_inserted == 1);
    MK_REQUIRE(device.gpu_timestamp_ticks_per_second() == 0);
}

MK_TEST("null rhi rejects invalid gpu debug labels") {
    mirakana::rhi::NullRhiDevice device;
    auto list = device.begin_command_list(mirakana::rhi::QueueKind::graphics);

    bool rejected_empty = false;
    try {
        list->begin_gpu_debug_scope("");
    } catch (const std::invalid_argument&) {
        rejected_empty = true;
    }
    MK_REQUIRE(rejected_empty);

    bool rejected_non_ascii = false;
    try {
        std::string non_ascii("bad-");
        non_ascii.push_back(static_cast<char>(0xff));
        non_ascii.append("-scope");
        list->begin_gpu_debug_scope(non_ascii);
    } catch (const std::invalid_argument&) {
        rejected_non_ascii = true;
    }
    MK_REQUIRE(rejected_non_ascii);
}

MK_TEST("null rhi command list close rejects unbalanced gpu debug scopes") {
    mirakana::rhi::NullRhiDevice device;
    auto list = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    list->begin_gpu_debug_scope("open");
    bool rejected = false;
    try {
        list->close();
    } catch (const std::logic_error&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("null rhi memory diagnostics sums active buffer and texture descriptor bytes") {
    mirakana::rhi::NullRhiDevice device;
    (void)device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 512,
        .usage = mirakana::rhi::BufferUsage::vertex,
    });
    (void)device.create_texture(mirakana::rhi::TextureDesc{
        .extent = mirakana::rhi::Extent3D{.width = 2, .height = 2, .depth = 1},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .usage = mirakana::rhi::TextureUsage::render_target,
    });

    const auto mem = device.memory_diagnostics();
    MK_REQUIRE(mem.committed_resources_byte_estimate_available);
    MK_REQUIRE(mem.committed_resources_byte_estimate == 512U + 16U);
    MK_REQUIRE(!mem.os_video_memory_budget_available);
}

MK_TEST("rhi debug label helper accepts printable ascii") {
    MK_REQUIRE(mirakana::rhi::is_valid_rhi_debug_label("pass-01"));
    MK_REQUIRE(!mirakana::rhi::is_valid_rhi_debug_label(""));
    MK_REQUIRE(!mirakana::rhi::is_valid_rhi_debug_label("\tbad"));
}

int main() {
    return mirakana::test::run_all();
}
