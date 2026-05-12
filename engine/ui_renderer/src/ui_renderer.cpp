// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace mirakana {

namespace {

[[nodiscard]] bool is_positive_ui_rect(ui::Rect rect) noexcept {
    return ui::is_valid_rect(rect) && rect.width > 0.0F && rect.height > 0.0F;
}

} // namespace

bool UiRendererTheme::try_add(UiThemeColor color) {
    if (color.token.empty() || find(color.token) != nullptr) {
        return false;
    }
    colors_.push_back(std::move(color));
    return true;
}

void UiRendererTheme::add(UiThemeColor color) {
    if (!try_add(std::move(color))) {
        throw std::logic_error("ui theme color could not be added");
    }
}

const Color* UiRendererTheme::find(std::string_view token) const noexcept {
    for (const auto& color : colors_) {
        if (color.token == token) {
            return &color.color;
        }
    }
    return nullptr;
}

std::size_t UiRendererTheme::count() const noexcept {
    return colors_.size();
}

bool UiRendererImagePalette::try_add(UiRendererImageBinding binding) {
    if (binding.resource_id.empty() && binding.asset_uri.empty()) {
        return false;
    }
    if (!binding.resource_id.empty() && find_by_resource_id(binding.resource_id) != nullptr) {
        return false;
    }
    if (!binding.asset_uri.empty() && find_by_asset_uri(binding.asset_uri) != nullptr) {
        return false;
    }
    bindings_.push_back(std::move(binding));
    return true;
}

void UiRendererImagePalette::add(UiRendererImageBinding binding) {
    if (!try_add(std::move(binding))) {
        throw std::logic_error("ui renderer image binding could not be added");
    }
}

const UiRendererImageBinding* UiRendererImagePalette::find_by_resource_id(std::string_view resource_id) const noexcept {
    if (resource_id.empty()) {
        return nullptr;
    }
    for (const auto& binding : bindings_) {
        if (binding.resource_id == resource_id) {
            return &binding;
        }
    }
    return nullptr;
}

const UiRendererImageBinding* UiRendererImagePalette::find_by_asset_uri(std::string_view asset_uri) const noexcept {
    if (asset_uri.empty()) {
        return nullptr;
    }
    for (const auto& binding : bindings_) {
        if (binding.asset_uri == asset_uri) {
            return &binding;
        }
    }
    return nullptr;
}

std::size_t UiRendererImagePalette::count() const noexcept {
    return bindings_.size();
}

bool UiRendererGlyphAtlasPalette::try_add(UiRendererGlyphAtlasBinding binding) {
    if (binding.font_family.empty() || binding.glyph == 0 || binding.atlas_page.value == 0) {
        return false;
    }
    if (find(binding.font_family, binding.glyph) != nullptr) {
        return false;
    }
    bindings_.push_back(std::move(binding));
    return true;
}

void UiRendererGlyphAtlasPalette::add(UiRendererGlyphAtlasBinding binding) {
    if (!try_add(std::move(binding))) {
        throw std::logic_error("ui renderer glyph atlas binding could not be added");
    }
}

const UiRendererGlyphAtlasBinding* UiRendererGlyphAtlasPalette::find(std::string_view font_family,
                                                                     std::uint32_t glyph) const noexcept {
    if (font_family.empty() || glyph == 0) {
        return nullptr;
    }
    for (const auto& binding : bindings_) {
        if (binding.font_family == font_family && binding.glyph == glyph) {
            return &binding;
        }
    }
    return nullptr;
}

std::size_t UiRendererGlyphAtlasPalette::count() const noexcept {
    return bindings_.size();
}

bool UiRendererImagePaletteBuildResult::succeeded() const noexcept {
    return failures.empty();
}

bool UiRendererGlyphAtlasPaletteBuildResult::succeeded() const noexcept {
    return failures.empty();
}

