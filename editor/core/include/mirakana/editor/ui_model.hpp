// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/timeline.hpp"
#include "mirakana/ui/ui.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct EditorPropertyRow {
    std::string id;
    std::string label;
    std::string value;
    bool editable{true};
};

struct EditorAssetListRow {
    std::string id;
    std::string path;
    std::string kind;
    bool enabled{true};
};

struct EditorCommandPaletteEntry {
    std::string id;
    std::string label;
    bool enabled{true};
};

enum class EditorDiagnosticSeverity {
    info,
    warning,
    error,
};

struct EditorDiagnosticRow {
    std::string id;
    EditorDiagnosticSeverity severity{EditorDiagnosticSeverity::info};
    std::string message;
};

struct EditorTimelineEventRow {
    std::string id;
    float time_seconds{0.0F};
    std::string track;
    std::string name;
    std::string payload;
};

struct EditorTimelineTrackRow {
    std::string id;
    std::string label;
    std::vector<EditorTimelineEventRow> events;
};

struct EditorTimelinePanelModel {
    float duration_seconds{0.0F};
    float playhead_seconds{0.0F};
    bool looping{false};
    bool playing{false};
    std::vector<EditorTimelineTrackRow> tracks;
};

[[nodiscard]] mirakana::ui::UiDocument make_inspector_ui_model(const std::vector<EditorPropertyRow>& rows);
[[nodiscard]] mirakana::ui::UiDocument make_asset_list_ui_model(const std::vector<EditorAssetListRow>& rows);
[[nodiscard]] mirakana::ui::UiDocument
make_command_palette_ui_model(const std::vector<EditorCommandPaletteEntry>& entries);
[[nodiscard]] mirakana::ui::UiDocument make_diagnostics_ui_model(const std::vector<EditorDiagnosticRow>& rows);
[[nodiscard]] EditorTimelinePanelModel
make_editor_timeline_panel_model(const mirakana::AnimationAuthoredTimelineDesc& desc, float playhead_seconds,
                                 bool playing);
[[nodiscard]] mirakana::ui::UiDocument make_timeline_ui_model(const EditorTimelinePanelModel& model);
[[nodiscard]] std::string serialize_editor_ui_model(const mirakana::ui::UiDocument& document);
[[nodiscard]] std::string_view editor_diagnostic_severity_label(EditorDiagnosticSeverity severity) noexcept;

} // namespace mirakana::editor
