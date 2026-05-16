// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/core/application.hpp"
#include "mirakana/core/log.hpp"
#include "mirakana/core/registry.hpp"
#include "mirakana/platform/lifecycle.hpp"
#include "mirakana/platform/sdl3/sdl_window.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_host/desktop_runner.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_event_pump.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "scene_gpu_binding_injecting_renderer.hpp"

#include <SDL3/SDL_events.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3dcompiler.h>
#include <wrl/client.h>
#endif

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

class OneFrameDesktopApp final : public mirakana::GameApp {
  public:
    bool on_update(mirakana::EngineContext&, double) override {
        ++updates;
        return true;
    }

    int updates{0};
};

class SdlDesktopGameHostProbeApp final : public mirakana::GameApp {
  public:
    SdlDesktopGameHostProbeApp(mirakana::VirtualInput& input, mirakana::IRenderer& renderer)
        : input_(input), renderer_(renderer) {}

    void on_start(mirakana::EngineContext&) override {
        renderer_.set_clear_color(mirakana::Color{.r = 0.01F, .g = 0.02F, .b = 0.03F, .a = 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        renderer_.begin_frame();
        const auto axis =
            input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
        transform_.position = transform_.position + axis;
        renderer_.draw_sprite(mirakana::SpriteCommand{
            .transform = transform_, .color = mirakana::Color{.r = 0.2F, .g = 0.7F, .b = 1.0F, .a = 1.0F}});
        renderer_.end_frame();
        ++updates;
        return true;
    }

    int updates{0};

  private:
    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::Transform2D transform_;
};

#if defined(_WIN32)
[[nodiscard]] bool d3d12_presentation_test_enabled() noexcept {
    return GetEnvironmentVariableA("MK_ENABLE_SDL3_D3D12_PRESENTATION_TEST", nullptr, 0) > 0;
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(const char* source, const char* entry_point,
                                                              const char* target) {
    Microsoft::WRL::ComPtr<ID3DBlob> bytecode;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry_point, target,
                                      D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &errors);
    MK_REQUIRE(SUCCEEDED(result));
    return bytecode;
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_triangle_vertex_shader() {
    return compile_shader("struct VsOut {"
                          "  float4 position : SV_Position;"
                          "};"
                          "VsOut vs_main(uint vertex_id : SV_VertexID) {"
                          "  float2 positions[3] = { float2(0.0, 0.5), float2(0.5, -0.5), float2(-0.5, -0.5) };"
                          "  VsOut output;"
                          "  output.position = float4(positions[vertex_id], 0.0, 1.0);"
                          "  return output;"
                          "}",
                          "vs_main", "vs_5_0");
}

[[nodiscard]] Microsoft::WRL::ComPtr<ID3DBlob> compile_triangle_pixel_shader() {
    return compile_shader("float4 ps_main(float4 position : SV_Position) : SV_Target {"
                          "  return float4(0.2, 0.7, 1.0, 1.0);"
                          "}",
                          "ps_main", "ps_5_0");
}
#endif

[[nodiscard]] mirakana::SceneRenderPacket make_probe_scene_packet() {
    mirakana::Scene scene("PresentationSceneGpuProbe");
    const auto node = scene.create_node("Mesh");
    mirakana::SceneNodeComponents components;
    components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/probe"),
                                        .material = mirakana::AssetId::from_name("materials/probe"),
                                        .visible = true};
    scene.set_components(node, components);
    return mirakana::build_scene_render_packet(scene);
}

[[nodiscard]] mirakana::SceneRenderPacket make_probe_shadow_scene_packet() {
    mirakana::Scene scene("PresentationDirectionalShadowProbe");
    const auto mesh_node = scene.create_node("Mesh");
    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer =
        mirakana::MeshRendererComponent{.mesh = mirakana::AssetId::from_name("meshes/probe"),
                                        .material = mirakana::AssetId::from_name("materials/probe"),
                                        .visible = true};
    scene.set_components(mesh_node, mesh_components);

    const auto light_node = scene.create_node("DirectionalLight");
    mirakana::SceneNodeComponents light_components;
    light_components.light = mirakana::LightComponent{
        .type = mirakana::LightType::directional,
        .color = mirakana::Vec3{.x = 1.0F, .y = 0.95F, .z = 0.8F},
        .intensity = 2.0F,
        .range = 100.0F,
        .inner_cone_radians = 0.0F,
        .outer_cone_radians = 0.0F,
        .casts_shadows = true,
    };
    scene.set_components(light_node, light_components);

    return mirakana::build_scene_render_packet(scene);
}

[[nodiscard]] std::string repeated_hex_bytes(std::size_t count, std::string_view byte) {
    std::string encoded;
    encoded.reserve(count * 2U);
    for (std::size_t index = 0; index < count; ++index) {
        encoded.append(byte);
    }
    return encoded;
}

[[nodiscard]] std::string probe_mesh_payload(mirakana::AssetId mesh, bool lit_layout) {
    return "format=GameEngine.CookedMesh.v2\n"
           "asset.id=" +
           std::to_string(mesh.value) +
           "\n"
           "asset.kind=mesh\n"
           "mesh.vertex_count=3\n"
           "mesh.index_count=3\n"
           "mesh.has_normals=" +
           std::string{lit_layout ? "true" : "false"} +
           "\n"
           "mesh.has_uvs=" +
           std::string{lit_layout ? "true" : "false"} +
           "\n"
           "mesh.has_tangent_frame=" +
           std::string{lit_layout ? "true" : "false"} +
           "\n"
           "mesh.vertex_data_hex=" +
           repeated_hex_bytes(lit_layout ? 144U : 36U, "3f") +
           "\n"
           "mesh.index_data_hex=000000000100000002000000\n";
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_probe_package(mirakana::AssetId mesh,
                                                                        bool lit_layout = false) {
    return mirakana::runtime::RuntimeAssetPackage(std::vector<mirakana::runtime::RuntimeAssetRecord>{
        mirakana::runtime::RuntimeAssetRecord{
            .handle = mirakana::runtime::RuntimeAssetHandle{1},
            .asset = mesh,
            .kind = mirakana::AssetKind::mesh,
            .path = lit_layout ? "assets/meshes/probe-lit.mesh" : "assets/meshes/probe.mesh",
            .content_hash = 1,
            .source_revision = 1,
            .dependencies = {},
            .content = probe_mesh_payload(mesh, lit_layout),
        },
    });
}

[[nodiscard]] mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingResult
make_probe_scene_gpu_bindings(mirakana::rhi::IRhiDevice& device, mirakana::AssetId mesh, mirakana::AssetId material) {
    mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingResult result;
    const auto vertex_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::copy_destination});
    const auto index_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 12, .usage = mirakana::rhi::BufferUsage::index | mirakana::rhi::BufferUsage::copy_destination});
    result.palette.add_mesh(mesh, mirakana::MeshGpuBinding{
                                      .vertex_buffer = vertex_buffer,
                                      .index_buffer = index_buffer,
                                      .vertex_count = 3,
                                      .index_count = 3,
                                      .vertex_offset = 0,
                                      .index_offset = 0,
                                      .vertex_stride = 12,
                                      .index_format = mirakana::rhi::IndexFormat::uint32,
                                      .owner_device = &device,
                                  });

    const auto material_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{});
    const auto material_set = device.allocate_descriptor_set(material_layout);
    const auto pipeline_layout = device.create_pipeline_layout(
        mirakana::rhi::PipelineLayoutDesc{.descriptor_sets = {material_layout}, .push_constant_bytes = 0});
    result.palette.add_material(material, mirakana::MaterialGpuBinding{
                                              .pipeline_layout = pipeline_layout,
                                              .descriptor_set = material_set,
                                              .descriptor_set_index = 0,
                                              .owner_device = &device,
                                          });
    result.material_bindings.push_back(mirakana::runtime_scene_rhi::RuntimeSceneMaterialGpuResource{
        .material = material,
        .binding =
            mirakana::runtime_rhi::RuntimeMaterialGpuBinding{
                .descriptor_set_layout = material_layout,
                .descriptor_set = material_set,
                .owner_device = &device,
            },
        .pipeline_layout = pipeline_layout,
    });
    result.material_pipeline_layouts.push_back(pipeline_layout);
    return result;
}

