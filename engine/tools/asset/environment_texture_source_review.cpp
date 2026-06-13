// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/environment_texture_source_review.hpp"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if MK_HAS_ASSET_IMPORTERS
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfStandardAttributes.h>
#include <OpenEXR/ImfTestFile.h>
#include <OpenEXR/ImfTiledInputFile.h>
#include <ktx.h>
#endif

namespace mirakana {
namespace {

void add_diagnostic(std::vector<OpenExrTextureSourceReviewDiagnostic>& diagnostics,
                    OpenExrTextureSourceReviewDiagnosticCode code, std::string message, std::string path = {}) {
    diagnostics.push_back(OpenExrTextureSourceReviewDiagnostic{
        .code = code,
        .message = std::move(message),
        .path = std::move(path),
    });
}

void add_diagnostic(std::vector<Ktx2BasisTextureSourceReviewDiagnostic>& diagnostics,
                    Ktx2BasisTextureSourceReviewDiagnosticCode code, std::string message, std::string path = {}) {
    diagnostics.push_back(Ktx2BasisTextureSourceReviewDiagnostic{
        .code = code,
        .message = std::move(message),
        .path = std::move(path),
    });
}

[[nodiscard]] bool clean_text_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool hex_digit(char value) noexcept {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}

[[nodiscard]] bool sha256_token(std::string_view value) noexcept {
    constexpr std::string_view prefix = "sha256:";
    return value.starts_with(prefix) && value.size() == prefix.size() + 64U && clean_text_token(value) &&
           std::ranges::all_of(value.substr(prefix.size()), hex_digit);
}

void validate_request(std::vector<OpenExrTextureSourceReviewDiagnostic>& diagnostics,
                      const OpenExrTextureSourceReviewRequest& request) {
    if (request.source_file_path.empty()) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::invalid_request,
                       "OpenEXR source file path is missing");
    }
    if (!clean_text_token(request.source_path)) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::invalid_request,
                       "OpenEXR source path must be a non-empty text token");
    }
    if (!sha256_token(request.source_hash)) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::invalid_request,
                       "OpenEXR source hash must be a sha256 token", request.source_path);
    }
    if (!clean_text_token(request.provenance_id) || !clean_text_token(request.license_id)) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::invalid_request,
                       "OpenEXR provenance and license ids must be non-empty text tokens", request.source_path);
    }
    if (request.sampler_class != TextureSamplerClassV2::environment_radiance) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::invalid_request,
                       "OpenEXR environment texture review requires environment_radiance sampler class",
                       request.source_path);
    }
    if (!request.scene_linear_intent) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::invalid_request,
                       "OpenEXR environment texture review requires explicit scene-linear intent", request.source_path);
    }
}

void validate_request(std::vector<Ktx2BasisTextureSourceReviewDiagnostic>& diagnostics,
                      const Ktx2BasisTextureSourceReviewRequest& request) {
    if (request.source_file_path.empty()) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_request,
                       "KTX2/Basis source file path is missing");
    }
    if (!clean_text_token(request.source_path)) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_request,
                       "KTX2/Basis source path must be a non-empty text token");
    }
    if (!sha256_token(request.source_hash)) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_request,
                       "KTX2/Basis source hash must be a sha256 token", request.source_path);
    }
    if (!clean_text_token(request.provenance_id) || !clean_text_token(request.license_id)) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_request,
                       "KTX2/Basis provenance and license ids must be non-empty text tokens", request.source_path);
    }
    if (request.color_space == TextureColorSpaceV2::unknown ||
        request.sampler_class == TextureSamplerClassV2::unknown) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_request,
                       "KTX2/Basis review requires explicit color space and sampler class intent", request.source_path);
    }
    if (!request.basis_required) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_request,
                       "KTX2/Basis review requires explicit Basis Universal intent", request.source_path);
    }
}

#if MK_HAS_ASSET_IMPORTERS
struct OpenExrChannelRow {
    std::string name;
    TexturePixelEncodingV2 encoding{TexturePixelEncodingV2::unknown};
};

