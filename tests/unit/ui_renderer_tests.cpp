// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

class CaptureRenderer final : public mirakana::IRenderer {
  public:
    explicit CaptureRenderer(mirakana::Extent2D extent) : extent_(extent) {}

    [[nodiscard]] std::string_view backend_name() const noexcept override {
        return "capture";
    }

    [[nodiscard]] mirakana::Extent2D backbuffer_extent() const noexcept override {
        return extent_;
    }

    [[nodiscard]] mirakana::RendererStats stats() const noexcept override {
        return stats_;
    }

    void resize(mirakana::Extent2D extent) override {
        extent_ = extent;
    }

    void set_clear_color(mirakana::Color color) noexcept override {
        clear_color_ = color;
    }

    void begin_frame() override {
        frame_active_ = true;
    }

    void draw_sprite(const mirakana::SpriteCommand& command) override {
        sprites.push_back(command);
        ++stats_.sprites_submitted;
    }

    void draw_mesh(const mirakana::MeshCommand& /*command*/) override {
        ++stats_.meshes_submitted;
    }

    void end_frame() override {
        frame_active_ = false;
    }

    std::vector<mirakana::SpriteCommand> sprites;

  private:
    mirakana::Extent2D extent_;
    mirakana::Color clear_color_{.r = 0.0F, .g = 0.0F, .b = 0.0F, .a = 1.0F};
    mirakana::RendererStats stats_{};
    bool frame_active_{false};
};

class CapturingAccessibilityAdapter final : public mirakana::ui::IAccessibilityAdapter {
  public:
    void publish_nodes(const std::vector<mirakana::ui::AccessibilityNode>& nodes) override {
        published_nodes = nodes;
        ++publish_calls;
    }

    std::vector<mirakana::ui::AccessibilityNode> published_nodes;
    std::size_t publish_calls{0};
};

class CapturingImeAdapter final : public mirakana::ui::IImeAdapter {
  public:
    void update_composition(const mirakana::ui::ImeComposition& composition) override {
        published_composition = composition;
        ++publish_calls;
    }

    mirakana::ui::ImeComposition published_composition;
    std::size_t publish_calls{0};
};

class CapturingPlatformIntegrationAdapter final : public mirakana::ui::IPlatformIntegrationAdapter {
  public:
    void begin_text_input(const mirakana::ui::PlatformTextInputRequest& request) override {
        published_begin_request = request;
        ++begin_calls;
    }

    void end_text_input(const mirakana::ui::ElementId& target) override {
        published_end_target = target;
        ++end_calls;
    }

    mirakana::ui::PlatformTextInputRequest published_begin_request;
    mirakana::ui::ElementId published_end_target;
    std::size_t begin_calls{0};
    std::size_t end_calls{0};
};

class CapturingClipboardTextAdapter final : public mirakana::ui::IClipboardTextAdapter {
  public:
    void set_clipboard_text(std::string_view value) override {
        text = std::string{value};
        ++write_calls;
    }

    [[nodiscard]] bool has_clipboard_text() const override {
        ++has_calls;
        return use_forced_has_text ? forced_has_text : !text.empty();
    }

    [[nodiscard]] std::string clipboard_text() const override {
        ++read_calls;
        return text;
    }

    std::string text;
    bool use_forced_has_text{false};
    bool forced_has_text{false};
    std::size_t write_calls{0};
    mutable std::size_t has_calls{0};
    mutable std::size_t read_calls{0};
};

class CapturingFontRasterizerAdapter final : public mirakana::ui::IFontRasterizerAdapter {
  public:
    [[nodiscard]] mirakana::ui::GlyphAtlasAllocation
    rasterize_glyph(const mirakana::ui::FontRasterizationRequest& request) override {
        published_request = request;
        ++publish_calls;
        return mirakana::ui::GlyphAtlasAllocation{
            .glyph = request.glyph,
            .atlas_bounds =
                mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = request.pixel_size, .height = request.pixel_size},
        };
    }

    mirakana::ui::FontRasterizationRequest published_request;
    std::size_t publish_calls{0};
};

class InvalidFontRasterizerAdapter final : public mirakana::ui::IFontRasterizerAdapter {
  public:
    [[nodiscard]] mirakana::ui::GlyphAtlasAllocation
    rasterize_glyph(const mirakana::ui::FontRasterizationRequest& request) override {
        published_request = request;
        ++publish_calls;
        return mirakana::ui::GlyphAtlasAllocation{
            .glyph = 0,
            .atlas_bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 0.0F},
        };
    }

    mirakana::ui::FontRasterizationRequest published_request;
    std::size_t publish_calls{0};
};

class CapturingImageDecodingAdapter final : public mirakana::ui::IImageDecodingAdapter {
  public:
    [[nodiscard]] std::optional<mirakana::ui::ImageDecodeResult>
    decode_image(const mirakana::ui::ImageDecodeRequest& request) override {
        published_request = request;
        ++publish_calls;
        return response;
    }

    std::optional<mirakana::ui::ImageDecodeResult> response;
    mirakana::ui::ImageDecodeRequest published_request;
    std::size_t publish_calls{0};
};

class CapturingTextShapingAdapter final : public mirakana::ui::ITextShapingAdapter {
  public:
    [[nodiscard]] std::vector<mirakana::ui::TextLayoutRun>
    shape_text(const mirakana::ui::TextLayoutRequest& request) override {
        published_request = request;
        ++publish_calls;
        return response;
    }

    std::vector<mirakana::ui::TextLayoutRun> response;
    mirakana::ui::TextLayoutRequest published_request;
    std::size_t publish_calls{0};
};

[[nodiscard]] std::vector<std::byte> byte_payload(std::size_t count, unsigned char value = 0xffU) {
    return std::vector<std::byte>(count, static_cast<std::byte>(value));
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord
make_runtime_record(mirakana::runtime::RuntimeAssetHandle handle, mirakana::AssetId asset, mirakana::AssetKind kind,
                    std::string path, std::string content, std::vector<mirakana::AssetId> dependencies = {}) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = std::move(path),
        .content_hash = mirakana::hash_asset_cooked_content(content),
        .source_revision = 1,
        .dependencies = std::move(dependencies),
        .content = std::move(content),
    };
}

[[nodiscard]] std::string cooked_texture_payload(mirakana::AssetId asset) {
    return "format=GameEngine.CookedTexture.v1\n"
           "asset.id=" +
           std::to_string(asset.value) +
           "\n"
           "asset.kind=texture\n"
           "source.path=source/ui/hud-atlas.texture\n"
           "texture.width=1\n"
           "texture.height=1\n"
           "texture.pixel_format=rgba8_unorm\n"
           "texture.source_bytes=4\n"
           "texture.data_hex=ffffffff\n";
}

[[nodiscard]] std::string cooked_material_payload(mirakana::AssetId asset) {
    return "format=GameEngine.Material.v1\n"
           "material.id=" +
           std::to_string(asset.value) +
           "\n"
           "material.name=NotTexture\n"
           "material.shading=unlit\n"
           "material.surface=opaque\n";
}

[[nodiscard]] std::string cooked_ui_atlas_payload(mirakana::AssetId atlas_asset, mirakana::AssetId page_asset) {
    return "format=GameEngine.UiAtlas.v1\n"
           "asset.id=" +
           std::to_string(atlas_asset.value) +
           "\n"
           "asset.kind=ui_atlas\n"
           "source.decoding=unsupported\n"
           "atlas.packing=unsupported\n"
           "page.count=1\n"
           "page.0.asset=" +
           std::to_string(page_asset.value) +
           "\n"
           "page.0.asset_uri=runtime/assets/ui/hud-atlas.texture.geasset\n"
           "image.count=1\n"
           "image.0.resource_id=hud.icon\n"
           "image.0.asset_uri=runtime/assets/ui/hud-atlas.texture.geasset\n"
           "image.0.page=" +
           std::to_string(page_asset.value) +
           "\n"
           "image.0.u0=0.25\n"
           "image.0.v0=0.5\n"
           "image.0.u1=0.75\n"
           "image.0.v1=1\n"
           "image.0.color=1,1,1,1\n"
           "glyph.count=0\n";
}

[[nodiscard]] std::string cooked_ui_glyph_atlas_payload(mirakana::AssetId atlas_asset, mirakana::AssetId page_asset) {
    return "format=GameEngine.UiAtlas.v1\n"
           "asset.id=" +
           std::to_string(atlas_asset.value) +
           "\n"
           "asset.kind=ui_atlas\n"
           "source.decoding=rasterized-glyph-adapter\n"
           "atlas.packing=deterministic-glyph-atlas-rgba8-max-side\n"
           "page.count=1\n"
           "page.0.asset=" +
           std::to_string(page_asset.value) +
           "\n"
           "page.0.asset_uri=runtime/assets/ui/body_glyphs.texture.geasset\n"
           "image.count=0\n"
           "glyph.count=2\n"
           "glyph.0.font_family=ui/body\n"
           "glyph.0.glyph=65\n"
           "glyph.0.page=" +
           std::to_string(page_asset.value) +
           "\n"
           "glyph.0.u0=0\n"
           "glyph.0.v0=0\n"
           "glyph.0.u1=0.5\n"
           "glyph.0.v1=1\n"
           "glyph.0.color=1,1,1,1\n"
           "glyph.1.font_family=ui/body\n"
           "glyph.1.glyph=66\n"
           "glyph.1.page=" +
           std::to_string(page_asset.value) +
           "\n"
           "glyph.1.u0=0.5\n"
           "glyph.1.v0=0\n"
           "glyph.1.u1=1\n"
           "glyph.1.v1=1\n"
           "glyph.1.color=0.9,0.9,0.9,1\n";
}

} // namespace

MK_TEST("ui renderer converts styled ui boxes into sprite commands") {
    mirakana::ui::RendererBox box;
    box.id = mirakana::ui::ElementId{"start"};
    box.role = mirakana::ui::SemanticRole::button;
    box.bounds = mirakana::ui::Rect{.x = 4.0F, .y = 8.0F, .width = 120.0F, .height = 32.0F};
    box.background_token = "button.primary";

    mirakana::UiRendererTheme theme;
    MK_REQUIRE(theme.try_add(mirakana::UiThemeColor{"button.primary", mirakana::Color{0.2F, 0.4F, 0.8F, 1.0F}}));

    const auto command = mirakana::make_ui_box_sprite_command(
        box, mirakana::UiRenderSubmitDesc{
                 .theme = &theme, .fallback_box_color = mirakana::Color{.r = 1.0F, .g = 0.0F, .b = 1.0F, .a = 1.0F}});

    MK_REQUIRE(command.transform.position == (mirakana::Vec2{64.0F, 24.0F}));
    MK_REQUIRE(command.transform.scale == (mirakana::Vec2{120.0F, 32.0F}));
    MK_REQUIRE(command.color.r == 0.2F);
    MK_REQUIRE(command.color.g == 0.4F);
    MK_REQUIRE(command.color.b == 0.8F);
    MK_REQUIRE(command.color.a == 1.0F);
}

MK_TEST("ui renderer submits only styled boxes while reporting ui payload counts") {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.style.layout = mirakana::ui::LayoutMode::column;
    root.style.padding = mirakana::ui::EdgeInsets{.top = 4.0F, .right = 4.0F, .bottom = 4.0F, .left = 4.0F};
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc button;
    button.id = mirakana::ui::ElementId{"start"};
    button.parent = root.id;
    button.role = mirakana::ui::SemanticRole::button;
    button.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 120.0F, .height = 32.0F};
    button.text.label = "Start";
    button.accessibility_label = "Start game";
    button.style.background_token = "button.primary";
    MK_REQUIRE(document.try_add_element(button));

    const auto layout = mirakana::ui::solve_layout(
        document, root.id, mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 240.0F, .height = 120.0F});
    const auto submission = mirakana::ui::build_renderer_submission(document, layout);

    mirakana::UiRendererTheme theme;
    MK_REQUIRE(theme.try_add(mirakana::UiThemeColor{"button.primary", mirakana::Color{0.1F, 0.3F, 0.5F, 1.0F}}));

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 240, .height = 120});
    renderer.begin_frame();
    const auto result =
        mirakana::submit_ui_renderer_submission(renderer, submission, mirakana::UiRenderSubmitDesc{.theme = &theme});
    renderer.end_frame();

    const auto stats = renderer.stats();
    MK_REQUIRE(result.boxes_submitted == 1);
    MK_REQUIRE(result.text_runs_available == 1);
    MK_REQUIRE(result.accessibility_nodes_available == 1);
    MK_REQUIRE(result.theme_colors_resolved == 1);
    MK_REQUIRE(stats.sprites_submitted == 1);
}