[[nodiscard]] mirakana::runtime_scene_rhi::RuntimeSceneGpuBindingResult
make_probe_scene_gpu_bindings_with_morph(mirakana::rhi::IRhiDevice& device, mirakana::AssetId mesh,
                                         mirakana::AssetId material, mirakana::AssetId morph_mesh) {
    auto result = make_probe_scene_gpu_bindings(device, mesh, material);

    mirakana::MorphMeshCpuSourceDocument morph;
    morph.vertex_count = 3;
    morph.bind_position_bytes.resize(36);
    mirakana::MorphMeshCpuTargetSourceDocument target;
    target.position_delta_bytes.resize(36, std::uint8_t{0x01});
    morph.targets.push_back(target);
    morph.target_weight_bytes.resize(4);
    const mirakana::runtime::RuntimeMorphMeshCpuPayload payload{
        .asset = morph_mesh, .handle = mirakana::runtime::RuntimeAssetHandle{42}, .morph = morph};

    auto upload = mirakana::runtime_rhi::upload_runtime_morph_mesh_cpu(device, payload);
    MK_REQUIRE(upload.succeeded());
    auto binding = mirakana::runtime_rhi::make_runtime_morph_mesh_gpu_binding(upload);
    const auto descriptor_diagnostic = mirakana::runtime_rhi::attach_morph_mesh_descriptor_set(
        device, upload, binding, result.morph_descriptor_set_layout);
    MK_REQUIRE(descriptor_diagnostic.empty());
    MK_REQUIRE(result.morph_palette.try_add_morph_mesh(morph_mesh, binding));
    result.morph_mesh_uploads.push_back(
        mirakana::runtime_scene_rhi::RuntimeSceneMorphMeshGpuResource{.morph_mesh = morph_mesh, .upload = upload});
    return result;
}

} // namespace

MK_TEST("sdl desktop event pump maps quit into lifecycle and window close") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Runtime Host Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 240}});
    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 320, .height = 240});
    mirakana::VirtualLifecycle lifecycle;
    mirakana::DesktopHostServices services{
        .window = &window,
        .renderer = &renderer,
        .lifecycle = &lifecycle,
    };
    mirakana::SdlDesktopEventPump pump(window);

    SDL_Event event{};
    event.type = SDL_EVENT_QUIT;
    MK_REQUIRE(SDL_PushEvent(&event));

    pump.pump_events(services);

    MK_REQUIRE(lifecycle.state().quit_requested);
    MK_REQUIRE(!window.is_open());
}

MK_TEST("sdl desktop presentation falls back to null renderer on dummy video driver") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"null"});
    MK_REQUIRE(presentation.renderer().backbuffer_extent().width == 320);
    MK_REQUIRE(presentation.renderer().backbuffer_extent().height == 180);
    MK_REQUIRE(!presentation.diagnostics().empty());
    MK_REQUIRE(presentation.diagnostics().front().reason ==
               mirakana::SdlDesktopPresentationFallbackReason::native_surface_unavailable);
    MK_REQUIRE(mirakana::sdl_desktop_presentation_fallback_reason_name(presentation.diagnostics().front().reason) ==
               std::string_view{"native_surface_unavailable"});
    MK_REQUIRE(presentation.diagnostics().front().message.find("NullRenderer fallback") != std::string::npos);

    const auto report = presentation.report();
    MK_REQUIRE(report.requested_backend == mirakana::SdlDesktopPresentationBackend::d3d12);
    MK_REQUIRE(report.selected_backend == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(report.fallback_reason == mirakana::SdlDesktopPresentationFallbackReason::native_surface_unavailable);
    MK_REQUIRE(report.used_null_fallback);
    MK_REQUIRE(report.allow_null_fallback);
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::not_requested);
    MK_REQUIRE(report.framegraph_passes == 0);
    MK_REQUIRE(report.postprocess_diagnostics_count == 0);
    MK_REQUIRE(report.diagnostics_count == presentation.diagnostics().size());
    MK_REQUIRE(report.backend_reports_count == presentation.backend_reports().size());
    MK_REQUIRE(!presentation.backend_reports().empty());
    MK_REQUIRE(presentation.backend_reports().front().backend == mirakana::SdlDesktopPresentationBackend::d3d12);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::native_surface_unavailable);
    MK_REQUIRE(mirakana::sdl_desktop_presentation_backend_report_status_name(
                   presentation.backend_reports().front().status) == std::string_view{"native_surface_unavailable"});
}

MK_TEST("sdl desktop presentation rejects d3d12 renderer request without shader bytecode") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Shader Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::SdlDesktopPresentationD3d12RendererDesc d3d12_renderer{
        .vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "vs_main"},
        .fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "ps_main"},
    };

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_renderer = &d3d12_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"null"});
    MK_REQUIRE(!presentation.diagnostics().empty());
    MK_REQUIRE(presentation.diagnostics().front().reason ==
               mirakana::SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable);
    MK_REQUIRE(mirakana::sdl_desktop_presentation_fallback_reason_name(presentation.diagnostics().front().reason) ==
               std::string_view{"runtime_pipeline_unavailable"});

    const auto report = presentation.report();
    MK_REQUIRE(report.requested_backend == mirakana::SdlDesktopPresentationBackend::d3d12);
    MK_REQUIRE(report.selected_backend == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(report.used_null_fallback);
    MK_REQUIRE(report.backend_reports_count == 1);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::missing_request);
    MK_REQUIRE(presentation.backend_reports().front().fallback_reason ==
               mirakana::SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable);
}

MK_TEST("sdl desktop presentation rejects vulkan renderer request without spirv bytecode") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Vulkan Shader Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::SdlDesktopPresentationVulkanRendererDesc vulkan_renderer{
        .vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "vs_main"},
        .fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "ps_main"},
    };

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_vulkan = true,
        .vulkan_renderer = &vulkan_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"null"});
    MK_REQUIRE(!presentation.diagnostics().empty());
    MK_REQUIRE(presentation.diagnostics().front().reason ==
               mirakana::SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable);
    MK_REQUIRE(mirakana::sdl_desktop_presentation_fallback_reason_name(presentation.diagnostics().front().reason) ==
               std::string_view{"runtime_pipeline_unavailable"});

    const auto report = presentation.report();
    MK_REQUIRE(report.requested_backend == mirakana::SdlDesktopPresentationBackend::vulkan);
    MK_REQUIRE(report.selected_backend == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(report.used_null_fallback);
    MK_REQUIRE(report.backend_reports_count == 1);
    MK_REQUIRE(presentation.backend_reports().front().backend == mirakana::SdlDesktopPresentationBackend::vulkan);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::missing_request);
}

MK_TEST("sdl desktop presentation rejects vulkan scene renderer request without scene metadata") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Vulkan Scene Request Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::SdlDesktopPresentationVulkanSceneRendererDesc vulkan_scene_renderer{
        .vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "vs_main"},
        .fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "ps_main"},
    };

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_vulkan = true,
        .vulkan_scene_renderer = &vulkan_scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"null"});
    MK_REQUIRE(presentation.scene_gpu_binding_status() ==
               mirakana::SdlDesktopPresentationSceneGpuBindingStatus::invalid_request);
    MK_REQUIRE(!presentation.scene_gpu_binding_diagnostics().empty());

    const auto report = presentation.report();
    MK_REQUIRE(report.requested_backend == mirakana::SdlDesktopPresentationBackend::vulkan);
    MK_REQUIRE(report.selected_backend == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(report.scene_gpu_status == mirakana::SdlDesktopPresentationSceneGpuBindingStatus::invalid_request);
    MK_REQUIRE(report.used_null_fallback);
    MK_REQUIRE(report.backend_reports_count == 1);
    MK_REQUIRE(report.scene_gpu_diagnostics_count == presentation.scene_gpu_binding_diagnostics().size());
    MK_REQUIRE(presentation.backend_reports().front().backend == mirakana::SdlDesktopPresentationBackend::vulkan);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::missing_request);
    MK_REQUIRE(presentation.backend_reports().front().message.find("Vulkan scene renderer") != std::string::npos);
}

MK_TEST("sdl desktop presentation rejects scene postprocess request without shader bytecode") {
    const std::array<std::uint8_t, 4> shader_bytes{0x01, 0x02, 0x03, 0x04};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_scene_packet();
    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Scene Postprocess Request Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.postprocess_status() == mirakana::SdlDesktopPresentationPostprocessStatus::invalid_request);
    MK_REQUIRE(!presentation.postprocess_ready());
    MK_REQUIRE(!presentation.postprocess_diagnostics().empty());
    MK_REQUIRE(presentation.postprocess_diagnostics().front().message.find("postprocess") != std::string::npos);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::missing_request);

    const auto report = presentation.report();
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::invalid_request);
    MK_REQUIRE(report.framegraph_passes == 0);
    MK_REQUIRE(report.postprocess_diagnostics_count == presentation.postprocess_diagnostics().size());
}

