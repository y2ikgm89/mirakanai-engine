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
[[nodiscard]] UiRenderSubmitResult submit_ui_renderer_submission(IRenderer& renderer,
                                                                 const ui::RendererSubmission& submission,
                                                                 UiRenderSubmitDesc desc = {});

} // namespace mirakana
