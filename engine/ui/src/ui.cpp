// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

[[nodiscard]] bool is_finite_nonnegative(float value) noexcept {
    return std::isfinite(value) && value >= 0.0F;
}

[[nodiscard]] bool is_valid_edge_insets(EdgeInsets insets) noexcept {
    return is_finite_nonnegative(insets.top) && is_finite_nonnegative(insets.right) &&
           is_finite_nonnegative(insets.bottom) && is_finite_nonnegative(insets.left);
}

[[nodiscard]] bool is_valid_size_constraints(SizeConstraints constraints) noexcept {
    if (!is_finite_nonnegative(constraints.min_width) || !is_finite_nonnegative(constraints.min_height) ||
        !is_finite_nonnegative(constraints.max_width) || !is_finite_nonnegative(constraints.max_height)) {
        return false;
    }
    if (constraints.max_width > 0.0F && constraints.min_width > constraints.max_width) {
        return false;
    }
    return constraints.max_height <= 0.0F || constraints.min_height <= constraints.max_height;
}

[[nodiscard]] bool is_focusable_role(SemanticRole role) noexcept {
    switch (role) {
    case SemanticRole::button:
    case SemanticRole::text_field:
    case SemanticRole::list_item:
    case SemanticRole::checkbox:
    case SemanticRole::slider:
        return true;
    case SemanticRole::none:
    case SemanticRole::root:
    case SemanticRole::panel:
    case SemanticRole::label:
    case SemanticRole::list:
    case SemanticRole::image:
    case SemanticRole::dialog:
        return false;
    }
    return false;
}

[[nodiscard]] bool navigates_forward(NavigationDirection direction) noexcept {
    return direction == NavigationDirection::next || direction == NavigationDirection::down ||
           direction == NavigationDirection::right;
}

[[nodiscard]] bool is_accessibility_candidate(const Element& element) noexcept {
    if (element.role == SemanticRole::none || element.role == SemanticRole::root) {
        return false;
    }
    return !element.accessibility_label.empty() || !element.text.label.empty() ||
           !element.text.localization_key.empty() || is_focusable_role(element.role);
}

[[nodiscard]] std::string accessibility_label_for(const Element& element) {
    if (!element.accessibility_label.empty()) {
        return element.accessibility_label;
    }
    return element.text.label;
}

[[nodiscard]] bool has_text_run(const Element& element) noexcept {
    return !element.text.label.empty() || !element.text.localization_key.empty();
}

[[nodiscard]] bool has_image_placeholder(const Element& element) noexcept {
    return element.role == SemanticRole::image || !element.image.resource_id.empty() ||
           !element.image.asset_uri.empty();
}

[[nodiscard]] bool is_valid_adapter_string(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos;
}

[[nodiscard]] std::string text_payload_value(const TextContent& text) {
    return text.label;
}

[[nodiscard]] bool is_positive_rect(Rect rect) noexcept {
    return is_valid_rect(rect) && rect.width > 0.0F && rect.height > 0.0F;
}

[[nodiscard]] std::optional<std::size_t> pixel_stride(ImageDecodePixelFormat format) noexcept {
    switch (format) {
    case ImageDecodePixelFormat::rgba8_unorm:
        return 4U;
    case ImageDecodePixelFormat::unknown:
        return std::nullopt;
    }
    return std::nullopt;
}

[[nodiscard]] bool is_valid_image_decode_result(const ImageDecodeResult& image) noexcept {
    if (image.width == 0U || image.height == 0U) {
        return false;
    }

    const auto stride = pixel_stride(image.pixel_format);
    if (!stride.has_value()) {
        return false;
    }

    const auto width = static_cast<std::size_t>(image.width);
    const auto height = static_cast<std::size_t>(image.height);
    const auto max_size = std::numeric_limits<std::size_t>::max();
    if (height != 0U && width > max_size / height) {
        return false;
    }
    const auto pixel_count = width * height;
    if (*stride != 0U && pixel_count > max_size / *stride) {
        return false;
    }
    return image.pixels.size() == pixel_count * *stride;
}

[[nodiscard]] bool is_valid_text_shaping_result(std::string_view request_text, const std::vector<TextLayoutRun>& runs) {
    if (runs.empty()) {
        return false;
    }

    std::string shaped_text;
    shaped_text.reserve(request_text.size());
    for (const auto& run : runs) {
        if (run.text.empty() || !is_valid_adapter_string(run.text) || !is_positive_rect(run.bounds)) {
            return false;
        }
        if (run.text.size() > std::numeric_limits<std::size_t>::max() - shaped_text.size()) {
            return false;
        }
        shaped_text += run.text;
    }

    return shaped_text == request_text;
}

[[nodiscard]] bool has_diagnostic(const std::vector<AdapterPayloadDiagnostic>& diagnostics, const ElementId& id,
                                  AdapterPayloadDiagnosticCode code) noexcept {
    return std::ranges::any_of(diagnostics, [&id, code](const AdapterPayloadDiagnostic& diagnostic) {
        return diagnostic.id == id && diagnostic.code == code;
    });
}

void append_text_placeholder_lines(const RendererTextRun& run, TextAdapterRow& row) {
    if (row.text.empty()) {
        return;
    }

    TextAdapterLine line;
    line.code_unit_offset = 0;
    line.code_unit_count = row.text.size();
    line.bounds = run.bounds;
    line.glyphs.push_back(TextAdapterGlyphPlaceholder{
        .code_unit_offset = 0,
        .code_unit_count = row.text.size(),
        .bounds = run.bounds,
    });
    row.lines.push_back(std::move(line));
}

struct TextScalarSpan {
    std::size_t code_unit_offset{0};
    std::size_t code_unit_count{0};
    std::uint32_t glyph{0};
    float advance{0.0F};
    bool whitespace{false};
};

struct MonospaceLineBuilder {
    TextAdapterLine line;
    float cursor_x{0.0F};
    float width{0.0F};
};

[[nodiscard]] bool is_ascii_whitespace(char value) noexcept {
    return value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\f' || value == '\v';
}

[[nodiscard]] bool is_utf8_continuation(unsigned char value) noexcept {
    return (value & 0xC0U) == 0x80U;
}

[[nodiscard]] std::size_t utf8_scalar_code_unit_count(std::string_view text, std::size_t offset) noexcept {
    if (offset >= text.size()) {
        return 0;
    }

    const auto first = static_cast<unsigned char>(text[offset]);
    if (first < 0x80U) {
        return 1;
    }

    if (first >= 0xC2U && first <= 0xDFU) {
        if (offset + 2 > text.size()) {
            return 1;
        }
        return is_utf8_continuation(static_cast<unsigned char>(text[offset + 1])) ? 2 : 1;
    }

    if (first >= 0xE0U && first <= 0xEFU) {
        if (offset + 3 > text.size()) {
            return 1;
        }
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);
        if (!is_utf8_continuation(third)) {
            return 1;
        }
        if (first == 0xE0U) {
            return second >= 0xA0U && second <= 0xBFU ? 3 : 1;
        }
        if (first == 0xEDU) {
            return second >= 0x80U && second <= 0x9FU ? 3 : 1;
        }
        return is_utf8_continuation(second) ? 3 : 1;
    }

    if (first >= 0xF0U && first <= 0xF4U) {
        if (offset + 4 > text.size()) {
            return 1;
        }
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);
        const auto fourth = static_cast<unsigned char>(text[offset + 3]);
        if (!is_utf8_continuation(third) || !is_utf8_continuation(fourth)) {
            return 1;
        }
        if (first == 0xF0U) {
            return second >= 0x90U && second <= 0xBFU ? 4 : 1;
        }
        if (first == 0xF4U) {
            return second >= 0x80U && second <= 0x8FU ? 4 : 1;
        }
        return is_utf8_continuation(second) ? 4 : 1;
    }

    return 1;
}

