// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_editor_launch.hpp"

namespace mirakana::editor {

class NativeEditorApp {
  public:
    explicit NativeEditorApp(NativeEditorLaunchOptions options);

    [[nodiscard]] const NativeEditorLaunchOptions& options() const noexcept;

    [[nodiscard]] int run();

  private:
    NativeEditorLaunchOptions options_;
};

} // namespace mirakana::editor