Color resolve_ui_box_color(const ui::RendererBox& box, const UiRenderSubmitDesc& desc) noexcept {
    if (desc.theme != nullptr) {
        if (const auto* color = desc.theme->find(box.background_token); color != nullptr) {
            return *color;
        }
    }
    return desc.fallback_box_color;
}

const UiRendererImageBinding* resolve_ui_image_binding(const ui::ImageAdapterRow& row,
                                                       const UiRenderSubmitDesc& desc) noexcept {
    if (desc.image_palette == nullptr) {
        return nullptr;
    }
    if (const auto* binding = desc.image_palette->find_by_resource_id(row.resource_id); binding != nullptr) {
        return binding;
    }
    if (const auto* binding = desc.image_palette->find_by_asset_uri(row.asset_uri); binding != nullptr) {
        return binding;
    }
    return nullptr;
}

const Color* resolve_ui_image_color(const ui::ImageAdapterRow& row, const UiRenderSubmitDesc& desc) noexcept {
    const auto* binding = resolve_ui_image_binding(row, desc);
    return binding != nullptr ? &binding->color : nullptr;
}

const UiRendererGlyphAtlasBinding* resolve_ui_text_glyph_binding(const ui::TextAdapterRow& row,
                                                                 const ui::TextAdapterGlyphPlaceholder& glyph,
                                                                 const UiRenderSubmitDesc& desc) noexcept {
    if (desc.glyph_atlas == nullptr) {
        return nullptr;
    }
    return desc.glyph_atlas->find(row.font_family, glyph.glyph);
}

SpriteCommand make_ui_box_sprite_command(const ui::RendererBox& box, const UiRenderSubmitDesc& desc) noexcept {
    return SpriteCommand{
        .transform =
            Transform2D{
                .position =
                    Vec2{.x = box.bounds.x + (box.bounds.width * 0.5F), .y = box.bounds.y + (box.bounds.height * 0.5F)},
                .scale = Vec2{.x = box.bounds.width, .y = box.bounds.height},
                .rotation_radians = 0.0F,
            },
        .color = resolve_ui_box_color(box, desc),
    };
}

SpriteCommand make_ui_image_sprite_command(const ui::ImageAdapterRow& row, Color color) noexcept {
    return SpriteCommand{
        .transform =
            Transform2D{
                .position =
                    Vec2{.x = row.bounds.x + (row.bounds.width * 0.5F), .y = row.bounds.y + (row.bounds.height * 0.5F)},
                .scale = Vec2{.x = row.bounds.width, .y = row.bounds.height},
                .rotation_radians = 0.0F,
            },
        .color = color,
    };
}

SpriteCommand make_ui_image_sprite_command(const ui::ImageAdapterRow& row,
                                           const UiRendererImageBinding& binding) noexcept {
    auto command = make_ui_image_sprite_command(row, binding.color);
    if (binding.atlas_page.value != 0) {
        command.texture = SpriteTextureRegion{
            .enabled = true,
            .atlas_page = binding.atlas_page,
            .uv_rect = binding.uv_rect,
        };
    }
    return command;
}

SpriteCommand make_ui_text_glyph_sprite_command(const ui::TextAdapterGlyphPlaceholder& glyph,
                                                const UiRendererGlyphAtlasBinding& binding) noexcept {
    return SpriteCommand{
        .transform =
            Transform2D{
                .position = Vec2{.x = glyph.bounds.x + (glyph.bounds.width * 0.5F),
                                 .y = glyph.bounds.y + (glyph.bounds.height * 0.5F)},
                .scale = Vec2{.x = glyph.bounds.width, .y = glyph.bounds.height},
                .rotation_radians = 0.0F,
            },
        .color = binding.color,
        .texture =
            SpriteTextureRegion{
                .enabled = true,
                .atlas_page = binding.atlas_page,
                .uv_rect = binding.uv_rect,
            },
    };
}