[[nodiscard]] std::optional<std::size_t> strict_utf8_scalar_code_unit_count(std::string_view text,
                                                                            std::size_t offset) noexcept {
    if (offset >= text.size()) {
        return std::nullopt;
    }

    const auto first = static_cast<unsigned char>(text[offset]);
    if (first < 0x80U) {
        return 1U;
    }

    if (first >= 0xC2U && first <= 0xDFU) {
        if (offset + 2U > text.size()) {
            return std::nullopt;
        }
        return is_utf8_continuation(static_cast<unsigned char>(text[offset + 1U])) ? std::optional<std::size_t>{2U}
                                                                                   : std::nullopt;
    }

    if (first >= 0xE0U && first <= 0xEFU) {
        if (offset + 3U > text.size()) {
            return std::nullopt;
        }
        const auto second = static_cast<unsigned char>(text[offset + 1U]);
        const auto third = static_cast<unsigned char>(text[offset + 2U]);
        if (!is_utf8_continuation(third)) {
            return std::nullopt;
        }
        if (first == 0xE0U) {
            return second >= 0xA0U && second <= 0xBFU ? std::optional<std::size_t>{3U} : std::nullopt;
        }
        if (first == 0xEDU) {
            return second >= 0x80U && second <= 0x9FU ? std::optional<std::size_t>{3U} : std::nullopt;
        }
        return is_utf8_continuation(second) ? std::optional<std::size_t>{3U} : std::nullopt;
    }

    if (first >= 0xF0U && first <= 0xF4U) {
        if (offset + 4U > text.size()) {
            return std::nullopt;
        }
        const auto second = static_cast<unsigned char>(text[offset + 1U]);
        const auto third = static_cast<unsigned char>(text[offset + 2U]);
        const auto fourth = static_cast<unsigned char>(text[offset + 3U]);
        if (!is_utf8_continuation(third) || !is_utf8_continuation(fourth)) {
            return std::nullopt;
        }
        if (first == 0xF0U) {
            return second >= 0x90U && second <= 0xBFU ? std::optional<std::size_t>{4U} : std::nullopt;
        }
        if (first == 0xF4U) {
            return second >= 0x80U && second <= 0x8FU ? std::optional<std::size_t>{4U} : std::nullopt;
        }
        return is_utf8_continuation(second) ? std::optional<std::size_t>{4U} : std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] bool is_strict_utf8(std::string_view text) noexcept {
    for (std::size_t offset = 0; offset < text.size();) {
        const auto count = strict_utf8_scalar_code_unit_count(text, offset);
        if (!count.has_value()) {
            return false;
        }
        offset += *count;
    }
    return true;
}

[[nodiscard]] bool is_utf8_scalar_boundary(std::string_view text, std::size_t offset) noexcept {
    if (offset > text.size()) {
        return false;
    }
    if (offset == 0U || offset == text.size()) {
        return true;
    }
    return !is_utf8_continuation(static_cast<unsigned char>(text[offset]));
}

[[nodiscard]] std::size_t previous_utf8_scalar_boundary(std::string_view text, std::size_t offset) noexcept {
    auto cursor = std::min(offset, text.size());
    if (cursor == 0U) {
        return 0U;
    }

    --cursor;
    while (cursor > 0U && is_utf8_continuation(static_cast<unsigned char>(text[cursor]))) {
        --cursor;
    }
    return cursor;
}

[[nodiscard]] std::size_t next_utf8_scalar_boundary(std::string_view text, std::size_t offset) noexcept {
    if (offset >= text.size()) {
        return text.size();
    }

    const auto code_unit_count = strict_utf8_scalar_code_unit_count(text, offset);
    if (!code_unit_count.has_value()) {
        return offset;
    }
    return std::min(text.size(), offset + *code_unit_count);
}

[[nodiscard]] bool is_valid_text_edit_command_kind(TextEditCommandKind kind) noexcept {
    switch (kind) {
    case TextEditCommandKind::move_cursor_backward:
    case TextEditCommandKind::move_cursor_forward:
    case TextEditCommandKind::move_cursor_to_start:
    case TextEditCommandKind::move_cursor_to_end:
    case TextEditCommandKind::delete_backward:
    case TextEditCommandKind::delete_forward:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_valid_text_edit_clipboard_command_kind(TextEditClipboardCommandKind kind) noexcept {
    switch (kind) {
    case TextEditClipboardCommandKind::copy_selection:
    case TextEditClipboardCommandKind::cut_selection:
    case TextEditClipboardCommandKind::paste_text:
        return true;
    }
    return false;
}

[[nodiscard]] std::uint32_t utf8_scalar_glyph(std::string_view text, std::size_t offset,
                                              std::size_t code_unit_count) noexcept {
    if (offset >= text.size()) {
        return 0;
    }

    const auto first = static_cast<unsigned char>(text[offset]);
    if (code_unit_count == 1 || first < 0x80U) {
        return first;
    }
    if (code_unit_count == 2 && offset + 1 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        return ((first & 0x1FU) << 6U) | (second & 0x3FU);
    }
    if (code_unit_count == 3 && offset + 2 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);
        return ((first & 0x0FU) << 12U) | ((second & 0x3FU) << 6U) | (third & 0x3FU);
    }
    if (code_unit_count == 4 && offset + 3 < text.size()) {
        const auto second = static_cast<unsigned char>(text[offset + 1]);
        const auto third = static_cast<unsigned char>(text[offset + 2]);
        const auto fourth = static_cast<unsigned char>(text[offset + 3]);
        return ((first & 0x07U) << 18U) | ((second & 0x3FU) << 12U) | ((third & 0x3FU) << 6U) | (fourth & 0x3FU);
    }
    return first;
}

[[nodiscard]] std::vector<TextScalarSpan> make_text_scalar_spans(std::string_view text,
                                                                 MonospaceTextLayoutPolicy policy) {
    std::vector<TextScalarSpan> spans;
    spans.reserve(text.size());

    for (std::size_t offset = 0; offset < text.size();) {
        const auto count = utf8_scalar_code_unit_count(text, offset);
        const auto code_unit_count = count == 0 ? 1 : count;
        const bool whitespace = count == 1 && is_ascii_whitespace(text[offset]);
        spans.push_back(TextScalarSpan{
            .code_unit_offset = offset,
            .code_unit_count = code_unit_count,
            .glyph = utf8_scalar_glyph(text, offset, code_unit_count),
            .advance = whitespace ? policy.whitespace_advance : policy.glyph_advance,
            .whitespace = whitespace,
        });
        offset += spans.back().code_unit_count;
    }

    return spans;
}

[[nodiscard]] float spans_width(const std::vector<TextScalarSpan>& spans, std::size_t begin, std::size_t end) noexcept {
    float width = 0.0F;
    for (std::size_t index = begin; index < end; ++index) {
        width += spans[index].advance;
    }
    return width;
}

[[nodiscard]] std::size_t skip_whitespace_spans(const std::vector<TextScalarSpan>& spans, std::size_t index) noexcept {
    while (index < spans.size() && spans[index].whitespace) {
        ++index;
    }
    return index;
}

[[nodiscard]] std::size_t find_word_end(const std::vector<TextScalarSpan>& spans, std::size_t index) noexcept {
    while (index < spans.size() && !spans[index].whitespace) {
        ++index;
    }
    return index;
}

[[nodiscard]] std::size_t find_whitespace_end(const std::vector<TextScalarSpan>& spans, std::size_t index) noexcept {
    while (index < spans.size() && spans[index].whitespace) {
        ++index;
    }
    return index;
}

[[nodiscard]] MonospaceLineBuilder make_monospace_line(Rect run_bounds, float line_y,
                                                       MonospaceTextLayoutPolicy policy) {
    MonospaceLineBuilder builder;
    builder.line.bounds = Rect{.x = run_bounds.x, .y = line_y, .width = 0.0F, .height = policy.line_height};
    builder.cursor_x = run_bounds.x;
    return builder;
}

void append_span_to_line(const TextScalarSpan& span, MonospaceLineBuilder& builder, MonospaceTextLayoutPolicy policy) {
    if (builder.line.glyphs.empty()) {
        builder.line.code_unit_offset = span.code_unit_offset;
    }

    builder.line.glyphs.push_back(TextAdapterGlyphPlaceholder{
        .code_unit_offset = span.code_unit_offset,
        .code_unit_count = span.code_unit_count,
        .bounds =
            Rect{
                .x = builder.cursor_x, .y = builder.line.bounds.y, .width = span.advance, .height = policy.line_height},
        .glyph = span.glyph,
    });
    builder.cursor_x += span.advance;
    builder.width += span.advance;
    builder.line.bounds.width = builder.width;
    builder.line.code_unit_count = (span.code_unit_offset + span.code_unit_count) - builder.line.code_unit_offset;
}

void append_span_range_to_line(const std::vector<TextScalarSpan>& spans, std::size_t begin, std::size_t end,
                               MonospaceLineBuilder& builder, MonospaceTextLayoutPolicy policy) {
    for (std::size_t index = begin; index < end; ++index) {
        append_span_to_line(spans[index], builder, policy);
    }
}

void append_layout_clipped_diagnostic(const ElementId& id, std::vector<AdapterPayloadDiagnostic>& diagnostics) {
    diagnostics.push_back(AdapterPayloadDiagnostic{
        .id = id,
        .code = AdapterPayloadDiagnosticCode::text_layout_clipped,
        .message = "text layout clipped content outside the available bounds",
    });
}

void append_monospace_clip_layout(const RendererTextRun& run, TextAdapterRow& row, MonospaceTextLayoutPolicy policy,
                                  std::vector<AdapterPayloadDiagnostic>& diagnostics) {
    const auto spans = make_text_scalar_spans(row.text, policy);
    if (spans.empty()) {
        return;
    }

    if (policy.line_height > run.bounds.height) {
        append_layout_clipped_diagnostic(row.id, diagnostics);
        return;
    }

    auto line = make_monospace_line(run.bounds, run.bounds.y, policy);
    std::size_t index = 0;
    for (; index < spans.size(); ++index) {
        if (line.width + spans[index].advance > run.bounds.width) {
            break;
        }
        append_span_to_line(spans[index], line, policy);
    }

    if (!line.line.glyphs.empty()) {
        row.lines.push_back(std::move(line.line));
    }
    if (index < spans.size()) {
        append_layout_clipped_diagnostic(row.id, diagnostics);
    }
}

void append_monospace_wrap_layout(const RendererTextRun& run, TextAdapterRow& row, MonospaceTextLayoutPolicy policy,
                                  std::vector<AdapterPayloadDiagnostic>& diagnostics) {
    const auto spans = make_text_scalar_spans(row.text, policy);
    if (spans.empty()) {
        return;
    }

    const float bottom = run.bounds.y + run.bounds.height;
    float line_y = run.bounds.y;
    std::size_t index = 0;
    bool clipped = false;

    while (index < spans.size()) {
        index = skip_whitespace_spans(spans, index);
        if (index >= spans.size()) {
            break;
        }
        if (line_y + policy.line_height > bottom) {
            clipped = true;
            break;
        }

        auto line = make_monospace_line(run.bounds, line_y, policy);
        const auto line_start_index = index;

        while (index < spans.size()) {
            if (spans[index].whitespace) {
                const auto whitespace_begin = index;
                const auto whitespace_end = find_whitespace_end(spans, index);
                const auto word_begin = skip_whitespace_spans(spans, whitespace_end);
                if (word_begin >= spans.size()) {
                    index = spans.size();
                    break;
                }

                const auto word_end = find_word_end(spans, word_begin);
                const float group_width =
                    spans_width(spans, whitespace_begin, whitespace_end) + spans_width(spans, word_begin, word_end);
                if (!line.line.glyphs.empty() && line.width + group_width <= run.bounds.width) {
                    append_span_range_to_line(spans, whitespace_begin, whitespace_end, line, policy);
                    append_span_range_to_line(spans, word_begin, word_end, line, policy);
                    index = word_end;
                    continue;
                }

                index = word_begin;
                break;
            }

            const auto word_begin = index;
            const auto word_end = find_word_end(spans, word_begin);
            const float word_width = spans_width(spans, word_begin, word_end);
            if (line.line.glyphs.empty() && word_width <= run.bounds.width) {
                append_span_range_to_line(spans, word_begin, word_end, line, policy);
                index = word_end;
                continue;
            }
            if (!line.line.glyphs.empty() && line.width + word_width <= run.bounds.width) {
                append_span_range_to_line(spans, word_begin, word_end, line, policy);
                index = word_end;
                continue;
            }

            if (line.line.glyphs.empty()) {
                while (index < word_end && line.width + spans[index].advance <= run.bounds.width) {
                    append_span_to_line(spans[index], line, policy);
                    ++index;
                }
                if (line.line.glyphs.empty()) {
                    clipped = true;
                    index = spans.size();
                }
            }
            break;
        }

        if (line.line.glyphs.empty() && index == line_start_index) {
            clipped = true;
            break;
        }
        if (!line.line.glyphs.empty()) {
            row.lines.push_back(std::move(line.line));
            line_y += policy.line_height;
        }
    }

    if (clipped) {
        append_layout_clipped_diagnostic(row.id, diagnostics);
    }
}

[[nodiscard]] std::size_t element_depth(const UiDocument& document, const Element& element) noexcept {
    std::size_t depth = 0;
    const auto* current = &element;
    while (current != nullptr && !current->parent.value.empty()) {
        ++depth;
        current = document.find(current->parent);
    }
    return depth;
}

[[nodiscard]] Rect inset_rect(Rect rect, EdgeInsets insets) noexcept {
    const float horizontal = insets.left + insets.right;
    const float vertical = insets.top + insets.bottom;
    return Rect{
        .x = rect.x + insets.left,
        .y = rect.y + insets.top,
        .width = std::max(0.0F, rect.width - horizontal),
        .height = std::max(0.0F, rect.height - vertical),
    };
}

[[nodiscard]] Size requested_layout_size(const Element& element, Size fallback) noexcept {
    const Size requested{
        .width = element.bounds.width > 0.0F ? element.bounds.width : fallback.width,
        .height = element.bounds.height > 0.0F ? element.bounds.height : fallback.height,
    };
    return constrain_size(requested, element.style.size);
}

[[nodiscard]] Rect anchored_rect(const Element& element, Rect content) noexcept {
    const auto margin = element.style.margin;
    const auto available = inset_rect(content, margin);

    if (element.style.anchor == AnchorMode::fill) {
        const auto size =
            constrain_size(Size{.width = available.width, .height = available.height}, element.style.size);
        return Rect{.x = available.x, .y = available.y, .width = size.width, .height = size.height};
    }

    const auto size =
        requested_layout_size(element, Size{.width = element.bounds.width, .height = element.bounds.height});
    switch (element.style.anchor) {
    case AnchorMode::top_left:
        return Rect{.x = available.x + element.bounds.x,
                    .y = available.y + element.bounds.y,
                    .width = size.width,
                    .height = size.height};
    case AnchorMode::top_right:
        return Rect{.x = available.x + available.width - size.width - element.bounds.x,
                    .y = available.y + element.bounds.y,
                    .width = size.width,
                    .height = size.height};
    case AnchorMode::bottom_left:
        return Rect{.x = available.x + element.bounds.x,
                    .y = available.y + available.height - size.height - element.bounds.y,
                    .width = size.width,
                    .height = size.height};
    case AnchorMode::bottom_right:
        return Rect{.x = available.x + available.width - size.width - element.bounds.x,
                    .y = available.y + available.height - size.height - element.bounds.y,
                    .width = size.width,
                    .height = size.height};
    case AnchorMode::center:
        return Rect{.x = available.x + ((available.width - size.width) * 0.5F) + element.bounds.x,
                    .y = available.y + ((available.height - size.height) * 0.5F) + element.bounds.y,
                    .width = size.width,
                    .height = size.height};
    case AnchorMode::fill:
        break;
    }
    return Rect{.x = available.x, .y = available.y, .width = size.width, .height = size.height};
}

void append_layout(const UiDocument& document, const Element& element, Rect bounds, LayoutResult& result) {
    result.elements.push_back(ElementLayout{.id = element.id,
                                            .parent = element.parent,
                                            .role = element.role,
                                            .bounds = bounds,
                                            .visible = element.visible});

    if (!element.visible) {
        return;
    }

    const auto content = inset_rect(bounds, element.style.padding);
    switch (element.style.layout) {
    case LayoutMode::row: {
        float cursor_x = content.x;
        bool first_child = true;
        for (const auto& child_id : element.children) {
            const auto* child = document.find(child_id);
            if (child == nullptr) {
                continue;
            }
            if (!first_child) {
                cursor_x += element.style.gap;
            }
            first_child = false;
            const auto margin = child->style.margin;
            const auto fallback_width = std::max(0.0F, content.width - margin.left - margin.right);
            const auto fallback_height = std::max(0.0F, content.height - margin.top - margin.bottom);
            const auto size = requested_layout_size(*child, Size{.width = fallback_width, .height = fallback_height});
            const auto child_bounds = Rect{
                .x = cursor_x + margin.left, .y = content.y + margin.top, .width = size.width, .height = size.height};
            append_layout(document, *child, child_bounds, result);
            cursor_x = child_bounds.x + child_bounds.width + margin.right;
        }
        break;
    }
    case LayoutMode::column: {
        float cursor_y = content.y;
        bool first_child = true;
        for (const auto& child_id : element.children) {
            const auto* child = document.find(child_id);
            if (child == nullptr) {
                continue;
            }
            if (!first_child) {
                cursor_y += element.style.gap;
            }
            first_child = false;
            const auto margin = child->style.margin;
            const auto fallback_width = std::max(0.0F, content.width - margin.left - margin.right);
            const auto fallback_height = std::max(0.0F, content.height - margin.top - margin.bottom);
            const auto size = requested_layout_size(*child, Size{.width = fallback_width, .height = fallback_height});
            const auto child_bounds = Rect{
                .x = content.x + margin.left, .y = cursor_y + margin.top, .width = size.width, .height = size.height};
            append_layout(document, *child, child_bounds, result);
            cursor_y = child_bounds.y + child_bounds.height + margin.bottom;
        }
        break;
    }
    case LayoutMode::stack:
    case LayoutMode::none:
        for (const auto& child_id : element.children) {
            const auto* child = document.find(child_id);
            if (child != nullptr) {
                append_layout(document, *child, anchored_rect(*child, content), result);
            }
        }
        break;
    }
}

} // namespace

bool empty(const ElementId& id) noexcept {
    return id.value.empty();
}

bool UiDocument::try_add_element(ElementDesc desc) {
    if (desc.id.value.empty() || find(desc.id) != nullptr || desc.role == SemanticRole::none ||
        !is_valid_rect(desc.bounds) || !is_valid_style(desc.style) || !is_valid_text_content(desc.text) ||
        !is_valid_image_content(desc.image)) {
        return false;
    }

    if (!desc.parent.value.empty() && find(desc.parent) == nullptr) {
        return false;
    }

    Element element;
    element.id = std::move(desc.id);
    element.parent = std::move(desc.parent);
    element.role = desc.role;
    element.bounds = desc.bounds;
    element.visible = desc.visible;
    element.enabled = desc.enabled;
    element.text = std::move(desc.text);
    element.image = std::move(desc.image);
    element.accessibility_label = std::move(desc.accessibility_label);
    element.style = std::move(desc.style);

    if (!element.parent.value.empty()) {
        auto* parent = find_mutable(element.parent);
        if (parent == nullptr) {
            return false;
        }
        parent->children.push_back(element.id);
    }

    elements_.push_back(std::move(element));
    return true;
}

const Element* UiDocument::find(const ElementId& id) const noexcept {
    const auto it = std::ranges::find_if(elements_, [&id](const Element& element) { return element.id == id; });
    return it == elements_.end() ? nullptr : &(*it);
}

std::size_t UiDocument::size() const noexcept {
    return elements_.size();
}

std::vector<Element> UiDocument::traverse() const {
    std::vector<Element> result;
    result.reserve(elements_.size());
    for (const auto& element : elements_) {
        if (element.parent.value.empty()) {
            append_subtree(element, result);
        }
    }
    return result;
}

bool UiDocument::set_visible(const ElementId& id, bool visible) noexcept {
    auto* element = find_mutable(id);
    if (element == nullptr) {
        return false;
    }
    element->visible = visible;
    return true;
}

bool UiDocument::set_enabled(const ElementId& id, bool enabled) noexcept {
    auto* element = find_mutable(id);
    if (element == nullptr) {
        return false;
    }
    element->enabled = enabled;
    return true;
}

bool UiDocument::set_text(const ElementId& id, TextContent text) {
    if (!is_valid_text_content(text)) {
        return false;
    }

    auto* element = find_mutable(id);
    if (element == nullptr) {
        return false;
    }
    element->text = std::move(text);
    return true;
}

bool UiDocument::is_descendant_or_same(const ElementId& ancestor, const ElementId& id) const noexcept {
    if (ancestor.value.empty() || id.value.empty()) {
        return false;
    }
    if (ancestor == id) {
        return true;
    }

    const auto* current = find(id);
    while (current != nullptr && !current->parent.value.empty()) {
        if (current->parent == ancestor) {
            return true;
        }
        current = find(current->parent);
    }
    return false;
}

Element* UiDocument::find_mutable(const ElementId& id) noexcept {
    const auto it = std::ranges::find_if(elements_, [&id](const Element& element) { return element.id == id; });
    return it == elements_.end() ? nullptr : &(*it);
}

void UiDocument::append_subtree(const Element& element, std::vector<Element>& out) const {
    out.push_back(element);
    for (const auto& child_id : element.children) {
        const auto* child = find(child_id);
        if (child != nullptr) {
            append_subtree(*child, out);
        }
    }
}

bool is_valid_rect(Rect rect) noexcept {
    return std::isfinite(rect.x) && std::isfinite(rect.y) && is_finite_nonnegative(rect.width) &&
           is_finite_nonnegative(rect.height);
}

bool is_valid_style(const Style& style) noexcept {
    return is_valid_edge_insets(style.margin) && is_valid_edge_insets(style.padding) &&
           is_finite_nonnegative(style.gap) && is_valid_size_constraints(style.size) &&
           std::isfinite(style.dpi_scale) && style.dpi_scale > 0.0F;
}

bool is_valid_text_content(const TextContent& text) noexcept {
    return is_valid_adapter_string(text.label) && is_valid_adapter_string(text.localization_key) &&
           is_valid_adapter_string(text.font_family);
}

bool is_valid_monospace_text_layout_policy(MonospaceTextLayoutPolicy policy) noexcept {
    return std::isfinite(policy.glyph_advance) && policy.glyph_advance > 0.0F &&
           std::isfinite(policy.whitespace_advance) && policy.whitespace_advance > 0.0F &&
           std::isfinite(policy.line_height) && policy.line_height > 0.0F;
}

bool is_valid_image_content(const ImageContent& image) noexcept {
    return is_valid_adapter_string(image.resource_id) && is_valid_adapter_string(image.asset_uri) &&
           is_valid_adapter_string(image.tint_token);
}

Size constrain_size(Size size, SizeConstraints constraints) noexcept {
    if (!std::isfinite(size.width)) {
        size.width = 0.0F;
    }
    if (!std::isfinite(size.height)) {
        size.height = 0.0F;
    }

    size.width = std::max(size.width, constraints.min_width);
    size.height = std::max(size.height, constraints.min_height);
    if (constraints.max_width > 0.0F) {
        size.width = std::min(size.width, constraints.max_width);
    }
    if (constraints.max_height > 0.0F) {
        size.height = std::min(size.height, constraints.max_height);
    }
    return size;
}

Style resolve_style(const Style& parent, const Style& element) {
    Style resolved = element;
    if (resolved.layout == LayoutMode::none) {
        resolved.layout = parent.layout;
    }
    if (resolved.background_token.empty()) {
        resolved.background_token = parent.background_token;
    }
    if (resolved.foreground_token.empty()) {
        resolved.foreground_token = parent.foreground_token;
    }
    if (resolved.size.min_width == 0.0F && resolved.size.min_height == 0.0F && resolved.size.max_width == 0.0F &&
        resolved.size.max_height == 0.0F) {
        resolved.size = parent.size;
    }
    if (resolved.dpi_scale == 1.0F) {
        resolved.dpi_scale = parent.dpi_scale;
    }
    return resolved;
}

std::string_view semantic_role_id(SemanticRole role) noexcept {
    switch (role) {
    case SemanticRole::none:
        return "none";
    case SemanticRole::root:
        return "root";
    case SemanticRole::panel:
        return "panel";
    case SemanticRole::button:
        return "button";
    case SemanticRole::label:
        return "label";
    case SemanticRole::text_field:
        return "text_field";
    case SemanticRole::list:
        return "list";
    case SemanticRole::list_item:
        return "list_item";
    case SemanticRole::image:
        return "image";
    case SemanticRole::checkbox:
        return "checkbox";
    case SemanticRole::slider:
        return "slider";
    case SemanticRole::dialog:
        return "dialog";
    }
    return "unknown";
}

const ElementLayout* find_layout(const LayoutResult& layout, const ElementId& id) noexcept {
    const auto it =
        std::ranges::find_if(layout.elements, [&id](const ElementLayout& element) { return element.id == id; });
    return it == layout.elements.end() ? nullptr : &(*it);
}

LayoutResult solve_layout(const UiDocument& document, const ElementId& root, Rect viewport) {
    LayoutResult result;
    if (!is_valid_rect(viewport)) {
        return result;
    }

    const auto* root_element = document.find(root);
    if (root_element == nullptr) {
        return result;
    }

    append_layout(document, *root_element, viewport, result);
    return result;
}

std::vector<AdapterContract> required_adapter_contracts() {
    return {
        AdapterContract{
            .boundary = AdapterBoundary::text_shaping,
            .id = "text_shaping",
            .purpose = "Complex text shaping for scripts and glyph clusters.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::bidirectional_text,
            .id = "bidirectional_text",
            .purpose = "Bidirectional text ordering and directional run resolution.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::line_breaking,
            .id = "line_breaking",
            .purpose = "Locale-aware text segmentation and line break decisions.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::font_rasterization,
            .id = "font_rasterization",
            .purpose = "Font loading, glyph rasterization, and font licensing records.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::glyph_atlas,
            .id = "glyph_atlas",
            .purpose = "Glyph atlas allocation and cache lifecycle.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::ime_composition,
            .id = "ime_composition",
            .purpose = "Platform IME composition and text input sessions.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::accessibility_bridge,
            .id = "accessibility_bridge",
            .purpose = "OS accessibility tree publication and screen-reader integration.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::image_decoding,
            .id = "image_decoding",
            .purpose = "UI bitmap or vector asset decoding before renderer upload.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::renderer_submission,
            .id = "renderer_submission",
            .purpose = "Renderer or RHI draw-data submission for retained UI elements.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::clipboard,
            .id = "clipboard",
            .purpose = "Text clipboard read/write operations behind platform adapters.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::platform_integration,
            .id = "platform_integration",
            .purpose = "Window, DPI, safe-area, cursor, and platform text-input glue.",
        },
        AdapterContract{
            .boundary = AdapterBoundary::middleware,
            .id = "middleware",
            .purpose = "Optional Qt, NoesisGUI, Slint, RmlUi, or similar adapters.",
        },
    };
}

RendererSubmission build_renderer_submission(const UiDocument& document, const LayoutResult& layout) {
    RendererSubmission submission;
    submission.elements.reserve(layout.elements.size());
    submission.layouts.reserve(layout.elements.size());
    submission.boxes.reserve(layout.elements.size());
    submission.text_runs.reserve(layout.elements.size());

    for (const auto& layout_element : layout.elements) {
        if (!layout_element.visible) {
            continue;
        }

        const auto* element = document.find(layout_element.id);
        if (element == nullptr || !element->visible) {
            continue;
        }

        Element submitted = *element;
        submitted.bounds = layout_element.bounds;
        submission.elements.push_back(std::move(submitted));
        submission.layouts.push_back(layout_element);

        const auto& submitted_element = submission.elements.back();
        submission.boxes.push_back(RendererBox{
            .id = submitted_element.id,
            .role = submitted_element.role,
            .bounds = layout_element.bounds,
            .background_token = submitted_element.style.background_token,
            .foreground_token = submitted_element.style.foreground_token,
            .enabled = submitted_element.enabled,
        });

        if (has_text_run(submitted_element)) {
            submission.text_runs.push_back(RendererTextRun{
                .id = submitted_element.id,
                .bounds = layout_element.bounds,
                .text = submitted_element.text,
                .foreground_token = submitted_element.style.foreground_token,
                .enabled = submitted_element.enabled,
            });
        }

        if (has_image_placeholder(submitted_element)) {
            submission.image_placeholders.push_back(RendererImagePlaceholder{
                .id = submitted_element.id,
                .bounds = layout_element.bounds,
                .image = submitted_element.image,
                .enabled = submitted_element.enabled,
            });
        }

        if (is_accessibility_candidate(submitted_element)) {
            submission.accessibility_nodes.push_back(AccessibilityNode{
                .id = submitted_element.id,
                .role = submitted_element.role,
                .label = accessibility_label_for(submitted_element),
                .bounds = layout_element.bounds,
                .localization_key = submitted_element.text.localization_key,
                .enabled = submitted_element.enabled,
                .focusable = submitted_element.enabled && is_focusable_role(submitted_element.role),
                .parent = submitted_element.parent,
                .depth = element_depth(document, submitted_element),
            });
        }
    }

    return submission;
}

TextAdapterPayload build_text_adapter_payload(const RendererSubmission& submission) {
    TextAdapterPayload payload;
    payload.rows.reserve(submission.text_runs.size());

    for (const auto& run : submission.text_runs) {
        TextAdapterRow row;
        row.id = run.id;
        row.bounds = run.bounds;
        row.text = text_payload_value(run.text);
        row.localization_key = run.text.localization_key;
        row.font_family = run.text.font_family;
        row.direction = run.text.direction;
        row.wrap = run.text.wrap;
        row.foreground_token = run.foreground_token;
        row.enabled = run.enabled;

        if (!is_positive_rect(run.bounds)) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::invalid_text_bounds,
                .message = "text adapter row has invalid or non-positive bounds",
            });
        } else if (row.text.empty() && !row.localization_key.empty()) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::unresolved_text_localization_key,
                .message = "text adapter row has a localization key but no resolved label",
            });
        } else if (row.text.empty()) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::empty_text_payload,
                .message = "text adapter row has no resolved label or localization key",
            });
        } else {
            append_text_placeholder_lines(run, row);
        }

        payload.rows.push_back(std::move(row));
    }

    return payload;
}