MK_TEST("ui renderer submits resolved image placeholders through image sprite palette") {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.style.layout = mirakana::ui::LayoutMode::column;
    root.style.padding = mirakana::ui::EdgeInsets{.top = 8.0F, .right = 8.0F, .bottom = 8.0F, .left = 8.0F};
    root.style.gap = 4.0F;
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc panel;
    panel.id = mirakana::ui::ElementId{"panel"};
    panel.parent = root.id;
    panel.role = mirakana::ui::SemanticRole::panel;
    panel.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 96.0F, .height = 48.0F};
    panel.style.background_token = "panel.background";
    MK_REQUIRE(document.try_add_element(panel));

    mirakana::ui::ElementDesc portrait;
    portrait.id = mirakana::ui::ElementId{"portrait"};
    portrait.parent = root.id;
    portrait.role = mirakana::ui::SemanticRole::image;
    portrait.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 32.0F};
    portrait.image.resource_id = "ui/portrait";
    portrait.image.asset_uri = "assets/ui/portrait.texture";
    portrait.image.tint_token = "image.tint";
    MK_REQUIRE(document.try_add_element(portrait));

    const auto layout = mirakana::ui::solve_layout(
        document, root.id, mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 128.0F, .height = 128.0F});
    const auto submission = mirakana::ui::build_renderer_submission(document, layout);

    mirakana::UiRendererTheme theme;
    MK_REQUIRE(theme.try_add(mirakana::UiThemeColor{"panel.background", mirakana::Color{0.05F, 0.05F, 0.05F, 1.0F}}));

    mirakana::UiRendererImagePalette image_palette;
    MK_REQUIRE(image_palette.try_add(mirakana::UiRendererImageBinding{"ui/portrait", "assets/ui/portrait.texture",
                                                                      mirakana::Color{0.8F, 0.6F, 0.4F, 1.0F}}));

    mirakana::UiRenderSubmitDesc desc;
    desc.theme = &theme;
    desc.image_palette = &image_palette;

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 128, .height = 128});
    renderer.begin_frame();
    const auto result = mirakana::submit_ui_renderer_submission(renderer, submission, desc);
    renderer.end_frame();

    const auto stats = renderer.stats();
    MK_REQUIRE(result.boxes_submitted == 1);
    MK_REQUIRE(result.image_placeholders_available == 1);
    MK_REQUIRE(result.image_sprites_submitted == 1);
    MK_REQUIRE(result.image_resources_resolved == 1);
    MK_REQUIRE(result.image_resources_missing == 0);
    MK_REQUIRE(stats.sprites_submitted == 2);
}

MK_TEST("ui renderer submits monospace text glyphs through glyph atlas palette") {
    const auto atlas_page = mirakana::AssetId::from_name("ui/font-atlas/body");

    mirakana::ui::RendererSubmission submission;
    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"title"};
    text.bounds = mirakana::ui::Rect{.x = 4.0F, .y = 6.0F, .width = 64.0F, .height = 16.0F};
    text.text.label = "Play";
    text.text.font_family = "ui/body";
    text.text.wrap = mirakana::ui::TextWrapMode::clip;
    submission.text_runs.push_back(text);

    mirakana::UiRendererGlyphAtlasPalette glyph_atlas;
    MK_REQUIRE(glyph_atlas.try_add(mirakana::UiRendererGlyphAtlasBinding{
        "ui/body", static_cast<std::uint32_t>('P'), mirakana::Color{1.0F, 1.0F, 1.0F, 1.0F}, atlas_page,
        mirakana::SpriteUvRect{0.0F, 0.0F, 0.25F, 0.25F}}));
    MK_REQUIRE(glyph_atlas.try_add(mirakana::UiRendererGlyphAtlasBinding{
        "ui/body", static_cast<std::uint32_t>('l'), mirakana::Color{1.0F, 1.0F, 1.0F, 1.0F}, atlas_page,
        mirakana::SpriteUvRect{0.25F, 0.0F, 0.5F, 0.25F}}));
    MK_REQUIRE(glyph_atlas.try_add(mirakana::UiRendererGlyphAtlasBinding{
        "ui/body", static_cast<std::uint32_t>('a'), mirakana::Color{1.0F, 1.0F, 1.0F, 1.0F}, atlas_page,
        mirakana::SpriteUvRect{0.5F, 0.0F, 0.75F, 0.25F}}));
    MK_REQUIRE(glyph_atlas.try_add(mirakana::UiRendererGlyphAtlasBinding{
        "ui/body", static_cast<std::uint32_t>('y'), mirakana::Color{1.0F, 1.0F, 1.0F, 1.0F}, atlas_page,
        mirakana::SpriteUvRect{0.75F, 0.0F, 1.0F, 0.25F}}));

    mirakana::UiRenderSubmitDesc desc;
    desc.glyph_atlas = &glyph_atlas;
    desc.text_layout_policy = mirakana::ui::MonospaceTextLayoutPolicy{
        .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F};

    CaptureRenderer renderer(mirakana::Extent2D{.width = 96, .height = 48});
    renderer.begin_frame();
    const auto result = mirakana::submit_ui_renderer_submission(renderer, submission, desc);
    renderer.end_frame();

    MK_REQUIRE(result.text_glyphs_available == 4);
    MK_REQUIRE(result.text_glyphs_resolved == 4);
    MK_REQUIRE(result.text_glyphs_missing == 0);
    MK_REQUIRE(result.text_glyph_sprites_submitted == 4);
    MK_REQUIRE(renderer.sprites.size() == 4);
    MK_REQUIRE(renderer.sprites[0].texture.enabled);
    MK_REQUIRE(renderer.sprites[0].texture.atlas_page == atlas_page);
    MK_REQUIRE(renderer.sprites[0].texture.uv_rect.u1 == 0.25F);
    MK_REQUIRE(renderer.sprites[0].transform.position == (mirakana::Vec2{8.0F, 11.0F}));
    MK_REQUIRE(renderer.stats().sprites_submitted == 4);
}

MK_TEST("ui renderer reports missing glyph atlas bindings without fake sprites") {
    mirakana::ui::RendererSubmission submission;
    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"title"};
    text.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 12.0F};
    text.text.label = "A";
    text.text.font_family = "ui/body";
    submission.text_runs.push_back(text);

    mirakana::UiRendererGlyphAtlasPalette glyph_atlas;
    mirakana::UiRenderSubmitDesc desc;
    desc.glyph_atlas = &glyph_atlas;
    desc.text_layout_policy = mirakana::ui::MonospaceTextLayoutPolicy{
        .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F};

    CaptureRenderer renderer(mirakana::Extent2D{.width = 64, .height = 32});
    renderer.begin_frame();
    const auto result = mirakana::submit_ui_renderer_submission(renderer, submission, desc);
    renderer.end_frame();

    MK_REQUIRE(result.text_glyphs_available == 1);
    MK_REQUIRE(result.text_glyphs_resolved == 0);
    MK_REQUIRE(result.text_glyphs_missing == 1);
    MK_REQUIRE(result.text_glyph_sprites_submitted == 0);
    MK_REQUIRE(renderer.sprites.empty());
    MK_REQUIRE(renderer.stats().sprites_submitted == 0);
}

MK_TEST("ui renderer resolves image palette resource id before asset uri fallback") {
    mirakana::UiRendererImagePalette image_palette;
    MK_REQUIRE(image_palette.try_add(
        mirakana::UiRendererImageBinding{"ui/portrait", "", mirakana::Color{1.0F, 0.0F, 0.0F, 1.0F}}));
    MK_REQUIRE(image_palette.try_add(
        mirakana::UiRendererImageBinding{"", "assets/ui/portrait.texture", mirakana::Color{0.0F, 1.0F, 0.0F, 1.0F}}));

    mirakana::UiRenderSubmitDesc desc;
    desc.image_palette = &image_palette;

    mirakana::ui::ImageAdapterRow resource_priority;
    resource_priority.resource_id = "ui/portrait";
    resource_priority.asset_uri = "assets/ui/portrait.texture";
    const auto* resource_priority_color = mirakana::resolve_ui_image_color(resource_priority, desc);

    MK_REQUIRE(resource_priority_color != nullptr);
    MK_REQUIRE(resource_priority_color->r == 1.0F);
    MK_REQUIRE(resource_priority_color->g == 0.0F);

    mirakana::ui::ImageAdapterRow asset_fallback;
    asset_fallback.resource_id = "ui/missing";
    asset_fallback.asset_uri = "assets/ui/portrait.texture";
    const auto* asset_fallback_color = mirakana::resolve_ui_image_color(asset_fallback, desc);

    MK_REQUIRE(asset_fallback_color != nullptr);
    MK_REQUIRE(asset_fallback_color->r == 0.0F);
    MK_REQUIRE(asset_fallback_color->g == 1.0F);
}

MK_TEST("ui renderer converts image adapter rows into sprite commands") {
    mirakana::ui::ImageAdapterRow image;
    image.id = mirakana::ui::ElementId{"portrait"};
    image.bounds = mirakana::ui::Rect{.x = 4.0F, .y = 8.0F, .width = 32.0F, .height = 16.0F};
    image.resource_id = "ui/portrait";

    const auto command =
        mirakana::make_ui_image_sprite_command(image, mirakana::Color{.r = 0.2F, .g = 0.4F, .b = 0.6F, .a = 0.8F});

    MK_REQUIRE(command.transform.position == (mirakana::Vec2{20.0F, 16.0F}));
    MK_REQUIRE(command.transform.scale == (mirakana::Vec2{32.0F, 16.0F}));
    MK_REQUIRE(command.color.r == 0.2F);
    MK_REQUIRE(command.color.g == 0.4F);
    MK_REQUIRE(command.color.b == 0.6F);
    MK_REQUIRE(command.color.a == 0.8F);
}

MK_TEST("ui renderer marks resolved image sprites with atlas metadata") {
    const auto atlas_page = mirakana::AssetId::from_name("sample/ui/atlas");

    mirakana::UiRendererImagePalette image_palette;
    mirakana::UiRendererImageBinding binding{.resource_id = "ui/portrait",
                                             .asset_uri = "assets/ui/portrait.texture",
                                             .color = mirakana::Color{.r = 0.2F, .g = 0.4F, .b = 0.6F, .a = 0.8F}};
    binding.atlas_page = atlas_page;
    binding.uv_rect = mirakana::SpriteUvRect{.u0 = 0.25F, .v0 = 0.5F, .u1 = 0.75F, .v1 = 1.0F};
    MK_REQUIRE(image_palette.try_add(binding));

    mirakana::UiRenderSubmitDesc desc;
    desc.image_palette = &image_palette;

    mirakana::ui::ImageAdapterRow image;
    image.id = mirakana::ui::ElementId{"portrait"};
    image.bounds = mirakana::ui::Rect{.x = 4.0F, .y = 8.0F, .width = 32.0F, .height = 16.0F};
    image.resource_id = "ui/portrait";
    image.asset_uri = "assets/ui/portrait.texture";

    const auto* resolved = mirakana::resolve_ui_image_binding(image, desc);
    MK_REQUIRE(resolved != nullptr);

    const auto command = mirakana::make_ui_image_sprite_command(image, *resolved);
    MK_REQUIRE(command.texture.enabled);
    MK_REQUIRE(command.texture.atlas_page == atlas_page);
    MK_REQUIRE(command.texture.uv_rect.u0 == 0.25F);
    MK_REQUIRE(command.texture.uv_rect.v0 == 0.5F);
    MK_REQUIRE(command.texture.uv_rect.u1 == 0.75F);
    MK_REQUIRE(command.texture.uv_rect.v1 == 1.0F);
}