UiRendererImagePaletteBuildResult
build_ui_renderer_image_palette_from_runtime_ui_atlas(const runtime::RuntimeAssetPackage& package,
                                                      AssetId atlas_metadata_asset) {
    UiRendererImagePaletteBuildResult result;
    const auto* atlas_record = package.find(atlas_metadata_asset);
    if (atlas_record == nullptr) {
        result.failures.push_back(UiRendererImagePaletteBuildFailure{
            .asset = atlas_metadata_asset,
            .diagnostic = "ui atlas metadata package record is missing",
        });
        return result;
    }

    const auto atlas = runtime::runtime_ui_atlas_payload(*atlas_record);
    if (!atlas.succeeded()) {
        result.failures.push_back(
            UiRendererImagePaletteBuildFailure{.asset = atlas_metadata_asset, .diagnostic = atlas.diagnostic});
        return result;
    }

    result.atlas_page_assets.reserve(atlas.payload.pages.size());
    for (const auto& page : atlas.payload.pages) {
        const auto* page_record = package.find(page.asset);
        if (page_record == nullptr) {
            result.failures.push_back(UiRendererImagePaletteBuildFailure{
                .asset = page.asset,
                .diagnostic = "ui atlas page asset is missing from the runtime package",
            });
            continue;
        }
        if (page_record->kind != AssetKind::texture) {
            result.failures.push_back(UiRendererImagePaletteBuildFailure{
                .asset = page.asset,
                .diagnostic = "ui atlas page asset is not a texture",
            });
            continue;
        }
        result.atlas_page_assets.push_back(page.asset);
    }
    if (!result.failures.empty()) {
        result.atlas_page_assets.clear();
        return result;
    }

    for (const auto& image : atlas.payload.images) {
        UiRendererImageBinding binding;
        binding.resource_id = image.resource_id;
        binding.asset_uri = image.asset_uri;
        binding.color = Color{.r = image.color[0], .g = image.color[1], .b = image.color[2], .a = image.color[3]};
        binding.atlas_page = image.page;
        binding.uv_rect = SpriteUvRect{.u0 = image.u0, .v0 = image.v0, .u1 = image.u1, .v1 = image.v1};
        if (!result.palette.try_add(std::move(binding))) {
            result.failures.push_back(UiRendererImagePaletteBuildFailure{
                .asset = atlas_metadata_asset,
                .diagnostic = "ui atlas image binding could not be added to the palette",
            });
            result.palette = UiRendererImagePalette{};
            result.atlas_page_assets.clear();
            return result;
        }
    }

    return result;
}

UiRendererGlyphAtlasPaletteBuildResult
build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas(const runtime::RuntimeAssetPackage& package,
                                                            AssetId atlas_metadata_asset) {
    UiRendererGlyphAtlasPaletteBuildResult result;
    const auto* atlas_record = package.find(atlas_metadata_asset);
    if (atlas_record == nullptr) {
        result.failures.push_back(UiRendererGlyphAtlasPaletteBuildFailure{
            .asset = atlas_metadata_asset,
            .diagnostic = "ui atlas metadata package record is missing",
        });
        return result;
    }

    const auto atlas = runtime::runtime_ui_atlas_payload(*atlas_record);
    if (!atlas.succeeded()) {
        result.failures.push_back(
            UiRendererGlyphAtlasPaletteBuildFailure{.asset = atlas_metadata_asset, .diagnostic = atlas.diagnostic});
        return result;
    }

    result.atlas_page_assets.reserve(atlas.payload.pages.size());
    for (const auto& page : atlas.payload.pages) {
        const auto* page_record = package.find(page.asset);
        if (page_record == nullptr) {
            result.failures.push_back(UiRendererGlyphAtlasPaletteBuildFailure{
                .asset = page.asset,
                .diagnostic = "ui atlas page asset is missing from the runtime package",
            });
            continue;
        }
        if (page_record->kind != AssetKind::texture) {
            result.failures.push_back(UiRendererGlyphAtlasPaletteBuildFailure{
                .asset = page.asset,
                .diagnostic = "ui atlas page asset is not a texture",
            });
            continue;
        }
        result.atlas_page_assets.push_back(page.asset);
    }
    if (!result.failures.empty()) {
        result.atlas_page_assets.clear();
        return result;
    }

    for (const auto& glyph : atlas.payload.glyphs) {
        UiRendererGlyphAtlasBinding binding;
        binding.font_family = glyph.font_family;
        binding.glyph = glyph.glyph;
        binding.color = Color{.r = glyph.color[0], .g = glyph.color[1], .b = glyph.color[2], .a = glyph.color[3]};
        binding.atlas_page = glyph.page;
        binding.uv_rect = SpriteUvRect{.u0 = glyph.u0, .v0 = glyph.v0, .u1 = glyph.u1, .v1 = glyph.v1};
        if (!result.palette.try_add(std::move(binding))) {
            result.failures.push_back(UiRendererGlyphAtlasPaletteBuildFailure{
                .asset = atlas_metadata_asset,
                .diagnostic = "ui atlas glyph binding could not be added to the palette",
            });
            result.palette = UiRendererGlyphAtlasPalette{};
            result.atlas_page_assets.clear();
            return result;
        }
    }

    return result;
}