TextAdapterPayload build_text_adapter_payload(const RendererSubmission& submission, MonospaceTextLayoutPolicy policy) {
    TextAdapterPayload payload;
    payload.rows.reserve(submission.text_runs.size());

    for (const auto& run : submission.text_runs) {
        TextAdapterRow row;
        row.id = run.id;
        row.bounds = run.bounds;
        row.text = text_payload_value(run.text);
        row.localization_key = run.text.localization_key;
        row.font_family = run.text.font_family;
        row.direction = run.text.direction;
        row.wrap = run.text.wrap;
        row.foreground_token = run.foreground_token;
        row.enabled = run.enabled;

        if (!is_positive_rect(run.bounds)) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::invalid_text_bounds,
                .message = "text adapter row has invalid or non-positive bounds",
            });
        } else if (row.text.empty() && !row.localization_key.empty()) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::unresolved_text_localization_key,
                .message = "text adapter row has a localization key but no resolved label",
            });
        } else if (row.text.empty()) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::empty_text_payload,
                .message = "text adapter row has no resolved label or localization key",
            });
        } else if (!is_valid_monospace_text_layout_policy(policy)) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::invalid_text_layout_policy,
                .message = "monospace text layout policy requires positive finite metrics",
            });
        } else {
            if (row.direction == TextDirection::right_to_left) {
                payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                    .id = row.id,
                    .code = AdapterPayloadDiagnosticCode::unsupported_text_direction,
                    .message = "monospace text layout uses logical-order fallback for right-to-left text",
                });
            }

            if (row.wrap == TextWrapMode::wrap) {
                append_monospace_wrap_layout(run, row, policy, payload.diagnostics);
            } else {
                append_monospace_clip_layout(run, row, policy, payload.diagnostics);
            }
        }

        payload.rows.push_back(std::move(row));
    }

    return payload;
}

