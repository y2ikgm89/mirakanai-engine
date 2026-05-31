// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_editor_app.hpp"
#include "native_editor_tsf_text_input.hpp"

#include "mirakana/platform/win32/win32_clipboard.hpp"
#include "mirakana/platform/win32/win32_file_dialog.hpp"
#include "mirakana/platform/win32_process.hpp"

#include <cstdint>

namespace mirakana::editor {

class NativeEditorWin32Services final {
  public:
    explicit NativeEditorWin32Services(std::uintptr_t owner_window_token);

    void bind(NativeEditorApp& app);

  private:
    mirakana::win32::Win32FileDialogService file_dialogs_;
    mirakana::win32::Win32Clipboard clipboard_;
    mirakana::win32::Win32ClipboardTextAdapter clipboard_adapter_;
    Win32ProcessRunner process_runner_;
    NativeEditorTsfTextServicesAdapter text_services_;
};

} // namespace mirakana::editor
