// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/core/application.hpp"
#include "mirakana/core/diagnostics.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/window.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime_host/desktop_runner.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

class CountingDesktopApp final : public mirakana::GameApp {
  public:
    explicit CountingDesktopApp(mirakana::IRenderer* renderer = nullptr) : renderer_(renderer) {}

    void on_start(mirakana::EngineContext& context) override {
        started = true;
        context.logger.write(
            mirakana::LogRecord{.level = mirakana::LogLevel::info, .category = "desktop-test", .message = "start"});
    }

    bool on_update(mirakana::EngineContext& /*context*/, double delta_seconds) override {
        MK_REQUIRE(std::abs(delta_seconds - expected_delta) < 0.0001);
        if (renderer_ != nullptr) {
            const auto extent = renderer_->backbuffer_extent();
            MK_REQUIRE(extent.width == expected_renderer_width);
            MK_REQUIRE(extent.height == expected_renderer_height);
        }
        ++updates;
        return updates < stop_after_updates;
    }

    void on_stop(mirakana::EngineContext& /*context*/) override {
        stopped = true;
    }

    mirakana::IRenderer* renderer_{nullptr};
    double expected_delta{1.0 / 30.0};
    std::uint32_t expected_renderer_width{320};
    std::uint32_t expected_renderer_height{180};
    int stop_after_updates{3};
    bool started{false};
    bool stopped{false};
    int updates{0};
};

class RenderingDesktopApp final : public mirakana::GameApp {
  public:
    RenderingDesktopApp(mirakana::IRenderer& renderer, mirakana::ManualProfileClock& clock)
        : renderer_(renderer), clock_(clock) {}

    bool on_update(mirakana::EngineContext& /*context*/, double delta_seconds) override {
        MK_REQUIRE(std::abs(delta_seconds - (1.0 / 30.0)) < 0.0001);

        renderer_.begin_frame();
        clock_.advance(250);
        renderer_.draw_sprite(
            mirakana::SpriteCommand{.transform = mirakana::Transform2D{},
                                    .color = mirakana::Color{.r = 1.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F}});
        renderer_.draw_mesh(
            mirakana::MeshCommand{.transform = mirakana::Transform3D{},
                                  .color = mirakana::Color{.r = 0.0F, .g = 1.0F, .b = 0.0F, .a = 1.0F}});
        renderer_.end_frame();
        clock_.advance(750);

        ++updates;
        return false;
    }

    int updates{0};

  private:
    mirakana::IRenderer& renderer_;
    mirakana::ManualProfileClock& clock_;
};

[[nodiscard]] const mirakana::CounterSample* counter_named(std::span<const mirakana::CounterSample> counters,
                                                           std::string_view name) noexcept {
    for (const auto& counter : counters) {
        if (counter.name == name) {
            return &counter;
        }
    }
    return nullptr;
}

class CloseWindowPump final : public mirakana::IDesktopEventPump {
  public:
    void pump_events(mirakana::DesktopHostServices& services) override {
        ++calls;
        services.window->request_close();
    }

    int calls{0};
};

class QuitLifecyclePump final : public mirakana::IDesktopEventPump {
  public:
    void pump_events(mirakana::DesktopHostServices& services) override {
        ++calls;
        services.lifecycle->push(mirakana::LifecycleEventKind::quit_requested);
    }

    int calls{0};
};

class ResizeRecordingRhiDevice final : public mirakana::rhi::IRhiDevice {
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
        last_resize_extent = extent;
        ++resize_calls;
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
    mirakana::rhi::Extent2D last_resize_extent{};
    std::uint64_t resize_calls{0};
};

class ThrowingReadFileSystem final : public mirakana::IFileSystem {
  public:
    [[nodiscard]] bool exists(std::string_view /*path*/) const override {
        return true;
    }

    [[nodiscard]] bool is_directory(std::string_view /*path*/) const override {
        return false;
    }

    [[nodiscard]] std::string read_text(std::string_view path) const override {
        throw std::runtime_error("read denied: " + std::string{path});
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view /*root*/) const override {
        return {};
    }

    void write_text(std::string_view /*path*/, std::string_view /*text*/) override {}
    void remove(std::string_view /*path*/) override {}
    void remove_empty_directory(std::string_view /*path*/) override {}
};

[[nodiscard]] mirakana::rhi::GraphicsPipelineHandle create_test_pipeline(mirakana::rhi::IRhiDevice& device) {
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
        .color_format = mirakana::rhi::Format::rgba8_unorm,
        .depth_format = mirakana::rhi::Format::unknown,
        .topology = mirakana::rhi::PrimitiveTopology::triangle_list,
    });
}

} // namespace

