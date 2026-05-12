// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/sdl3/sdl_window.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

namespace runtime {
class RuntimeAssetPackage;
}

struct SceneRenderPacket;

enum class SdlDesktopPresentationBackend {
    null_renderer = 0,
    d3d12,
    vulkan,
};

enum class SdlDesktopPresentationFallbackReason {
    none = 0,
    native_surface_unavailable,
    native_backend_unavailable,
    runtime_pipeline_unavailable,
};

enum class SdlDesktopPresentationBackendReportStatus {
    not_requested = 0,
    missing_request,
    native_surface_unavailable,
    native_backend_unavailable,
    runtime_pipeline_unavailable,
    ready,
};

struct SdlDesktopPresentationDiagnostic {
    SdlDesktopPresentationFallbackReason reason{SdlDesktopPresentationFallbackReason::none};
    std::string message;
};

struct SdlDesktopPresentationBackendReport {
    SdlDesktopPresentationBackend backend{SdlDesktopPresentationBackend::null_renderer};
    SdlDesktopPresentationBackendReportStatus status{SdlDesktopPresentationBackendReportStatus::not_requested};
    SdlDesktopPresentationFallbackReason fallback_reason{SdlDesktopPresentationFallbackReason::none};
    std::string message;
};

enum class SdlDesktopPresentationSceneGpuBindingStatus {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

struct SdlDesktopPresentationSceneGpuBindingDiagnostic {
    SdlDesktopPresentationSceneGpuBindingStatus status{SdlDesktopPresentationSceneGpuBindingStatus::not_requested};
    std::string message;
};

struct SdlDesktopPresentationSceneGpuBindingStats {
    std::size_t mesh_bindings{0};
    std::size_t skinned_mesh_bindings{0};
    std::size_t morph_mesh_bindings{0};
    std::size_t compute_morph_mesh_bindings{0};
    std::size_t compute_morph_mesh_dispatches{0};
    std::size_t compute_morph_queue_waits{0};
    std::size_t compute_morph_mesh_draws{0};
    std::uint64_t compute_morph_async_compute_queue_submits{0};
    std::uint64_t compute_morph_async_graphics_queue_submits{0};
    std::uint64_t compute_morph_async_graphics_queue_waits{0};
    std::uint64_t compute_morph_async_last_compute_submitted_fence_value{0};
    std::uint64_t compute_morph_async_last_graphics_queue_wait_fence_value{0};
    std::uint64_t compute_morph_async_last_graphics_submitted_fence_value{0};
    std::size_t compute_morph_skinned_mesh_bindings{0};
    std::size_t compute_morph_skinned_mesh_dispatches{0};
    std::size_t compute_morph_skinned_queue_waits{0};
    std::size_t compute_morph_skinned_mesh_draws{0};
    std::size_t material_bindings{0};
    std::size_t mesh_uploads{0};
    std::size_t skinned_mesh_uploads{0};
    std::size_t morph_mesh_uploads{0};
    std::size_t texture_uploads{0};
    std::size_t material_uploads{0};
    std::size_t material_pipeline_layouts{0};
    std::uint64_t uploaded_texture_bytes{0};
    std::uint64_t uploaded_mesh_bytes{0};
    std::uint64_t uploaded_morph_bytes{0};
    std::uint64_t uploaded_material_factor_bytes{0};
    std::size_t mesh_bindings_resolved{0};
    std::size_t skinned_mesh_bindings_resolved{0};
    std::size_t morph_mesh_bindings_resolved{0};
    std::size_t compute_morph_mesh_bindings_resolved{0};
    std::size_t compute_morph_skinned_mesh_bindings_resolved{0};
    std::size_t material_bindings_resolved{0};
    std::uint64_t compute_morph_output_position_bytes{0};
};

enum class SdlDesktopPresentationPostprocessStatus {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

struct SdlDesktopPresentationPostprocessDiagnostic {
    SdlDesktopPresentationPostprocessStatus status{SdlDesktopPresentationPostprocessStatus::not_requested};
    std::string message;
};

enum class SdlDesktopPresentationDirectionalShadowStatus {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

enum class SdlDesktopPresentationDirectionalShadowFilterMode {
    none = 0,
    fixed_pcf_3x3,
};

struct SdlDesktopPresentationDirectionalShadowDiagnostic {
    SdlDesktopPresentationDirectionalShadowStatus status{SdlDesktopPresentationDirectionalShadowStatus::not_requested};
    std::string message;
};

enum class SdlDesktopPresentationNativeUiOverlayStatus {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

struct SdlDesktopPresentationNativeUiOverlayDiagnostic {
    SdlDesktopPresentationNativeUiOverlayStatus status{SdlDesktopPresentationNativeUiOverlayStatus::not_requested};
    std::string message;
};

enum class SdlDesktopPresentationNativeUiTextureOverlayStatus {
    not_requested = 0,
    unavailable,
    invalid_request,
    failed,
    ready,
};

struct SdlDesktopPresentationNativeUiTextureOverlayDiagnostic {
    SdlDesktopPresentationNativeUiTextureOverlayStatus status{
        SdlDesktopPresentationNativeUiTextureOverlayStatus::not_requested};
    std::string message;
};

struct SdlDesktopPresentationReport {
    SdlDesktopPresentationBackend requested_backend{SdlDesktopPresentationBackend::null_renderer};
    SdlDesktopPresentationBackend selected_backend{SdlDesktopPresentationBackend::null_renderer};
    SdlDesktopPresentationFallbackReason fallback_reason{SdlDesktopPresentationFallbackReason::none};
    bool used_null_fallback{false};
    bool allow_null_fallback{true};
    SdlDesktopPresentationSceneGpuBindingStatus scene_gpu_status{
        SdlDesktopPresentationSceneGpuBindingStatus::not_requested};
    SdlDesktopPresentationSceneGpuBindingStats scene_gpu_stats;
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
    std::uint64_t native_ui_overlay_sprites_submitted{0};
    std::uint64_t native_ui_overlay_draws{0};
    SdlDesktopPresentationNativeUiTextureOverlayStatus native_ui_texture_overlay_status{
        SdlDesktopPresentationNativeUiTextureOverlayStatus::not_requested};
    bool native_ui_texture_overlay_requested{false};
    bool native_ui_texture_overlay_atlas_ready{false};
    std::uint64_t native_ui_texture_overlay_sprites_submitted{0};
    std::uint64_t native_ui_texture_overlay_texture_binds{0};
    std::uint64_t native_ui_texture_overlay_draws{0};
    std::size_t framegraph_passes{0};
    RendererStats renderer_stats;
    Extent2D backbuffer_extent;
    std::size_t diagnostics_count{0};
    std::size_t backend_reports_count{0};
    std::size_t scene_gpu_diagnostics_count{0};
    std::size_t postprocess_diagnostics_count{0};
    std::size_t directional_shadow_diagnostics_count{0};
    std::size_t native_ui_overlay_diagnostics_count{0};
    std::size_t native_ui_texture_overlay_diagnostics_count{0};
};

enum class SdlDesktopPresentationQualityGateStatus {
    not_requested = 0,
    blocked,
    ready,
};

struct SdlDesktopPresentationQualityGateDesc {
    bool require_scene_gpu_bindings{false};
    bool require_postprocess{false};
    bool require_postprocess_depth_input{false};
    bool require_directional_shadow{false};
    bool require_directional_shadow_filtering{false};
    std::uint64_t expected_frames{0};
};

struct SdlDesktopPresentationQualityGateReport {
    SdlDesktopPresentationQualityGateStatus status{SdlDesktopPresentationQualityGateStatus::not_requested};
    bool ready{false};
    bool scene_gpu_ready{false};
    bool postprocess_ready{false};
    bool postprocess_depth_input_ready{false};
    bool directional_shadow_ready{false};
    bool directional_shadow_filter_ready{false};
    std::uint32_t expected_framegraph_passes{0};
    bool framegraph_passes_current{false};
    bool framegraph_execution_budget_current{false};
    std::uint32_t diagnostics_count{0};
};

struct SdlDesktopPresentationShaderBytecode {
    std::string_view entry_point;
    std::span<const std::uint8_t> bytecode;
};

struct SdlDesktopPresentationSceneMorphMeshBinding {
    AssetId mesh;
    AssetId morph_mesh;
};

struct SdlDesktopPresentationD3d12RendererDesc {
    SdlDesktopPresentationShaderBytecode vertex_shader;
    SdlDesktopPresentationShaderBytecode fragment_shader;
    SdlDesktopPresentationShaderBytecode native_sprite_overlay_vertex_shader;
    SdlDesktopPresentationShaderBytecode native_sprite_overlay_fragment_shader;
    const runtime::RuntimeAssetPackage* native_sprite_overlay_package{nullptr};
    AssetId native_sprite_overlay_atlas_asset;
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    bool enable_native_sprite_overlay{false};
    bool enable_native_sprite_overlay_textures{false};
};

struct SdlDesktopPresentationVulkanRendererDesc {
    SdlDesktopPresentationShaderBytecode vertex_shader;
    SdlDesktopPresentationShaderBytecode fragment_shader;
    SdlDesktopPresentationShaderBytecode native_sprite_overlay_vertex_shader;
    SdlDesktopPresentationShaderBytecode native_sprite_overlay_fragment_shader;
    const runtime::RuntimeAssetPackage* native_sprite_overlay_package{nullptr};
    AssetId native_sprite_overlay_atlas_asset;
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    bool enable_native_sprite_overlay{false};
    bool enable_native_sprite_overlay_textures{false};
};

struct SdlDesktopPresentationD3d12SceneRendererDesc {
    SdlDesktopPresentationShaderBytecode vertex_shader;
    SdlDesktopPresentationShaderBytecode fragment_shader;
    /// Optional skinned mesh scene vertex shader bytecode (`vs_skinned` in `runtime_scene.hlsl`).
    SdlDesktopPresentationShaderBytecode skinned_vertex_shader;
    /// Optional morph mesh scene vertex shader bytecode (`vs_morph` in generated package shaders).
    SdlDesktopPresentationShaderBytecode morph_vertex_shader;
    /// Optional compute morph POSITION-only scene vertex shader bytecode (`vs_compute_morph` in generated package
    /// shaders).
    SdlDesktopPresentationShaderBytecode compute_morph_vertex_shader;
    /// Optional D3D12 compute shader bytecode (`cs_compute_morph_position`) that writes morphed POSITION bytes.
    SdlDesktopPresentationShaderBytecode compute_morph_shader;
    /// Optional D3D12 compute shader bytecode (`cs_compute_morph_skinned_position`) for skinned mesh POSITION output.
    SdlDesktopPresentationShaderBytecode compute_morph_skinned_shader;
    /// Optional directional shadow scene fragment bytecode (`ps_shadow_receiver`) compiled so shadow resources use the
    /// descriptor set after deformation-specific sets. Required when directional shadow smoke is true and the scene
    /// pass uses skinned or graphics morph descriptors at set index 1.
    SdlDesktopPresentationShaderBytecode skinned_scene_fragment_shader;
    SdlDesktopPresentationShaderBytecode postprocess_vertex_shader;
    SdlDesktopPresentationShaderBytecode postprocess_fragment_shader;
    SdlDesktopPresentationShaderBytecode shadow_vertex_shader;
    SdlDesktopPresentationShaderBytecode shadow_fragment_shader;
    SdlDesktopPresentationShaderBytecode native_ui_overlay_vertex_shader;
    SdlDesktopPresentationShaderBytecode native_ui_overlay_fragment_shader;
    const runtime::RuntimeAssetPackage* package{nullptr};
    const SceneRenderPacket* packet{nullptr};
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    std::vector<rhi::VertexBufferLayoutDesc> skinned_vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> skinned_vertex_attributes;
    std::vector<AssetId> morph_mesh_assets;
    std::vector<SdlDesktopPresentationSceneMorphMeshBinding> morph_mesh_bindings;
    std::vector<SdlDesktopPresentationSceneMorphMeshBinding> compute_morph_mesh_bindings;
    std::vector<SdlDesktopPresentationSceneMorphMeshBinding> compute_morph_skinned_mesh_bindings;
    bool enable_compute_morph_tangent_frame_output{false};
    bool enable_postprocess{false};
    bool enable_postprocess_depth_input{false};
    bool enable_directional_shadow_smoke{false};
    bool enable_native_ui_overlay{false};
    AssetId native_ui_overlay_atlas_asset;
    bool enable_native_ui_overlay_textures{false};
};

struct SdlDesktopPresentationVulkanSceneRendererDesc {
    SdlDesktopPresentationShaderBytecode vertex_shader;
    SdlDesktopPresentationShaderBytecode fragment_shader;
    SdlDesktopPresentationShaderBytecode skinned_vertex_shader;
    SdlDesktopPresentationShaderBytecode morph_vertex_shader;
    /// Optional Vulkan compute morph scene vertex SPIR-V (`vs_compute_morph` or `vs_compute_morph_tangent_frame` in
    /// generated package shaders).
    SdlDesktopPresentationShaderBytecode compute_morph_vertex_shader;
    /// Optional Vulkan compute shader SPIR-V (`cs_compute_morph_position` or `cs_compute_morph_tangent_frame`) that
    /// writes morphed vertex stream bytes when compute morph bindings are selected. When provided without compute morph
    /// bindings, the runtime host may use it only as the Vulkan IRhiDevice mapping compute-dispatch proof.
    SdlDesktopPresentationShaderBytecode compute_morph_shader;
    /// Optional Vulkan compute shader SPIR-V (`cs_compute_morph_skinned_position`) for skinned mesh POSITION output.
    SdlDesktopPresentationShaderBytecode compute_morph_skinned_shader;
    /// See `SdlDesktopPresentationD3d12SceneRendererDesc::skinned_scene_fragment_shader`.
    SdlDesktopPresentationShaderBytecode skinned_scene_fragment_shader;
    SdlDesktopPresentationShaderBytecode postprocess_vertex_shader;
    SdlDesktopPresentationShaderBytecode postprocess_fragment_shader;
    SdlDesktopPresentationShaderBytecode shadow_vertex_shader;
    SdlDesktopPresentationShaderBytecode shadow_fragment_shader;
    SdlDesktopPresentationShaderBytecode native_ui_overlay_vertex_shader;
    SdlDesktopPresentationShaderBytecode native_ui_overlay_fragment_shader;
    const runtime::RuntimeAssetPackage* package{nullptr};
    const SceneRenderPacket* packet{nullptr};
    rhi::PrimitiveTopology topology{rhi::PrimitiveTopology::triangle_list};
    std::vector<rhi::VertexBufferLayoutDesc> vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> vertex_attributes;
    std::vector<rhi::VertexBufferLayoutDesc> skinned_vertex_buffers;
    std::vector<rhi::VertexAttributeDesc> skinned_vertex_attributes;
    std::vector<AssetId> morph_mesh_assets;
    std::vector<SdlDesktopPresentationSceneMorphMeshBinding> morph_mesh_bindings;
    std::vector<SdlDesktopPresentationSceneMorphMeshBinding> compute_morph_mesh_bindings;
    std::vector<SdlDesktopPresentationSceneMorphMeshBinding> compute_morph_skinned_mesh_bindings;
    bool enable_compute_morph_tangent_frame_output{false};
    bool enable_postprocess{false};
    bool enable_postprocess_depth_input{false};
    bool enable_directional_shadow_smoke{false};
    bool enable_native_ui_overlay{false};
    AssetId native_ui_overlay_atlas_asset;
    bool enable_native_ui_overlay_textures{false};
};

struct SdlDesktopPresentationDesc {
    SdlWindow* window{nullptr};
    Extent2D extent;
    bool prefer_d3d12{true};
    bool prefer_vulkan{false};
    bool allow_null_fallback{true};
    bool prefer_warp{false};
    bool enable_debug_layer{false};
    bool vsync{true};
    const SdlDesktopPresentationD3d12RendererDesc* d3d12_renderer{nullptr};
    const SdlDesktopPresentationVulkanRendererDesc* vulkan_renderer{nullptr};
    const SdlDesktopPresentationD3d12SceneRendererDesc* d3d12_scene_renderer{nullptr};
    const SdlDesktopPresentationVulkanSceneRendererDesc* vulkan_scene_renderer{nullptr};
};

class SdlDesktopPresentation final {
  public:
    explicit SdlDesktopPresentation(const SdlDesktopPresentationDesc& desc);
    ~SdlDesktopPresentation();

