// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_win32_services.hpp"

namespace mirakana::editor {

NativeEditorWin32Services::NativeEditorWin32Services(std::uintptr_t owner_window_token)
    : file_dialogs_(owner_window_token), clipboard_adapter_(clipboard_) {}

void NativeEditorWin32Services::bind(NativeEditorApp& app) {
    app.bind_native_services(NativeEditorServiceBindings{
        .file_dialog_service = &file_dialogs_,
        .clipboard_text_adapter = &clipboard_adapter_,
        .reviewed_process_runner = &process_runner_,
        .platform_text_input_adapter = &text_services_,
        .ime_adapter = &text_services_,
        .file_dialog_service_id = "win32",
        .clipboard_service_id = "win32",
        .reviewed_process_runner_id = "win32",
        .platform_text_input_service_id = "win32_tsf",
        .ime_service_id = "win32_tsf",
    });
}

} // namespace mirakana::editor
