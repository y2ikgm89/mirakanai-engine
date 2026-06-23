// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_ui_text_font.hpp"

#include "win32_utf.hpp"

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
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::win32 {
namespace {

using Microsoft::WRL::ComPtr;

[[nodiscard]] bool is_adapter_safe(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos;
}

[[nodiscard]] bool is_continuation_byte(char value) noexcept {
    return (static_cast<unsigned char>(value) & 0xC0U) == 0x80U;
}

[[nodiscard]] bool is_utf8_scalar_boundary(std::string_view text, std::size_t offset) noexcept {
    return offset == text.size() || (offset < text.size() && !is_continuation_byte(text[offset]));
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

[[nodiscard]] std::vector<std::size_t> build_utf8_scalar_byte_offsets(std::string_view text) {
    std::vector<std::size_t> offsets;
    offsets.push_back(0U);
    std::size_t byte_offset = 0U;
    while (byte_offset < text.size()) {
        const auto lead = static_cast<unsigned char>(text[byte_offset]);
        byte_offset += std::min(utf8_sequence_length(lead), text.size() - byte_offset);
        offsets.push_back(byte_offset);
    }
    return offsets;
}

[[nodiscard]] std::size_t byte_offset_for_utf16_index(const std::vector<std::size_t>& offsets,
                                                      std::uint32_t utf16_index) noexcept {
    if (offsets.size() <= 1U) {
        return 0U;
    }
    const auto last_scalar_index = static_cast<std::uint32_t>(offsets.size() - 2U);
    return offsets[std::min(utf16_index, last_scalar_index)];
}

[[nodiscard]] std::uint32_t checked_text_length(std::size_t value) {
    if (value > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return 0U;
    }
    return static_cast<std::uint32_t>(value);
}

[[nodiscard]] ui::TextDirection resolved_direction(ui::TextDirection direction) noexcept {
    return direction == ui::TextDirection::right_to_left ? ui::TextDirection::right_to_left
                                                         : ui::TextDirection::left_to_right;
}

[[nodiscard]] ui::TextDirection direction_from_bidi_level(std::uint8_t bidi_level) noexcept {
    return (bidi_level % 2U) == 0U ? ui::TextDirection::left_to_right : ui::TextDirection::right_to_left;
}

[[nodiscard]] DWRITE_READING_DIRECTION dwrite_reading_direction(ui::TextDirection direction) noexcept {
    return direction == ui::TextDirection::right_to_left ? DWRITE_READING_DIRECTION_RIGHT_TO_LEFT
                                                         : DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
}

[[nodiscard]] bool line_break_is_allowed(const DWRITE_LINE_BREAKPOINT& breakpoint) noexcept {
    return breakpoint.breakConditionBefore == DWRITE_BREAK_CONDITION_CAN_BREAK ||
           breakpoint.breakConditionBefore == DWRITE_BREAK_CONDITION_MUST_BREAK ||
           breakpoint.breakConditionAfter == DWRITE_BREAK_CONDITION_CAN_BREAK ||
           breakpoint.breakConditionAfter == DWRITE_BREAK_CONDITION_MUST_BREAK;
}

void append_diagnostic(std::vector<Win32UiTextShapeDiagnostic>& diagnostics, Win32UiTextShapeDiagnosticCode code,
                       std::string message, std::size_t byte_offset = 0U) {
    diagnostics.push_back(Win32UiTextShapeDiagnostic{
        .code = code,
        .message = std::move(message),
        .byte_offset = byte_offset,
    });
}

void append_diagnostic(std::vector<Win32UiFontDiagnostic>& diagnostics, Win32UiFontDiagnosticCode code,
                       std::string message) {
    diagnostics.push_back(Win32UiFontDiagnostic{
        .code = code,
        .message = std::move(message),
    });
}

void validate_request(const Win32UiTextShapeRequest& request, Win32UiTextShapeResult& result) {
    if (request.text.empty() || !is_adapter_safe(request.text)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_text,
                          "DirectWrite text shaping requires non-empty adapter-safe text");
    }
    if (request.font_family.empty() || !is_adapter_safe(request.font_family)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::missing_font_family,
                          "DirectWrite text shaping requires a font family id");
    }
    if (!std::isfinite(request.pixel_size) || request.pixel_size <= 0.0F) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_pixel_size,
                          "DirectWrite text shaping requires a positive finite pixel size");
    }
    if (!std::isfinite(request.max_width) || request.max_width < 0.0F) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_max_width,
                          "DirectWrite text shaping requires a finite non-negative max width");
    }
    if (request.language_tag.empty() || !is_adapter_safe(request.language_tag)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::missing_language_tag,
                          "DirectWrite text shaping requires a language tag");
    }
    if (request.script_tag.empty() || !is_adapter_safe(request.script_tag)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::missing_script_tag,
                          "DirectWrite text shaping requires a script tag");
    }
    if (request.row_budget == 0U) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::row_budget_exceeded,
                          "DirectWrite text shaping requires a positive row budget");
    }
}

