// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

using FileDialogId = std::uint64_t;

enum class FileDialogKind {
    open_file,
    save_file,
    open_folder,
};

enum class FileDialogStatus {
    accepted,
    canceled,
    failed,
};

struct FileDialogFilter {
    std::string name;
    std::string pattern;
};

struct FileDialogRequest {
    FileDialogKind kind{FileDialogKind::open_file};
    std::string title;
    std::vector<FileDialogFilter> filters;
    std::string default_location;
    bool allow_many{false};
    std::string accept_label;
    std::string cancel_label;
};

struct FileDialogResult {
    FileDialogId id{0};
    FileDialogStatus status{FileDialogStatus::canceled};
    std::vector<std::string> paths;
    int selected_filter{-1};
    std::string error;
};

struct MemoryFileDialogResponse {
    FileDialogStatus status{FileDialogStatus::canceled};
    std::vector<std::string> paths;
    int selected_filter{-1};
    std::string error;
};

[[nodiscard]] bool is_valid_file_dialog_filter(const FileDialogFilter& filter) noexcept;
[[nodiscard]] std::string validate_file_dialog_request(const FileDialogRequest& request);
[[nodiscard]] std::string validate_file_dialog_result(const FileDialogResult& result);

class FileDialogResultQueue final {
  public:
    void push(FileDialogResult result);
    [[nodiscard]] std::optional<FileDialogResult> poll(FileDialogId id);

  private:
    std::mutex mutex_;
    std::deque<FileDialogResult> results_;
};

class IFileDialogService {
  public:
    virtual ~IFileDialogService() = default;

    [[nodiscard]] virtual FileDialogId show(FileDialogRequest request) = 0;
    [[nodiscard]] virtual std::optional<FileDialogResult> poll_result(FileDialogId id) = 0;
};

class MemoryFileDialogService final : public IFileDialogService {
  public:
    void enqueue_response(MemoryFileDialogResponse response);

    [[nodiscard]] FileDialogId show(FileDialogRequest request) override;
    [[nodiscard]] std::optional<FileDialogResult> poll_result(FileDialogId id) override;
    [[nodiscard]] const std::optional<FileDialogRequest>& last_request() const noexcept;

  private:
    FileDialogId next_id_{1};
    std::deque<MemoryFileDialogResponse> responses_;
    FileDialogResultQueue results_;
    std::optional<FileDialogRequest> last_request_;
};

} // namespace mirakana