    SdlDesktopPresentation(const SdlDesktopPresentation&) = delete;
    SdlDesktopPresentation& operator=(const SdlDesktopPresentation&) = delete;
    SdlDesktopPresentation(SdlDesktopPresentation&&) = delete;
    SdlDesktopPresentation& operator=(SdlDesktopPresentation&&) = delete;

    [[nodiscard]] IRenderer& renderer() noexcept;
    [[nodiscard]] const IRenderer& renderer() const noexcept;
    [[nodiscard]] SdlDesktopPresentationBackend backend() const noexcept;
    [[nodiscard]] std::string_view backend_name() const noexcept;
    [[nodiscard]] SdlDesktopPresentationReport report() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationBackendReport> backend_reports() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationDiagnostic> diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationSceneGpuBindingStatus scene_gpu_binding_status() const noexcept;
    [[nodiscard]] bool scene_gpu_bindings_ready() const noexcept;
    [[nodiscard]] SdlDesktopPresentationSceneGpuBindingStats scene_gpu_binding_stats() const noexcept;
    [[nodiscard]] rhi::BufferHandle scene_pbr_frame_uniform_buffer() const noexcept;
    [[nodiscard]] rhi::IRhiDevice* scene_pbr_frame_rhi_device() noexcept;
    [[nodiscard]] const rhi::IRhiDevice* scene_pbr_frame_rhi_device() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationSceneGpuBindingDiagnostic>
    scene_gpu_binding_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationPostprocessStatus postprocess_status() const noexcept;
    [[nodiscard]] bool postprocess_ready() const noexcept;
    [[nodiscard]] bool postprocess_depth_input_ready() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationPostprocessDiagnostic> postprocess_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationDirectionalShadowStatus directional_shadow_status() const noexcept;
    [[nodiscard]] bool directional_shadow_ready() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationDirectionalShadowDiagnostic>
    directional_shadow_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationNativeUiOverlayStatus native_ui_overlay_status() const noexcept;
    [[nodiscard]] bool native_ui_overlay_ready() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationNativeUiOverlayDiagnostic>
    native_ui_overlay_diagnostics() const noexcept;
    [[nodiscard]] SdlDesktopPresentationNativeUiTextureOverlayStatus native_ui_texture_overlay_status() const noexcept;
    [[nodiscard]] bool native_ui_texture_overlay_atlas_ready() const noexcept;
    [[nodiscard]] std::span<const SdlDesktopPresentationNativeUiTextureOverlayDiagnostic>
    native_ui_texture_overlay_diagnostics() const noexcept;

