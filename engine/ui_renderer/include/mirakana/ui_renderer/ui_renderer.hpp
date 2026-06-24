// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct UiThemeColor {
    std::string token;
    Color color;
};

class UiRendererTheme {
  public:
    bool try_add(UiThemeColor color);
    void add(UiThemeColor color);

    [[nodiscard]] const Color* find(std::string_view token) const noexcept;
    [[nodiscard]] std::size_t count() const noexcept;

  private:
    std::vector<UiThemeColor> colors_;
};

struct UiRendererImageBinding {
    std::string resource_id;
    std::string asset_uri;
    Color color;
    AssetId atlas_page;
    SpriteUvRect uv_rect;
};

class UiRendererImagePalette {
  public:
    bool try_add(UiRendererImageBinding binding);
    void add(UiRendererImageBinding binding);

    [[nodiscard]] const UiRendererImageBinding* find_by_resource_id(std::string_view resource_id) const noexcept;
    [[nodiscard]] const UiRendererImageBinding* find_by_asset_uri(std::string_view asset_uri) const noexcept;
    [[nodiscard]] std::size_t count() const noexcept;

  private:
    std::vector<UiRendererImageBinding> bindings_;
};

struct UiRendererGlyphAtlasBinding {
    std::string font_family;
    std::uint32_t glyph{0};
    Color color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F};
    AssetId atlas_page;
    SpriteUvRect uv_rect;
};

class UiRendererGlyphAtlasPalette {
  public:
    bool try_add(UiRendererGlyphAtlasBinding binding);
    void add(UiRendererGlyphAtlasBinding binding);

    [[nodiscard]] const UiRendererGlyphAtlasBinding* find(std::string_view font_family,
                                                          std::uint32_t glyph) const noexcept;
    [[nodiscard]] std::size_t count() const noexcept;

  private:
    std::vector<UiRendererGlyphAtlasBinding> bindings_;
};

struct UiRendererImagePaletteBuildFailure {
    AssetId asset;
    std::string diagnostic;
};