MK_TEST("sdl desktop presentation rejects d3d12 postprocess depth input without postprocess") {
    const std::array<std::uint8_t, 4> shader_bytes{0x21, 0x22, 0x23, 0x24};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_scene_packet();
    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess_depth_input = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation D3D12 Invalid Depth Input Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.postprocess_status() == mirakana::SdlDesktopPresentationPostprocessStatus::invalid_request);
    MK_REQUIRE(!presentation.postprocess_ready());
    MK_REQUIRE(!presentation.postprocess_depth_input_ready());
    MK_REQUIRE(!presentation.postprocess_diagnostics().empty());
    MK_REQUIRE(presentation.postprocess_diagnostics().front().message.find("depth input") != std::string::npos);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::missing_request);

    const auto report = presentation.report();
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::invalid_request);
    MK_REQUIRE(report.postprocess_depth_input_requested);
    MK_REQUIRE(!report.postprocess_depth_input_ready);
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation rejects d3d12 directional shadow without postprocess depth input") {
    const std::array<std::uint8_t, 4> shader_bytes{0x41, 0x42, 0x43, 0x44};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_shadow_scene_packet();
    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_shadow_receiver",
                .bytecode = shader_bytes,
            },
        .shadow_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_shadow",
                .bytecode = shader_bytes,
            },
        .shadow_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_shadow",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_directional_shadow_smoke = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation D3D12 Invalid Shadow Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.directional_shadow_status() ==
               mirakana::SdlDesktopPresentationDirectionalShadowStatus::invalid_request);
    MK_REQUIRE(!presentation.directional_shadow_ready());
    MK_REQUIRE(!presentation.directional_shadow_diagnostics().empty());
    MK_REQUIRE(presentation.directional_shadow_diagnostics().front().message.find("postprocess depth input") !=
               std::string::npos);

    const auto report = presentation.report();
    MK_REQUIRE(report.directional_shadow_status ==
               mirakana::SdlDesktopPresentationDirectionalShadowStatus::invalid_request);
    MK_REQUIRE(report.directional_shadow_requested);
    MK_REQUIRE(!report.directional_shadow_ready);
    MK_REQUIRE(report.directional_shadow_diagnostics_count == presentation.directional_shadow_diagnostics().size());
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation rejects vulkan directional shadow without postprocess depth input") {
    const std::array<std::uint8_t, 4> shader_bytes{0x45, 0x46, 0x47, 0x48};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_shadow_scene_packet();
    mirakana::SdlDesktopPresentationVulkanSceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_shadow_receiver",
                .bytecode = shader_bytes,
            },
        .shadow_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_shadow",
                .bytecode = shader_bytes,
            },
        .shadow_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_shadow",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_directional_shadow_smoke = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Vulkan Invalid Shadow Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_vulkan = true,
        .vulkan_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.directional_shadow_status() ==
               mirakana::SdlDesktopPresentationDirectionalShadowStatus::invalid_request);
    MK_REQUIRE(!presentation.directional_shadow_ready());
    MK_REQUIRE(!presentation.directional_shadow_diagnostics().empty());
    MK_REQUIRE(presentation.directional_shadow_diagnostics().front().message.find("postprocess depth input") !=
               std::string::npos);

    const auto report = presentation.report();
    MK_REQUIRE(report.directional_shadow_status ==
               mirakana::SdlDesktopPresentationDirectionalShadowStatus::invalid_request);
    MK_REQUIRE(report.directional_shadow_requested);
    MK_REQUIRE(!report.directional_shadow_ready);
    MK_REQUIRE(report.directional_shadow_diagnostics_count == presentation.directional_shadow_diagnostics().size());
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation rejects vulkan postprocess depth input without postprocess") {
    const std::array<std::uint8_t, 4> shader_bytes{0x31, 0x32, 0x33, 0x34};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_scene_packet();
    mirakana::SdlDesktopPresentationVulkanSceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess_depth_input = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Vulkan Invalid Depth Input Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_vulkan = true,
        .vulkan_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.postprocess_status() == mirakana::SdlDesktopPresentationPostprocessStatus::invalid_request);
    MK_REQUIRE(!presentation.postprocess_ready());
    MK_REQUIRE(!presentation.postprocess_depth_input_ready());
    MK_REQUIRE(!presentation.postprocess_diagnostics().empty());
    MK_REQUIRE(presentation.postprocess_diagnostics().front().message.find("depth input") != std::string::npos);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::missing_request);

    const auto report = presentation.report();
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::invalid_request);
    MK_REQUIRE(report.postprocess_depth_input_requested);
    MK_REQUIRE(!report.postprocess_depth_input_ready);
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation reports native presentation not requested") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Not Requested Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_d3d12 = false,
        .prefer_vulkan = false,
    });

    const auto report = presentation.report();
    MK_REQUIRE(report.requested_backend == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(report.selected_backend == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(report.fallback_reason == mirakana::SdlDesktopPresentationFallbackReason::none);
    MK_REQUIRE(!report.used_null_fallback);
    MK_REQUIRE(report.backend_reports_count == 1);
    MK_REQUIRE(presentation.backend_reports().front().backend ==
               mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::not_requested);
    MK_REQUIRE(presentation.backend_reports().front().fallback_reason ==
               mirakana::SdlDesktopPresentationFallbackReason::none);
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::not_requested);
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation quality gate accepts ready package renderer counters") {
    mirakana::SdlDesktopPresentationReport report;
    report.scene_gpu_status = mirakana::SdlDesktopPresentationSceneGpuBindingStatus::ready;
    report.scene_gpu_stats.mesh_bindings = 1;
    report.scene_gpu_stats.material_bindings = 1;
    report.scene_gpu_stats.mesh_bindings_resolved = 2;
    report.scene_gpu_stats.material_bindings_resolved = 2;
    report.postprocess_status = mirakana::SdlDesktopPresentationPostprocessStatus::ready;
    report.postprocess_depth_input_requested = true;
    report.postprocess_depth_input_ready = true;
    report.directional_shadow_status = mirakana::SdlDesktopPresentationDirectionalShadowStatus::ready;
    report.directional_shadow_requested = true;
    report.directional_shadow_ready = true;
    report.directional_shadow_filter_mode = mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3;
    report.directional_shadow_filter_tap_count = 9;
    report.directional_shadow_filter_radius_texels = 1.0F;
    report.framegraph_passes = 3;
    report.renderer_stats.frames_finished = 2;
    report.renderer_stats.framegraph_passes_executed = 6;
    report.renderer_stats.framegraph_barrier_steps_executed = 15;
    report.renderer_stats.postprocess_passes_executed = 2;

    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    desc.require_scene_gpu_bindings = true;
    desc.require_postprocess = true;
    desc.require_postprocess_depth_input = true;
    desc.require_directional_shadow = true;
    desc.require_directional_shadow_filtering = true;
    desc.expected_frames = 2;

    const auto quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);

    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::ready);
    MK_REQUIRE(quality.ready);
    MK_REQUIRE(quality.diagnostics_count == 0);
    MK_REQUIRE(quality.expected_framegraph_passes == 3);
    MK_REQUIRE(quality.expected_framegraph_barrier_steps == 15);
    MK_REQUIRE(quality.scene_gpu_ready);
    MK_REQUIRE(quality.postprocess_ready);
    MK_REQUIRE(quality.postprocess_depth_input_ready);
    MK_REQUIRE(quality.directional_shadow_ready);
    MK_REQUIRE(quality.directional_shadow_filter_ready);
    MK_REQUIRE(quality.framegraph_passes_current);
    MK_REQUIRE(quality.framegraph_barrier_steps_current);
    MK_REQUIRE(quality.framegraph_execution_budget_current);
}

MK_TEST("sdl desktop presentation quality gate expects postprocess depth framegraph barrier steps") {
    mirakana::SdlDesktopPresentationReport report;
    report.postprocess_status = mirakana::SdlDesktopPresentationPostprocessStatus::ready;
    report.postprocess_depth_input_requested = true;
    report.postprocess_depth_input_ready = true;
    report.framegraph_passes = 2;
    report.renderer_stats.frames_finished = 3;
    report.renderer_stats.framegraph_passes_executed = 6;
    report.renderer_stats.framegraph_barrier_steps_executed = 13;
    report.renderer_stats.postprocess_passes_executed = 3;

    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    desc.require_postprocess = true;
    desc.require_postprocess_depth_input = true;
    desc.expected_frames = 3;

    const auto quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);

    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::ready);
    MK_REQUIRE(quality.ready);
    MK_REQUIRE(quality.diagnostics_count == 0);
    MK_REQUIRE(quality.expected_framegraph_passes == 2);
    MK_REQUIRE(quality.expected_framegraph_barrier_steps == 13);
    MK_REQUIRE(quality.postprocess_ready);
    MK_REQUIRE(quality.postprocess_depth_input_ready);
    MK_REQUIRE(quality.framegraph_passes_current);
    MK_REQUIRE(quality.framegraph_barrier_steps_current);
    MK_REQUIRE(quality.framegraph_execution_budget_current);
}

