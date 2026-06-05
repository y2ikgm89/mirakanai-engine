// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_text_font_adapters.hpp"

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <dwrite.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

using Microsoft::WRL::ComPtr;

[[nodiscard]] std::wstring utf8_to_wide(std::string_view text) {
    if (text.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("native editor DirectWrite text is too large");
    }

    const std::string narrow{text};
    const auto byte_count = static_cast<int>(narrow.size());
    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, narrow.data(), byte_count, nullptr, 0);
    if (required <= 0) {
        throw std::invalid_argument("native editor DirectWrite text must be strict UTF-8");
    }

    std::wstring wide(static_cast<std::size_t>(required), L'\0');
    const int converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, narrow.data(), byte_count, wide.data(),
                                              static_cast<int>(wide.size()));
    if (converted != required) {
        throw std::runtime_error("native editor DirectWrite text conversion failed");
    }
    return wide;
}

[[nodiscard]] std::size_t utf8_sequence_length(unsigned char lead) noexcept {
    if ((lead & 0x80U) == 0U) {
        return 1U;
    }
    if ((lead & 0xE0U) == 0xC0U) {
        return 2U;
    }
    if ((lead & 0xF0U) == 0xE0U) {
        return 3U;
    }
    if ((lead & 0xF8U) == 0xF0U) {
        return 4U;
    }
    return 1U;
}

[[nodiscard]] std::uint32_t utf8_codepoint(std::string_view text, std::size_t byte_offset,
                                           std::size_t sequence_length) noexcept {
    const auto byte = [text](std::size_t index) {
        return static_cast<std::uint32_t>(static_cast<unsigned char>(text[index]));
    };

    switch (sequence_length) {
    case 1U:
        return byte(byte_offset);
    case 2U:
        return ((byte(byte_offset) & 0x1FU) << 6U) | (byte(byte_offset + 1U) & 0x3FU);
    case 3U:
        return ((byte(byte_offset) & 0x0FU) << 12U) | ((byte(byte_offset + 1U) & 0x3FU) << 6U) |
               (byte(byte_offset + 2U) & 0x3FU);
    case 4U:
        return ((byte(byte_offset) & 0x07U) << 18U) | ((byte(byte_offset + 1U) & 0x3FU) << 12U) |
               ((byte(byte_offset + 2U) & 0x3FU) << 6U) | (byte(byte_offset + 3U) & 0x3FU);
    default:
        return byte(byte_offset);
    }
}

[[nodiscard]] std::vector<std::size_t> build_utf16_to_utf8_byte_offsets(std::string_view text,
                                                                        std::wstring_view wide_text) {
    std::vector<std::size_t> offsets(wide_text.size() + 1U, text.size());
    std::size_t byte_offset = 0U;
    std::size_t utf16_offset = 0U;

    while (byte_offset < text.size() && utf16_offset < wide_text.size()) {
        const auto lead = static_cast<unsigned char>(text[byte_offset]);
        const std::size_t sequence_length = std::min(utf8_sequence_length(lead), text.size() - byte_offset);
        const std::uint32_t codepoint = utf8_codepoint(text, byte_offset, sequence_length);
        const std::size_t utf16_units = codepoint > 0xFFFFU ? 2U : 1U;
        for (std::size_t unit = 0U; unit < utf16_units && utf16_offset + unit < wide_text.size(); ++unit) {
            offsets[utf16_offset + unit] = byte_offset;
        }
        byte_offset += sequence_length;
        utf16_offset += utf16_units;
    }

    offsets.back() = text.size();
    return offsets;
}

[[nodiscard]] std::size_t utf8_byte_offset_for_utf16_index(const std::vector<std::size_t>& offsets,
                                                           std::uint32_t utf16_index) noexcept {
    if (offsets.size() <= 1U) {
        return 0U;
    }
    const auto last_scalar_index = static_cast<std::uint32_t>(offsets.size() - 2U);
    return offsets[std::min(utf16_index, last_scalar_index)];
}

[[nodiscard]] ComPtr<IDWriteFactory> make_directwrite_factory() {
    ComPtr<IDWriteFactory> factory;
    const HRESULT result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                               reinterpret_cast<IUnknown**>(factory.GetAddressOf()));
    if (FAILED(result)) {
        return {};
    }
    return factory;
}

[[nodiscard]] float positive_or_default(float value, float fallback) noexcept {
    return std::isfinite(value) && value > 0.0F ? value : fallback;
}