class DirectWriteTextAnalysisRows final : public IDWriteTextAnalysisSource, public IDWriteTextAnalysisSink {
  public:
    DirectWriteTextAnalysisRows(std::wstring text, std::wstring locale, ui::TextDirection direction)
        : text_(std::move(text)), locale_(std::move(locale)), direction_(resolved_direction(direction)),
          scripts_(text_.size(), DWRITE_SCRIPT_ANALYSIS{}), line_breaks_(text_.size(), DWRITE_LINE_BREAKPOINT{}),
          bidi_levels_(text_.size(), 0U) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override {
        if (object == nullptr) {
            return E_POINTER;
        }
        *object = nullptr;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWriteTextAnalysisSource)) {
            *object = static_cast<IDWriteTextAnalysisSource*>(this);
        } else if (riid == __uuidof(IDWriteTextAnalysisSink)) {
            *object = static_cast<IDWriteTextAnalysisSink*>(this);
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
        return --ref_count_;
    }

    HRESULT STDMETHODCALLTYPE GetTextAtPosition(UINT32 text_position, WCHAR const** text_string,
                                                UINT32* text_length) override {
        if (text_string == nullptr || text_length == nullptr) {
            return E_POINTER;
        }
        if (text_position >= text_.size()) {
            *text_string = nullptr;
            *text_length = 0U;
            return S_OK;
        }
        *text_string = text_.c_str() + text_position;
        *text_length = static_cast<UINT32>(text_.size() - text_position);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetTextBeforePosition(UINT32 text_position, WCHAR const** text_string,
                                                    UINT32* text_length) override {
        if (text_string == nullptr || text_length == nullptr) {
            return E_POINTER;
        }
        if (text_position == 0U || text_.empty()) {
            *text_string = nullptr;
            *text_length = 0U;
            return S_OK;
        }
        const auto clamped_position = std::min<std::size_t>(text_position, text_.size());
        *text_string = text_.c_str();
        *text_length = static_cast<UINT32>(clamped_position);
        return S_OK;
    }

    DWRITE_READING_DIRECTION STDMETHODCALLTYPE GetParagraphReadingDirection() override {
        return dwrite_reading_direction(direction_);
    }

    HRESULT STDMETHODCALLTYPE GetLocaleName(UINT32 text_position, UINT32* text_length,
                                            WCHAR const** locale_name) override {
        if (text_length == nullptr || locale_name == nullptr) {
            return E_POINTER;
        }
        if (text_position >= text_.size()) {
            *text_length = 0U;
        } else {
            *text_length = static_cast<UINT32>(text_.size() - text_position);
        }
        *locale_name = locale_.c_str();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetNumberSubstitution(UINT32 text_position, UINT32* text_length,
                                                    IDWriteNumberSubstitution** number_substitution) override {
        if (text_length == nullptr || number_substitution == nullptr) {
            return E_POINTER;
        }
        if (text_position >= text_.size()) {
            *text_length = 0U;
        } else {
            *text_length = static_cast<UINT32>(text_.size() - text_position);
        }
        *number_substitution = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetScriptAnalysis(UINT32 text_position, UINT32 text_length,
                                                DWRITE_SCRIPT_ANALYSIS const* script_analysis) override {
        if (script_analysis == nullptr) {
            return E_POINTER;
        }
        const auto end = std::min<std::size_t>(text_.size(), static_cast<std::size_t>(text_position) + text_length);
        for (std::size_t index = text_position; index < end; ++index) {
            scripts_[index] = *script_analysis;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetLineBreakpoints(UINT32 text_position, UINT32 text_length,
                                                 DWRITE_LINE_BREAKPOINT const* line_breakpoints) override {
        if (line_breakpoints == nullptr) {
            return E_POINTER;
        }
        const auto end = std::min<std::size_t>(text_.size(), static_cast<std::size_t>(text_position) + text_length);
        for (std::size_t index = text_position; index < end; ++index) {
            line_breaks_[index] = line_breakpoints[index - text_position];
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetBidiLevel(UINT32 text_position, UINT32 text_length, UINT8,
                                           UINT8 resolved_level) override {
        const auto end = std::min<std::size_t>(text_.size(), static_cast<std::size_t>(text_position) + text_length);
        for (std::size_t index = text_position; index < end; ++index) {
            bidi_levels_[index] = resolved_level;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE SetNumberSubstitution(UINT32, UINT32, IDWriteNumberSubstitution*) override {
        return S_OK;
    }

    [[nodiscard]] const std::vector<DWRITE_SCRIPT_ANALYSIS>& scripts() const noexcept {
        return scripts_;
    }

    [[nodiscard]] const std::vector<DWRITE_LINE_BREAKPOINT>& line_breaks() const noexcept {
        return line_breaks_;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& bidi_levels() const noexcept {
        return bidi_levels_;
    }

  private:
    std::atomic<ULONG> ref_count_{1U};
    std::wstring text_;
    std::wstring locale_;
    ui::TextDirection direction_{ui::TextDirection::left_to_right};
    std::vector<DWRITE_SCRIPT_ANALYSIS> scripts_;
    std::vector<DWRITE_LINE_BREAKPOINT> line_breaks_;
    std::vector<std::uint8_t> bidi_levels_;
};

class DirectWriteGlyphRunCollector final : public IDWriteTextRenderer {
  public:
    DirectWriteGlyphRunCollector(Win32UiTextShapeResult& result, ui::TextLayoutRun& run,
                                 const std::vector<std::size_t>& utf16_to_utf8_offsets,
                                 std::string requested_font_family)
        : result_(result), run_(run), utf16_to_utf8_offsets_(utf16_to_utf8_offsets),
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
        return --ref_count_;
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
            const auto cluster_byte_offset = byte_offset_for_utf16_index(utf16_to_utf8_offsets_, cluster_utf16_index);
            const auto glyph_id = static_cast<std::uint32_t>(glyph_run->glyphIndices[index]);
            result_.glyph_rows.push_back(Win32UiTextGlyphRow{
                .glyph_id = glyph_id,
                .cluster_byte_offset = cluster_byte_offset,
                .advance_x = advance,
                .advance_y = 0.0F,
                .offset_x = offset.advanceOffset,
                .offset_y = offset.ascenderOffset,
                .font_family = requested_font_family_,
            });
            run_.glyphs.push_back(ui::TextShapedGlyph{
                .glyph = glyph_id,
                .cluster_byte_offset = cluster_byte_offset,
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
    Win32UiTextShapeResult& result_;
    ui::TextLayoutRun& run_;
    const std::vector<std::size_t>& utf16_to_utf8_offsets_;
    std::string requested_font_family_;
};

void append_script_rows(const Win32UiTextShapeRequest& request, const DirectWriteTextAnalysisRows& analysis,
                        const std::vector<std::size_t>& utf16_offsets, Win32UiTextShapeResult& result,
                        ui::TextLayoutRun& run) {
    const auto& scripts = analysis.scripts();
    std::size_t start = 0U;
    while (start < scripts.size()) {
        std::size_t end = start + 1U;
        while (end < scripts.size() && scripts[end].script == scripts[start].script &&
               scripts[end].shapes == scripts[start].shapes) {
            ++end;
        }
        const auto start_byte = byte_offset_for_utf16_index(utf16_offsets, static_cast<std::uint32_t>(start));
        const auto end_byte = byte_offset_for_utf16_index(utf16_offsets, static_cast<std::uint32_t>(end));
        if (start_byte < end_byte) {
            const auto direction = resolved_direction(request.direction);
            run.segments.push_back(ui::TextShapingSegmentEvidence{
                .start_byte = start_byte,
                .end_byte = end_byte,
                .direction = direction,
                .script_tag = request.script_tag,
                .language_tag = request.language_tag,
            });
            result.boundary_rows.push_back(Win32UiTextBoundaryRow{
                .kind = Win32UiTextBoundaryKind::script_run,
                .start_byte = start_byte,
                .end_byte = end_byte,
                .direction = direction,
                .language_tag = request.language_tag,
                .script_tag = request.script_tag,
                .bidi_level = 0U,
                .line_break_allowed = false,
            });
        }
        start = end;
    }
}

void append_bidi_rows(const Win32UiTextShapeRequest& request, const DirectWriteTextAnalysisRows& analysis,
                      const std::vector<std::size_t>& utf16_offsets, Win32UiTextShapeResult& result,
                      ui::TextLayoutRun& run) {
    const auto& levels = analysis.bidi_levels();
    std::size_t start = 0U;
    while (start < levels.size()) {
        std::size_t end = start + 1U;
        while (end < levels.size() && levels[end] == levels[start]) {
            ++end;
        }
        const auto start_byte = byte_offset_for_utf16_index(utf16_offsets, static_cast<std::uint32_t>(start));
        const auto end_byte = byte_offset_for_utf16_index(utf16_offsets, static_cast<std::uint32_t>(end));
        if (start_byte < end_byte) {
            const auto direction = direction_from_bidi_level(levels[start]);
            run.boundaries.push_back(ui::TextBoundaryEvidence{
                .kind = ui::TextBoundaryEvidenceKind::bidi_run,
                .start_byte = start_byte,
                .end_byte = end_byte,
            });
            result.boundary_rows.push_back(Win32UiTextBoundaryRow{
                .kind = Win32UiTextBoundaryKind::bidi_run,
                .start_byte = start_byte,
                .end_byte = end_byte,
                .direction = direction,
                .language_tag = request.language_tag,
                .script_tag = request.script_tag,
                .bidi_level = levels[start],
                .line_break_allowed = false,
            });
        }
        start = end;
    }
}

void append_line_break_rows(const Win32UiTextShapeRequest& request, const DirectWriteTextAnalysisRows& analysis,
                            const std::vector<std::size_t>& utf16_offsets, Win32UiTextShapeResult& result,
                            ui::TextLayoutRun& run) {
    const auto& breaks = analysis.line_breaks();
    for (std::size_t index = 0U; index < breaks.size(); ++index) {
        const auto start_byte = byte_offset_for_utf16_index(utf16_offsets, static_cast<std::uint32_t>(index));
        const auto end_byte = byte_offset_for_utf16_index(utf16_offsets, static_cast<std::uint32_t>(index + 1U));
        if (start_byte >= end_byte) {
            continue;
        }
        const bool allowed = line_break_is_allowed(breaks[index]) || index + 1U == breaks.size();
        run.boundaries.push_back(ui::TextBoundaryEvidence{
            .kind = ui::TextBoundaryEvidenceKind::line_break,
            .start_byte = start_byte,
            .end_byte = end_byte,
        });
        result.boundary_rows.push_back(Win32UiTextBoundaryRow{
            .kind = Win32UiTextBoundaryKind::line_break,
            .start_byte = start_byte,
            .end_byte = end_byte,
            .direction = resolved_direction(request.direction),
            .language_tag = request.language_tag,
            .script_tag = request.script_tag,
            .bidi_level = 0U,
            .line_break_allowed = allowed,
        });
    }
}

void append_grapheme_rows(std::string_view text, ui::TextLayoutRun& run) {
    const auto scalar_offsets = build_utf8_scalar_byte_offsets(text);
    for (std::size_t index = 0U; index + 1U < scalar_offsets.size(); ++index) {
        if (scalar_offsets[index] < scalar_offsets[index + 1U]) {
            run.boundaries.push_back(ui::TextBoundaryEvidence{
                .kind = ui::TextBoundaryEvidenceKind::grapheme_cluster,
                .start_byte = scalar_offsets[index],
                .end_byte = scalar_offsets[index + 1U],
            });
        }
    }
}

[[nodiscard]] bool has_boundary_kind(const std::vector<Win32UiTextBoundaryRow>& rows,
                                     Win32UiTextBoundaryKind kind) noexcept {
    return std::ranges::any_of(rows, [kind](const auto& row) { return row.kind == kind; });
}

[[nodiscard]] std::size_t evidence_row_count(const Win32UiTextShapeResult& result) noexcept {
    return result.glyph_rows.size() + result.boundary_rows.size() + result.fallback_family_rows.size() +
           result.layout_runs.size();
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

[[nodiscard]] bool valid_font_source_kind(Win32UiFontSourceKind source_kind) noexcept {
    return source_kind == Win32UiFontSourceKind::system_font_collection ||
           source_kind == Win32UiFontSourceKind::project_font_asset;
}

[[nodiscard]] bool valid_font_license_status(Win32UiFontSourceKind source_kind,
                                             Win32UiFontLicenseStatus license_status) noexcept {
    if (source_kind == Win32UiFontSourceKind::system_font_collection) {
        return license_status == Win32UiFontLicenseStatus::system_font_runtime_reference;
    }
    if (source_kind == Win32UiFontSourceKind::project_font_asset) {
        return license_status == Win32UiFontLicenseStatus::project_font_asset_license_recorded;
    }
    return false;
}

[[nodiscard]] bool valid_font_pixel_format(ui::FontRasterizationPixelFormat pixel_format) noexcept {
    return pixel_format == ui::FontRasterizationPixelFormat::alpha8;
}

void validate_font_load_request(const Win32UiFontLoadRequest& request,
                                std::vector<Win32UiFontDiagnostic>& diagnostics) {
    if (request.font_family.empty() || !is_adapter_safe(request.font_family)) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::invalid_font_family,
                          "DirectWrite font loading requires an adapter-safe font family");
    }
    if (!valid_font_source_kind(request.source_kind)) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::invalid_source_kind,
                          "DirectWrite font loading requires a known font source kind");
    }
    if (request.provenance_id.empty() || !is_adapter_safe(request.provenance_id)) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::missing_provenance,
                          "DirectWrite font loading requires a source provenance row");
    }
    if (!valid_font_license_status(request.source_kind, request.license_status)) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::missing_license_status,
                          "DirectWrite font loading requires a matching font license status row");
    }
    if (request.source_kind == Win32UiFontSourceKind::project_font_asset) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::unsupported_project_font_asset,
                          "Project font asset loading is not selected for the Win32 DirectWrite proof");
    }
    if (request.row_budget == 0U) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::row_budget_exceeded,
                          "DirectWrite font loading requires a positive row budget");
    }
}