ImageAdapterPayload build_image_adapter_payload(const RendererSubmission& submission) {
    ImageAdapterPayload payload;
    payload.rows.reserve(submission.image_placeholders.size());

    for (const auto& placeholder : submission.image_placeholders) {
        ImageAdapterRow row{
            .id = placeholder.id,
            .bounds = placeholder.bounds,
            .resource_id = placeholder.image.resource_id,
            .asset_uri = placeholder.image.asset_uri,
            .tint_token = placeholder.image.tint_token,
            .enabled = placeholder.enabled,
        };
        if (row.resource_id.empty() && row.asset_uri.empty()) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::empty_image_reference,
                .message = "image adapter row has no resource id or asset uri",
            });
        }
        if (!is_positive_rect(row.bounds)) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = row.id,
                .code = AdapterPayloadDiagnosticCode::invalid_image_bounds,
                .message = "image adapter row has invalid or non-positive bounds",
            });
        }
        payload.rows.push_back(std::move(row));
    }

    return payload;
}

AccessibilityPayload build_accessibility_payload(const RendererSubmission& submission) {
    AccessibilityPayload payload;
    payload.nodes = submission.accessibility_nodes;
    for (const auto& node : payload.nodes) {
        if (!is_positive_rect(node.bounds)) {
            payload.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = node.id,
                .code = AdapterPayloadDiagnosticCode::invalid_accessibility_bounds,
                .message = "accessibility node has invalid or non-positive bounds",
            });
        }
    }
    return payload;
}

