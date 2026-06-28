// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_win32_services.hpp"

#include <filesystem>

namespace mirakana::editor {

NativeEditorWin32Services::NativeEditorWin32Services(std::uintptr_t owner_window_token)
    : file_dialogs_(owner_window_token), clipboard_adapter_(clipboard_),
      accessibility_(make_native_editor_win32_uia_accessibility_adapter(owner_window_token)) {}

void NativeEditorWin32Services::bind(NativeEditorApp& app) {
    asset_import_filesystem_.emplace(
        std::filesystem::absolute(std::filesystem::path{app.project().root_path}).lexically_normal());
    app.bind_native_services(NativeEditorServiceBindings{
        .file_dialog_service = &file_dialogs_,
        .clipboard_text_adapter = &clipboard_adapter_,
        .reviewed_process_runner = &process_runner_,
        .platform_text_input_adapter = &text_services_,
        .ime_adapter = &text_services_,
        .accessibility_adapter = accessibility_.get(),
        .asset_import_filesystem = std::addressof(*asset_import_filesystem_),
        .file_dialog_service_id = "win32",
        .clipboard_service_id = "win32",
        .reviewed_process_runner_id = "win32",
        .platform_text_input_service_id = "win32_tsf",
        .ime_service_id = "win32_tsf",
        .accessibility_service_id = "win32_uia",
        .asset_import_filesystem_id = "rooted_project",
    });
}

} // namespace mirakana::editor