MK_TEST("sdl desktop presentation quality gate expects postprocess framegraph scene target prep steps") {
    mirakana::SdlDesktopPresentationReport report;
    report.postprocess_status = mirakana::SdlDesktopPresentationPostprocessStatus::ready;
    report.framegraph_passes = 2;
    report.renderer_stats.frames_finished = 2;
    report.renderer_stats.framegraph_passes_executed = 4;
    report.renderer_stats.framegraph_barrier_steps_executed = 4;
    report.renderer_stats.postprocess_passes_executed = 2;

    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    desc.require_postprocess = true;
    desc.expected_frames = 2;

    const auto quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);

    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::ready);
    MK_REQUIRE(quality.ready);
    MK_REQUIRE(quality.diagnostics_count == 0);
    MK_REQUIRE(quality.expected_framegraph_passes == 2);
    MK_REQUIRE(quality.expected_framegraph_barrier_steps == 4);
    MK_REQUIRE(quality.postprocess_ready);
    MK_REQUIRE(quality.framegraph_passes_current);
    MK_REQUIRE(quality.framegraph_barrier_steps_current);
    MK_REQUIRE(quality.framegraph_execution_budget_current);
}

MK_TEST("sdl desktop presentation quality gate expects minimum postprocess target prep steps") {
    mirakana::SdlDesktopPresentationReport report;
    report.postprocess_status = mirakana::SdlDesktopPresentationPostprocessStatus::ready;
    report.framegraph_passes = 2;
    report.renderer_stats.frames_finished = 1;
    report.renderer_stats.framegraph_passes_executed = 2;
    report.renderer_stats.framegraph_barrier_steps_executed = 1;
    report.renderer_stats.postprocess_passes_executed = 1;

    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    desc.require_postprocess = true;

    auto quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);
    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::blocked);
    MK_REQUIRE(!quality.ready);
    MK_REQUIRE(quality.expected_framegraph_passes == 2);
    MK_REQUIRE(quality.expected_framegraph_barrier_steps == 2);
    MK_REQUIRE(!quality.framegraph_barrier_steps_current);

    report.renderer_stats.framegraph_barrier_steps_executed = 2;
    quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);
    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::ready);
    MK_REQUIRE(quality.ready);
    MK_REQUIRE(quality.framegraph_barrier_steps_current);
    MK_REQUIRE(quality.framegraph_execution_budget_current);
}

MK_TEST("sdl desktop presentation quality gate expects minimum postprocess depth target prep steps") {
    mirakana::SdlDesktopPresentationReport report;
    report.postprocess_status = mirakana::SdlDesktopPresentationPostprocessStatus::ready;
    report.postprocess_depth_input_requested = true;
    report.postprocess_depth_input_ready = true;
    report.framegraph_passes = 2;
    report.renderer_stats.frames_finished = 1;
    report.renderer_stats.framegraph_passes_executed = 2;
    report.renderer_stats.framegraph_barrier_steps_executed = 4;
    report.renderer_stats.postprocess_passes_executed = 1;

    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    desc.require_postprocess = true;
    desc.require_postprocess_depth_input = true;

    auto quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);
    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::blocked);
    MK_REQUIRE(!quality.ready);
    MK_REQUIRE(quality.expected_framegraph_passes == 2);
    MK_REQUIRE(quality.expected_framegraph_barrier_steps == 5);
    MK_REQUIRE(!quality.framegraph_barrier_steps_current);

    report.renderer_stats.framegraph_barrier_steps_executed = 5;
    quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);
    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::ready);
    MK_REQUIRE(quality.ready);
    MK_REQUIRE(quality.framegraph_barrier_steps_current);
    MK_REQUIRE(quality.framegraph_execution_budget_current);
}

MK_TEST("sdl desktop presentation quality gate expects minimum directional shadow target prep steps") {
    mirakana::SdlDesktopPresentationReport report;
    report.postprocess_status = mirakana::SdlDesktopPresentationPostprocessStatus::ready;
    report.postprocess_depth_input_requested = true;
    report.postprocess_depth_input_ready = true;
    report.directional_shadow_status = mirakana::SdlDesktopPresentationDirectionalShadowStatus::ready;
    report.directional_shadow_requested = true;
    report.directional_shadow_ready = true;
    report.directional_shadow_filter_mode = mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3;
    report.directional_shadow_filter_tap_count = 9;
    report.directional_shadow_filter_radius_texels = 1.0F;
    report.framegraph_passes = 3;
    report.renderer_stats.frames_finished = 1;
    report.renderer_stats.framegraph_passes_executed = 3;
    report.renderer_stats.framegraph_barrier_steps_executed = 8;
    report.renderer_stats.postprocess_passes_executed = 1;

    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    desc.require_postprocess = true;
    desc.require_postprocess_depth_input = true;
    desc.require_directional_shadow = true;
    desc.require_directional_shadow_filtering = true;

    auto quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);
    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::blocked);
    MK_REQUIRE(!quality.ready);
    MK_REQUIRE(quality.expected_framegraph_passes == 3);
    MK_REQUIRE(quality.expected_framegraph_barrier_steps == 9);
    MK_REQUIRE(!quality.framegraph_barrier_steps_current);

    report.renderer_stats.framegraph_barrier_steps_executed = 9;
    quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);
    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::ready);
    MK_REQUIRE(quality.ready);
    MK_REQUIRE(quality.framegraph_barrier_steps_current);
    MK_REQUIRE(quality.framegraph_execution_budget_current);
}

MK_TEST("sdl desktop presentation quality gate blocks stale package renderer counters") {
    mirakana::SdlDesktopPresentationReport report;
    report.scene_gpu_status = mirakana::SdlDesktopPresentationSceneGpuBindingStatus::ready;
    report.scene_gpu_stats.mesh_bindings = 1;
    report.scene_gpu_stats.material_bindings = 1;
    report.scene_gpu_stats.mesh_bindings_resolved = 2;
    report.scene_gpu_stats.material_bindings_resolved = 2;
    report.postprocess_status = mirakana::SdlDesktopPresentationPostprocessStatus::ready;
    report.postprocess_depth_input_requested = true;
    report.postprocess_depth_input_ready = true;
    report.directional_shadow_status = mirakana::SdlDesktopPresentationDirectionalShadowStatus::unavailable;
    report.directional_shadow_requested = true;
    report.directional_shadow_ready = false;
    report.directional_shadow_filter_mode = mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::none;
    report.framegraph_passes = 2;
    report.renderer_stats.frames_finished = 2;
    report.renderer_stats.framegraph_passes_executed = 4;
    report.renderer_stats.framegraph_barrier_steps_executed = 6;
    report.renderer_stats.postprocess_passes_executed = 2;

    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    desc.require_scene_gpu_bindings = true;
    desc.require_postprocess = true;
    desc.require_postprocess_depth_input = true;
    desc.require_directional_shadow = true;
    desc.require_directional_shadow_filtering = true;
    desc.expected_frames = 2;

    const auto quality = mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, desc);

    MK_REQUIRE(quality.status == mirakana::SdlDesktopPresentationQualityGateStatus::blocked);
    MK_REQUIRE(!quality.ready);
    MK_REQUIRE(quality.diagnostics_count >= 4);
    MK_REQUIRE(quality.expected_framegraph_barrier_steps == 15);
    MK_REQUIRE(quality.scene_gpu_ready);
    MK_REQUIRE(quality.postprocess_ready);
    MK_REQUIRE(quality.postprocess_depth_input_ready);
    MK_REQUIRE(!quality.directional_shadow_ready);
    MK_REQUIRE(!quality.directional_shadow_filter_ready);
    MK_REQUIRE(!quality.framegraph_passes_current);
    MK_REQUIRE(!quality.framegraph_barrier_steps_current);
    MK_REQUIRE(!quality.framegraph_execution_budget_current);
}

MK_TEST("sdl desktop presentation throws when d3d12 shader bytecode is required without fallback") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Required Shader Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::SdlDesktopPresentationD3d12RendererDesc d3d12_renderer{
        .vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "vs_main"},
        .fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "ps_main"},
    };

    bool rejected = false;
    try {
        mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
            .window = &window,
            .extent = mirakana::Extent2D{.width = 320, .height = 180},
            .allow_null_fallback = false,
            .d3d12_renderer = &d3d12_renderer,
        });
    } catch (const std::logic_error& error) {
        rejected = std::string_view{error.what()}.find("shader bytecode") != std::string_view::npos;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("sdl desktop presentation throws when vulkan spirv bytecode is required without fallback") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Required Vulkan Shader Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::SdlDesktopPresentationVulkanRendererDesc vulkan_renderer{
        .vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "vs_main"},
        .fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{.entry_point = "ps_main"},
    };

    bool rejected = false;
    try {
        mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
            .window = &window,
            .extent = mirakana::Extent2D{.width = 320, .height = 180},
            .prefer_vulkan = true,
            .allow_null_fallback = false,
            .vulkan_renderer = &vulkan_renderer,
        });
    } catch (const std::logic_error& error) {
        rejected = std::string_view{error.what()}.find("SPIR-V") != std::string_view::npos;
    }
    MK_REQUIRE(rejected);
}