void validate_glyph_raster_request(const Win32UiGlyphRasterRequest& request,
                                   std::vector<Win32UiFontDiagnostic>& diagnostics) {
    validate_font_load_request(
        Win32UiFontLoadRequest{
            .font_family = request.font_family,
            .source_kind = request.source_kind,
            .provenance_id = request.provenance_id,
            .license_status = request.license_status,
            .row_budget = request.row_budget,
        },
        diagnostics);
    if (request.resolved_face_id.empty() || !is_adapter_safe(request.resolved_face_id)) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::missing_face_id,
                          "DirectWrite glyph rasterization requires a resolved face id");
    }
    if (request.glyph_id == 0U || request.glyph_id > std::numeric_limits<std::uint16_t>::max()) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::invalid_glyph_id,
                          "DirectWrite glyph rasterization requires a non-zero 16-bit glyph id");
    }
    if (!std::isfinite(request.pixel_size) || request.pixel_size <= 0.0F) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::invalid_pixel_size,
                          "DirectWrite glyph rasterization requires a positive finite pixel size");
    }
    if (!std::isfinite(request.dpi_scale) || request.dpi_scale <= 0.0F) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::invalid_dpi_scale,
                          "DirectWrite glyph rasterization requires a positive finite DPI scale");
    }
    if (!valid_font_pixel_format(request.pixel_format)) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::invalid_pixel_format,
                          "DirectWrite glyph rasterization currently emits alpha8 bitmap rows only");
    }
    if (request.atlas_padding > 4096U) {
        append_diagnostic(diagnostics, Win32UiFontDiagnosticCode::invalid_atlas_padding,
                          "DirectWrite glyph rasterization atlas padding exceeds the supported row budget");
    }
}