bool ImageDecodeRequestPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool TextShapingRequestPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool TextShapingResult::succeeded() const noexcept {
    return shaped && !runs.empty() && diagnostics.empty();
}

bool ImageDecodeDispatchResult::succeeded() const noexcept {
    return decoded && image.has_value() && diagnostics.empty();
}

TextShapingRequestPlan plan_text_shaping_request(const TextLayoutRequest& request) {
    TextShapingRequestPlan plan;
    plan.request = request;

    if (plan.request.text.empty() || !is_valid_adapter_string(plan.request.text)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"text.shaping"},
            .code = AdapterPayloadDiagnosticCode::invalid_text_shaping_text,
            .message = "text shaping request text must be non-empty and adapter-safe",
        });
    }
    if (plan.request.font_family.empty() || !is_valid_adapter_string(plan.request.font_family)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"text.shaping"},
            .code = AdapterPayloadDiagnosticCode::invalid_text_shaping_font_family,
            .message = "text shaping request font family must be non-empty and adapter-safe",
        });
    }
    if (!std::isfinite(plan.request.max_width) || plan.request.max_width < 0.0F) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"text.shaping"},
            .code = AdapterPayloadDiagnosticCode::invalid_text_shaping_max_width,
            .message = "text shaping request max width must be finite and non-negative",
        });
    }

    return plan;
}

