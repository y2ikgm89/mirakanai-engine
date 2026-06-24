// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace mirakana {

namespace {

constexpr std::uint64_t kAtlasHandoffFnvOffset{14695981039346656037ULL};
constexpr std::uint64_t kAtlasHandoffFnvPrime{1099511628211ULL};

[[nodiscard]] bool is_positive_ui_rect(ui::Rect rect) noexcept {
    return ui::is_valid_rect(rect) && rect.width > 0.0F && rect.height > 0.0F;
}

void append_atlas_handoff_diagnostic(std::vector<UiRendererAtlasHandoffDiagnostic>& diagnostics,
                                     UiRendererAtlasHandoffDiagnosticCode code, std::string message) {
    diagnostics.push_back(UiRendererAtlasHandoffDiagnostic{
        .code = code,
        .message = std::move(message),
    });
}

void hash_atlas_handoff_byte(std::uint64_t& hash, std::uint8_t value) noexcept {
    hash ^= value;
    hash *= kAtlasHandoffFnvPrime;
}

void hash_atlas_handoff_bool(std::uint64_t& hash, bool value) noexcept {
    hash_atlas_handoff_byte(hash, value ? 1U : 0U);
}

void hash_atlas_handoff_u64(std::uint64_t& hash, std::uint64_t value) noexcept {
    for (std::size_t shift = 0U; shift < 64U; shift += 8U) {
        hash_atlas_handoff_byte(hash, static_cast<std::uint8_t>((value >> shift) & 0xffU));
    }
}

void hash_atlas_handoff_string(std::uint64_t& hash, std::string_view value) noexcept {
    for (const unsigned char character : value) {
        hash_atlas_handoff_byte(hash, character);
    }
    hash_atlas_handoff_byte(hash, 0xffU);
}

[[nodiscard]] std::uint64_t compute_atlas_handoff_replay_hash(const UiRendererAtlasHandoffRequest& request) noexcept {
    auto hash = kAtlasHandoffFnvOffset;
    hash_atlas_handoff_string(hash, request.id);
    hash_atlas_handoff_u64(hash, request.image_atlas_page_count);
    hash_atlas_handoff_u64(hash, request.image_atlas_binding_count);
    hash_atlas_handoff_u64(hash, request.glyph_atlas_page_count);
    hash_atlas_handoff_u64(hash, request.glyph_atlas_binding_count);
    hash_atlas_handoff_u64(hash, request.submit_result.text_glyphs_available);
    hash_atlas_handoff_u64(hash, request.submit_result.text_glyphs_resolved);
    hash_atlas_handoff_u64(hash, request.submit_result.text_glyphs_missing);
    hash_atlas_handoff_u64(hash, request.submit_result.text_glyph_sprites_submitted);
    hash_atlas_handoff_u64(hash, request.submit_result.image_placeholders_available);
    hash_atlas_handoff_u64(hash, request.submit_result.image_resources_resolved);
    hash_atlas_handoff_u64(hash, request.submit_result.image_resources_missing);
    hash_atlas_handoff_u64(hash, request.submit_result.image_sprites_submitted);
    hash_atlas_handoff_u64(hash, request.renderer_sprites_submitted);
    hash_atlas_handoff_u64(hash, request.max_image_bindings);
    hash_atlas_handoff_u64(hash, request.max_glyph_bindings);
    hash_atlas_handoff_bool(hash, request.image_atlas_metadata_built);
    hash_atlas_handoff_bool(hash, request.glyph_atlas_metadata_built);
    hash_atlas_handoff_bool(hash, request.atlas_eviction_diagnostics_reviewed);
    hash_atlas_handoff_bool(hash, request.texture_upload_handoff_reviewed);
    hash_atlas_handoff_bool(hash, request.texture_upload_execution_reviewed);
    hash_atlas_handoff_bool(hash, request.texture_upload_execution_ready);
    hash_atlas_handoff_bool(hash, request.renderer_submission_counters_reviewed);
    hash_atlas_handoff_bool(hash, request.selected_package_counter_evidence);
    hash_atlas_handoff_bool(hash, request.requested_renderer_texture_upload_api);
    hash_atlas_handoff_bool(hash, request.requested_public_native_handle);
    hash_atlas_handoff_bool(hash, request.claims_broad_text_rendering);
    hash_atlas_handoff_bool(hash, request.claims_broad_image_rendering);
    hash_atlas_handoff_bool(hash, request.invoked_source_image_decode);
    hash_atlas_handoff_bool(hash, request.invoked_live_glyph_atlas_generation);
    hash_atlas_handoff_bool(hash, request.invoked_renderer_upload);
    hash_atlas_handoff_u64(hash, request.seed);
    return hash == 0U ? 1U : hash;
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

bool UiRendererAtlasHandoffPlan::ready() const noexcept {
    return status == UiRendererAtlasHandoffStatus::ready && selected_package_evidence_ready && diagnostics.empty();
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

UiRendererAtlasHandoffPlan review_ui_renderer_atlas_handoff(const UiRendererAtlasHandoffRequest& request) {
    UiRendererAtlasHandoffPlan plan;
    plan.replay_hash = compute_atlas_handoff_replay_hash(request);
    plan.image_atlas_pages = request.image_atlas_page_count;
    plan.image_atlas_bindings = request.image_atlas_binding_count;
    plan.glyph_atlas_pages = request.glyph_atlas_page_count;
    plan.glyph_atlas_bindings = request.glyph_atlas_binding_count;
    plan.atlas_placement_rows = request.image_atlas_binding_count + request.glyph_atlas_binding_count;
    plan.atlas_budget_rows = (request.max_image_bindings > 0U ? 1U : 0U) + (request.max_glyph_bindings > 0U ? 1U : 0U);
    plan.atlas_eviction_diagnostic_rows = request.atlas_eviction_diagnostics_reviewed ? 1U : 0U;
    plan.texture_upload_handoff_rows = request.texture_upload_handoff_reviewed ? 1U : 0U;
    plan.texture_upload_execution_rows =
        request.texture_upload_execution_reviewed && request.texture_upload_execution_ready ? 1U : 0U;
    plan.texture_upload_execution_ready = request.texture_upload_execution_ready;
    plan.renderer_submission_counter_rows = request.renderer_submission_counters_reviewed ? 1U : 0U;
    plan.text_glyphs_available = request.submit_result.text_glyphs_available;
    plan.text_glyphs_resolved = request.submit_result.text_glyphs_resolved;
    plan.text_glyphs_missing = request.submit_result.text_glyphs_missing;
    plan.text_glyph_sprites_submitted = request.submit_result.text_glyph_sprites_submitted;
    plan.image_placeholders_available = request.submit_result.image_placeholders_available;
    plan.image_resources_resolved = request.submit_result.image_resources_resolved;
    plan.image_resources_missing = request.submit_result.image_resources_missing;
    plan.image_sprites_submitted = request.submit_result.image_sprites_submitted;
    plan.renderer_sprites_submitted = request.renderer_sprites_submitted;
    plan.requested_renderer_texture_upload_api = request.requested_renderer_texture_upload_api;
    plan.requested_public_native_handle = request.requested_public_native_handle;
    plan.invoked_source_image_decode = request.invoked_source_image_decode;
    plan.invoked_live_glyph_atlas_generation = request.invoked_live_glyph_atlas_generation;
    plan.invoked_renderer_upload = request.invoked_renderer_upload;

    if (request.id.empty()) {
        append_atlas_handoff_diagnostic(plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::missing_request_id,
                                        "runtime UI renderer atlas handoff evidence requires a request id");
    }
    if (!request.image_atlas_metadata_built || request.image_atlas_page_count == 0U ||
        request.image_atlas_binding_count == 0U) {
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::missing_image_atlas_metadata,
            "runtime UI renderer atlas handoff requires cooked image atlas metadata pages and bindings");
    }
    if (!request.glyph_atlas_metadata_built || request.glyph_atlas_page_count == 0U ||
        request.glyph_atlas_binding_count == 0U) {
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::missing_glyph_atlas_metadata,
            "runtime UI renderer atlas handoff requires cooked glyph atlas metadata pages and bindings");
    }
    if (plan.atlas_placement_rows == 0U) {
        append_atlas_handoff_diagnostic(plan.diagnostics,
                                        UiRendererAtlasHandoffDiagnosticCode::missing_atlas_placement_rows,
                                        "runtime UI renderer atlas handoff requires atlas placement rows");
    }
    if (plan.atlas_budget_rows != 2U) {
        append_atlas_handoff_diagnostic(plan.diagnostics,
                                        UiRendererAtlasHandoffDiagnosticCode::missing_atlas_budget_rows,
                                        "runtime UI renderer atlas handoff requires image and glyph atlas budgets");
    }
    if ((request.max_image_bindings > 0U && request.image_atlas_binding_count > request.max_image_bindings) ||
        (request.max_glyph_bindings > 0U && request.glyph_atlas_binding_count > request.max_glyph_bindings)) {
        append_atlas_handoff_diagnostic(plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::atlas_budget_exceeded,
                                        "runtime UI renderer atlas handoff bindings exceed reviewed atlas budgets");
    }
    if (!request.atlas_eviction_diagnostics_reviewed) {
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::missing_atlas_eviction_diagnostics,
            "runtime UI renderer atlas handoff requires reviewed eviction or no-eviction diagnostics");
    }
    if (!request.texture_upload_handoff_reviewed) {
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::missing_texture_upload_handoff,
            "runtime UI renderer atlas handoff requires texture upload handoff evidence rows");
    }
    if (!request.texture_upload_execution_reviewed || !request.texture_upload_execution_ready) {
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::missing_texture_upload_execution,
            "runtime UI renderer atlas handoff requires selected renderer texture upload execution evidence");
    }
    if (!request.renderer_submission_counters_reviewed) {
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::missing_renderer_submission_counters,
            "runtime UI renderer atlas handoff requires renderer-owned submission counters");
    }
    if (!request.selected_package_counter_evidence) {
        append_atlas_handoff_diagnostic(plan.diagnostics,
                                        UiRendererAtlasHandoffDiagnosticCode::missing_selected_package_counter_evidence,
                                        "runtime UI renderer atlas handoff requires selected package counter evidence");
    }
    if (request.submit_result.text_glyphs_missing > 0U ||
        request.submit_result.text_glyphs_resolved != request.submit_result.text_glyphs_available) {
        append_atlas_handoff_diagnostic(plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::unresolved_text_glyphs,
                                        "runtime UI renderer atlas handoff requires all planned glyphs to resolve");
    }
    if (request.submit_result.image_resources_missing > 0U ||
        request.submit_result.image_resources_resolved != request.submit_result.image_placeholders_available) {
        append_atlas_handoff_diagnostic(plan.diagnostics,
                                        UiRendererAtlasHandoffDiagnosticCode::unresolved_image_resources,
                                        "runtime UI renderer atlas handoff requires all image placeholders to resolve");
    }
    if (request.renderer_sprites_submitted <
        request.submit_result.text_glyph_sprites_submitted + request.submit_result.image_sprites_submitted) {
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::renderer_submission_counter_mismatch,
            "runtime UI renderer atlas handoff requires renderer sprite counters to include atlas submissions");
    }
    if (request.requested_renderer_texture_upload_api) {
        ++plan.unsupported_claim_rows;
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::unsupported_renderer_texture_upload_api,
            "runtime UI renderer atlas handoff does not expose renderer texture upload APIs to gameplay UI");
    }
    if (request.requested_public_native_handle) {
        ++plan.unsupported_claim_rows;
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::unsupported_public_native_handle,
            "runtime UI renderer atlas handoff does not expose public native, GPU, or RHI handles");
    }
    if (request.claims_broad_text_rendering) {
        ++plan.unsupported_claim_rows;
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::unsupported_broad_text_rendering_claim,
            "runtime UI renderer atlas handoff is not broad production text rendering evidence");
    }
    if (request.claims_broad_image_rendering) {
        ++plan.unsupported_claim_rows;
        append_atlas_handoff_diagnostic(
            plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::unsupported_broad_image_rendering_claim,
            "runtime UI renderer atlas handoff is not broad production image rendering evidence");
    }
    if (request.invoked_source_image_decode) {
        ++plan.side_effect_rows;
        append_atlas_handoff_diagnostic(plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::side_effect_invocation,
                                        "runtime UI renderer atlas handoff must not invoke source image decoding");
    }
    if (request.invoked_live_glyph_atlas_generation) {
        ++plan.side_effect_rows;
        append_atlas_handoff_diagnostic(plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::side_effect_invocation,
                                        "runtime UI renderer atlas handoff must not generate live glyph atlases");
    }
    if (request.invoked_renderer_upload) {
        ++plan.side_effect_rows;
        append_atlas_handoff_diagnostic(plan.diagnostics, UiRendererAtlasHandoffDiagnosticCode::side_effect_invocation,
                                        "runtime UI renderer atlas handoff must not invoke renderer upload APIs");
    }

    plan.reviewed = request.image_atlas_metadata_built && request.glyph_atlas_metadata_built &&
                    request.atlas_eviction_diagnostics_reviewed && request.texture_upload_handoff_reviewed &&
                    request.texture_upload_execution_reviewed && request.texture_upload_execution_ready &&
                    request.renderer_submission_counters_reviewed && request.selected_package_counter_evidence;
    plan.selected_package_evidence_ready = plan.reviewed && plan.diagnostics.empty();
    plan.status = plan.selected_package_evidence_ready ? UiRendererAtlasHandoffStatus::ready
                                                       : UiRendererAtlasHandoffStatus::invalid_request;
    return plan;
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