MK_TEST("desktop game runner drives lifecycle until app stops") {
    mirakana::RingBufferLogger logger(8);
    mirakana::Registry registry;
    mirakana::HeadlessWindow window(
        mirakana::WindowDesc{.title = "Desktop Test", .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    mirakana::VirtualInput input;
    mirakana::VirtualPointerInput pointer_input;
    mirakana::VirtualGamepadInput gamepad_input;
    mirakana::VirtualLifecycle lifecycle;
    mirakana::DesktopHostServices services{
        .window = &window,
        .renderer = &renderer,
        .input = &input,
        .pointer_input = &pointer_input,
        .gamepad_input = &gamepad_input,
        .lifecycle = &lifecycle,
    };
    mirakana::DesktopGameRunner runner(logger, registry);
    CountingDesktopApp app(&renderer);

    const auto result =
        runner.run(app, services, mirakana::DesktopRunConfig{.max_frames = 10, .fixed_delta_seconds = 1.0 / 30.0});

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::stopped_by_app);
    MK_REQUIRE(result.frames_run == 3);
    MK_REQUIRE(app.started);
    MK_REQUIRE(app.stopped);
    MK_REQUIRE(app.updates == 3);
    MK_REQUIRE(logger.records().size() == 1);
    MK_REQUIRE(window.is_open());
}

MK_TEST("desktop game runner stops before update when the window closes") {
    mirakana::RingBufferLogger logger(8);
    mirakana::Registry registry;
    mirakana::HeadlessWindow window(
        mirakana::WindowDesc{.title = "Desktop Test", .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    mirakana::VirtualLifecycle lifecycle;
    mirakana::DesktopHostServices services{
        .window = &window,
        .renderer = &renderer,
        .lifecycle = &lifecycle,
    };
    mirakana::DesktopGameRunner runner(logger, registry);
    CountingDesktopApp app;
    CloseWindowPump pump;

    const auto result = runner.run(
        app, services, mirakana::DesktopRunConfig{.max_frames = 10, .fixed_delta_seconds = 1.0 / 30.0}, &pump);

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::window_closed);
    MK_REQUIRE(result.frames_run == 0);
    MK_REQUIRE(pump.calls == 1);
    MK_REQUIRE(app.started);
    MK_REQUIRE(app.stopped);
    MK_REQUIRE(app.updates == 0);
}

MK_TEST("desktop game runner stops before update when lifecycle requests quit") {
    mirakana::RingBufferLogger logger(8);
    mirakana::Registry registry;
    mirakana::HeadlessWindow window(
        mirakana::WindowDesc{.title = "Desktop Test", .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    mirakana::VirtualLifecycle lifecycle;
    mirakana::DesktopHostServices services{
        .window = &window,
        .renderer = &renderer,
        .lifecycle = &lifecycle,
    };
    mirakana::DesktopGameRunner runner(logger, registry);
    CountingDesktopApp app;
    QuitLifecyclePump pump;

    const auto result = runner.run(
        app, services, mirakana::DesktopRunConfig{.max_frames = 10, .fixed_delta_seconds = 1.0 / 30.0}, &pump);

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::lifecycle_quit);
    MK_REQUIRE(result.frames_run == 0);
    MK_REQUIRE(pump.calls == 1);
    MK_REQUIRE(lifecycle.state().quit_requested);
    MK_REQUIRE(app.started);
    MK_REQUIRE(app.stopped);
    MK_REQUIRE(app.updates == 0);
}

MK_TEST("desktop game runner resizes renderer to the window extent") {
    mirakana::RingBufferLogger logger(8);
    mirakana::Registry registry;
    mirakana::HeadlessWindow window(
        mirakana::WindowDesc{.title = "Desktop Test", .extent = mirakana::WindowExtent{.width = 640, .height = 360}});
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    mirakana::VirtualLifecycle lifecycle;
    mirakana::DesktopHostServices services{
        .window = &window,
        .renderer = &renderer,
        .lifecycle = &lifecycle,
    };
    mirakana::DesktopGameRunner runner(logger, registry);
    CountingDesktopApp app(&renderer);
    app.expected_renderer_width = 640;
    app.expected_renderer_height = 360;
    app.stop_after_updates = 1;

    const auto result =
        runner.run(app, services, mirakana::DesktopRunConfig{.max_frames = 2, .fixed_delta_seconds = 1.0 / 30.0});

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::stopped_by_app);
    MK_REQUIRE(result.frames_run == 1);
    MK_REQUIRE(renderer.backbuffer_extent().width == 640);
    MK_REQUIRE(renderer.backbuffer_extent().height == 360);
}

MK_TEST("desktop game runner resizes an rhi swapchain renderer to the window extent") {
    mirakana::RingBufferLogger logger(8);
    mirakana::Registry registry;
    mirakana::HeadlessWindow window(
        mirakana::WindowDesc{.title = "Desktop Test", .extent = mirakana::WindowExtent{.width = 800, .height = 450}});
    mirakana::VirtualLifecycle lifecycle;
    ResizeRecordingRhiDevice device;
    const auto swapchain = device.create_swapchain(mirakana::rhi::SwapchainDesc{
        .extent = mirakana::rhi::Extent2D{.width = 320, .height = 180},
        .format = mirakana::rhi::Format::rgba8_unorm,
        .buffer_count = 2,
        .vsync = true,
        .surface = mirakana::rhi::SurfaceHandle{1},
    });
    const auto pipeline = create_test_pipeline(device);
    mirakana::RhiFrameRenderer renderer(mirakana::RhiFrameRendererDesc{
        .device = &device,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .color_texture = mirakana::rhi::TextureHandle{},
        .swapchain = swapchain,
        .graphics_pipeline = pipeline,
        .wait_for_completion = true,
    });
    mirakana::DesktopHostServices services{
        .window = &window,
        .renderer = &renderer,
        .lifecycle = &lifecycle,
    };
    mirakana::DesktopGameRunner runner(logger, registry);
    CountingDesktopApp app(&renderer);
    app.expected_renderer_width = 800;
    app.expected_renderer_height = 450;
    app.stop_after_updates = 1;

    const auto result =
        runner.run(app, services, mirakana::DesktopRunConfig{.max_frames = 2, .fixed_delta_seconds = 1.0 / 30.0});

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::stopped_by_app);
    MK_REQUIRE(result.frames_run == 1);
    MK_REQUIRE(renderer.backbuffer_extent().width == 800);
    MK_REQUIRE(renderer.backbuffer_extent().height == 450);
    MK_REQUIRE(device.resize_calls == 1);
    MK_REQUIRE(device.last_resize_extent.width == 800);
    MK_REQUIRE(device.last_resize_extent.height == 450);
    MK_REQUIRE(device.stats().swapchain_resizes == 1);
}

MK_TEST("desktop game runner records frame diagnostics and renderer counters") {
    mirakana::RingBufferLogger logger(8);
    mirakana::Registry registry;
    mirakana::HeadlessWindow window(
        mirakana::WindowDesc{.title = "Desktop Test", .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 180});
    mirakana::VirtualLifecycle lifecycle;
    mirakana::DiagnosticsRecorder recorder(16);
    mirakana::ManualProfileClock clock(1000);
    mirakana::DesktopHostServices services{
        .window = &window,
        .renderer = &renderer,
        .lifecycle = &lifecycle,
        .diagnostics_recorder = &recorder,
        .profile_clock = &clock,
    };
    mirakana::DesktopGameRunner runner(logger, registry);
    RenderingDesktopApp app(renderer, clock);

    const auto result =
        runner.run(app, services, mirakana::DesktopRunConfig{.max_frames = 4, .fixed_delta_seconds = 1.0 / 30.0});

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::stopped_by_app);
    MK_REQUIRE(result.frames_run == 1);
    MK_REQUIRE(app.updates == 1);

    const auto capture = recorder.snapshot();
    MK_REQUIRE(capture.profiles.size() == 1);
    MK_REQUIRE(capture.profiles[0].name == "runtime_host.frame");
    MK_REQUIRE(capture.profiles[0].frame_index == 0);
    MK_REQUIRE(capture.profiles[0].start_time_ns == 1000);
    MK_REQUIRE(capture.profiles[0].duration_ns == 1000);
    MK_REQUIRE(capture.profiles[0].depth == 0);

    MK_REQUIRE(capture.counters.size() == 4);
    const auto* frames_started = counter_named(capture.counters, "renderer.frames_started");
    const auto* frames_finished = counter_named(capture.counters, "renderer.frames_finished");
    const auto* sprites_submitted = counter_named(capture.counters, "renderer.sprites_submitted");
    const auto* meshes_submitted = counter_named(capture.counters, "renderer.meshes_submitted");
    MK_REQUIRE(frames_started != nullptr);
    MK_REQUIRE(frames_finished != nullptr);
    MK_REQUIRE(sprites_submitted != nullptr);
    MK_REQUIRE(meshes_submitted != nullptr);
    MK_REQUIRE(frames_started->frame_index == 0);
    MK_REQUIRE(frames_started->value == 1.0);
    MK_REQUIRE(frames_finished->value == 1.0);
    MK_REQUIRE(sprites_submitted->value == 1.0);
    MK_REQUIRE(meshes_submitted->value == 1.0);
}