TextShapingResult shape_text_run(ITextShapingAdapter& adapter, const TextLayoutRequest& request) {
    const auto plan = plan_text_shaping_request(request);
    TextShapingResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    result.runs = adapter.shape_text(plan.request);
    result.shaped = true;
    if (!is_valid_text_shaping_result(plan.request.text, result.runs)) {
        result.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"text.shaping"},
            .code = AdapterPayloadDiagnosticCode::invalid_text_shaping_result,
            .message = "text shaping adapter returned missing or invalid text layout runs",
        });
    }

    return result;
}

ImageDecodeRequestPlan plan_image_decode_request(const ImageDecodeRequest& request) {
    ImageDecodeRequestPlan plan;
    plan.request = request;

    if (plan.request.asset_uri.empty() || !is_valid_adapter_string(plan.request.asset_uri)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"image.decode"},
            .code = AdapterPayloadDiagnosticCode::invalid_image_decode_uri,
            .message = "image decode request asset uri must be non-empty and adapter-safe",
        });
    }
    if (plan.request.bytes.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"image.decode"},
            .code = AdapterPayloadDiagnosticCode::empty_image_decode_bytes,
            .message = "image decode request bytes must not be empty",
        });
    }

    return plan;
}

ImageDecodeDispatchResult decode_image_request(IImageDecodingAdapter& adapter, const ImageDecodeRequest& request) {
    const auto plan = plan_image_decode_request(request);
    ImageDecodeDispatchResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    result.image = adapter.decode_image(plan.request);
    result.decoded = true;
    if (!result.image.has_value() || !is_valid_image_decode_result(*result.image)) {
        result.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"image.decode"},
            .code = AdapterPayloadDiagnosticCode::invalid_image_decode_result,
            .message = "image decode adapter returned missing or invalid image data",
        });
    }
    return result;
}

bool FontRasterizationRequestPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool FontRasterizationResult::succeeded() const noexcept {
    return rasterized && allocation.has_value() && diagnostics.empty();
}

FontRasterizationRequestPlan plan_font_rasterization_request(const FontRasterizationRequest& request) {
    FontRasterizationRequestPlan plan;
    plan.request = request;

    if (plan.request.font_family.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"font.rasterization"},
            .code = AdapterPayloadDiagnosticCode::invalid_font_family,
            .message = "font rasterization request font family must not be empty",
        });
    }
    if (plan.request.glyph == 0U) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"font.rasterization"},
            .code = AdapterPayloadDiagnosticCode::invalid_font_glyph,
            .message = "font rasterization request glyph must not be zero",
        });
    }
    if (!std::isfinite(plan.request.pixel_size) || plan.request.pixel_size <= 0.0F) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"font.rasterization"},
            .code = AdapterPayloadDiagnosticCode::invalid_font_pixel_size,
            .message = "font rasterization request pixel size must be finite and positive",
        });
    }

    return plan;
}

FontRasterizationResult rasterize_font_glyph(IFontRasterizerAdapter& adapter, const FontRasterizationRequest& request) {
    const auto plan = plan_font_rasterization_request(request);
    FontRasterizationResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    result.allocation = adapter.rasterize_glyph(plan.request);
    result.rasterized = true;
    if (result.allocation->glyph != plan.request.glyph || !is_positive_rect(result.allocation->atlas_bounds)) {
        result.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = ElementId{"font.rasterization"},
            .code = AdapterPayloadDiagnosticCode::invalid_font_allocation,
            .message = "font rasterization adapter returned an invalid glyph atlas allocation",
        });
    }
    return result;
}

bool ImeCompositionPublishPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool ImeCompositionPublishResult::succeeded() const noexcept {
    return published && diagnostics.empty();
}

ImeCompositionPublishPlan plan_ime_composition_update(const ImeComposition& composition) {
    ImeCompositionPublishPlan plan;
    plan.composition = composition;

    if (plan.composition.target.value.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.composition.target,
            .code = AdapterPayloadDiagnosticCode::invalid_ime_target,
            .message = "ime composition target must not be empty",
        });
    }
    if (plan.composition.cursor_index > plan.composition.composition_text.size()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.composition.target,
            .code = AdapterPayloadDiagnosticCode::invalid_ime_cursor,
            .message = "ime composition cursor index must be within the composition text",
        });
    }

    return plan;
}

ImeCompositionPublishResult publish_ime_composition(IImeAdapter& adapter, const ImeComposition& composition) {
    const auto plan = plan_ime_composition_update(composition);
    ImeCompositionPublishResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    adapter.update_composition(plan.composition);
    result.published = true;
    return result;
}

bool PlatformTextInputSessionPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool PlatformTextInputSessionResult::succeeded() const noexcept {
    return begun && diagnostics.empty();
}

bool PlatformTextInputEndPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool PlatformTextInputEndResult::succeeded() const noexcept {
    return ended && diagnostics.empty();
}

PlatformTextInputSessionPlan plan_platform_text_input_session(const PlatformTextInputRequest& request) {
    PlatformTextInputSessionPlan plan;
    plan.request = request;

    if (plan.request.target.value.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.request.target,
            .code = AdapterPayloadDiagnosticCode::invalid_platform_text_input_target,
            .message = "platform text input target must not be empty",
        });
    }
    if (!is_positive_rect(plan.request.text_bounds)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.request.target,
            .code = AdapterPayloadDiagnosticCode::invalid_platform_text_input_bounds,
            .message = "platform text input bounds must be finite and positive",
        });
    }

    return plan;
}

PlatformTextInputSessionResult begin_platform_text_input(IPlatformIntegrationAdapter& adapter,
                                                         const PlatformTextInputRequest& request) {
    const auto plan = plan_platform_text_input_session(request);
    PlatformTextInputSessionResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    adapter.begin_text_input(plan.request);
    result.begun = true;
    return result;
}

PlatformTextInputEndPlan plan_platform_text_input_end(const ElementId& target) {
    PlatformTextInputEndPlan plan;
    plan.target = target;

    if (plan.target.value.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.target,
            .code = AdapterPayloadDiagnosticCode::invalid_platform_text_input_target,
            .message = "platform text input target must not be empty",
        });
    }

    return plan;
}

PlatformTextInputEndResult end_platform_text_input(IPlatformIntegrationAdapter& adapter, const ElementId& target) {
    const auto plan = plan_platform_text_input_end(target);
    PlatformTextInputEndResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    adapter.end_text_input(plan.target);
    result.ended = true;
    return result;
}

bool TextEditCommitPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool TextEditCommitResult::succeeded() const noexcept {
    return committed && diagnostics.empty();
}

TextEditCommitPlan plan_committed_text_input(const TextEditState& state, const CommittedTextInput& input) {
    TextEditCommitPlan plan;
    plan.state = state;
    plan.input = input;

    if (plan.state.target.value.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_target,
            .message = "text edit target must not be empty",
        });
    }
    if (plan.input.target != plan.state.target) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.input.target,
            .code = AdapterPayloadDiagnosticCode::mismatched_committed_text_target,
            .message = "committed text target must match the text edit state target",
        });
    }
    if (!is_strict_utf8(plan.state.text) || plan.state.cursor_byte_offset > plan.state.text.size() ||
        !is_utf8_scalar_boundary(plan.state.text, plan.state.cursor_byte_offset)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_cursor,
            .message = "text edit cursor must be within the UTF-8 text and on a scalar boundary",
        });
    }
    const auto selection_available =
        plan.state.cursor_byte_offset <= plan.state.text.size() &&
        plan.state.selection_byte_length <= plan.state.text.size() - plan.state.cursor_byte_offset;
    const auto selection_end =
        selection_available ? plan.state.cursor_byte_offset + plan.state.selection_byte_length : 0U;
    if (!selection_available || !is_strict_utf8(plan.state.text) ||
        !is_utf8_scalar_boundary(plan.state.text, selection_end)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_selection,
            .message = "text edit selection must fit the UTF-8 text and end on a scalar boundary",
        });
    }
    if (plan.input.text.empty() || !is_strict_utf8(plan.input.text)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.input.target,
            .code = AdapterPayloadDiagnosticCode::invalid_committed_text,
            .message = "committed text input must be non-empty valid UTF-8",
        });
    }

    return plan;
}

TextEditCommitResult apply_committed_text_input(const TextEditState& state, const CommittedTextInput& input) {
    const auto plan = plan_committed_text_input(state, input);
    TextEditCommitResult result;
    result.state = state;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    result.state.text.replace(plan.state.cursor_byte_offset, plan.state.selection_byte_length, plan.input.text);
    result.state.cursor_byte_offset = plan.state.cursor_byte_offset + plan.input.text.size();
    result.state.selection_byte_length = 0U;
    result.committed = true;
    return result;
}

bool TextEditCommandPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool TextEditCommandResult::succeeded() const noexcept {
    return applied && diagnostics.empty();
}