MK_TEST("ui renderer builds image palette from runtime ui atlas package metadata") {
    const auto atlas = mirakana::AssetId::from_name("ui/hud-atlas-metadata");
    const auto texture = mirakana::AssetId::from_name("textures/hud-atlas");
    const auto atlas_payload = cooked_ui_atlas_payload(atlas, texture);
    const auto texture_payload = cooked_texture_payload(texture);
    mirakana::runtime::RuntimeAssetPackage package({
        make_runtime_record(mirakana::runtime::RuntimeAssetHandle{1}, atlas, mirakana::AssetKind::ui_atlas,
                            "runtime/assets/ui/hud-atlas.uiatlas", atlas_payload, {texture}),
        make_runtime_record(mirakana::runtime::RuntimeAssetHandle{2}, texture, mirakana::AssetKind::texture,
                            "runtime/assets/ui/hud-atlas.texture.geasset", texture_payload),
    });

    const auto result = mirakana::build_ui_renderer_image_palette_from_runtime_ui_atlas(package, atlas);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.atlas_page_assets.size() == 1);
    MK_REQUIRE(result.atlas_page_assets[0] == texture);
    MK_REQUIRE(result.palette.count() == 1);
    const auto* binding = result.palette.find_by_resource_id("hud.icon");
    MK_REQUIRE(binding != nullptr);
    MK_REQUIRE(binding->asset_uri == "runtime/assets/ui/hud-atlas.texture.geasset");
    MK_REQUIRE(binding->atlas_page == texture);
    MK_REQUIRE(binding->uv_rect.u0 == 0.25F);
    MK_REQUIRE(binding->uv_rect.v0 == 0.5F);
    MK_REQUIRE(binding->uv_rect.u1 == 0.75F);
    MK_REQUIRE(binding->uv_rect.v1 == 1.0F);
}

MK_TEST("ui renderer rejects runtime ui atlas metadata that references a non texture page") {
    const auto atlas = mirakana::AssetId::from_name("ui/hud-atlas-metadata");
    const auto material = mirakana::AssetId::from_name("materials/not-texture");
    const auto atlas_payload = cooked_ui_atlas_payload(atlas, material);
    const auto material_payload = cooked_material_payload(material);
    mirakana::runtime::RuntimeAssetPackage package({
        make_runtime_record(mirakana::runtime::RuntimeAssetHandle{1}, atlas, mirakana::AssetKind::ui_atlas,
                            "runtime/assets/ui/hud-atlas.uiatlas", atlas_payload, {material}),
        make_runtime_record(mirakana::runtime::RuntimeAssetHandle{2}, material, mirakana::AssetKind::material,
                            "runtime/assets/ui/not-texture.material", material_payload),
    });

    const auto result = mirakana::build_ui_renderer_image_palette_from_runtime_ui_atlas(package, atlas);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.palette.count() == 0);
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("not a texture") != std::string::npos);
}

MK_TEST("ui renderer builds glyph atlas palette from runtime ui atlas package metadata") {
    const auto atlas = mirakana::AssetId::from_name("ui/body-glyph-atlas-metadata");
    const auto texture = mirakana::AssetId::from_name("textures/body-glyph-atlas");
    const auto atlas_payload = cooked_ui_glyph_atlas_payload(atlas, texture);
    const auto texture_payload = cooked_texture_payload(texture);
    mirakana::runtime::RuntimeAssetPackage package({
        make_runtime_record(mirakana::runtime::RuntimeAssetHandle{1}, atlas, mirakana::AssetKind::ui_atlas,
                            "runtime/assets/ui/body_glyphs.uiatlas", atlas_payload, {texture}),
        make_runtime_record(mirakana::runtime::RuntimeAssetHandle{2}, texture, mirakana::AssetKind::texture,
                            "runtime/assets/ui/body_glyphs.texture.geasset", texture_payload),
    });

    const auto result = mirakana::build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas(package, atlas);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.failures.empty());
    MK_REQUIRE(result.atlas_page_assets.size() == 1);
    MK_REQUIRE(result.atlas_page_assets[0] == texture);
    MK_REQUIRE(result.palette.count() == 2);
    const auto* binding = result.palette.find("ui/body", 65);
    MK_REQUIRE(binding != nullptr);
    MK_REQUIRE(binding->atlas_page == texture);
    MK_REQUIRE(binding->uv_rect.u0 == 0.0F);
    MK_REQUIRE(binding->uv_rect.v0 == 0.0F);
    MK_REQUIRE(binding->uv_rect.u1 == 0.5F);
    MK_REQUIRE(binding->uv_rect.v1 == 1.0F);
}

MK_TEST("ui renderer rejects runtime glyph atlas metadata that references a non texture page") {
    const auto atlas = mirakana::AssetId::from_name("ui/body-glyph-atlas-metadata");
    const auto material = mirakana::AssetId::from_name("materials/not-texture");
    const auto atlas_payload = cooked_ui_glyph_atlas_payload(atlas, material);
    const auto material_payload = cooked_material_payload(material);
    mirakana::runtime::RuntimeAssetPackage package({
        make_runtime_record(mirakana::runtime::RuntimeAssetHandle{1}, atlas, mirakana::AssetKind::ui_atlas,
                            "runtime/assets/ui/body_glyphs.uiatlas", atlas_payload, {material}),
        make_runtime_record(mirakana::runtime::RuntimeAssetHandle{2}, material, mirakana::AssetKind::material,
                            "runtime/assets/ui/not-texture.material", material_payload),
    });

    const auto result = mirakana::build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas(package, atlas);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.palette.count() == 0);
    MK_REQUIRE(result.failures.size() == 1);
    MK_REQUIRE(result.failures[0].diagnostic.find("not a texture") != std::string::npos);
}

MK_TEST("ui renderer reports unresolved image placeholders without submitting fake sprites") {
    mirakana::ui::RendererSubmission submission;

    mirakana::ui::RendererImagePlaceholder image;
    image.id = mirakana::ui::ElementId{"portrait"};
    image.bounds = mirakana::ui::Rect{.x = 4.0F, .y = 8.0F, .width = 32.0F, .height = 32.0F};
    image.image.resource_id = "ui/missing";
    submission.image_placeholders.push_back(image);

    mirakana::UiRendererImagePalette image_palette;
    mirakana::UiRenderSubmitDesc desc;
    desc.image_palette = &image_palette;

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 64, .height = 64});
    renderer.begin_frame();
    const auto result = mirakana::submit_ui_renderer_submission(renderer, submission, desc);
    renderer.end_frame();

    const auto stats = renderer.stats();
    MK_REQUIRE(result.image_placeholders_available == 1);
    MK_REQUIRE(result.image_sprites_submitted == 0);
    MK_REQUIRE(result.image_resources_resolved == 0);
    MK_REQUIRE(result.image_resources_missing == 1);
    MK_REQUIRE(stats.sprites_submitted == 0);
}

MK_TEST("ui renderer keeps invalid image adapter rows diagnostic only") {
    mirakana::ui::RendererSubmission submission;

    mirakana::ui::RendererImagePlaceholder image;
    image.id = mirakana::ui::ElementId{"bad_image"};
    image.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 16.0F};
    image.image.resource_id = "ui/bad";
    submission.image_placeholders.push_back(image);

    mirakana::UiRendererImagePalette image_palette;
    MK_REQUIRE(
        image_palette.try_add(mirakana::UiRendererImageBinding{"ui/bad", "", mirakana::Color{1.0F, 0.0F, 0.0F, 1.0F}}));

    mirakana::UiRenderSubmitDesc desc;
    desc.image_palette = &image_palette;

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 64, .height = 64});
    renderer.begin_frame();
    const auto result = mirakana::submit_ui_renderer_submission(renderer, submission, desc);
    renderer.end_frame();

    const auto stats = renderer.stats();
    MK_REQUIRE(result.image_placeholders_available == 1);
    MK_REQUIRE(result.image_sprites_submitted == 0);
    MK_REQUIRE(result.image_resources_resolved == 0);
    MK_REQUIRE(result.image_resources_missing == 0);
    MK_REQUIRE(result.adapter_diagnostics_available == 1);
    MK_REQUIRE(stats.sprites_submitted == 0);
}

MK_TEST("ui adapter payloads expose deterministic text image and accessibility rows") {
    mirakana::ui::UiDocument document;

    mirakana::ui::ElementDesc root;
    root.id = mirakana::ui::ElementId{"root"};
    root.role = mirakana::ui::SemanticRole::root;
    root.style.layout = mirakana::ui::LayoutMode::column;
    root.style.padding = mirakana::ui::EdgeInsets{.top = 8.0F, .right = 8.0F, .bottom = 8.0F, .left = 8.0F};
    root.style.gap = 4.0F;
    MK_REQUIRE(document.try_add_element(root));

    mirakana::ui::ElementDesc title;
    title.id = mirakana::ui::ElementId{"title"};
    title.parent = root.id;
    title.role = mirakana::ui::SemanticRole::label;
    title.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 64.0F, .height = 16.0F};
    title.text.label = "Start Game";
    title.text.localization_key = "menu.start";
    title.text.font_family = "ui/default";
    title.text.wrap = mirakana::ui::TextWrapMode::wrap;
    title.style.foreground_token = "text.primary";
    MK_REQUIRE(document.try_add_element(title));

    mirakana::ui::ElementDesc portrait;
    portrait.id = mirakana::ui::ElementId{"portrait"};
    portrait.parent = root.id;
    portrait.role = mirakana::ui::SemanticRole::image;
    portrait.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 32.0F};
    portrait.image.resource_id = "ui/portrait";
    portrait.image.asset_uri = "assets/ui/portrait.texture";
    portrait.image.tint_token = "image.tint";
    portrait.accessibility_label = "Player portrait";
    MK_REQUIRE(document.try_add_element(portrait));

    mirakana::ui::ElementDesc button;
    button.id = mirakana::ui::ElementId{"start"};
    button.parent = root.id;
    button.role = mirakana::ui::SemanticRole::button;
    button.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 96.0F, .height = 24.0F};
    button.text.label = "Play";
    button.accessibility_label = "Start game";
    button.style.background_token = "button.primary";
    MK_REQUIRE(document.try_add_element(button));

    mirakana::ui::ElementDesc options;
    options.id = mirakana::ui::ElementId{"options"};
    options.parent = root.id;
    options.role = mirakana::ui::SemanticRole::button;
    options.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 96.0F, .height = 24.0F};
    options.text.localization_key = "menu.options";
    options.style.background_token = "button.secondary";
    MK_REQUIRE(document.try_add_element(options));

    const auto layout = mirakana::ui::solve_layout(
        document, root.id, mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 128.0F, .height = 128.0F});
    const auto submission = mirakana::ui::build_renderer_submission(document, layout);

    const auto text_payload = mirakana::ui::build_text_adapter_payload(submission);
    const auto image_payload = mirakana::ui::build_image_adapter_payload(submission);
    const auto accessibility_payload = mirakana::ui::build_accessibility_payload(submission);

    MK_REQUIRE(text_payload.rows.size() == 3);
    MK_REQUIRE(text_payload.rows[0].id == title.id);
    MK_REQUIRE(text_payload.rows[0].text == "Start Game");
    MK_REQUIRE(text_payload.rows[0].localization_key == "menu.start");
    MK_REQUIRE(text_payload.rows[0].lines.size() == 1);
    MK_REQUIRE(text_payload.rows[0].lines[0].glyphs.size() == 1);
    MK_REQUIRE(text_payload.rows[0].lines[0].glyphs[0].code_unit_offset == 0);
    MK_REQUIRE(text_payload.rows[0].lines[0].glyphs[0].code_unit_count == 10);
    MK_REQUIRE(text_payload.rows[0].lines[0].glyphs[0].bounds == (mirakana::ui::Rect{8.0F, 8.0F, 64.0F, 16.0F}));
    MK_REQUIRE(text_payload.rows[2].id == options.id);
    MK_REQUIRE(text_payload.rows[2].text.empty());
    MK_REQUIRE(text_payload.rows[2].localization_key == "menu.options");
    MK_REQUIRE(text_payload.rows[2].lines.empty());
    MK_REQUIRE(text_payload.diagnostics.size() == 1);
    MK_REQUIRE(text_payload.diagnostics[0].id == options.id);
    MK_REQUIRE(text_payload.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::unresolved_text_localization_key);

    MK_REQUIRE(image_payload.rows.size() == 1);
    MK_REQUIRE(image_payload.rows[0].id == portrait.id);
    MK_REQUIRE(image_payload.rows[0].resource_id == "ui/portrait");
    MK_REQUIRE(image_payload.rows[0].asset_uri == "assets/ui/portrait.texture");
    MK_REQUIRE(image_payload.rows[0].bounds == (mirakana::ui::Rect{8.0F, 28.0F, 32.0F, 32.0F}));
    MK_REQUIRE(image_payload.diagnostics.empty());

    MK_REQUIRE(accessibility_payload.nodes.size() == 4);
    MK_REQUIRE(accessibility_payload.nodes[0].id == title.id);
    MK_REQUIRE(accessibility_payload.nodes[0].label == "Start Game");
    MK_REQUIRE(accessibility_payload.nodes[0].localization_key == "menu.start");
    MK_REQUIRE(!accessibility_payload.nodes[0].focusable);
    MK_REQUIRE(accessibility_payload.nodes[0].parent == root.id);
    MK_REQUIRE(accessibility_payload.nodes[0].depth == 1);
    MK_REQUIRE(accessibility_payload.nodes[1].id == portrait.id);
    MK_REQUIRE(accessibility_payload.nodes[1].label == "Player portrait");
    MK_REQUIRE(accessibility_payload.nodes[2].id == button.id);
    MK_REQUIRE(accessibility_payload.nodes[2].focusable);
    MK_REQUIRE(accessibility_payload.nodes[3].id == options.id);
    MK_REQUIRE(accessibility_payload.nodes[3].label.empty());
    MK_REQUIRE(accessibility_payload.nodes[3].localization_key == "menu.options");
    MK_REQUIRE(accessibility_payload.nodes[3].focusable);
    MK_REQUIRE(accessibility_payload.diagnostics.empty());

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 128, .height = 128});
    renderer.begin_frame();
    const auto result = mirakana::submit_ui_renderer_submission(renderer, submission);
    renderer.end_frame();

    MK_REQUIRE(result.text_adapter_rows_available == 3);
    MK_REQUIRE(result.image_placeholders_available == 1);
    MK_REQUIRE(result.accessibility_nodes_available == 4);
    MK_REQUIRE(result.adapter_diagnostics_available == 1);
}

