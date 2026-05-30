// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_panels.hpp"

#include "native_editor_app.hpp"

#include "mirakana/editor/workspace.hpp"

#include <imgui.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

void text_unformatted(std::string_view text) {
    if (text.empty()) {
        ImGui::TextUnformatted("");
        return;
    }
    ImGui::TextUnformatted(text.data(), text.data() + text.size());
}

void key_value(std::string_view key, std::string_view value) {
    text_unformatted(key);
    ImGui::SameLine();
    text_unformatted(value);
}

void key_value(std::string_view key, std::uint64_t value) {
    key_value(key, std::to_string(value));
}

void text_vector(std::string_view label, const std::vector<std::string>& values) {
    if (values.empty()) {
        key_value(label, "-");
        return;
    }

    std::string joined;
    for (const auto& value : values) {
        if (!joined.empty()) {
            joined += ", ";
        }
        joined += value;
    }
    key_value(label, joined);
}

[[nodiscard]] bool begin_workspace_panel(NativeEditorApp& app, PanelId panel, const char* title) {
    bool open = app.is_panel_visible(panel);
    if (!open) {
        return false;
    }

    const bool expanded = ImGui::Begin(title, &open);
    app.set_panel_visible(panel, open);
    if (!expanded) {
        ImGui::End();
    }
    return expanded;
}

void end_workspace_panel() {
    ImGui::End();
}

void render_property_rows(std::span<const EditorPropertyRow> rows) {
    for (const auto& row : rows) {
        ImGui::SeparatorText(row.label.c_str());
        key_value("id", row.id);
        key_value("value", row.value);
        key_value("editable", row.editable ? "yes" : "no");
    }
}

void render_asset_rows(std::span<const EditorAssetListRow> rows) {
    for (const auto& row : rows) {
        ImGui::BulletText("%s", row.path.c_str());
        key_value("kind", row.kind);
        key_value("enabled", row.enabled ? "yes" : "no");
    }
}

void render_console_rows(std::span<const EditorDiagnosticRow> rows) {
    for (const auto& row : rows) {
        ImGui::BulletText("%s", row.id.c_str());
        key_value("severity", editor_diagnostic_severity_label(row.severity));
        key_value("message", row.message);
    }
}

void render_key_value_rows(std::span<const EditorResourceRow> rows) {
    for (const auto& row : rows) {
        ImGui::BulletText("%s", row.label.c_str());
        key_value("value", row.value);
        key_value("available", row.available ? "yes" : "no");
    }
}

void render_profiler_key_value_rows(std::span<const EditorProfilerKeyValueRow> rows) {
    for (const auto& row : rows) {
        ImGui::BulletText("%s", row.label.c_str());
        key_value("value", row.value);
    }
}

void render_profiler_events(std::span<const EditorProfilerEventRow> rows) {
    for (const auto& row : rows) {
        ImGui::BulletText("%s", row.message.c_str());
        key_value("severity", row.severity);
        key_value("category", row.category);
        key_value("frame", row.frame_index);
    }
}

void render_timeline_track(const EditorTimelineTrackRow& track) {
    if (ImGui::TreeNode(track.label.c_str())) {
        for (const auto& event : track.events) {
            ImGui::BulletText("%s", event.name.c_str());
            key_value("time", std::to_string(event.time_seconds));
            key_value("payload", event.payload.empty() ? "-" : event.payload);
        }
        ImGui::TreePop();
    }
}

template <typename RenderFn>
void render_counted_panel(NativeEditorApp& app, PanelId panel, RenderFn&& render_fn, std::uint32_t& count) {
    if (!app.is_panel_visible(panel)) {
        return;
    }
    std::forward<RenderFn>(render_fn)(app);
    ++count;
}

} // namespace

void render_native_editor_main_menu(NativeEditorApp& app) {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        ImGui::MenuItem("Open Project", nullptr, false, false);
        ImGui::MenuItem("Save Project", nullptr, false, false);
        ImGui::Separator();
        ImGui::MenuItem("Exit", nullptr, false, false);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
        for (const auto panel :
             {PanelId::scene, PanelId::inspector, PanelId::assets, PanelId::console, PanelId::resources,
              PanelId::ai_commands, PanelId::profiler, PanelId::timeline, PanelId::project_settings}) {
            bool visible = app.is_panel_visible(panel);
            if (ImGui::MenuItem(panel_id_to_string(panel).data(), nullptr, visible)) {
                app.set_panel_visible(panel, !visible);
            }
        }
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void render_native_editor_scene_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::scene, "Scene")) {
        return;
    }

    const auto& document = app.scene_document();
    key_value("path", std::string_view{document.scene_path()});
    key_value("dirty", document.dirty() ? "yes" : "no");
    key_value("nodes", static_cast<std::uint64_t>(document.hierarchy_rows().size()));
    ImGui::SeparatorText("Hierarchy");
    for (const auto& row : document.hierarchy_rows()) {
        std::string label(row.depth * 2U, ' ');
        label += row.name;
        if (row.selected) {
            label += " [selected]";
        }
        ImGui::BulletText("%s", label.c_str());
    }

    end_workspace_panel();
}