MK_TEST("desktop shader bytecode loader preserves packaged binary bytes") {
    mirakana::MemoryFileSystem filesystem;
    const std::string vertex_bytes{"vs\0\x7f\xff", 5};
    const std::string fragment_bytes{"ps\0\x01", 4};
    filesystem.write_text("shaders/runtime_shell.vs.dxil", vertex_bytes);
    filesystem.write_text("shaders/runtime_shell.ps.dxil", fragment_bytes);

    const auto loaded = mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = "shaders/runtime_shell.vs.dxil",
        .fragment_path = "shaders/runtime_shell.ps.dxil",
    });

    MK_REQUIRE(loaded.ready());
    MK_REQUIRE(loaded.status == mirakana::DesktopShaderBytecodeLoadStatus::ready);
    MK_REQUIRE(mirakana::desktop_shader_bytecode_load_status_name(loaded.status) == std::string_view{"ready"});
    MK_REQUIRE(loaded.vertex_shader.entry_point == "vs_main");
    MK_REQUIRE(loaded.fragment_shader.entry_point == "ps_main");
    MK_REQUIRE(loaded.vertex_shader.bytecode == std::vector<std::uint8_t>({'v', 's', 0, 0x7f, 0xff}));
    MK_REQUIRE(loaded.fragment_shader.bytecode == std::vector<std::uint8_t>({'p', 's', 0, 0x01}));
}

