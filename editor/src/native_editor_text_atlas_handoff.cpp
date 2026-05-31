// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_text_atlas_handoff.hpp"

#include "native_editor_text_font_adapters.hpp"

#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

void append_row(NativeEditorTextAtlasHandoffEvidence& evidence, NativeEditorTextAtlasHandoffEvidenceRow row) {
    if (row.adapter_invoked) {
        ++evidence.adapter_invoked_rows;
    }
    if (row.glyphs_ready) {
        ++evidence.glyphs_ready_rows;
    }
    if (row.host_evidence_required && !row.host_evidence_available) {
        ++evidence.host_gated_rows;
    }
    if (row.unsupported) {
        ++evidence.unsupported_rows;
    }
    evidence.native_handles_exposed = evidence.native_handles_exposed || row.native_handles_public;
    evidence.rows.push_back(std::move(row));
}

struct RasterGlyphKey {
    std::string font_family;
    std::uint32_t glyph{0};
};

[[nodiscard]] bool contains_glyph(const std::vector<RasterGlyphKey>& glyphs, const RasterGlyphKey& glyph) {
    return std::ranges::any_of(glyphs, [&glyph](const RasterGlyphKey& candidate) {
        return candidate.font_family == glyph.font_family && candidate.glyph == glyph.glyph;
    });
}

[[nodiscard]] ui::TextLayoutRequest make_text_request(const NativeEditorTextAtlasHandoffDesc& desc) {
    return ui::TextLayoutRequest{
        .text = desc.text,
        .font_family = desc.font_family,
        .direction = ui::TextDirection::left_to_right,
        .wrap = ui::TextWrapMode::clip,
        .max_width = desc.max_width,
    };
}

[[nodiscard]] NativeEditorTextAtlasHandoffEvidence make_host_unavailable_evidence() {
    NativeEditorTextAtlasHandoffEvidence evidence;
    evidence.status = "host_unavailable";
    append_row(evidence, NativeEditorTextAtlasHandoffEvidenceRow{
                             .id = "editor.text_atlas.directwrite.host_gate",
                             .status = "host_evidence_required",
                             .host_evidence_required = true,
                         });
    append_row(evidence, NativeEditorTextAtlasHandoffEvidenceRow{
                             .id = "editor.text_atlas.gpu_upload.unsupported",
                             .status = "unsupported",
                             .unsupported = true,
                         });
    return evidence;
}

void append_adapter_rows(NativeEditorTextAtlasHandoffEvidence& evidence) {
    evidence.text_shaping_adapter_invoked = true;
    evidence.font_rasterizer_adapter_invoked = true;

    append_row(evidence, NativeEditorTextAtlasHandoffEvidenceRow{
                             .id = "editor.text_atlas.adapter.text_shaping",
                             .status = "adapter_invoked",
                             .adapter_invoked = true,
                         });
    append_row(evidence, NativeEditorTextAtlasHandoffEvidenceRow{
                             .id = "editor.text_atlas.adapter.font_rasterizer",
                             .status = "adapter_invoked",
                             .adapter_invoked = true,
                         });
}

void append_gate_rows(NativeEditorTextAtlasHandoffEvidence& evidence) {
    append_row(evidence, NativeEditorTextAtlasHandoffEvidenceRow{
                             .id = "editor.text_atlas.direct2d_atlas.host_gate",
                             .status = "host_evidence_required",
                             .host_evidence_required = true,
                         });
    append_row(evidence, NativeEditorTextAtlasHandoffEvidenceRow{
                             .id = "editor.text_atlas.gpu_upload.unsupported",
                             .status = "unsupported",
                             .unsupported = true,
                         });
}

} // namespace

NativeEditorTextAtlasHandoffEvidence
make_native_editor_directwrite_text_atlas_handoff_evidence(const NativeEditorTextAtlasHandoffDesc& desc) {
#if defined(_WIN32)
    auto text_shaping = make_native_editor_directwrite_text_shaping_adapter();
    auto font_rasterizer = make_native_editor_directwrite_font_rasterizer_adapter();
    if (text_shaping == nullptr || font_rasterizer == nullptr) {
        return make_host_unavailable_evidence();
    }

    NativeEditorTextAtlasHandoffEvidence evidence;
    append_adapter_rows(evidence);

    const auto shaped = ui::shape_text_run(*text_shaping, make_text_request(desc));
    std::vector<RasterGlyphKey> unique_glyphs;
    for (const auto& run : shaped.runs) {
        for (const auto& fallback : run.fallback_rows) {
            ++evidence.fallback_rows;
            evidence.fallback_used = evidence.fallback_used || fallback.fallback_used;
            if (fallback.fallback_used) {
                ++evidence.fallback_used_rows;
            } else {
                ++evidence.fallback_not_used_rows;
            }
        }
        for (const auto& glyph : run.glyphs) {
            ++evidence.shaped_glyph_count;
            const RasterGlyphKey raster_glyph{.font_family = glyph.font_family, .glyph = glyph.glyph};
            if (!contains_glyph(unique_glyphs, raster_glyph)) {
                unique_glyphs.push_back(raster_glyph);
            }
        }
    }

    for (const auto& glyph : unique_glyphs) {
        const auto rasterized = ui::rasterize_font_glyph(*font_rasterizer, ui::FontRasterizationRequest{
                                                                               .font_family = glyph.font_family,
                                                                               .glyph = glyph.glyph,
                                                                               .pixel_size = desc.pixel_size,
                                                                           });
        if (rasterized.succeeded()) {
            ++evidence.rasterized_glyph_count;
            const auto& bitmap = rasterized.allocation->bitmap;
            if (bitmap.width == 0U || bitmap.height == 0U || bitmap.pixels.empty()) {
                ++evidence.zero_ink_glyph_count;
            }
        }
    }

    evidence.glyphs_ready = shaped.succeeded() && evidence.shaped_glyph_count > 0U &&
                            evidence.rasterized_glyph_count == unique_glyphs.size();
    append_row(evidence, NativeEditorTextAtlasHandoffEvidenceRow{
                             .id = "editor.text_atlas.glyphs.ready",
                             .status = evidence.glyphs_ready ? "ready" : "blocked",
                             .glyphs_ready = evidence.glyphs_ready,
                         });
    append_row(evidence, NativeEditorTextAtlasHandoffEvidenceRow{
                             .id = "editor.text_atlas.font_fallback",
                             .status = evidence.fallback_used ? "used" : "not_used",
                             .fallback_used = evidence.fallback_used,
                         });
    append_gate_rows(evidence);

    evidence.atlas_handoff_ready = false;
    evidence.status =
        evidence.glyphs_ready ? "glyphs_ready_atlas_handoff_host_gated" : "glyphs_pending_atlas_handoff_host_gated";
    return evidence;
#else
    (void)desc;
    return make_host_unavailable_evidence();
#endif
}

} // namespace mirakana::editor
