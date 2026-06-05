// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/filesystem.hpp"

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
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

[[nodiscard]] constexpr char native_narrow_separator() noexcept {
#if defined(_WIN32)
    return '\\';
#else
    return '/';
#endif
}

[[nodiscard]] std::filesystem::path filesystem_api_path(const std::filesystem::path& path) {
#if defined(_WIN32)
    if (!path.is_absolute()) {
        return path;
    }

    const std::wstring text = path.wstring();
    constexpr std::wstring_view long_path_prefix{L"\\\\?\\"};
    constexpr std::wstring_view device_path_prefix{L"\\\\.\\"};
    constexpr std::wstring_view unc_prefix{L"\\\\"};
    if (text.starts_with(long_path_prefix) || text.starts_with(device_path_prefix)) {
        return path;
    }
    if (text.starts_with(unc_prefix)) {
        return std::filesystem::path{std::wstring{L"\\\\?\\UNC\\"} + text.substr(2)};
    }
    return std::filesystem::path{std::wstring{long_path_prefix} + text};
#else
    return path;
#endif
}

[[nodiscard]] std::vector<std::uint8_t> bytes_from_text(std::string_view text) {
    std::vector<std::uint8_t> bytes;
    bytes.reserve(text.size());
    for (const auto character : text) {
        bytes.push_back(static_cast<std::uint8_t>(character));
    }
    return bytes;
}

[[nodiscard]] std::string text_from_bytes(std::span<const std::uint8_t> bytes) {
    std::string text;
    text.reserve(bytes.size());
    for (const auto byte : bytes) {
        text.push_back(static_cast<char>(byte));
    }
    return text;
}

[[nodiscard]] std::vector<std::uint8_t> slice_bytes(std::string_view path, std::span<const std::uint8_t> bytes,
                                                    std::uint64_t byte_offset, std::uint64_t byte_size) {
    if (byte_offset > static_cast<std::uint64_t>(bytes.size()) ||
        byte_size > static_cast<std::uint64_t>(bytes.size()) - byte_offset) {
        throw std::out_of_range("file byte range is outside file: " + std::string(path));
    }
    const auto offset = static_cast<std::size_t>(byte_offset);
    const auto size = static_cast<std::size_t>(byte_size);
    return std::vector<std::uint8_t>(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
                                     bytes.begin() + static_cast<std::ptrdiff_t>(offset + size));
}

[[nodiscard]] std::uint64_t checked_file_size(std::ifstream& input, std::string_view path) {
    input.seekg(0, std::ios::end);
    const auto end = input.tellg();
    if (end < std::streampos{0}) {
        throw std::runtime_error("failed to query file size: " + std::string(path));
    }
    input.seekg(0, std::ios::beg);
    return static_cast<std::uint64_t>(end);
}

} // namespace

std::vector<std::uint8_t> IFileSystem::read_bytes(std::string_view path) const {
    return bytes_from_text(read_text(path));
}

std::vector<std::uint8_t> IFileSystem::read_byte_range(std::string_view path, std::uint64_t byte_offset,
                                                       std::uint64_t byte_size) const {
    return slice_bytes(path, read_bytes(path), byte_offset, byte_size);
}

void IFileSystem::write_bytes(std::string_view path, std::span<const std::uint8_t> bytes) {
    const auto text = text_from_bytes(bytes);
    write_text(path, text);
}

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

std::vector<std::uint8_t> MemoryFileSystem::read_bytes(std::string_view path) const {
    const auto it = files_.find(std::string(path));
    if (it == files_.end()) {
        throw std::out_of_range("file does not exist");
    }
    return bytes_from_text(it->second);
}

