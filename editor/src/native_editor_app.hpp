// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "native_editor_launch.hpp"

namespace mirakana::editor {

class NativeEditorApp {
  public:
    explicit NativeEditorApp(NativeEditorLaunchOptions options);

    [[nodiscard]] const NativeEditorLaunchOptions& options() const noexcept;
    [[nodiscard]] std::uint32_t frames_recorded() const noexcept;

    [[nodiscard]] int run();
    void record_native_frame() noexcept;

  private:
    NativeEditorLaunchOptions options_;
    std::uint32_t frames_recorded_{0};
};

} // namespace mirakana::editor