MK_TEST("sdl desktop presentation reports vulkan renderer unavailable on dummy video driver") {
    const std::array<std::uint8_t, 4> shader_bytes{0x03, 0x02, 0x23, 0x07};
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Vulkan Fallback Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::SdlDesktopPresentationVulkanRendererDesc vulkan_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
    };

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_vulkan = true,
        .vulkan_renderer = &vulkan_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"null"});
    MK_REQUIRE(!presentation.diagnostics().empty());
    MK_REQUIRE(presentation.diagnostics().front().reason ==
               mirakana::SdlDesktopPresentationFallbackReason::native_surface_unavailable);
    MK_REQUIRE(presentation.diagnostics().front().message.find("Vulkan") != std::string::npos);
    MK_REQUIRE(mirakana::sdl_desktop_presentation_backend_name(mirakana::SdlDesktopPresentationBackend::vulkan) ==
               std::string_view{"vulkan"});

    const auto report = presentation.report();
    MK_REQUIRE(report.requested_backend == mirakana::SdlDesktopPresentationBackend::vulkan);
    MK_REQUIRE(report.selected_backend == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(report.fallback_reason == mirakana::SdlDesktopPresentationFallbackReason::native_surface_unavailable);
    MK_REQUIRE(report.used_null_fallback);
    MK_REQUIRE(report.backend_reports_count == 1);
    MK_REQUIRE(presentation.backend_reports().front().backend == mirakana::SdlDesktopPresentationBackend::vulkan);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::native_surface_unavailable);
}

MK_TEST("sdl desktop presentation reports vulkan scene gpu bindings unavailable on null fallback") {
    const std::array<std::uint8_t, 4> shader_bytes{0x03, 0x02, 0x23, 0x07};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_scene_packet();
    mirakana::SdlDesktopPresentationVulkanSceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
        .postprocess_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_postprocess",
                .bytecode = shader_bytes,
            },
        .postprocess_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_postprocess",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Vulkan Scene GPU Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_vulkan = true,
        .vulkan_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.scene_gpu_binding_status() ==
               mirakana::SdlDesktopPresentationSceneGpuBindingStatus::unavailable);
    MK_REQUIRE(!presentation.scene_gpu_bindings_ready());
    MK_REQUIRE(!presentation.scene_gpu_binding_diagnostics().empty());
    MK_REQUIRE(presentation.scene_gpu_binding_diagnostics().front().message.find("Vulkan scene GPU") !=
               std::string::npos);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"null"});
    MK_REQUIRE(presentation.postprocess_status() == mirakana::SdlDesktopPresentationPostprocessStatus::unavailable);
    MK_REQUIRE(!presentation.postprocess_ready());

    const auto report = presentation.report();
    MK_REQUIRE(report.requested_backend == mirakana::SdlDesktopPresentationBackend::vulkan);
    MK_REQUIRE(report.scene_gpu_status == mirakana::SdlDesktopPresentationSceneGpuBindingStatus::unavailable);
    MK_REQUIRE(report.scene_gpu_stats.mesh_bindings == 0);
    MK_REQUIRE(report.scene_gpu_diagnostics_count == presentation.scene_gpu_binding_diagnostics().size());
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::unavailable);
    MK_REQUIRE(report.framegraph_passes == 0);
    MK_REQUIRE(presentation.backend_reports().front().backend == mirakana::SdlDesktopPresentationBackend::vulkan);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::native_surface_unavailable);
}

MK_TEST("sdl desktop presentation reports scene gpu bindings unavailable on null fallback") {
    const std::array<std::uint8_t, 4> shader_bytes{0x01, 0x02, 0x03, 0x04};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_scene_packet();
    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
        .postprocess_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_postprocess",
                .bytecode = shader_bytes,
            },
        .postprocess_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_postprocess",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Scene GPU Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.scene_gpu_binding_status() ==
               mirakana::SdlDesktopPresentationSceneGpuBindingStatus::unavailable);
    MK_REQUIRE(!presentation.scene_gpu_bindings_ready());
    MK_REQUIRE(!presentation.scene_gpu_binding_diagnostics().empty());
    MK_REQUIRE(presentation.scene_gpu_binding_stats().mesh_bindings == 0);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"null"});
    MK_REQUIRE(presentation.postprocess_status() == mirakana::SdlDesktopPresentationPostprocessStatus::unavailable);

    const auto report = presentation.report();
    MK_REQUIRE(report.scene_gpu_status == mirakana::SdlDesktopPresentationSceneGpuBindingStatus::unavailable);
    MK_REQUIRE(report.scene_gpu_stats.mesh_bindings == 0);
    MK_REQUIRE(report.scene_gpu_diagnostics_count == presentation.scene_gpu_binding_diagnostics().size());
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::unavailable);
}

MK_TEST("sdl desktop presentation reports d3d12 postprocess depth input unavailable on null fallback") {
    const std::array<std::uint8_t, 4> shader_bytes{0x11, 0x22, 0x33, 0x44};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_scene_packet();
    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
        .postprocess_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_postprocess",
                .bytecode = shader_bytes,
            },
        .postprocess_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_postprocess",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess = true,
        .enable_postprocess_depth_input = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation D3D12 Postprocess Depth Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.postprocess_status() == mirakana::SdlDesktopPresentationPostprocessStatus::unavailable);
    MK_REQUIRE(!presentation.postprocess_ready());
    MK_REQUIRE(!presentation.postprocess_depth_input_ready());
    MK_REQUIRE(!presentation.postprocess_diagnostics().empty());

    bool has_depth_diagnostic = false;
    for (const auto& diagnostic : presentation.postprocess_diagnostics()) {
        has_depth_diagnostic = has_depth_diagnostic || diagnostic.message.find("depth input") != std::string::npos;
    }
    MK_REQUIRE(has_depth_diagnostic);

    const auto report = presentation.report();
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::unavailable);
    MK_REQUIRE(report.postprocess_depth_input_requested);
    MK_REQUIRE(!report.postprocess_depth_input_ready);
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation reports vulkan postprocess depth input unavailable on null fallback") {
    const std::array<std::uint8_t, 4> shader_bytes{0x55, 0x66, 0x77, 0x88};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_scene_packet();
    mirakana::SdlDesktopPresentationVulkanSceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
        .postprocess_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_postprocess",
                .bytecode = shader_bytes,
            },
        .postprocess_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_postprocess",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess = true,
        .enable_postprocess_depth_input = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Vulkan Postprocess Depth Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_vulkan = true,
        .vulkan_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.postprocess_status() == mirakana::SdlDesktopPresentationPostprocessStatus::unavailable);
    MK_REQUIRE(!presentation.postprocess_ready());
    MK_REQUIRE(!presentation.postprocess_depth_input_ready());
    MK_REQUIRE(!presentation.postprocess_diagnostics().empty());

    bool has_depth_diagnostic = false;
    for (const auto& diagnostic : presentation.postprocess_diagnostics()) {
        has_depth_diagnostic = has_depth_diagnostic || diagnostic.message.find("depth input") != std::string::npos;
    }
    MK_REQUIRE(has_depth_diagnostic);

    const auto report = presentation.report();
    MK_REQUIRE(report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::unavailable);
    MK_REQUIRE(report.postprocess_depth_input_requested);
    MK_REQUIRE(!report.postprocess_depth_input_ready);
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation reports d3d12 directional shadow unavailable on null fallback") {
    const std::array<std::uint8_t, 4> shader_bytes{0x91, 0x92, 0x93, 0x94};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_shadow_scene_packet();
    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_shadow_receiver",
                .bytecode = shader_bytes,
            },
        .postprocess_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_postprocess",
                .bytecode = shader_bytes,
            },
        .postprocess_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_postprocess",
                .bytecode = shader_bytes,
            },
        .shadow_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_shadow",
                .bytecode = shader_bytes,
            },
        .shadow_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_shadow",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess = true,
        .enable_postprocess_depth_input = true,
        .enable_directional_shadow_smoke = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation D3D12 Shadow Fallback Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.directional_shadow_status() ==
               mirakana::SdlDesktopPresentationDirectionalShadowStatus::unavailable);
    MK_REQUIRE(!presentation.directional_shadow_ready());
    MK_REQUIRE(!presentation.directional_shadow_diagnostics().empty());

    bool has_shadow_diagnostic = false;
    for (const auto& diagnostic : presentation.directional_shadow_diagnostics()) {
        has_shadow_diagnostic =
            has_shadow_diagnostic || diagnostic.message.find("directional shadow") != std::string::npos;
    }
    MK_REQUIRE(has_shadow_diagnostic);

    const auto report = presentation.report();
    MK_REQUIRE(report.directional_shadow_status ==
               mirakana::SdlDesktopPresentationDirectionalShadowStatus::unavailable);
    MK_REQUIRE(report.directional_shadow_requested);
    MK_REQUIRE(!report.directional_shadow_ready);
    MK_REQUIRE(report.directional_shadow_diagnostics_count == presentation.directional_shadow_diagnostics().size());
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation reports vulkan directional shadow unavailable on null fallback") {
    const std::array<std::uint8_t, 4> shader_bytes{0x95, 0x96, 0x97, 0x98};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_shadow_scene_packet();
    mirakana::SdlDesktopPresentationVulkanSceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_shadow_receiver",
                .bytecode = shader_bytes,
            },
        .postprocess_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_postprocess",
                .bytecode = shader_bytes,
            },
        .postprocess_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_postprocess",
                .bytecode = shader_bytes,
            },
        .shadow_vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_shadow",
                .bytecode = shader_bytes,
            },
        .shadow_fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_shadow",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 12}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position}},
        .enable_postprocess = true,
        .enable_postprocess_depth_input = true,
        .enable_directional_shadow_smoke = true,
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Vulkan Shadow Fallback Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .prefer_vulkan = true,
        .vulkan_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.directional_shadow_status() ==
               mirakana::SdlDesktopPresentationDirectionalShadowStatus::unavailable);
    MK_REQUIRE(!presentation.directional_shadow_ready());
    MK_REQUIRE(!presentation.directional_shadow_diagnostics().empty());

    bool has_shadow_diagnostic = false;
    for (const auto& diagnostic : presentation.directional_shadow_diagnostics()) {
        has_shadow_diagnostic =
            has_shadow_diagnostic || diagnostic.message.find("directional shadow") != std::string::npos;
    }
    MK_REQUIRE(has_shadow_diagnostic);

    const auto report = presentation.report();
    MK_REQUIRE(report.directional_shadow_status ==
               mirakana::SdlDesktopPresentationDirectionalShadowStatus::unavailable);
    MK_REQUIRE(report.directional_shadow_requested);
    MK_REQUIRE(!report.directional_shadow_ready);
    MK_REQUIRE(report.directional_shadow_diagnostics_count == presentation.directional_shadow_diagnostics().size());
    MK_REQUIRE(report.framegraph_passes == 0);
}

