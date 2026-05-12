// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/file_dialog.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool is_pattern_character(char value) noexcept {
    const auto c = static_cast<unsigned char>(value);
    return std::isalnum(c) != 0 || value == '-' || value == '_' || value == '.';
}

[[nodiscard]] bool is_valid_filter_pattern(std::string_view pattern) noexcept {
    if (pattern.empty()) {
        return false;
    }
    if (pattern == "*") {
        return true;
    }

    std::size_t part_start = 0;
    while (part_start <= pattern.size()) {
        const auto part_end = pattern.find(';', part_start);
        const auto part = pattern.substr(part_start, part_end == std::string_view::npos ? std::string_view::npos
                                                                                        : part_end - part_start);
        if (part.empty() || !std::ranges::all_of(part, is_pattern_character)) {
            return false;
        }
        if (part_end == std::string_view::npos) {
            break;
        }
        part_start = part_end + 1;
    }
    return true;
}

[[nodiscard]] FileDialogResult make_result(FileDialogId id, const MemoryFileDialogResponse& response) {
    FileDialogResult result{
        .id = id,
        .status = response.status,
        .paths = response.paths,
        .selected_filter = response.selected_filter,
        .error = response.error,
    };
    if (const auto error = validate_file_dialog_result(result); !error.empty()) {
        throw std::invalid_argument(error);
    }
    return result;
}

} // namespace

bool is_valid_file_dialog_filter(const FileDialogFilter& filter) noexcept {
    return !filter.name.empty() && is_valid_filter_pattern(filter.pattern);
}

std::string validate_file_dialog_request(const FileDialogRequest& request) {
    if (request.title.empty()) {
        return "file dialog title must not be empty";
    }
    if (request.kind == FileDialogKind::save_file && request.allow_many) {
        return "save file dialogs cannot allow multiple selections";
    }
    for (const auto& filter : request.filters) {
        if (!is_valid_file_dialog_filter(filter)) {
            return "file dialog filter is invalid";
        }
    }
    return {};
}

std::string validate_file_dialog_result(const FileDialogResult& result) {
    if (result.id == 0) {
        return "file dialog result id must be non-zero";
    }
    if (result.status == FileDialogStatus::accepted && result.paths.empty()) {
        return "accepted file dialog results must contain at least one path";
    }
    if (result.status == FileDialogStatus::canceled && !result.paths.empty()) {
        return "canceled file dialog results must not contain paths";
    }
    if (result.status == FileDialogStatus::failed && result.error.empty()) {
        return "failed file dialog results must include an error";
    }
    if (result.status != FileDialogStatus::failed && !result.error.empty()) {
        return "successful or canceled file dialog results must not include errors";
    }
    return {};
}

void FileDialogResultQueue::push(FileDialogResult result) {
    if (const auto error = validate_file_dialog_result(result); !error.empty()) {
        throw std::invalid_argument(error);
    }

    std::lock_guard lock(mutex_);
    results_.push_back(std::move(result));
}

std::optional<FileDialogResult> FileDialogResultQueue::poll(FileDialogId id) {
    std::lock_guard lock(mutex_);
    const auto it = std::ranges::find_if(results_, [id](const FileDialogResult& result) { return result.id == id; });
    if (it == results_.end()) {
        return std::nullopt;
    }

    auto result = std::move(*it);
    results_.erase(it);
    return result;
}

void MemoryFileDialogService::enqueue_response(MemoryFileDialogResponse response) {
    responses_.push_back(std::move(response));
}

FileDialogId MemoryFileDialogService::show(FileDialogRequest request) {
    if (const auto error = validate_file_dialog_request(request); !error.empty()) {
        throw std::invalid_argument(error);
    }

    const auto id = next_id_++;
    last_request_ = std::move(request);
    if (!responses_.empty()) {
        const auto response = std::move(responses_.front());
        responses_.pop_front();
        results_.push(make_result(id, response));
    }
    return id;
}

std::optional<FileDialogResult> MemoryFileDialogService::poll_result(FileDialogId id) {
    return results_.poll(id);
}

const std::optional<FileDialogRequest>& MemoryFileDialogService::last_request() const noexcept {
    return last_request_;
}

} // namespace mirakana