struct UiRendererImagePaletteBuildResult {
    UiRendererImagePalette palette;
    std::vector<AssetId> atlas_page_assets;
    std::vector<UiRendererImagePaletteBuildFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct UiRendererGlyphAtlasPaletteBuildFailure {
    AssetId asset;
    std::string diagnostic;
};

struct UiRendererGlyphAtlasPaletteBuildResult {
    UiRendererGlyphAtlasPalette palette;
    std::vector<AssetId> atlas_page_assets;
    std::vector<UiRendererGlyphAtlasPaletteBuildFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct UiRenderSubmitDesc {
    const UiRendererTheme* theme{nullptr};
    Color fallback_box_color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F};
    const UiRendererImagePalette* image_palette{nullptr};
    const UiRendererGlyphAtlasPalette* glyph_atlas{nullptr};
    ui::MonospaceTextLayoutPolicy text_layout_policy{};
};

struct UiRenderSubmitResult {
    std::size_t boxes_submitted{0};
    std::size_t text_runs_available{0};
    std::size_t text_adapter_rows_available{0};
    std::size_t text_glyphs_available{0};
    std::size_t text_glyph_sprites_submitted{0};
    std::size_t text_glyphs_resolved{0};
    std::size_t text_glyphs_missing{0};
    std::size_t image_placeholders_available{0};
    std::size_t image_sprites_submitted{0};
    std::size_t image_resources_resolved{0};
    std::size_t image_resources_missing{0};
    std::size_t accessibility_nodes_available{0};
    std::size_t adapter_diagnostics_available{0};
    std::size_t theme_colors_resolved{0};
};

enum class UiRendererAtlasHandoffStatus : std::uint8_t {
    ready,
    invalid_request,
};

enum class UiRendererAtlasHandoffDiagnosticCode : std::uint8_t {
    missing_request_id,
    missing_image_atlas_metadata,
    missing_glyph_atlas_metadata,
    missing_atlas_placement_rows,
    missing_atlas_budget_rows,
    atlas_budget_exceeded,
    missing_atlas_eviction_diagnostics,
    missing_texture_upload_handoff,
    missing_texture_upload_execution,
    missing_renderer_submission_counters,
    missing_selected_package_counter_evidence,
    unresolved_text_glyphs,
    unresolved_image_resources,
    renderer_submission_counter_mismatch,
    unsupported_renderer_texture_upload_api,
    unsupported_public_native_handle,
    unsupported_broad_text_rendering_claim,
    unsupported_broad_image_rendering_claim,
    side_effect_invocation,
};

struct UiRendererAtlasHandoffDiagnostic {
    UiRendererAtlasHandoffDiagnosticCode code{UiRendererAtlasHandoffDiagnosticCode::missing_request_id};
    std::string message;
};

struct UiRendererAtlasHandoffRequest {
    std::string id;
    std::size_t image_atlas_page_count{0};
    std::size_t image_atlas_binding_count{0};
    std::size_t glyph_atlas_page_count{0};
    std::size_t glyph_atlas_binding_count{0};
    UiRenderSubmitResult submit_result;
    std::size_t renderer_sprites_submitted{0};
    std::size_t max_image_bindings{0};
    std::size_t max_glyph_bindings{0};
    bool image_atlas_metadata_built{false};
    bool glyph_atlas_metadata_built{false};
    bool atlas_eviction_diagnostics_reviewed{false};
    bool texture_upload_handoff_reviewed{false};
    bool texture_upload_execution_reviewed{false};
    bool texture_upload_execution_ready{false};
    bool renderer_submission_counters_reviewed{false};
    bool selected_package_counter_evidence{false};
    bool requested_renderer_texture_upload_api{false};
    bool requested_public_native_handle{false};
    bool claims_broad_text_rendering{false};
    bool claims_broad_image_rendering{false};
    bool invoked_source_image_decode{false};
    bool invoked_live_glyph_atlas_generation{false};
    bool invoked_renderer_upload{false};
    std::uint64_t seed{0};
};

struct UiRendererAtlasHandoffPlan {
    UiRendererAtlasHandoffStatus status{UiRendererAtlasHandoffStatus::invalid_request};
    bool selected_package_evidence_ready{false};
    bool reviewed{false};
    std::size_t image_atlas_pages{0};
    std::size_t image_atlas_bindings{0};
    std::size_t glyph_atlas_pages{0};
    std::size_t glyph_atlas_bindings{0};
    std::size_t atlas_placement_rows{0};
    std::size_t atlas_budget_rows{0};
    std::size_t atlas_eviction_diagnostic_rows{0};
    std::size_t texture_upload_handoff_rows{0};
    std::size_t texture_upload_execution_rows{0};
    bool texture_upload_execution_ready{false};
    std::size_t renderer_submission_counter_rows{0};
    std::size_t text_glyphs_available{0};
    std::size_t text_glyphs_resolved{0};
    std::size_t text_glyphs_missing{0};
    std::size_t text_glyph_sprites_submitted{0};
    std::size_t image_placeholders_available{0};
    std::size_t image_resources_resolved{0};
    std::size_t image_resources_missing{0};
    std::size_t image_sprites_submitted{0};
    std::size_t renderer_sprites_submitted{0};
    std::size_t unsupported_claim_rows{0};
    std::size_t side_effect_rows{0};
    bool requested_renderer_texture_upload_api{false};
    bool requested_public_native_handle{false};
    bool invoked_source_image_decode{false};
    bool invoked_live_glyph_atlas_generation{false};
    bool invoked_renderer_upload{false};
    std::vector<UiRendererAtlasHandoffDiagnostic> diagnostics;
    std::uint64_t replay_hash{0};

    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] Color resolve_ui_box_color(const ui::RendererBox& box, const UiRenderSubmitDesc& desc) noexcept;
// Resolves image placeholders deterministically: resource_id is authoritative,
// then asset_uri is used as a fallback when no resource binding exists.
[[nodiscard]] const Color* resolve_ui_image_color(const ui::ImageAdapterRow& row,
                                                  const UiRenderSubmitDesc& desc) noexcept;
[[nodiscard]] const UiRendererImageBinding* resolve_ui_image_binding(const ui::ImageAdapterRow& row,
                                                                     const UiRenderSubmitDesc& desc) noexcept;
[[nodiscard]] const UiRendererGlyphAtlasBinding*
resolve_ui_text_glyph_binding(const ui::TextAdapterRow& row, const ui::TextAdapterGlyphPlaceholder& glyph,
                              const UiRenderSubmitDesc& desc) noexcept;
[[nodiscard]] SpriteCommand make_ui_box_sprite_command(const ui::RendererBox& box,
                                                       const UiRenderSubmitDesc& desc = {}) noexcept;
// Builds a colored placeholder sprite from the row bounds; this is not image texture rendering.
[[nodiscard]] SpriteCommand make_ui_image_sprite_command(const ui::ImageAdapterRow& row, Color color) noexcept;
[[nodiscard]] SpriteCommand make_ui_image_sprite_command(const ui::ImageAdapterRow& row,
                                                         const UiRendererImageBinding& binding) noexcept;
[[nodiscard]] SpriteCommand make_ui_text_glyph_sprite_command(const ui::TextAdapterGlyphPlaceholder& glyph,
                                                              const UiRendererGlyphAtlasBinding& binding) noexcept;
[[nodiscard]] UiRendererImagePaletteBuildResult
build_ui_renderer_image_palette_from_runtime_ui_atlas(const runtime::RuntimeAssetPackage& package,
                                                      AssetId atlas_metadata_asset);
[[nodiscard]] UiRendererGlyphAtlasPaletteBuildResult
build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas(const runtime::RuntimeAssetPackage& package,
                                                            AssetId atlas_metadata_asset);
[[nodiscard]] UiRendererAtlasHandoffPlan review_ui_renderer_atlas_handoff(const UiRendererAtlasHandoffRequest& request);
[[nodiscard]] UiRenderSubmitResult submit_ui_renderer_submission(IRenderer& renderer,
                                                                 const ui::RendererSubmission& submission,
                                                                 UiRenderSubmitDesc desc = {});

} // namespace mirakana
