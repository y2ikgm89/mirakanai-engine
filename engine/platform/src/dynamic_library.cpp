// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/dynamic_library.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>

#include <cctype>
#include <utility>

namespace mirakana {
namespace {

#if defined(_WIN32)
[[nodiscard]] std::string last_error_message(std::string_view prefix) {
    const auto error = GetLastError();
    LPSTR buffer = nullptr;
    const auto chars = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

    std::string message(prefix);
    message.append(" failed with error ");
    message.append(std::to_string(error));
    if (chars != 0 && buffer != nullptr) {
        message.append(": ");
        message.append(buffer, chars);
        LocalFree(buffer);
    }
    return message;
}
#endif

[[nodiscard]] bool has_embedded_null(std::string_view text) noexcept {
    return text.find('\0') != std::string_view::npos;
}

[[nodiscard]] bool is_safe_dynamic_library_path(const std::filesystem::path& path) {
    if (path.empty() || !path.is_absolute()) {
        return false;
    }
    const auto generic = path.generic_string();
    return !has_embedded_null(generic);
}

} // namespace

struct DynamicLibraryState {
#if defined(_WIN32)
    explicit DynamicLibraryState(HMODULE module_handle) noexcept : module(module_handle) {}

    DynamicLibraryState(const DynamicLibraryState&) = delete;
    DynamicLibraryState& operator=(const DynamicLibraryState&) = delete;

    ~DynamicLibraryState() {
        if (module != nullptr) {
            FreeLibrary(module);
        }
    }

    HMODULE module{nullptr};
#else
    DynamicLibraryState() = default;
#endif
};

DynamicLibrary::DynamicLibrary() noexcept = default;
DynamicLibrary::DynamicLibrary(std::unique_ptr<DynamicLibraryState> state) noexcept : state_(std::move(state)) {}
DynamicLibrary::DynamicLibrary(DynamicLibrary&& other) noexcept = default;
DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& other) noexcept = default;
DynamicLibrary::~DynamicLibrary() = default;

bool DynamicLibrary::loaded() const noexcept {
#if defined(_WIN32)
    return state_ != nullptr && state_->module != nullptr;
#else
    return state_ != nullptr;
#endif
}

void DynamicLibrary::reset() noexcept {
    state_.reset();
}

bool is_safe_dynamic_library_symbol_name(std::string_view symbol_name) noexcept {
    if (symbol_name.empty() || has_embedded_null(symbol_name)) {
        return false;
    }

    const auto first = static_cast<unsigned char>(symbol_name.front());
    if (std::isalpha(first) == 0 && symbol_name.front() != '_') {
        return false;
    }

    return std::ranges::all_of(symbol_name, [](const auto character) {
        return std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_';
    });
}

DynamicLibraryLoadResult load_dynamic_library(const std::filesystem::path& path) {
    DynamicLibraryLoadResult result;
    if (!is_safe_dynamic_library_path(path)) {
        result.status = DynamicLibraryLoadStatus::blocked;
        result.diagnostic = "dynamic library path must be absolute and contain no embedded null bytes";
        return result;
    }

#if defined(_WIN32)
    const auto wide_path = path.wstring();
    const auto flags = static_cast<DWORD>(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    SetLastError(0);
    auto* const module = LoadLibraryExW(wide_path.c_str(), nullptr, flags);
    if (module == nullptr) {
        result.status = DynamicLibraryLoadStatus::failed;
        result.diagnostic = last_error_message("LoadLibraryExW");
        return result;
    }

    result.status = DynamicLibraryLoadStatus::loaded;
    result.library = DynamicLibrary(std::make_unique<DynamicLibraryState>(module));
    return result;
#else
    result.status = DynamicLibraryLoadStatus::unsupported;
    result.diagnostic = "dynamic library loading is not implemented for this host";
    return result;
#endif
}

DynamicLibrarySymbolResult resolve_dynamic_library_symbol(const DynamicLibrary& library, std::string_view symbol_name) {
    DynamicLibrarySymbolResult result;
    if (!library.loaded()) {
        result.status = DynamicLibrarySymbolStatus::unloaded;
        result.diagnostic = "dynamic library is not loaded";
        return result;
    }
    if (!is_safe_dynamic_library_symbol_name(symbol_name)) {
        result.status = DynamicLibrarySymbolStatus::blocked;
        result.diagnostic = "dynamic library symbol name is unsafe";
        return result;
    }

#if defined(_WIN32)
    const std::string symbol(symbol_name);
    SetLastError(0);
    const auto address = GetProcAddress(library.state_->module, symbol.c_str());
    if (address == nullptr) {
        result.status = DynamicLibrarySymbolStatus::missing;
        result.diagnostic = last_error_message("GetProcAddress");
        return result;
    }

    result.status = DynamicLibrarySymbolStatus::resolved;
    result.address = reinterpret_cast<void*>(address);
    return result;
#else
    result.status = DynamicLibrarySymbolStatus::unsupported;
    result.diagnostic = "dynamic library symbol resolution is not implemented for this host";
    return result;
#endif
}

} // namespace mirakana
