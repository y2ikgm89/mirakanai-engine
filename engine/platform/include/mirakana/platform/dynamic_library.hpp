// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace mirakana {

enum class DynamicLibraryLoadStatus : std::uint8_t {
    loaded,
    blocked,
    failed,
    unsupported,
};

enum class DynamicLibrarySymbolStatus : std::uint8_t {
    resolved,
    blocked,
    missing,
    failed,
    unloaded,
    unsupported,
};

struct DynamicLibraryState;
struct DynamicLibraryLoadResult;
struct DynamicLibrarySymbolResult;

class DynamicLibrary final {
  public:
    DynamicLibrary() noexcept;
    DynamicLibrary(const DynamicLibrary&) = delete;
    DynamicLibrary& operator=(const DynamicLibrary&) = delete;
    DynamicLibrary(DynamicLibrary&& other) noexcept;
    DynamicLibrary& operator=(DynamicLibrary&& other) noexcept;
    ~DynamicLibrary();

    [[nodiscard]] bool loaded() const noexcept;
    void reset() noexcept;

  private:
    explicit DynamicLibrary(std::unique_ptr<DynamicLibraryState> state) noexcept;

    std::unique_ptr<DynamicLibraryState> state_;

    friend struct DynamicLibraryLoadResult;
    friend struct DynamicLibrarySymbolResult;
    friend DynamicLibraryLoadResult load_dynamic_library(const std::filesystem::path& path);
    friend DynamicLibrarySymbolResult resolve_dynamic_library_symbol(const DynamicLibrary& library,
                                                                     std::string_view symbol_name);
};

struct DynamicLibraryLoadResult {
    DynamicLibraryLoadStatus status{DynamicLibraryLoadStatus::blocked};
    std::string diagnostic;
    DynamicLibrary library;

    [[nodiscard]] bool succeeded() const noexcept {
        return status == DynamicLibraryLoadStatus::loaded && library.loaded();
    }
};

struct DynamicLibrarySymbolResult {
    DynamicLibrarySymbolStatus status{DynamicLibrarySymbolStatus::blocked};
    std::string diagnostic;
    void* address{nullptr};

    [[nodiscard]] bool succeeded() const noexcept {
        return status == DynamicLibrarySymbolStatus::resolved && address != nullptr;
    }
};

[[nodiscard]] bool is_safe_dynamic_library_symbol_name(std::string_view symbol_name) noexcept;
[[nodiscard]] DynamicLibraryLoadResult load_dynamic_library(const std::filesystem::path& path);
[[nodiscard]] DynamicLibrarySymbolResult resolve_dynamic_library_symbol(const DynamicLibrary& library,
                                                                        std::string_view symbol_name);

} // namespace mirakana
