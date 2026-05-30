// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "native_editor_app.hpp"

namespace mirakana::editor {

NativeEditorApp::NativeEditorApp(NativeEditorLaunchOptions options) : options_{options} {}

const NativeEditorLaunchOptions& NativeEditorApp::options() const noexcept {
    return options_;
}

int NativeEditorApp::run() {
    return 0;
}

} // namespace mirakana::editor