[[nodiscard]] std::string make_resolved_face_id(const Win32UiFontLoadRequest& request,
                                                const DWRITE_FONT_METRICS& metrics) {
    return request.font_family + ":system:regular:normal:" + std::to_string(metrics.designUnitsPerEm);
}

struct LoadedDirectWriteFontFace {
    ComPtr<IDWriteFactory> factory;
    ComPtr<IDWriteFontFace> face;
    Win32UiFontFaceRow row;
    bool factory_created{false};
    bool system_font_collection_loaded{false};
    std::vector<Win32UiFontDiagnostic> diagnostics;
};

[[nodiscard]] LoadedDirectWriteFontFace load_directwrite_font_face(const Win32UiFontLoadRequest& request) {
    LoadedDirectWriteFontFace loaded;
    validate_font_load_request(request, loaded.diagnostics);
    if (!loaded.diagnostics.empty()) {
        return loaded;
    }

    std::wstring wide_font_family;
    try {
        wide_font_family = detail::wide_from_utf8(request.font_family);
    } catch (...) {
        append_diagnostic(loaded.diagnostics, Win32UiFontDiagnosticCode::invalid_font_family,
                          "DirectWrite font family must be strict UTF-8");
        return loaded;
    }

    loaded.factory = make_directwrite_factory();
    if (loaded.factory == nullptr) {
        append_diagnostic(loaded.diagnostics, Win32UiFontDiagnosticCode::directwrite_factory_unavailable,
                          "DirectWrite factory creation failed");
        return loaded;
    }
    loaded.factory_created = true;

    ComPtr<IDWriteFontCollection> collection;
    HRESULT hr = loaded.factory->GetSystemFontCollection(collection.GetAddressOf());
    if (FAILED(hr) || collection == nullptr) {
        append_diagnostic(loaded.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_collection_failed,
                          "DirectWrite system font collection load failed");
        return loaded;
    }
    loaded.system_font_collection_loaded = true;

    UINT32 family_index = 0U;
    BOOL family_exists = FALSE;
    hr = collection->FindFamilyName(wide_font_family.c_str(), &family_index, &family_exists);
    if (FAILED(hr) || family_exists == FALSE) {
        append_diagnostic(loaded.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_family_not_found,
                          "DirectWrite system font family was not found");
        return loaded;
    }

    ComPtr<IDWriteFontFamily> family;
    hr = collection->GetFontFamily(family_index, family.GetAddressOf());
    if (FAILED(hr) || family == nullptr) {
        append_diagnostic(loaded.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_face_failed,
                          "DirectWrite font family resolution failed");
        return loaded;
    }

    ComPtr<IDWriteFont> font;
    hr = family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                      font.GetAddressOf());
    if (FAILED(hr) || font == nullptr) {
        append_diagnostic(loaded.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_face_failed,
                          "DirectWrite matching font resolution failed");
        return loaded;
    }

    hr = font->CreateFontFace(loaded.face.GetAddressOf());
    if (FAILED(hr) || loaded.face == nullptr) {
        append_diagnostic(loaded.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_face_failed,
                          "DirectWrite font face creation failed");
        return loaded;
    }

    DWRITE_FONT_METRICS metrics{};
    loaded.face->GetMetrics(&metrics);
    loaded.row = Win32UiFontFaceRow{
        .font_family = request.font_family,
        .resolved_face_id = make_resolved_face_id(request, metrics),
        .source_kind = request.source_kind,
        .provenance_id = request.provenance_id,
        .license_status = request.license_status,
        .glyph_count = static_cast<std::uint32_t>(loaded.face->GetGlyphCount()),
        .design_units_per_em = metrics.designUnitsPerEm,
    };
    return loaded;
}

[[nodiscard]] std::uint32_t extent_from_rect(const RECT& bounds, bool horizontal) noexcept {
    const auto first = horizontal ? bounds.left : bounds.top;
    const auto second = horizontal ? bounds.right : bounds.bottom;
    if (second <= first) {
        return 0U;
    }
    return static_cast<std::uint32_t>(second - first);
}

[[nodiscard]] bool checked_atlas_extent(std::uint32_t glyph_extent, std::uint32_t padding, float& out_extent) noexcept {
    if (padding > (std::numeric_limits<std::uint32_t>::max() - glyph_extent) / 2U) {
        return false;
    }
    out_extent = static_cast<float>(glyph_extent + (padding * 2U));
    return std::isfinite(out_extent);
}