struct KtxTexture2Deleter {
    void operator()(ktxTexture2* texture) const noexcept {
        if (texture != nullptr) {
            ktxTexture_Destroy(ktxTexture(texture));
        }
    }
};

using UniqueKtxTexture2 = std::unique_ptr<ktxTexture2, KtxTexture2Deleter>;

constexpr ktx_uint32_t kKtxVkFormatUndefined = 0U;

[[nodiscard]] TextureSourceWindowV2 source_window_from_box(const IMATH_NAMESPACE::Box2i& box) noexcept {
    return TextureSourceWindowV2{
        .min_x = box.min.x,
        .min_y = box.min.y,
        .max_x = box.max.x,
        .max_y = box.max.y,
    };
}

[[nodiscard]] std::uint32_t window_extent(int min_value, int max_value) noexcept {
    return max_value >= min_value ? static_cast<std::uint32_t>(max_value - min_value + 1) : 0U;
}

[[nodiscard]] TexturePixelEncodingV2 pixel_encoding_from_openexr(OPENEXR_IMF_NAMESPACE::PixelType type) noexcept {
    switch (type) {
    case OPENEXR_IMF_NAMESPACE::HALF:
        return TexturePixelEncodingV2::float16;
    case OPENEXR_IMF_NAMESPACE::FLOAT:
        return TexturePixelEncodingV2::float32;
    case OPENEXR_IMF_NAMESPACE::UINT:
        break;
    }
    return TexturePixelEncodingV2::unknown;
}

[[nodiscard]] std::string_view pixel_encoding_channel_name(TexturePixelEncodingV2 encoding) noexcept {
    switch (encoding) {
    case TexturePixelEncodingV2::float16:
        return "half";
    case TexturePixelEncodingV2::float32:
        return "float";
    case TexturePixelEncodingV2::unknown:
    case TexturePixelEncodingV2::uint8_unorm:
    case TexturePixelEncodingV2::basis_etc1s:
    case TexturePixelEncodingV2::basis_uastc:
        break;
    }
    return "unsupported";
}

[[nodiscard]] int channel_sort_key(std::string_view name) noexcept {
    if (name == "R") {
        return 0;
    }
    if (name == "G") {
        return 1;
    }
    if (name == "B") {
        return 2;
    }
    if (name == "A") {
        return 3;
    }
    return 100;
}

[[nodiscard]] bool has_channel(const std::vector<OpenExrChannelRow>& rows, std::string_view name) noexcept {
    return std::ranges::any_of(rows, [name](const OpenExrChannelRow& row) { return row.name == name; });
}

[[nodiscard]] std::string channel_list_text(const std::vector<OpenExrChannelRow>& rows) {
    std::ostringstream output;
    for (std::size_t index = 0; index < rows.size(); ++index) {
        if (index > 0) {
            output << ',';
        }
        output << rows[index].name << ':' << pixel_encoding_channel_name(rows[index].encoding);
    }
    return output.str();
}

[[nodiscard]] std::string ktx_error_text(KTX_error_code code) {
    const char* const text = ktxErrorString(code);
    return text == nullptr ? "unknown KTX error" : std::string{text};
}

[[nodiscard]] std::string vk_format_name(ktx_uint32_t format) {
    if (format == kKtxVkFormatUndefined) {
        return "VK_FORMAT_UNDEFINED";
    }
    std::ostringstream output;
    output << "VK_FORMAT_" << format;
    return output.str();
}

[[nodiscard]] bool color_transfer_matches_intent(khr_df_transfer_e transfer, TextureColorSpaceV2 color_space) noexcept {
    switch (color_space) {
    case TextureColorSpaceV2::srgb:
        return transfer == KHR_DF_TRANSFER_SRGB;
    case TextureColorSpaceV2::scene_linear:
    case TextureColorSpaceV2::linear_data:
        return transfer == KHR_DF_TRANSFER_LINEAR;
    case TextureColorSpaceV2::unknown:
        break;
    }
    return false;
}

