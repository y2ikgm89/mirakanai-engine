// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/ui_model.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace mirakana::editor {

namespace {

[[nodiscard]] bool contains_line_separator(std::string_view value) noexcept {
    return value.find('\n') != std::string_view::npos || value.find('\r') != std::string_view::npos;
}

void require_safe_field(std::string_view field, std::string_view value) {
    if (value.empty() || contains_line_separator(value) || value.find('=') != std::string_view::npos) {
        throw std::invalid_argument(std::string("editor ui field is invalid: ") + std::string(field));
    }
}

void require_safe_optional_field(std::string_view field, std::string_view value) {
    if (contains_line_separator(value) || value.find('=') != std::string_view::npos) {
        throw std::invalid_argument(std::string("editor ui field is invalid: ") + std::string(field));
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::ElementDesc make_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label, std::string localization_key = {}) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.localization_key = std::move(localization_key);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

void append_button(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                   std::string label, bool enabled) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::button);
    desc.text = make_text(std::move(label));
    desc.enabled = enabled;
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] std::string format_seconds(float value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << value;
    auto text = out.str();
    while (text.size() > 1U && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    return text;
}

void serialize_element(std::ostringstream& out, const mirakana::ui::Element& element, std::size_t index) {
    out << "element." << index << ".id=" << element.id.value << '\n';
    out << "element." << index << ".parent=" << element.parent.value << '\n';
    out << "element." << index << ".role=" << mirakana::ui::semantic_role_id(element.role) << '\n';
    out << "element." << index << ".visible=" << (element.visible ? "true" : "false") << '\n';
    out << "element." << index << ".enabled=" << (element.enabled ? "true" : "false") << '\n';
    out << "element." << index << ".label=" << element.text.label << '\n';
    out << "element." << index << ".localization_key=" << element.text.localization_key << '\n';
}

} // namespace

mirakana::ui::UiDocument make_inspector_ui_model(const std::vector<EditorPropertyRow>& rows) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("inspector", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"inspector"};

    for (const auto& row : rows) {
        require_safe_field("property.id", row.id);
        require_safe_field("property.label", row.label);
        require_safe_field("property.value", row.value);

        append_label(document, root, "inspector." + row.id + ".label", row.label);

        mirakana::ui::ElementDesc value =
            make_child("inspector." + row.id + ".value", root, mirakana::ui::SemanticRole::text_field);
        value.text = make_text(row.value);
        value.enabled = row.editable;
        add_or_throw(document, std::move(value));
    }

    return document;
}

mirakana::ui::UiDocument make_asset_list_ui_model(const std::vector<EditorAssetListRow>& rows) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("assets", mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId root{"assets"};

    for (const auto& row : rows) {
        require_safe_field("asset.id", row.id);
        require_safe_field("asset.path", row.path);
        require_safe_field("asset.kind", row.kind);

        mirakana::ui::ElementDesc item = make_child("assets." + row.id, root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.kind);
        item.enabled = row.enabled;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{"assets." + row.id};
        append_label(document, item_id, "assets." + row.id + ".path", row.path);
        append_label(document, item_id, "assets." + row.id + ".kind", row.kind);
    }

    return document;
}

mirakana::ui::UiDocument make_command_palette_ui_model(const std::vector<EditorCommandPaletteEntry>& entries) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("commands", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"commands"};

    for (const auto& entry : entries) {
        require_safe_field("command.id", entry.id);
        require_safe_field("command.label", entry.label);
        append_button(document, root, "commands." + entry.id, entry.label, entry.enabled);
    }

    return document;
}

