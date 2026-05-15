// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"

#include "scene_gpu_binding_injecting_renderer.hpp"

#include "mirakana/renderer/rhi_directional_shadow_smoke_frame_renderer.hpp"
#include "mirakana/renderer/rhi_frame_renderer.hpp"
#include "mirakana/renderer/rhi_postprocess_frame_renderer.hpp"
#include "mirakana/rhi/rhi.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#if defined(MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_D3D12)
#include "mirakana/rhi/d3d12/d3d12_backend.hpp"
#endif

#if defined(MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_VULKAN)
#include "mirakana/rhi/vulkan/vulkan_backend.hpp"
#endif

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mirakana {
namespace {
using runtime_host_sdl3_detail::SceneComputeMorphMeshBinding;
using runtime_host_sdl3_detail::SceneGpuBindingInjectingRenderer;

struct SurfaceProbe {
    rhi::SurfaceHandle surface;
    SdlDesktopPresentationFallbackReason failure_reason{SdlDesktopPresentationFallbackReason::none};
    std::string diagnostic;
};

struct NativeRendererCreateResult {
    bool succeeded{false};
    SdlDesktopPresentationFallbackReason failure_reason{SdlDesktopPresentationFallbackReason::none};
    std::string diagnostic;
    std::unique_ptr<rhi::IRhiDevice> device;
    std::unique_ptr<IRenderer> renderer;
    SdlDesktopPresentationSceneGpuBindingStatus scene_gpu_status{
        SdlDesktopPresentationSceneGpuBindingStatus::not_requested};
    std::vector<SdlDesktopPresentationSceneGpuBindingDiagnostic> scene_gpu_diagnostics;
    SdlDesktopPresentationPostprocessStatus postprocess_status{SdlDesktopPresentationPostprocessStatus::not_requested};
    std::vector<SdlDesktopPresentationPostprocessDiagnostic> postprocess_diagnostics;
    bool postprocess_depth_input_requested{false};
    bool postprocess_depth_input_ready{false};
    SdlDesktopPresentationDirectionalShadowStatus directional_shadow_status{
        SdlDesktopPresentationDirectionalShadowStatus::not_requested};
    std::vector<SdlDesktopPresentationDirectionalShadowDiagnostic> directional_shadow_diagnostics;
    bool directional_shadow_requested{false};
    bool directional_shadow_ready{false};
    SdlDesktopPresentationDirectionalShadowFilterMode directional_shadow_filter_mode{
        SdlDesktopPresentationDirectionalShadowFilterMode::none};
    std::uint32_t directional_shadow_filter_tap_count{0};
    float directional_shadow_filter_radius_texels{0.0F};
    SdlDesktopPresentationNativeUiOverlayStatus native_ui_overlay_status{
        SdlDesktopPresentationNativeUiOverlayStatus::not_requested};
    std::vector<SdlDesktopPresentationNativeUiOverlayDiagnostic> native_ui_overlay_diagnostics;
    bool native_ui_overlay_requested{false};
    bool native_ui_overlay_ready{false};
    SdlDesktopPresentationNativeUiTextureOverlayStatus native_ui_texture_overlay_status{
        SdlDesktopPresentationNativeUiTextureOverlayStatus::not_requested};
    std::vector<SdlDesktopPresentationNativeUiTextureOverlayDiagnostic> native_ui_texture_overlay_diagnostics;
    bool native_ui_texture_overlay_requested{false};
    bool native_ui_texture_overlay_atlas_ready{false};
    std::size_t framegraph_passes{0};
    class SceneGpuBindingInjectingRenderer* scene_gpu_renderer{nullptr};
};

struct SceneRendererRequestValidation {
    bool valid{false};
    std::string diagnostic;
};

[[nodiscard]] SdlDesktopPresentationDirectionalShadowDiagnostic
make_directional_shadow_diagnostic(SdlDesktopPresentationDirectionalShadowStatus status, std::string message);
[[nodiscard]] SdlDesktopPresentationNativeUiOverlayDiagnostic
make_native_ui_overlay_diagnostic(SdlDesktopPresentationNativeUiOverlayStatus status, std::string message);
[[nodiscard]] SdlDesktopPresentationNativeUiTextureOverlayDiagnostic
make_native_ui_texture_overlay_diagnostic(SdlDesktopPresentationNativeUiTextureOverlayStatus status,
                                          std::string message);

[[nodiscard]] bool has_extent(Extent2D extent) noexcept {
    return extent.width != 0 && extent.height != 0;
}

[[nodiscard]] SdlDesktopPresentationDirectionalShadowFilterMode
to_presentation_filter_mode(ShadowReceiverFilterMode mode) noexcept {
    switch (mode) {
    case ShadowReceiverFilterMode::none:
        return SdlDesktopPresentationDirectionalShadowFilterMode::none;
    case ShadowReceiverFilterMode::fixed_pcf_3x3:
        return SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3;
    }
    return SdlDesktopPresentationDirectionalShadowFilterMode::none;
}

[[nodiscard]] bool has_shader_bytecode(const SdlDesktopPresentationShaderBytecode& shader) noexcept {
    return !shader.entry_point.empty() && !shader.bytecode.empty();
}

void append_unique_asset(std::vector<AssetId>& assets, AssetId asset) {
    if (asset.value == 0) {
        return;
    }
    if (std::ranges::find(assets, asset) == assets.end()) {
        assets.push_back(asset);
    }
}

void append_morph_binding_assets(std::vector<AssetId>& assets,
                                 std::span<const SdlDesktopPresentationSceneMorphMeshBinding> bindings) {
    for (const auto& binding : bindings) {
        append_unique_asset(assets, binding.morph_mesh);
    }
}

[[nodiscard]] bool has_morph_binding_for_mesh(std::span<const SdlDesktopPresentationSceneMorphMeshBinding> bindings,
                                              AssetId mesh) noexcept {
    return std::ranges::any_of(
        bindings, [mesh](const SdlDesktopPresentationSceneMorphMeshBinding& binding) { return binding.mesh == mesh; });
}

[[nodiscard]] std::vector<rhi::VertexBufferLayoutDesc> compute_morph_position_vertex_buffers() {
    return {rhi::VertexBufferLayoutDesc{
        .binding = 0,
        .stride = runtime_rhi::runtime_mesh_position_vertex_stride_bytes,
        .input_rate = rhi::VertexInputRate::vertex,
    }};
}

[[nodiscard]] std::vector<rhi::VertexBufferLayoutDesc> compute_morph_tangent_frame_vertex_buffers() {
    return {
        rhi::VertexBufferLayoutDesc{
            .binding = 0,
            .stride = runtime_rhi::runtime_mesh_position_vertex_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
        rhi::VertexBufferLayoutDesc{
            .binding = 1,
            .stride = runtime_rhi::runtime_morph_normal_delta_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
        rhi::VertexBufferLayoutDesc{
            .binding = 2,
            .stride = runtime_rhi::runtime_morph_tangent_delta_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
    };
}

[[nodiscard]] std::vector<rhi::VertexBufferLayoutDesc> compute_morph_skinned_vertex_buffers() {
    return {
        rhi::VertexBufferLayoutDesc{
            .binding = 0,
            .stride = runtime_rhi::runtime_mesh_position_vertex_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
        rhi::VertexBufferLayoutDesc{
            .binding = 3,
            .stride = runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes,
            .input_rate = rhi::VertexInputRate::vertex,
        },
    };
}

[[nodiscard]] std::vector<rhi::VertexAttributeDesc> compute_morph_position_vertex_attributes() {
    return {rhi::VertexAttributeDesc{
        .location = 0,
        .binding = 0,
        .offset = 0,
        .format = rhi::VertexFormat::float32x3,
        .semantic = rhi::VertexSemantic::position,
        .semantic_index = 0,
    }};
}

[[nodiscard]] std::vector<rhi::VertexAttributeDesc> compute_morph_skinned_vertex_attributes() {
    return {
        rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = rhi::VertexFormat::float32x3,
            .semantic = rhi::VertexSemantic::position,
            .semantic_index = 0,
        },
        rhi::VertexAttributeDesc{
            .location = 1,
            .binding = 3,
            .offset = 48,
            .format = rhi::VertexFormat::uint16x4,
            .semantic = rhi::VertexSemantic::joint_indices,
            .semantic_index = 0,
        },
        rhi::VertexAttributeDesc{
            .location = 2,
            .binding = 3,
            .offset = 56,
            .format = rhi::VertexFormat::float32x4,
            .semantic = rhi::VertexSemantic::joint_weights,
            .semantic_index = 0,
        },
    };
}

[[nodiscard]] std::vector<rhi::VertexAttributeDesc> compute_morph_tangent_frame_vertex_attributes() {
    return {
        rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = rhi::VertexFormat::float32x3,
            .semantic = rhi::VertexSemantic::position,
            .semantic_index = 0,
        },
        rhi::VertexAttributeDesc{
            .location = 1,
            .binding = 1,
            .offset = 0,
            .format = rhi::VertexFormat::float32x3,
            .semantic = rhi::VertexSemantic::normal,
            .semantic_index = 0,
        },
        rhi::VertexAttributeDesc{
            .location = 2,
            .binding = 2,
            .offset = 0,
            .format = rhi::VertexFormat::float32x3,
            .semantic = rhi::VertexSemantic::tangent,
            .semantic_index = 0,
        },
    };
}

[[nodiscard]] const runtime_scene_rhi::RuntimeSceneMeshGpuResource*
find_uploaded_scene_mesh(const runtime_scene_rhi::RuntimeSceneGpuBindingResult& bindings, AssetId mesh) noexcept {
    const auto it = std::ranges::find_if(
        bindings.mesh_uploads,
        [mesh](const runtime_scene_rhi::RuntimeSceneMeshGpuResource& upload) { return upload.mesh == mesh; });
    return it == bindings.mesh_uploads.end() ? nullptr : &*it;
}

[[nodiscard]] const runtime_scene_rhi::RuntimeSceneMorphMeshGpuResource*
find_uploaded_scene_morph_mesh(const runtime_scene_rhi::RuntimeSceneGpuBindingResult& bindings,
                               AssetId morph_mesh) noexcept {
    const auto it = std::ranges::find_if(
        bindings.morph_mesh_uploads, [morph_mesh](const runtime_scene_rhi::RuntimeSceneMorphMeshGpuResource& upload) {
            return upload.morph_mesh == morph_mesh;
        });
    return it == bindings.morph_mesh_uploads.end() ? nullptr : &*it;
}

struct SceneComputeMorphBuildResult {
    std::vector<SceneComputeMorphMeshBinding> bindings;
    std::string diagnostic;
    std::size_t queue_waits{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

struct SceneComputeMorphDispatchResult {
    std::string diagnostic;
    std::size_t dispatches{0};
    std::size_t queue_waits{0};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

[[nodiscard]] bool has_postprocess_bytecode(const SdlDesktopPresentationShaderBytecode& vertex,
                                            const SdlDesktopPresentationShaderBytecode& fragment) noexcept {
    return has_shader_bytecode(vertex) && has_shader_bytecode(fragment);
}

[[nodiscard]] bool has_directional_shadow_bytecode(const SdlDesktopPresentationShaderBytecode& vertex,
                                                   const SdlDesktopPresentationShaderBytecode& fragment) noexcept {
    return has_shader_bytecode(vertex) && has_shader_bytecode(fragment);
}

[[nodiscard]] bool has_native_ui_overlay_bytecode(const SdlDesktopPresentationShaderBytecode& vertex,
                                                  const SdlDesktopPresentationShaderBytecode& fragment) noexcept {
    return has_shader_bytecode(vertex) && has_shader_bytecode(fragment);
}

[[nodiscard]] bool valid_d3d12_renderer_request(const SdlDesktopPresentationD3d12RendererDesc* desc) noexcept {
    if (desc == nullptr || !has_shader_bytecode(desc->vertex_shader) || !has_shader_bytecode(desc->fragment_shader)) {
        return false;
    }
    if (desc->enable_native_sprite_overlay &&
        !has_native_ui_overlay_bytecode(desc->native_sprite_overlay_vertex_shader,
                                        desc->native_sprite_overlay_fragment_shader)) {
        return false;
    }
    if (desc->enable_native_sprite_overlay_textures &&
        (!desc->enable_native_sprite_overlay || desc->native_sprite_overlay_package == nullptr ||
         desc->native_sprite_overlay_atlas_asset.value == 0)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool valid_vulkan_renderer_request(const SdlDesktopPresentationVulkanRendererDesc* desc) noexcept {
    if (desc == nullptr || !has_shader_bytecode(desc->vertex_shader) || !has_shader_bytecode(desc->fragment_shader)) {
        return false;
    }
    if (desc->enable_native_sprite_overlay &&
        !has_native_ui_overlay_bytecode(desc->native_sprite_overlay_vertex_shader,
                                        desc->native_sprite_overlay_fragment_shader)) {
        return false;
    }
    if (desc->enable_native_sprite_overlay_textures &&
        (!desc->enable_native_sprite_overlay || desc->native_sprite_overlay_package == nullptr ||
         desc->native_sprite_overlay_atlas_asset.value == 0)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool same_vertex_buffer_layout(const rhi::VertexBufferLayoutDesc& lhs,
                                             const rhi::VertexBufferLayoutDesc& rhs) noexcept {
    return lhs.binding == rhs.binding && lhs.stride == rhs.stride && lhs.input_rate == rhs.input_rate;
}

[[nodiscard]] bool same_vertex_attribute(const rhi::VertexAttributeDesc& lhs,
                                         const rhi::VertexAttributeDesc& rhs) noexcept {
    return lhs.location == rhs.location && lhs.binding == rhs.binding && lhs.offset == rhs.offset &&
           lhs.format == rhs.format && lhs.semantic == rhs.semantic && lhs.semantic_index == rhs.semantic_index;
}

[[nodiscard]] bool same_vertex_buffer_layouts(std::span<const rhi::VertexBufferLayoutDesc> lhs,
                                              std::span<const rhi::VertexBufferLayoutDesc> rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (const auto& expected : rhs) {
        const auto found = std::ranges::find_if(lhs, [&expected](const rhi::VertexBufferLayoutDesc& item) {
            return same_vertex_buffer_layout(item, expected);
        });
        if (found == lhs.end()) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool same_vertex_attributes(std::span<const rhi::VertexAttributeDesc> lhs,
                                          std::span<const rhi::VertexAttributeDesc> rhs) noexcept {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (const auto& expected : rhs) {
        const auto found = std::ranges::find_if(
            lhs, [&expected](const rhi::VertexAttributeDesc& item) { return same_vertex_attribute(item, expected); });
        if (found == lhs.end()) {
            return false;
        }
    }
    return true;
}

void append_unique_mesh(std::vector<AssetId>& meshes, AssetId mesh) {
    const auto found = std::ranges::find(meshes, mesh);
    if (found == meshes.end()) {
        meshes.push_back(mesh);
    }
}

[[nodiscard]] bool scene_packet_references_skinned_mesh(const runtime::RuntimeAssetPackage& package,
                                                        const SceneRenderPacket& packet) noexcept {
    for (const auto& mesh : packet.meshes) {
        const auto* record = package.find(mesh.renderer.mesh);
        if (record != nullptr && record->kind == AssetKind::skinned_mesh) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] SceneRendererRequestValidation validate_scene_renderer_mesh_layout_request(
    const runtime::RuntimeAssetPackage& package, const SceneRenderPacket& packet,
    std::span<const rhi::VertexBufferLayoutDesc> static_vertex_buffers,
    std::span<const rhi::VertexAttributeDesc> static_vertex_attributes,
    std::span<const rhi::VertexBufferLayoutDesc> skinned_vertex_buffers,
    std::span<const rhi::VertexAttributeDesc> skinned_vertex_attributes,
    std::span<const SdlDesktopPresentationSceneMorphMeshBinding> compute_morph_skinned_mesh_bindings,
    std::string_view backend_name) {
    std::vector<AssetId> meshes;
    for (const auto& mesh : packet.meshes) {
        append_unique_mesh(meshes, mesh.renderer.mesh);
    }
    for (const auto mesh : meshes) {
        const auto* record = package.find(mesh);
        if (record == nullptr) {
            return SceneRendererRequestValidation{
                .valid = false,
                .diagnostic = std::string(backend_name) +
                              " scene renderer request references a mesh without cooked payload layout metadata.",
            };
        }
        if (record->kind == AssetKind::mesh) {
            const auto payload = runtime::runtime_mesh_payload(*record);
            if (!payload.succeeded()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request mesh payload is invalid: " + payload.diagnostic,
                };
            }
            const auto layout = runtime_rhi::make_runtime_mesh_vertex_layout_desc(payload.payload);
            if (!layout.succeeded()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request mesh layout is invalid: " + layout.diagnostic,
                };
            }
            if (!same_vertex_buffer_layouts(static_vertex_buffers, layout.vertex_buffers) ||
                !same_vertex_attributes(static_vertex_attributes, layout.vertex_attributes)) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request vertex input does not match cooked mesh payload layout.",
                };
            }
            continue;
        }
        if (record->kind == AssetKind::skinned_mesh) {
            if (skinned_vertex_buffers.empty() || skinned_vertex_attributes.empty()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic =
                        std::string(backend_name) +
                        " scene renderer request references a skinned mesh but does not provide skinned vertex input.",
                };
            }
            const auto payload = runtime::runtime_skinned_mesh_payload(*record);
            if (!payload.succeeded()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request skinned mesh payload is invalid: " + payload.diagnostic,
                };
            }
            const auto layout = runtime_rhi::make_runtime_skinned_mesh_vertex_layout_desc(payload.payload);
            if (!layout.succeeded()) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic = std::string(backend_name) +
                                  " scene renderer request skinned mesh layout is invalid: " + layout.diagnostic,
                };
            }
            auto expected_vertex_buffers = layout.vertex_buffers;
            auto expected_vertex_attributes = layout.vertex_attributes;
            if (has_morph_binding_for_mesh(compute_morph_skinned_mesh_bindings, mesh)) {
                expected_vertex_buffers = compute_morph_skinned_vertex_buffers();
                expected_vertex_attributes = compute_morph_skinned_vertex_attributes();
            }
            if (!same_vertex_buffer_layouts(skinned_vertex_buffers, expected_vertex_buffers) ||
                !same_vertex_attributes(skinned_vertex_attributes, expected_vertex_attributes)) {
                return SceneRendererRequestValidation{
                    .valid = false,
                    .diagnostic =
                        std::string(backend_name) +
                        " scene renderer request skinned vertex input does not match cooked skinned mesh layout.",
                };
            }
            continue;
        }
        return SceneRendererRequestValidation{
            .valid = false,
            .diagnostic =
                std::string(backend_name) + " scene renderer request references an unsupported mesh asset kind.",
        };
    }
    return SceneRendererRequestValidation{.valid = true, .diagnostic = {}};
}

[[nodiscard]] bool
valid_d3d12_scene_renderer_request(const SdlDesktopPresentationD3d12SceneRendererDesc* desc) noexcept {
    if (desc == nullptr || !has_shader_bytecode(desc->vertex_shader) || !has_shader_bytecode(desc->fragment_shader) ||
        desc->package == nullptr || desc->packet == nullptr || desc->packet->meshes.empty() ||
        desc->vertex_buffers.empty() || desc->vertex_attributes.empty()) {
        return false;
    }
    if (scene_packet_references_skinned_mesh(*desc->package, *desc->packet)) {
        if (!has_shader_bytecode(desc->skinned_vertex_shader) || desc->skinned_vertex_buffers.empty() ||
            desc->skinned_vertex_attributes.empty()) {
            return false;
        }
        if (!desc->morph_mesh_bindings.empty()) {
            return false;
        }
        if (!desc->compute_morph_mesh_bindings.empty()) {
            return false;
        }
        if (!desc->compute_morph_skinned_mesh_bindings.empty() &&
            (!has_shader_bytecode(desc->compute_morph_skinned_shader) || desc->enable_directional_shadow_smoke)) {
            return false;
        }
        if (desc->enable_directional_shadow_smoke && !has_shader_bytecode(desc->skinned_scene_fragment_shader)) {
            return false;
        }
    } else if (!desc->compute_morph_skinned_mesh_bindings.empty()) {
        return false;
    }
    if (!desc->morph_mesh_bindings.empty() && !has_shader_bytecode(desc->morph_vertex_shader)) {
        return false;
    }
    if (!desc->morph_mesh_bindings.empty() && desc->enable_directional_shadow_smoke &&
        !has_shader_bytecode(desc->skinned_scene_fragment_shader)) {
        return false;
    }
    if (!desc->compute_morph_mesh_bindings.empty() &&
        (!has_shader_bytecode(desc->compute_morph_vertex_shader) || !has_shader_bytecode(desc->compute_morph_shader) ||
         desc->enable_directional_shadow_smoke)) {
        return false;
    }
    return true;
}

[[nodiscard]] bool
valid_vulkan_scene_renderer_request(const SdlDesktopPresentationVulkanSceneRendererDesc* desc) noexcept {
    if (desc == nullptr || !has_shader_bytecode(desc->vertex_shader) || !has_shader_bytecode(desc->fragment_shader) ||
        desc->package == nullptr || desc->packet == nullptr || desc->packet->meshes.empty() ||
        desc->vertex_buffers.empty() || desc->vertex_attributes.empty()) {
        return false;
    }
    if (scene_packet_references_skinned_mesh(*desc->package, *desc->packet)) {
        if (!has_shader_bytecode(desc->skinned_vertex_shader) || desc->skinned_vertex_buffers.empty() ||
            desc->skinned_vertex_attributes.empty()) {
            return false;
        }
        if (!desc->morph_mesh_bindings.empty()) {
            return false;
        }
        if (!desc->compute_morph_mesh_bindings.empty()) {
            return false;
        }
        if (!desc->compute_morph_skinned_mesh_bindings.empty() &&
            (!has_shader_bytecode(desc->compute_morph_skinned_shader) || desc->enable_directional_shadow_smoke)) {
            return false;
        }
        if (desc->enable_directional_shadow_smoke && !has_shader_bytecode(desc->skinned_scene_fragment_shader)) {
            return false;
        }
    } else if (!desc->compute_morph_skinned_mesh_bindings.empty()) {
        return false;
    }
    if (!desc->morph_mesh_bindings.empty() && !has_shader_bytecode(desc->morph_vertex_shader)) {
        return false;
    }
    if (!desc->morph_mesh_bindings.empty() && desc->enable_directional_shadow_smoke &&
        !has_shader_bytecode(desc->skinned_scene_fragment_shader)) {
        return false;
    }
    if (!desc->compute_morph_mesh_bindings.empty() &&
        (!has_shader_bytecode(desc->compute_morph_vertex_shader) || !has_shader_bytecode(desc->compute_morph_shader) ||
         desc->enable_directional_shadow_smoke)) {
        return false;
    }
    return true;
}

[[nodiscard]] SdlDesktopPresentationBackend
requested_backend_from_desc(const SdlDesktopPresentationDesc& desc) noexcept {
    if (desc.prefer_vulkan) {
        return SdlDesktopPresentationBackend::vulkan;
    }
    if (desc.prefer_d3d12) {
        return SdlDesktopPresentationBackend::d3d12;
    }
    return SdlDesktopPresentationBackend::null_renderer;
}

[[nodiscard]] SdlDesktopPresentationBackendReportStatus
backend_report_status_from_fallback_reason(SdlDesktopPresentationFallbackReason reason) noexcept {
    switch (reason) {
    case SdlDesktopPresentationFallbackReason::none:
        return SdlDesktopPresentationBackendReportStatus::ready;
    case SdlDesktopPresentationFallbackReason::native_surface_unavailable:
        return SdlDesktopPresentationBackendReportStatus::native_surface_unavailable;
    case SdlDesktopPresentationFallbackReason::native_backend_unavailable:
        return SdlDesktopPresentationBackendReportStatus::native_backend_unavailable;
    case SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable:
        return SdlDesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
    }
    return SdlDesktopPresentationBackendReportStatus::runtime_pipeline_unavailable;
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_renderer_request() {
    return NativeRendererCreateResult{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "D3D12 renderer creation requires non-empty vertex and fragment shader bytecode; using "
                      "NullRenderer fallback.",
    };
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_renderer_request() {
    return NativeRendererCreateResult{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic =
            "Vulkan renderer creation requires non-empty vertex and fragment SPIR-V bytecode; using NullRenderer "
            "fallback.",
    };
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_scene_renderer_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "D3D12 scene renderer creation requires non-empty shader bytecode, package, render packet, and "
                      "vertex input "
                      "metadata; using NullRenderer fallback.",
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    return result;
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_postprocess_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "D3D12 scene postprocess creation requires non-empty postprocess vertex and fragment shader "
                      "bytecode; using "
                      "NullRenderer fallback.",
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.postprocess_status = SdlDesktopPresentationPostprocessStatus::invalid_request;
    result.postprocess_diagnostics.push_back(SdlDesktopPresentationPostprocessDiagnostic{
        .status = result.postprocess_status,
        .message = result.diagnostic,
    });
    return result;
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_scene_renderer_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "Vulkan scene renderer creation requires non-empty SPIR-V shader bytecode, package, render "
                      "packet, and vertex "
                      "input metadata; using NullRenderer fallback.",
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    return result;
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_postprocess_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "Vulkan scene postprocess creation requires non-empty postprocess vertex and fragment SPIR-V "
                      "bytecode; using "
                      "NullRenderer fallback.",
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.postprocess_status = SdlDesktopPresentationPostprocessStatus::invalid_request;
    result.postprocess_diagnostics.push_back(SdlDesktopPresentationPostprocessDiagnostic{
        .status = result.postprocess_status,
        .message = result.diagnostic,
    });
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_d3d12_postprocess_depth_input_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "D3D12 scene postprocess depth input requires scene postprocess to be enabled; using "
                      "NullRenderer fallback.",
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.postprocess_status = SdlDesktopPresentationPostprocessStatus::invalid_request;
    result.postprocess_diagnostics.push_back(SdlDesktopPresentationPostprocessDiagnostic{
        .status = result.postprocess_status,
        .message = result.diagnostic,
    });
    result.postprocess_depth_input_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_vulkan_postprocess_depth_input_request() {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = "Vulkan scene postprocess depth input requires scene postprocess to be enabled; using "
                      "NullRenderer fallback.",
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.postprocess_status = SdlDesktopPresentationPostprocessStatus::invalid_request;
    result.postprocess_diagnostics.push_back(SdlDesktopPresentationPostprocessDiagnostic{
        .status = result.postprocess_status,
        .message = result.diagnostic,
    });
    result.postprocess_depth_input_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_d3d12_directional_shadow_request(std::string message) {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = std::move(message),
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.directional_shadow_status = SdlDesktopPresentationDirectionalShadowStatus::invalid_request;
    result.directional_shadow_diagnostics.push_back(
        make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
    result.directional_shadow_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_vulkan_directional_shadow_request(std::string message) {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = std::move(message),
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.directional_shadow_status = SdlDesktopPresentationDirectionalShadowStatus::invalid_request;
    result.directional_shadow_diagnostics.push_back(
        make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
    result.directional_shadow_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_d3d12_native_ui_overlay_request(std::string message) {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = std::move(message),
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::invalid_request;
    result.native_ui_overlay_diagnostics.push_back(
        make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
    result.native_ui_overlay_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_d3d12_native_ui_texture_overlay_request(std::string message) {
    auto result = invalid_d3d12_native_ui_overlay_request(std::move(message));
    result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::invalid_request;
    result.native_ui_texture_overlay_diagnostics.push_back(
        make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
    result.native_ui_texture_overlay_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_vulkan_native_ui_overlay_request(std::string message) {
    NativeRendererCreateResult result{
        .succeeded = false,
        .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
        .diagnostic = std::move(message),
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
    result.scene_gpu_diagnostics.push_back(SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = result.scene_gpu_status,
        .message = result.diagnostic,
    });
    result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::invalid_request;
    result.native_ui_overlay_diagnostics.push_back(
        make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
    result.native_ui_overlay_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult invalid_vulkan_native_ui_texture_overlay_request(std::string message) {
    auto result = invalid_vulkan_native_ui_overlay_request(std::move(message));
    result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::invalid_request;
    result.native_ui_texture_overlay_diagnostics.push_back(
        make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
    result.native_ui_texture_overlay_requested = true;
    return result;
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_directional_shadow_request() {
    return invalid_d3d12_directional_shadow_request(
        "D3D12 directional shadow smoke requires non-empty shadow vertex and fragment shader bytecode; using "
        "NullRenderer fallback.");
}

[[nodiscard]] NativeRendererCreateResult missing_d3d12_native_ui_overlay_request() {
    return invalid_d3d12_native_ui_overlay_request(
        "D3D12 native UI overlay requires non-empty overlay vertex and fragment shader bytecode; using NullRenderer "
        "fallback.");
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_native_ui_overlay_request() {
    return invalid_vulkan_native_ui_overlay_request(
        "Vulkan native UI overlay requires non-empty overlay vertex and fragment SPIR-V bytecode; using "
        "NullRenderer fallback.");
}

[[nodiscard]] NativeRendererCreateResult missing_vulkan_directional_shadow_request() {
    return invalid_vulkan_directional_shadow_request(
        "Vulkan directional shadow smoke requires non-empty shadow vertex and fragment SPIR-V bytecode; using "
        "NullRenderer fallback.");
}

[[nodiscard]] SurfaceProbe probe_d3d12_surface(const SdlWindow& window) {
    const auto native = window.native_window();
    if (native.value == 0) {
        return SurfaceProbe{
            .surface = {},
            .failure_reason = SdlDesktopPresentationFallbackReason::native_surface_unavailable,
            .diagnostic = "SDL window handle is unavailable; using NullRenderer fallback.",
        };
    }

#if defined(SDL_PLATFORM_WIN32)
    auto* sdl_window = reinterpret_cast<SDL_Window*>(native.value);
    const SDL_PropertiesID properties = SDL_GetWindowProperties(sdl_window);
    if (properties == 0) {
        return SurfaceProbe{
            .surface = {},
            .failure_reason = SdlDesktopPresentationFallbackReason::native_surface_unavailable,
            .diagnostic = std::string{"SDL window properties are unavailable: "} + SDL_GetError() +
                          "; using NullRenderer fallback.",
        };
    }

    void* hwnd = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (hwnd == nullptr) {
        return SurfaceProbe{
            .surface = {},
            .failure_reason = SdlDesktopPresentationFallbackReason::native_surface_unavailable,
            .diagnostic =
                "SDL window does not expose a Win32 HWND for D3D12 presentation; using NullRenderer fallback.",
        };
    }

    return SurfaceProbe{
        .surface = rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(hwnd)},
        .failure_reason = SdlDesktopPresentationFallbackReason::none,
        .diagnostic = {},
    };
#else
    (void)window;
    return SurfaceProbe{
        {},
        SdlDesktopPresentationFallbackReason::native_surface_unavailable,
        "D3D12 presentation requires a Win32 SDL window surface; using NullRenderer fallback.",
    };
#endif
}

[[nodiscard]] SurfaceProbe probe_vulkan_surface(const SdlWindow& window) {
    const auto native = window.native_window();
    if (native.value == 0) {
        return SurfaceProbe{
            .surface = {},
            .failure_reason = SdlDesktopPresentationFallbackReason::native_surface_unavailable,
            .diagnostic = "SDL window handle is unavailable for Vulkan presentation; using NullRenderer fallback.",
        };
    }

#if defined(SDL_PLATFORM_WIN32)
    auto* sdl_window = reinterpret_cast<SDL_Window*>(native.value);
    const SDL_PropertiesID properties = SDL_GetWindowProperties(sdl_window);
    if (properties == 0) {
        return SurfaceProbe{
            .surface = {},
            .failure_reason = SdlDesktopPresentationFallbackReason::native_surface_unavailable,
            .diagnostic = std::string{"SDL window properties are unavailable for Vulkan presentation: "} +
                          SDL_GetError() + "; using NullRenderer fallback.",
        };
    }

    void* hwnd = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (hwnd == nullptr) {
        return SurfaceProbe{
            .surface = {},
            .failure_reason = SdlDesktopPresentationFallbackReason::native_surface_unavailable,
            .diagnostic =
                "SDL window does not expose a Win32 HWND for Vulkan presentation; using NullRenderer fallback.",
        };
    }

    return SurfaceProbe{
        .surface = rhi::SurfaceHandle{reinterpret_cast<std::uintptr_t>(hwnd)},
        .failure_reason = SdlDesktopPresentationFallbackReason::none,
        .diagnostic = {},
    };
#else
    (void)window;
    return SurfaceProbe{
        {},
        SdlDesktopPresentationFallbackReason::native_surface_unavailable,
        "Vulkan presentation currently requires a Win32 SDL window surface; using NullRenderer fallback.",
    };
#endif
}

[[nodiscard]] SdlDesktopPresentationDiagnostic make_diagnostic(SdlDesktopPresentationFallbackReason reason,
                                                               std::string message) {
    return SdlDesktopPresentationDiagnostic{
        .reason = reason,
        .message = std::move(message),
    };
}

[[nodiscard]] SdlDesktopPresentationSceneGpuBindingDiagnostic
make_scene_gpu_diagnostic(SdlDesktopPresentationSceneGpuBindingStatus status, std::string message) {
    return SdlDesktopPresentationSceneGpuBindingDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

[[nodiscard]] SdlDesktopPresentationPostprocessDiagnostic
make_postprocess_diagnostic(SdlDesktopPresentationPostprocessStatus status, std::string message) {
    return SdlDesktopPresentationPostprocessDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

[[nodiscard]] SdlDesktopPresentationDirectionalShadowDiagnostic
make_directional_shadow_diagnostic(SdlDesktopPresentationDirectionalShadowStatus status, std::string message) {
    return SdlDesktopPresentationDirectionalShadowDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

[[nodiscard]] SdlDesktopPresentationNativeUiOverlayDiagnostic
make_native_ui_overlay_diagnostic(SdlDesktopPresentationNativeUiOverlayStatus status, std::string message) {
    return SdlDesktopPresentationNativeUiOverlayDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

[[nodiscard]] SdlDesktopPresentationNativeUiTextureOverlayDiagnostic
make_native_ui_texture_overlay_diagnostic(SdlDesktopPresentationNativeUiTextureOverlayStatus status,
                                          std::string message) {
    return SdlDesktopPresentationNativeUiTextureOverlayDiagnostic{
        .status = status,
        .message = std::move(message),
    };
}

void mark_directional_shadow_result(NativeRendererCreateResult& result,
                                    SdlDesktopPresentationDirectionalShadowStatus status, std::string message) {
    result.directional_shadow_status = status;
    result.directional_shadow_requested = true;
    result.directional_shadow_ready = false;
    result.directional_shadow_diagnostics.push_back(make_directional_shadow_diagnostic(status, std::move(message)));
}

void mark_native_ui_overlay_result(NativeRendererCreateResult& result,
                                   SdlDesktopPresentationNativeUiOverlayStatus status, std::string message) {
    result.native_ui_overlay_status = status;
    result.native_ui_overlay_requested = true;
    result.native_ui_overlay_ready = false;
    result.native_ui_overlay_diagnostics.push_back(make_native_ui_overlay_diagnostic(status, std::move(message)));
}

void mark_native_ui_texture_overlay_result(NativeRendererCreateResult& result,
                                           SdlDesktopPresentationNativeUiTextureOverlayStatus status,
                                           std::string message) {
    result.native_ui_texture_overlay_status = status;
    result.native_ui_texture_overlay_requested = true;
    result.native_ui_texture_overlay_atlas_ready = false;
    result.native_ui_texture_overlay_diagnostics.push_back(
        make_native_ui_texture_overlay_diagnostic(status, std::move(message)));
}

[[nodiscard]] SceneComputeMorphBuildResult
build_scene_compute_morph_bindings(rhi::IRhiDevice& device,
                                   const runtime_scene_rhi::RuntimeSceneGpuBindingResult& gpu_bindings,
                                   std::span<const SdlDesktopPresentationSceneMorphMeshBinding> requested_bindings,
                                   const SdlDesktopPresentationShaderBytecode& compute_shader_bytecode,
                                   bool enable_tangent_frame_output, std::string_view backend_name) {
    SceneComputeMorphBuildResult result;
    if (requested_bindings.empty()) {
        return result;
    }
    if (!has_shader_bytecode(compute_shader_bytecode)) {
        result.diagnostic =
            std::string{backend_name} + " scene compute morph requires non-empty compute shader bytecode";
        return result;
    }

    const auto compute_shader = device.create_shader(rhi::ShaderDesc{
        .stage = rhi::ShaderStage::compute,
        .entry_point = compute_shader_bytecode.entry_point,
        .bytecode_size = compute_shader_bytecode.bytecode.size(),
        .bytecode = compute_shader_bytecode.bytecode.data(),
    });

    result.bindings.reserve(requested_bindings.size());
    for (const auto& requested : requested_bindings) {
        const auto* mesh_upload = find_uploaded_scene_mesh(gpu_bindings, requested.mesh);
        if (mesh_upload == nullptr) {
            result.diagnostic = std::string{backend_name} +
                                " scene compute morph requested a mesh that was not uploaded by the scene GPU palette";
            return result;
        }
        const auto* morph_upload = find_uploaded_scene_morph_mesh(gpu_bindings, requested.morph_mesh);
        if (morph_upload == nullptr) {
            result.diagnostic =
                std::string{backend_name} +
                " scene compute morph requested a morph mesh that was not uploaded by the scene GPU palette";
            return result;
        }

        runtime_rhi::RuntimeMorphMeshComputeBindingOptions options;
        options.output_position_usage =
            rhi::BufferUsage::storage | rhi::BufferUsage::copy_source | rhi::BufferUsage::vertex;
        if (enable_tangent_frame_output) {
            options.output_normal_usage =
                rhi::BufferUsage::storage | rhi::BufferUsage::copy_source | rhi::BufferUsage::vertex;
            options.output_tangent_usage =
                rhi::BufferUsage::storage | rhi::BufferUsage::copy_source | rhi::BufferUsage::vertex;
        }
        const auto compute_binding = runtime_rhi::create_runtime_morph_mesh_compute_binding(
            device, mesh_upload->upload, morph_upload->upload, options);
        if (!compute_binding.succeeded()) {
            result.diagnostic =
                std::string{backend_name} + " scene compute morph binding failed: " + compute_binding.diagnostic;
            return result;
        }

        const auto compute_layout = device.create_pipeline_layout(rhi::PipelineLayoutDesc{
            .descriptor_sets = {compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
        const auto compute_pipeline = device.create_compute_pipeline(
            rhi::ComputePipelineDesc{.layout = compute_layout, .compute_shader = compute_shader});
        auto commands = device.begin_command_list(rhi::QueueKind::compute);
        commands->bind_compute_pipeline(compute_pipeline);
        commands->bind_descriptor_set(compute_layout, 0, compute_binding.descriptor_set);
        commands->dispatch(compute_binding.vertex_count, 1, 1);
        commands->close();
        const auto fence = device.submit(*commands);
        device.wait_for_queue(rhi::QueueKind::graphics, fence);
        ++result.queue_waits;

        const auto output_mesh_binding =
            enable_tangent_frame_output
                ? runtime_rhi::make_runtime_compute_morph_tangent_frame_output_mesh_gpu_binding(mesh_upload->upload,
                                                                                                compute_binding)
                : runtime_rhi::make_runtime_compute_morph_output_mesh_gpu_binding(mesh_upload->upload, compute_binding);
        if (output_mesh_binding.vertex_buffer.value == 0 || output_mesh_binding.owner_device != &device) {
            result.diagnostic = std::string{backend_name} + " scene compute morph output mesh binding failed";
            return result;
        }
        result.bindings.push_back(SceneComputeMorphMeshBinding{
            .mesh = requested.mesh,
            .morph_mesh = requested.morph_mesh,
            .mesh_binding = output_mesh_binding,
        });
    }
    return result;
}

[[nodiscard]] SceneComputeMorphDispatchResult dispatch_scene_compute_morph_skinned_bindings(
    rhi::IRhiDevice& device,
    std::span<const runtime_scene_rhi::RuntimeSceneComputeMorphSkinnedMeshGpuResource> bindings,
    const SdlDesktopPresentationShaderBytecode& compute_shader_bytecode, std::string_view backend_name) {
    SceneComputeMorphDispatchResult result;
    if (bindings.empty()) {
        return result;
    }
    if (!has_shader_bytecode(compute_shader_bytecode)) {
        result.diagnostic =
            std::string{backend_name} + " scene compute morph skinned path requires non-empty compute shader bytecode";
        return result;
    }

    const auto compute_shader = device.create_shader(rhi::ShaderDesc{
        .stage = rhi::ShaderStage::compute,
        .entry_point = compute_shader_bytecode.entry_point,
        .bytecode_size = compute_shader_bytecode.bytecode.size(),
        .bytecode = compute_shader_bytecode.bytecode.data(),
    });

    for (const auto& binding : bindings) {
        if (!binding.compute_binding.succeeded() || binding.compute_binding.descriptor_set_layout.value == 0 ||
            binding.compute_binding.descriptor_set.value == 0 || binding.compute_binding.vertex_count == 0) {
            result.diagnostic = std::string{backend_name} + " scene compute morph skinned binding is incomplete";
            return result;
        }
        const auto compute_layout = device.create_pipeline_layout(rhi::PipelineLayoutDesc{
            .descriptor_sets = {binding.compute_binding.descriptor_set_layout}, .push_constant_bytes = 0});
        const auto compute_pipeline = device.create_compute_pipeline(
            rhi::ComputePipelineDesc{.layout = compute_layout, .compute_shader = compute_shader});
        auto commands = device.begin_command_list(rhi::QueueKind::compute);
        commands->bind_compute_pipeline(compute_pipeline);
        commands->bind_descriptor_set(compute_layout, 0, binding.compute_binding.descriptor_set);
        commands->dispatch(binding.compute_binding.vertex_count, 1, 1);
        commands->close();
        const auto fence = device.submit(*commands);
        device.wait_for_queue(rhi::QueueKind::graphics, fence);
        ++result.dispatches;
        ++result.queue_waits;
    }
    return result;
}

[[nodiscard]] NativeUiOverlayAtlasBinding
make_native_ui_overlay_atlas_binding(rhi::IRhiDevice& device, const runtime::RuntimeAssetPackage& package,
                                     AssetId atlas_asset, std::string_view backend_name,
                                     std::vector<SdlDesktopPresentationNativeUiTextureOverlayDiagnostic>& diagnostics) {
    if (atlas_asset.value == 0) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            SdlDesktopPresentationNativeUiTextureOverlayStatus::invalid_request,
            std::string{backend_name} + " textured native UI overlay requires a non-zero atlas asset id."));
        return {};
    }

    const auto* record = package.find(atlas_asset);
    if (record == nullptr) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            SdlDesktopPresentationNativeUiTextureOverlayStatus::failed,
            std::string{backend_name} +
                " textured native UI overlay atlas asset is missing from the runtime package."));
        return {};
    }
    if (record->kind != AssetKind::texture) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            SdlDesktopPresentationNativeUiTextureOverlayStatus::invalid_request,
            std::string{backend_name} + " textured native UI overlay atlas asset is not a cooked texture payload."));
        return {};
    }

    const auto payload = runtime::runtime_texture_payload(*record);
    if (!payload.succeeded()) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            SdlDesktopPresentationNativeUiTextureOverlayStatus::failed,
            std::string{backend_name} + " textured native UI overlay atlas payload is invalid: " + payload.diagnostic));
        return {};
    }

    auto upload = runtime_rhi::upload_runtime_texture(device, payload.payload);
    if (!upload.succeeded()) {
        diagnostics.push_back(make_native_ui_texture_overlay_diagnostic(
            SdlDesktopPresentationNativeUiTextureOverlayStatus::failed,
            std::string{backend_name} + " textured native UI overlay atlas upload failed: " + upload.diagnostic));
        return {};
    }

    auto sampler = device.create_sampler(rhi::SamplerDesc{});
    return NativeUiOverlayAtlasBinding{
        .atlas_page = atlas_asset,
        .texture = upload.texture,
        .sampler = sampler,
        .owner_device = upload.owner_device,
    };
}

[[nodiscard]] rhi::Format postprocess_scene_depth_format(bool enable_depth_input) noexcept {
    return enable_depth_input ? rhi::Format::depth24_stencil8 : rhi::Format::unknown;
}

[[nodiscard]] rhi::DepthStencilStateDesc postprocess_scene_depth_state(bool enable_depth_input) noexcept {
    if (!enable_depth_input) {
        return {};
    }
    return rhi::DepthStencilStateDesc{
        .depth_test_enabled = true, .depth_write_enabled = true, .depth_compare = rhi::CompareOp::less_equal};
}

[[nodiscard]] ShadowMapPlan make_scene_directional_shadow_plan(const SceneRenderPacket& packet,
                                                               Extent2D window_extent) {
    const auto tile_dim =
        std::min<std::uint32_t>(512u, std::max(128u, std::min(window_extent.width, window_extent.height) / 2u));
    return build_scene_shadow_map_plan(packet, SceneShadowMapDesc{
                                                   .extent = rhi::Extent2D{.width = tile_dim, .height = tile_dim},
                                                   .depth_format = rhi::Format::depth24_stencil8,
                                                   .directional_cascade_count = 4,
                                               });
}

[[nodiscard]] ShadowReceiverPlan make_scene_shadow_receiver_plan(const ShadowMapPlan& shadow_plan,
                                                                 const DirectionalShadowLightSpacePlan& light_space) {
    return build_shadow_receiver_plan(ShadowReceiverDesc{
        .shadow_map = &shadow_plan,
        .light_space = &light_space,
        .depth_bias = 0.003F,
        .lit_intensity = 1.0F,
        .shadow_intensity = 0.42F,
    });
}

[[nodiscard]] std::string directional_shadow_plan_diagnostic(std::string_view backend_name,
                                                             const ShadowMapPlan& shadow_plan,
                                                             const DirectionalShadowLightSpacePlan& light_space,
                                                             const ShadowReceiverPlan& receiver_plan) {
    if (!shadow_plan.succeeded()) {
        return std::string{backend_name} +
               " directional shadow smoke requires one shadow-casting directional light and at least one mesh caster/"
               "receiver; using NullRenderer fallback.";
    }
    if (!light_space.succeeded()) {
        return std::string{backend_name} +
               " directional shadow light-space plan is invalid; using NullRenderer fallback.";
    }
    if (!receiver_plan.succeeded()) {
        return std::string{backend_name} + " directional shadow receiver plan is invalid; using NullRenderer fallback.";
    }
    return {};
}

[[nodiscard]] NativeRendererCreateResult create_d3d12_renderer(const SdlDesktopPresentationDesc& desc,
                                                               rhi::SurfaceHandle surface) {
#if defined(MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_D3D12)
    const bool native_sprite_overlay_requested =
        desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay;
    const bool native_sprite_texture_overlay_requested =
        desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay_textures;
    const auto probe = rhi::d3d12::probe_runtime();
    if (!probe.windows_sdk_available || !probe.dxgi_factory_created ||
        (!probe.hardware_device_supported && !probe.warp_device_supported)) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::native_backend_unavailable,
            .diagnostic = "D3D12 runtime support is unavailable in this build or host; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(
                result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
        }
        return result;
    }

    try {
        auto device = rhi::d3d12::create_rhi_device(rhi::d3d12::DeviceBootstrapDesc{
            .prefer_warp = desc.prefer_warp,
            .enable_debug_layer = desc.enable_debug_layer,
        });
        if (device == nullptr) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::native_backend_unavailable,
                .diagnostic = "D3D12 device creation is unavailable; using NullRenderer fallback.",
            };
            if (native_sprite_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                              result.diagnostic);
            }
            if (native_sprite_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
            }
            return result;
        }
        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto pipeline_layout =
            device->create_pipeline_layout(rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.d3d12_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.d3d12_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.d3d12_renderer->vertex_shader.bytecode.data(),
        });
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.d3d12_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.d3d12_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.d3d12_renderer->fragment_shader.bytecode.data(),
        });
        rhi::ShaderHandle native_sprite_overlay_vertex_shader;
        rhi::ShaderHandle native_sprite_overlay_fragment_shader;
        NativeUiOverlayAtlasBinding native_sprite_overlay_atlas;
        if (native_sprite_overlay_requested) {
            native_sprite_overlay_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_renderer->native_sprite_overlay_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_renderer->native_sprite_overlay_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_renderer->native_sprite_overlay_vertex_shader.bytecode.data(),
            });
            native_sprite_overlay_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.d3d12_renderer->native_sprite_overlay_fragment_shader.entry_point,
                .bytecode_size = desc.d3d12_renderer->native_sprite_overlay_fragment_shader.bytecode.size(),
                .bytecode = desc.d3d12_renderer->native_sprite_overlay_fragment_shader.bytecode.data(),
            });
        }
        std::vector<SdlDesktopPresentationNativeUiTextureOverlayDiagnostic> texture_overlay_diagnostics;
        if (native_sprite_texture_overlay_requested) {
            native_sprite_overlay_atlas =
                make_native_ui_overlay_atlas_binding(*device, *desc.d3d12_renderer->native_sprite_overlay_package,
                                                     desc.d3d12_renderer->native_sprite_overlay_atlas_asset,
                                                     "D3D12 native 2D sprite overlay", texture_overlay_diagnostics);
            if (native_sprite_overlay_atlas.texture.value == 0 || native_sprite_overlay_atlas.sampler.value == 0) {
                NativeRendererCreateResult result{
                    .succeeded = false,
                    .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                    .diagnostic = "D3D12 native 2D sprite overlay atlas upload failed; using NullRenderer fallback.",
                };
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
                result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::failed;
                result.native_ui_texture_overlay_requested = true;
                result.native_ui_texture_overlay_atlas_ready = false;
                result.native_ui_texture_overlay_diagnostics = std::move(texture_overlay_diagnostics);
                return result;
            }
        }
        const auto pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = desc.d3d12_renderer->topology,
            .vertex_buffers = desc.d3d12_renderer->vertex_buffers,
            .vertex_attributes = desc.d3d12_renderer->vertex_attributes,
        });
        auto renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
            .device = device.get(),
            .extent = desc.extent,
            .swapchain = swapchain,
            .graphics_pipeline = pipeline,
            .wait_for_completion = true,
            .native_sprite_overlay_color_format = rhi::Format::bgra8_unorm,
            .native_sprite_overlay_vertex_shader = native_sprite_overlay_vertex_shader,
            .native_sprite_overlay_fragment_shader = native_sprite_overlay_fragment_shader,
            .native_sprite_overlay_atlas = native_sprite_overlay_atlas,
            .enable_native_sprite_overlay = native_sprite_overlay_requested,
            .enable_native_sprite_overlay_textures = native_sprite_texture_overlay_requested,
        });

        NativeRendererCreateResult result{
            .succeeded = true,
            .failure_reason = SdlDesktopPresentationFallbackReason::none,
            .diagnostic = {},
            .device = std::move(device),
            .renderer = std::move(renderer),
        };
        if (native_sprite_overlay_requested) {
            result.native_ui_overlay_requested = true;
            result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::ready;
            result.native_ui_overlay_ready = true;
        }
        if (native_sprite_texture_overlay_requested) {
            result.native_ui_texture_overlay_requested = true;
            result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::ready;
            result.native_ui_texture_overlay_atlas_ready = true;
        }
        return result;
    } catch (const std::exception&) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "D3D12 renderer creation failed; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(result, SdlDesktopPresentationNativeUiTextureOverlayStatus::failed,
                                                  result.diagnostic);
        }
        return result;
    }
