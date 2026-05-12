// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/editor/project.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

enum class EditorSourceRegistryBrowserRefreshStatus : std::uint8_t { missing, loaded, blocked };

struct EditorSourceRegistryBrowserRefreshResult {
    std::string source_registry_path;
    EditorSourceRegistryBrowserRefreshStatus status{EditorSourceRegistryBrowserRefreshStatus::missing};
    std::string status_label;
    bool source_registry_exists{false};
    bool loaded{false};
    std::size_t asset_count{0};
    mirakana::AssetImportPlan import_plan;
    std::vector<std::string> diagnostics;
};

[[nodiscard]] EditorSourceRegistryBrowserRefreshResult
refresh_content_browser_from_project_source_registry(ITextStore& store, const ProjectDocument& project,
                                                     ContentBrowserState& browser);

} // namespace mirakana::editor
