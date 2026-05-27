// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "win32_utf.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <limits>
#include <stdexcept>

namespace mirakana::win32::detail {
namespace {

[[nodiscard]] int checked_size(std::size_t size) {
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("text is too large for Win32 UTF conversion");
    }
    return static_cast<int>(size);
}

} // namespace

std::wstring wide_from_utf8(std::string_view text) {
    if (text.empty()) {
        return {};
    }

    const std::string source{text};
    const int source_size = checked_size(source.size());
    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source.c_str(), source_size, nullptr, 0);
    if (required <= 0) {
        throw std::invalid_argument("text must be valid UTF-8");
    }

    std::wstring result(static_cast<std::size_t>(required), L'\0');
    const int written =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source.c_str(), source_size, result.data(), required);
    if (written != required) {
        throw std::runtime_error("UTF-8 to UTF-16 conversion failed");
    }
    return result;
}

std::string utf8_from_wide(std::wstring_view text) {
    if (text.empty()) {
        return {};
    }

    const std::wstring source{text};
    const int source_size = checked_size(source.size());
    const int required =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, source.c_str(), source_size, nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        throw std::invalid_argument("text must be valid UTF-16");
    }

    std::string result(static_cast<std::size_t>(required), '\0');
    const int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, source.c_str(), source_size, result.data(),
                                            required, nullptr, nullptr);
    if (written != required) {
        throw std::runtime_error("UTF-16 to UTF-8 conversion failed");
    }
    return result;
}

std::u16string utf16_from_utf8(std::string_view text) {
    const auto wide = wide_from_utf8(text);
    std::u16string result;
    result.reserve(wide.size());
    for (const wchar_t value : wide) {
        result.push_back(static_cast<char16_t>(value));
    }
    return result;
}

std::string utf8_from_utf16(std::u16string_view text) {
    std::wstring wide;
    wide.reserve(text.size());
    for (const char16_t value : text) {
        wide.push_back(static_cast<wchar_t>(value));
    }
    return utf8_from_wide(wide);
}

} // namespace mirakana::win32::detail