void render_native_editor_inspector_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::inspector, "Inspector")) {
        return;
    }

    render_property_rows(app.inspector_rows());
    end_workspace_panel();
}

void render_native_editor_assets_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::assets, "Assets")) {
        return;
    }

    render_asset_rows(app.asset_rows());
    end_workspace_panel();
}

void render_native_editor_console_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::console, "Console")) {
        return;
    }

    render_console_rows(app.console_rows());
    end_workspace_panel();
}

void render_native_editor_resources_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::resources, "Resources")) {
        return;
    }

    const auto& resources = app.resources();
    key_value("status", resources.status);
    key_value("device", resources.device_available ? "available" : "unavailable");
    ImGui::SeparatorText("Status");
    render_key_value_rows(resources.status_rows);
    ImGui::SeparatorText("Memory");
    render_key_value_rows(resources.memory_rows);
    ImGui::SeparatorText("Capture requests");
    for (const auto& row : resources.capture_request_rows) {
        ImGui::BulletText("%s", row.label.c_str());
        key_value("status", row.status_label);
        key_value("tool", row.tool_label);
        key_value("acknowledgement", row.acknowledgement_label);
        text_vector("host gates", row.host_gates);
    }

    end_workspace_panel();
}

void render_native_editor_ai_commands_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::ai_commands, "AI Commands")) {
        return;
    }

    const auto& ai = app.ai_commands();
    key_value("status", ai.status_label);
    key_value("ready stages", static_cast<std::uint64_t>(ai.ready_stage_count));
    key_value("blocked stages", static_cast<std::uint64_t>(ai.blocked_stage_count));
    key_value("host gated stages", static_cast<std::uint64_t>(ai.host_gated_stage_count));
    key_value("operator handoff", ai.ready_for_operator_handoff ? "ready" : "not ready");
    for (const auto& diagnostic : ai.diagnostics) {
        ImGui::BulletText("%s", diagnostic.c_str());
    }

    end_workspace_panel();
}

void render_native_editor_profiler_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::profiler, "Profiler")) {
        return;
    }

    const auto& profiler = app.profiler();
    key_value("trace", profiler.trace_status);
    ImGui::SeparatorText("Status");
    render_profiler_key_value_rows(profiler.status_rows);
    ImGui::SeparatorText("Summary");
    render_profiler_key_value_rows(profiler.summary_rows);
    ImGui::SeparatorText("Events");
    render_profiler_events(profiler.event_rows);

    end_workspace_panel();
}

void render_native_editor_timeline_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::timeline, "Timeline")) {
        return;
    }

    const auto& timeline = app.timeline();
    key_value("duration", std::to_string(timeline.duration_seconds));
    key_value("playhead", std::to_string(timeline.playhead_seconds));
    key_value("state", timeline.playing ? "playing" : "stopped");
    key_value("looping", timeline.looping ? "yes" : "no");
    ImGui::SeparatorText("Tracks");
    for (const auto& track : timeline.tracks) {
        render_timeline_track(track);
    }

    end_workspace_panel();
}

void render_native_editor_project_settings_panel(NativeEditorApp& app) {
    if (!begin_workspace_panel(app, PanelId::project_settings, "Project Settings")) {
        return;
    }

    const auto& project = app.project();
    key_value("name", project.name);
    key_value("root", project.root_path);
    key_value("asset root", project.asset_root);
    key_value("source registry", project.source_registry_path);
    key_value("game manifest", project.game_manifest_path);
    key_value("startup scene", project.startup_scene_path);
    key_value("shader tool", project.shader_tool.executable);
    const auto errors = app.project_settings_errors();
    if (!errors.empty()) {
        ImGui::SeparatorText("Validation");
        for (const auto& error : errors) {
            ImGui::BulletText("%s: %s", error.field.c_str(), error.message.c_str());
        }
    }

    end_workspace_panel();
}

std::uint32_t render_native_editor_panels(NativeEditorApp& app) {
    std::uint32_t rendered_count = 0;
    render_native_editor_main_menu(app);
    ++rendered_count;
    render_counted_panel(app, PanelId::scene, render_native_editor_scene_panel, rendered_count);
    render_counted_panel(app, PanelId::inspector, render_native_editor_inspector_panel, rendered_count);
    render_counted_panel(app, PanelId::assets, render_native_editor_assets_panel, rendered_count);
    render_counted_panel(app, PanelId::console, render_native_editor_console_panel, rendered_count);
    render_counted_panel(app, PanelId::resources, render_native_editor_resources_panel, rendered_count);
    render_counted_panel(app, PanelId::ai_commands, render_native_editor_ai_commands_panel, rendered_count);
    render_counted_panel(app, PanelId::profiler, render_native_editor_profiler_panel, rendered_count);
    render_counted_panel(app, PanelId::timeline, render_native_editor_timeline_panel, rendered_count);
    render_counted_panel(app, PanelId::project_settings, render_native_editor_project_settings_panel, rendered_count);
    app.record_native_panels_rendered(rendered_count);
    return rendered_count;
}

} // namespace mirakana::editor