[[nodiscard]] std::vector<std::byte> alpha_texture_to_pixels(const std::vector<std::uint8_t>& alpha_texture,
                                                             std::uint32_t width, std::uint32_t height,
                                                             DWRITE_TEXTURE_TYPE texture_type) {
    const auto pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<std::byte> pixels(pixel_count);
    if (texture_type == DWRITE_TEXTURE_ALIASED_1x1) {
        for (std::size_t index = 0U; index < pixel_count; ++index) {
            pixels[index] = static_cast<std::byte>(alpha_texture[index]);
        }
        return pixels;
    }

    for (std::size_t index = 0U; index < pixel_count; ++index) {
        const auto offset = index * 3U;
        const auto alpha = static_cast<std::uint8_t>((static_cast<std::uint32_t>(alpha_texture[offset]) +
                                                      static_cast<std::uint32_t>(alpha_texture[offset + 1U]) +
                                                      static_cast<std::uint32_t>(alpha_texture[offset + 2U])) /
                                                     3U);
        pixels[index] = static_cast<std::byte>(alpha);
    }
    return pixels;
}

[[nodiscard]] bool valid_raster_allocation(const ui::GlyphAtlasAllocation& allocation) noexcept {
    const auto& bitmap = allocation.bitmap;
    const auto& metrics = allocation.metrics;
    const auto pixel_count = static_cast<std::size_t>(bitmap.width) * static_cast<std::size_t>(bitmap.height);
    return allocation.glyph != 0U && bitmap.pixel_format == ui::FontRasterizationPixelFormat::alpha8 &&
           bitmap.pixels.size() == pixel_count && std::isfinite(metrics.width) && std::isfinite(metrics.height) &&
           std::isfinite(metrics.bearing_x) && std::isfinite(metrics.bearing_y) && std::isfinite(metrics.advance_x) &&
           std::isfinite(metrics.advance_y) && metrics.width >= 0.0F && metrics.height >= 0.0F &&
           metrics.advance_x >= 0.0F && metrics.advance_y >= 0.0F && std::isfinite(allocation.atlas_bounds.width) &&
           std::isfinite(allocation.atlas_bounds.height) && allocation.atlas_bounds.width >= 0.0F &&
           allocation.atlas_bounds.height >= 0.0F;
}

} // namespace

bool Win32UiTextShapeResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

Win32UiTextShapeResult validate_win32_ui_text_shape_result_rows(Win32UiTextShapeResult result, std::size_t row_budget) {
    if (row_budget == 0U || evidence_row_count(result) > row_budget) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::row_budget_exceeded,
                          "DirectWrite text shaping evidence rows exceed the request budget");
    }
    if (result.public_native_handles_exposed) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::public_native_handles_exposed,
                          "DirectWrite text shaping evidence must not expose native handles");
    }
    if (!result.directwrite_factory_created) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_factory_unavailable,
                          "DirectWrite factory was not created");
    }
    if (!result.directwrite_text_layout_created) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_layout_failed,
                          "DirectWrite text layout was not created");
    }
    if (!result.directwrite_text_analyzer_invoked) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_analysis_failed,
                          "DirectWrite text analyzer was not invoked");
    }
    if (result.shaped_text.empty() || !is_adapter_safe(result.shaped_text)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_text,
                          "DirectWrite text shaping result requires shaped text");
    }
    if (result.glyph_rows.empty()) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::missing_glyph_rows,
                          "DirectWrite text shaping result requires glyph rows");
    }
    if (result.boundary_rows.empty() || !has_boundary_kind(result.boundary_rows, Win32UiTextBoundaryKind::script_run) ||
        !has_boundary_kind(result.boundary_rows, Win32UiTextBoundaryKind::line_break) ||
        !has_boundary_kind(result.boundary_rows, Win32UiTextBoundaryKind::bidi_run)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::missing_boundary_rows,
                          "DirectWrite text shaping result requires script, line-break, and bidi boundary rows");
    }
    if (result.fallback_family_rows.empty()) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::missing_fallback_rows,
                          "DirectWrite text shaping result requires fallback family rows");
    }

    std::vector<std::size_t> clusters;
    clusters.reserve(result.glyph_rows.size());
    for (const auto& glyph : result.glyph_rows) {
        if (glyph.cluster_byte_offset >= result.shaped_text.size() ||
            !is_utf8_scalar_boundary(result.shaped_text, glyph.cluster_byte_offset)) {
            append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_cluster_boundary,
                              "glyph cluster byte offset must be on a UTF-8 scalar boundary",
                              glyph.cluster_byte_offset);
        }
        if (std::ranges::find(clusters, glyph.cluster_byte_offset) != clusters.end()) {
            append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::duplicate_cluster_row,
                              "glyph cluster byte offsets must be unique for selected production evidence",
                              glyph.cluster_byte_offset);
        } else {
            clusters.push_back(glyph.cluster_byte_offset);
        }
    }

    for (const auto& boundary : result.boundary_rows) {
        if (boundary.start_byte >= boundary.end_byte || boundary.end_byte > result.shaped_text.size() ||
            !is_utf8_scalar_boundary(result.shaped_text, boundary.start_byte) ||
            !is_utf8_scalar_boundary(result.shaped_text, boundary.end_byte)) {
            append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_cluster_boundary,
                              "boundary rows must use UTF-8 scalar byte ranges", boundary.start_byte);
        }
    }

    result.ready = result.diagnostics.empty();
    return result;
}

