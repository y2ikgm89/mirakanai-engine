// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <memory>

namespace mirakana::editor {

[[nodiscard]] std::unique_ptr<ui::ITextShapingAdapter> make_native_editor_directwrite_text_shaping_adapter();

[[nodiscard]] std::unique_ptr<ui::IFontRasterizerAdapter> make_native_editor_directwrite_font_rasterizer_adapter();

[[nodiscard]] bool native_editor_directwrite_text_font_adapters_expose_native_handles() noexcept;

} // namespace mirakana::editor