[[nodiscard]] TextureSourceDocumentV2
make_source_document_from_ktx2(std::vector<Ktx2BasisTextureSourceReviewDiagnostic>& diagnostics,
                               const Ktx2BasisTextureSourceReviewRequest& request, ktxTexture2& texture) {
    const ktxTexture* const base = ktxTexture(&texture);
    if (texture.isVideo || base->numDimensions != 2U || base->baseDepth != 1U || base->baseWidth == 0U ||
        base->baseHeight == 0U || base->numLevels == 0U || base->numLayers == 0U ||
        (base->numFaces != 1U && base->numFaces != 6U)) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::unsupported_ktx_layout,
                       "KTX2/Basis review supports non-video 2D textures or cubemaps with explicit levels, layers, "
                       "and 1 or 6 faces",
                       request.source_path);
        return {};
    }

    if (texture.vkFormat != kKtxVkFormatUndefined || !ktxTexture2_NeedsTranscoding(&texture)) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::unsupported_ktx_format,
                       "KTX2/Basis review requires a Basis Universal texture that needs transcoding before GPU upload",
                       request.source_path);
        return {};
    }

    const auto transfer = ktxTexture2_GetTransferFunction_e(&texture);
    if (!color_transfer_matches_intent(transfer, request.color_space)) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_ktx_metadata,
                       "KTX2 transfer function does not match the requested color-space intent", request.source_path);
        return {};
    }

    const auto color_model = ktxTexture2_GetColorModel_e(&texture);
    TextureCompressionKindV2 supercompression{TextureCompressionKindV2::unknown};
    TexturePixelEncodingV2 basis_codec{TexturePixelEncodingV2::unknown};
    switch (color_model) {
    case KHR_DF_MODEL_ETC1S:
        if (texture.supercompressionScheme != KTX_SS_BASIS_LZ) {
            add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_ktx_metadata,
                           "KTX2 ETC1S Basis Universal textures must use BasisLZ supercompression",
                           request.source_path);
            return {};
        }
        supercompression = TextureCompressionKindV2::basis_lz;
        basis_codec = TexturePixelEncodingV2::basis_etc1s;
        break;
    case KHR_DF_MODEL_UASTC:
        if (texture.supercompressionScheme != KTX_SS_NONE && texture.supercompressionScheme != KTX_SS_ZSTD) {
            add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_ktx_metadata,
                           "KTX2 UASTC Basis Universal textures must use no supercompression or Zstd",
                           request.source_path);
            return {};
        }
        supercompression = TextureCompressionKindV2::uastc;
        basis_codec = TexturePixelEncodingV2::basis_uastc;
        break;
    default:
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::unsupported_ktx_format,
                       "KTX2/Basis review supports ETC1S and UASTC color models only", request.source_path);
        return {};
    }

    const TextureSourceDocumentV2 document{
        .source_path = request.source_path,
        .source_hash = request.source_hash,
        .provenance_id = request.provenance_id,
        .license_id = request.license_id,
        .source_kind = TextureSourceKindV2::ktx2_basis,
        .width = base->baseWidth,
        .height = base->baseHeight,
        .color_space = request.color_space,
        .sampler_class = request.sampler_class,
        .openexr = {},
        .ktx2_basis =
            TextureKtx2BasisSourceReviewV2{
                .vk_format = vk_format_name(texture.vkFormat),
                .level_count = base->numLevels,
                .layer_count = base->numLayers,
                .face_count = base->numFaces,
                .supercompression = supercompression,
                .basis_codec = basis_codec,
                .requires_transcoding = true,
            },
    };

    if (!is_valid_texture_source_document_v2(document)) {
        add_diagnostic(diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::invalid_ktx_metadata,
                       "KTX2/Basis metadata does not satisfy GameEngine.TextureSource.v2 contract",
                       request.source_path);
    }
    return document;
}

