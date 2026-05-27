// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <string>
#include <string_view>

namespace mirakana::win32::detail {

[[nodiscard]] std::wstring wide_from_utf8(std::string_view text);
[[nodiscard]] std::string utf8_from_wide(std::wstring_view text);
[[nodiscard]] std::u16string utf16_from_utf8(std::string_view text);
[[nodiscard]] std::string utf8_from_utf16(std::u16string_view text);

} // namespace mirakana::win32::detail