mirakana::ui::UiDocument make_diagnostics_ui_model(const std::vector<EditorDiagnosticRow>& rows) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("diagnostics", mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId root{"diagnostics"};

    for (const auto& row : rows) {
        require_safe_field("diagnostic.id", row.id);
        require_safe_field("diagnostic.message", row.message);

        mirakana::ui::ElementDesc item =
            make_child("diagnostics." + row.id, root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(std::string(editor_diagnostic_severity_label(row.severity)));
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{"diagnostics." + row.id};
        append_label(document, item_id, "diagnostics." + row.id + ".severity",
                     std::string(editor_diagnostic_severity_label(row.severity)));
        append_label(document, item_id, "diagnostics." + row.id + ".message", row.message);
    }

    return document;
}

EditorTimelinePanelModel make_editor_timeline_panel_model(const mirakana::AnimationAuthoredTimelineDesc& desc,
                                                          float playhead_seconds, bool playing) {
    if (!mirakana::is_valid_animation_authored_timeline_desc(desc) || !std::isfinite(playhead_seconds)) {
        throw std::invalid_argument("editor timeline panel model input is invalid");
    }

    std::vector<mirakana::AnimationTimelineEventTrackDesc> tracks = desc.tracks;
    std::ranges::sort(tracks, [](const mirakana::AnimationTimelineEventTrackDesc& lhs,
                                 const mirakana::AnimationTimelineEventTrackDesc& rhs) { return lhs.name < rhs.name; });

    EditorTimelinePanelModel model;
    model.duration_seconds = desc.duration_seconds;
    model.playhead_seconds = std::clamp(playhead_seconds, 0.0F, desc.duration_seconds);
    model.looping = desc.looping;
    model.playing = playing;
    model.tracks.reserve(tracks.size());

    for (const auto& track : tracks) {
        EditorTimelineTrackRow row;
        row.id = track.name;
        row.label = track.name;
        row.events.reserve(track.events.size());

        std::size_t event_index = 1;
        for (const auto& event : track.events) {
            row.events.push_back(EditorTimelineEventRow{
                .id = std::to_string(event_index),
                .time_seconds = event.time_seconds,
                .track = track.name,
                .name = event.name,
                .payload = event.payload,
            });
            ++event_index;
        }
        model.tracks.push_back(std::move(row));
    }

    return model;
}

mirakana::ui::UiDocument make_timeline_ui_model(const EditorTimelinePanelModel& model) {
    if (!std::isfinite(model.duration_seconds) || model.duration_seconds <= 0.0F ||
        !std::isfinite(model.playhead_seconds) || model.playhead_seconds < 0.0F ||
        model.playhead_seconds > model.duration_seconds) {
        throw std::invalid_argument("editor timeline panel model is invalid");
    }

    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("timeline", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"timeline"};

    append_button(document, root, "timeline.play", "Play", !model.playing);
    append_button(document, root, "timeline.pause", "Pause", model.playing);
    append_button(document, root, "timeline.step", "Step", true);
    append_label(document, root, "timeline.duration", format_seconds(model.duration_seconds));
    append_label(document, root, "timeline.playhead", format_seconds(model.playhead_seconds));
    append_label(document, root, "timeline.looping", model.looping ? "Looping" : "One Shot");

    for (const auto& track : model.tracks) {
        require_safe_field("timeline.track.id", track.id);
        require_safe_field("timeline.track.label", track.label);

        mirakana::ui::ElementDesc track_item =
            make_child("timeline.track." + track.id, root, mirakana::ui::SemanticRole::list_item);
        track_item.text = make_text(track.label);
        add_or_throw(document, std::move(track_item));

        const mirakana::ui::ElementId track_id{"timeline.track." + track.id};
        for (const auto& event : track.events) {
            require_safe_field("timeline.event.id", event.id);
            require_safe_field("timeline.event.track", event.track);
            require_safe_field("timeline.event.name", event.name);
            require_safe_optional_field("timeline.event.payload", event.payload);

            const auto event_prefix = "timeline.track." + track.id + ".event." + event.id;
            mirakana::ui::ElementDesc event_item =
                make_child(event_prefix, track_id, mirakana::ui::SemanticRole::list_item);
            event_item.text = make_text(event.name);
            add_or_throw(document, std::move(event_item));

            const mirakana::ui::ElementId event_id{event_prefix};
            append_label(document, event_id, event_prefix + ".time", format_seconds(event.time_seconds));
            append_label(document, event_id, event_prefix + ".name", event.name);
            append_label(document, event_id, event_prefix + ".payload", event.payload.empty() ? "-" : event.payload);
        }
    }

    return document;
}

std::string serialize_editor_ui_model(const mirakana::ui::UiDocument& document) {
    const auto elements = document.traverse();
    std::ostringstream out;
    out << "format=GameEngine.EditorUiModel.v1\n";
    out << "element.count=" << elements.size() << '\n';

    std::size_t index = 1;
    for (const auto& element : elements) {
        serialize_element(out, element, index);
        ++index;
    }

    return out.str();
}

std::string_view editor_diagnostic_severity_label(EditorDiagnosticSeverity severity) noexcept {
    switch (severity) {
    case EditorDiagnosticSeverity::info:
        return "Info";
    case EditorDiagnosticSeverity::warning:
        return "Warning";
    case EditorDiagnosticSeverity::error:
        return "Error";
    }
    return "Unknown";
}

} // namespace mirakana::editor