#else
    (void)surface;
    NativeRendererCreateResult result{
        false,
        SdlDesktopPresentationFallbackReason::native_backend_unavailable,
        "D3D12 runtime support is unavailable in this build; using NullRenderer fallback.",
    };
    if (desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay) {
        mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                      result.diagnostic);
    }
    if (desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay_textures) {
        mark_native_ui_texture_overlay_result(result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable,
                                              result.diagnostic);
    }
    return result;
#endif
}

[[nodiscard]] NativeRendererCreateResult create_d3d12_scene_renderer(const SdlDesktopPresentationDesc& desc,
                                                                     rhi::SurfaceHandle surface) {
#if defined(MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_D3D12)
    if (desc.d3d12_scene_renderer == nullptr) {
        return missing_d3d12_scene_renderer_request();
    }
    const bool directional_shadow_requested =
        desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke;
    const bool native_ui_overlay_requested =
        desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay;
    const bool native_ui_texture_overlay_requested =
        desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay_textures;
    const bool scene_compute_morph_mesh_requested =
        desc.d3d12_scene_renderer != nullptr &&
        has_shader_bytecode(desc.d3d12_scene_renderer->compute_morph_vertex_shader) &&
        has_shader_bytecode(desc.d3d12_scene_renderer->compute_morph_shader) &&
        !desc.d3d12_scene_renderer->compute_morph_mesh_bindings.empty();
    if (native_ui_texture_overlay_requested && !native_ui_overlay_requested) {
        return invalid_d3d12_native_ui_texture_overlay_request(
            "D3D12 textured native UI overlay requires native UI overlay to be enabled; using NullRenderer fallback.");
    }
    const auto probe = rhi::d3d12::probe_runtime();
    if (!probe.windows_sdk_available || !probe.dxgi_factory_created ||
        (!probe.hardware_device_supported && !probe.warp_device_supported)) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::native_backend_unavailable,
            .diagnostic = "D3D12 runtime support is unavailable in this build or host; using NullRenderer fallback.",
        };
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::unavailable;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::unavailable,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        if (native_ui_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(
                result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
        }
        return result;
    }

    try {
        auto device = rhi::d3d12::create_rhi_device(rhi::d3d12::DeviceBootstrapDesc{
            .prefer_warp = desc.prefer_warp,
            .enable_debug_layer = desc.enable_debug_layer,
        });
        if (device == nullptr) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::native_backend_unavailable,
                .diagnostic = "D3D12 device creation is unavailable; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::unavailable;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::unavailable,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
            }
            return result;
        }
        const auto create_scene_graphics_pipeline =
            [&device](std::string_view label,
                      const rhi::GraphicsPipelineDesc& pipeline_desc) -> rhi::GraphicsPipelineHandle {
            try {
                return device->create_graphics_pipeline(pipeline_desc);
            } catch (const std::exception& exception) {
                throw std::invalid_argument(std::string(label) + " failed: " + exception.what());
            }
        };

        const float directional_shadow_viewport_aspect =
            desc.extent.height > 0 ? static_cast<float>(desc.extent.width) / static_cast<float>(desc.extent.height)
                                   : (16.0F / 9.0F);

        ShadowMapPlan shadow_map_plan{};
        DirectionalShadowLightSpacePlan directional_light_space_plan{};
        ShadowReceiverPlan shadow_receiver_plan{};
        rhi::DescriptorSetLayoutHandle shadow_receiver_layout;
        runtime_scene_rhi::RuntimeSceneGpuBindingOptions gpu_binding_options;
        gpu_binding_options.morph_mesh_assets = desc.d3d12_scene_renderer->morph_mesh_assets;
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.d3d12_scene_renderer->morph_mesh_bindings);
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.d3d12_scene_renderer->compute_morph_mesh_bindings);
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.d3d12_scene_renderer->compute_morph_skinned_mesh_bindings);
        for (const auto& binding : desc.d3d12_scene_renderer->compute_morph_skinned_mesh_bindings) {
            gpu_binding_options.compute_morph_skinned_mesh_bindings.push_back(
                runtime_scene_rhi::RuntimeSceneComputeMorphSkinnedMeshBinding{.mesh = binding.mesh,
                                                                              .morph_mesh = binding.morph_mesh});
        }
        if (scene_compute_morph_mesh_requested) {
            gpu_binding_options.mesh_upload.vertex_usage =
                gpu_binding_options.mesh_upload.vertex_usage | rhi::BufferUsage::storage;
        }
        if (directional_shadow_requested) {
            shadow_map_plan = make_scene_directional_shadow_plan(*desc.d3d12_scene_renderer->packet, desc.extent);
            directional_light_space_plan = build_scene_directional_shadow_light_space_plan(
                *desc.d3d12_scene_renderer->packet, shadow_map_plan,
                SceneShadowLightSpaceDesc{.viewport_aspect = directional_shadow_viewport_aspect});
            shadow_receiver_plan = make_scene_shadow_receiver_plan(shadow_map_plan, directional_light_space_plan);
            if (const auto diagnostic = directional_shadow_plan_diagnostic(
                    "D3D12", shadow_map_plan, directional_light_space_plan, shadow_receiver_plan);
                !diagnostic.empty()) {
                return invalid_d3d12_directional_shadow_request(diagnostic);
            }
            shadow_receiver_layout = device->create_descriptor_set_layout(shadow_receiver_plan.descriptor_set_layout);
            gpu_binding_options.additional_pipeline_descriptor_set_layouts.push_back(shadow_receiver_layout);
        }

        auto gpu_upload =
            runtime_scene_rhi::execute_runtime_scene_gpu_upload(runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc{
                .device = device.get(),
                .package = desc.d3d12_scene_renderer->package,
                .packet = desc.d3d12_scene_renderer->packet,
                .binding_options = gpu_binding_options,
            });
        auto& gpu_bindings = gpu_upload.bindings;
        if (!gpu_bindings.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "D3D12 scene GPU binding creation failed; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            for (const auto& failure : gpu_bindings.failures) {
                result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                    result.scene_gpu_status, "scene GPU binding failure asset=" + std::to_string(failure.asset.value) +
                                                 ": " + failure.diagnostic));
            }
            return result;
        }
        if (gpu_bindings.material_pipeline_layouts.empty()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic =
                    "D3D12 scene GPU binding creation did not produce a material pipeline layout; using NullRenderer "
                    "fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }
        auto compute_morph_bindings = build_scene_compute_morph_bindings(
            *device, gpu_bindings, desc.d3d12_scene_renderer->compute_morph_mesh_bindings,
            desc.d3d12_scene_renderer->compute_morph_shader,
            desc.d3d12_scene_renderer->enable_compute_morph_tangent_frame_output, "D3D12");
        if (!compute_morph_bindings.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = compute_morph_bindings.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }
        auto compute_morph_skinned_dispatch = dispatch_scene_compute_morph_skinned_bindings(
            *device, gpu_bindings.compute_morph_skinned_mesh_bindings,
            desc.d3d12_scene_renderer->compute_morph_skinned_shader, "D3D12");
        if (!compute_morph_skinned_dispatch.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = compute_morph_skinned_dispatch.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            if (native_ui_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }
        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.d3d12_scene_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.d3d12_scene_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.d3d12_scene_renderer->vertex_shader.bytecode.data(),
        });
        rhi::ShaderHandle compute_morph_scene_vertex_shader{};
        if (scene_compute_morph_mesh_requested) {
            compute_morph_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->compute_morph_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->compute_morph_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->compute_morph_vertex_shader.bytecode.data(),
            });
        }
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.d3d12_scene_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.d3d12_scene_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.d3d12_scene_renderer->fragment_shader.bytecode.data(),
        });
        const bool scene_gpu_skinning_requested = has_shader_bytecode(desc.d3d12_scene_renderer->skinned_vertex_shader);
        const bool scene_gpu_morph_requested = has_shader_bytecode(desc.d3d12_scene_renderer->morph_vertex_shader) &&
                                               !desc.d3d12_scene_renderer->morph_mesh_bindings.empty();
        rhi::ShaderHandle skinned_scene_vertex_shader{};
        rhi::GraphicsPipelineHandle skinned_scene_graphics_pipeline{};
        if (scene_gpu_skinning_requested) {
            skinned_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->skinned_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->skinned_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->skinned_vertex_shader.bytecode.data(),
            });
        }
        rhi::ShaderHandle morph_scene_vertex_shader{};
        rhi::GraphicsPipelineHandle morph_scene_graphics_pipeline{};
        if (scene_gpu_morph_requested) {
            morph_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->morph_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->morph_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->morph_vertex_shader.bytecode.data(),
            });
        }
        rhi::ShaderHandle shifted_scene_fragment_shader_handle = fragment_shader;
        const bool shifted_shadow_receiver_fragment_requested =
            directional_shadow_requested && (scene_gpu_skinning_requested || scene_gpu_morph_requested) &&
            has_shader_bytecode(desc.d3d12_scene_renderer->skinned_scene_fragment_shader);
        if (shifted_shadow_receiver_fragment_requested) {
            shifted_scene_fragment_shader_handle = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.d3d12_scene_renderer->skinned_scene_fragment_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->skinned_scene_fragment_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->skinned_scene_fragment_shader.bytecode.data(),
            });
        }
        const auto scene_fragment_shader_handle =
            shifted_shadow_receiver_fragment_requested ? shifted_scene_fragment_shader_handle : fragment_shader;
        const bool shadow_stage_skinned =
            scene_gpu_skinning_requested && scene_packet_references_skinned_mesh(*desc.d3d12_scene_renderer->package,
                                                                                 *desc.d3d12_scene_renderer->packet);
        const auto& shadow_stage_vertex_buffers = shadow_stage_skinned
                                                      ? desc.d3d12_scene_renderer->skinned_vertex_buffers
                                                      : desc.d3d12_scene_renderer->vertex_buffers;
        const auto& shadow_stage_vertex_attributes = shadow_stage_skinned
                                                         ? desc.d3d12_scene_renderer->skinned_vertex_attributes
                                                         : desc.d3d12_scene_renderer->vertex_attributes;
        rhi::GraphicsPipelineHandle shadow_pipeline;
        if (directional_shadow_requested) {
            const auto shadow_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->shadow_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->shadow_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->shadow_vertex_shader.bytecode.data(),
            });
            const auto shadow_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.d3d12_scene_renderer->shadow_fragment_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->shadow_fragment_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->shadow_fragment_shader.bytecode.data(),
            });
            const auto shadow_pipeline_layout = device->create_pipeline_layout(
                rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
            shadow_pipeline = create_scene_graphics_pipeline(
                "D3D12 shadow pipeline",
                rhi::GraphicsPipelineDesc{
                    .layout = shadow_pipeline_layout,
                    .vertex_shader = shadow_vertex_shader,
                    .fragment_shader = shadow_fragment_shader,
                    .color_format = rhi::Format::bgra8_unorm,
                    .depth_format = rhi::Format::depth24_stencil8,
                    .topology = desc.d3d12_scene_renderer->topology,
                    .vertex_buffers = shadow_stage_vertex_buffers,
                    .vertex_attributes = shadow_stage_vertex_attributes,
                    .depth_state = rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                              .depth_write_enabled = true,
                                                              .depth_compare = rhi::CompareOp::less_equal},
                });
        }
        const bool postprocess_depth_input_requested = desc.d3d12_scene_renderer->enable_postprocess_depth_input;
        const bool enable_postprocess_depth_input =
            desc.d3d12_scene_renderer->enable_postprocess && postprocess_depth_input_requested;
        const auto compute_morph_vertex_buffers = desc.d3d12_scene_renderer->enable_compute_morph_tangent_frame_output
                                                      ? compute_morph_tangent_frame_vertex_buffers()
                                                      : compute_morph_position_vertex_buffers();
        const auto compute_morph_vertex_attributes =
            desc.d3d12_scene_renderer->enable_compute_morph_tangent_frame_output
                ? compute_morph_tangent_frame_vertex_attributes()
                : compute_morph_position_vertex_attributes();
        const auto pipeline = create_scene_graphics_pipeline(
            "D3D12 scene pipeline",
            rhi::GraphicsPipelineDesc{
                .layout = gpu_bindings.material_pipeline_layouts.front(),
                .vertex_shader = scene_compute_morph_mesh_requested ? compute_morph_scene_vertex_shader : vertex_shader,
                .fragment_shader = scene_fragment_shader_handle,
                .color_format = rhi::Format::bgra8_unorm,
                .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                .topology = desc.d3d12_scene_renderer->topology,
                .vertex_buffers = scene_compute_morph_mesh_requested ? compute_morph_vertex_buffers
                                                                     : desc.d3d12_scene_renderer->vertex_buffers,
                .vertex_attributes = scene_compute_morph_mesh_requested ? compute_morph_vertex_attributes
                                                                        : desc.d3d12_scene_renderer->vertex_attributes,
                .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
            });
        if (scene_gpu_skinning_requested) {
            skinned_scene_graphics_pipeline = create_scene_graphics_pipeline(
                "D3D12 skinned scene pipeline",
                rhi::GraphicsPipelineDesc{
                    .layout = gpu_bindings.material_pipeline_layouts.front(),
                    .vertex_shader = skinned_scene_vertex_shader,
                    .fragment_shader = scene_fragment_shader_handle,
                    .color_format = rhi::Format::bgra8_unorm,
                    .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                    .topology = desc.d3d12_scene_renderer->topology,
                    .vertex_buffers = desc.d3d12_scene_renderer->skinned_vertex_buffers,
                    .vertex_attributes = desc.d3d12_scene_renderer->skinned_vertex_attributes,
                    .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
                });
        }
        if (scene_gpu_morph_requested) {
            morph_scene_graphics_pipeline = create_scene_graphics_pipeline(
                "D3D12 morph scene pipeline",
                rhi::GraphicsPipelineDesc{
                    .layout = gpu_bindings.material_pipeline_layouts.front(),
                    .vertex_shader = morph_scene_vertex_shader,
                    .fragment_shader = scene_fragment_shader_handle,
                    .color_format = rhi::Format::bgra8_unorm,
                    .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                    .topology = desc.d3d12_scene_renderer->topology,
                    .vertex_buffers = desc.d3d12_scene_renderer->vertex_buffers,
                    .vertex_attributes = desc.d3d12_scene_renderer->vertex_attributes,
                    .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
                });
        }
        std::unique_ptr<IRenderer> frame_renderer;
        NativeRendererCreateResult result;
        result.succeeded = true;
        result.failure_reason = SdlDesktopPresentationFallbackReason::none;
        result.postprocess_depth_input_requested = postprocess_depth_input_requested;
        result.directional_shadow_requested = directional_shadow_requested;
        result.native_ui_overlay_requested = native_ui_overlay_requested;
        result.native_ui_texture_overlay_requested = native_ui_texture_overlay_requested;
        if (desc.d3d12_scene_renderer->enable_postprocess) {
            const auto postprocess_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.d3d12_scene_renderer->postprocess_vertex_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->postprocess_vertex_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->postprocess_vertex_shader.bytecode.data(),
            });
            const auto postprocess_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.d3d12_scene_renderer->postprocess_fragment_shader.entry_point,
                .bytecode_size = desc.d3d12_scene_renderer->postprocess_fragment_shader.bytecode.size(),
                .bytecode = desc.d3d12_scene_renderer->postprocess_fragment_shader.bytecode.data(),
            });
            rhi::ShaderHandle native_ui_overlay_vertex_shader;
            rhi::ShaderHandle native_ui_overlay_fragment_shader;
            if (native_ui_overlay_requested) {
                native_ui_overlay_vertex_shader = device->create_shader(rhi::ShaderDesc{
                    .stage = rhi::ShaderStage::vertex,
                    .entry_point = desc.d3d12_scene_renderer->native_ui_overlay_vertex_shader.entry_point,
                    .bytecode_size = desc.d3d12_scene_renderer->native_ui_overlay_vertex_shader.bytecode.size(),
                    .bytecode = desc.d3d12_scene_renderer->native_ui_overlay_vertex_shader.bytecode.data(),
                });
                native_ui_overlay_fragment_shader = device->create_shader(rhi::ShaderDesc{
                    .stage = rhi::ShaderStage::fragment,
                    .entry_point = desc.d3d12_scene_renderer->native_ui_overlay_fragment_shader.entry_point,
                    .bytecode_size = desc.d3d12_scene_renderer->native_ui_overlay_fragment_shader.bytecode.size(),
                    .bytecode = desc.d3d12_scene_renderer->native_ui_overlay_fragment_shader.bytecode.data(),
                });
            }
            NativeUiOverlayAtlasBinding native_ui_overlay_atlas;
            if (native_ui_texture_overlay_requested) {
                native_ui_overlay_atlas =
                    make_native_ui_overlay_atlas_binding(*device, *desc.d3d12_scene_renderer->package,
                                                         desc.d3d12_scene_renderer->native_ui_overlay_atlas_asset,
                                                         "D3D12", result.native_ui_texture_overlay_diagnostics);
                if (native_ui_overlay_atlas.texture.value == 0) {
                    result.succeeded = false;
                    result.failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = result.native_ui_texture_overlay_diagnostics.empty()
                                            ? "D3D12 textured native UI overlay atlas binding failed; using "
                                              "NullRenderer fallback."
                                            : result.native_ui_texture_overlay_diagnostics.front().message +
                                                  "; using NullRenderer fallback.";
                    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                                  result.diagnostic);
                    result.native_ui_texture_overlay_status =
                        SdlDesktopPresentationNativeUiTextureOverlayStatus::failed;
                    return result;
                }
            }
            std::unique_ptr<IRenderer> postprocess_renderer;
            if (directional_shadow_requested) {
                std::array<std::uint8_t, shadow_receiver_constants_byte_size()> shadow_cb{};
                Mat4 camera_view = Mat4::identity();
                if (const auto* primary = desc.d3d12_scene_renderer->packet->primary_camera(); primary != nullptr) {
                    camera_view =
                        make_scene_camera_matrices(*primary, directional_shadow_viewport_aspect).view_from_world;
                }
                pack_shadow_receiver_constants(shadow_cb, directional_light_space_plan,
                                               shadow_map_plan.directional_cascade_count, camera_view);
                const bool scene_deformation_descriptor_set =
                    gpu_bindings.skinned_joint_descriptor_set_layout.value != 0 ||
                    gpu_bindings.morph_descriptor_set_layout.value != 0;
                const std::uint32_t shadow_receiver_descriptor_set_index = scene_deformation_descriptor_set ? 2u : 1u;
                auto shadow_renderer =
                    std::make_unique<RhiDirectionalShadowSmokeFrameRenderer>(RhiDirectionalShadowSmokeFrameRendererDesc{
                        .device = device.get(),
                        .extent = desc.extent,
                        .swapchain = swapchain,
                        .color_format = rhi::Format::bgra8_unorm,
                        .scene_graphics_pipeline = pipeline,
                        .scene_skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                        .scene_morph_graphics_pipeline = morph_scene_graphics_pipeline,
                        .scene_pipeline_layout = gpu_bindings.material_pipeline_layouts.front(),
                        .shadow_graphics_pipeline = shadow_pipeline,
                        .shadow_receiver_descriptor_set_layout = shadow_receiver_layout,
                        .postprocess_vertex_shader = postprocess_vertex_shader,
                        .postprocess_fragment_stages =
                            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
                        .wait_for_completion = true,
                        .scene_depth_format = rhi::Format::depth24_stencil8,
                        .shadow_depth_format = rhi::Format::depth24_stencil8,
                        .shadow_filter_mode = shadow_receiver_plan.filter_mode,
                        .shadow_filter_radius_texels = shadow_receiver_plan.filter_radius_texels,
                        .shadow_filter_tap_count = shadow_receiver_plan.filter_tap_count,
                        .native_ui_overlay_vertex_shader = native_ui_overlay_vertex_shader,
                        .native_ui_overlay_fragment_shader = native_ui_overlay_fragment_shader,
                        .native_ui_overlay_atlas = native_ui_overlay_atlas,
                        .enable_native_ui_overlay = native_ui_overlay_requested,
                        .enable_native_ui_overlay_textures = native_ui_texture_overlay_requested,
                        .shadow_depth_atlas_extent = Extent2D{.width = shadow_map_plan.depth_texture.extent.width,
                                                              .height = shadow_map_plan.depth_texture.extent.height},
                        .directional_shadow_cascade_count = shadow_map_plan.directional_cascade_count,
                        .shadow_receiver_constants_initial = shadow_cb,
                        .shadow_receiver_descriptor_set_index = shadow_receiver_descriptor_set_index,
                    });
                result.directional_shadow_status = SdlDesktopPresentationDirectionalShadowStatus::ready;
                result.directional_shadow_ready = shadow_renderer->directional_shadow_ready();
                result.directional_shadow_filter_mode =
                    to_presentation_filter_mode(shadow_renderer->shadow_filter_mode());
                result.directional_shadow_filter_tap_count = shadow_renderer->shadow_filter_tap_count();
                result.directional_shadow_filter_radius_texels = shadow_renderer->shadow_filter_radius_texels();
                if (native_ui_overlay_requested) {
                    result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::ready;
                    result.native_ui_overlay_ready = shadow_renderer->native_ui_overlay_ready();
                }
                if (native_ui_texture_overlay_requested) {
                    result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::ready;
                    result.native_ui_texture_overlay_atlas_ready = shadow_renderer->native_ui_overlay_atlas_ready();
                }
                result.framegraph_passes = shadow_renderer->frame_graph_pass_count();
                postprocess_renderer = std::move(shadow_renderer);
            } else {
                auto color_postprocess_renderer =
                    std::make_unique<RhiPostprocessFrameRenderer>(RhiPostprocessFrameRendererDesc{
                        .device = device.get(),
                        .extent = desc.extent,
                        .swapchain = swapchain,
                        .color_format = rhi::Format::bgra8_unorm,
                        .scene_graphics_pipeline = pipeline,
                        .scene_skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                        .scene_morph_graphics_pipeline = morph_scene_graphics_pipeline,
                        .postprocess_vertex_shader = postprocess_vertex_shader,
                        .postprocess_fragment_stages =
                            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
                        .wait_for_completion = true,
                        .enable_depth_input = enable_postprocess_depth_input,
                        .depth_format = rhi::Format::depth24_stencil8,
                        .native_ui_overlay_vertex_shader = native_ui_overlay_vertex_shader,
                        .native_ui_overlay_fragment_shader = native_ui_overlay_fragment_shader,
                        .native_ui_overlay_atlas = native_ui_overlay_atlas,
                        .enable_native_ui_overlay = native_ui_overlay_requested,
                        .enable_native_ui_overlay_textures = native_ui_texture_overlay_requested,
                    });
                if (native_ui_overlay_requested) {
                    result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::ready;
                    result.native_ui_overlay_ready = color_postprocess_renderer->native_ui_overlay_ready();
                }
                if (native_ui_texture_overlay_requested) {
                    result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::ready;
                    result.native_ui_texture_overlay_atlas_ready =
                        color_postprocess_renderer->native_ui_overlay_atlas_ready();
                }
                result.framegraph_passes = color_postprocess_renderer->frame_graph_pass_count();
                postprocess_renderer = std::move(color_postprocess_renderer);
            }
            result.postprocess_status = SdlDesktopPresentationPostprocessStatus::ready;
            result.postprocess_depth_input_ready = enable_postprocess_depth_input;
            frame_renderer = std::move(postprocess_renderer);
        } else {
            frame_renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
                .device = device.get(),
                .extent = desc.extent,
                .color_texture = rhi::TextureHandle{},
                .swapchain = swapchain,
                .graphics_pipeline = pipeline,
                .wait_for_completion = true,
                .skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                .morph_graphics_pipeline = morph_scene_graphics_pipeline,
            });
        }
        auto scene_renderer = std::make_unique<SceneGpuBindingInjectingRenderer>(
            std::move(frame_renderer), std::move(gpu_bindings), desc.d3d12_scene_renderer->morph_mesh_bindings,
            std::move(compute_morph_bindings.bindings), compute_morph_bindings.queue_waits,
            compute_morph_skinned_dispatch.dispatches, compute_morph_skinned_dispatch.queue_waits);
        auto* scene_renderer_ptr = scene_renderer.get();

        result.device = std::move(device);
        result.renderer = std::move(scene_renderer);
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::ready;
        result.scene_gpu_renderer = scene_renderer_ptr;
        return result;
    } catch (const std::exception& exception) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "D3D12 scene renderer creation failed: " + std::string(exception.what()) +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_postprocess) {
            result.postprocess_status = SdlDesktopPresentationPostprocessStatus::failed;
            result.postprocess_diagnostics.push_back(
                make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
            result.postprocess_depth_input_requested = desc.d3d12_scene_renderer->enable_postprocess_depth_input;
        }
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke) {
            result.directional_shadow_status = SdlDesktopPresentationDirectionalShadowStatus::failed;
            result.directional_shadow_requested = true;
            result.directional_shadow_diagnostics.push_back(
                make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
        }
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay) {
            result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::failed;
            result.native_ui_overlay_requested = true;
            result.native_ui_overlay_diagnostics.push_back(
                make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
        }
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay_textures) {
            result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::failed;
            result.native_ui_texture_overlay_requested = true;
            result.native_ui_texture_overlay_diagnostics.push_back(
                make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
        }
        return result;
    }