MK_TEST("sdl desktop presentation rejects scene renderer mesh layout mismatches before native surface work") {
    const std::array<std::uint8_t, 4> shader_bytes{0x01, 0x02, 0x03, 0x04};
    const auto package = make_probe_package(mirakana::AssetId::from_name("meshes/probe"));
    const auto packet = make_probe_scene_packet();
    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc scene_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = shader_bytes,
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = shader_bytes,
            },
        .package = &package,
        .packet = &packet,
        .vertex_buffers = {mirakana::rhi::VertexBufferLayoutDesc{.binding = 0, .stride = 32}},
        .vertex_attributes = {mirakana::rhi::VertexAttributeDesc{.location = 0,
                                                                 .binding = 0,
                                                                 .offset = 0,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::position},
                              mirakana::rhi::VertexAttributeDesc{.location = 1,
                                                                 .binding = 0,
                                                                 .offset = 12,
                                                                 .format = mirakana::rhi::VertexFormat::float32x3,
                                                                 .semantic = mirakana::rhi::VertexSemantic::normal},
                              mirakana::rhi::VertexAttributeDesc{.location = 2,
                                                                 .binding = 0,
                                                                 .offset = 24,
                                                                 .format = mirakana::rhi::VertexFormat::float32x2,
                                                                 .semantic = mirakana::rhi::VertexSemantic::texcoord}},
    };
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Scene Layout Mismatch Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});

    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_scene_renderer = &scene_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(presentation.scene_gpu_binding_status() ==
               mirakana::SdlDesktopPresentationSceneGpuBindingStatus::invalid_request);
    MK_REQUIRE(!presentation.scene_gpu_bindings_ready());
    MK_REQUIRE(!presentation.scene_gpu_binding_diagnostics().empty());
    MK_REQUIRE(presentation.scene_gpu_binding_diagnostics().front().message.find("vertex input") != std::string::npos);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::missing_request);
}

MK_TEST("sdl scene gpu binding renderer injects palette bindings and reports counters") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/probe");
    const auto material = mirakana::AssetId::from_name("materials/probe");
    auto renderer = mirakana::runtime_host_sdl3_detail::SceneGpuBindingInjectingRenderer(
        std::make_unique<mirakana::NullRenderer>(mirakana::Extent2D{.width = 64, .height = 64}),
        make_probe_scene_gpu_bindings(device, mesh, material));

    const auto initial_stats = renderer.scene_gpu_binding_stats();
    MK_REQUIRE(initial_stats.mesh_bindings == 1);
    MK_REQUIRE(initial_stats.material_bindings == 1);
    MK_REQUIRE(initial_stats.mesh_bindings_resolved == 0);
    MK_REQUIRE(initial_stats.material_bindings_resolved == 0);

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{.mesh = mesh, .material = material});
    renderer.end_frame();

    const auto resolved_stats = renderer.scene_gpu_binding_stats();
    MK_REQUIRE(resolved_stats.mesh_bindings == 1);
    MK_REQUIRE(resolved_stats.material_bindings == 1);
    MK_REQUIRE(resolved_stats.mesh_bindings_resolved == 1);
    MK_REQUIRE(resolved_stats.material_bindings_resolved == 1);
    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(renderer.stats().frames_finished == 1);

    const auto prebound = make_probe_scene_gpu_bindings(device, mesh, material);
    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{.mesh = mesh,
                                             .material = material,
                                             .mesh_binding = *prebound.palette.find_mesh(mesh),
                                             .material_binding = *prebound.palette.find_material(material)});
    renderer.end_frame();

    const auto prebound_stats = renderer.scene_gpu_binding_stats();
    MK_REQUIRE(prebound_stats.mesh_bindings_resolved == 1);
    MK_REQUIRE(prebound_stats.material_bindings_resolved == 1);
    MK_REQUIRE(renderer.stats().meshes_submitted == 2);
}

MK_TEST("sdl scene gpu binding renderer reports retained morph palette upload counters") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/probe");
    const auto material = mirakana::AssetId::from_name("materials/probe");
    const auto morph_mesh = mirakana::AssetId::from_name("morphs/probe");
    auto renderer = mirakana::runtime_host_sdl3_detail::SceneGpuBindingInjectingRenderer(
        std::make_unique<mirakana::NullRenderer>(mirakana::Extent2D{.width = 64, .height = 64}),
        make_probe_scene_gpu_bindings_with_morph(device, mesh, material, morph_mesh));

    const auto stats = renderer.scene_gpu_binding_stats();
    MK_REQUIRE(stats.morph_mesh_bindings == 1);
    MK_REQUIRE(stats.morph_mesh_uploads == 1);
    MK_REQUIRE(stats.uploaded_morph_bytes == 36U + 256U);
}

MK_TEST("sdl scene gpu binding renderer resolves explicit mesh morph bindings") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/probe");
    const auto material = mirakana::AssetId::from_name("materials/probe");
    const auto morph_mesh = mirakana::AssetId::from_name("morphs/probe");
    auto renderer = mirakana::runtime_host_sdl3_detail::SceneGpuBindingInjectingRenderer(
        std::make_unique<mirakana::NullRenderer>(mirakana::Extent2D{.width = 64, .height = 64}),
        make_probe_scene_gpu_bindings_with_morph(device, mesh, material, morph_mesh),
        std::vector<mirakana::SdlDesktopPresentationSceneMorphMeshBinding>{{.mesh = mesh, .morph_mesh = morph_mesh}});

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{.mesh = mesh, .material = material});
    renderer.end_frame();

    const auto stats = renderer.scene_gpu_binding_stats();
    MK_REQUIRE(stats.mesh_bindings_resolved == 1);
    MK_REQUIRE(stats.material_bindings_resolved == 1);
    MK_REQUIRE(stats.morph_mesh_bindings_resolved == 1);
    MK_REQUIRE(renderer.stats().gpu_morph_draws == 1);
}

