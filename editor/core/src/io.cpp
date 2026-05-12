// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/io.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace mirakana::editor {
namespace {

void validate_path(std::string_view path, const char* name) {
    if (path.empty()) {
        throw std::invalid_argument(std::string(name) + " must not be empty");
    }
    if (path.find_first_of("\r\n=") != std::string_view::npos) {
        throw std::invalid_argument(std::string(name) + " must not contain newlines or '='");
    }
}

[[nodiscard]] bool is_absolute_like(std::string_view path) noexcept {
    return path.starts_with('/') || path.starts_with('\\') ||
           (path.size() >= 2 && path[1] == ':' &&
            ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')));
}

[[nodiscard]] bool has_parent_segment(std::string_view path) noexcept {
    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto separator = path.find_first_of("/\\", begin);
        const auto end = separator == std::string_view::npos ? path.size() : separator;
        if (path.substr(begin, end - begin) == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        begin = separator + 1;
    }
    return false;
}

void validate_store_path(std::string_view path) {
    validate_path(path, "text store path");
    if (path.empty()) {
        throw std::invalid_argument("text store path must not be empty");
    }
    if (is_absolute_like(path)) {
        throw std::invalid_argument("text store path must be relative");
    }
    if (has_parent_segment(path)) {
        throw std::invalid_argument("text store path must not contain '..'");
    }
}

} // namespace

bool MemoryTextStore::exists(std::string_view path) const {
    validate_store_path(path);
    return files_.find(std::string(path)) != files_.end();
}

std::string MemoryTextStore::read_text(std::string_view path) const {
    validate_store_path(path);
    const auto it = files_.find(std::string(path));
    if (it == files_.end()) {
        throw std::out_of_range("text store path does not exist");
    }
    return it->second;
}

void MemoryTextStore::write_text(std::string_view path, std::string_view text) {
    validate_store_path(path);
    files_[std::string(path)] = std::string(text);
}

FileTextStore::FileTextStore(std::filesystem::path root_path) : root_path_(std::move(root_path)) {
    if (root_path_.empty()) {
        throw std::invalid_argument("file text store root path must not be empty");
    }
}

bool FileTextStore::exists(std::string_view path) const {
    return std::filesystem::exists(resolve(path));
}

std::string FileTextStore::read_text(std::string_view path) const {
    const auto full_path = resolve(path);
    std::ifstream input(full_path, std::ios::binary);
    if (!input) {
        throw std::out_of_range("text store path does not exist");
    }

    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void FileTextStore::write_text(std::string_view path, std::string_view text) {
    const auto full_path = resolve(path);
    if (const auto parent = full_path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream output(full_path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to open text store path for write");
    }
    output << text;
}

std::filesystem::path FileTextStore::resolve(std::string_view path) const {
    validate_store_path(path);
    return root_path_ / std::filesystem::path{std::string(path)};
}

bool DocumentDirtyState::dirty() const noexcept {
    return revision_ != saved_revision_;
}

std::uint64_t DocumentDirtyState::revision() const noexcept {
    return revision_;
}

std::uint64_t DocumentDirtyState::saved_revision() const noexcept {
    return saved_revision_;
}

void DocumentDirtyState::mark_dirty() noexcept {
    ++revision_;
}

void DocumentDirtyState::mark_saved() noexcept {
    saved_revision_ = revision_;
}

void DocumentDirtyState::reset_clean() noexcept {
    revision_ = 0;
    saved_revision_ = 0;
}

void validate_project_bundle_paths(const ProjectBundlePaths& paths) {
    validate_path(paths.project_path, "project file path");
    validate_path(paths.workspace_path, "workspace file path");
    validate_path(paths.scene_path, "scene file path");
}

void save_project_bundle(ITextStore& store, const ProjectBundlePaths& paths, const ProjectDocument& project,
                         const Workspace& workspace, std::string_view scene_text) {
    validate_project_bundle_paths(paths);
    validate_project_document(project);
    if (scene_text.empty()) {
        throw std::invalid_argument("scene text must not be empty");
    }

    store.write_text(paths.project_path, serialize_project_document(project));
    store.write_text(paths.workspace_path, serialize_workspace(workspace));
    store.write_text(paths.scene_path, scene_text);
}

ProjectBundle load_project_bundle(ITextStore& store, const ProjectBundlePaths& paths) {
    validate_project_bundle_paths(paths);

    auto project = deserialize_project_document(store.read_text(paths.project_path));
    auto workspace = deserialize_workspace(store.read_text(paths.workspace_path));
    auto scene_text = store.read_text(paths.scene_path);
    if (scene_text.empty()) {
        throw std::invalid_argument("scene text must not be empty");
    }

    return ProjectBundle{
        .project = std::move(project),
        .workspace = std::move(workspace),
        .scene_text = std::move(scene_text),
        .dirty = false,
    };
}

} // namespace mirakana::editor