  private:
    struct Impl;

    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] std::string_view sdl_desktop_presentation_backend_name(SdlDesktopPresentationBackend backend) noexcept;
[[nodiscard]] std::string_view
sdl_desktop_presentation_backend_report_status_name(SdlDesktopPresentationBackendReportStatus status) noexcept;
[[nodiscard]] std::string_view
sdl_desktop_presentation_fallback_reason_name(SdlDesktopPresentationFallbackReason reason) noexcept;
[[nodiscard]] std::string_view
sdl_desktop_presentation_scene_gpu_binding_status_name(SdlDesktopPresentationSceneGpuBindingStatus status) noexcept;
[[nodiscard]] std::string_view
sdl_desktop_presentation_postprocess_status_name(SdlDesktopPresentationPostprocessStatus status) noexcept;
[[nodiscard]] std::string_view
sdl_desktop_presentation_directional_shadow_status_name(SdlDesktopPresentationDirectionalShadowStatus status) noexcept;
[[nodiscard]] std::string_view sdl_desktop_presentation_directional_shadow_filter_mode_name(
    SdlDesktopPresentationDirectionalShadowFilterMode mode) noexcept;
[[nodiscard]] std::string_view
sdl_desktop_presentation_native_ui_overlay_status_name(SdlDesktopPresentationNativeUiOverlayStatus status) noexcept;
[[nodiscard]] std::string_view sdl_desktop_presentation_native_ui_texture_overlay_status_name(
    SdlDesktopPresentationNativeUiTextureOverlayStatus status) noexcept;
[[nodiscard]] std::string_view
sdl_desktop_presentation_quality_gate_status_name(SdlDesktopPresentationQualityGateStatus status) noexcept;
[[nodiscard]] SdlDesktopPresentationQualityGateReport
evaluate_sdl_desktop_presentation_quality_gate(const SdlDesktopPresentationReport& report,
                                               const SdlDesktopPresentationQualityGateDesc& desc) noexcept;

} // namespace mirakana
