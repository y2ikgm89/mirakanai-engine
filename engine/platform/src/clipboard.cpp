// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/clipboard.hpp"

namespace mirakana {

bool MemoryClipboard::has_text() const {
    return !text_.empty();
}

std::string MemoryClipboard::text() const {
    return text_;
}

void MemoryClipboard::set_text(std::string_view text) {
    text_ = std::string{text};
}

void MemoryClipboard::clear() {
    text_.clear();
}

} // namespace mirakana