TextEditCommandPlan plan_text_edit_command(const TextEditState& state, const TextEditCommand& command) {
    TextEditCommandPlan plan;
    plan.state = state;
    plan.command = command;

    if (plan.state.target.value.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_target,
            .message = "text edit target must not be empty",
        });
    }
    if (plan.command.target != plan.state.target) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.command.target,
            .code = AdapterPayloadDiagnosticCode::mismatched_text_edit_command_target,
            .message = "text edit command target must match the text edit state target",
        });
    }

    const bool state_text_valid = is_strict_utf8(plan.state.text);
    if (!state_text_valid || plan.state.cursor_byte_offset > plan.state.text.size() ||
        !is_utf8_scalar_boundary(plan.state.text, plan.state.cursor_byte_offset)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_cursor,
            .message = "text edit cursor must be within the UTF-8 text and on a scalar boundary",
        });
    }

    const auto selection_available =
        plan.state.cursor_byte_offset <= plan.state.text.size() &&
        plan.state.selection_byte_length <= plan.state.text.size() - plan.state.cursor_byte_offset;
    const auto selection_end =
        selection_available ? plan.state.cursor_byte_offset + plan.state.selection_byte_length : 0U;
    if (!selection_available || !state_text_valid || !is_utf8_scalar_boundary(plan.state.text, selection_end)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_selection,
            .message = "text edit selection must fit the UTF-8 text and end on a scalar boundary",
        });
    }

    if (!is_valid_text_edit_command_kind(plan.command.kind)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.command.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_command,
            .message = "text edit command kind is not supported",
        });
    }

    return plan;
}

TextEditCommandResult apply_text_edit_command(const TextEditState& state, const TextEditCommand& command) {
    const auto plan = plan_text_edit_command(state, command);
    TextEditCommandResult result;
    result.state = state;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    const auto selection_end = result.state.cursor_byte_offset + result.state.selection_byte_length;
    const auto erase_selection = [&result]() {
        if (result.state.selection_byte_length == 0U) {
            return false;
        }

        result.state.text.erase(result.state.cursor_byte_offset, result.state.selection_byte_length);
        result.state.selection_byte_length = 0U;
        return true;
    };

    switch (plan.command.kind) {
    case TextEditCommandKind::move_cursor_backward:
        if (result.state.selection_byte_length == 0U) {
            result.state.cursor_byte_offset =
                previous_utf8_scalar_boundary(result.state.text, result.state.cursor_byte_offset);
        }
        result.state.selection_byte_length = 0U;
        break;
    case TextEditCommandKind::move_cursor_forward:
        result.state.cursor_byte_offset =
            result.state.selection_byte_length > 0U
                ? selection_end
                : next_utf8_scalar_boundary(result.state.text, result.state.cursor_byte_offset);
        result.state.selection_byte_length = 0U;
        break;
    case TextEditCommandKind::move_cursor_to_start:
        result.state.cursor_byte_offset = 0U;
        result.state.selection_byte_length = 0U;
        break;
    case TextEditCommandKind::move_cursor_to_end:
        result.state.cursor_byte_offset = result.state.text.size();
        result.state.selection_byte_length = 0U;
        break;
    case TextEditCommandKind::delete_backward:
        if (!erase_selection() && result.state.cursor_byte_offset > 0U) {
            const auto erase_begin = previous_utf8_scalar_boundary(result.state.text, result.state.cursor_byte_offset);
            result.state.text.erase(erase_begin, result.state.cursor_byte_offset - erase_begin);
            result.state.cursor_byte_offset = erase_begin;
        }
        break;
    case TextEditCommandKind::delete_forward:
        if (!erase_selection() && result.state.cursor_byte_offset < result.state.text.size()) {
            const auto erase_end = next_utf8_scalar_boundary(result.state.text, result.state.cursor_byte_offset);
            result.state.text.erase(result.state.cursor_byte_offset, erase_end - result.state.cursor_byte_offset);
        }
        break;
    }

    result.applied = true;
    return result;
}

bool TextEditClipboardCommandPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool TextEditClipboardCommandResult::succeeded() const noexcept {
    return applied && diagnostics.empty();
}

TextEditClipboardCommandPlan plan_text_edit_clipboard_command(const TextEditState& state,
                                                              const TextEditClipboardCommand& command) {
    TextEditClipboardCommandPlan plan;
    plan.state = state;
    plan.command = command;

    if (plan.state.target.value.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_target,
            .message = "text edit target must not be empty",
        });
    }
    if (plan.command.target != plan.state.target) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.command.target,
            .code = AdapterPayloadDiagnosticCode::mismatched_text_edit_clipboard_command_target,
            .message = "text edit clipboard command target must match the text edit state target",
        });
    }

    const bool state_text_valid = is_strict_utf8(plan.state.text);
    if (!state_text_valid || plan.state.cursor_byte_offset > plan.state.text.size() ||
        !is_utf8_scalar_boundary(plan.state.text, plan.state.cursor_byte_offset)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_cursor,
            .message = "text edit cursor must be within the UTF-8 text and on a scalar boundary",
        });
    }

    const auto selection_available =
        plan.state.cursor_byte_offset <= plan.state.text.size() &&
        plan.state.selection_byte_length <= plan.state.text.size() - plan.state.cursor_byte_offset;
    const auto selection_end =
        selection_available ? plan.state.cursor_byte_offset + plan.state.selection_byte_length : 0U;
    if (!selection_available || !state_text_valid || !is_utf8_scalar_boundary(plan.state.text, selection_end)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_selection,
            .message = "text edit selection must fit the UTF-8 text and end on a scalar boundary",
        });
    }

    const bool command_kind_valid = is_valid_text_edit_clipboard_command_kind(plan.command.kind);
    if (!command_kind_valid) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.command.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_clipboard_command,
            .message = "text edit clipboard command kind is not supported",
        });
    }

    if (command_kind_valid &&
        (plan.command.kind == TextEditClipboardCommandKind::copy_selection ||
         plan.command.kind == TextEditClipboardCommandKind::cut_selection) &&
        plan.state.selection_byte_length == 0U) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.state.target,
            .code = AdapterPayloadDiagnosticCode::invalid_text_edit_selection,
            .message = "clipboard copy and cut require a non-empty selection",
        });
    }

    return plan;
}

TextEditClipboardCommandResult apply_text_edit_clipboard_command(IClipboardTextAdapter& adapter,
                                                                 const TextEditState& state,
                                                                 const TextEditClipboardCommand& command) {
    const auto plan = plan_text_edit_clipboard_command(state, command);
    TextEditClipboardCommandResult result;
    result.state = state;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    const auto append_diagnostics = [&result](const std::vector<AdapterPayloadDiagnostic>& diagnostics) {
        result.diagnostics.insert(result.diagnostics.end(), diagnostics.begin(), diagnostics.end());
    };

    switch (plan.command.kind) {
    case TextEditClipboardCommandKind::copy_selection: {
        const auto selected_text =
            plan.state.text.substr(plan.state.cursor_byte_offset, plan.state.selection_byte_length);
        const auto write_result = write_clipboard_text(adapter, ClipboardTextWriteRequest{
                                                                    .target = plan.state.target,
                                                                    .text = selected_text,
                                                                });
        append_diagnostics(write_result.diagnostics);
        if (!write_result.succeeded()) {
            return result;
        }

        result.applied = true;
        return result;
    }
    case TextEditClipboardCommandKind::cut_selection: {
        const auto selected_text =
            plan.state.text.substr(plan.state.cursor_byte_offset, plan.state.selection_byte_length);
        const auto write_result = write_clipboard_text(adapter, ClipboardTextWriteRequest{
                                                                    .target = plan.state.target,
                                                                    .text = selected_text,
                                                                });
        append_diagnostics(write_result.diagnostics);
        if (!write_result.succeeded()) {
            return result;
        }

        result.state.text.erase(plan.state.cursor_byte_offset, plan.state.selection_byte_length);
        result.state.cursor_byte_offset = plan.state.cursor_byte_offset;
        result.state.selection_byte_length = 0U;
        result.applied = true;
        return result;
    }
    case TextEditClipboardCommandKind::paste_text: {
        const auto read_result = read_clipboard_text(adapter, ClipboardTextReadRequest{.target = plan.state.target});
        append_diagnostics(read_result.diagnostics);
        if (!read_result.succeeded()) {
            return result;
        }
        if (!read_result.has_text) {
            result.applied = true;
            return result;
        }

        result.state.text.replace(plan.state.cursor_byte_offset, plan.state.selection_byte_length, read_result.text);
        result.state.cursor_byte_offset = plan.state.cursor_byte_offset + read_result.text.size();
        result.state.selection_byte_length = 0U;
        result.applied = true;
        return result;
    }
    }

    return result;
}

