// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mirakana {

class IFileSystem {
  public:
    virtual ~IFileSystem() = default;

    [[nodiscard]] virtual bool exists(std::string_view path) const = 0;
    [[nodiscard]] virtual bool is_directory(std::string_view path) const = 0;
    [[nodiscard]] virtual std::string read_text(std::string_view path) const = 0;
    [[nodiscard]] virtual std::vector<std::string> list_files(std::string_view root) const = 0;

    virtual void write_text(std::string_view path, std::string_view text) = 0;
    virtual void remove(std::string_view path) = 0;
    virtual void remove_empty_directory(std::string_view path) = 0;
};

class MemoryFileSystem final : public IFileSystem {
  public:
    [[nodiscard]] bool exists(std::string_view path) const override;
    [[nodiscard]] bool is_directory(std::string_view path) const override;
    [[nodiscard]] std::string read_text(std::string_view path) const override;
    [[nodiscard]] std::vector<std::string> list_files(std::string_view root) const override;

    void write_text(std::string_view path, std::string_view text) override;
    void remove(std::string_view path) override;
    void remove_empty_directory(std::string_view path) override;

  private:
    std::unordered_map<std::string, std::string> files_;
};

class RootedFileSystem final : public IFileSystem {
  public:
    explicit RootedFileSystem(std::filesystem::path root_path);

    [[nodiscard]] bool exists(std::string_view path) const override;
    [[nodiscard]] bool is_directory(std::string_view path) const override;
    [[nodiscard]] std::string read_text(std::string_view path) const override;
    [[nodiscard]] std::vector<std::string> list_files(std::string_view root) const override;

    void write_text(std::string_view path, std::string_view text) override;
    void remove(std::string_view path) override;
    void remove_empty_directory(std::string_view path) override;

    [[nodiscard]] const std::filesystem::path& root_path() const noexcept;

  private:
    [[nodiscard]] std::filesystem::path resolve(std::string_view path) const;
    [[nodiscard]] static std::string relative_portable_path(const std::filesystem::path& path);

    std::filesystem::path root_path_;
};

} // namespace mirakana