#else
    (void)surface;
    NativeRendererCreateResult result{
        false,
        SdlDesktopPresentationFallbackReason::native_backend_unavailable,
        "D3D12 runtime support is unavailable in this build; using NullRenderer fallback.",
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::unavailable;
    result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
    if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke) {
        mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::unavailable,
                                       result.diagnostic);
    }
    if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay) {
        mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                      result.diagnostic);
    }
    if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay_textures) {
        mark_native_ui_texture_overlay_result(result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable,
                                              result.diagnostic);
    }
    return result;
#endif
}

#if defined(MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_VULKAN)
[[nodiscard]] rhi::vulkan::VulkanSwapchainCreatePlan
make_vulkan_presentation_swapchain_mapping_plan(rhi::vulkan::VulkanRuntimeDevice& device, rhi::SurfaceHandle surface,
                                                Extent2D extent, rhi::Format format, bool vsync) {
    rhi::vulkan::VulkanSwapchainCreatePlan plan;
    if (surface.value == 0 || extent.width == 0 || extent.height == 0 || format == rhi::Format::unknown ||
        !device.has_graphics_queue() || !device.has_present_queue()) {
        plan.diagnostic = "Vulkan presentation swapchain mapping requires a ready surface and graphics/present queues";
        return plan;
    }

    plan.supported = true;
    plan.extent = rhi::Extent2D{.width = extent.width, .height = extent.height};
    plan.format = format;
    plan.image_count = 2;
    plan.image_view_count = 2;
    plan.present_mode = vsync ? rhi::vulkan::VulkanPresentMode::fifo : rhi::vulkan::VulkanPresentMode::mailbox;
    plan.acquire_before_render = true;
    plan.diagnostic = "Vulkan presentation swapchain mapping ready";
    return plan;
}