[[nodiscard]] ui::TextDirection resolved_direction(ui::TextDirection direction) noexcept {
    return direction == ui::TextDirection::right_to_left ? ui::TextDirection::right_to_left
                                                         : ui::TextDirection::left_to_right;
}

[[nodiscard]] DWRITE_READING_DIRECTION dwrite_reading_direction(ui::TextDirection direction) noexcept {
    return direction == ui::TextDirection::right_to_left ? DWRITE_READING_DIRECTION_RIGHT_TO_LEFT
                                                         : DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
}

[[nodiscard]] DWRITE_WORD_WRAPPING dwrite_word_wrapping(ui::TextWrapMode wrap) noexcept {
    return wrap == ui::TextWrapMode::wrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP;
}

[[nodiscard]] std::string script_tag_for_text(std::string_view text) {
    const bool ascii = std::ranges::all_of(text, [](char value) { return static_cast<unsigned char>(value) < 0x80U; });
    return ascii ? "Latn" : "Zyyy";
}

class DirectWriteGlyphRunCollector final : public IDWriteTextRenderer {
  public:
    DirectWriteGlyphRunCollector(ui::TextLayoutRun& run, const std::vector<std::size_t>& utf16_to_utf8_offsets,
                                 std::string requested_font_family)
        : run_(run), utf16_to_utf8_offsets_(utf16_to_utf8_offsets),
          requested_font_family_(std::move(requested_font_family)) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (object == nullptr) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWritePixelSnapping)) {
            *object = static_cast<IDWritePixelSnapping*>(this);
        } else if (riid == __uuidof(IDWriteTextRenderer)) {
            *object = static_cast<IDWriteTextRenderer*>(this);
        } else {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return ++ref_count_;
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG count = --ref_count_;
        return count;
    }

    HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void*, BOOL* is_disabled) override {
        if (is_disabled == nullptr) {
            return E_POINTER;
        }
        *is_disabled = FALSE;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetCurrentTransform(void*, DWRITE_MATRIX* transform) override {
        if (transform == nullptr) {
            return E_POINTER;
        }
        *transform = DWRITE_MATRIX{.m11 = 1.0F, .m12 = 0.0F, .m21 = 0.0F, .m22 = 1.0F, .dx = 0.0F, .dy = 0.0F};
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void*, FLOAT* pixels_per_dip) override {
        if (pixels_per_dip == nullptr) {
            return E_POINTER;
        }
        *pixels_per_dip = 1.0F;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawGlyphRun(void*, FLOAT, FLOAT, DWRITE_MEASURING_MODE,
                                           const DWRITE_GLYPH_RUN* glyph_run,
                                           const DWRITE_GLYPH_RUN_DESCRIPTION* glyph_run_description,
                                           IUnknown*) override {
        if (glyph_run == nullptr || glyph_run->glyphIndices == nullptr || glyph_run_description == nullptr) {
            return E_INVALIDARG;
        }

        for (std::uint32_t index = 0U; index < glyph_run->glyphCount; ++index) {
            std::uint32_t text_index = 0U;
            if (glyph_run_description->clusterMap != nullptr && glyph_run_description->stringLength > 0U) {
                for (std::uint32_t candidate = 0U; candidate < glyph_run_description->stringLength; ++candidate) {
                    if (glyph_run_description->clusterMap[candidate] <= index) {
                        text_index = candidate;
                    }
                }
            }
            const auto cluster_utf16_index = glyph_run_description->textPosition + text_index;
            const float advance =
                glyph_run->glyphAdvances == nullptr ? glyph_run->fontEmSize : glyph_run->glyphAdvances[index];
            const DWRITE_GLYPH_OFFSET offset = glyph_run->glyphOffsets == nullptr
                                                   ? DWRITE_GLYPH_OFFSET{.advanceOffset = 0.0F, .ascenderOffset = 0.0F}
                                                   : glyph_run->glyphOffsets[index];
            run_.glyphs.push_back(ui::TextShapedGlyph{
                .glyph = glyph_run->glyphIndices[index],
                .cluster_byte_offset = utf8_byte_offset_for_utf16_index(utf16_to_utf8_offsets_, cluster_utf16_index),
                .advance_x = advance,
                .advance_y = 0.0F,
                .offset_x = offset.advanceOffset,
                .offset_y = offset.ascenderOffset,
                .font_family = requested_font_family_,
            });
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawUnderline(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawStrikethrough(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*) override {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL,
                                               IUnknown*) override {
        return S_OK;
    }

  private:
    std::atomic<ULONG> ref_count_{1U};
    ui::TextLayoutRun& run_;
    const std::vector<std::size_t>& utf16_to_utf8_offsets_;
    std::string requested_font_family_;
};

class NativeEditorDirectWriteTextShapingAdapter final : public ui::ITextShapingAdapter {
  public:
    [[nodiscard]] std::vector<ui::TextLayoutRun> shape_text(const ui::TextLayoutRequest& request) override {
        auto factory = make_directwrite_factory();
        if (factory == nullptr) {
            return {};
        }

        const std::wstring wide_text = utf8_to_wide(request.text);
        const std::wstring font_family = utf8_to_wide(request.font_family);
        const float layout_width = positive_or_default(request.max_width, 4096.0F);
        constexpr float font_size = 14.0F;
        constexpr float layout_height = 4096.0F;

        ComPtr<IDWriteTextFormat> text_format;
        HRESULT result = factory->CreateTextFormat(font_family.c_str(), nullptr, DWRITE_FONT_WEIGHT_REGULAR,
                                                   DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, font_size, L"",
                                                   text_format.GetAddressOf());
        if (FAILED(result) || text_format == nullptr) {
            return {};
        }

        result = text_format->SetReadingDirection(dwrite_reading_direction(request.direction));
        if (FAILED(result)) {
            return {};
        }
        result = text_format->SetWordWrapping(dwrite_word_wrapping(request.wrap));
        if (FAILED(result)) {
            return {};
        }

        ComPtr<IDWriteTextLayout> text_layout;
        result = factory->CreateTextLayout(wide_text.c_str(), static_cast<UINT32>(wide_text.size()), text_format.Get(),
                                           layout_width, layout_height, text_layout.GetAddressOf());
        if (FAILED(result) || text_layout == nullptr) {
            return {};
        }

        DWRITE_TEXT_METRICS metrics{};
        result = text_layout->GetMetrics(&metrics);
        if (FAILED(result)) {
            return {};
        }

        ui::TextLayoutRun run;
        run.text = request.text;
        run.bounds = ui::Rect{.x = 0.0F,
                              .y = 0.0F,
                              .width = positive_or_default(metrics.widthIncludingTrailingWhitespace, 1.0F),
                              .height = positive_or_default(metrics.height, font_size)};
        run.segments.push_back(ui::TextShapingSegmentEvidence{
            .start_byte = 0U,
            .end_byte = request.text.size(),
            .direction = resolved_direction(request.direction),
            .script_tag = script_tag_for_text(request.text),
            .language_tag = "und",
        });
        run.boundaries.push_back(ui::TextBoundaryEvidence{
            .kind = ui::TextBoundaryEvidenceKind::grapheme_cluster,
            .start_byte = 0U,
            .end_byte = request.text.size(),
        });
        run.boundaries.push_back(ui::TextBoundaryEvidence{
            .kind = ui::TextBoundaryEvidenceKind::word,
            .start_byte = 0U,
            .end_byte = request.text.size(),
        });
        run.boundaries.push_back(ui::TextBoundaryEvidence{
            .kind = ui::TextBoundaryEvidenceKind::line_break,
            .start_byte = 0U,
            .end_byte = request.text.size(),
        });
        run.boundaries.push_back(ui::TextBoundaryEvidence{
            .kind = ui::TextBoundaryEvidenceKind::bidi_run,
            .start_byte = 0U,
            .end_byte = request.text.size(),
        });
        run.fallback_rows.push_back(ui::TextFontFallbackEvidence{
            .cluster_byte_offset = 0U,
            .requested_font_family = request.font_family,
            .resolved_font_family = request.font_family,
            .fallback_used = false,
        });

        const auto utf16_offsets = build_utf16_to_utf8_byte_offsets(request.text, wide_text);
        DirectWriteGlyphRunCollector collector{run, utf16_offsets, request.font_family};
        result = text_layout->Draw(nullptr, &collector, 0.0F, 0.0F);
        if (FAILED(result) || run.glyphs.empty()) {
            return {};
        }

        return {std::move(run)};
    }
};

[[nodiscard]] ComPtr<IDWriteFontFace> resolve_font_face(IDWriteFactory& factory, const std::wstring& font_family) {
    ComPtr<IDWriteFontCollection> collection;
    HRESULT result = factory.GetSystemFontCollection(collection.GetAddressOf(), FALSE);
    if (FAILED(result) || collection == nullptr) {
        return {};
    }

    UINT32 family_index = 0U;
    BOOL family_exists = FALSE;
    result = collection->FindFamilyName(font_family.c_str(), &family_index, &family_exists);
    if (FAILED(result)) {
        return {};
    }
    if (family_exists == FALSE) {
        result = collection->FindFamilyName(L"Segoe UI", &family_index, &family_exists);
        if (FAILED(result) || family_exists == FALSE) {
            family_index = 0U;
        }
    }

    ComPtr<IDWriteFontFamily> family;
    result = collection->GetFontFamily(family_index, family.GetAddressOf());
    if (FAILED(result) || family == nullptr) {
        return {};
    }

    ComPtr<IDWriteFont> font;
    result = family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL,
                                          DWRITE_FONT_STYLE_NORMAL, font.GetAddressOf());
    if (FAILED(result) || font == nullptr) {
        return {};
    }

    ComPtr<IDWriteFontFace> font_face;
    result = font->CreateFontFace(font_face.GetAddressOf());
    if (FAILED(result)) {
        return {};
    }
    return font_face;
}

[[nodiscard]] float design_units_to_pixels(float value, float pixel_size, std::uint16_t design_units_per_em) noexcept {
    if (design_units_per_em == 0U) {
        return 0.0F;
    }
    return value * pixel_size / static_cast<float>(design_units_per_em);
}

[[nodiscard]] ui::GlyphRasterMetrics glyph_metrics_to_ui_metrics(const DWRITE_GLYPH_METRICS& glyph_metrics,
                                                                 const DWRITE_FONT_METRICS& font_metrics,
                                                                 float pixel_size,
                                                                 const ui::GlyphRasterBitmap& bitmap) noexcept {
    const float advance_width = design_units_to_pixels(static_cast<float>(glyph_metrics.advanceWidth), pixel_size,
                                                       font_metrics.designUnitsPerEm);
    const float advance_height = design_units_to_pixels(static_cast<float>(glyph_metrics.advanceHeight), pixel_size,
                                                        font_metrics.designUnitsPerEm);
    const float ink_width_design = std::max(0.0F, static_cast<float>(glyph_metrics.advanceWidth) -
                                                      static_cast<float>(glyph_metrics.leftSideBearing) -
                                                      static_cast<float>(glyph_metrics.rightSideBearing));
    const float ink_height_design = std::max(0.0F, static_cast<float>(glyph_metrics.advanceHeight) -
                                                       static_cast<float>(glyph_metrics.topSideBearing) -
                                                       static_cast<float>(glyph_metrics.bottomSideBearing));

    ui::GlyphRasterMetrics metrics{
        .width = design_units_to_pixels(ink_width_design, pixel_size, font_metrics.designUnitsPerEm),
        .height = design_units_to_pixels(ink_height_design, pixel_size, font_metrics.designUnitsPerEm),
        .bearing_x = design_units_to_pixels(static_cast<float>(glyph_metrics.leftSideBearing), pixel_size,
                                            font_metrics.designUnitsPerEm),
        .bearing_y = design_units_to_pixels(static_cast<float>(glyph_metrics.topSideBearing), pixel_size,
                                            font_metrics.designUnitsPerEm),
        .advance_x = std::max(0.0F, advance_width),
        .advance_y = std::max(0.0F, advance_height),
    };
    if (bitmap.width > 0U && bitmap.height > 0U) {
        metrics.width = std::max(metrics.width, static_cast<float>(bitmap.width));
        metrics.height = std::max(metrics.height, static_cast<float>(bitmap.height));
    }
    return metrics;
}

class NativeEditorDirectWriteFontRasterizerAdapter final : public ui::IFontRasterizerAdapter {
  public:
    [[nodiscard]] ui::GlyphAtlasAllocation rasterize_glyph(const ui::FontRasterizationRequest& request) override {
        ui::GlyphAtlasAllocation allocation;
        allocation.glyph = request.glyph;

        if (request.glyph > std::numeric_limits<UINT16>::max()) {
            return allocation;
        }

        auto factory = make_directwrite_factory();
        if (factory == nullptr) {
            return allocation;
        }

        const std::wstring font_family = utf8_to_wide(request.font_family);
        auto font_face = resolve_font_face(*factory.Get(), font_family);
        if (font_face == nullptr) {
            return allocation;
        }

        const auto glyph_index = static_cast<UINT16>(request.glyph);
        DWRITE_GLYPH_METRICS glyph_metrics{};
        HRESULT result = font_face->GetDesignGlyphMetrics(&glyph_index, 1U, &glyph_metrics, FALSE);
        if (FAILED(result)) {
            return allocation;
        }

        FLOAT glyph_advance = request.pixel_size;
        DWRITE_GLYPH_OFFSET glyph_offset{.advanceOffset = 0.0F, .ascenderOffset = 0.0F};
        DWRITE_GLYPH_RUN glyph_run{};
        glyph_run.fontFace = font_face.Get();
        glyph_run.fontEmSize = request.pixel_size;
        glyph_run.glyphCount = 1U;
        glyph_run.glyphIndices = &glyph_index;
        glyph_run.glyphAdvances = &glyph_advance;
        glyph_run.glyphOffsets = &glyph_offset;
        glyph_run.isSideways = FALSE;
        glyph_run.bidiLevel = 0U;

        ComPtr<IDWriteGlyphRunAnalysis> analysis;
        result = factory->CreateGlyphRunAnalysis(&glyph_run, 1.0F, nullptr, DWRITE_RENDERING_MODE_ALIASED,
                                                 DWRITE_MEASURING_MODE_NATURAL, 0.0F, 0.0F, analysis.GetAddressOf());
        if (FAILED(result) || analysis == nullptr) {
            return allocation;
        }

        RECT texture_bounds{};
        result = analysis->GetAlphaTextureBounds(DWRITE_TEXTURE_ALIASED_1x1, &texture_bounds);
        if (FAILED(result)) {
            return allocation;
        }

        const LONG width = std::max<LONG>(0L, texture_bounds.right - texture_bounds.left);
        const LONG height = std::max<LONG>(0L, texture_bounds.bottom - texture_bounds.top);
        ui::GlyphRasterBitmap bitmap{
            .width = static_cast<std::uint32_t>(width),
            .height = static_cast<std::uint32_t>(height),
            .pixel_format = ui::FontRasterizationPixelFormat::alpha8,
            .pixels = {},
        };
        const auto pixel_count = static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height);
        bitmap.pixels.resize(pixel_count);
        if (!bitmap.pixels.empty()) {
            result = analysis->CreateAlphaTexture(DWRITE_TEXTURE_ALIASED_1x1, &texture_bounds,
                                                  reinterpret_cast<BYTE*>(bitmap.pixels.data()),
                                                  static_cast<UINT32>(bitmap.pixels.size()));
            if (FAILED(result)) {
                return allocation;
            }
        }

        DWRITE_FONT_METRICS font_metrics{};
        font_face->GetMetrics(&font_metrics);

        allocation.atlas_bounds = ui::Rect{
            .x = 0.0F,
            .y = 0.0F,
            .width = static_cast<float>(bitmap.width),
            .height = static_cast<float>(bitmap.height),
        };
        allocation.bitmap = std::move(bitmap);
        allocation.metrics =
            glyph_metrics_to_ui_metrics(glyph_metrics, font_metrics, request.pixel_size, allocation.bitmap);
        return allocation;
    }
};

} // namespace

std::unique_ptr<ui::ITextShapingAdapter> make_native_editor_directwrite_text_shaping_adapter() {
    return std::make_unique<NativeEditorDirectWriteTextShapingAdapter>();
}

std::unique_ptr<ui::IFontRasterizerAdapter> make_native_editor_directwrite_font_rasterizer_adapter() {
    return std::make_unique<NativeEditorDirectWriteFontRasterizerAdapter>();
}

bool native_editor_directwrite_text_font_adapters_expose_native_handles() noexcept {
    return false;
}

} // namespace mirakana::editor

#else

namespace mirakana::editor {

std::unique_ptr<ui::ITextShapingAdapter> make_native_editor_directwrite_text_shaping_adapter() {
    return {};
}

std::unique_ptr<ui::IFontRasterizerAdapter> make_native_editor_directwrite_font_rasterizer_adapter() {
    return {};
}

bool native_editor_directwrite_text_font_adapters_expose_native_handles() noexcept {
    return false;
}

} // namespace mirakana::editor

#endif
