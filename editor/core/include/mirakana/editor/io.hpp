// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/editor/project.hpp"
#include "mirakana/editor/workspace.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace mirakana::editor {

class ITextStore {
  public:
    virtual ~ITextStore() = default;

    [[nodiscard]] virtual bool exists(std::string_view path) const = 0;
    [[nodiscard]] virtual std::string read_text(std::string_view path) const = 0;
    virtual void write_text(std::string_view path, std::string_view text) = 0;
};

class MemoryTextStore final : public ITextStore {
  public:
    [[nodiscard]] bool exists(std::string_view path) const override;
    [[nodiscard]] std::string read_text(std::string_view path) const override;
    void write_text(std::string_view path, std::string_view text) override;

  private:
    std::unordered_map<std::string, std::string> files_;
};

class FileTextStore final : public ITextStore {
  public:
    explicit FileTextStore(std::filesystem::path root_path);

    [[nodiscard]] bool exists(std::string_view path) const override;
    [[nodiscard]] std::string read_text(std::string_view path) const override;
    void write_text(std::string_view path, std::string_view text) override;

  private:
    [[nodiscard]] std::filesystem::path resolve(std::string_view path) const;

    std::filesystem::path root_path_;
};

struct ProjectBundlePaths {
    std::string project_path;
    std::string workspace_path;
    std::string scene_path;
};

struct ProjectBundle {
    ProjectDocument project;
    Workspace workspace;
    std::string scene_text;
    bool dirty{false};
};

class DocumentDirtyState {
  public:
    [[nodiscard]] bool dirty() const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;
    [[nodiscard]] std::uint64_t saved_revision() const noexcept;

    void mark_dirty() noexcept;
    void mark_saved() noexcept;
    void reset_clean() noexcept;

  private:
    std::uint64_t revision_{0};
    std::uint64_t saved_revision_{0};
};

void validate_project_bundle_paths(const ProjectBundlePaths& paths);

void save_project_bundle(ITextStore& store, const ProjectBundlePaths& paths, const ProjectDocument& project,
                         const Workspace& workspace, std::string_view scene_text);

[[nodiscard]] ProjectBundle load_project_bundle(ITextStore& store, const ProjectBundlePaths& paths);

} // namespace mirakana::editor