[[nodiscard]] std::vector<OpenExrChannelRow>
collect_channels(std::vector<OpenExrTextureSourceReviewDiagnostic>& diagnostics,
                 const OPENEXR_IMF_NAMESPACE::ChannelList& channels, const std::string& source_path) {
    std::vector<OpenExrChannelRow> rows;
    for (auto it = channels.begin(); it != channels.end(); ++it) {
        const auto encoding = pixel_encoding_from_openexr(it.channel().type);
        if (encoding == TexturePixelEncodingV2::unknown) {
            add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::unsupported_openexr_channels,
                           "OpenEXR environment textures support HALF or FLOAT channels only", source_path);
            return {};
        }
        if (channel_sort_key(it.name()) == 100) {
            add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::unsupported_openexr_channels,
                           "OpenEXR environment textures support R, G, B, and optional A channels only", source_path);
            return {};
        }
        rows.push_back(OpenExrChannelRow{
            .name = it.name(),
            .encoding = encoding,
        });
    }

    std::ranges::sort(rows, [](const OpenExrChannelRow& lhs, const OpenExrChannelRow& rhs) {
        const auto lhs_key = channel_sort_key(lhs.name);
        const auto rhs_key = channel_sort_key(rhs.name);
        if (lhs_key != rhs_key) {
            return lhs_key < rhs_key;
        }
        return lhs.name < rhs.name;
    });

    if (!has_channel(rows, "R") || !has_channel(rows, "G") || !has_channel(rows, "B")) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::unsupported_openexr_channels,
                       "OpenEXR environment textures require R, G, and B channels", source_path);
        return {};
    }
    if (rows.empty() || rows.front().encoding == TexturePixelEncodingV2::unknown) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::unsupported_openexr_channels,
                       "OpenEXR channel list is empty", source_path);
        return {};
    }
    const auto encoding = rows.front().encoding;
    if (!std::ranges::all_of(rows, [encoding](const OpenExrChannelRow& row) { return row.encoding == encoding; })) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::unsupported_openexr_channels,
                       "OpenEXR environment textures require a single HALF or FLOAT channel encoding", source_path);
        return {};
    }
    return rows;
}

[[nodiscard]] TextureSourceDocumentV2
make_source_document_from_header(std::vector<OpenExrTextureSourceReviewDiagnostic>& diagnostics,
                                 const OpenExrTextureSourceReviewRequest& request,
                                 const OPENEXR_IMF_NAMESPACE::Header& header, bool multipart, bool deep, bool tiled) {
    const auto data_window = header.dataWindow();
    const auto display_window = header.displayWindow();
    const auto channels = collect_channels(diagnostics, header.channels(), request.source_path);
    if (!diagnostics.empty()) {
        return {};
    }

    const TextureSourceDocumentV2 document{
        .source_path = request.source_path,
        .source_hash = request.source_hash,
        .provenance_id = request.provenance_id,
        .license_id = request.license_id,
        .source_kind = TextureSourceKindV2::openexr,
        .width = window_extent(data_window.min.x, data_window.max.x),
        .height = window_extent(data_window.min.y, data_window.max.y),
        .color_space = TextureColorSpaceV2::scene_linear,
        .sampler_class = request.sampler_class,
        .openexr =
            TextureOpenExrSourceReviewV2{
                .data_window = source_window_from_box(data_window),
                .display_window = source_window_from_box(display_window),
                .channel_list = channel_list_text(channels),
                .pixel_encoding = channels.front().encoding,
                .channel_count = static_cast<std::uint32_t>(channels.size()),
                .chromaticities_recorded = OPENEXR_IMF_NAMESPACE::hasChromaticities(header),
                .scene_linear_intent = request.scene_linear_intent,
                .multipart = multipart,
                .deep = deep,
                .tiled = tiled,
            },
        .ktx2_basis = {},
    };

    if (!is_valid_texture_source_document_v2(document)) {
        add_diagnostic(diagnostics, OpenExrTextureSourceReviewDiagnosticCode::invalid_openexr_metadata,
                       "OpenEXR header metadata does not satisfy GameEngine.TextureSource.v2 environment contract",
                       request.source_path);
    }
    return document;
}
#endif

} // namespace