bool ClipboardTextWritePlan::ready() const noexcept {
    return diagnostics.empty();
}

bool ClipboardTextWriteResult::succeeded() const noexcept {
    return written && diagnostics.empty();
}

bool ClipboardTextReadPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool ClipboardTextReadResult::succeeded() const noexcept {
    return read && diagnostics.empty();
}

ClipboardTextWritePlan plan_clipboard_text_write(const ClipboardTextWriteRequest& request) {
    ClipboardTextWritePlan plan;
    plan.request = request;

    if (plan.request.target.value.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.request.target,
            .code = AdapterPayloadDiagnosticCode::invalid_clipboard_text_target,
            .message = "clipboard text target must not be empty",
        });
    }
    if (!is_strict_utf8(plan.request.text)) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.request.target,
            .code = AdapterPayloadDiagnosticCode::invalid_clipboard_text,
            .message = "clipboard text must be strict UTF-8",
        });
    }

    return plan;
}

ClipboardTextWriteResult write_clipboard_text(IClipboardTextAdapter& adapter,
                                              const ClipboardTextWriteRequest& request) {
    const auto plan = plan_clipboard_text_write(request);
    ClipboardTextWriteResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    adapter.set_clipboard_text(plan.request.text);
    result.written = true;
    return result;
}

ClipboardTextReadPlan plan_clipboard_text_read(const ClipboardTextReadRequest& request) {
    ClipboardTextReadPlan plan;
    plan.request = request;

    if (plan.request.target.value.empty()) {
        plan.diagnostics.push_back(AdapterPayloadDiagnostic{
            .id = plan.request.target,
            .code = AdapterPayloadDiagnosticCode::invalid_clipboard_text_target,
            .message = "clipboard text target must not be empty",
        });
    }

    return plan;
}

ClipboardTextReadResult read_clipboard_text(IClipboardTextAdapter& adapter, const ClipboardTextReadRequest& request) {
    const auto plan = plan_clipboard_text_read(request);
    ClipboardTextReadResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    result.has_text = adapter.has_clipboard_text();
    if (result.has_text) {
        result.text = adapter.clipboard_text();
        if (!is_strict_utf8(result.text)) {
            result.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = plan.request.target,
                .code = AdapterPayloadDiagnosticCode::invalid_clipboard_text_result,
                .message = "clipboard text adapter returned invalid UTF-8",
            });
        }
    }
    result.read = true;
    return result;
}

bool AccessibilityPublishPlan::ready() const noexcept {
    return diagnostics.empty();
}

bool AccessibilityPublishResult::succeeded() const noexcept {
    return published && diagnostics.empty();
}

AccessibilityPublishPlan plan_accessibility_publish(const AccessibilityPayload& payload) {
    AccessibilityPublishPlan plan;
    plan.nodes = payload.nodes;
    plan.diagnostics = payload.diagnostics;

    for (const auto& node : plan.nodes) {
        if (!is_positive_rect(node.bounds) &&
            !has_diagnostic(plan.diagnostics, node.id, AdapterPayloadDiagnosticCode::invalid_accessibility_bounds)) {
            plan.diagnostics.push_back(AdapterPayloadDiagnostic{
                .id = node.id,
                .code = AdapterPayloadDiagnosticCode::invalid_accessibility_bounds,
                .message = "accessibility node has invalid or non-positive bounds",
            });
        }
    }

    return plan;
}

AccessibilityPublishResult publish_accessibility_payload(IAccessibilityAdapter& adapter,
                                                         const AccessibilityPayload& payload) {
    const auto plan = plan_accessibility_publish(payload);
    AccessibilityPublishResult result;
    result.diagnostics = plan.diagnostics;
    if (!plan.ready()) {
        return result;
    }

    adapter.publish_nodes(plan.nodes);
    result.published = true;
    result.nodes_published = plan.nodes.size();
    return result;
}

const ElementId& InteractionState::focused() const noexcept {
    return focused_;
}

const ElementId& InteractionState::hovered() const noexcept {
    return hovered_;
}

const ElementId& InteractionState::active() const noexcept {
    return active_;
}

const ElementId& InteractionState::modal_layer() const noexcept {
    return modal_layer_;
}

bool InteractionState::set_focus(const UiDocument& document, const ElementId& id) {
    if (!can_focus(document, id)) {
        return false;
    }
    focused_ = id;
    return true;
}

bool InteractionState::route_navigation(const UiDocument& document, NavigationDirection direction) {
    const auto traversal = document.traverse();
    std::vector<ElementId> candidates;
    candidates.reserve(traversal.size());
    for (const auto& element : traversal) {
        if (can_focus(document, element.id)) {
            candidates.push_back(element.id);
        }
    }
    if (candidates.empty()) {
        return false;
    }

    const auto current = std::ranges::find(candidates, focused_);
    if (current == candidates.end()) {
        focused_ = candidates.front();
        return true;
    }

    if (navigates_forward(direction)) {
        const auto next = std::next(current);
        focused_ = next == candidates.end() ? candidates.front() : *next;
        return true;
    }

    focused_ = current == candidates.begin() ? candidates.back() : *std::prev(current);
    return true;
}

void InteractionState::set_hovered(const ElementId& id) noexcept {
    hovered_ = id;
}

void InteractionState::set_active(const ElementId& id) noexcept {
    active_ = id;
}

void InteractionState::push_modal_layer(ElementId id) {
    if (id.value.empty()) {
        throw std::invalid_argument("ui modal layer id must not be empty");
    }
    modal_layer_ = std::move(id);
    focused_ = {};
}

bool InteractionState::pop_modal_layer() noexcept {
    if (modal_layer_.value.empty()) {
        return false;
    }
    modal_layer_ = {};
    focused_ = {};
    return true;
}

bool InteractionState::can_focus(const UiDocument& document, const ElementId& id) const noexcept {
    const auto* element = document.find(id);
    if (element == nullptr || !element->visible || !element->enabled || !is_focusable_role(element->role)) {
        return false;
    }
    if (modal_layer_.value.empty()) {
        return true;
    }
    return document.is_descendant_or_same(modal_layer_, id);
}

TransitionSample sample_transition(const TransitionState& transition) noexcept {
    const float progress = transition.duration_seconds <= 0.0F
                               ? 1.0F
                               : std::clamp(transition.elapsed_seconds / transition.duration_seconds, 0.0F, 1.0F);
    const float value = transition.start_value + ((transition.end_value - transition.start_value) * progress);
    return TransitionSample{.value = value, .progress = progress, .finished = progress >= 1.0F};
}

TransitionSample advance_transition(TransitionState& transition, float delta_seconds) {
    if (!std::isfinite(delta_seconds) || delta_seconds < 0.0F) {
        throw std::invalid_argument("ui transition delta must be finite and non-negative");
    }
    transition.elapsed_seconds += delta_seconds;
    if (transition.duration_seconds > 0.0F) {
        transition.elapsed_seconds = std::min(transition.elapsed_seconds, transition.duration_seconds);
    }
    return sample_transition(transition);
}

void BindingContext::set_value(std::string key, std::string value) {
    if (key.empty()) {
        throw std::invalid_argument("ui binding key must not be empty");
    }

    const auto it = std::ranges::find_if(values_, [&key](const auto& entry) { return entry.first == key; });
    if (it == values_.end()) {
        values_.emplace_back(std::move(key), std::move(value));
        return;
    }
    it->second = std::move(value);
}

std::optional<std::string_view> BindingContext::value(std::string_view key) const noexcept {
    const auto it = std::ranges::find_if(values_, [key](const auto& entry) { return entry.first == key; });
    if (it == values_.end()) {
        return std::nullopt;
    }
    return std::string_view{it->second};
}

bool apply_text_binding(UiDocument& document, const TextBinding& binding, const BindingContext& context) {
    const auto value = context.value(binding.source_key);
    if (!value.has_value()) {
        return false;
    }

    const auto* existing = document.find(binding.element);
    if (existing == nullptr) {
        return false;
    }

    auto text = existing->text;
    text.label = std::string(*value);
    return document.set_text(binding.element, std::move(text));
}

bool CommandRegistry::try_add(CommandBinding command) {
    if (command.id.empty() || !command.action || find(command.id) != nullptr) {
        return false;
    }
    commands_.push_back(std::move(command));
    return true;
}

bool CommandRegistry::execute(std::string_view id) const {
    const auto* command = find(id);
    if (command == nullptr || !command->enabled) {
        return false;
    }
    command->action();
    return true;
}

const CommandBinding* CommandRegistry::find(std::string_view id) const noexcept {
    const auto it = std::ranges::find_if(commands_, [id](const CommandBinding& command) { return command.id == id; });
    return it == commands_.end() ? nullptr : &(*it);
}

} // namespace mirakana::ui