MK_TEST("ui adapter payloads diagnose invalid public submission bounds") {
    mirakana::ui::RendererSubmission submission;

    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"bad_text"};
    text.bounds =
        mirakana::ui::Rect{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F, .width = 16.0F, .height = 16.0F};
    text.text.label = "Bad";
    submission.text_runs.push_back(text);

    mirakana::ui::RendererImagePlaceholder image;
    image.id = mirakana::ui::ElementId{"bad_image"};
    image.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 16.0F};
    image.image.resource_id = "ui/bad";
    submission.image_placeholders.push_back(image);

    mirakana::ui::AccessibilityNode node;
    node.id = mirakana::ui::ElementId{"bad_accessibility"};
    node.role = mirakana::ui::SemanticRole::button;
    node.label = "Bad";
    node.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 16.0F, .height = -1.0F};
    node.enabled = true;
    node.focusable = true;
    submission.accessibility_nodes.push_back(node);

    const auto text_payload = mirakana::ui::build_text_adapter_payload(submission);
    const auto image_payload = mirakana::ui::build_image_adapter_payload(submission);
    const auto accessibility_payload = mirakana::ui::build_accessibility_payload(submission);

    MK_REQUIRE(text_payload.rows.size() == 1);
    MK_REQUIRE(text_payload.rows[0].lines.empty());
    MK_REQUIRE(text_payload.diagnostics.size() == 1);
    MK_REQUIRE(text_payload.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_bounds);

    MK_REQUIRE(image_payload.rows.size() == 1);
    MK_REQUIRE(image_payload.diagnostics.size() == 1);
    MK_REQUIRE(image_payload.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_image_bounds);

    MK_REQUIRE(accessibility_payload.nodes.size() == 1);
    MK_REQUIRE(accessibility_payload.diagnostics.size() == 1);
    MK_REQUIRE(accessibility_payload.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_accessibility_bounds);
}

MK_TEST("ui accessibility publish plan dispatches validated nodes to adapter") {
    mirakana::ui::AccessibilityPayload payload;
    payload.nodes.push_back(mirakana::ui::AccessibilityNode{
        .id = mirakana::ui::ElementId{"start"},
        .role = mirakana::ui::SemanticRole::button,
        .label = "Start game",
        .bounds = mirakana::ui::Rect{.x = 4.0F, .y = 8.0F, .width = 96.0F, .height = 24.0F},
        .localization_key = "menu.start",
        .enabled = true,
        .focusable = true,
        .parent = mirakana::ui::ElementId{"root"},
        .depth = 1,
    });

    const auto plan = mirakana::ui::plan_accessibility_publish(payload);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.nodes.size() == 1);
    MK_REQUIRE(plan.diagnostics.empty());

    CapturingAccessibilityAdapter adapter;
    const auto result = mirakana::ui::publish_accessibility_payload(adapter, payload);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.published);
    MK_REQUIRE(result.nodes_published == 1);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.publish_calls == 1);
    MK_REQUIRE(adapter.published_nodes.size() == 1);
    MK_REQUIRE(adapter.published_nodes[0].id == mirakana::ui::ElementId{"start"});
    MK_REQUIRE(adapter.published_nodes[0].label == "Start game");
}

MK_TEST("ui accessibility publish plan blocks invalid nodes before adapter") {
    mirakana::ui::AccessibilityPayload payload;
    payload.nodes.push_back(mirakana::ui::AccessibilityNode{
        .id = mirakana::ui::ElementId{"bad"},
        .role = mirakana::ui::SemanticRole::button,
        .label = "Bad",
        .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 24.0F},
        .localization_key = "",
        .enabled = true,
        .focusable = true,
        .parent = mirakana::ui::ElementId{"root"},
        .depth = 1,
    });
    payload.diagnostics.push_back(mirakana::ui::AdapterPayloadDiagnostic{
        .id = mirakana::ui::ElementId{"bad"},
        .code = mirakana::ui::AdapterPayloadDiagnosticCode::invalid_accessibility_bounds,
        .message = "accessibility node has invalid or non-positive bounds",
    });

    const auto plan = mirakana::ui::plan_accessibility_publish(payload);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.nodes.size() == 1);
    MK_REQUIRE(plan.diagnostics.size() == 1);

    CapturingAccessibilityAdapter adapter;
    const auto result = mirakana::ui::publish_accessibility_payload(adapter, payload);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.published);
    MK_REQUIRE(result.nodes_published == 0);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(adapter.publish_calls == 0);
    MK_REQUIRE(adapter.published_nodes.empty());
}

MK_TEST("ui ime composition publish plan dispatches valid composition to adapter") {
    mirakana::ui::ImeComposition composition;
    composition.target = mirakana::ui::ElementId{"chat.input"};
    composition.composition_text = "nihon";
    composition.cursor_index = 3;

    const auto plan = mirakana::ui::plan_ime_composition_update(composition);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.composition.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(plan.composition.composition_text == "nihon");
    MK_REQUIRE(plan.composition.cursor_index == 3);
    MK_REQUIRE(plan.diagnostics.empty());

    CapturingImeAdapter adapter;
    const auto result = mirakana::ui::publish_ime_composition(adapter, composition);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.published);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.publish_calls == 1);
    MK_REQUIRE(adapter.published_composition.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(adapter.published_composition.composition_text == "nihon");
    MK_REQUIRE(adapter.published_composition.cursor_index == 3);

    mirakana::ui::ImeComposition clear_composition;
    clear_composition.target = mirakana::ui::ElementId{"chat.input"};

    const auto clear_plan = mirakana::ui::plan_ime_composition_update(clear_composition);
    MK_REQUIRE(clear_plan.ready());

    const auto clear_result = mirakana::ui::publish_ime_composition(adapter, clear_composition);
    MK_REQUIRE(clear_result.succeeded());
    MK_REQUIRE(adapter.publish_calls == 2);
    MK_REQUIRE(adapter.published_composition.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(adapter.published_composition.composition_text.empty());
    MK_REQUIRE(adapter.published_composition.cursor_index == 0);
}

MK_TEST("ui ime composition publish plan blocks invalid composition before adapter") {
    mirakana::ui::ImeComposition composition;
    composition.target = mirakana::ui::ElementId{""};
    composition.composition_text = "ime";
    composition.cursor_index = 4;

    const auto plan = mirakana::ui::plan_ime_composition_update(composition);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.composition.composition_text == "ime");
    MK_REQUIRE(plan.diagnostics.size() == 2);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_ime_target);
    MK_REQUIRE(plan.diagnostics[1].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_ime_cursor);

    CapturingImeAdapter adapter;
    const auto result = mirakana::ui::publish_ime_composition(adapter, composition);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.published);
    MK_REQUIRE(result.diagnostics.size() == 2);
    MK_REQUIRE(adapter.publish_calls == 0);
    MK_REQUIRE(adapter.published_composition.target.value.empty());
}

MK_TEST("ui committed text input inserts at cursor and advances cursor") {
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "heo",
        .cursor_byte_offset = 2,
        .selection_byte_length = 0,
    };
    const mirakana::ui::CommittedTextInput input{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "ll",
    };

    const auto plan = mirakana::ui::plan_committed_text_input(state, input);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.state.text == "heo");
    MK_REQUIRE(plan.input.text == "ll");
    MK_REQUIRE(plan.diagnostics.empty());

    const auto result = mirakana::ui::apply_committed_text_input(state, input);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.committed);
    MK_REQUIRE(result.state.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(result.state.text == "hello");
    MK_REQUIRE(result.state.cursor_byte_offset == 4U);
    MK_REQUIRE(result.state.selection_byte_length == 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("ui committed text input replaces selected utf8 scalar range") {
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 3,
    };
    const mirakana::ui::CommittedTextInput input{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "\xE3\x81\x84",
    };

    const auto result = mirakana::ui::apply_committed_text_input(state, input);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.state.text == "a\xE3\x81\x84"
                                    "b");
    MK_REQUIRE(result.state.cursor_byte_offset == 4U);
    MK_REQUIRE(result.state.selection_byte_length == 0U);
}

MK_TEST("ui committed text input rejects invalid state and input without mutation") {
    const mirakana::ui::CommittedTextInput valid_input{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "x",
    };

    const mirakana::ui::TextEditState empty_target{
        .target = mirakana::ui::ElementId{""},
        .text = "abc",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };
    const auto empty_target_result = mirakana::ui::apply_committed_text_input(empty_target, valid_input);
    MK_REQUIRE(!empty_target_result.succeeded());
    MK_REQUIRE(!empty_target_result.committed);
    MK_REQUIRE(empty_target_result.state.text == empty_target.text);
    MK_REQUIRE(empty_target_result.state.cursor_byte_offset == empty_target.cursor_byte_offset);
    MK_REQUIRE(empty_target_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_target);

    const mirakana::ui::TextEditState valid_state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };
    const mirakana::ui::CommittedTextInput mismatched_target{
        .target = mirakana::ui::ElementId{"other.input"},
        .text = "x",
    };
    const auto mismatch_result = mirakana::ui::apply_committed_text_input(valid_state, mismatched_target);
    MK_REQUIRE(!mismatch_result.succeeded());
    MK_REQUIRE(mismatch_result.state.text == valid_state.text);
    MK_REQUIRE(mismatch_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::mismatched_committed_text_target);

    const mirakana::ui::TextEditState split_cursor{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 2,
        .selection_byte_length = 0,
    };
    const auto split_cursor_result = mirakana::ui::apply_committed_text_input(split_cursor, valid_input);
    MK_REQUIRE(!split_cursor_result.succeeded());
    MK_REQUIRE(split_cursor_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_cursor);

    const mirakana::ui::TextEditState split_selection{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 2,
    };
    const auto split_selection_result = mirakana::ui::apply_committed_text_input(split_selection, valid_input);
    MK_REQUIRE(!split_selection_result.succeeded());
    MK_REQUIRE(split_selection_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_selection);

    const mirakana::ui::TextEditState range_overflow{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "abc",
        .cursor_byte_offset = 4,
        .selection_byte_length = 0,
    };
    const auto overflow_result = mirakana::ui::apply_committed_text_input(range_overflow, valid_input);
    MK_REQUIRE(!overflow_result.succeeded());
    MK_REQUIRE(overflow_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_cursor);

    const mirakana::ui::TextEditState malformed_state_text{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "\xED\xA0\x80",
        .cursor_byte_offset = 0,
        .selection_byte_length = 0,
    };
    const auto malformed_state_result = mirakana::ui::apply_committed_text_input(malformed_state_text, valid_input);
    MK_REQUIRE(!malformed_state_result.succeeded());
    MK_REQUIRE(!malformed_state_result.committed);
    MK_REQUIRE(malformed_state_result.state.text == malformed_state_text.text);
    MK_REQUIRE(malformed_state_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_cursor);

    const mirakana::ui::CommittedTextInput empty_text{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "",
    };
    const auto empty_text_result = mirakana::ui::apply_committed_text_input(valid_state, empty_text);
    MK_REQUIRE(!empty_text_result.succeeded());
    MK_REQUIRE(empty_text_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_committed_text);

    const mirakana::ui::CommittedTextInput malformed_text{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "\xE3\x81",
    };
    const auto malformed_text_result = mirakana::ui::apply_committed_text_input(valid_state, malformed_text);
    MK_REQUIRE(!malformed_text_result.succeeded());
    MK_REQUIRE(malformed_text_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_committed_text);
}

