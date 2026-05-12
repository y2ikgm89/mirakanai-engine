// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/shader_artifact_io.hpp"

#include "mirakana/assets/shader_pipeline.hpp"
#include "mirakana/editor/io.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace mirakana::editor {
namespace {

void validate_manifest_path(std::string_view path) {
    if (path.empty()) {
        throw std::invalid_argument("shader artifact manifest path must not be empty");
    }
    if (path.find_first_of("\r\n=") != std::string_view::npos) {
        throw std::invalid_argument("shader artifact manifest path must not contain newlines or '='");
    }

    const std::filesystem::path relative_path{std::string(path)};
    if (relative_path.is_absolute()) {
        throw std::invalid_argument("shader artifact manifest path must be relative");
    }
    for (const auto& part : relative_path) {
        if (part == "..") {
            throw std::invalid_argument("shader artifact manifest path must not contain '..'");
        }
    }
}

} // namespace

void save_shader_artifact_manifest(ITextStore& store, std::string_view path,
                                   const std::vector<ShaderSourceMetadata>& shaders) {
    validate_manifest_path(path);
    store.write_text(path, serialize_shader_artifact_manifest(shaders));
}

std::vector<ShaderSourceMetadata> load_shader_artifact_manifest(ITextStore& store, std::string_view path) {
    validate_manifest_path(path);
    return deserialize_shader_artifact_manifest(store.read_text(path));
}

} // namespace mirakana::editor
