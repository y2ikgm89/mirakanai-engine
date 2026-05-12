// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/shader_metadata.hpp"

#include <string_view>
#include <vector>

namespace mirakana::editor {

class ITextStore;

void save_shader_artifact_manifest(ITextStore& store, std::string_view path,
                                   const std::vector<ShaderSourceMetadata>& shaders);

[[nodiscard]] std::vector<ShaderSourceMetadata> load_shader_artifact_manifest(ITextStore& store, std::string_view path);

} // namespace mirakana::editor
