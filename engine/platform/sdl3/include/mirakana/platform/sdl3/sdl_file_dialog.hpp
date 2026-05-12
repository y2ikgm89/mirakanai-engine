// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/platform/sdl3/sdl_window.hpp"

#include <memory>

namespace mirakana {

class SdlFileDialogService final : public IFileDialogService {
  public:
    explicit SdlFileDialogService(SdlWindow* owner_window = nullptr);

    [[nodiscard]] FileDialogId show(FileDialogRequest request) override;
    [[nodiscard]] std::optional<FileDialogResult> poll_result(FileDialogId id) override;

  private:
    std::shared_ptr<FileDialogResultQueue> results_;
    FileDialogId next_id_{1};
    SdlWindow* owner_window_{nullptr};
};

} // namespace mirakana