MK_TEST("ui text edit command moves cursor by utf8 scalar boundaries") {
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "bc",
        .cursor_byte_offset = 1,
        .selection_byte_length = 4,
    };

    const auto collapsed_start = mirakana::ui::apply_text_edit_command(
        state, mirakana::ui::TextEditCommand{
                   .target = mirakana::ui::ElementId{"chat.input"},
                   .kind = mirakana::ui::TextEditCommandKind::move_cursor_backward,
               });

    MK_REQUIRE(collapsed_start.succeeded());
    MK_REQUIRE(collapsed_start.state.cursor_byte_offset == 1U);
    MK_REQUIRE(collapsed_start.state.selection_byte_length == 0U);

    const auto forward =
        mirakana::ui::apply_text_edit_command(state, mirakana::ui::TextEditCommand{
                                                         .target = mirakana::ui::ElementId{"chat.input"},
                                                         .kind = mirakana::ui::TextEditCommandKind::move_cursor_forward,
                                                     });

    MK_REQUIRE(forward.succeeded());
    MK_REQUIRE(forward.applied);
    MK_REQUIRE(forward.state.text == state.text);
    MK_REQUIRE(forward.state.cursor_byte_offset == 5U);
    MK_REQUIRE(forward.state.selection_byte_length == 0U);

    const auto end = mirakana::ui::apply_text_edit_command(
        forward.state, mirakana::ui::TextEditCommand{
                           .target = mirakana::ui::ElementId{"chat.input"},
                           .kind = mirakana::ui::TextEditCommandKind::move_cursor_to_end,
                       });

    MK_REQUIRE(end.succeeded());
    MK_REQUIRE(end.state.cursor_byte_offset == 6U);
    MK_REQUIRE(end.state.selection_byte_length == 0U);

    const auto backward = mirakana::ui::apply_text_edit_command(
        end.state, mirakana::ui::TextEditCommand{
                       .target = mirakana::ui::ElementId{"chat.input"},
                       .kind = mirakana::ui::TextEditCommandKind::move_cursor_backward,
                   });

    MK_REQUIRE(backward.succeeded());
    MK_REQUIRE(backward.state.cursor_byte_offset == 5U);

    const auto start = mirakana::ui::apply_text_edit_command(
        backward.state, mirakana::ui::TextEditCommand{
                            .target = mirakana::ui::ElementId{"chat.input"},
                            .kind = mirakana::ui::TextEditCommandKind::move_cursor_to_start,
                        });

    MK_REQUIRE(start.succeeded());
    MK_REQUIRE(start.state.cursor_byte_offset == 0U);
    MK_REQUIRE(start.state.selection_byte_length == 0U);
}

MK_TEST("ui text edit command deletes backward and forward utf8 scalars") {
    const mirakana::ui::TextEditState before_hiragana{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 4,
        .selection_byte_length = 0,
    };

    const auto backward = mirakana::ui::apply_text_edit_command(
        before_hiragana, mirakana::ui::TextEditCommand{
                             .target = mirakana::ui::ElementId{"chat.input"},
                             .kind = mirakana::ui::TextEditCommandKind::delete_backward,
                         });

    MK_REQUIRE(backward.succeeded());
    MK_REQUIRE(backward.applied);
    MK_REQUIRE(backward.state.text == "ab");
    MK_REQUIRE(backward.state.cursor_byte_offset == 1U);
    MK_REQUIRE(backward.state.selection_byte_length == 0U);

    const mirakana::ui::TextEditState after_ascii{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };

    const auto forward = mirakana::ui::apply_text_edit_command(
        after_ascii, mirakana::ui::TextEditCommand{
                         .target = mirakana::ui::ElementId{"chat.input"},
                         .kind = mirakana::ui::TextEditCommandKind::delete_forward,
                     });

    MK_REQUIRE(forward.succeeded());
    MK_REQUIRE(forward.state.text == "ab");
    MK_REQUIRE(forward.state.cursor_byte_offset == 1U);
    MK_REQUIRE(forward.state.selection_byte_length == 0U);
}

MK_TEST("ui text edit command deletes selected utf8 range before scalar fallback") {
    const mirakana::ui::TextEditState selected{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "bc",
        .cursor_byte_offset = 1,
        .selection_byte_length = 4,
    };

    const auto backward =
        mirakana::ui::apply_text_edit_command(selected, mirakana::ui::TextEditCommand{
                                                            .target = mirakana::ui::ElementId{"chat.input"},
                                                            .kind = mirakana::ui::TextEditCommandKind::delete_backward,
                                                        });

    MK_REQUIRE(backward.succeeded());
    MK_REQUIRE(backward.state.text == "ac");
    MK_REQUIRE(backward.state.cursor_byte_offset == 1U);
    MK_REQUIRE(backward.state.selection_byte_length == 0U);

    const auto forward =
        mirakana::ui::apply_text_edit_command(selected, mirakana::ui::TextEditCommand{
                                                            .target = mirakana::ui::ElementId{"chat.input"},
                                                            .kind = mirakana::ui::TextEditCommandKind::delete_forward,
                                                        });

    MK_REQUIRE(forward.succeeded());
    MK_REQUIRE(forward.state.text == "ac");
    MK_REQUIRE(forward.state.cursor_byte_offset == 1U);
    MK_REQUIRE(forward.state.selection_byte_length == 0U);

    const mirakana::ui::TextEditState at_start{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "abc",
        .cursor_byte_offset = 0,
        .selection_byte_length = 0,
    };
    const auto start_noop =
        mirakana::ui::apply_text_edit_command(at_start, mirakana::ui::TextEditCommand{
                                                            .target = mirakana::ui::ElementId{"chat.input"},
                                                            .kind = mirakana::ui::TextEditCommandKind::delete_backward,
                                                        });

    MK_REQUIRE(start_noop.succeeded());
    MK_REQUIRE(start_noop.applied);
    MK_REQUIRE(start_noop.state.text == "abc");
    MK_REQUIRE(start_noop.state.cursor_byte_offset == 0U);

    const mirakana::ui::TextEditState at_end{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "abc",
        .cursor_byte_offset = 3,
        .selection_byte_length = 0,
    };
    const auto end_noop =
        mirakana::ui::apply_text_edit_command(at_end, mirakana::ui::TextEditCommand{
                                                          .target = mirakana::ui::ElementId{"chat.input"},
                                                          .kind = mirakana::ui::TextEditCommandKind::delete_forward,
                                                      });

    MK_REQUIRE(end_noop.succeeded());
    MK_REQUIRE(end_noop.applied);
    MK_REQUIRE(end_noop.state.text == "abc");
    MK_REQUIRE(end_noop.state.cursor_byte_offset == 3U);
}

MK_TEST("ui text edit command rejects invalid rows without mutation") {
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };
    const mirakana::ui::TextEditCommand valid_command{
        .target = mirakana::ui::ElementId{"chat.input"},
        .kind = mirakana::ui::TextEditCommandKind::delete_forward,
    };

    const mirakana::ui::TextEditState empty_target{
        .target = mirakana::ui::ElementId{""},
        .text = "abc",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };
    const auto empty_target_result = mirakana::ui::apply_text_edit_command(empty_target, valid_command);
    MK_REQUIRE(!empty_target_result.succeeded());
    MK_REQUIRE(!empty_target_result.applied);
    MK_REQUIRE(empty_target_result.state.text == empty_target.text);
    MK_REQUIRE(empty_target_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_target);

    const mirakana::ui::TextEditCommand mismatched_target{
        .target = mirakana::ui::ElementId{"other.input"},
        .kind = mirakana::ui::TextEditCommandKind::delete_forward,
    };
    const auto mismatch_result = mirakana::ui::apply_text_edit_command(state, mismatched_target);
    MK_REQUIRE(!mismatch_result.succeeded());
    MK_REQUIRE(mismatch_result.state.text == state.text);
    MK_REQUIRE(mismatch_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::mismatched_text_edit_command_target);

    const mirakana::ui::TextEditState split_cursor{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 2,
        .selection_byte_length = 0,
    };
    const auto split_cursor_result = mirakana::ui::apply_text_edit_command(split_cursor, valid_command);
    MK_REQUIRE(!split_cursor_result.succeeded());
    MK_REQUIRE(split_cursor_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_cursor);

    const mirakana::ui::TextEditState split_selection{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 2,
    };
    const auto split_selection_result = mirakana::ui::apply_text_edit_command(split_selection, valid_command);
    MK_REQUIRE(!split_selection_result.succeeded());
    MK_REQUIRE(split_selection_result.state.text == split_selection.text);
    MK_REQUIRE(split_selection_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_selection);

    const mirakana::ui::TextEditState malformed_state_text{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "\xED\xA0\x80",
        .cursor_byte_offset = 0,
        .selection_byte_length = 0,
    };
    const auto malformed_state_result = mirakana::ui::apply_text_edit_command(malformed_state_text, valid_command);
    MK_REQUIRE(!malformed_state_result.succeeded());
    MK_REQUIRE(!malformed_state_result.applied);
    MK_REQUIRE(malformed_state_result.state.text == malformed_state_text.text);
    MK_REQUIRE(malformed_state_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_cursor);

    const mirakana::ui::TextEditCommand invalid_kind{
        .target = mirakana::ui::ElementId{"chat.input"},
        .kind = static_cast<mirakana::ui::TextEditCommandKind>(255),
    };
    const auto invalid_kind_result = mirakana::ui::apply_text_edit_command(state, invalid_kind);
    MK_REQUIRE(!invalid_kind_result.succeeded());
    MK_REQUIRE(!invalid_kind_result.applied);
    MK_REQUIRE(invalid_kind_result.state.text == state.text);
    MK_REQUIRE(invalid_kind_result.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_command);
}

MK_TEST("ui text edit clipboard command copies selected text without mutating state") {
    CapturingClipboardTextAdapter adapter;
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello world",
        .cursor_byte_offset = 6,
        .selection_byte_length = 5,
    };

    const auto plan = mirakana::ui::plan_text_edit_clipboard_command(
        state, mirakana::ui::TextEditClipboardCommand{
                   .target = state.target, .kind = mirakana::ui::TextEditClipboardCommandKind::copy_selection});
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.state.text == "hello world");

    const auto result = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, state,
        mirakana::ui::TextEditClipboardCommand{.target = state.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::copy_selection});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.state.text == state.text);
    MK_REQUIRE(result.state.cursor_byte_offset == 6U);
    MK_REQUIRE(result.state.selection_byte_length == 5U);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.write_calls == 1);
    MK_REQUIRE(adapter.text == "world");
}

MK_TEST("ui text edit clipboard command cuts selected utf8 text") {
    CapturingClipboardTextAdapter adapter;
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "bc",
        .cursor_byte_offset = 1,
        .selection_byte_length = 4,
    };

    const auto result = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, state,
        mirakana::ui::TextEditClipboardCommand{.target = state.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::cut_selection});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.state.text == "ac");
    MK_REQUIRE(result.state.cursor_byte_offset == 1U);
    MK_REQUIRE(result.state.selection_byte_length == 0U);
    MK_REQUIRE(adapter.write_calls == 1);
    MK_REQUIRE(adapter.text == "\xE3\x81\x82"
                               "b");
}