[[nodiscard]] rhi::vulkan::VulkanDynamicRenderingPlan
make_vulkan_presentation_dynamic_rendering_plan(rhi::vulkan::VulkanRuntimeDevice& device, Extent2D extent,
                                                rhi::Format color_format) {
    rhi::vulkan::VulkanDynamicRenderingDesc rendering_desc;
    rendering_desc.extent = rhi::Extent2D{.width = extent.width, .height = extent.height};
    rendering_desc.color_attachments.push_back(rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = color_format,
        .load_action = rhi::LoadAction::clear,
        .store_action = rhi::StoreAction::store,
    });
    return rhi::vulkan::build_dynamic_rendering_plan(rendering_desc, device.command_plan());
}

[[nodiscard]] rhi::vulkan::VulkanFrameSynchronizationPlan
make_vulkan_presentation_frame_synchronization_plan(rhi::vulkan::VulkanRuntimeDevice& device) {
    rhi::vulkan::VulkanFrameSynchronizationDesc sync_desc;
    sync_desc.readback_required = true;
    sync_desc.present_required = true;
    return rhi::vulkan::build_frame_synchronization_plan(sync_desc, device.command_plan());
}

[[nodiscard]] bool probe_vulkan_runtime_command_pool_ready(rhi::vulkan::VulkanRuntimeDevice& device) {
    auto pool_result = rhi::vulkan::create_runtime_command_pool(device, {});
    return pool_result.created && pool_result.pool.owns_pool() && pool_result.pool.owns_primary_command_buffer();
}

[[nodiscard]] bool probe_vulkan_runtime_descriptor_binding_ready(rhi::vulkan::VulkanRuntimeDevice& device) {
    auto pool_result = rhi::vulkan::create_runtime_command_pool(device, {});
    if (!pool_result.created || !pool_result.pool.owns_primary_command_buffer()) {
        return false;
    }

    rhi::vulkan::VulkanRuntimeDescriptorSetLayoutDesc layout_desc;
    layout_desc.layout.bindings.push_back(rhi::DescriptorBindingDesc{
        .binding = 0,
        .type = rhi::DescriptorType::uniform_buffer,
        .count = 1,
        .stages = rhi::ShaderStageVisibility::vertex,
    });
    auto layout_result = rhi::vulkan::create_runtime_descriptor_set_layout(device, layout_desc);
    if (!layout_result.created) {
        return false;
    }
    auto set_result = rhi::vulkan::create_runtime_descriptor_set(device, layout_result.layout, {});
    if (!set_result.created) {
        return false;
    }

    rhi::vulkan::VulkanRuntimePipelineLayoutDesc pipeline_layout_desc;
    pipeline_layout_desc.descriptor_set_layouts.push_back(&layout_result.layout);
    auto pipeline_layout_result = rhi::vulkan::create_runtime_pipeline_layout(device, pipeline_layout_desc);
    if (!pipeline_layout_result.created) {
        return false;
    }
    if (!pool_result.pool.begin_primary_command_buffer()) {
        return false;
    }

    const auto bind_result = rhi::vulkan::record_runtime_descriptor_set_binding(
        device, pool_result.pool, pipeline_layout_result.layout, set_result.set, {});
    const auto ended = pool_result.pool.end_primary_command_buffer();
    return bind_result.recorded && ended;
}

[[nodiscard]] bool probe_vulkan_runtime_depth_mapping_ready(rhi::vulkan::VulkanRuntimeDevice& device,
                                                            rhi::Format color_format) {
    rhi::vulkan::VulkanDynamicRenderingDesc rendering_desc;
    rendering_desc.extent = rhi::Extent2D{.width = 1, .height = 1};
    rendering_desc.color_attachments.push_back(rhi::vulkan::VulkanDynamicRenderingColorAttachmentDesc{
        .format = color_format,
        .load_action = rhi::LoadAction::clear,
        .store_action = rhi::StoreAction::store,
    });
    rendering_desc.has_depth_attachment = true;
    rendering_desc.depth_format = rhi::Format::depth24_stencil8;

    const auto rendering_plan = rhi::vulkan::build_dynamic_rendering_plan(rendering_desc, device.command_plan());
    const auto depth_barrier =
        rhi::vulkan::build_texture_transition_barrier(rhi::ResourceState::undefined, rhi::ResourceState::depth_write);
    return rendering_plan.supported && rendering_plan.depth_attachment_enabled && depth_barrier.supported;
}

[[nodiscard]] rhi::vulkan::VulkanRhiDeviceMappingPlan build_vulkan_presentation_mapping_plan(
    rhi::vulkan::VulkanRuntimeDevice& device, const rhi::vulkan::VulkanSpirvShaderArtifactValidation& vertex_validation,
    const rhi::vulkan::VulkanSpirvShaderArtifactValidation& fragment_validation, rhi::SurfaceHandle surface,
    Extent2D extent, rhi::Format color_format, bool vsync,
    const rhi::vulkan::VulkanSpirvShaderArtifactValidation* compute_validation = nullptr) {
    rhi::vulkan::VulkanRhiDeviceMappingDesc mapping_desc;
    mapping_desc.command_pool_ready = probe_vulkan_runtime_command_pool_ready(device);
    mapping_desc.swapchain =
        make_vulkan_presentation_swapchain_mapping_plan(device, surface, extent, color_format, vsync);
    mapping_desc.dynamic_rendering = make_vulkan_presentation_dynamic_rendering_plan(device, extent, color_format);
    mapping_desc.frame_synchronization = make_vulkan_presentation_frame_synchronization_plan(device);
    mapping_desc.vertex_shader = vertex_validation;
    mapping_desc.fragment_shader = fragment_validation;
    if (compute_validation != nullptr) {
        mapping_desc.compute_shader = *compute_validation;
        mapping_desc.compute_dispatch_ready = compute_validation->valid && device.command_plan().supported;
    }
    mapping_desc.descriptor_binding_ready = probe_vulkan_runtime_descriptor_binding_ready(device);
    mapping_desc.visible_clear_readback_ready = mapping_desc.swapchain.supported &&
                                                mapping_desc.dynamic_rendering.supported &&
                                                mapping_desc.frame_synchronization.supported;
    mapping_desc.visible_draw_readback_ready =
        mapping_desc.visible_clear_readback_ready && vertex_validation.valid && fragment_validation.valid;
    mapping_desc.visible_texture_sampling_readback_ready =
        mapping_desc.visible_draw_readback_ready && mapping_desc.descriptor_binding_ready;
    mapping_desc.visible_depth_readback_ready =
        mapping_desc.visible_draw_readback_ready && probe_vulkan_runtime_depth_mapping_ready(device, color_format);
    return rhi::vulkan::build_rhi_device_mapping_plan(mapping_desc);
}
#endif

[[nodiscard]] NativeRendererCreateResult create_vulkan_renderer(const SdlDesktopPresentationDesc& desc,
                                                                rhi::SurfaceHandle surface) {
#if defined(MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_VULKAN)
    const bool native_sprite_overlay_requested =
        desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay;
    const bool native_sprite_texture_overlay_requested =
        desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay_textures;
    const auto vertex_validation =
        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = rhi::ShaderStage::vertex,
            .bytecode = desc.vulkan_renderer->vertex_shader.bytecode.data(),
            .bytecode_size = desc.vulkan_renderer->vertex_shader.bytecode.size(),
        });
    if (!vertex_validation.valid) {
        return NativeRendererCreateResult{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan vertex SPIR-V validation failed: " + vertex_validation.diagnostic +
                          "; using NullRenderer fallback.",
        };
    }

    const auto fragment_validation =
        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = rhi::ShaderStage::fragment,
            .bytecode = desc.vulkan_renderer->fragment_shader.bytecode.data(),
            .bytecode_size = desc.vulkan_renderer->fragment_shader.bytecode.size(),
        });
    if (!fragment_validation.valid) {
        return NativeRendererCreateResult{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan fragment SPIR-V validation failed: " + fragment_validation.diagnostic +
                          "; using NullRenderer fallback.",
        };
    }
    if (native_sprite_overlay_requested) {
        const auto native_sprite_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.bytecode.size(),
            });
        if (!native_sprite_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan native 2D sprite overlay vertex SPIR-V validation failed: " +
                              native_sprite_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
            if (native_sprite_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }

        const auto native_sprite_fragment_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::fragment,
                .bytecode = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.bytecode.size(),
            });
        if (!native_sprite_fragment_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan native 2D sprite overlay fragment SPIR-V validation failed: " +
                              native_sprite_fragment_validation.diagnostic + "; using NullRenderer fallback.",
            };
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
            if (native_sprite_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::failed, result.diagnostic);
            }
            return result;
        }
    }

    auto runtime_device = rhi::vulkan::create_runtime_device({}, {}, {}, surface);
    if (!runtime_device.created) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::native_backend_unavailable,
            .diagnostic = "Vulkan runtime device creation failed: " + runtime_device.diagnostic +
                          "; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(
                result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
        }
        return result;
    }

    const auto mapping_plan =
        build_vulkan_presentation_mapping_plan(runtime_device.device, vertex_validation, fragment_validation, surface,
                                               desc.extent, rhi::Format::bgra8_unorm, desc.vsync);
    if (!mapping_plan.supported) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan IRhiDevice mapping is unavailable: " + mapping_plan.diagnostic +
                          "; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(
                result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
        }
        return result;
    }

    try {
        auto device = rhi::vulkan::create_rhi_device(std::move(runtime_device.device), mapping_plan);
        if (device == nullptr) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::native_backend_unavailable,
                .diagnostic = "Vulkan device creation is unavailable; using NullRenderer fallback.",
            };
            if (native_sprite_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                              result.diagnostic);
            }
            if (native_sprite_texture_overlay_requested) {
                mark_native_ui_texture_overlay_result(
                    result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable, result.diagnostic);
            }
            return result;
        }
        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto pipeline_layout =
            device->create_pipeline_layout(rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.vulkan_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.vulkan_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.vulkan_renderer->vertex_shader.bytecode.data(),
        });
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.vulkan_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.vulkan_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.vulkan_renderer->fragment_shader.bytecode.data(),
        });
        rhi::ShaderHandle native_sprite_overlay_vertex_shader;
        rhi::ShaderHandle native_sprite_overlay_fragment_shader;
        NativeUiOverlayAtlasBinding native_sprite_overlay_atlas;
        if (native_sprite_overlay_requested) {
            native_sprite_overlay_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_renderer->native_sprite_overlay_vertex_shader.bytecode.data(),
            });
            native_sprite_overlay_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.entry_point,
                .bytecode_size = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.bytecode.size(),
                .bytecode = desc.vulkan_renderer->native_sprite_overlay_fragment_shader.bytecode.data(),
            });
        }
        std::vector<SdlDesktopPresentationNativeUiTextureOverlayDiagnostic> texture_overlay_diagnostics;
        if (native_sprite_texture_overlay_requested) {
            native_sprite_overlay_atlas =
                make_native_ui_overlay_atlas_binding(*device, *desc.vulkan_renderer->native_sprite_overlay_package,
                                                     desc.vulkan_renderer->native_sprite_overlay_atlas_asset,
                                                     "Vulkan native 2D sprite overlay", texture_overlay_diagnostics);
            if (native_sprite_overlay_atlas.texture.value == 0 || native_sprite_overlay_atlas.sampler.value == 0) {
                NativeRendererCreateResult result{
                    .succeeded = false,
                    .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                    .diagnostic = "Vulkan native 2D sprite overlay atlas upload failed; using NullRenderer fallback.",
                };
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
                result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::failed;
                result.native_ui_texture_overlay_requested = true;
                result.native_ui_texture_overlay_atlas_ready = false;
                result.native_ui_texture_overlay_diagnostics = std::move(texture_overlay_diagnostics);
                return result;
            }
        }
        const auto pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = pipeline_layout,
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = rhi::Format::unknown,
            .topology = desc.vulkan_renderer->topology,
            .vertex_buffers = desc.vulkan_renderer->vertex_buffers,
            .vertex_attributes = desc.vulkan_renderer->vertex_attributes,
        });
        auto renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
            .device = device.get(),
            .extent = desc.extent,
            .swapchain = swapchain,
            .graphics_pipeline = pipeline,
            .wait_for_completion = true,
            .native_sprite_overlay_color_format = rhi::Format::bgra8_unorm,
            .native_sprite_overlay_vertex_shader = native_sprite_overlay_vertex_shader,
            .native_sprite_overlay_fragment_shader = native_sprite_overlay_fragment_shader,
            .native_sprite_overlay_atlas = native_sprite_overlay_atlas,
            .enable_native_sprite_overlay = native_sprite_overlay_requested,
            .enable_native_sprite_overlay_textures = native_sprite_texture_overlay_requested,
        });

        NativeRendererCreateResult result{
            .succeeded = true,
            .failure_reason = SdlDesktopPresentationFallbackReason::none,
            .diagnostic = {},
            .device = std::move(device),
            .renderer = std::move(renderer),
        };
        if (native_sprite_overlay_requested) {
            result.native_ui_overlay_requested = true;
            result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::ready;
            result.native_ui_overlay_ready = true;
        }
        if (native_sprite_texture_overlay_requested) {
            result.native_ui_texture_overlay_requested = true;
            result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::ready;
            result.native_ui_texture_overlay_atlas_ready = true;
        }
        return result;
    } catch (const std::exception& error) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic =
                std::string{"Vulkan renderer creation failed: "} + error.what() + "; using NullRenderer fallback.",
        };
        if (native_sprite_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        if (native_sprite_texture_overlay_requested) {
            mark_native_ui_texture_overlay_result(result, SdlDesktopPresentationNativeUiTextureOverlayStatus::failed,
                                                  result.diagnostic);
        }
        return result;
    }
