// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/filesystem.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

void validate_relative_path(std::string_view path, const char* name) {
    if (path.empty()) {
        throw std::invalid_argument(std::string(name) + " must not be empty");
    }
    if (path.find('\n') != std::string_view::npos || path.find('\r') != std::string_view::npos ||
        path.find('\0') != std::string_view::npos) {
        throw std::invalid_argument(std::string(name) + " must not contain control characters");
    }

    const std::filesystem::path parsed{std::string(path)};
    if (parsed.is_absolute()) {
        throw std::invalid_argument(std::string(name) + " must be relative");
    }
    for (const auto& part : parsed) {
        if (part == "..") {
            throw std::invalid_argument(std::string(name) + " must not contain '..'");
        }
    }
}

} // namespace

bool MemoryFileSystem::exists(std::string_view path) const {
    return files_.find(std::string(path)) != files_.end();
}

bool MemoryFileSystem::is_directory(std::string_view path) const {
    if (path.empty()) {
        return false;
    }

    const std::string prefix = std::string(path) + "/";
    return std::ranges::any_of(files_, [&prefix](const auto& file) { return file.first.rfind(prefix, 0) == 0; });
}

std::string MemoryFileSystem::read_text(std::string_view path) const {
    const auto it = files_.find(std::string(path));
    if (it == files_.end()) {
        throw std::out_of_range("file does not exist");
    }
    return it->second;
}

std::vector<std::string> MemoryFileSystem::list_files(std::string_view root) const {
    std::vector<std::string> result;
    const std::string prefix(root);
    for (const auto& [path, _] : files_) {
        if (path.starts_with(prefix)) {
            result.push_back(path);
        }
    }
    std::ranges::sort(result);
    return result;
}

void MemoryFileSystem::write_text(std::string_view path, std::string_view text) {
    if (path.empty()) {
        throw std::invalid_argument("path must not be empty");
    }
    files_[std::string(path)] = std::string(text);
}

void MemoryFileSystem::remove(std::string_view path) {
    if (path.empty()) {
        throw std::invalid_argument("path must not be empty");
    }
    files_.erase(std::string(path));
}

void MemoryFileSystem::remove_empty_directory(std::string_view path) {
    if (path.empty()) {
        throw std::invalid_argument("path must not be empty");
    }
}

RootedFileSystem::RootedFileSystem(std::filesystem::path root_path) : root_path_(std::move(root_path)) {
    if (root_path_.empty()) {
        throw std::invalid_argument("rooted filesystem root path must not be empty");
    }
}

bool RootedFileSystem::exists(std::string_view path) const {
    return std::filesystem::exists(resolve(path));
}

bool RootedFileSystem::is_directory(std::string_view path) const {
    return std::filesystem::is_directory(resolve(path));
}

std::string RootedFileSystem::read_text(std::string_view path) const {
    const auto full_path = resolve(path);
    std::ifstream input(full_path, std::ios::binary);
    if (!input) {
        throw std::out_of_range("file does not exist: " + std::string(path));
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::vector<std::string> RootedFileSystem::list_files(std::string_view root) const {
    const auto full_root = resolve(root);
    std::vector<std::string> result;
    if (!std::filesystem::exists(full_root)) {
        return result;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(full_root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        result.push_back(relative_portable_path(std::filesystem::relative(entry.path(), root_path_)));
    }
    std::ranges::sort(result);
    return result;
}

void RootedFileSystem::write_text(std::string_view path, std::string_view text) {
    const auto full_path = resolve(path);
    if (const auto parent = full_path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream output(full_path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to open file for write: " + std::string(path));
    }
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
    output.close();
    if (!output) {
        throw std::runtime_error("failed to write file: " + std::string(path));
    }
}

void RootedFileSystem::remove(std::string_view path) {
    const auto full_path = resolve(path);
    std::error_code error;
    std::filesystem::remove(full_path, error);
    if (error) {
        throw std::runtime_error("failed to remove file: " + std::string(path) + ": " + error.message());
    }
}

void RootedFileSystem::remove_empty_directory(std::string_view path) {
    const auto full_path = resolve(path);
    if (!std::filesystem::exists(full_path)) {
        return;
    }
    if (!std::filesystem::is_directory(full_path)) {
        throw std::runtime_error("path is not a directory: " + std::string(path));
    }

    std::error_code error;
    std::filesystem::remove(full_path, error);
    if (error) {
        throw std::runtime_error("failed to remove empty directory: " + std::string(path) + ": " + error.message());
    }
}

const std::filesystem::path& RootedFileSystem::root_path() const noexcept {
    return root_path_;
}

std::filesystem::path RootedFileSystem::resolve(std::string_view path) const {
    validate_relative_path(path, "filesystem path");
    return root_path_ / std::filesystem::path{std::string(path)};
}

std::string RootedFileSystem::relative_portable_path(const std::filesystem::path& path) {
    auto text = path.generic_string();
    while (!text.empty() && text.front() == '/') {
        text.erase(text.begin());
    }
    return text;
}

} // namespace mirakana