Win32UiTextShapeResult shape_win32_ui_text_with_directwrite(const Win32UiTextShapeRequest& request) {
    Win32UiTextShapeResult result;
    result.shaped_text = request.text;
    validate_request(request, result);

    std::wstring wide_text;
    std::wstring wide_font_family;
    std::wstring wide_language;
    try {
        wide_text = detail::wide_from_utf8(request.text);
        wide_font_family = detail::wide_from_utf8(request.font_family);
        wide_language = detail::wide_from_utf8(request.language_tag);
    } catch (...) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_utf8,
                          "DirectWrite text shaping request must be strict UTF-8");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }

    const auto text_length = checked_text_length(wide_text.size());
    if (text_length == 0U) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::invalid_text,
                          "DirectWrite text shaping text is empty or too large");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }

    if (!result.diagnostics.empty()) {
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }

    auto factory = make_directwrite_factory();
    if (factory == nullptr) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_factory_unavailable,
                          "DirectWrite factory creation failed");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }
    result.directwrite_factory_created = true;

    ComPtr<IDWriteTextFormat> text_format;
    HRESULT hr = factory->CreateTextFormat(wide_font_family.c_str(), nullptr, DWRITE_FONT_WEIGHT_REGULAR,
                                           DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, request.pixel_size,
                                           wide_language.c_str(), text_format.GetAddressOf());
    if (FAILED(hr) || text_format == nullptr) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_format_failed,
                          "DirectWrite text format creation failed");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }

    hr = text_format->SetReadingDirection(dwrite_reading_direction(request.direction));
    if (FAILED(hr)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_format_failed,
                          "DirectWrite reading direction setup failed");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }

    constexpr float default_layout_extent = 4096.0F;
    const float layout_width = request.max_width > 0.0F ? request.max_width : default_layout_extent;
    ComPtr<IDWriteTextLayout> text_layout;
    hr = factory->CreateTextLayout(wide_text.c_str(), text_length, text_format.Get(), layout_width,
                                   default_layout_extent, text_layout.GetAddressOf());
    if (FAILED(hr) || text_layout == nullptr) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_layout_failed,
                          "DirectWrite text layout creation failed");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }
    result.directwrite_text_layout_created = true;

    ComPtr<IDWriteTextAnalyzer> analyzer;
    hr = factory->CreateTextAnalyzer(analyzer.GetAddressOf());
    if (FAILED(hr) || analyzer == nullptr) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_analysis_failed,
                          "DirectWrite text analyzer creation failed");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }

    DirectWriteTextAnalysisRows analysis{wide_text, wide_language, request.direction};
    hr = analyzer->AnalyzeScript(&analysis, 0U, text_length, &analysis);
    if (SUCCEEDED(hr)) {
        hr = analyzer->AnalyzeBidi(&analysis, 0U, text_length, &analysis);
    }
    if (SUCCEEDED(hr)) {
        hr = analyzer->AnalyzeLineBreakpoints(&analysis, 0U, text_length, &analysis);
    }
    if (FAILED(hr)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_analysis_failed,
                          "DirectWrite text analysis failed");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }
    result.directwrite_text_analyzer_invoked = true;

    DWRITE_TEXT_METRICS metrics{};
    hr = text_layout->GetMetrics(&metrics);
    if (FAILED(hr)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_layout_failed,
                          "DirectWrite text metrics query failed");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }

    ui::TextLayoutRun run;
    run.text = request.text;
    run.bounds = ui::Rect{
        .x = 0.0F,
        .y = 0.0F,
        .width = std::max(metrics.widthIncludingTrailingWhitespace, 1.0F),
        .height = std::max(metrics.height, request.pixel_size),
    };

    const auto utf16_offsets = build_utf16_to_utf8_byte_offsets(request.text, wide_text);
    append_script_rows(request, analysis, utf16_offsets, result, run);
    append_grapheme_rows(request.text, run);
    append_line_break_rows(request, analysis, utf16_offsets, result, run);
    append_bidi_rows(request, analysis, utf16_offsets, result, run);

    result.fallback_family_rows.push_back(Win32UiTextFallbackFamilyRow{
        .cluster_byte_offset = 0U,
        .requested_font_family = request.font_family,
        .resolved_font_family = request.font_family,
        .fallback_used = false,
    });
    run.fallback_rows.push_back(ui::TextFontFallbackEvidence{
        .cluster_byte_offset = 0U,
        .requested_font_family = request.font_family,
        .resolved_font_family = request.font_family,
        .fallback_used = false,
    });

    DirectWriteGlyphRunCollector collector{result, run, utf16_offsets, request.font_family};
    hr = text_layout->Draw(nullptr, &collector, 0.0F, 0.0F);
    if (FAILED(hr)) {
        append_diagnostic(result.diagnostics, Win32UiTextShapeDiagnosticCode::directwrite_text_layout_failed,
                          "DirectWrite text layout draw failed");
        return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
    }

    result.layout_runs.push_back(std::move(run));
    return validate_win32_ui_text_shape_result_rows(std::move(result), request.row_budget);
}

ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_directwrite_text_shaping_production_evidence(const Win32UiTextShapeResult& result) {
    ui::RuntimeUiPlatformProductionEvidenceRow row{
        .id = "runtime-ui-platform.text-shaping.win32.directwrite",
        .feature = ui::RuntimeUiPlatformProductionFeature::production_text_shaping,
        .proof = ui::RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = result.succeeded(),
        .dependency_recorded = true,
        .host_evidence_available = result.directwrite_factory_created && result.directwrite_text_layout_created &&
                                   result.directwrite_text_analyzer_invoked,
        .public_native_handles = result.public_native_handles_exposed,
    };
    if (!row.ready) {
        row.blocker = result.diagnostics.empty() ? "Windows DirectWrite text shaping evidence is not ready"
                                                 : result.diagnostics.front().message;
    }
    return row;
}

bool Win32UiFontLoadResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

bool Win32UiGlyphRasterResult::succeeded() const noexcept {
    return ready && diagnostics.empty();
}

Win32UiFontLoadResult validate_win32_ui_font_load_result_rows(Win32UiFontLoadResult result, std::size_t row_budget) {
    if (row_budget == 0U || result.font_face_rows.size() > row_budget) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::row_budget_exceeded,
                          "DirectWrite font loading evidence rows exceed the request budget");
    }
    if (result.public_native_handles_exposed) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::public_native_handles_exposed,
                          "DirectWrite font loading evidence must not expose native handles");
    }
    if (!result.directwrite_factory_created) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_factory_unavailable,
                          "DirectWrite factory was not created for font loading");
    }
    if (!result.system_font_collection_loaded) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_collection_failed,
                          "DirectWrite system font collection was not loaded");
    }
    if (result.font_face_rows.empty()) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_face_failed,
                          "DirectWrite font loading requires a resolved font face row");
    }
    for (const auto& row : result.font_face_rows) {
        if (row.font_family.empty() || !is_adapter_safe(row.font_family)) {
            append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::invalid_font_family,
                              "DirectWrite font face row requires an adapter-safe font family");
        }
        if (row.resolved_face_id.empty() || !is_adapter_safe(row.resolved_face_id)) {
            append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::missing_face_id,
                              "DirectWrite font face row requires a resolved face id");
        }
        if (!valid_font_source_kind(row.source_kind)) {
            append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::invalid_source_kind,
                              "DirectWrite font face row requires a known source kind");
        }
        if (row.provenance_id.empty() || !is_adapter_safe(row.provenance_id)) {
            append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::missing_provenance,
                              "DirectWrite font face row requires source provenance");
        }
        if (!valid_font_license_status(row.source_kind, row.license_status)) {
            append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::missing_license_status,
                              "DirectWrite font face row requires a matching license status");
        }
        if (row.glyph_count == 0U || row.design_units_per_em == 0U) {
            append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_face_failed,
                              "DirectWrite font face row requires glyph count and design units");
        }
    }
    result.ready = result.diagnostics.empty();
    return result;
}