bool has_openexr_texture_source_review() noexcept {
#if MK_HAS_ASSET_IMPORTERS
    return true;
#else
    return false;
#endif
}

bool has_ktx2_basis_texture_source_review() noexcept {
#if MK_HAS_ASSET_IMPORTERS
    return true;
#else
    return false;
#endif
}

OpenExrTextureSourceReviewResult
review_openexr_texture_source_metadata(const OpenExrTextureSourceReviewRequest& request) {
    OpenExrTextureSourceReviewResult result;

#if !MK_HAS_ASSET_IMPORTERS
    add_diagnostic(result.diagnostics, OpenExrTextureSourceReviewDiagnosticCode::asset_importers_disabled,
                   "asset-importers feature is disabled; OpenEXR source metadata review is unavailable",
                   request.source_path);
    return result;
#else
    validate_request(result.diagnostics, request);
    if (!result.diagnostics.empty()) {
        return result;
    }

    const auto native_path = request.source_file_path.string();
    try {
        const bool multipart = OPENEXR_IMF_NAMESPACE::isMultiPartOpenExrFile(native_path.c_str());
        const bool deep = OPENEXR_IMF_NAMESPACE::isDeepOpenExrFile(native_path.c_str());
        const bool tiled = OPENEXR_IMF_NAMESPACE::isTiledOpenExrFile(native_path.c_str());

        if (multipart || deep) {
            add_diagnostic(result.diagnostics, OpenExrTextureSourceReviewDiagnosticCode::unsupported_openexr_layout,
                           "OpenEXR environment texture review supports single-part non-deep images only",
                           request.source_path);
            return result;
        }

        if (tiled) {
            const OPENEXR_IMF_NAMESPACE::TiledInputFile file{native_path.c_str()};
            auto document =
                make_source_document_from_header(result.diagnostics, request, file.header(), multipart, deep, tiled);
            if (result.diagnostics.empty()) {
                result.source = std::move(document);
            }
            return result;
        }

        const OPENEXR_IMF_NAMESPACE::InputFile file{native_path.c_str()};
        auto document =
            make_source_document_from_header(result.diagnostics, request, file.header(), multipart, deep, tiled);
        if (result.diagnostics.empty()) {
            result.source = std::move(document);
        }
        return result;
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, OpenExrTextureSourceReviewDiagnosticCode::openexr_read_failed,
                       std::string{"failed to read OpenEXR source metadata: "} + error.what(), request.source_path);
        return result;
    }
#endif
}

Ktx2BasisTextureSourceReviewResult
review_ktx2_basis_texture_source_metadata(const Ktx2BasisTextureSourceReviewRequest& request) {
    Ktx2BasisTextureSourceReviewResult result;

#if !MK_HAS_ASSET_IMPORTERS
    add_diagnostic(result.diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::asset_importers_disabled,
                   "asset-importers feature is disabled; KTX2/Basis source metadata review is unavailable",
                   request.source_path);
    return result;
#else
    validate_request(result.diagnostics, request);
    if (!result.diagnostics.empty()) {
        return result;
    }

    ktxTexture2* raw_texture = nullptr;
    const auto native_path = request.source_file_path.string();
    const auto create_result =
        ktxTexture2_CreateFromNamedFile(native_path.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &raw_texture);
    if (create_result != KTX_SUCCESS) {
        add_diagnostic(result.diagnostics, Ktx2BasisTextureSourceReviewDiagnosticCode::ktx_read_failed,
                       "failed to read KTX2/Basis source metadata: " + ktx_error_text(create_result),
                       request.source_path);
        return result;
    }

    UniqueKtxTexture2 texture{raw_texture};
    auto document = make_source_document_from_ktx2(result.diagnostics, request, *texture);
    if (result.diagnostics.empty()) {
        result.source = std::move(document);
    }
    return result;
#endif
}

} // namespace mirakana