#else
    (void)surface;
    NativeRendererCreateResult result{
        false,
        SdlDesktopPresentationFallbackReason::native_backend_unavailable,
        "Vulkan runtime support is unavailable in this build; using NullRenderer fallback.",
    };
    if (desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay) {
        mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                      result.diagnostic);
    }
    if (desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay_textures) {
        mark_native_ui_texture_overlay_result(result, SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable,
                                              result.diagnostic);
    }
    return result;
#endif
}

[[nodiscard]] NativeRendererCreateResult create_vulkan_scene_renderer(const SdlDesktopPresentationDesc& desc,
                                                                      rhi::SurfaceHandle surface) {
#if defined(MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_VULKAN)
    if (desc.vulkan_scene_renderer == nullptr) {
        return missing_vulkan_scene_renderer_request();
    }
    const bool directional_shadow_requested =
        desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_directional_shadow_smoke;
    const bool native_ui_overlay_requested =
        desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay;
    const bool native_ui_texture_overlay_requested =
        desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay_textures;
    const bool scene_compute_morph_mesh_requested =
        desc.vulkan_scene_renderer != nullptr &&
        has_shader_bytecode(desc.vulkan_scene_renderer->compute_morph_vertex_shader) &&
        has_shader_bytecode(desc.vulkan_scene_renderer->compute_morph_shader) &&
        !desc.vulkan_scene_renderer->compute_morph_mesh_bindings.empty();
    const bool scene_compute_morph_skinned_requested =
        desc.vulkan_scene_renderer != nullptr &&
        has_shader_bytecode(desc.vulkan_scene_renderer->compute_morph_skinned_shader) &&
        !desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings.empty();
    if (native_ui_texture_overlay_requested && !native_ui_overlay_requested) {
        return invalid_vulkan_native_ui_texture_overlay_request(
            "Vulkan textured native UI overlay requires native UI overlay to be enabled; using NullRenderer fallback.");
    }
    const auto vertex_validation =
        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = rhi::ShaderStage::vertex,
            .bytecode = desc.vulkan_scene_renderer->vertex_shader.bytecode.data(),
            .bytecode_size = desc.vulkan_scene_renderer->vertex_shader.bytecode.size(),
        });
    if (!vertex_validation.valid) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan scene vertex SPIR-V validation failed: " + vertex_validation.diagnostic +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        return result;
    }

    const auto fragment_validation =
        rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
            .stage = rhi::ShaderStage::fragment,
            .bytecode = desc.vulkan_scene_renderer->fragment_shader.bytecode.data(),
            .bytecode_size = desc.vulkan_scene_renderer->fragment_shader.bytecode.size(),
        });
    if (!fragment_validation.valid) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan scene fragment SPIR-V validation failed: " + fragment_validation.diagnostic +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        return result;
    }

    if (!desc.vulkan_scene_renderer->morph_mesh_bindings.empty()) {
        const auto morph_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->morph_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->morph_vertex_shader.bytecode.size(),
            });
        if (!morph_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan scene morph vertex SPIR-V validation failed: " +
                              morph_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }

    rhi::vulkan::VulkanSpirvShaderArtifactValidation compute_morph_shader_validation;
    if (has_shader_bytecode(desc.vulkan_scene_renderer->compute_morph_shader)) {
        compute_morph_shader_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::compute,
                .bytecode = desc.vulkan_scene_renderer->compute_morph_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->compute_morph_shader.bytecode.size(),
            });
        if (!compute_morph_shader_validation.valid) {
            const char* diagnostic_prefix = desc.vulkan_scene_renderer->compute_morph_mesh_bindings.empty()
                                                ? "Vulkan scene compute mapping SPIR-V validation failed: "
                                                : "Vulkan scene compute morph compute SPIR-V validation failed: ";
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = std::string{diagnostic_prefix} + compute_morph_shader_validation.diagnostic +
                              "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }
    if (!desc.vulkan_scene_renderer->compute_morph_mesh_bindings.empty()) {
        const auto compute_morph_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->compute_morph_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->compute_morph_vertex_shader.bytecode.size(),
            });
        if (!compute_morph_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan scene compute morph vertex SPIR-V validation failed: " +
                              compute_morph_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }
    rhi::vulkan::VulkanSpirvShaderArtifactValidation compute_morph_skinned_shader_validation;
    if (!desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings.empty()) {
        compute_morph_skinned_shader_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::compute,
                .bytecode = desc.vulkan_scene_renderer->compute_morph_skinned_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->compute_morph_skinned_shader.bytecode.size(),
            });
        if (!compute_morph_skinned_shader_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan scene compute morph skinned compute SPIR-V validation failed: " +
                              compute_morph_skinned_shader_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }

    rhi::vulkan::VulkanSpirvShaderArtifactValidation postprocess_vertex_validation;
    rhi::vulkan::VulkanSpirvShaderArtifactValidation postprocess_fragment_validation;
    if (desc.vulkan_scene_renderer->enable_postprocess) {
        postprocess_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->postprocess_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->postprocess_vertex_shader.bytecode.size(),
            });
        if (!postprocess_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan postprocess vertex SPIR-V validation failed: " +
                              postprocess_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            result.postprocess_status = SdlDesktopPresentationPostprocessStatus::failed;
            result.postprocess_diagnostics.push_back(
                make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }

        postprocess_fragment_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::fragment,
                .bytecode = desc.vulkan_scene_renderer->postprocess_fragment_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->postprocess_fragment_shader.bytecode.size(),
            });
        if (!postprocess_fragment_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan postprocess fragment SPIR-V validation failed: " +
                              postprocess_fragment_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            result.postprocess_status = SdlDesktopPresentationPostprocessStatus::failed;
            result.postprocess_diagnostics.push_back(
                make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }

    rhi::vulkan::VulkanSpirvShaderArtifactValidation native_ui_overlay_vertex_validation;
    rhi::vulkan::VulkanSpirvShaderArtifactValidation native_ui_overlay_fragment_validation;
    if (native_ui_overlay_requested) {
        native_ui_overlay_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.bytecode.size(),
            });
        if (!native_ui_overlay_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan native UI overlay vertex SPIR-V validation failed: " +
                              native_ui_overlay_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
            return result;
        }

        native_ui_overlay_fragment_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::fragment,
                .bytecode = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.bytecode.size(),
            });
        if (!native_ui_overlay_fragment_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan native UI overlay fragment SPIR-V validation failed: " +
                              native_ui_overlay_fragment_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
            return result;
        }
    }

    rhi::vulkan::VulkanSpirvShaderArtifactValidation shadow_vertex_validation;
    rhi::vulkan::VulkanSpirvShaderArtifactValidation shadow_fragment_validation;
    if (desc.vulkan_scene_renderer->enable_directional_shadow_smoke) {
        shadow_vertex_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::vertex,
                .bytecode = desc.vulkan_scene_renderer->shadow_vertex_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->shadow_vertex_shader.bytecode.size(),
            });
        if (!shadow_vertex_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan directional shadow vertex SPIR-V validation failed: " +
                              shadow_vertex_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }

        shadow_fragment_validation =
            rhi::vulkan::validate_spirv_shader_artifact(rhi::vulkan::VulkanSpirvShaderArtifactDesc{
                .stage = rhi::ShaderStage::fragment,
                .bytecode = desc.vulkan_scene_renderer->shadow_fragment_shader.bytecode.data(),
                .bytecode_size = desc.vulkan_scene_renderer->shadow_fragment_shader.bytecode.size(),
            });
        if (!shadow_fragment_validation.valid) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan directional shadow fragment SPIR-V validation failed: " +
                              shadow_fragment_validation.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
    }

    auto runtime_device = rhi::vulkan::create_runtime_device({}, {}, {}, surface);
    if (!runtime_device.created) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::native_backend_unavailable,
            .diagnostic = "Vulkan runtime device creation failed: " + runtime_device.diagnostic +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::unavailable;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::unavailable,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                          result.diagnostic);
        }
        return result;
    }

    const auto* mapping_compute_validation =
        scene_compute_morph_mesh_requested
            ? &compute_morph_shader_validation
            : (scene_compute_morph_skinned_requested
                   ? &compute_morph_skinned_shader_validation
                   : (compute_morph_shader_validation.valid ? &compute_morph_shader_validation : nullptr));
    const auto mapping_plan = build_vulkan_presentation_mapping_plan(
        runtime_device.device, vertex_validation, fragment_validation, surface, desc.extent, rhi::Format::bgra8_unorm,
        desc.vsync, mapping_compute_validation);
    if (!mapping_plan.supported) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = "Vulkan scene IRhiDevice mapping is unavailable: " + mapping_plan.diagnostic +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (directional_shadow_requested) {
            mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                           result.diagnostic);
        }
        if (native_ui_overlay_requested) {
            mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                          result.diagnostic);
        }
        return result;
    }

    try {
        auto device = rhi::vulkan::create_rhi_device(std::move(runtime_device.device), mapping_plan);
        if (device == nullptr) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::native_backend_unavailable,
                .diagnostic = "Vulkan device creation is unavailable; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::unavailable;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::unavailable,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::unavailable,
                                              result.diagnostic);
            }
            return result;
        }

        const float directional_shadow_viewport_aspect =
            desc.extent.height > 0 ? static_cast<float>(desc.extent.width) / static_cast<float>(desc.extent.height)
                                   : (16.0F / 9.0F);

        ShadowMapPlan shadow_map_plan{};
        DirectionalShadowLightSpacePlan directional_light_space_plan{};
        ShadowReceiverPlan shadow_receiver_plan{};
        rhi::DescriptorSetLayoutHandle shadow_receiver_layout;
        runtime_scene_rhi::RuntimeSceneGpuBindingOptions gpu_binding_options;
        gpu_binding_options.morph_mesh_assets = desc.vulkan_scene_renderer->morph_mesh_assets;
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.vulkan_scene_renderer->morph_mesh_bindings);
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.vulkan_scene_renderer->compute_morph_mesh_bindings);
        append_morph_binding_assets(gpu_binding_options.morph_mesh_assets,
                                    desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings);
        for (const auto& binding : desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings) {
            gpu_binding_options.compute_morph_skinned_mesh_bindings.push_back(
                runtime_scene_rhi::RuntimeSceneComputeMorphSkinnedMeshBinding{.mesh = binding.mesh,
                                                                              .morph_mesh = binding.morph_mesh});
        }
        if (scene_compute_morph_mesh_requested) {
            gpu_binding_options.mesh_upload.vertex_usage =
                gpu_binding_options.mesh_upload.vertex_usage | rhi::BufferUsage::storage;
        }
        if (directional_shadow_requested) {
            shadow_map_plan = make_scene_directional_shadow_plan(*desc.vulkan_scene_renderer->packet, desc.extent);
            directional_light_space_plan = build_scene_directional_shadow_light_space_plan(
                *desc.vulkan_scene_renderer->packet, shadow_map_plan,
                SceneShadowLightSpaceDesc{.viewport_aspect = directional_shadow_viewport_aspect});
            shadow_receiver_plan = make_scene_shadow_receiver_plan(shadow_map_plan, directional_light_space_plan);
            if (const auto diagnostic = directional_shadow_plan_diagnostic(
                    "Vulkan", shadow_map_plan, directional_light_space_plan, shadow_receiver_plan);
                !diagnostic.empty()) {
                return invalid_vulkan_directional_shadow_request(diagnostic);
            }
            shadow_receiver_layout = device->create_descriptor_set_layout(shadow_receiver_plan.descriptor_set_layout);
            gpu_binding_options.additional_pipeline_descriptor_set_layouts.push_back(shadow_receiver_layout);
        }

        auto gpu_upload =
            runtime_scene_rhi::execute_runtime_scene_gpu_upload(runtime_scene_rhi::RuntimeSceneGpuUploadExecutionDesc{
                .device = device.get(),
                .package = desc.vulkan_scene_renderer->package,
                .packet = desc.vulkan_scene_renderer->packet,
                .binding_options = gpu_binding_options,
            });
        auto& gpu_bindings = gpu_upload.bindings;
        if (!gpu_bindings.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = "Vulkan scene GPU binding creation failed; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            for (const auto& failure : gpu_bindings.failures) {
                result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                    result.scene_gpu_status, "scene GPU binding failure asset=" + std::to_string(failure.asset.value) +
                                                 ": " + failure.diagnostic));
            }
            return result;
        }
        if (gpu_bindings.material_pipeline_layouts.empty()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic =
                    "Vulkan scene GPU binding creation did not produce a material pipeline layout; using NullRenderer "
                    "fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (directional_shadow_requested) {
                mark_directional_shadow_result(result, SdlDesktopPresentationDirectionalShadowStatus::failed,
                                               result.diagnostic);
            }
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
        auto compute_morph_bindings = build_scene_compute_morph_bindings(
            *device, gpu_bindings, desc.vulkan_scene_renderer->compute_morph_mesh_bindings,
            desc.vulkan_scene_renderer->compute_morph_shader,
            desc.vulkan_scene_renderer->enable_compute_morph_tangent_frame_output, "Vulkan");
        if (!compute_morph_bindings.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = compute_morph_bindings.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }
        auto compute_morph_skinned_dispatch = dispatch_scene_compute_morph_skinned_bindings(
            *device, gpu_bindings.compute_morph_skinned_mesh_bindings,
            desc.vulkan_scene_renderer->compute_morph_skinned_shader, "Vulkan");
        if (!compute_morph_skinned_dispatch.succeeded()) {
            NativeRendererCreateResult result{
                .succeeded = false,
                .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                .diagnostic = compute_morph_skinned_dispatch.diagnostic + "; using NullRenderer fallback.",
            };
            result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
            result.scene_gpu_diagnostics.push_back(
                make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
            if (native_ui_overlay_requested) {
                mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                              result.diagnostic);
            }
            return result;
        }

        const auto swapchain = device->create_swapchain(rhi::SwapchainDesc{
            .extent = rhi::Extent2D{.width = desc.extent.width, .height = desc.extent.height},
            .format = rhi::Format::bgra8_unorm,
            .buffer_count = 2,
            .vsync = desc.vsync,
            .surface = surface,
        });
        const auto vertex_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::vertex,
            .entry_point = desc.vulkan_scene_renderer->vertex_shader.entry_point,
            .bytecode_size = desc.vulkan_scene_renderer->vertex_shader.bytecode.size(),
            .bytecode = desc.vulkan_scene_renderer->vertex_shader.bytecode.data(),
        });
        rhi::ShaderHandle compute_morph_scene_vertex_shader{};
        if (scene_compute_morph_mesh_requested) {
            compute_morph_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->compute_morph_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->compute_morph_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->compute_morph_vertex_shader.bytecode.data(),
            });
        }
        const auto fragment_shader = device->create_shader(rhi::ShaderDesc{
            .stage = rhi::ShaderStage::fragment,
            .entry_point = desc.vulkan_scene_renderer->fragment_shader.entry_point,
            .bytecode_size = desc.vulkan_scene_renderer->fragment_shader.bytecode.size(),
            .bytecode = desc.vulkan_scene_renderer->fragment_shader.bytecode.data(),
        });
        const bool scene_gpu_skinning_requested =
            has_shader_bytecode(desc.vulkan_scene_renderer->skinned_vertex_shader);
        const bool scene_gpu_morph_requested = has_shader_bytecode(desc.vulkan_scene_renderer->morph_vertex_shader) &&
                                               !desc.vulkan_scene_renderer->morph_mesh_bindings.empty();
        rhi::ShaderHandle skinned_scene_vertex_shader{};
        rhi::GraphicsPipelineHandle skinned_scene_graphics_pipeline{};
        if (scene_gpu_skinning_requested) {
            skinned_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->skinned_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->skinned_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->skinned_vertex_shader.bytecode.data(),
            });
        }
        rhi::ShaderHandle morph_scene_vertex_shader{};
        rhi::GraphicsPipelineHandle morph_scene_graphics_pipeline{};
        if (scene_gpu_morph_requested) {
            morph_scene_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->morph_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->morph_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->morph_vertex_shader.bytecode.data(),
            });
        }
        rhi::ShaderHandle shifted_scene_fragment_shader_handle = fragment_shader;
        const bool shifted_shadow_receiver_fragment_requested =
            directional_shadow_requested && (scene_gpu_skinning_requested || scene_gpu_morph_requested) &&
            has_shader_bytecode(desc.vulkan_scene_renderer->skinned_scene_fragment_shader);
        if (shifted_shadow_receiver_fragment_requested) {
            shifted_scene_fragment_shader_handle = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.vulkan_scene_renderer->skinned_scene_fragment_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->skinned_scene_fragment_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->skinned_scene_fragment_shader.bytecode.data(),
            });
        }
        const auto scene_fragment_shader_handle =
            shifted_shadow_receiver_fragment_requested ? shifted_scene_fragment_shader_handle : fragment_shader;
        const bool shadow_stage_skinned =
            scene_gpu_skinning_requested && scene_packet_references_skinned_mesh(*desc.vulkan_scene_renderer->package,
                                                                                 *desc.vulkan_scene_renderer->packet);
        const auto& shadow_stage_vertex_buffers = shadow_stage_skinned
                                                      ? desc.vulkan_scene_renderer->skinned_vertex_buffers
                                                      : desc.vulkan_scene_renderer->vertex_buffers;
        const auto& shadow_stage_vertex_attributes = shadow_stage_skinned
                                                         ? desc.vulkan_scene_renderer->skinned_vertex_attributes
                                                         : desc.vulkan_scene_renderer->vertex_attributes;
        rhi::GraphicsPipelineHandle shadow_pipeline;
        if (directional_shadow_requested) {
            const auto shadow_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->shadow_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->shadow_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->shadow_vertex_shader.bytecode.data(),
            });
            const auto shadow_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.vulkan_scene_renderer->shadow_fragment_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->shadow_fragment_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->shadow_fragment_shader.bytecode.data(),
            });
            const auto shadow_pipeline_layout = device->create_pipeline_layout(
                rhi::PipelineLayoutDesc{.descriptor_sets = {}, .push_constant_bytes = 0});
            shadow_pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
                .layout = shadow_pipeline_layout,
                .vertex_shader = shadow_vertex_shader,
                .fragment_shader = shadow_fragment_shader,
                .color_format = rhi::Format::bgra8_unorm,
                .depth_format = rhi::Format::depth24_stencil8,
                .topology = desc.vulkan_scene_renderer->topology,
                .vertex_buffers = shadow_stage_vertex_buffers,
                .vertex_attributes = shadow_stage_vertex_attributes,
                .depth_state = rhi::DepthStencilStateDesc{.depth_test_enabled = true,
                                                          .depth_write_enabled = true,
                                                          .depth_compare = rhi::CompareOp::less_equal},
            });
        }
        const bool postprocess_depth_input_requested = desc.vulkan_scene_renderer->enable_postprocess_depth_input;
        const bool enable_postprocess_depth_input =
            desc.vulkan_scene_renderer->enable_postprocess && postprocess_depth_input_requested;
        const auto compute_morph_vertex_buffers = desc.vulkan_scene_renderer->enable_compute_morph_tangent_frame_output
                                                      ? compute_morph_tangent_frame_vertex_buffers()
                                                      : compute_morph_position_vertex_buffers();
        const auto compute_morph_vertex_attributes =
            desc.vulkan_scene_renderer->enable_compute_morph_tangent_frame_output
                ? compute_morph_tangent_frame_vertex_attributes()
                : compute_morph_position_vertex_attributes();
        const auto pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
            .layout = gpu_bindings.material_pipeline_layouts.front(),
            .vertex_shader = scene_compute_morph_mesh_requested ? compute_morph_scene_vertex_shader : vertex_shader,
            .fragment_shader = scene_fragment_shader_handle,
            .color_format = rhi::Format::bgra8_unorm,
            .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
            .topology = desc.vulkan_scene_renderer->topology,
            .vertex_buffers = scene_compute_morph_mesh_requested ? compute_morph_vertex_buffers
                                                                 : desc.vulkan_scene_renderer->vertex_buffers,
            .vertex_attributes = scene_compute_morph_mesh_requested ? compute_morph_vertex_attributes
                                                                    : desc.vulkan_scene_renderer->vertex_attributes,
            .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
        });
        if (scene_gpu_skinning_requested) {
            skinned_scene_graphics_pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
                .layout = gpu_bindings.material_pipeline_layouts.front(),
                .vertex_shader = skinned_scene_vertex_shader,
                .fragment_shader = scene_fragment_shader_handle,
                .color_format = rhi::Format::bgra8_unorm,
                .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                .topology = desc.vulkan_scene_renderer->topology,
                .vertex_buffers = desc.vulkan_scene_renderer->skinned_vertex_buffers,
                .vertex_attributes = desc.vulkan_scene_renderer->skinned_vertex_attributes,
                .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
            });
        }
        if (scene_gpu_morph_requested) {
            morph_scene_graphics_pipeline = device->create_graphics_pipeline(rhi::GraphicsPipelineDesc{
                .layout = gpu_bindings.material_pipeline_layouts.front(),
                .vertex_shader = morph_scene_vertex_shader,
                .fragment_shader = scene_fragment_shader_handle,
                .color_format = rhi::Format::bgra8_unorm,
                .depth_format = postprocess_scene_depth_format(enable_postprocess_depth_input),
                .topology = desc.vulkan_scene_renderer->topology,
                .vertex_buffers = desc.vulkan_scene_renderer->vertex_buffers,
                .vertex_attributes = desc.vulkan_scene_renderer->vertex_attributes,
                .depth_state = postprocess_scene_depth_state(enable_postprocess_depth_input),
            });
        }
        std::unique_ptr<IRenderer> frame_renderer;
        NativeRendererCreateResult result;
        result.succeeded = true;
        result.failure_reason = SdlDesktopPresentationFallbackReason::none;
        result.postprocess_depth_input_requested = postprocess_depth_input_requested;
        result.directional_shadow_requested = directional_shadow_requested;
        result.native_ui_overlay_requested = native_ui_overlay_requested;
        result.native_ui_texture_overlay_requested = native_ui_texture_overlay_requested;
        if (desc.vulkan_scene_renderer->enable_postprocess) {
            const auto postprocess_vertex_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::vertex,
                .entry_point = desc.vulkan_scene_renderer->postprocess_vertex_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->postprocess_vertex_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->postprocess_vertex_shader.bytecode.data(),
            });
            const auto postprocess_fragment_shader = device->create_shader(rhi::ShaderDesc{
                .stage = rhi::ShaderStage::fragment,
                .entry_point = desc.vulkan_scene_renderer->postprocess_fragment_shader.entry_point,
                .bytecode_size = desc.vulkan_scene_renderer->postprocess_fragment_shader.bytecode.size(),
                .bytecode = desc.vulkan_scene_renderer->postprocess_fragment_shader.bytecode.data(),
            });
            rhi::ShaderHandle native_ui_overlay_vertex_shader;
            rhi::ShaderHandle native_ui_overlay_fragment_shader;
            if (native_ui_overlay_requested) {
                native_ui_overlay_vertex_shader = device->create_shader(rhi::ShaderDesc{
                    .stage = rhi::ShaderStage::vertex,
                    .entry_point = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.entry_point,
                    .bytecode_size = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.bytecode.size(),
                    .bytecode = desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader.bytecode.data(),
                });
                native_ui_overlay_fragment_shader = device->create_shader(rhi::ShaderDesc{
                    .stage = rhi::ShaderStage::fragment,
                    .entry_point = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.entry_point,
                    .bytecode_size = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.bytecode.size(),
                    .bytecode = desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader.bytecode.data(),
                });
            }
            NativeUiOverlayAtlasBinding native_ui_overlay_atlas;
            if (native_ui_texture_overlay_requested) {
                native_ui_overlay_atlas =
                    make_native_ui_overlay_atlas_binding(*device, *desc.vulkan_scene_renderer->package,
                                                         desc.vulkan_scene_renderer->native_ui_overlay_atlas_asset,
                                                         "Vulkan", result.native_ui_texture_overlay_diagnostics);
                if (native_ui_overlay_atlas.texture.value == 0) {
                    result.succeeded = false;
                    result.failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable;
                    result.diagnostic = result.native_ui_texture_overlay_diagnostics.empty()
                                            ? "Vulkan textured native UI overlay atlas binding failed; using "
                                              "NullRenderer fallback."
                                            : result.native_ui_texture_overlay_diagnostics.front().message +
                                                  "; using NullRenderer fallback.";
                    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
                    result.scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
                    mark_native_ui_overlay_result(result, SdlDesktopPresentationNativeUiOverlayStatus::failed,
                                                  result.diagnostic);
                    result.native_ui_texture_overlay_status =
                        SdlDesktopPresentationNativeUiTextureOverlayStatus::failed;
                    return result;
                }
            }
            std::unique_ptr<IRenderer> postprocess_renderer;
            if (directional_shadow_requested) {
                std::array<std::uint8_t, shadow_receiver_constants_byte_size()> shadow_cb{};
                Mat4 camera_view = Mat4::identity();
                if (const auto* primary = desc.vulkan_scene_renderer->packet->primary_camera(); primary != nullptr) {
                    camera_view =
                        make_scene_camera_matrices(*primary, directional_shadow_viewport_aspect).view_from_world;
                }
                pack_shadow_receiver_constants(shadow_cb, directional_light_space_plan,
                                               shadow_map_plan.directional_cascade_count, camera_view);
                const bool scene_deformation_descriptor_set =
                    gpu_bindings.skinned_joint_descriptor_set_layout.value != 0 ||
                    gpu_bindings.morph_descriptor_set_layout.value != 0;
                const std::uint32_t shadow_receiver_descriptor_set_index = scene_deformation_descriptor_set ? 2u : 1u;
                auto shadow_renderer =
                    std::make_unique<RhiDirectionalShadowSmokeFrameRenderer>(RhiDirectionalShadowSmokeFrameRendererDesc{
                        .device = device.get(),
                        .extent = desc.extent,
                        .swapchain = swapchain,
                        .color_format = rhi::Format::bgra8_unorm,
                        .scene_graphics_pipeline = pipeline,
                        .scene_skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                        .scene_morph_graphics_pipeline = morph_scene_graphics_pipeline,
                        .scene_pipeline_layout = gpu_bindings.material_pipeline_layouts.front(),
                        .shadow_graphics_pipeline = shadow_pipeline,
                        .shadow_receiver_descriptor_set_layout = shadow_receiver_layout,
                        .postprocess_vertex_shader = postprocess_vertex_shader,
                        .postprocess_fragment_stages =
                            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
                        .wait_for_completion = true,
                        .scene_depth_format = rhi::Format::depth24_stencil8,
                        .shadow_depth_format = rhi::Format::depth24_stencil8,
                        .shadow_filter_mode = shadow_receiver_plan.filter_mode,
                        .shadow_filter_radius_texels = shadow_receiver_plan.filter_radius_texels,
                        .shadow_filter_tap_count = shadow_receiver_plan.filter_tap_count,
                        .native_ui_overlay_vertex_shader = native_ui_overlay_vertex_shader,
                        .native_ui_overlay_fragment_shader = native_ui_overlay_fragment_shader,
                        .native_ui_overlay_atlas = native_ui_overlay_atlas,
                        .enable_native_ui_overlay = native_ui_overlay_requested,
                        .enable_native_ui_overlay_textures = native_ui_texture_overlay_requested,
                        .shadow_depth_atlas_extent = Extent2D{.width = shadow_map_plan.depth_texture.extent.width,
                                                              .height = shadow_map_plan.depth_texture.extent.height},
                        .directional_shadow_cascade_count = shadow_map_plan.directional_cascade_count,
                        .shadow_receiver_constants_initial = shadow_cb,
                        .shadow_receiver_descriptor_set_index = shadow_receiver_descriptor_set_index,
                    });
                result.directional_shadow_status = SdlDesktopPresentationDirectionalShadowStatus::ready;
                result.directional_shadow_ready = shadow_renderer->directional_shadow_ready();
                result.directional_shadow_filter_mode =
                    to_presentation_filter_mode(shadow_renderer->shadow_filter_mode());
                result.directional_shadow_filter_tap_count = shadow_renderer->shadow_filter_tap_count();
                result.directional_shadow_filter_radius_texels = shadow_renderer->shadow_filter_radius_texels();
                if (native_ui_overlay_requested) {
                    result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::ready;
                    result.native_ui_overlay_ready = shadow_renderer->native_ui_overlay_ready();
                }
                if (native_ui_texture_overlay_requested) {
                    result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::ready;
                    result.native_ui_texture_overlay_atlas_ready = shadow_renderer->native_ui_overlay_atlas_ready();
                }
                result.framegraph_passes = shadow_renderer->frame_graph_pass_count();
                postprocess_renderer = std::move(shadow_renderer);
            } else {
                auto color_postprocess_renderer =
                    std::make_unique<RhiPostprocessFrameRenderer>(RhiPostprocessFrameRendererDesc{
                        .device = device.get(),
                        .extent = desc.extent,
                        .swapchain = swapchain,
                        .color_format = rhi::Format::bgra8_unorm,
                        .scene_graphics_pipeline = pipeline,
                        .scene_skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                        .scene_morph_graphics_pipeline = morph_scene_graphics_pipeline,
                        .postprocess_vertex_shader = postprocess_vertex_shader,
                        .postprocess_fragment_stages =
                            std::vector<mirakana::rhi::ShaderHandle>{postprocess_fragment_shader},
                        .wait_for_completion = true,
                        .enable_depth_input = enable_postprocess_depth_input,
                        .depth_format = rhi::Format::depth24_stencil8,
                        .native_ui_overlay_vertex_shader = native_ui_overlay_vertex_shader,
                        .native_ui_overlay_fragment_shader = native_ui_overlay_fragment_shader,
                        .native_ui_overlay_atlas = native_ui_overlay_atlas,
                        .enable_native_ui_overlay = native_ui_overlay_requested,
                        .enable_native_ui_overlay_textures = native_ui_texture_overlay_requested,
                    });
                if (native_ui_overlay_requested) {
                    result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::ready;
                    result.native_ui_overlay_ready = color_postprocess_renderer->native_ui_overlay_ready();
                }
                if (native_ui_texture_overlay_requested) {
                    result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::ready;
                    result.native_ui_texture_overlay_atlas_ready =
                        color_postprocess_renderer->native_ui_overlay_atlas_ready();
                }
                result.framegraph_passes = color_postprocess_renderer->frame_graph_pass_count();
                postprocess_renderer = std::move(color_postprocess_renderer);
            }
            result.postprocess_status = SdlDesktopPresentationPostprocessStatus::ready;
            result.postprocess_depth_input_ready = enable_postprocess_depth_input;
            frame_renderer = std::move(postprocess_renderer);
        } else {
            frame_renderer = std::make_unique<RhiFrameRenderer>(RhiFrameRendererDesc{
                .device = device.get(),
                .extent = desc.extent,
                .color_texture = rhi::TextureHandle{},
                .swapchain = swapchain,
                .graphics_pipeline = pipeline,
                .wait_for_completion = true,
                .skinned_graphics_pipeline = skinned_scene_graphics_pipeline,
                .morph_graphics_pipeline = morph_scene_graphics_pipeline,
            });
        }
        auto scene_renderer = std::make_unique<SceneGpuBindingInjectingRenderer>(
            std::move(frame_renderer), std::move(gpu_bindings), desc.vulkan_scene_renderer->morph_mesh_bindings,
            std::move(compute_morph_bindings.bindings), compute_morph_bindings.queue_waits,
            compute_morph_skinned_dispatch.dispatches, compute_morph_skinned_dispatch.queue_waits);
        auto* scene_renderer_ptr = scene_renderer.get();

        result.device = std::move(device);
        result.renderer = std::move(scene_renderer);
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::ready;
        result.scene_gpu_renderer = scene_renderer_ptr;
        return result;
    } catch (const std::exception& error) {
        NativeRendererCreateResult result{
            .succeeded = false,
            .failure_reason = SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
            .diagnostic = std::string{"Vulkan scene renderer creation failed: "} + error.what() +
                          "; using NullRenderer fallback.",
        };
        result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::failed;
        result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_postprocess) {
            result.postprocess_status = SdlDesktopPresentationPostprocessStatus::failed;
            result.postprocess_diagnostics.push_back(
                make_postprocess_diagnostic(result.postprocess_status, result.diagnostic));
            result.postprocess_depth_input_requested = desc.vulkan_scene_renderer->enable_postprocess_depth_input;
        }
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_directional_shadow_smoke) {
            result.directional_shadow_status = SdlDesktopPresentationDirectionalShadowStatus::failed;
            result.directional_shadow_requested = true;
            result.directional_shadow_diagnostics.push_back(
                make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
        }
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay) {
            result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::failed;
            result.native_ui_overlay_requested = true;
            result.native_ui_overlay_diagnostics.push_back(
                make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
        }
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay_textures) {
            result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::failed;
            result.native_ui_texture_overlay_requested = true;
            result.native_ui_texture_overlay_diagnostics.push_back(
                make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
        }
        return result;
    }