Win32UiFontLoadResult load_win32_ui_font_face(const Win32UiFontLoadRequest& request) {
    auto loaded = load_directwrite_font_face(request);
    Win32UiFontLoadResult result;
    result.directwrite_factory_created = loaded.factory_created;
    result.system_font_collection_loaded = loaded.system_font_collection_loaded;
    result.diagnostics = std::move(loaded.diagnostics);
    if (loaded.face != nullptr) {
        result.font_face_rows.push_back(std::move(loaded.row));
    }
    return validate_win32_ui_font_load_result_rows(std::move(result), request.row_budget);
}

Win32UiGlyphRasterResult validate_win32_ui_glyph_raster_result_rows(Win32UiGlyphRasterResult result,
                                                                    std::size_t row_budget) {
    const std::size_t row_count = result.font_face_rows.size() + result.bitmap_rows.size() +
                                  result.metrics_rows.size() + result.color_rows.size() +
                                  (result.allocation.has_value() ? 1U : 0U);
    if (row_budget == 0U || row_count > row_budget) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::row_budget_exceeded,
                          "DirectWrite glyph raster evidence rows exceed the request budget");
    }
    if (result.public_native_handles_exposed) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::public_native_handles_exposed,
                          "DirectWrite glyph raster evidence must not expose native handles");
    }
    if (!result.directwrite_factory_created) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_factory_unavailable,
                          "DirectWrite factory was not created for glyph rasterization");
    }
    if (!result.font_face_loaded || result.font_face_rows.empty()) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_font_face_failed,
                          "DirectWrite glyph rasterization requires a resolved font face");
    }
    if (!result.glyph_metrics_queried || result.metrics_rows.empty()) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_glyph_metrics_failed,
                          "DirectWrite glyph rasterization requires glyph metric rows");
    }
    if (!result.recommended_rendering_mode_queried) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_rendering_mode_failed,
                          "DirectWrite glyph rasterization requires rendering mode evidence");
    }
    if (!result.glyph_run_analysis_created) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_glyph_run_analysis_failed,
                          "DirectWrite glyph run analysis was not created");
    }
    if (!result.alpha_texture_created || result.bitmap_rows.empty()) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::missing_bitmap_pixels,
                          "DirectWrite glyph rasterization requires bitmap pixel rows");
    }
    if (!result.allocation.has_value() || !valid_raster_allocation(*result.allocation)) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::invalid_metrics,
                          "DirectWrite glyph rasterization requires a valid allocation, bitmap, and metrics row");
    }
    result.ready = result.diagnostics.empty();
    return result;
}

