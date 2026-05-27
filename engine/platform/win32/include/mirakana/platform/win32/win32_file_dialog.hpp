// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/file_dialog.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::win32 {

struct Win32FileDialogFilterSpec {
    std::string display_name;
    std::string spec;
};

struct Win32FileDialogRequestPlan {
    FileDialogRequest request;
    std::vector<Win32FileDialogFilterSpec> filters;
    bool use_open_dialog{false};
    bool use_save_dialog{false};
    bool pick_folders{false};
    bool force_filesystem{true};
    bool path_must_exist{true};
    bool file_must_exist{false};
    bool overwrite_prompt{false};
    bool allow_many{false};
    std::string diagnostic;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostic.empty();
    }
};

[[nodiscard]] Win32FileDialogRequestPlan plan_win32_file_dialog_request(FileDialogRequest request);

class Win32FileDialogService final : public IFileDialogService {
  public:
    explicit Win32FileDialogService(std::uintptr_t owner_window_token = 0);

    [[nodiscard]] FileDialogId show(FileDialogRequest request) override;
    [[nodiscard]] std::optional<FileDialogResult> poll_result(FileDialogId id) override;

  private:
    std::shared_ptr<FileDialogResultQueue> results_;
    FileDialogId next_id_{1};
    std::uintptr_t owner_window_token_{0};
};

} // namespace mirakana::win32