#else
    (void)surface;
    NativeRendererCreateResult result{
        false,
        SdlDesktopPresentationFallbackReason::native_backend_unavailable,
        "Vulkan runtime support is unavailable in this build; using NullRenderer fallback.",
    };
    result.scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::unavailable;
    result.scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(result.scene_gpu_status, result.diagnostic));
    if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_directional_shadow_smoke) {
        result.directional_shadow_status = SdlDesktopPresentationDirectionalShadowStatus::unavailable;
        result.directional_shadow_requested = true;
        result.directional_shadow_diagnostics.push_back(
            make_directional_shadow_diagnostic(result.directional_shadow_status, result.diagnostic));
    }
    if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay) {
        result.native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::unavailable;
        result.native_ui_overlay_requested = true;
        result.native_ui_overlay_diagnostics.push_back(
            make_native_ui_overlay_diagnostic(result.native_ui_overlay_status, result.diagnostic));
    }
    if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay_textures) {
        result.native_ui_texture_overlay_status = SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable;
        result.native_ui_texture_overlay_requested = true;
        result.native_ui_texture_overlay_diagnostics.push_back(
            make_native_ui_texture_overlay_diagnostic(result.native_ui_texture_overlay_status, result.diagnostic));
    }
    return result;
#endif
}

[[nodiscard]] bool quality_gate_requested(const SdlDesktopPresentationQualityGateDesc& desc) noexcept {
    return desc.require_scene_gpu_bindings || desc.require_postprocess || desc.require_postprocess_depth_input ||
           desc.require_directional_shadow || desc.require_directional_shadow_filtering;
}

[[nodiscard]] std::uint32_t
quality_gate_expected_framegraph_passes(const SdlDesktopPresentationQualityGateDesc& desc) noexcept {
    if (desc.require_directional_shadow) {
        return 3;
    }
    if (desc.require_postprocess || desc.require_postprocess_depth_input) {
        return 2;
    }
    return 0;
}

[[nodiscard]] std::uint32_t
quality_gate_expected_framegraph_barrier_steps(const SdlDesktopPresentationQualityGateDesc& desc) noexcept {
    if (desc.require_directional_shadow) {
        return 5;
    }
    if (desc.require_postprocess_depth_input) {
        return 3;
    }
    if (desc.require_postprocess) {
        return 1;
    }
    return 0;
}

} // namespace

struct SdlDesktopPresentation::Impl {
    SdlDesktopPresentationBackend requested_backend{SdlDesktopPresentationBackend::null_renderer};
    SdlDesktopPresentationBackend backend{SdlDesktopPresentationBackend::null_renderer};
    SdlDesktopPresentationFallbackReason fallback_reason{SdlDesktopPresentationFallbackReason::none};
    bool allow_null_fallback{true};
    SdlDesktopPresentationSceneGpuBindingStatus scene_gpu_status{
        SdlDesktopPresentationSceneGpuBindingStatus::not_requested};
    SdlDesktopPresentationPostprocessStatus postprocess_status{SdlDesktopPresentationPostprocessStatus::not_requested};
    bool postprocess_depth_input_requested{false};
    bool postprocess_depth_input_ready{false};
    SdlDesktopPresentationDirectionalShadowStatus directional_shadow_status{
        SdlDesktopPresentationDirectionalShadowStatus::not_requested};
    bool directional_shadow_requested{false};
    bool directional_shadow_ready{false};
    SdlDesktopPresentationDirectionalShadowFilterMode directional_shadow_filter_mode{
        SdlDesktopPresentationDirectionalShadowFilterMode::none};
    std::uint32_t directional_shadow_filter_tap_count{0};
    float directional_shadow_filter_radius_texels{0.0F};
    SdlDesktopPresentationNativeUiOverlayStatus native_ui_overlay_status{
        SdlDesktopPresentationNativeUiOverlayStatus::not_requested};
    bool native_ui_overlay_requested{false};
    bool native_ui_overlay_ready{false};
    SdlDesktopPresentationNativeUiTextureOverlayStatus native_ui_texture_overlay_status{
        SdlDesktopPresentationNativeUiTextureOverlayStatus::not_requested};
    bool native_ui_texture_overlay_requested{false};
    bool native_ui_texture_overlay_atlas_ready{false};
    std::size_t framegraph_passes{0};
    std::vector<SdlDesktopPresentationBackendReport> backend_reports;
    std::vector<SdlDesktopPresentationDiagnostic> diagnostics;
    std::vector<SdlDesktopPresentationSceneGpuBindingDiagnostic> scene_gpu_diagnostics;
    std::vector<SdlDesktopPresentationPostprocessDiagnostic> postprocess_diagnostics;
    std::vector<SdlDesktopPresentationDirectionalShadowDiagnostic> directional_shadow_diagnostics;
    std::vector<SdlDesktopPresentationNativeUiOverlayDiagnostic> native_ui_overlay_diagnostics;
    std::vector<SdlDesktopPresentationNativeUiTextureOverlayDiagnostic> native_ui_texture_overlay_diagnostics;
    std::unique_ptr<rhi::IRhiDevice> device;
    std::unique_ptr<IRenderer> renderer;
    SceneGpuBindingInjectingRenderer* scene_gpu_renderer{nullptr};
};