MK_TEST("sdl scene gpu binding renderer resolves compute morph output bindings") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/probe");
    const auto material = mirakana::AssetId::from_name("materials/probe");
    const auto morph_mesh = mirakana::AssetId::from_name("morphs/probe");
    auto bindings = make_probe_scene_gpu_bindings(device, mesh, material);
    const auto base_mesh = bindings.palette.find_mesh(mesh);
    MK_REQUIRE(base_mesh != nullptr);
    const auto base_mesh_binding = *base_mesh;
    const auto compute_output = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage});
    auto compute_commands = device.begin_command_list(mirakana::rhi::QueueKind::compute);
    compute_commands->close();
    const auto compute_fence = device.submit(*compute_commands);
    device.wait_for_queue(mirakana::rhi::QueueKind::graphics, compute_fence);
    auto graphics_commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    graphics_commands->close();
    const auto graphics_fence = device.submit(*graphics_commands);

    auto renderer = mirakana::runtime_host_sdl3_detail::SceneGpuBindingInjectingRenderer(
        std::make_unique<mirakana::NullRenderer>(mirakana::Extent2D{.width = 64, .height = 64}), std::move(bindings),
        {},
        std::vector<mirakana::runtime_host_sdl3_detail::SceneComputeMorphMeshBinding>{
            mirakana::runtime_host_sdl3_detail::SceneComputeMorphMeshBinding{
                .mesh = mesh,
                .morph_mesh = morph_mesh,
                .mesh_binding =
                    mirakana::MeshGpuBinding{
                        .vertex_buffer = compute_output,
                        .index_buffer = base_mesh_binding.index_buffer,
                        .vertex_count = base_mesh_binding.vertex_count,
                        .index_count = base_mesh_binding.index_count,
                        .vertex_offset = 0,
                        .index_offset = base_mesh_binding.index_offset,
                        .vertex_stride = mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes,
                        .index_format = base_mesh_binding.index_format,
                        .owner_device = &device,
                    },
            },
        },
        1);

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .mesh = mesh,
        .material = material,
    });
    renderer.end_frame();

    const auto stats = renderer.scene_gpu_binding_stats();
    MK_REQUIRE(stats.compute_morph_mesh_bindings == 1);
    MK_REQUIRE(stats.compute_morph_queue_waits == 1);
    MK_REQUIRE(stats.compute_morph_async_compute_queue_submits == 1);
    MK_REQUIRE(stats.compute_morph_async_graphics_queue_waits == 1);
    MK_REQUIRE(stats.compute_morph_async_graphics_queue_submits == 1);
    MK_REQUIRE(stats.compute_morph_async_last_compute_submitted_fence_value == compute_fence.value);
    MK_REQUIRE(stats.compute_morph_async_last_graphics_queue_wait_fence_value == compute_fence.value);
    MK_REQUIRE(stats.compute_morph_async_last_graphics_submitted_fence_value == graphics_fence.value);
    MK_REQUIRE(stats.compute_morph_mesh_bindings_resolved == 1);
    MK_REQUIRE(stats.compute_morph_mesh_draws == 1);
    MK_REQUIRE(renderer.stats().meshes_submitted == 1);
    MK_REQUIRE(renderer.stats().gpu_morph_draws == 0);
}

MK_TEST("sdl scene gpu binding renderer resolves compute morph skinned bindings") {
    mirakana::rhi::NullRhiDevice device;
    const auto mesh = mirakana::AssetId::from_name("meshes/skinned-probe");
    const auto material = mirakana::AssetId::from_name("materials/probe");
    const auto morph_mesh = mirakana::AssetId::from_name("morphs/probe");
    auto bindings =
        make_probe_scene_gpu_bindings(device, mirakana::AssetId::from_name("meshes/static-probe"), material);
    const auto position_buffer = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 36, .usage = mirakana::rhi::BufferUsage::vertex | mirakana::rhi::BufferUsage::storage});
    const auto skin_attribute_buffer =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 192, .usage = mirakana::rhi::BufferUsage::vertex});
    const auto index_buffer =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 12, .usage = mirakana::rhi::BufferUsage::index});
    const auto joint_buffer =
        device.create_buffer(mirakana::rhi::BufferDesc{.size_bytes = 64, .usage = mirakana::rhi::BufferUsage::uniform});
    const auto joint_layout = device.create_descriptor_set_layout(mirakana::rhi::DescriptorSetLayoutDesc{{
        mirakana::rhi::DescriptorBindingDesc{.binding = 0,
                                             .type = mirakana::rhi::DescriptorType::uniform_buffer,
                                             .count = 1,
                                             .stages = mirakana::rhi::ShaderStageVisibility::vertex},
    }});
    const auto joint_set = device.allocate_descriptor_set(joint_layout);

    bindings.skinned_palette.try_add_skinned_mesh(
        mesh, mirakana::SkinnedMeshGpuBinding{
                  .mesh =
                      mirakana::MeshGpuBinding{
                          .vertex_buffer = position_buffer,
                          .index_buffer = index_buffer,
                          .vertex_count = 3,
                          .index_count = 3,
                          .vertex_offset = 0,
                          .index_offset = 0,
                          .vertex_stride = mirakana::runtime_rhi::runtime_mesh_position_vertex_stride_bytes,
                          .index_format = mirakana::rhi::IndexFormat::uint32,
                          .owner_device = &device,
                      },
                  .joint_palette_buffer = joint_buffer,
                  .joint_palette_upload_buffer = {},
                  .joint_descriptor_set = joint_set,
                  .joint_count = 1,
                  .joint_palette_uniform_allocation_bytes = 64,
                  .owner_device = &device,
                  .skin_attribute_vertex_buffer = skin_attribute_buffer,
                  .skin_attribute_vertex_offset = 0,
                  .skin_attribute_vertex_stride = mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes,
              });
    bindings.compute_morph_skinned_mesh_bindings.push_back(
        mirakana::runtime_scene_rhi::RuntimeSceneComputeMorphSkinnedMeshGpuResource{
            .mesh = mesh,
            .morph_mesh = morph_mesh,
            .base_position_upload = {},
            .compute_binding =
                mirakana::runtime_rhi::RuntimeMorphMeshComputeBinding{
                    .output_position_buffer = position_buffer,
                    .output_position_buffer_desc =
                        mirakana::rhi::BufferDesc{.size_bytes = 36,
                                                  .usage = mirakana::rhi::BufferUsage::storage |
                                                           mirakana::rhi::BufferUsage::vertex},
                    .vertex_count = 3,
                    .target_count = 1,
                    .output_position_bytes = 36,
                    .owner_device = &device,
                },
        });

    auto renderer = mirakana::runtime_host_sdl3_detail::SceneGpuBindingInjectingRenderer(
        std::make_unique<mirakana::NullRenderer>(mirakana::Extent2D{.width = 64, .height = 64}), std::move(bindings),
        {}, {}, 0, 1, 1);

    renderer.begin_frame();
    renderer.draw_mesh(mirakana::MeshCommand{
        .mesh = mesh,
        .material = material,
    });
    renderer.end_frame();

    const auto stats = renderer.scene_gpu_binding_stats();
    MK_REQUIRE(stats.compute_morph_skinned_mesh_bindings == 1);
    MK_REQUIRE(stats.compute_morph_skinned_mesh_dispatches == 1);
    MK_REQUIRE(stats.compute_morph_skinned_queue_waits == 1);
    MK_REQUIRE(stats.compute_morph_skinned_mesh_bindings_resolved == 1);
    MK_REQUIRE(stats.compute_morph_skinned_mesh_draws == 1);
    MK_REQUIRE(stats.compute_morph_output_position_bytes == 36);
    MK_REQUIRE(renderer.stats().gpu_skinning_draws == 1);
}

