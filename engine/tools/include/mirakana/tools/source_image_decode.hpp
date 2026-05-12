// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <optional>
#include <span>

namespace mirakana {

/// Decodes an audited PNG file into an RGBA8 `TextureSourceDocument` using the same libspng
/// policy as `PngTextureExternalAssetImporter`. When `MK_ENABLE_ASSET_IMPORTERS=OFF`, throws
/// `std::runtime_error` with the same feature-disabled wording as other importer adapters.
[[nodiscard]] TextureSourceDocument decode_audited_png_rgba8(std::span<const std::uint8_t> encoded_png_bytes);

class PngImageDecodingAdapter final : public ui::IImageDecodingAdapter {
  public:
    [[nodiscard]] std::optional<ui::ImageDecodeResult> decode_image(const ui::ImageDecodeRequest& request) override;
};

} // namespace mirakana
