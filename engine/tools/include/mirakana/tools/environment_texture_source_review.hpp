// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_source_format.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mirakana {

enum class OpenExrTextureSourceReviewDiagnosticCode : std::uint8_t {
    none,
    invalid_request,
    asset_importers_disabled,
    openexr_read_failed,
    unsupported_openexr_layout,
    unsupported_openexr_channels,
    invalid_openexr_metadata,
};

struct OpenExrTextureSourceReviewDiagnostic {
    OpenExrTextureSourceReviewDiagnosticCode code{OpenExrTextureSourceReviewDiagnosticCode::none};
    std::string message;
    std::string path;
};

struct OpenExrTextureSourceReviewRequest {
    std::filesystem::path source_file_path;
    std::string source_path;
    std::string source_hash;
    std::string provenance_id;
    std::string license_id;
    TextureSamplerClassV2 sampler_class{TextureSamplerClassV2::environment_radiance};
    bool scene_linear_intent{true};
};

struct OpenExrTextureSourceReviewResult {
    std::optional<TextureSourceDocumentV2> source;
    std::vector<OpenExrTextureSourceReviewDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return source.has_value() && diagnostics.empty();
    }
};

[[nodiscard]] bool has_openexr_texture_source_review() noexcept;
[[nodiscard]] OpenExrTextureSourceReviewResult
review_openexr_texture_source_metadata(const OpenExrTextureSourceReviewRequest& request);

} // namespace mirakana