MK_TEST("desktop shader bytecode loader reports missing and empty artifacts") {
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_text("shaders/empty.vs.dxil", "");
    filesystem.write_text("shaders/runtime_shell.ps.dxil", std::string{"ps", 2});

    const auto missing = mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = "shaders/missing.vs.dxil",
        .fragment_path = "shaders/runtime_shell.ps.dxil",
    });
    MK_REQUIRE(!missing.ready());
    MK_REQUIRE(missing.status == mirakana::DesktopShaderBytecodeLoadStatus::missing);
    MK_REQUIRE(mirakana::desktop_shader_bytecode_load_status_name(missing.status) == std::string_view{"missing"});
    MK_REQUIRE(missing.diagnostic.find("missing.vs.dxil") != std::string::npos);

    const auto empty = mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = "shaders/empty.vs.dxil",
        .fragment_path = "shaders/runtime_shell.ps.dxil",
    });
    MK_REQUIRE(!empty.ready());
    MK_REQUIRE(empty.status == mirakana::DesktopShaderBytecodeLoadStatus::empty);
    MK_REQUIRE(mirakana::desktop_shader_bytecode_load_status_name(empty.status) == std::string_view{"empty"});
    MK_REQUIRE(empty.diagnostic.find("empty.vs.dxil") != std::string::npos);
}

MK_TEST("desktop shader bytecode loader rejects invalid requests") {
    mirakana::MemoryFileSystem filesystem;
    filesystem.write_text("shaders/runtime_shell.vs.dxil", std::string{"vs", 2});
    filesystem.write_text("shaders/runtime_shell.ps.dxil", std::string{"ps", 2});

    const auto no_filesystem = mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .vertex_path = "shaders/runtime_shell.vs.dxil",
        .fragment_path = "shaders/runtime_shell.ps.dxil",
    });
    MK_REQUIRE(!no_filesystem.ready());
    MK_REQUIRE(no_filesystem.status == mirakana::DesktopShaderBytecodeLoadStatus::invalid_request);
    MK_REQUIRE(mirakana::desktop_shader_bytecode_load_status_name(no_filesystem.status) ==
               std::string_view{"invalid_request"});

    const auto empty_entry = mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = "shaders/runtime_shell.vs.dxil",
        .fragment_path = "shaders/runtime_shell.ps.dxil",
        .vertex_entry_point = "",
    });
    MK_REQUIRE(!empty_entry.ready());
    MK_REQUIRE(empty_entry.status == mirakana::DesktopShaderBytecodeLoadStatus::invalid_request);
}

MK_TEST("desktop shader bytecode loader reports filesystem read failures") {
    ThrowingReadFileSystem filesystem;

    const auto loaded = mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = "shaders/runtime_shell.vs.dxil",
        .fragment_path = "shaders/runtime_shell.ps.dxil",
    });

    MK_REQUIRE(!loaded.ready());
    MK_REQUIRE(loaded.status == mirakana::DesktopShaderBytecodeLoadStatus::read_failed);
    MK_REQUIRE(mirakana::desktop_shader_bytecode_load_status_name(loaded.status) == std::string_view{"read_failed"});
    MK_REQUIRE(loaded.diagnostic.find("runtime_shell.vs.dxil") != std::string::npos);
    MK_REQUIRE(loaded.diagnostic.find("read denied") != std::string::npos);
}

int main() {
    return mirakana::test::run_all();
}