UiRenderSubmitResult submit_ui_renderer_submission(IRenderer& renderer, const ui::RendererSubmission& submission,
                                                   UiRenderSubmitDesc desc) {
    UiRenderSubmitResult result;
    const auto text_payload = desc.glyph_atlas == nullptr
                                  ? ui::build_text_adapter_payload(submission)
                                  : ui::build_text_adapter_payload(submission, desc.text_layout_policy);
    const auto image_payload = ui::build_image_adapter_payload(submission);
    const auto accessibility_payload = ui::build_accessibility_payload(submission);

    result.text_runs_available = submission.text_runs.size();
    result.text_adapter_rows_available = text_payload.rows.size();
    result.image_placeholders_available = image_payload.rows.size();
    result.accessibility_nodes_available = accessibility_payload.nodes.size();
    result.adapter_diagnostics_available =
        text_payload.diagnostics.size() + image_payload.diagnostics.size() + accessibility_payload.diagnostics.size();

    for (const auto& box : submission.boxes) {
        if (box.background_token.empty()) {
            continue;
        }
        if (desc.theme != nullptr && desc.theme->find(box.background_token) != nullptr) {
            ++result.theme_colors_resolved;
        }
        renderer.draw_sprite(make_ui_box_sprite_command(box, desc));
        ++result.boxes_submitted;
    }

    if (desc.glyph_atlas != nullptr) {
        for (const auto& row : text_payload.rows) {
            for (const auto& line : row.lines) {
                for (const auto& glyph : line.glyphs) {
                    ++result.text_glyphs_available;
                    if (!is_positive_ui_rect(glyph.bounds)) {
                        ++result.text_glyphs_missing;
                        continue;
                    }
                    const auto* binding = resolve_ui_text_glyph_binding(row, glyph, desc);
                    if (binding == nullptr) {
                        ++result.text_glyphs_missing;
                        continue;
                    }
                    renderer.draw_sprite(make_ui_text_glyph_sprite_command(glyph, *binding));
                    ++result.text_glyphs_resolved;
                    ++result.text_glyph_sprites_submitted;
                }
            }
        }
    }

    for (const auto& image : image_payload.rows) {
        if (!is_positive_ui_rect(image.bounds) || (image.resource_id.empty() && image.asset_uri.empty())) {
            continue;
        }
        const auto* binding = resolve_ui_image_binding(image, desc);
        if (binding == nullptr) {
            ++result.image_resources_missing;
            continue;
        }
        renderer.draw_sprite(make_ui_image_sprite_command(image, *binding));
        ++result.image_resources_resolved;
        ++result.image_sprites_submitted;
    }

    return result;
}

} // namespace mirakana