MK_TEST("sdl desktop scene renderer descs carry selected morph mesh assets and mappings") {
    const auto mesh = mirakana::AssetId::from_name("meshes/probe");
    const auto morph_mesh = mirakana::AssetId::from_name("morphs/probe");
    constexpr std::array<std::uint8_t, 4> k_shader{1, 2, 3, 4};

    mirakana::SdlDesktopPresentationD3d12SceneRendererDesc d3d12_desc;
    d3d12_desc.morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
        .entry_point = "vs_morph",
        .bytecode = k_shader,
    };
    d3d12_desc.morph_mesh_assets = {morph_mesh};
    d3d12_desc.morph_mesh_bindings = {{.mesh = mesh, .morph_mesh = morph_mesh}};
    MK_REQUIRE(d3d12_desc.morph_vertex_shader.entry_point == "vs_morph");
    MK_REQUIRE(d3d12_desc.morph_mesh_assets.size() == 1);
    MK_REQUIRE(d3d12_desc.morph_mesh_assets.front() == morph_mesh);
    MK_REQUIRE(d3d12_desc.morph_mesh_bindings.size() == 1);
    MK_REQUIRE(d3d12_desc.morph_mesh_bindings.front().mesh == mesh);
    MK_REQUIRE(d3d12_desc.morph_mesh_bindings.front().morph_mesh == morph_mesh);
    d3d12_desc.enable_compute_morph_tangent_frame_output = true;
    d3d12_desc.compute_morph_mesh_bindings = {{.mesh = mesh, .morph_mesh = morph_mesh}};
    d3d12_desc.compute_morph_skinned_shader = mirakana::SdlDesktopPresentationShaderBytecode{
        .entry_point = "cs_compute_morph_skinned_position",
        .bytecode = k_shader,
    };
    d3d12_desc.compute_morph_skinned_mesh_bindings = {{.mesh = mesh, .morph_mesh = morph_mesh}};
    MK_REQUIRE(d3d12_desc.enable_compute_morph_tangent_frame_output);
    MK_REQUIRE(d3d12_desc.compute_morph_mesh_bindings.front().mesh == mesh);
    MK_REQUIRE(d3d12_desc.compute_morph_mesh_bindings.front().morph_mesh == morph_mesh);
    MK_REQUIRE(d3d12_desc.compute_morph_skinned_shader.entry_point == "cs_compute_morph_skinned_position");
    MK_REQUIRE(d3d12_desc.compute_morph_skinned_mesh_bindings.front().mesh == mesh);
    MK_REQUIRE(d3d12_desc.compute_morph_skinned_mesh_bindings.front().morph_mesh == morph_mesh);

    mirakana::SdlDesktopPresentationVulkanSceneRendererDesc vulkan_desc;
    vulkan_desc.morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
        .entry_point = "vs_morph",
        .bytecode = k_shader,
    };
    vulkan_desc.morph_mesh_assets = {morph_mesh};
    vulkan_desc.morph_mesh_bindings = {{.mesh = mesh, .morph_mesh = morph_mesh}};
    MK_REQUIRE(vulkan_desc.morph_vertex_shader.entry_point == "vs_morph");
    MK_REQUIRE(vulkan_desc.morph_mesh_assets.size() == 1);
    MK_REQUIRE(vulkan_desc.morph_mesh_assets.front() == morph_mesh);
    MK_REQUIRE(vulkan_desc.morph_mesh_bindings.size() == 1);
    MK_REQUIRE(vulkan_desc.morph_mesh_bindings.front().mesh == mesh);
    MK_REQUIRE(vulkan_desc.morph_mesh_bindings.front().morph_mesh == morph_mesh);
    vulkan_desc.compute_morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
        .entry_point = "vs_compute_morph",
        .bytecode = k_shader,
    };
    vulkan_desc.compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode{
        .entry_point = "cs_compute_morph_position",
        .bytecode = k_shader,
    };
    vulkan_desc.enable_compute_morph_tangent_frame_output = true;
    vulkan_desc.compute_morph_mesh_bindings = {{.mesh = mesh, .morph_mesh = morph_mesh}};
    vulkan_desc.compute_morph_skinned_shader = mirakana::SdlDesktopPresentationShaderBytecode{
        .entry_point = "cs_compute_morph_skinned_position",
        .bytecode = k_shader,
    };
    vulkan_desc.compute_morph_skinned_mesh_bindings = {{.mesh = mesh, .morph_mesh = morph_mesh}};
    MK_REQUIRE(vulkan_desc.compute_morph_vertex_shader.entry_point == "vs_compute_morph");
    MK_REQUIRE(vulkan_desc.compute_morph_shader.entry_point == "cs_compute_morph_position");
    MK_REQUIRE(vulkan_desc.enable_compute_morph_tangent_frame_output);
    MK_REQUIRE(vulkan_desc.compute_morph_mesh_bindings.size() == 1);
    MK_REQUIRE(vulkan_desc.compute_morph_mesh_bindings.front().mesh == mesh);
    MK_REQUIRE(vulkan_desc.compute_morph_mesh_bindings.front().morph_mesh == morph_mesh);
    MK_REQUIRE(vulkan_desc.compute_morph_skinned_shader.entry_point == "cs_compute_morph_skinned_position");
    MK_REQUIRE(vulkan_desc.compute_morph_skinned_mesh_bindings.front().mesh == mesh);
    MK_REQUIRE(vulkan_desc.compute_morph_skinned_mesh_bindings.front().morph_mesh == morph_mesh);
}

MK_TEST("sdl desktop presentation renderer works through desktop runner resize") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL Presentation Runner Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
    });

    window.resize(mirakana::WindowExtent{.width = 400, .height = 220});

    mirakana::RingBufferLogger logger(16);
    mirakana::Registry registry;
    mirakana::VirtualLifecycle lifecycle;
    mirakana::DesktopHostServices services{
        .window = &window,
        .renderer = &presentation.renderer(),
        .lifecycle = &lifecycle,
    };
    mirakana::DesktopGameRunner runner(logger, registry);
    OneFrameDesktopApp app;
    const auto result = runner.run(app, services, mirakana::DesktopRunConfig{.max_frames = 1});

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::completed);
    MK_REQUIRE(result.frames_run == 1);
    MK_REQUIRE(app.updates == 1);
    MK_REQUIRE(presentation.renderer().backbuffer_extent().width == 400);
    MK_REQUIRE(presentation.renderer().backbuffer_extent().height == 220);
}

MK_TEST("sdl desktop game host runs a game through dummy windowed services") {
    mirakana::SdlDesktopGameHost host(mirakana::SdlDesktopGameHostDesc{
        .title = "SDL Desktop Game Host Test",
        .extent = mirakana::WindowExtent{.width = 320, .height = 180},
        .video_driver_hint = "dummy",
    });
    SdlDesktopGameHostProbeApp app(host.input(), host.renderer());

    const auto result = host.run(app, mirakana::DesktopRunConfig{.max_frames = 2});

    MK_REQUIRE(result.status == mirakana::DesktopRunStatus::completed);
    MK_REQUIRE(result.frames_run == 2);
    MK_REQUIRE(app.updates == 2);
    MK_REQUIRE(host.presentation_backend() == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(host.presentation_backend_name() == std::string_view{"null"});
    MK_REQUIRE(host.renderer().stats().frames_finished == 2);
    MK_REQUIRE(!host.presentation_diagnostics().empty());

    const auto report = host.presentation_report();
    MK_REQUIRE(report.requested_backend == mirakana::SdlDesktopPresentationBackend::d3d12);
    MK_REQUIRE(report.selected_backend == mirakana::SdlDesktopPresentationBackend::null_renderer);
    MK_REQUIRE(report.used_null_fallback);
    MK_REQUIRE(report.renderer_stats.frames_finished == 2);
    MK_REQUIRE(report.backend_reports_count == host.presentation_backend_reports().size());
    MK_REQUIRE(!host.presentation_backend_reports().empty());
}

#if defined(_WIN32)
MK_TEST("sdl desktop presentation can create d3d12 rhi frame renderer when shader bytecode is supplied") {
    if (!d3d12_presentation_test_enabled()) {
        return;
    }

    auto vertex_shader = compile_triangle_vertex_shader();
    auto fragment_shader = compile_triangle_pixel_shader();
    const auto* vertex_bytes = static_cast<const std::uint8_t*>(vertex_shader->GetBufferPointer());
    const auto* fragment_bytes = static_cast<const std::uint8_t*>(fragment_shader->GetBufferPointer());
    mirakana::SdlDesktopPresentationD3d12RendererDesc d3d12_renderer{
        .vertex_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "vs_main",
                .bytecode = std::span<const std::uint8_t>{vertex_bytes, vertex_shader->GetBufferSize()},
            },
        .fragment_shader =
            mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = "ps_main",
                .bytecode = std::span<const std::uint8_t>{fragment_bytes, fragment_shader->GetBufferSize()},
            },
    };

    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "windows"});
    mirakana::SdlWindow window(mirakana::WindowDesc{.title = "SDL D3D12 Presentation Test",
                                                    .extent = mirakana::WindowExtent{.width = 320, .height = 180}});
    mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
        .window = &window,
        .extent = mirakana::Extent2D{.width = 320, .height = 180},
        .d3d12_renderer = &d3d12_renderer,
    });

    MK_REQUIRE(presentation.backend() == mirakana::SdlDesktopPresentationBackend::d3d12);
    MK_REQUIRE(presentation.renderer().backend_name() == std::string_view{"d3d12"});
    MK_REQUIRE(presentation.diagnostics().empty());
    MK_REQUIRE(presentation.report().requested_backend == mirakana::SdlDesktopPresentationBackend::d3d12);
    MK_REQUIRE(presentation.report().selected_backend == mirakana::SdlDesktopPresentationBackend::d3d12);
    MK_REQUIRE(!presentation.report().used_null_fallback);
    MK_REQUIRE(presentation.backend_reports().size() == 1);
    MK_REQUIRE(presentation.backend_reports().front().status ==
               mirakana::SdlDesktopPresentationBackendReportStatus::ready);

    presentation.renderer().begin_frame();
    presentation.renderer().draw_sprite(mirakana::SpriteCommand{});
    presentation.renderer().end_frame();

    MK_REQUIRE(presentation.renderer().stats().frames_finished == 1);
}
#endif

MK_TEST("sdl desktop presentation rejects missing window without fallback") {
    bool rejected = false;
    try {
        mirakana::SdlDesktopPresentation presentation(mirakana::SdlDesktopPresentationDesc{
            .window = nullptr,
            .extent = mirakana::Extent2D{.width = 320, .height = 180},
            .allow_null_fallback = false,
        });
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    MK_REQUIRE(rejected);
}

int main() {
    return mirakana::test::run_all();
}