std::vector<std::uint8_t> MemoryFileSystem::read_byte_range(std::string_view path, std::uint64_t byte_offset,
                                                            std::uint64_t byte_size) const {
    const auto it = files_.find(std::string(path));
    if (it == files_.end()) {
        throw std::out_of_range("file does not exist");
    }
    const auto bytes = bytes_from_text(it->second);
    return slice_bytes(path, bytes, byte_offset, byte_size);
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

void MemoryFileSystem::write_bytes(std::string_view path, std::span<const std::uint8_t> bytes) {
    if (path.empty()) {
        throw std::invalid_argument("path must not be empty");
    }
    files_[std::string(path)] = text_from_bytes(bytes);
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
    return std::filesystem::exists(filesystem_api_path(resolve(path)));
}

bool RootedFileSystem::is_directory(std::string_view path) const {
    return std::filesystem::is_directory(filesystem_api_path(resolve(path)));
}

std::string RootedFileSystem::read_text(std::string_view path) const {
    const auto full_path = filesystem_api_path(resolve(path));
    std::ifstream input(full_path, std::ios::binary);
    if (!input) {
        throw std::out_of_range("file does not exist: " + std::string(path));
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> RootedFileSystem::read_bytes(std::string_view path) const {
    const auto text = read_text(path);
    return bytes_from_text(text);
}

std::vector<std::uint8_t> RootedFileSystem::read_byte_range(std::string_view path, std::uint64_t byte_offset,
                                                            std::uint64_t byte_size) const {
    const auto full_path = filesystem_api_path(resolve(path));
    std::ifstream input(full_path, std::ios::binary);
    if (!input) {
        throw std::out_of_range("file does not exist: " + std::string(path));
    }

    const auto file_size = checked_file_size(input, path);
    if (byte_offset > file_size || byte_size > file_size - byte_offset) {
        throw std::out_of_range("file byte range is outside file: " + std::string(path));
    }
    constexpr auto max_read_size = std::min(static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()),
                                            static_cast<std::uint64_t>(std::numeric_limits<std::streamsize>::max()));
    if (byte_size > max_read_size) {
        throw std::length_error("file byte range is too large: " + std::string(path));
    }

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(byte_size));
    input.seekg(static_cast<std::streamoff>(byte_offset), std::ios::beg);
    if (!input) {
        throw std::runtime_error("failed to seek file byte range: " + std::string(path));
    }
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (input.gcount() != static_cast<std::streamsize>(bytes.size())) {
        throw std::runtime_error("failed to read file byte range: " + std::string(path));
    }
    return bytes;
}

std::vector<std::string> RootedFileSystem::list_files(std::string_view root) const {
    const auto full_root = filesystem_api_path(resolve(root));
    const auto relative_root = filesystem_api_path(root_path_);
    std::vector<std::string> result;
    if (!std::filesystem::exists(full_root)) {
        return result;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(full_root)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        result.push_back(relative_portable_path(std::filesystem::relative(entry.path(), relative_root)));
    }
    std::ranges::sort(result);
    return result;
}

void RootedFileSystem::write_text(std::string_view path, std::string_view text) {
    const auto full_path = filesystem_api_path(resolve(path));
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

void RootedFileSystem::write_bytes(std::string_view path, std::span<const std::uint8_t> bytes) {
    write_text(path, text_from_bytes(bytes));
}

void RootedFileSystem::remove(std::string_view path) {
    const auto full_path = filesystem_api_path(resolve(path));
    std::error_code error;
    std::filesystem::remove(full_path, error);
    if (error) {
        throw std::runtime_error("failed to remove file: " + std::string(path) + ": " + error.message());
    }
}

void RootedFileSystem::remove_empty_directory(std::string_view path) {
    const auto full_path = filesystem_api_path(resolve(path));
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
    std::string native_path{path};
    std::ranges::replace(native_path, '/', native_narrow_separator());
    return root_path_ / std::filesystem::path{native_path};
}

std::string RootedFileSystem::relative_portable_path(const std::filesystem::path& path) {
    auto text = path.generic_string();
    while (!text.empty() && text.front() == '/') {
        text.erase(text.begin());
    }
    return text;
}

} // namespace mirakana
