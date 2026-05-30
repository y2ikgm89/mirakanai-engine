// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>

namespace mirakana::editor {

class NativeEditorApp;

void render_native_editor_main_menu(NativeEditorApp& app);
void render_native_editor_scene_panel(NativeEditorApp& app);
void render_native_editor_inspector_panel(NativeEditorApp& app);
void render_native_editor_assets_panel(NativeEditorApp& app);
void render_native_editor_console_panel(NativeEditorApp& app);
void render_native_editor_resources_panel(NativeEditorApp& app);
void render_native_editor_ai_commands_panel(NativeEditorApp& app);
void render_native_editor_profiler_panel(NativeEditorApp& app);
void render_native_editor_timeline_panel(NativeEditorApp& app);
void render_native_editor_project_settings_panel(NativeEditorApp& app);
[[nodiscard]] std::uint32_t render_native_editor_panels(NativeEditorApp& app);

} // namespace mirakana::editor
