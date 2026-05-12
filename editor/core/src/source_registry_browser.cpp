// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/source_registry_browser.hpp"

#include "mirakana/assets/source_asset_registry.hpp"

#include <exception>
#include <string>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string source_registry_browser_status_label(EditorSourceRegistryBrowserRefreshStatus status) {
    switch (status) {
    case EditorSourceRegistryBrowserRefreshStatus::missing:
        return "Source asset registry missing";
    case EditorSourceRegistryBrowserRefreshStatus::loaded:
        return "Source asset registry loaded";
    case EditorSourceRegistryBrowserRefreshStatus::blocked:
        return "Source asset registry blocked";
    }
    return "Source asset registry blocked";
}

[[nodiscard]] std::string project_store_source_registry_path(const ProjectDocument& project) {
    auto root = project.root_path;
    while (!root.empty() && (root.back() == '/' || root.back() == '\\')) {
        root.pop_back();
    }
    if (root.empty() || root == ".") {
        return project.source_registry_path;
    }
    return root + "/" + project.source_registry_path;
}

void set_result_status(EditorSourceRegistryBrowserRefreshResult& result,
                       EditorSourceRegistryBrowserRefreshStatus status) {
    result.status = status;
    result.status_label = source_registry_browser_status_label(status);
}

} // namespace

EditorSourceRegistryBrowserRefreshResult
refresh_content_browser_from_project_source_registry(ITextStore& store, const ProjectDocument& project,
                                                     ContentBrowserState& browser) {
    EditorSourceRegistryBrowserRefreshResult result;
    result.source_registry_path = project_store_source_registry_path(project);
    set_result_status(result, EditorSourceRegistryBrowserRefreshStatus::missing);

    if (!store.exists(result.source_registry_path)) {
        result.diagnostics.push_back("source asset registry not found: " + result.source_registry_path);
        return result;
    }

    result.source_registry_exists = true;
    try {
        const auto registry =
            mirakana::deserialize_source_asset_registry_document(store.read_text(result.source_registry_path));
        auto import_metadata = mirakana::build_source_asset_import_metadata_registry(registry);
        auto import_plan = mirakana::build_asset_import_plan(import_metadata);
        browser.refresh_from(registry);
        result.asset_count = registry.assets.size();
        result.import_plan = std::move(import_plan);
        result.loaded = true;
        set_result_status(result, EditorSourceRegistryBrowserRefreshStatus::loaded);
    } catch (const std::exception& error) {
        set_result_status(result, EditorSourceRegistryBrowserRefreshStatus::blocked);
        result.diagnostics.push_back(error.what());
    }

    return result;
}

} // namespace mirakana::editor
