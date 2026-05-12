// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/source_image_decode.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifndef MK_HAS_ASSET_IMPORTERS
#define MK_HAS_ASSET_IMPORTERS 0
#endif

#if MK_HAS_ASSET_IMPORTERS
#include <spng.h>
#endif

namespace mirakana {
namespace {

constexpr std::uint32_t max_texture_extent = 16384;
constexpr std::size_t max_png_decoded_bytes = 512ULL * 1024ULL * 1024ULL;

[[noreturn]] void throw_feature_disabled() {
    throw std::runtime_error("asset-importers feature is disabled for this build");
}

#if MK_HAS_ASSET_IMPORTERS

[[nodiscard]] std::runtime_error spng_error(std::string_view action, int error) {
    return std::runtime_error(std::string(action) + ": " + spng_strerror(error));
}

#endif

} // namespace

TextureSourceDocument decode_audited_png_rgba8(std::span<const std::uint8_t> encoded_png_bytes) {
#if MK_HAS_ASSET_IMPORTERS
    if (encoded_png_bytes.empty()) {
        throw std::invalid_argument("PNG source is empty");
    }

    std::unique_ptr<spng_ctx, decltype(&spng_ctx_free)> context{spng_ctx_new(0), spng_ctx_free};
    if (!context) {
        throw std::runtime_error("failed to allocate PNG decoder context");
    }
    if (const int error = spng_set_image_limits(context.get(), max_texture_extent, max_texture_extent);
        error != SPNG_OK) {
        throw spng_error("failed to set PNG image limits", error);
    }
    if (const int error = spng_set_png_buffer(context.get(), encoded_png_bytes.data(), encoded_png_bytes.size());
        error != SPNG_OK) {
        throw spng_error("failed to set PNG buffer", error);
    }

    spng_ihdr ihdr{};
    if (const int error = spng_get_ihdr(context.get(), &ihdr); error != SPNG_OK) {
        throw spng_error("failed to read PNG IHDR", error);
    }

    std::size_t decoded_size = 0;
    if (const int error = spng_decoded_image_size(context.get(), SPNG_FMT_RGBA8, &decoded_size); error != SPNG_OK) {
        throw spng_error("failed to size decoded PNG", error);
    }
    if (decoded_size == 0 || decoded_size > max_png_decoded_bytes) {
        throw std::runtime_error("decoded PNG size exceeds importer limits");
    }

    std::vector<std::uint8_t> decoded_bytes(decoded_size);
    if (const int error =
            spng_decode_image(context.get(), decoded_bytes.data(), decoded_bytes.size(), SPNG_FMT_RGBA8, 0);
        error != SPNG_OK) {
        throw spng_error("failed to decode PNG", error);
    }

    return TextureSourceDocument{
        ihdr.width,
        ihdr.height,
        TextureSourcePixelFormat::rgba8_unorm,
        std::move(decoded_bytes),
    };
#else
    (void)encoded_png_bytes;
    throw_feature_disabled();
#endif
}

std::optional<ui::ImageDecodeResult> PngImageDecodingAdapter::decode_image(const ui::ImageDecodeRequest& request) {
    try {
        std::vector<std::uint8_t> encoded_bytes;
        encoded_bytes.reserve(request.bytes.size());
        for (const std::byte byte : request.bytes) {
            encoded_bytes.push_back(std::to_integer<std::uint8_t>(byte));
        }

        const auto decoded = decode_audited_png_rgba8(encoded_bytes);
        if (decoded.pixel_format != TextureSourcePixelFormat::rgba8_unorm) {
            return std::nullopt;
        }

        std::vector<std::byte> pixels;
        pixels.reserve(decoded.bytes.size());
        for (const std::uint8_t byte : decoded.bytes) {
            pixels.push_back(static_cast<std::byte>(byte));
        }

        return ui::ImageDecodeResult{
            .width = decoded.width,
            .height = decoded.height,
            .pixel_format = ui::ImageDecodePixelFormat::rgba8_unorm,
            .pixels = std::move(pixels),
        };
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace mirakana