MK_TEST("ui text edit clipboard command pastes text into cursor or selection") {
    CapturingClipboardTextAdapter adapter;
    adapter.text = "++";

    const mirakana::ui::TextEditState insert_state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "ab",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };

    const auto inserted = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, insert_state,
        mirakana::ui::TextEditClipboardCommand{.target = insert_state.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::paste_text});

    MK_REQUIRE(inserted.succeeded());
    MK_REQUIRE(inserted.applied);
    MK_REQUIRE(inserted.state.text == "a++b");
    MK_REQUIRE(inserted.state.cursor_byte_offset == 3U);
    MK_REQUIRE(inserted.state.selection_byte_length == 0U);
    MK_REQUIRE(adapter.has_calls == 1);
    MK_REQUIRE(adapter.read_calls == 1);

    adapter.text = "\xE3\x81\x84";
    const mirakana::ui::TextEditState replace_state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 3,
    };

    const auto replaced = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, replace_state,
        mirakana::ui::TextEditClipboardCommand{.target = replace_state.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::paste_text});

    MK_REQUIRE(replaced.succeeded());
    MK_REQUIRE(replaced.state.text == "a\xE3\x81\x84"
                                      "b");
    MK_REQUIRE(replaced.state.cursor_byte_offset == 4U);
    MK_REQUIRE(replaced.state.selection_byte_length == 0U);
    MK_REQUIRE(adapter.has_calls == 2);
    MK_REQUIRE(adapter.read_calls == 2);
}

MK_TEST("ui text edit clipboard command treats empty clipboard paste as no op") {
    CapturingClipboardTextAdapter adapter;
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello",
        .cursor_byte_offset = 2,
        .selection_byte_length = 0,
    };

    const auto result = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, state,
        mirakana::ui::TextEditClipboardCommand{.target = state.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::paste_text});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.applied);
    MK_REQUIRE(result.state.text == "hello");
    MK_REQUIRE(result.state.cursor_byte_offset == 2U);
    MK_REQUIRE(result.state.selection_byte_length == 0U);
    MK_REQUIRE(adapter.has_calls == 1);
    MK_REQUIRE(adapter.read_calls == 0);
}

MK_TEST("ui text edit clipboard command rejects invalid rows before adapter mutation") {
    CapturingClipboardTextAdapter adapter;
    const mirakana::ui::TextEditState valid_state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };

    const mirakana::ui::TextEditState empty_target_state{
        .target = mirakana::ui::ElementId{},
        .text = "hello",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };
    const auto empty_target = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, empty_target_state,
        mirakana::ui::TextEditClipboardCommand{.target = empty_target_state.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::paste_text});
    MK_REQUIRE(!empty_target.succeeded());
    MK_REQUIRE(!empty_target.applied);
    MK_REQUIRE(empty_target.state.text == empty_target_state.text);
    MK_REQUIRE(empty_target.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_target);
    MK_REQUIRE(adapter.has_calls == 0);
    MK_REQUIRE(adapter.write_calls == 0);

    const auto empty_copy = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, valid_state,
        mirakana::ui::TextEditClipboardCommand{.target = valid_state.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::copy_selection});
    MK_REQUIRE(!empty_copy.succeeded());
    MK_REQUIRE(!empty_copy.applied);
    MK_REQUIRE(empty_copy.state.text == valid_state.text);
    MK_REQUIRE(empty_copy.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_selection);
    MK_REQUIRE(adapter.write_calls == 0);

    const auto mismatch = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, valid_state,
        mirakana::ui::TextEditClipboardCommand{.target = mirakana::ui::ElementId{"other.input"},
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::paste_text});
    MK_REQUIRE(!mismatch.succeeded());
    MK_REQUIRE(mismatch.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::mismatched_text_edit_clipboard_command_target);
    MK_REQUIRE(adapter.has_calls == 0);

    const mirakana::ui::TextEditState split_selection{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82",
        .cursor_byte_offset = 1,
        .selection_byte_length = 2,
    };
    const auto split = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, split_selection,
        mirakana::ui::TextEditClipboardCommand{.target = split_selection.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::cut_selection});
    MK_REQUIRE(!split.succeeded());
    MK_REQUIRE(split.state.text == split_selection.text);
    MK_REQUIRE(split.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_selection);

    const mirakana::ui::TextEditClipboardCommand invalid_kind{
        .target = valid_state.target,
        .kind = static_cast<mirakana::ui::TextEditClipboardCommandKind>(255),
    };
    const auto invalid = mirakana::ui::apply_text_edit_clipboard_command(adapter, valid_state, invalid_kind);
    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_edit_clipboard_command);
}

MK_TEST("ui text edit clipboard command rejects invalid adapter paste text without mutation") {
    CapturingClipboardTextAdapter adapter;
    adapter.text = "\xE3\x81";
    adapter.use_forced_has_text = true;
    adapter.forced_has_text = true;

    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello",
        .cursor_byte_offset = 2,
        .selection_byte_length = 0,
    };

    const auto result = mirakana::ui::apply_text_edit_clipboard_command(
        adapter, state,
        mirakana::ui::TextEditClipboardCommand{.target = state.target,
                                               .kind = mirakana::ui::TextEditClipboardCommandKind::paste_text});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.applied);
    MK_REQUIRE(result.state.text == "hello");
    MK_REQUIRE(result.state.cursor_byte_offset == 2U);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text_result);
    MK_REQUIRE(adapter.has_calls == 1);
    MK_REQUIRE(adapter.read_calls == 1);
}

MK_TEST("ui platform text input session plan dispatches valid begin request to adapter") {
    mirakana::ui::PlatformTextInputRequest request;
    request.target = mirakana::ui::ElementId{"chat.input"};
    request.text_bounds = mirakana::ui::Rect{.x = 8.0F, .y = 12.0F, .width = 240.0F, .height = 32.0F};

    const auto plan = mirakana::ui::plan_platform_text_input_session(request);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.request.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(plan.request.text_bounds.x == 8.0F);
    MK_REQUIRE(plan.request.text_bounds.y == 12.0F);
    MK_REQUIRE(plan.request.text_bounds.width == 240.0F);
    MK_REQUIRE(plan.request.text_bounds.height == 32.0F);
    MK_REQUIRE(plan.diagnostics.empty());

    CapturingPlatformIntegrationAdapter adapter;
    const auto result = mirakana::ui::begin_platform_text_input(adapter, request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.begun);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.begin_calls == 1);
    MK_REQUIRE(adapter.published_begin_request.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(adapter.published_begin_request.text_bounds.width == 240.0F);
    MK_REQUIRE(adapter.end_calls == 0);
}

MK_TEST("ui platform text input session plan blocks invalid begin request before adapter") {
    mirakana::ui::PlatformTextInputRequest request;
    request.target = mirakana::ui::ElementId{""};
    request.text_bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 24.0F};

    const auto plan = mirakana::ui::plan_platform_text_input_session(request);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.diagnostics.size() == 2);
    MK_REQUIRE(plan.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_platform_text_input_target);
    MK_REQUIRE(plan.diagnostics[1].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_platform_text_input_bounds);

    CapturingPlatformIntegrationAdapter adapter;
    const auto result = mirakana::ui::begin_platform_text_input(adapter, request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.begun);
    MK_REQUIRE(result.diagnostics.size() == 2);
    MK_REQUIRE(adapter.begin_calls == 0);
    MK_REQUIRE(adapter.published_begin_request.target.value.empty());
    MK_REQUIRE(adapter.end_calls == 0);
}

MK_TEST("ui platform text input end plan dispatches valid target and blocks invalid target") {
    const mirakana::ui::ElementId target{"chat.input"};

    const auto plan = mirakana::ui::plan_platform_text_input_end(target);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(plan.diagnostics.empty());

    CapturingPlatformIntegrationAdapter adapter;
    const auto result = mirakana::ui::end_platform_text_input(adapter, target);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.ended);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.end_calls == 1);
    MK_REQUIRE(adapter.published_end_target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(adapter.begin_calls == 0);

    const auto invalid_plan = mirakana::ui::plan_platform_text_input_end(mirakana::ui::ElementId{""});
    MK_REQUIRE(!invalid_plan.ready());
    MK_REQUIRE(invalid_plan.diagnostics.size() == 1);
    MK_REQUIRE(invalid_plan.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_platform_text_input_target);

    const auto invalid_result = mirakana::ui::end_platform_text_input(adapter, mirakana::ui::ElementId{""});
    MK_REQUIRE(!invalid_result.succeeded());
    MK_REQUIRE(!invalid_result.ended);
    MK_REQUIRE(invalid_result.diagnostics.size() == 1);
    MK_REQUIRE(adapter.end_calls == 1);
}

MK_TEST("ui clipboard text write request dispatches valid text and clear rows") {
    CapturingClipboardTextAdapter adapter;

    const mirakana::ui::ClipboardTextWriteRequest request{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "copy me",
    };

    const auto plan = mirakana::ui::plan_clipboard_text_write(request);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.request.target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(plan.request.text == "copy me");

    const auto write = mirakana::ui::write_clipboard_text(adapter, request);
    MK_REQUIRE(write.succeeded());
    MK_REQUIRE(write.written);
    MK_REQUIRE(write.diagnostics.empty());
    MK_REQUIRE(adapter.write_calls == 1);
    MK_REQUIRE(adapter.text == "copy me");

    const auto clear = mirakana::ui::write_clipboard_text(
        adapter, mirakana::ui::ClipboardTextWriteRequest{.target = mirakana::ui::ElementId{"chat.input"}, .text = ""});
    MK_REQUIRE(clear.succeeded());
    MK_REQUIRE(clear.written);
    MK_REQUIRE(adapter.write_calls == 2);
    MK_REQUIRE(adapter.text.empty());
}

MK_TEST("ui clipboard text write request blocks invalid rows before adapter") {
    CapturingClipboardTextAdapter adapter;
    const mirakana::ui::ClipboardTextWriteRequest request{
        .target = mirakana::ui::ElementId{""},
        .text = "\xE3\x81",
    };

    const auto plan = mirakana::ui::plan_clipboard_text_write(request);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.diagnostics.size() == 2);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text_target);
    MK_REQUIRE(plan.diagnostics[1].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text);

    const auto result = mirakana::ui::write_clipboard_text(adapter, request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.written);
    MK_REQUIRE(result.diagnostics.size() == 2);
    MK_REQUIRE(adapter.write_calls == 0);
    MK_REQUIRE(adapter.text.empty());
}

MK_TEST("ui clipboard text read request reports text and empty clipboard rows") {
    CapturingClipboardTextAdapter adapter;
    adapter.text = "paste me";

    const mirakana::ui::ClipboardTextReadRequest request{
        .target = mirakana::ui::ElementId{"chat.input"},
    };

    const auto plan = mirakana::ui::plan_clipboard_text_read(request);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.request.target == mirakana::ui::ElementId{"chat.input"});

    const auto read = mirakana::ui::read_clipboard_text(adapter, request);
    MK_REQUIRE(read.succeeded());
    MK_REQUIRE(read.read);
    MK_REQUIRE(read.has_text);
    MK_REQUIRE(read.text == "paste me");
    MK_REQUIRE(read.diagnostics.empty());
    MK_REQUIRE(adapter.has_calls == 1);
    MK_REQUIRE(adapter.read_calls == 1);

    adapter.text.clear();
    const auto empty = mirakana::ui::read_clipboard_text(adapter, request);
    MK_REQUIRE(empty.succeeded());
    MK_REQUIRE(empty.read);
    MK_REQUIRE(!empty.has_text);
    MK_REQUIRE(empty.text.empty());
    MK_REQUIRE(adapter.has_calls == 2);
    MK_REQUIRE(adapter.read_calls == 1);
}