SdlDesktopPresentation::SdlDesktopPresentation(const SdlDesktopPresentationDesc& desc)
    : impl_(std::make_unique<Impl>()) {
    if (desc.window == nullptr) {
        throw std::invalid_argument("sdl desktop presentation requires a window");
    }
    if (!has_extent(desc.extent)) {
        throw std::invalid_argument("sdl desktop presentation extent must be non-zero");
    }

    impl_->requested_backend = requested_backend_from_desc(desc);
    impl_->allow_null_fallback = desc.allow_null_fallback;
    impl_->postprocess_depth_input_requested =
        desc.prefer_vulkan
            ? desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_postprocess_depth_input
            : desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
                  desc.d3d12_scene_renderer->enable_postprocess_depth_input;
    impl_->directional_shadow_requested =
        desc.prefer_vulkan
            ? desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_directional_shadow_smoke
            : desc.prefer_d3d12 && desc.d3d12_scene_renderer != nullptr &&
                  desc.d3d12_scene_renderer->enable_directional_shadow_smoke;
    impl_->native_ui_overlay_requested =
        desc.prefer_vulkan
            ? (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay) ||
                  (desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay)
            : desc.prefer_d3d12 &&
                  ((desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay) ||
                   (desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay));
    impl_->native_ui_texture_overlay_requested =
        desc.prefer_vulkan
            ? (desc.vulkan_scene_renderer != nullptr &&
               desc.vulkan_scene_renderer->enable_native_ui_overlay_textures) ||
                  (desc.vulkan_renderer != nullptr && desc.vulkan_renderer->enable_native_sprite_overlay_textures)
            : desc.prefer_d3d12 &&
                  ((desc.d3d12_scene_renderer != nullptr &&
                    desc.d3d12_scene_renderer->enable_native_ui_overlay_textures) ||
                   (desc.d3d12_renderer != nullptr && desc.d3d12_renderer->enable_native_sprite_overlay_textures));

    SdlDesktopPresentationFallbackReason fallback_reason{
        SdlDesktopPresentationFallbackReason::native_backend_unavailable};
    std::string fallback_message{"Native presentation is not selected; using NullRenderer fallback."};

    auto record_backend_report = [&](SdlDesktopPresentationBackend backend,
                                     SdlDesktopPresentationBackendReportStatus status,
                                     SdlDesktopPresentationFallbackReason reason, std::string message) {
        impl_->backend_reports.push_back(SdlDesktopPresentationBackendReport{
            .backend = backend,
            .status = status,
            .fallback_reason = reason,
            .message = message,
        });
        fallback_reason = reason;
        fallback_message = std::move(message);
    };

    auto record_backend_ready = [&](SdlDesktopPresentationBackend backend, std::string message) {
        impl_->backend_reports.push_back(SdlDesktopPresentationBackendReport{
            .backend = backend,
            .status = SdlDesktopPresentationBackendReportStatus::ready,
            .fallback_reason = SdlDesktopPresentationFallbackReason::none,
            .message = std::move(message),
        });
        impl_->fallback_reason = SdlDesktopPresentationFallbackReason::none;
    };

    if (desc.prefer_vulkan) {
        if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_postprocess_depth_input &&
            !desc.vulkan_scene_renderer->enable_postprocess) {
            const auto invalid_request = invalid_vulkan_postprocess_depth_input_request();
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->postprocess_status = invalid_request.postprocess_status;
            impl_->postprocess_diagnostics = invalid_request.postprocess_diagnostics;
            impl_->postprocess_depth_input_requested =
                impl_->postprocess_depth_input_requested || invalid_request.postprocess_depth_input_requested;
            impl_->postprocess_depth_input_ready = false;
        } else if (desc.vulkan_scene_renderer != nullptr &&
                   desc.vulkan_scene_renderer->enable_native_ui_overlay_textures &&
                   !desc.vulkan_scene_renderer->enable_native_ui_overlay) {
            const auto invalid_request = invalid_vulkan_native_ui_texture_overlay_request(
                "Vulkan textured native UI overlay requires the native UI overlay pass to be enabled; using "
                "NullRenderer fallback.");
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = invalid_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = invalid_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || invalid_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = invalid_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = invalid_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || invalid_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = false;
        } else if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay &&
                   !desc.vulkan_scene_renderer->enable_postprocess) {
            const auto invalid_request =
                desc.vulkan_scene_renderer->enable_native_ui_overlay_textures
                    ? invalid_vulkan_native_ui_texture_overlay_request(
                          "Vulkan textured native UI overlay requires scene postprocess to be enabled; using "
                          "NullRenderer fallback.")
                    : invalid_vulkan_native_ui_overlay_request(
                          "Vulkan native UI overlay requires scene postprocess to be enabled; using NullRenderer "
                          "fallback.");
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = invalid_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = invalid_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || invalid_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = invalid_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = invalid_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || invalid_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = invalid_request.native_ui_texture_overlay_atlas_ready;
        } else if (desc.vulkan_scene_renderer != nullptr &&
                   desc.vulkan_scene_renderer->enable_directional_shadow_smoke &&
                   (!desc.vulkan_scene_renderer->enable_postprocess ||
                    !desc.vulkan_scene_renderer->enable_postprocess_depth_input)) {
            const auto invalid_request = invalid_vulkan_directional_shadow_request(
                "Vulkan directional shadow smoke requires scene postprocess and postprocess depth input to be "
                "enabled; using NullRenderer fallback.");
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->directional_shadow_status = invalid_request.directional_shadow_status;
            impl_->directional_shadow_diagnostics = invalid_request.directional_shadow_diagnostics;
            impl_->directional_shadow_requested =
                impl_->directional_shadow_requested || invalid_request.directional_shadow_requested;
        } else if (desc.vulkan_scene_renderer != nullptr &&
                   desc.vulkan_scene_renderer->enable_directional_shadow_smoke &&
                   !has_directional_shadow_bytecode(desc.vulkan_scene_renderer->shadow_vertex_shader,
                                                    desc.vulkan_scene_renderer->shadow_fragment_shader)) {
            const auto missing_request = missing_vulkan_directional_shadow_request();
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->directional_shadow_status = missing_request.directional_shadow_status;
            impl_->directional_shadow_diagnostics = missing_request.directional_shadow_diagnostics;
            impl_->directional_shadow_requested =
                impl_->directional_shadow_requested || missing_request.directional_shadow_requested;
        } else if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_postprocess &&
                   !has_postprocess_bytecode(desc.vulkan_scene_renderer->postprocess_vertex_shader,
                                             desc.vulkan_scene_renderer->postprocess_fragment_shader)) {
            const auto missing_request = missing_vulkan_postprocess_request();
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->postprocess_status = missing_request.postprocess_status;
            impl_->postprocess_diagnostics = missing_request.postprocess_diagnostics;
            impl_->postprocess_depth_input_ready = false;
        } else if (desc.vulkan_scene_renderer != nullptr && desc.vulkan_scene_renderer->enable_native_ui_overlay &&
                   !has_native_ui_overlay_bytecode(desc.vulkan_scene_renderer->native_ui_overlay_vertex_shader,
                                                   desc.vulkan_scene_renderer->native_ui_overlay_fragment_shader)) {
            const auto missing_request =
                desc.vulkan_scene_renderer->enable_native_ui_overlay_textures
                    ? invalid_vulkan_native_ui_texture_overlay_request(
                          "Vulkan textured native UI overlay requires non-empty overlay vertex and fragment SPIR-V "
                          "bytecode; using NullRenderer fallback.")
                    : missing_vulkan_native_ui_overlay_request();
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = missing_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = missing_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || missing_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = missing_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = missing_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || missing_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = missing_request.native_ui_texture_overlay_atlas_ready;
        } else if (desc.vulkan_scene_renderer != nullptr &&
                   !valid_vulkan_scene_renderer_request(desc.vulkan_scene_renderer)) {
            const auto missing_request = missing_vulkan_scene_renderer_request();
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
        } else if (desc.vulkan_renderer != nullptr && !valid_vulkan_renderer_request(desc.vulkan_renderer)) {
            const auto missing_request = missing_vulkan_renderer_request();
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
        } else if (desc.vulkan_scene_renderer == nullptr && desc.vulkan_renderer == nullptr) {
            const auto missing_request = missing_vulkan_renderer_request();
            record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
        } else {
            bool scene_request_valid = true;
            if (desc.vulkan_scene_renderer != nullptr) {
                const auto scene_request = validate_scene_renderer_mesh_layout_request(
                    *desc.vulkan_scene_renderer->package, *desc.vulkan_scene_renderer->packet,
                    desc.vulkan_scene_renderer->vertex_buffers, desc.vulkan_scene_renderer->vertex_attributes,
                    desc.vulkan_scene_renderer->skinned_vertex_buffers,
                    desc.vulkan_scene_renderer->skinned_vertex_attributes,
                    desc.vulkan_scene_renderer->compute_morph_skinned_mesh_bindings, "Vulkan");
                if (!scene_request.valid) {
                    record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                          SdlDesktopPresentationBackendReportStatus::missing_request,
                                          SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                                          scene_request.diagnostic);
                    impl_->scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
                    impl_->scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(impl_->scene_gpu_status, scene_request.diagnostic));
                    scene_request_valid = false;
                }
            }
            if (scene_request_valid) {
                const auto surface = probe_vulkan_surface(*desc.window);
                if (surface.surface.value == 0) {
                    record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                          SdlDesktopPresentationBackendReportStatus::native_surface_unavailable,
                                          surface.failure_reason, surface.diagnostic);
                    if (desc.vulkan_scene_renderer != nullptr) {
                        impl_->scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::unavailable;
                        impl_->scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                            impl_->scene_gpu_status,
                            "Vulkan scene GPU bindings require a native Vulkan presentation surface; using "
                            "NullRenderer fallback."));
                        if (desc.vulkan_scene_renderer->enable_postprocess) {
                            impl_->postprocess_status = SdlDesktopPresentationPostprocessStatus::unavailable;
                            impl_->postprocess_diagnostics.push_back(make_postprocess_diagnostic(
                                impl_->postprocess_status,
                                "Vulkan scene postprocess requires a native Vulkan presentation surface; using "
                                "NullRenderer fallback."));
                            if (desc.vulkan_scene_renderer->enable_postprocess_depth_input) {
                                impl_->postprocess_diagnostics.push_back(make_postprocess_diagnostic(
                                    impl_->postprocess_status,
                                    "Vulkan scene postprocess depth input requires a native Vulkan presentation "
                                    "surface; using NullRenderer fallback."));
                            }
                        }
                        if (desc.vulkan_scene_renderer->enable_directional_shadow_smoke) {
                            impl_->directional_shadow_status =
                                SdlDesktopPresentationDirectionalShadowStatus::unavailable;
                            impl_->directional_shadow_diagnostics.push_back(make_directional_shadow_diagnostic(
                                impl_->directional_shadow_status,
                                "Vulkan directional shadow smoke requires a native Vulkan presentation surface; "
                                "using NullRenderer fallback."));
                        }
                        if (desc.vulkan_scene_renderer->enable_native_ui_overlay) {
                            impl_->native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::unavailable;
                            impl_->native_ui_overlay_diagnostics.push_back(make_native_ui_overlay_diagnostic(
                                impl_->native_ui_overlay_status,
                                "Vulkan native UI overlay requires a native Vulkan presentation surface; using "
                                "NullRenderer fallback."));
                        }
                        if (desc.vulkan_scene_renderer->enable_native_ui_overlay_textures) {
                            impl_->native_ui_texture_overlay_status =
                                SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable;
                            impl_->native_ui_texture_overlay_requested = true;
                            impl_->native_ui_texture_overlay_atlas_ready = false;
                            impl_->native_ui_texture_overlay_diagnostics.push_back(
                                make_native_ui_texture_overlay_diagnostic(
                                    impl_->native_ui_texture_overlay_status,
                                    "Vulkan textured native UI overlay requires a native Vulkan presentation surface; "
                                    "using NullRenderer fallback."));
                        }
                    }
                } else if (desc.vulkan_scene_renderer != nullptr) {
                    auto renderer_result = create_vulkan_scene_renderer(desc, surface.surface);
                    if (renderer_result.succeeded) {
                        impl_->backend = SdlDesktopPresentationBackend::vulkan;
                        impl_->scene_gpu_status = renderer_result.scene_gpu_status;
                        impl_->scene_gpu_diagnostics = std::move(renderer_result.scene_gpu_diagnostics);
                        impl_->postprocess_status = renderer_result.postprocess_status;
                        impl_->postprocess_diagnostics = std::move(renderer_result.postprocess_diagnostics);
                        impl_->postprocess_depth_input_requested = impl_->postprocess_depth_input_requested ||
                                                                   renderer_result.postprocess_depth_input_requested;
                        impl_->postprocess_depth_input_ready = renderer_result.postprocess_depth_input_ready;
                        impl_->directional_shadow_requested =
                            impl_->directional_shadow_requested || renderer_result.directional_shadow_requested;
                        impl_->directional_shadow_status = renderer_result.directional_shadow_status;
                        impl_->directional_shadow_diagnostics =
                            std::move(renderer_result.directional_shadow_diagnostics);
                        impl_->directional_shadow_ready = renderer_result.directional_shadow_ready;
                        impl_->directional_shadow_filter_mode = renderer_result.directional_shadow_filter_mode;
                        impl_->directional_shadow_filter_tap_count =
                            renderer_result.directional_shadow_filter_tap_count;
                        impl_->directional_shadow_filter_radius_texels =
                            renderer_result.directional_shadow_filter_radius_texels;
                        impl_->native_ui_overlay_requested =
                            impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                        impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                        impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                        impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                        impl_->native_ui_texture_overlay_requested =
                            impl_->native_ui_texture_overlay_requested ||
                            renderer_result.native_ui_texture_overlay_requested;
                        impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                        impl_->native_ui_texture_overlay_diagnostics =
                            std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                        impl_->native_ui_texture_overlay_atlas_ready =
                            renderer_result.native_ui_texture_overlay_atlas_ready;
                        impl_->framegraph_passes = renderer_result.framegraph_passes;
                        impl_->device = std::move(renderer_result.device);
                        impl_->renderer = std::move(renderer_result.renderer);
                        impl_->scene_gpu_renderer = renderer_result.scene_gpu_renderer;
                        record_backend_ready(SdlDesktopPresentationBackend::vulkan, "Vulkan scene renderer ready.");
                        return;
                    }
                    record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                          backend_report_status_from_fallback_reason(renderer_result.failure_reason),
                                          renderer_result.failure_reason, renderer_result.diagnostic);
                    impl_->scene_gpu_status = renderer_result.scene_gpu_status;
                    impl_->scene_gpu_diagnostics = std::move(renderer_result.scene_gpu_diagnostics);
                    impl_->postprocess_status = renderer_result.postprocess_status;
                    impl_->postprocess_diagnostics = std::move(renderer_result.postprocess_diagnostics);
                    impl_->postprocess_depth_input_requested =
                        impl_->postprocess_depth_input_requested || renderer_result.postprocess_depth_input_requested;
                    impl_->postprocess_depth_input_ready = renderer_result.postprocess_depth_input_ready;
                    impl_->directional_shadow_requested =
                        impl_->directional_shadow_requested || renderer_result.directional_shadow_requested;
                    impl_->directional_shadow_status = renderer_result.directional_shadow_status;
                    impl_->directional_shadow_diagnostics = std::move(renderer_result.directional_shadow_diagnostics);
                    impl_->directional_shadow_ready = renderer_result.directional_shadow_ready;
                    impl_->directional_shadow_filter_mode = renderer_result.directional_shadow_filter_mode;
                    impl_->directional_shadow_filter_tap_count = renderer_result.directional_shadow_filter_tap_count;
                    impl_->directional_shadow_filter_radius_texels =
                        renderer_result.directional_shadow_filter_radius_texels;
                    impl_->native_ui_overlay_requested =
                        impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                    impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                    impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                    impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                    impl_->native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested ||
                                                                 renderer_result.native_ui_texture_overlay_requested;
                    impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                    impl_->native_ui_texture_overlay_diagnostics =
                        std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                    impl_->native_ui_texture_overlay_atlas_ready =
                        renderer_result.native_ui_texture_overlay_atlas_ready;
                    impl_->framegraph_passes = renderer_result.framegraph_passes;
                } else {
                    auto renderer_result = create_vulkan_renderer(desc, surface.surface);
                    if (renderer_result.succeeded) {
                        impl_->backend = SdlDesktopPresentationBackend::vulkan;
                        impl_->device = std::move(renderer_result.device);
                        impl_->renderer = std::move(renderer_result.renderer);
                        impl_->native_ui_overlay_requested =
                            impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                        impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                        impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                        impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                        impl_->native_ui_texture_overlay_requested =
                            impl_->native_ui_texture_overlay_requested ||
                            renderer_result.native_ui_texture_overlay_requested;
                        impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                        impl_->native_ui_texture_overlay_diagnostics =
                            std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                        impl_->native_ui_texture_overlay_atlas_ready =
                            renderer_result.native_ui_texture_overlay_atlas_ready;
                        record_backend_ready(SdlDesktopPresentationBackend::vulkan, "Vulkan renderer ready.");
                        return;
                    }
                    record_backend_report(SdlDesktopPresentationBackend::vulkan,
                                          backend_report_status_from_fallback_reason(renderer_result.failure_reason),
                                          renderer_result.failure_reason, renderer_result.diagnostic);
                    impl_->native_ui_overlay_requested =
                        impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                    impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                    impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                    impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                    impl_->native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested ||
                                                                 renderer_result.native_ui_texture_overlay_requested;
                    impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                    impl_->native_ui_texture_overlay_diagnostics =
                        std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                    impl_->native_ui_texture_overlay_atlas_ready =
                        renderer_result.native_ui_texture_overlay_atlas_ready;
                }
            }
        }
    } else if (desc.prefer_d3d12) {
        if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_postprocess_depth_input &&
            !desc.d3d12_scene_renderer->enable_postprocess) {
            const auto invalid_request = invalid_d3d12_postprocess_depth_input_request();
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->postprocess_status = invalid_request.postprocess_status;
            impl_->postprocess_diagnostics = invalid_request.postprocess_diagnostics;
            impl_->postprocess_depth_input_requested =
                impl_->postprocess_depth_input_requested || invalid_request.postprocess_depth_input_requested;
            impl_->postprocess_depth_input_ready = false;
        } else if (desc.d3d12_scene_renderer != nullptr &&
                   desc.d3d12_scene_renderer->enable_native_ui_overlay_textures &&
                   !desc.d3d12_scene_renderer->enable_native_ui_overlay) {
            const auto invalid_request = invalid_d3d12_native_ui_texture_overlay_request(
                "D3D12 textured native UI overlay requires the native UI overlay pass to be enabled; using "
                "NullRenderer fallback.");
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = invalid_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = invalid_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || invalid_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = invalid_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = invalid_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || invalid_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = false;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay &&
                   !desc.d3d12_scene_renderer->enable_postprocess) {
            const auto invalid_request =
                desc.d3d12_scene_renderer->enable_native_ui_overlay_textures
                    ? invalid_d3d12_native_ui_texture_overlay_request(
                          "D3D12 textured native UI overlay requires scene postprocess to be enabled; using "
                          "NullRenderer fallback.")
                    : invalid_d3d12_native_ui_overlay_request(
                          "D3D12 native UI overlay requires scene postprocess to be enabled; using NullRenderer "
                          "fallback.");
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = invalid_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = invalid_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || invalid_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = invalid_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = invalid_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || invalid_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = invalid_request.native_ui_texture_overlay_atlas_ready;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke &&
                   (!desc.d3d12_scene_renderer->enable_postprocess ||
                    !desc.d3d12_scene_renderer->enable_postprocess_depth_input)) {
            const auto invalid_request = invalid_d3d12_directional_shadow_request(
                "D3D12 directional shadow smoke requires scene postprocess and postprocess depth input to be "
                "enabled; using NullRenderer fallback.");
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  invalid_request.failure_reason, invalid_request.diagnostic);
            impl_->scene_gpu_status = invalid_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = invalid_request.scene_gpu_diagnostics;
            impl_->directional_shadow_status = invalid_request.directional_shadow_status;
            impl_->directional_shadow_diagnostics = invalid_request.directional_shadow_diagnostics;
            impl_->directional_shadow_requested =
                impl_->directional_shadow_requested || invalid_request.directional_shadow_requested;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_directional_shadow_smoke &&
                   !has_directional_shadow_bytecode(desc.d3d12_scene_renderer->shadow_vertex_shader,
                                                    desc.d3d12_scene_renderer->shadow_fragment_shader)) {
            const auto missing_request = missing_d3d12_directional_shadow_request();
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->directional_shadow_status = missing_request.directional_shadow_status;
            impl_->directional_shadow_diagnostics = missing_request.directional_shadow_diagnostics;
            impl_->directional_shadow_requested =
                impl_->directional_shadow_requested || missing_request.directional_shadow_requested;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_postprocess &&
                   !has_postprocess_bytecode(desc.d3d12_scene_renderer->postprocess_vertex_shader,
                                             desc.d3d12_scene_renderer->postprocess_fragment_shader)) {
            const auto missing_request = missing_d3d12_postprocess_request();
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->postprocess_status = missing_request.postprocess_status;
            impl_->postprocess_diagnostics = missing_request.postprocess_diagnostics;
            impl_->postprocess_depth_input_ready = false;
        } else if (desc.d3d12_scene_renderer != nullptr && desc.d3d12_scene_renderer->enable_native_ui_overlay &&
                   !has_native_ui_overlay_bytecode(desc.d3d12_scene_renderer->native_ui_overlay_vertex_shader,
                                                   desc.d3d12_scene_renderer->native_ui_overlay_fragment_shader)) {
            const auto missing_request =
                desc.d3d12_scene_renderer->enable_native_ui_overlay_textures
                    ? invalid_d3d12_native_ui_texture_overlay_request(
                          "D3D12 textured native UI overlay requires non-empty overlay vertex and fragment shader "
                          "bytecode; using NullRenderer fallback.")
                    : missing_d3d12_native_ui_overlay_request();
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
            impl_->native_ui_overlay_status = missing_request.native_ui_overlay_status;
            impl_->native_ui_overlay_diagnostics = missing_request.native_ui_overlay_diagnostics;
            impl_->native_ui_overlay_requested =
                impl_->native_ui_overlay_requested || missing_request.native_ui_overlay_requested;
            impl_->native_ui_texture_overlay_status = missing_request.native_ui_texture_overlay_status;
            impl_->native_ui_texture_overlay_diagnostics = missing_request.native_ui_texture_overlay_diagnostics;
            impl_->native_ui_texture_overlay_requested =
                impl_->native_ui_texture_overlay_requested || missing_request.native_ui_texture_overlay_requested;
            impl_->native_ui_texture_overlay_atlas_ready = missing_request.native_ui_texture_overlay_atlas_ready;
        } else if (desc.d3d12_scene_renderer != nullptr &&
                   !valid_d3d12_scene_renderer_request(desc.d3d12_scene_renderer)) {
            const auto missing_request = missing_d3d12_scene_renderer_request();
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
            impl_->scene_gpu_status = missing_request.scene_gpu_status;
            impl_->scene_gpu_diagnostics = missing_request.scene_gpu_diagnostics;
        } else if (desc.d3d12_renderer != nullptr && !valid_d3d12_renderer_request(desc.d3d12_renderer)) {
            const auto missing_request = missing_d3d12_renderer_request();
            record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                  SdlDesktopPresentationBackendReportStatus::missing_request,
                                  missing_request.failure_reason, missing_request.diagnostic);
        } else {
            bool scene_request_valid = true;
            if (desc.d3d12_scene_renderer != nullptr) {
                const auto scene_request = validate_scene_renderer_mesh_layout_request(
                    *desc.d3d12_scene_renderer->package, *desc.d3d12_scene_renderer->packet,
                    desc.d3d12_scene_renderer->vertex_buffers, desc.d3d12_scene_renderer->vertex_attributes,
                    desc.d3d12_scene_renderer->skinned_vertex_buffers,
                    desc.d3d12_scene_renderer->skinned_vertex_attributes,
                    desc.d3d12_scene_renderer->compute_morph_skinned_mesh_bindings, "D3D12");
                if (!scene_request.valid) {
                    record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                          SdlDesktopPresentationBackendReportStatus::missing_request,
                                          SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable,
                                          scene_request.diagnostic);
                    impl_->scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::invalid_request;
                    impl_->scene_gpu_diagnostics.push_back(
                        make_scene_gpu_diagnostic(impl_->scene_gpu_status, scene_request.diagnostic));
                    scene_request_valid = false;
                }
            }
            if (scene_request_valid) {
                const auto surface = probe_d3d12_surface(*desc.window);
                if (surface.surface.value == 0) {
                    record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                          SdlDesktopPresentationBackendReportStatus::native_surface_unavailable,
                                          surface.failure_reason, surface.diagnostic);
                    if (desc.d3d12_scene_renderer != nullptr) {
                        impl_->scene_gpu_status = SdlDesktopPresentationSceneGpuBindingStatus::unavailable;
                        impl_->scene_gpu_diagnostics.push_back(make_scene_gpu_diagnostic(
                            impl_->scene_gpu_status,
                            "D3D12 scene GPU bindings require a native D3D12 presentation surface; using "
                            "NullRenderer fallback."));
                        if (desc.d3d12_scene_renderer->enable_postprocess) {
                            impl_->postprocess_status = SdlDesktopPresentationPostprocessStatus::unavailable;
                            impl_->postprocess_diagnostics.push_back(make_postprocess_diagnostic(
                                impl_->postprocess_status,
                                "D3D12 scene postprocess requires a native D3D12 presentation surface; using "
                                "NullRenderer fallback."));
                            if (desc.d3d12_scene_renderer->enable_postprocess_depth_input) {
                                impl_->postprocess_diagnostics.push_back(make_postprocess_diagnostic(
                                    impl_->postprocess_status,
                                    "D3D12 scene postprocess depth input requires a native D3D12 presentation "
                                    "surface; using NullRenderer fallback."));
                            }
                        }
                        if (desc.d3d12_scene_renderer->enable_directional_shadow_smoke) {
                            impl_->directional_shadow_status =
                                SdlDesktopPresentationDirectionalShadowStatus::unavailable;
                            impl_->directional_shadow_diagnostics.push_back(make_directional_shadow_diagnostic(
                                impl_->directional_shadow_status,
                                "D3D12 directional shadow smoke requires a native D3D12 presentation surface; "
                                "using NullRenderer fallback."));
                        }
                        if (desc.d3d12_scene_renderer->enable_native_ui_overlay) {
                            impl_->native_ui_overlay_status = SdlDesktopPresentationNativeUiOverlayStatus::unavailable;
                            impl_->native_ui_overlay_diagnostics.push_back(make_native_ui_overlay_diagnostic(
                                impl_->native_ui_overlay_status,
                                "D3D12 native UI overlay requires a native D3D12 presentation surface; using "
                                "NullRenderer fallback."));
                        }
                        if (desc.d3d12_scene_renderer->enable_native_ui_overlay_textures) {
                            impl_->native_ui_texture_overlay_status =
                                SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable;
                            impl_->native_ui_texture_overlay_requested = true;
                            impl_->native_ui_texture_overlay_atlas_ready = false;
                            impl_->native_ui_texture_overlay_diagnostics.push_back(
                                make_native_ui_texture_overlay_diagnostic(
                                    impl_->native_ui_texture_overlay_status,
                                    "D3D12 textured native UI overlay requires a native D3D12 presentation surface; "
                                    "using NullRenderer fallback."));
                        }
                    }
                } else if (desc.d3d12_scene_renderer != nullptr) {
                    auto renderer_result = create_d3d12_scene_renderer(desc, surface.surface);
                    if (renderer_result.succeeded) {
                        impl_->backend = SdlDesktopPresentationBackend::d3d12;
                        impl_->scene_gpu_status = renderer_result.scene_gpu_status;
                        impl_->scene_gpu_diagnostics = std::move(renderer_result.scene_gpu_diagnostics);
                        impl_->postprocess_status = renderer_result.postprocess_status;
                        impl_->postprocess_diagnostics = std::move(renderer_result.postprocess_diagnostics);
                        impl_->postprocess_depth_input_requested = impl_->postprocess_depth_input_requested ||
                                                                   renderer_result.postprocess_depth_input_requested;
                        impl_->postprocess_depth_input_ready = renderer_result.postprocess_depth_input_ready;
                        impl_->directional_shadow_requested =
                            impl_->directional_shadow_requested || renderer_result.directional_shadow_requested;
                        impl_->directional_shadow_status = renderer_result.directional_shadow_status;
                        impl_->directional_shadow_diagnostics =
                            std::move(renderer_result.directional_shadow_diagnostics);
                        impl_->directional_shadow_ready = renderer_result.directional_shadow_ready;
                        impl_->directional_shadow_filter_mode = renderer_result.directional_shadow_filter_mode;
                        impl_->directional_shadow_filter_tap_count =
                            renderer_result.directional_shadow_filter_tap_count;
                        impl_->directional_shadow_filter_radius_texels =
                            renderer_result.directional_shadow_filter_radius_texels;
                        impl_->native_ui_overlay_requested =
                            impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                        impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                        impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                        impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                        impl_->native_ui_texture_overlay_requested =
                            impl_->native_ui_texture_overlay_requested ||
                            renderer_result.native_ui_texture_overlay_requested;
                        impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                        impl_->native_ui_texture_overlay_diagnostics =
                            std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                        impl_->native_ui_texture_overlay_atlas_ready =
                            renderer_result.native_ui_texture_overlay_atlas_ready;
                        impl_->framegraph_passes = renderer_result.framegraph_passes;
                        impl_->device = std::move(renderer_result.device);
                        impl_->renderer = std::move(renderer_result.renderer);
                        impl_->scene_gpu_renderer = renderer_result.scene_gpu_renderer;
                        record_backend_ready(SdlDesktopPresentationBackend::d3d12, "D3D12 scene renderer ready.");
                        return;
                    }
                    record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                          backend_report_status_from_fallback_reason(renderer_result.failure_reason),
                                          renderer_result.failure_reason, renderer_result.diagnostic);
                    impl_->scene_gpu_status = renderer_result.scene_gpu_status;
                    impl_->scene_gpu_diagnostics = std::move(renderer_result.scene_gpu_diagnostics);
                    impl_->postprocess_status = renderer_result.postprocess_status;
                    impl_->postprocess_diagnostics = std::move(renderer_result.postprocess_diagnostics);
                    impl_->postprocess_depth_input_requested =
                        impl_->postprocess_depth_input_requested || renderer_result.postprocess_depth_input_requested;
                    impl_->postprocess_depth_input_ready = renderer_result.postprocess_depth_input_ready;
                    impl_->directional_shadow_requested =
                        impl_->directional_shadow_requested || renderer_result.directional_shadow_requested;
                    impl_->directional_shadow_status = renderer_result.directional_shadow_status;
                    impl_->directional_shadow_diagnostics = std::move(renderer_result.directional_shadow_diagnostics);
                    impl_->directional_shadow_ready = renderer_result.directional_shadow_ready;
                    impl_->directional_shadow_filter_mode = renderer_result.directional_shadow_filter_mode;
                    impl_->directional_shadow_filter_tap_count = renderer_result.directional_shadow_filter_tap_count;
                    impl_->directional_shadow_filter_radius_texels =
                        renderer_result.directional_shadow_filter_radius_texels;
                    impl_->native_ui_overlay_requested =
                        impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                    impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                    impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                    impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                    impl_->native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested ||
                                                                 renderer_result.native_ui_texture_overlay_requested;
                    impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                    impl_->native_ui_texture_overlay_diagnostics =
                        std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                    impl_->native_ui_texture_overlay_atlas_ready =
                        renderer_result.native_ui_texture_overlay_atlas_ready;
                    impl_->framegraph_passes = renderer_result.framegraph_passes;
                } else if (desc.d3d12_renderer == nullptr) {
                    const auto missing_request = missing_d3d12_renderer_request();
                    record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                          SdlDesktopPresentationBackendReportStatus::missing_request,
                                          missing_request.failure_reason, missing_request.diagnostic);
                } else {
                    auto renderer_result = create_d3d12_renderer(desc, surface.surface);
                    if (renderer_result.succeeded) {
                        impl_->backend = SdlDesktopPresentationBackend::d3d12;
                        impl_->device = std::move(renderer_result.device);
                        impl_->renderer = std::move(renderer_result.renderer);
                        impl_->native_ui_overlay_requested =
                            impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                        impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                        impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                        impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                        impl_->native_ui_texture_overlay_requested =
                            impl_->native_ui_texture_overlay_requested ||
                            renderer_result.native_ui_texture_overlay_requested;
                        impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                        impl_->native_ui_texture_overlay_diagnostics =
                            std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                        impl_->native_ui_texture_overlay_atlas_ready =
                            renderer_result.native_ui_texture_overlay_atlas_ready;
                        record_backend_ready(SdlDesktopPresentationBackend::d3d12, "D3D12 renderer ready.");
                        return;
                    }
                    record_backend_report(SdlDesktopPresentationBackend::d3d12,
                                          backend_report_status_from_fallback_reason(renderer_result.failure_reason),
                                          renderer_result.failure_reason, renderer_result.diagnostic);
                    impl_->native_ui_overlay_requested =
                        impl_->native_ui_overlay_requested || renderer_result.native_ui_overlay_requested;
                    impl_->native_ui_overlay_status = renderer_result.native_ui_overlay_status;
                    impl_->native_ui_overlay_diagnostics = std::move(renderer_result.native_ui_overlay_diagnostics);
                    impl_->native_ui_overlay_ready = renderer_result.native_ui_overlay_ready;
                    impl_->native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested ||
                                                                 renderer_result.native_ui_texture_overlay_requested;
                    impl_->native_ui_texture_overlay_status = renderer_result.native_ui_texture_overlay_status;
                    impl_->native_ui_texture_overlay_diagnostics =
                        std::move(renderer_result.native_ui_texture_overlay_diagnostics);
                    impl_->native_ui_texture_overlay_atlas_ready =
                        renderer_result.native_ui_texture_overlay_atlas_ready;
                }
            }
        }
    } else {
        record_backend_report(
            SdlDesktopPresentationBackend::null_renderer, SdlDesktopPresentationBackendReportStatus::not_requested,
            SdlDesktopPresentationFallbackReason::none, "Native presentation is not selected; using NullRenderer.");
    }

    if (!desc.allow_null_fallback) {
        throw std::logic_error(fallback_message);
    }

    impl_->backend = SdlDesktopPresentationBackend::null_renderer;
    impl_->fallback_reason = fallback_reason;
    impl_->diagnostics.push_back(make_diagnostic(fallback_reason, std::move(fallback_message)));
    impl_->renderer = std::make_unique<NullRenderer>(desc.extent);
}