Win32UiGlyphRasterResult rasterize_win32_ui_glyph(const Win32UiGlyphRasterRequest& request) {
    Win32UiGlyphRasterResult result;
    validate_glyph_raster_request(request, result.diagnostics);
    if (!result.diagnostics.empty()) {
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }

    auto loaded = load_directwrite_font_face(Win32UiFontLoadRequest{
        .font_family = request.font_family,
        .source_kind = request.source_kind,
        .provenance_id = request.provenance_id,
        .license_status = request.license_status,
        .row_budget = request.row_budget,
    });
    result.directwrite_factory_created = loaded.factory_created;
    result.system_font_collection_loaded = loaded.system_font_collection_loaded;
    result.diagnostics.insert(result.diagnostics.end(), loaded.diagnostics.begin(), loaded.diagnostics.end());
    if (loaded.face == nullptr) {
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }
    result.font_face_loaded = true;
    result.font_face_rows.push_back(loaded.row);
    if (loaded.row.resolved_face_id != request.resolved_face_id) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::font_face_mismatch,
                          "DirectWrite glyph rasterization request face id does not match the resolved face");
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }

    const auto glyph_index = static_cast<UINT16>(request.glyph_id);
    DWRITE_GLYPH_METRICS glyph_metrics{};
    HRESULT hr = loaded.face->GetDesignGlyphMetrics(&glyph_index, 1U, &glyph_metrics, FALSE);
    if (FAILED(hr)) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_glyph_metrics_failed,
                          "DirectWrite glyph metrics query failed");
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }
    result.glyph_metrics_queried = true;

    const float design_to_dip = request.pixel_size / static_cast<float>(loaded.row.design_units_per_em);
    const float design_to_pixel = design_to_dip * request.dpi_scale;
    const FLOAT glyph_advance = static_cast<FLOAT>(static_cast<float>(glyph_metrics.advanceWidth) * design_to_dip);
    const DWRITE_GLYPH_OFFSET glyph_offset{.advanceOffset = 0.0F, .ascenderOffset = 0.0F};
    const DWRITE_GLYPH_RUN glyph_run{
        .fontFace = loaded.face.Get(),
        .fontEmSize = request.pixel_size,
        .glyphCount = 1U,
        .glyphIndices = &glyph_index,
        .glyphAdvances = &glyph_advance,
        .glyphOffsets = &glyph_offset,
        .isSideways = FALSE,
        .bidiLevel = 0U,
    };

    ComPtr<IDWriteRenderingParams> rendering_params;
    hr = loaded.factory->CreateRenderingParams(rendering_params.GetAddressOf());
    if (FAILED(hr) || rendering_params == nullptr) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_rendering_mode_failed,
                          "DirectWrite rendering params creation failed");
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }

    DWRITE_RENDERING_MODE rendering_mode = DWRITE_RENDERING_MODE_ALIASED;
    hr = loaded.face->GetRecommendedRenderingMode(request.pixel_size, request.dpi_scale, DWRITE_MEASURING_MODE_NATURAL,
                                                  rendering_params.Get(), &rendering_mode);
    if (FAILED(hr)) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_rendering_mode_failed,
                          "DirectWrite recommended rendering mode query failed");
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }
    result.recommended_rendering_mode_queried = true;

    const DWRITE_TEXTURE_TYPE texture_type =
        rendering_mode == DWRITE_RENDERING_MODE_ALIASED ? DWRITE_TEXTURE_ALIASED_1x1 : DWRITE_TEXTURE_CLEARTYPE_3x1;
    const std::size_t texture_stride = texture_type == DWRITE_TEXTURE_ALIASED_1x1 ? 1U : 3U;

    ComPtr<IDWriteGlyphRunAnalysis> analysis;
    hr = loaded.factory->CreateGlyphRunAnalysis(&glyph_run, request.dpi_scale, nullptr, rendering_mode,
                                                DWRITE_MEASURING_MODE_NATURAL, 0.0F, 0.0F, analysis.GetAddressOf());
    if (FAILED(hr) || analysis == nullptr) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_glyph_run_analysis_failed,
                          "DirectWrite glyph run analysis creation failed");
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }
    result.glyph_run_analysis_created = true;

    RECT texture_bounds{};
    hr = analysis->GetAlphaTextureBounds(texture_type, &texture_bounds);
    if (FAILED(hr)) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::directwrite_glyph_run_analysis_failed,
                          "DirectWrite alpha texture bounds query failed");
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }

    const auto width = extent_from_rect(texture_bounds, true);
    const auto height = extent_from_rect(texture_bounds, false);
    const auto pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    std::vector<std::uint8_t> alpha_texture(pixel_count * texture_stride);
    if (!alpha_texture.empty()) {
        hr = analysis->CreateAlphaTexture(texture_type, &texture_bounds, alpha_texture.data(),
                                          static_cast<UINT32>(alpha_texture.size()));
        if (FAILED(hr)) {
            append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::missing_bitmap_pixels,
                              "DirectWrite alpha texture creation failed");
            return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
        }
    }
    result.alpha_texture_created = true;

    float atlas_width = 0.0F;
    float atlas_height = 0.0F;
    if (!checked_atlas_extent(width, request.atlas_padding, atlas_width) ||
        !checked_atlas_extent(height, request.atlas_padding, atlas_height)) {
        append_diagnostic(result.diagnostics, Win32UiFontDiagnosticCode::atlas_overflow,
                          "DirectWrite glyph rasterization atlas bounds overflowed");
        return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
    }

    const auto pixels = alpha_texture_to_pixels(alpha_texture, width, height, texture_type);
    result.bitmap_rows.push_back(Win32UiGlyphBitmapRow{
        .glyph_id = request.glyph_id,
        .width = width,
        .height = height,
        .pixel_format = request.pixel_format,
        .pixel_bytes = pixels.size(),
        .atlas_padding = request.atlas_padding,
    });
    result.metrics_rows.push_back(Win32UiGlyphMetricsRow{
        .glyph_id = request.glyph_id,
        .width = static_cast<float>(width),
        .height = static_cast<float>(height),
        .bearing_x = static_cast<float>(texture_bounds.left),
        .bearing_y = static_cast<float>(-texture_bounds.top),
        .advance_x = static_cast<float>(glyph_metrics.advanceWidth) * design_to_pixel,
        .advance_y = static_cast<float>(glyph_metrics.advanceHeight) * design_to_pixel,
    });
    result.color_rows.push_back(Win32UiGlyphColorRow{
        .glyph_id = request.glyph_id,
        .color_glyph_checked = true,
        .color_glyph_used = false,
    });
    result.allocation = ui::GlyphAtlasAllocation{
        .glyph = request.glyph_id,
        .atlas_bounds = ui::Rect{.x = 0.0F, .y = 0.0F, .width = atlas_width, .height = atlas_height},
        .bitmap =
            ui::GlyphRasterBitmap{
                .width = width,
                .height = height,
                .pixel_format = request.pixel_format,
                .pixels = pixels,
            },
        .metrics =
            ui::GlyphRasterMetrics{
                .width = static_cast<float>(width),
                .height = static_cast<float>(height),
                .bearing_x = static_cast<float>(texture_bounds.left),
                .bearing_y = static_cast<float>(-texture_bounds.top),
                .advance_x = static_cast<float>(glyph_metrics.advanceWidth) * design_to_pixel,
                .advance_y = static_cast<float>(glyph_metrics.advanceHeight) * design_to_pixel,
            },
    };

    return validate_win32_ui_glyph_raster_result_rows(std::move(result), request.row_budget);
}

ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_directwrite_font_loading_production_evidence(const Win32UiFontLoadResult& result) {
    ui::RuntimeUiPlatformProductionEvidenceRow row{
        .id = "runtime-ui-platform.font-loading.win32.directwrite",
        .feature = ui::RuntimeUiPlatformProductionFeature::real_font_loading,
        .proof = ui::RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = result.succeeded(),
        .dependency_recorded = true,
        .host_evidence_available = result.directwrite_factory_created && result.system_font_collection_loaded,
        .public_native_handles = result.public_native_handles_exposed,
    };
    if (!row.ready) {
        row.blocker = result.diagnostics.empty() ? "Windows DirectWrite font loading evidence is not ready"
                                                 : result.diagnostics.front().message;
    }
    return row;
}

ui::RuntimeUiPlatformProductionEvidenceRow
make_win32_directwrite_font_rasterization_production_evidence(const Win32UiGlyphRasterResult& result) {
    ui::RuntimeUiPlatformProductionEvidenceRow row{
        .id = "runtime-ui-platform.font-rasterization.win32.directwrite",
        .feature = ui::RuntimeUiPlatformProductionFeature::font_rasterization,
        .proof = ui::RuntimeUiPlatformProductionProofKind::official_sdk_adapter,
        .selected = true,
        .ready = result.succeeded(),
        .dependency_recorded = true,
        .host_evidence_available = result.directwrite_factory_created && result.font_face_loaded &&
                                   result.glyph_metrics_queried && result.glyph_run_analysis_created &&
                                   result.alpha_texture_created,
        .public_native_handles = result.public_native_handles_exposed,
    };
    if (!row.ready) {
        row.blocker = result.diagnostics.empty() ? "Windows DirectWrite glyph rasterization evidence is not ready"
                                                 : result.diagnostics.front().message;
    }
    return row;
}

} // namespace mirakana::win32

#endif