MK_TEST("ui clipboard text read request blocks invalid targets and invalid adapter text") {
    CapturingClipboardTextAdapter adapter;
    adapter.text = "paste me";

    const auto invalid_target = mirakana::ui::read_clipboard_text(
        adapter, mirakana::ui::ClipboardTextReadRequest{.target = mirakana::ui::ElementId{""}});
    MK_REQUIRE(!invalid_target.succeeded());
    MK_REQUIRE(!invalid_target.read);
    MK_REQUIRE(invalid_target.diagnostics.size() == 1);
    MK_REQUIRE(invalid_target.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text_target);
    MK_REQUIRE(adapter.has_calls == 0);
    MK_REQUIRE(adapter.read_calls == 0);

    adapter.text = "\xE3\x81";
    adapter.use_forced_has_text = true;
    adapter.forced_has_text = true;
    const auto invalid_text = mirakana::ui::read_clipboard_text(
        adapter, mirakana::ui::ClipboardTextReadRequest{.target = mirakana::ui::ElementId{"chat.input"}});
    MK_REQUIRE(!invalid_text.succeeded());
    MK_REQUIRE(invalid_text.read);
    MK_REQUIRE(invalid_text.has_text);
    MK_REQUIRE(invalid_text.text == "\xE3\x81");
    MK_REQUIRE(invalid_text.diagnostics.size() == 1);
    MK_REQUIRE(invalid_text.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_clipboard_text_result);
    MK_REQUIRE(adapter.has_calls == 1);
    MK_REQUIRE(adapter.read_calls == 1);
}

MK_TEST("ui font rasterization request plan dispatches valid request to adapter") {
    mirakana::ui::FontRasterizationRequest request;
    request.font_family = "Inter";
    request.glyph = 65;
    request.pixel_size = 18.0F;

    const auto plan = mirakana::ui::plan_font_rasterization_request(request);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.request.font_family == "Inter");
    MK_REQUIRE(plan.request.glyph == 65);
    MK_REQUIRE(plan.request.pixel_size == 18.0F);
    MK_REQUIRE(plan.diagnostics.empty());

    CapturingFontRasterizerAdapter adapter;
    const auto result = mirakana::ui::rasterize_font_glyph(adapter, request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.rasterized);
    MK_REQUIRE(result.allocation.has_value());
    MK_REQUIRE(result.allocation->glyph == 65);
    MK_REQUIRE(result.allocation->atlas_bounds.width == 18.0F);
    MK_REQUIRE(result.allocation->atlas_bounds.height == 18.0F);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.publish_calls == 1);
    MK_REQUIRE(adapter.published_request.font_family == "Inter");
    MK_REQUIRE(adapter.published_request.glyph == 65);
    MK_REQUIRE(adapter.published_request.pixel_size == 18.0F);
}

MK_TEST("ui font rasterization request plan blocks invalid request before adapter") {
    mirakana::ui::FontRasterizationRequest request;
    request.font_family = "";
    request.glyph = 0;
    request.pixel_size = std::numeric_limits<float>::infinity();

    const auto plan = mirakana::ui::plan_font_rasterization_request(request);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.diagnostics.size() == 3);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_font_family);
    MK_REQUIRE(plan.diagnostics[1].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_font_glyph);
    MK_REQUIRE(plan.diagnostics[2].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_font_pixel_size);

    CapturingFontRasterizerAdapter adapter;
    const auto result = mirakana::ui::rasterize_font_glyph(adapter, request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.rasterized);
    MK_REQUIRE(!result.allocation.has_value());
    MK_REQUIRE(result.diagnostics.size() == 3);
    MK_REQUIRE(adapter.publish_calls == 0);
    MK_REQUIRE(adapter.published_request.font_family.empty());

    mirakana::ui::FontRasterizationRequest zero_size_request;
    zero_size_request.font_family = "Inter";
    zero_size_request.glyph = 65;
    zero_size_request.pixel_size = 0.0F;

    const auto zero_size_plan = mirakana::ui::plan_font_rasterization_request(zero_size_request);
    MK_REQUIRE(!zero_size_plan.ready());
    MK_REQUIRE(zero_size_plan.diagnostics.size() == 1);
    MK_REQUIRE(zero_size_plan.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_font_pixel_size);
}

MK_TEST("ui font rasterization result reports invalid adapter allocation") {
    mirakana::ui::FontRasterizationRequest request;
    request.font_family = "Inter";
    request.glyph = 65;
    request.pixel_size = 18.0F;

    InvalidFontRasterizerAdapter adapter;
    const auto result = mirakana::ui::rasterize_font_glyph(adapter, request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.rasterized);
    MK_REQUIRE(result.allocation.has_value());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_font_allocation);
    MK_REQUIRE(adapter.publish_calls == 1);
    MK_REQUIRE(adapter.published_request.font_family == "Inter");
}

MK_TEST("ui image decode request plan dispatches valid request to adapter") {
    mirakana::ui::ImageDecodeRequest request;
    request.asset_uri = "assets/ui/icon.png";
    request.bytes = byte_payload(4);

    const auto plan = mirakana::ui::plan_image_decode_request(request);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.request.asset_uri == "assets/ui/icon.png");
    MK_REQUIRE(plan.request.bytes.size() == 4);
    MK_REQUIRE(plan.diagnostics.empty());

    CapturingImageDecodingAdapter adapter;
    adapter.response = mirakana::ui::ImageDecodeResult{
        .width = 2,
        .height = 1,
        .pixel_format = mirakana::ui::ImageDecodePixelFormat::rgba8_unorm,
        .pixels = byte_payload(8),
    };

    const auto result = mirakana::ui::decode_image_request(adapter, request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.decoded);
    MK_REQUIRE(result.image.has_value());
    MK_REQUIRE(result.image->width == 2);
    MK_REQUIRE(result.image->height == 1);
    MK_REQUIRE(result.image->pixel_format == mirakana::ui::ImageDecodePixelFormat::rgba8_unorm);
    MK_REQUIRE(result.image->pixels.size() == 8);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.publish_calls == 1);
    MK_REQUIRE(adapter.published_request.asset_uri == "assets/ui/icon.png");
    MK_REQUIRE(adapter.published_request.bytes.size() == 4);
}

MK_TEST("ui image decode request plan blocks invalid request before adapter") {
    mirakana::ui::ImageDecodeRequest request;
    request.asset_uri = "assets/ui/\nicon.png";

    const auto plan = mirakana::ui::plan_image_decode_request(request);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.diagnostics.size() == 2);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_image_decode_uri);
    MK_REQUIRE(plan.diagnostics[1].code == mirakana::ui::AdapterPayloadDiagnosticCode::empty_image_decode_bytes);

    CapturingImageDecodingAdapter adapter;
    adapter.response = mirakana::ui::ImageDecodeResult{
        .width = 1,
        .height = 1,
        .pixel_format = mirakana::ui::ImageDecodePixelFormat::rgba8_unorm,
        .pixels = byte_payload(4),
    };
    const auto result = mirakana::ui::decode_image_request(adapter, request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.decoded);
    MK_REQUIRE(!result.image.has_value());
    MK_REQUIRE(result.diagnostics.size() == 2);
    MK_REQUIRE(adapter.publish_calls == 0);
    MK_REQUIRE(adapter.published_request.asset_uri.empty());

    mirakana::ui::ImageDecodeRequest empty_uri_request;
    empty_uri_request.bytes = byte_payload(1);

    const auto empty_uri_plan = mirakana::ui::plan_image_decode_request(empty_uri_request);
    MK_REQUIRE(!empty_uri_plan.ready());
    MK_REQUIRE(empty_uri_plan.diagnostics.size() == 1);
    MK_REQUIRE(empty_uri_plan.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_image_decode_uri);
}

MK_TEST("ui image decode result reports missing or invalid adapter output") {
    mirakana::ui::ImageDecodeRequest request;
    request.asset_uri = "assets/ui/icon.png";
    request.bytes = byte_payload(4);

    CapturingImageDecodingAdapter missing_adapter;
    const auto missing = mirakana::ui::decode_image_request(missing_adapter, request);

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.decoded);
    MK_REQUIRE(!missing.image.has_value());
    MK_REQUIRE(missing.diagnostics.size() == 1);
    MK_REQUIRE(missing.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_image_decode_result);
    MK_REQUIRE(missing_adapter.publish_calls == 1);

    CapturingImageDecodingAdapter invalid_adapter;
    invalid_adapter.response = mirakana::ui::ImageDecodeResult{
        .width = 2,
        .height = 2,
        .pixel_format = mirakana::ui::ImageDecodePixelFormat::rgba8_unorm,
        .pixels = byte_payload(7),
    };
    const auto invalid = mirakana::ui::decode_image_request(invalid_adapter, request);

    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.decoded);
    MK_REQUIRE(invalid.image.has_value());
    MK_REQUIRE(invalid.diagnostics.size() == 1);
    MK_REQUIRE(invalid.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_image_decode_result);
    MK_REQUIRE(invalid_adapter.publish_calls == 1);
}

MK_TEST("ui text shaping request plan dispatches valid request to adapter") {
    mirakana::ui::TextLayoutRequest request;
    request.text = "Play";
    request.font_family = "Inter";
    request.direction = mirakana::ui::TextDirection::left_to_right;
    request.wrap = mirakana::ui::TextWrapMode::clip;
    request.max_width = 160.0F;

    const auto plan = mirakana::ui::plan_text_shaping_request(request);
    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.request.text == "Play");
    MK_REQUIRE(plan.request.font_family == "Inter");
    MK_REQUIRE(plan.request.max_width == 160.0F);
    MK_REQUIRE(plan.diagnostics.empty());

    CapturingTextShapingAdapter adapter;
    adapter.response.push_back(mirakana::ui::TextLayoutRun{
        .text = "Play",
        .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 16.0F},
    });

    const auto result = mirakana::ui::shape_text_run(adapter, request);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.shaped);
    MK_REQUIRE(result.runs.size() == 1);
    MK_REQUIRE(result.runs[0].text == "Play");
    MK_REQUIRE(result.runs[0].bounds == (mirakana::ui::Rect{0.0F, 0.0F, 32.0F, 16.0F}));
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(adapter.publish_calls == 1);
    MK_REQUIRE(adapter.published_request.text == "Play");
    MK_REQUIRE(adapter.published_request.font_family == "Inter");
    MK_REQUIRE(adapter.published_request.max_width == 160.0F);
}

MK_TEST("ui text shaping request plan blocks invalid request before adapter") {
    mirakana::ui::TextLayoutRequest request;
    request.text = "Play\nNow";
    request.font_family = "";
    request.max_width = -1.0F;

    const auto plan = mirakana::ui::plan_text_shaping_request(request);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.diagnostics.size() == 3);
    MK_REQUIRE(plan.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_text);
    MK_REQUIRE(plan.diagnostics[1].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_font_family);
    MK_REQUIRE(plan.diagnostics[2].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_max_width);

    CapturingTextShapingAdapter adapter;
    adapter.response.push_back(mirakana::ui::TextLayoutRun{
        .text = "Play\nNow",
        .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 64.0F, .height = 16.0F},
    });

    const auto result = mirakana::ui::shape_text_run(adapter, request);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.shaped);
    MK_REQUIRE(result.runs.empty());
    MK_REQUIRE(result.diagnostics.size() == 3);
    MK_REQUIRE(adapter.publish_calls == 0);
    MK_REQUIRE(adapter.published_request.text.empty());

    mirakana::ui::TextLayoutRequest empty_text_request;
    empty_text_request.text = "";
    empty_text_request.font_family = "Inter";
    empty_text_request.max_width = 160.0F;

    const auto empty_text_plan = mirakana::ui::plan_text_shaping_request(empty_text_request);
    MK_REQUIRE(!empty_text_plan.ready());
    MK_REQUIRE(empty_text_plan.diagnostics.size() == 1);
    MK_REQUIRE(empty_text_plan.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_text);

    mirakana::ui::TextLayoutRequest unsafe_font_request;
    unsafe_font_request.text = "Play";
    unsafe_font_request.font_family = "Inter\nFallback";
    unsafe_font_request.max_width = 160.0F;

    const auto unsafe_font_plan = mirakana::ui::plan_text_shaping_request(unsafe_font_request);
    MK_REQUIRE(!unsafe_font_plan.ready());
    MK_REQUIRE(unsafe_font_plan.diagnostics.size() == 1);
    MK_REQUIRE(unsafe_font_plan.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_font_family);

    mirakana::ui::TextLayoutRequest infinite_width_request;
    infinite_width_request.text = "Play";
    infinite_width_request.font_family = "Inter";
    infinite_width_request.max_width = std::numeric_limits<float>::infinity();

    const auto infinite_width_plan = mirakana::ui::plan_text_shaping_request(infinite_width_request);
    MK_REQUIRE(!infinite_width_plan.ready());
    MK_REQUIRE(infinite_width_plan.diagnostics.size() == 1);
    MK_REQUIRE(infinite_width_plan.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_max_width);

    mirakana::ui::TextLayoutRequest nan_width_request;
    nan_width_request.text = "Play";
    nan_width_request.font_family = "Inter";
    nan_width_request.max_width = std::numeric_limits<float>::quiet_NaN();

    const auto nan_width_plan = mirakana::ui::plan_text_shaping_request(nan_width_request);
    MK_REQUIRE(!nan_width_plan.ready());
    MK_REQUIRE(nan_width_plan.diagnostics.size() == 1);
    MK_REQUIRE(nan_width_plan.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_max_width);
}