SdlDesktopPresentation::~SdlDesktopPresentation() = default;

IRenderer& SdlDesktopPresentation::renderer() noexcept {
    return *impl_->renderer;
}

const IRenderer& SdlDesktopPresentation::renderer() const noexcept {
    return *impl_->renderer;
}

SdlDesktopPresentationBackend SdlDesktopPresentation::backend() const noexcept {
    return impl_->backend;
}

std::string_view SdlDesktopPresentation::backend_name() const noexcept {
    return sdl_desktop_presentation_backend_name(impl_->backend);
}

SdlDesktopPresentationReport SdlDesktopPresentation::report() const noexcept {
    const auto renderer_stats = impl_->renderer != nullptr ? impl_->renderer->stats() : RendererStats{};
    return SdlDesktopPresentationReport{
        .requested_backend = impl_->requested_backend,
        .selected_backend = impl_->backend,
        .fallback_reason = impl_->fallback_reason,
        .used_null_fallback = impl_->backend == SdlDesktopPresentationBackend::null_renderer &&
                              impl_->requested_backend != SdlDesktopPresentationBackend::null_renderer,
        .allow_null_fallback = impl_->allow_null_fallback,
        .scene_gpu_status = impl_->scene_gpu_status,
        .scene_gpu_stats = scene_gpu_binding_stats(),
        .postprocess_status = impl_->postprocess_status,
        .postprocess_depth_input_requested = impl_->postprocess_depth_input_requested,
        .postprocess_depth_input_ready = impl_->postprocess_depth_input_ready,
        .directional_shadow_status = impl_->directional_shadow_status,
        .directional_shadow_requested = impl_->directional_shadow_requested,
        .directional_shadow_ready = impl_->directional_shadow_ready,
        .directional_shadow_filter_mode = impl_->directional_shadow_filter_mode,
        .directional_shadow_filter_tap_count = impl_->directional_shadow_filter_tap_count,
        .directional_shadow_filter_radius_texels = impl_->directional_shadow_filter_radius_texels,
        .native_ui_overlay_status = impl_->native_ui_overlay_status,
        .native_ui_overlay_requested = impl_->native_ui_overlay_requested,
        .native_ui_overlay_ready = impl_->native_ui_overlay_ready,
        .native_ui_overlay_sprites_submitted = renderer_stats.native_ui_overlay_sprites_submitted,
        .native_ui_overlay_draws = renderer_stats.native_ui_overlay_draws,
        .native_ui_texture_overlay_status = impl_->native_ui_texture_overlay_status,
        .native_ui_texture_overlay_requested = impl_->native_ui_texture_overlay_requested,
        .native_ui_texture_overlay_atlas_ready = impl_->native_ui_texture_overlay_atlas_ready,
        .native_ui_texture_overlay_sprites_submitted = renderer_stats.native_ui_overlay_textured_sprites_submitted,
        .native_ui_texture_overlay_texture_binds = renderer_stats.native_ui_overlay_texture_binds,
        .native_ui_texture_overlay_draws = renderer_stats.native_ui_overlay_textured_draws,
        .framegraph_passes = impl_->framegraph_passes,
        .renderer_stats = renderer_stats,
        .backbuffer_extent = impl_->renderer != nullptr ? impl_->renderer->backbuffer_extent() : Extent2D{},
        .diagnostics_count = impl_->diagnostics.size(),
        .backend_reports_count = impl_->backend_reports.size(),
        .scene_gpu_diagnostics_count = impl_->scene_gpu_diagnostics.size(),
        .postprocess_diagnostics_count = impl_->postprocess_diagnostics.size(),
        .directional_shadow_diagnostics_count = impl_->directional_shadow_diagnostics.size(),
        .native_ui_overlay_diagnostics_count = impl_->native_ui_overlay_diagnostics.size(),
        .native_ui_texture_overlay_diagnostics_count = impl_->native_ui_texture_overlay_diagnostics.size(),
    };
}

std::span<const SdlDesktopPresentationBackendReport> SdlDesktopPresentation::backend_reports() const noexcept {
    return impl_->backend_reports;
}

std::span<const SdlDesktopPresentationDiagnostic> SdlDesktopPresentation::diagnostics() const noexcept {
    return impl_->diagnostics;
}

SdlDesktopPresentationSceneGpuBindingStatus SdlDesktopPresentation::scene_gpu_binding_status() const noexcept {
    return impl_->scene_gpu_status;
}

bool SdlDesktopPresentation::scene_gpu_bindings_ready() const noexcept {
    return impl_->scene_gpu_status == SdlDesktopPresentationSceneGpuBindingStatus::ready;
}

SdlDesktopPresentationSceneGpuBindingStats SdlDesktopPresentation::scene_gpu_binding_stats() const noexcept {
    if (impl_->scene_gpu_renderer != nullptr) {
        return impl_->scene_gpu_renderer->scene_gpu_binding_stats();
    }
    return {};
}

rhi::BufferHandle SdlDesktopPresentation::scene_pbr_frame_uniform_buffer() const noexcept {
    if (impl_->scene_gpu_renderer == nullptr) {
        return {};
    }
    return impl_->scene_gpu_renderer->scene_pbr_frame_uniform_buffer();
}

rhi::IRhiDevice* SdlDesktopPresentation::scene_pbr_frame_rhi_device() noexcept {
    return impl_->device.get();
}

const rhi::IRhiDevice* SdlDesktopPresentation::scene_pbr_frame_rhi_device() const noexcept {
    return impl_->device.get();
}

std::span<const SdlDesktopPresentationSceneGpuBindingDiagnostic>
SdlDesktopPresentation::scene_gpu_binding_diagnostics() const noexcept {
    return impl_->scene_gpu_diagnostics;
}

SdlDesktopPresentationPostprocessStatus SdlDesktopPresentation::postprocess_status() const noexcept {
    return impl_->postprocess_status;
}

bool SdlDesktopPresentation::postprocess_ready() const noexcept {
    return impl_->postprocess_status == SdlDesktopPresentationPostprocessStatus::ready;
}

bool SdlDesktopPresentation::postprocess_depth_input_ready() const noexcept {
    return impl_->postprocess_depth_input_ready;
}

std::span<const SdlDesktopPresentationPostprocessDiagnostic>
SdlDesktopPresentation::postprocess_diagnostics() const noexcept {
    return impl_->postprocess_diagnostics;
}

SdlDesktopPresentationDirectionalShadowStatus SdlDesktopPresentation::directional_shadow_status() const noexcept {
    return impl_->directional_shadow_status;
}

bool SdlDesktopPresentation::directional_shadow_ready() const noexcept {
    return impl_->directional_shadow_ready;
}

std::span<const SdlDesktopPresentationDirectionalShadowDiagnostic>
SdlDesktopPresentation::directional_shadow_diagnostics() const noexcept {
    return impl_->directional_shadow_diagnostics;
}

SdlDesktopPresentationNativeUiOverlayStatus SdlDesktopPresentation::native_ui_overlay_status() const noexcept {
    return impl_->native_ui_overlay_status;
}

bool SdlDesktopPresentation::native_ui_overlay_ready() const noexcept {
    return impl_->native_ui_overlay_ready;
}

std::span<const SdlDesktopPresentationNativeUiOverlayDiagnostic>
SdlDesktopPresentation::native_ui_overlay_diagnostics() const noexcept {
    return impl_->native_ui_overlay_diagnostics;
}

SdlDesktopPresentationNativeUiTextureOverlayStatus
SdlDesktopPresentation::native_ui_texture_overlay_status() const noexcept {
    return impl_->native_ui_texture_overlay_status;
}

bool SdlDesktopPresentation::native_ui_texture_overlay_atlas_ready() const noexcept {
    return impl_->native_ui_texture_overlay_atlas_ready;
}

std::span<const SdlDesktopPresentationNativeUiTextureOverlayDiagnostic>
SdlDesktopPresentation::native_ui_texture_overlay_diagnostics() const noexcept {
    return impl_->native_ui_texture_overlay_diagnostics;
}

std::string_view sdl_desktop_presentation_backend_name(SdlDesktopPresentationBackend backend) noexcept {
    switch (backend) {
    case SdlDesktopPresentationBackend::null_renderer:
        return "null";
    case SdlDesktopPresentationBackend::d3d12:
        return "d3d12";
    case SdlDesktopPresentationBackend::vulkan:
        return "vulkan";
    }
    return "unknown";
}

std::string_view
sdl_desktop_presentation_backend_report_status_name(SdlDesktopPresentationBackendReportStatus status) noexcept {
    switch (status) {
    case SdlDesktopPresentationBackendReportStatus::not_requested:
        return "not_requested";
    case SdlDesktopPresentationBackendReportStatus::missing_request:
        return "missing_request";
    case SdlDesktopPresentationBackendReportStatus::native_surface_unavailable:
        return "native_surface_unavailable";
    case SdlDesktopPresentationBackendReportStatus::native_backend_unavailable:
        return "native_backend_unavailable";
    case SdlDesktopPresentationBackendReportStatus::runtime_pipeline_unavailable:
        return "runtime_pipeline_unavailable";
    case SdlDesktopPresentationBackendReportStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view sdl_desktop_presentation_fallback_reason_name(SdlDesktopPresentationFallbackReason reason) noexcept {
    switch (reason) {
    case SdlDesktopPresentationFallbackReason::none:
        return "none";
    case SdlDesktopPresentationFallbackReason::native_surface_unavailable:
        return "native_surface_unavailable";
    case SdlDesktopPresentationFallbackReason::native_backend_unavailable:
        return "native_backend_unavailable";
    case SdlDesktopPresentationFallbackReason::runtime_pipeline_unavailable:
        return "runtime_pipeline_unavailable";
    }
    return "unknown";
}

std::string_view
sdl_desktop_presentation_scene_gpu_binding_status_name(SdlDesktopPresentationSceneGpuBindingStatus status) noexcept {
    switch (status) {
    case SdlDesktopPresentationSceneGpuBindingStatus::not_requested:
        return "not_requested";
    case SdlDesktopPresentationSceneGpuBindingStatus::unavailable:
        return "unavailable";
    case SdlDesktopPresentationSceneGpuBindingStatus::invalid_request:
        return "invalid_request";
    case SdlDesktopPresentationSceneGpuBindingStatus::failed:
        return "failed";
    case SdlDesktopPresentationSceneGpuBindingStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
sdl_desktop_presentation_postprocess_status_name(SdlDesktopPresentationPostprocessStatus status) noexcept {
    switch (status) {
    case SdlDesktopPresentationPostprocessStatus::not_requested:
        return "not_requested";
    case SdlDesktopPresentationPostprocessStatus::unavailable:
        return "unavailable";
    case SdlDesktopPresentationPostprocessStatus::invalid_request:
        return "invalid_request";
    case SdlDesktopPresentationPostprocessStatus::failed:
        return "failed";
    case SdlDesktopPresentationPostprocessStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
sdl_desktop_presentation_directional_shadow_status_name(SdlDesktopPresentationDirectionalShadowStatus status) noexcept {
    switch (status) {
    case SdlDesktopPresentationDirectionalShadowStatus::not_requested:
        return "not_requested";
    case SdlDesktopPresentationDirectionalShadowStatus::unavailable:
        return "unavailable";
    case SdlDesktopPresentationDirectionalShadowStatus::invalid_request:
        return "invalid_request";
    case SdlDesktopPresentationDirectionalShadowStatus::failed:
        return "failed";
    case SdlDesktopPresentationDirectionalShadowStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view sdl_desktop_presentation_directional_shadow_filter_mode_name(
    SdlDesktopPresentationDirectionalShadowFilterMode mode) noexcept {
    switch (mode) {
    case SdlDesktopPresentationDirectionalShadowFilterMode::none:
        return "none";
    case SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3:
        return "fixed_pcf_3x3";
    }
    return "unknown";
}

std::string_view
sdl_desktop_presentation_native_ui_overlay_status_name(SdlDesktopPresentationNativeUiOverlayStatus status) noexcept {
    switch (status) {
    case SdlDesktopPresentationNativeUiOverlayStatus::not_requested:
        return "not_requested";
    case SdlDesktopPresentationNativeUiOverlayStatus::unavailable:
        return "unavailable";
    case SdlDesktopPresentationNativeUiOverlayStatus::invalid_request:
        return "invalid_request";
    case SdlDesktopPresentationNativeUiOverlayStatus::failed:
        return "failed";
    case SdlDesktopPresentationNativeUiOverlayStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view sdl_desktop_presentation_native_ui_texture_overlay_status_name(
    SdlDesktopPresentationNativeUiTextureOverlayStatus status) noexcept {
    switch (status) {
    case SdlDesktopPresentationNativeUiTextureOverlayStatus::not_requested:
        return "not_requested";
    case SdlDesktopPresentationNativeUiTextureOverlayStatus::unavailable:
        return "unavailable";
    case SdlDesktopPresentationNativeUiTextureOverlayStatus::invalid_request:
        return "invalid_request";
    case SdlDesktopPresentationNativeUiTextureOverlayStatus::failed:
        return "failed";
    case SdlDesktopPresentationNativeUiTextureOverlayStatus::ready:
        return "ready";
    }
    return "unknown";
}

std::string_view
sdl_desktop_presentation_quality_gate_status_name(SdlDesktopPresentationQualityGateStatus status) noexcept {
    switch (status) {
    case SdlDesktopPresentationQualityGateStatus::not_requested:
        return "not_requested";
    case SdlDesktopPresentationQualityGateStatus::blocked:
        return "blocked";
    case SdlDesktopPresentationQualityGateStatus::ready:
        return "ready";
    }
    return "unknown";
}

SdlDesktopPresentationQualityGateReport
evaluate_sdl_desktop_presentation_quality_gate(const SdlDesktopPresentationReport& report,
                                               const SdlDesktopPresentationQualityGateDesc& desc) noexcept {
    SdlDesktopPresentationQualityGateReport result;
    result.expected_framegraph_passes = quality_gate_expected_framegraph_passes(desc);
    result.expected_framegraph_barrier_steps = quality_gate_expected_framegraph_barrier_steps(desc);
    if (!quality_gate_requested(desc)) {
        return result;
    }

    auto add_diagnostic = [&result]() noexcept { ++result.diagnostics_count; };

    if (desc.require_scene_gpu_bindings) {
        const auto mesh_bindings = report.scene_gpu_stats.mesh_bindings + report.scene_gpu_stats.skinned_mesh_bindings;
        const auto mesh_bindings_resolved =
            report.scene_gpu_stats.mesh_bindings_resolved + report.scene_gpu_stats.skinned_mesh_bindings_resolved;
        result.scene_gpu_ready = report.scene_gpu_status == SdlDesktopPresentationSceneGpuBindingStatus::ready &&
                                 mesh_bindings > 0 && report.scene_gpu_stats.material_bindings > 0;
        if (desc.expected_frames > 0) {
            result.scene_gpu_ready = result.scene_gpu_ready && mesh_bindings_resolved == desc.expected_frames &&
                                     report.scene_gpu_stats.material_bindings_resolved == desc.expected_frames;
        }
        if (!result.scene_gpu_ready) {
            add_diagnostic();
        }
    }

    if (desc.require_postprocess) {
        result.postprocess_ready = report.postprocess_status == SdlDesktopPresentationPostprocessStatus::ready;
        if (!result.postprocess_ready) {
            add_diagnostic();
        }
    }

    if (desc.require_postprocess_depth_input) {
        result.postprocess_depth_input_ready =
            report.postprocess_depth_input_requested && report.postprocess_depth_input_ready;
        if (!result.postprocess_depth_input_ready) {
            add_diagnostic();
        }
    }

    if (desc.require_directional_shadow) {
        result.directional_shadow_ready =
            report.directional_shadow_requested &&
            report.directional_shadow_status == SdlDesktopPresentationDirectionalShadowStatus::ready &&
            report.directional_shadow_ready;
        if (!result.directional_shadow_ready) {
            add_diagnostic();
        }
    }

    if (desc.require_directional_shadow_filtering) {
        result.directional_shadow_filter_ready =
            report.directional_shadow_filter_mode == SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 &&
            report.directional_shadow_filter_tap_count == 9 && report.directional_shadow_filter_radius_texels == 1.0F;
        if (!result.directional_shadow_filter_ready) {
            add_diagnostic();
        }
    }

    if (result.expected_framegraph_passes > 0) {
        result.framegraph_passes_current = report.framegraph_passes == result.expected_framegraph_passes;
        if (!result.framegraph_passes_current) {
            add_diagnostic();
        }
    }

    if (result.expected_framegraph_barrier_steps > 0) {
        if (desc.expected_frames > 0) {
            const auto expected_framegraph_executions =
                desc.expected_frames * static_cast<std::uint64_t>(result.expected_framegraph_passes);
            const auto expected_framegraph_barriers =
                desc.expected_frames * static_cast<std::uint64_t>(result.expected_framegraph_barrier_steps);
            result.framegraph_barrier_steps_current =
                report.renderer_stats.framegraph_barrier_steps_executed == expected_framegraph_barriers;
            result.framegraph_execution_budget_current =
                report.renderer_stats.frames_finished == desc.expected_frames &&
                report.renderer_stats.framegraph_passes_executed == expected_framegraph_executions &&
                result.framegraph_barrier_steps_current &&
                (!desc.require_postprocess ||
                 report.renderer_stats.postprocess_passes_executed == desc.expected_frames);
        } else {
            result.framegraph_barrier_steps_current =
                report.renderer_stats.framegraph_barrier_steps_executed >=
                static_cast<std::uint64_t>(result.expected_framegraph_barrier_steps);
            result.framegraph_execution_budget_current =
                result.framegraph_passes_current && result.framegraph_barrier_steps_current;
        }
        if (!result.framegraph_barrier_steps_current) {
            add_diagnostic();
        }
        if (!result.framegraph_execution_budget_current) {
            add_diagnostic();
        }
    } else if (result.expected_framegraph_passes > 0) {
        result.framegraph_execution_budget_current = result.framegraph_passes_current;
    }

    result.ready = result.diagnostics_count == 0;
    result.status = result.ready ? SdlDesktopPresentationQualityGateStatus::ready
                                 : SdlDesktopPresentationQualityGateStatus::blocked;
    return result;
}

} // namespace mirakana