MK_TEST("ui text shaping result reports invalid adapter runs") {
    mirakana::ui::TextLayoutRequest request;
    request.text = "Play";
    request.font_family = "Inter";

    CapturingTextShapingAdapter missing_adapter;
    const auto missing = mirakana::ui::shape_text_run(missing_adapter, request);

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.shaped);
    MK_REQUIRE(missing.runs.empty());
    MK_REQUIRE(missing.diagnostics.size() == 1);
    MK_REQUIRE(missing.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_result);
    MK_REQUIRE(missing_adapter.publish_calls == 1);

    CapturingTextShapingAdapter invalid_bounds_adapter;
    invalid_bounds_adapter.response.push_back(mirakana::ui::TextLayoutRun{
        .text = "Play",
        .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 0.0F, .height = 16.0F},
    });
    const auto invalid_bounds = mirakana::ui::shape_text_run(invalid_bounds_adapter, request);

    MK_REQUIRE(!invalid_bounds.succeeded());
    MK_REQUIRE(invalid_bounds.shaped);
    MK_REQUIRE(invalid_bounds.runs.size() == 1);
    MK_REQUIRE(invalid_bounds.diagnostics.size() == 1);
    MK_REQUIRE(invalid_bounds.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_result);
    MK_REQUIRE(invalid_bounds_adapter.publish_calls == 1);

    CapturingTextShapingAdapter empty_text_adapter;
    empty_text_adapter.response.push_back(mirakana::ui::TextLayoutRun{
        .text = "",
        .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 16.0F},
    });
    const auto empty_text = mirakana::ui::shape_text_run(empty_text_adapter, request);

    MK_REQUIRE(!empty_text.succeeded());
    MK_REQUIRE(empty_text.shaped);
    MK_REQUIRE(empty_text.runs.size() == 1);
    MK_REQUIRE(empty_text.diagnostics.size() == 1);
    MK_REQUIRE(empty_text.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_result);
    MK_REQUIRE(empty_text_adapter.publish_calls == 1);

    CapturingTextShapingAdapter unsafe_text_adapter;
    unsafe_text_adapter.response.push_back(mirakana::ui::TextLayoutRun{
        .text = "Pl\nay",
        .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 16.0F},
    });
    const auto unsafe_text = mirakana::ui::shape_text_run(unsafe_text_adapter, request);

    MK_REQUIRE(!unsafe_text.succeeded());
    MK_REQUIRE(unsafe_text.shaped);
    MK_REQUIRE(unsafe_text.runs.size() == 1);
    MK_REQUIRE(unsafe_text.diagnostics.size() == 1);
    MK_REQUIRE(unsafe_text.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_result);
    MK_REQUIRE(unsafe_text_adapter.publish_calls == 1);

    CapturingTextShapingAdapter mismatch_adapter;
    mismatch_adapter.response.push_back(mirakana::ui::TextLayoutRun{
        .text = "Plan",
        .bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 16.0F},
    });
    const auto mismatch = mirakana::ui::shape_text_run(mismatch_adapter, request);

    MK_REQUIRE(!mismatch.succeeded());
    MK_REQUIRE(mismatch.shaped);
    MK_REQUIRE(mismatch.runs.size() == 1);
    MK_REQUIRE(mismatch.diagnostics.size() == 1);
    MK_REQUIRE(mismatch.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_shaping_result);
    MK_REQUIRE(mismatch_adapter.publish_calls == 1);
}

MK_TEST("ui monospace text layout policy wraps words into deterministic glyph boxes") {
    mirakana::ui::RendererSubmission submission;

    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"title"};
    text.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 36.0F, .height = 24.0F};
    text.text.label = "Play Now";
    text.text.wrap = mirakana::ui::TextWrapMode::wrap;
    submission.text_runs.push_back(text);

    const auto payload = mirakana::ui::build_text_adapter_payload(
        submission, mirakana::ui::MonospaceTextLayoutPolicy{
                        .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F});

    MK_REQUIRE(payload.rows.size() == 1);
    MK_REQUIRE(payload.rows[0].lines.size() == 2);
    MK_REQUIRE(payload.rows[0].lines[0].code_unit_offset == 0);
    MK_REQUIRE(payload.rows[0].lines[0].code_unit_count == 4);
    MK_REQUIRE(payload.rows[0].lines[0].bounds == (mirakana::ui::Rect{0.0F, 0.0F, 32.0F, 10.0F}));
    MK_REQUIRE(payload.rows[0].lines[0].glyphs.size() == 4);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[0].bounds == (mirakana::ui::Rect{0.0F, 0.0F, 8.0F, 10.0F}));
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[1].bounds == (mirakana::ui::Rect{8.0F, 0.0F, 8.0F, 10.0F}));
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[2].bounds == (mirakana::ui::Rect{16.0F, 0.0F, 8.0F, 10.0F}));
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[3].bounds == (mirakana::ui::Rect{24.0F, 0.0F, 8.0F, 10.0F}));
    MK_REQUIRE(payload.rows[0].lines[1].code_unit_offset == 5);
    MK_REQUIRE(payload.rows[0].lines[1].code_unit_count == 3);
    MK_REQUIRE(payload.rows[0].lines[1].bounds == (mirakana::ui::Rect{0.0F, 10.0F, 24.0F, 10.0F}));
    MK_REQUIRE(payload.rows[0].lines[1].glyphs.size() == 3);
    MK_REQUIRE(payload.diagnostics.empty());
}

MK_TEST("ui monospace text layout policy clips text with diagnostics") {
    mirakana::ui::RendererSubmission submission;

    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"short"};
    text.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 24.0F, .height = 10.0F};
    text.text.label = "ABCDE";
    text.text.wrap = mirakana::ui::TextWrapMode::clip;
    submission.text_runs.push_back(text);

    const auto payload = mirakana::ui::build_text_adapter_payload(
        submission, mirakana::ui::MonospaceTextLayoutPolicy{
                        .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F});

    MK_REQUIRE(payload.rows.size() == 1);
    MK_REQUIRE(payload.rows[0].lines.size() == 1);
    MK_REQUIRE(payload.rows[0].lines[0].code_unit_offset == 0);
    MK_REQUIRE(payload.rows[0].lines[0].code_unit_count == 3);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs.size() == 3);
    MK_REQUIRE(payload.diagnostics.size() == 1);
    MK_REQUIRE(payload.diagnostics[0].id == text.id);
    MK_REQUIRE(payload.diagnostics[0].code == mirakana::ui::AdapterPayloadDiagnosticCode::text_layout_clipped);
}

MK_TEST("ui monospace text layout policy reports invalid policy and unsupported direction") {
    mirakana::ui::RendererSubmission invalid_policy_submission;

    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"invalid_policy"};
    text.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 24.0F, .height = 10.0F};
    text.text.label = "ABC";
    invalid_policy_submission.text_runs.push_back(text);

    const auto invalid_policy_payload = mirakana::ui::build_text_adapter_payload(
        invalid_policy_submission, mirakana::ui::MonospaceTextLayoutPolicy{
                                       .glyph_advance = 0.0F, .whitespace_advance = 4.0F, .line_height = 10.0F});

    MK_REQUIRE(invalid_policy_payload.rows.size() == 1);
    MK_REQUIRE(invalid_policy_payload.rows[0].lines.empty());
    MK_REQUIRE(invalid_policy_payload.diagnostics.size() == 1);
    MK_REQUIRE(invalid_policy_payload.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::invalid_text_layout_policy);

    mirakana::ui::RendererSubmission rtl_submission;
    mirakana::ui::RendererTextRun rtl_text;
    rtl_text.id = mirakana::ui::ElementId{"rtl"};
    rtl_text.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 24.0F, .height = 10.0F};
    rtl_text.text.label = "RTL";
    rtl_text.text.direction = mirakana::ui::TextDirection::right_to_left;
    rtl_submission.text_runs.push_back(rtl_text);

    const auto rtl_payload = mirakana::ui::build_text_adapter_payload(
        rtl_submission, mirakana::ui::MonospaceTextLayoutPolicy{
                            .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F});

    MK_REQUIRE(rtl_payload.rows.size() == 1);
    MK_REQUIRE(rtl_payload.rows[0].lines.size() == 1);
    MK_REQUIRE(rtl_payload.diagnostics.size() == 1);
    MK_REQUIRE(rtl_payload.diagnostics[0].code ==
               mirakana::ui::AdapterPayloadDiagnosticCode::unsupported_text_direction);
}

MK_TEST("ui monospace text layout policy treats malformed utf8 bytes as single byte fallback glyphs") {
    std::string malformed;
    malformed.push_back(static_cast<char>(0xC0));
    malformed.push_back(static_cast<char>(0x80));
    malformed.push_back(static_cast<char>(0xED));
    malformed.push_back(static_cast<char>(0xA0));
    malformed.push_back(static_cast<char>(0x80));
    malformed.push_back('A');

    mirakana::ui::RendererSubmission submission;
    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"malformed_utf8"};
    text.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 64.0F, .height = 10.0F};
    text.text.label = malformed;
    text.text.wrap = mirakana::ui::TextWrapMode::clip;
    submission.text_runs.push_back(text);

    const auto payload = mirakana::ui::build_text_adapter_payload(
        submission, mirakana::ui::MonospaceTextLayoutPolicy{
                        .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F});

    MK_REQUIRE(payload.rows.size() == 1);
    MK_REQUIRE(payload.rows[0].lines.size() == 1);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs.size() == 6);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[0].code_unit_offset == 0);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[0].code_unit_count == 1);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[1].code_unit_offset == 1);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[1].code_unit_count == 1);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[2].code_unit_offset == 2);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[2].code_unit_count == 1);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[3].code_unit_offset == 3);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[3].code_unit_count == 1);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[4].code_unit_offset == 4);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[4].code_unit_count == 1);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[5].code_unit_offset == 5);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[5].code_unit_count == 1);
}

MK_TEST("ui monospace text layout policy treats line feed as ascii whitespace for wrapping") {
    mirakana::ui::RendererSubmission submission;

    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"linefeed"};
    text.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 20.0F, .height = 10.0F};
    text.text.label = "A\nB";
    text.text.wrap = mirakana::ui::TextWrapMode::wrap;
    submission.text_runs.push_back(text);

    const auto payload = mirakana::ui::build_text_adapter_payload(
        submission, mirakana::ui::MonospaceTextLayoutPolicy{
                        .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F});

    MK_REQUIRE(payload.rows.size() == 1);
    MK_REQUIRE(payload.rows[0].lines.size() == 1);
    MK_REQUIRE(payload.rows[0].lines[0].code_unit_offset == 0);
    MK_REQUIRE(payload.rows[0].lines[0].code_unit_count == 3);
    MK_REQUIRE(payload.rows[0].lines[0].bounds == (mirakana::ui::Rect{0.0F, 0.0F, 20.0F, 10.0F}));
    MK_REQUIRE(payload.rows[0].lines[0].glyphs.size() == 3);
    MK_REQUIRE(payload.rows[0].lines[0].glyphs[1].bounds == (mirakana::ui::Rect{8.0F, 0.0F, 4.0F, 10.0F}));
    MK_REQUIRE(payload.diagnostics.empty());
}

int main() {
    return mirakana::test::run_all();
}
