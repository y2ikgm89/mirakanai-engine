// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/registered_source_asset_cook_package_tool.hpp"

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/asset_package.hpp"
#include "mirakana/tools/asset_import_tool.hpp"
#include "mirakana/tools/asset_package_tool.hpp"

#include <algorithm>
#include <exception>
#include <functional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] std::vector<std::string> default_validation_recipes() {
    return {"agent-contract", "public-api-boundary", "default"};
}

[[nodiscard]] std::vector<std::string> default_unsupported_gap_ids() {
    return {"runtime-resource-v2", "renderer-rhi-resource-foundation", "production-ui-importer-platform-adapters",
            "editor-productization", "3d-playable-vertical-slice"};
}

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto value = static_cast<unsigned char>(character);
        return value < 0x20U || value == 0x7FU;
    });
}

[[nodiscard]] bool is_safe_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || path.find(';') != std::string_view::npos ||
        has_control_character(path)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto token = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (token.empty() || token == "." || token == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

[[nodiscard]] bool is_safe_repository_path(std::string_view path) noexcept {
    return is_safe_path(path);
}

[[nodiscard]] bool is_safe_package_path(std::string_view path) noexcept {
    return is_safe_path(path);
}

[[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] bool is_valid_asset_key_segment(std::string_view segment) noexcept {
    if (segment.empty() || segment == "." || segment == "..") {
        return false;
    }
    return std::ranges::all_of(segment, [](char character) {
        return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '.' ||
               character == '_' || character == '-';
    });
}

[[nodiscard]] bool is_valid_asset_key(std::string_view key) noexcept {
    if (key.empty() || key.front() == '/' || key.find('\\') != std::string_view::npos ||
        key.find(':') != std::string_view::npos || has_control_character(key)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= key.size()) {
        const auto end = key.find('/', begin);
        const auto segment = key.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (!is_valid_asset_key_segment(segment)) {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

void add_diagnostic(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics, std::string code,
                    std::string message, std::string path = {}, AssetKeyV2 asset_key = {},
                    std::string unsupported_gap_id = {}, std::string validation_recipe = {}) {
    diagnostics.push_back(RegisteredSourceAssetCookPackageDiagnostic{
        .severity = "error",
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .asset_key = std::move(asset_key),
        .unsupported_gap_id = std::move(unsupported_gap_id),
        .validation_recipe = std::move(validation_recipe),
    });
}

void sort_diagnostics(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics, [](const RegisteredSourceAssetCookPackageDiagnostic& lhs,
                                      const RegisteredSourceAssetCookPackageDiagnostic& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset_key.value != rhs.asset_key.value) {
            return lhs.asset_key.value < rhs.asset_key.value;
        }
        if (lhs.code != rhs.code) {
            return lhs.code < rhs.code;
        }
        return lhs.message < rhs.message;
    });
}

void validate_claim(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics, std::string_view value,
                    std::string code, std::string message, std::string unsupported_gap_id) {
    if (value != "unsupported") {
        add_diagnostic(diagnostics, std::move(code), std::move(message), {}, {}, std::move(unsupported_gap_id));
    }
}

/// Validates the `dependency_cooking` acknowledgement string against `dependency_expansion`.
/// Explicit selection keeps the historical `unsupported` sentinel; registry closure requires the
/// dedicated `registry_closure` acknowledgement so agents do not confuse this with importer-driven
/// broad cooking.
void validate_dependency_cooking_sentinel(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                                          const RegisteredSourceAssetCookPackageRequest& request) {
    using enum RegisteredSourceAssetCookDependencyExpansion;
    switch (request.dependency_expansion) {
    case explicit_dependency_selection:
        if (request.dependency_cooking != "unsupported") {
            if (request.dependency_cooking == "registry_closure") {
                add_diagnostic(diagnostics, "invalid_dependency_cooking_sentinel",
                               "dependency_cooking must remain unsupported when dependency_expansion is "
                               "explicit_dependency_selection",
                               request.source_registry_path);
            } else {
                add_diagnostic(diagnostics, "unsupported_dependency_cooking",
                               "broad dependency cooking is not supported by registered source asset cook/package "
                               "tooling",
                               request.source_registry_path, {}, "runtime-resource-v2");
            }
        }
        break;
    case registered_source_registry_closure:
        if (request.dependency_cooking != "registry_closure") {
            add_diagnostic(diagnostics, "invalid_dependency_cooking_sentinel",
                           "dependency_cooking must be registry_closure when dependency_expansion is "
                           "registered_source_registry_closure",
                           request.source_registry_path);
        }
        break;
    }
}

void validate_unsupported_claims(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                                 const RegisteredSourceAssetCookPackageRequest& request) {
    validate_dependency_cooking_sentinel(diagnostics, request);
    validate_claim(diagnostics, request.external_importer_execution, "unsupported_external_importer_execution",
                   "external importer execution is not supported by registered source asset cook/package tooling",
                   "production-ui-importer-platform-adapters");
    validate_claim(diagnostics, request.renderer_rhi_residency, "unsupported_renderer_rhi_residency",
                   "renderer/RHI residency is not supported by registered source asset cook/package tooling",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.package_streaming, "unsupported_package_streaming",
                   "package streaming is not supported by registered source asset cook/package tooling",
                   "runtime-resource-v2");
    validate_claim(diagnostics, request.material_graph, "unsupported_material_graph",
                   "material graph is not supported by registered source asset cook/package tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.shader_graph, "unsupported_shader_graph",
                   "shader graph is not supported by registered source asset cook/package tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.live_shader_generation, "unsupported_live_shader_generation",
                   "live shader generation is not supported by registered source asset cook/package tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.editor_productization, "unsupported_editor_productization",
                   "editor productization is not supported by registered source asset cook/package tooling",
                   "editor-productization");
    validate_claim(diagnostics, request.metal_readiness, "unsupported_metal_readiness",
                   "Metal readiness is host-gated and is not supported by registered source asset cook/package tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.public_native_rhi_handles, "unsupported_public_native_rhi_handles",
                   "public native/RHI handles are not supported by registered source asset cook/package tooling",
                   "renderer-rhi-resource-foundation");
    validate_claim(
        diagnostics, request.general_production_renderer_quality, "unsupported_general_production_renderer_quality",
        "general production renderer quality is not supported by registered source asset cook/package tooling",
        "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.arbitrary_shell, "unsupported_arbitrary_shell",
                   "arbitrary shell execution is not supported by registered source asset cook/package tooling",
                   "editor-productization");
    validate_claim(diagnostics, request.free_form_edit, "unsupported_free_form_edit",
                   "free-form edits are not supported by registered source asset cook/package tooling",
                   "editor-productization");
}

void validate_request_shape(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                            const RegisteredSourceAssetCookPackageRequest& request, bool validate_inline_source_files) {
    if (!is_safe_repository_path(request.source_registry_path) ||
        !ends_with(request.source_registry_path, ".geassets")) {
        add_diagnostic(diagnostics, "unsafe_source_registry_path",
                       "source registry path must be a safe repository-relative .geassets path",
                       request.source_registry_path);
    }
    if (!is_safe_package_path(request.package_index_path) || !ends_with(request.package_index_path, ".geindex")) {
        add_diagnostic(diagnostics, "unsafe_package_index_path",
                       "package index path must be a package-relative safe .geindex path", request.package_index_path);
    }

    validate_unsupported_claims(diagnostics, request);

    if (request.kind == RegisteredSourceAssetCookPackageCommandKind::free_form_edit) {
        add_diagnostic(diagnostics, "unsupported_free_form_edit",
                       "free-form edits are not supported by registered source asset cook/package tooling",
                       request.source_registry_path, {}, "editor-productization");
        return;
    }

    if (request.kind != RegisteredSourceAssetCookPackageCommandKind::cook_registered_source_assets) {
        add_diagnostic(
            diagnostics, "unsupported_operation",
            "only cook_registered_source_assets is supported by registered source asset cook/package tooling",
            request.source_registry_path);
    }
    if (request.source_revision == 0) {
        add_diagnostic(diagnostics, "invalid_source_revision",
                       "source revision must be non-zero for registered source asset cook/package tooling",
                       request.package_index_path);
    }
    if (request.selected_asset_keys.empty()) {
        add_diagnostic(diagnostics, "missing_selected_asset_keys",
                       "at least one registered source asset key must be selected", request.source_registry_path);
    }

    std::unordered_set<std::string> selected_keys;
    selected_keys.reserve(request.selected_asset_keys.size());
    for (const auto& key : request.selected_asset_keys) {
        if (!is_valid_asset_key(key.value)) {
            add_diagnostic(diagnostics, "invalid_selected_asset_key", "selected asset key must be a valid AssetKeyV2",
                           request.source_registry_path, key);
        }
        if (!selected_keys.insert(key.value).second) {
            add_diagnostic(diagnostics, "duplicate_selected_asset_key", "selected asset key is duplicated",
                           request.source_registry_path, key);
        }
    }

    if (!validate_inline_source_files) {
        return;
    }

    std::unordered_set<std::string> source_paths;
    source_paths.reserve(request.source_files.size());
    for (const auto& file : request.source_files) {
        if (!is_safe_repository_path(file.path)) {
            add_diagnostic(diagnostics, "unsafe_source_path",
                           "source file path must be a safe repository-relative path", file.path);
        }
        if (!source_paths.insert(file.path).second) {
            add_diagnostic(diagnostics, "duplicate_source_file", "source file payload path is duplicated", file.path);
        }
    }
}

[[nodiscard]] SourceAssetRegistryDocumentV1
parse_source_registry(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                      const RegisteredSourceAssetCookPackageRequest& request) {
    if (request.source_registry_content.empty()) {
        return {};
    }
    try {
        return parse_source_asset_registry_document_unvalidated_v1(request.source_registry_content);
    } catch (const std::exception& error) {
        add_diagnostic(diagnostics, "invalid_source_registry",
                       std::string{"failed to parse source asset registry: "} + error.what(),
                       request.source_registry_path);
    }
    return {};
}

[[nodiscard]] std::string registry_diagnostic_code(SourceAssetRegistryDiagnosticCodeV1 code) {
    switch (code) {
    case SourceAssetRegistryDiagnosticCodeV1::invalid_key:
        return "invalid_asset_key";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_key:
        return "duplicate_asset_key";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_asset_id:
        return "duplicate_asset_id";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_kind:
        return "unsupported_source_kind";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_source_path:
        return "unsafe_source_path";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_source_path:
        return "duplicate_source_path";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_source_format:
        return "unsupported_source_format";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_imported_path:
        return "unsafe_imported_path";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_imported_path:
        return "duplicate_imported_path";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_kind:
        return "invalid_dependency_kind";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_target:
        return "invalid_dependency_target";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_key:
        return "invalid_dependency_key";
    case SourceAssetRegistryDiagnosticCodeV1::missing_dependency_key:
        return "missing_dependency_key";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_dependency:
        return "duplicate_dependency";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_identity_projection:
        return "invalid_identity_projection";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_import_metadata:
        return "invalid_import_metadata";
    }
    return "invalid_source_asset_registry";
}

[[nodiscard]] std::string registry_diagnostic_message(SourceAssetRegistryDiagnosticCodeV1 code) {
    switch (code) {
    case SourceAssetRegistryDiagnosticCodeV1::invalid_key:
        return "invalid asset key";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_key:
        return "duplicate asset key";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_asset_id:
        return "duplicate asset id";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_kind:
        return "unsupported source asset kind";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_source_path:
        return "source path must be a safe repository-relative path";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_source_path:
        return "duplicate source path";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_source_format:
        return "unsupported source format";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_imported_path:
        return "imported path must be a safe repository-relative path";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_imported_path:
        return "duplicate imported path";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_kind:
        return "unsupported dependency kind";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_target:
        return "dependency target kind does not match dependency kind";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_key:
        return "invalid dependency key";
    case SourceAssetRegistryDiagnosticCodeV1::missing_dependency_key:
        return "dependency key must already be registered";
    case SourceAssetRegistryDiagnosticCodeV1::duplicate_dependency:
        return "duplicate dependency";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_identity_projection:
        return "asset identity projection is invalid";
    case SourceAssetRegistryDiagnosticCodeV1::invalid_import_metadata:
        return "source import metadata is invalid";
    }
    return "source asset registry is invalid";
}

void append_registry_diagnostics(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                                 const std::vector<SourceAssetRegistryDiagnosticV1>& registry_diagnostics,
                                 const std::string& source_registry_path) {
    for (const auto& diagnostic : registry_diagnostics) {
        add_diagnostic(diagnostics, registry_diagnostic_code(diagnostic.code),
                       registry_diagnostic_message(diagnostic.code),
                       diagnostic.path.empty() ? source_registry_path : diagnostic.path, diagnostic.key);
    }
}

[[nodiscard]] const SourceAssetRegistryRowV1* find_row_by_key(const SourceAssetRegistryDocumentV1& document,
                                                              const AssetKeyV2& key) noexcept {
    const auto it =
        std::ranges::find_if(document.assets, [&key](const auto& row) { return row.key.value == key.value; });
    return it == document.assets.end() ? nullptr : &*it;
}

[[nodiscard]] bool row_less(const SourceAssetRegistryRowV1& lhs, const SourceAssetRegistryRowV1& rhs) noexcept {
    if (lhs.imported_path != rhs.imported_path) {
        return lhs.imported_path < rhs.imported_path;
    }
    return lhs.key.value < rhs.key.value;
}

void append_selected_path_diagnostics(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                                      const RegisteredSourceAssetCookPackageRequest& request,
                                      const SourceAssetRegistryRowV1& row) {
    if (!is_safe_repository_path(row.source_path)) {
        add_diagnostic(diagnostics, "unsafe_source_path", "source path must be a safe repository-relative path",
                       row.source_path, row.key);
    }
    if (!is_safe_package_path(row.imported_path)) {
        add_diagnostic(diagnostics, "unsafe_imported_path",
                       "imported path must be package-relative and must not escape the package", row.imported_path,
                       row.key);
    }
    if (row.imported_path == request.source_registry_path || row.imported_path == request.package_index_path ||
        row.imported_path == row.source_path) {
        add_diagnostic(diagnostics, "aliased_output_path", "cooked output path must not alias an input path",
                       row.imported_path, row.key);
    }
}

/// Expands the selected asset key set by following `SourceAssetRegistryRowV1::dependencies[].key` edges
/// within the same validated registry document. This is intentionally registry-local: it does not
/// infer cross-package dependencies, importer graphs, or scene references.
[[nodiscard]] std::unordered_set<std::string>
compute_registry_dependency_closure_keys(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                                         const SourceAssetRegistryDocumentV1& document,
                                         const RegisteredSourceAssetCookPackageRequest& request) {
    std::unordered_set<std::string> closure;
    std::queue<std::string> pending;
    for (const auto& key : request.selected_asset_keys) {
        pending.push(key.value);
    }

    while (!pending.empty()) {
        const auto k = pending.front();
        pending.pop();
        if (!closure.insert(k).second) {
            continue;
        }

        const auto* row = find_row_by_key(document, AssetKeyV2{k});
        if (row == nullptr) {
            add_diagnostic(diagnostics, "missing_source_asset_key",
                           "selected source asset key is missing from GameEngine.SourceAssetRegistry.v1",
                           request.source_registry_path, AssetKeyV2{k});
            return {};
        }
        append_selected_path_diagnostics(diagnostics, request, *row);
        if (!diagnostics.empty()) {
            return {};
        }
        for (const auto& dependency : row->dependencies) {
            pending.push(dependency.key.value);
        }
    }

    return closure;
}

/// Detects directed cycles within the dependency subgraph induced by `closure_keys`.
[[nodiscard]] bool registry_dependency_subgraph_has_cycle(const SourceAssetRegistryDocumentV1& document,
                                                          const std::unordered_set<std::string>& closure_keys) {
    enum class Color : std::uint8_t { white, gray, black };
    std::unordered_map<std::string, Color> colors;

    const std::function<bool(const std::string&)> dfs = [&](const std::string& key) -> bool {
        auto& mark = colors[key];
        if (mark == Color::gray) {
            return true;
        }
        if (mark == Color::black) {
            return false;
        }
        mark = Color::gray;
        const auto* row = find_row_by_key(document, AssetKeyV2{key});
        if (row != nullptr) {
            for (const auto& dependency : row->dependencies) {
                if (closure_keys.find(dependency.key.value) == closure_keys.end()) {
                    continue;
                }
                if (dfs(dependency.key.value)) {
                    return true;
                }
            }
        }
        mark = Color::black;
        return false;
    };

    for (const auto& key_value : closure_keys) {
        const auto it = colors.find(key_value);
        if (it == colors.end() || it->second == Color::white) {
            if (dfs(key_value)) {
                return true;
            }
        }
    }

    return false;
}

[[nodiscard]] std::vector<SourceAssetRegistryRowV1>
prepare_selected_rows(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                      const RegisteredSourceAssetCookPackageRequest& request) {
    auto document = parse_source_registry(diagnostics, request);
    if (!diagnostics.empty()) {
        return {};
    }

    const auto registry_diagnostics = validate_source_asset_registry_document(document);
    if (!registry_diagnostics.empty()) {
        append_registry_diagnostics(diagnostics, registry_diagnostics, request.source_registry_path);
        return {};
    }

    using enum RegisteredSourceAssetCookDependencyExpansion;
    if (request.dependency_expansion == registered_source_registry_closure) {
        const auto closure_keys = compute_registry_dependency_closure_keys(diagnostics, document, request);
        if (!diagnostics.empty()) {
            return {};
        }
        if (registry_dependency_subgraph_has_cycle(document, closure_keys)) {
            add_diagnostic(diagnostics, "registry_dependency_cycle",
                           "registered source asset dependency graph contains a cycle within the registry closure",
                           request.source_registry_path);
            return {};
        }
        std::vector<std::string> ordered(closure_keys.begin(), closure_keys.end());
        std::ranges::sort(ordered);
        std::vector<SourceAssetRegistryRowV1> rows;
        rows.reserve(ordered.size());
        for (const auto& key_value : ordered) {
            const auto* row = find_row_by_key(document, AssetKeyV2{key_value});
            if (row != nullptr) {
                rows.push_back(*row);
            }
        }
        std::ranges::sort(rows, row_less);
        return rows;
    }

    std::unordered_set<std::string> selected_keys;
    selected_keys.reserve(request.selected_asset_keys.size());
    for (const auto& key : request.selected_asset_keys) {
        selected_keys.insert(key.value);
    }

    std::vector<SourceAssetRegistryRowV1> rows;
    rows.reserve(request.selected_asset_keys.size());
    for (const auto& key : request.selected_asset_keys) {
        const auto* row = find_row_by_key(document, key);
        if (row == nullptr) {
            add_diagnostic(diagnostics, "missing_source_asset_key",
                           "selected source asset key is missing from GameEngine.SourceAssetRegistry.v1",
                           request.source_registry_path, key);
            continue;
        }
        append_selected_path_diagnostics(diagnostics, request, *row);
        for (const auto& dependency : row->dependencies) {
            if (selected_keys.find(dependency.key.value) == selected_keys.end()) {
                add_diagnostic(diagnostics, "unselected_dependency",
                               "registered source asset dependencies must be selected explicitly",
                               request.source_registry_path, row->key);
            }
        }
        rows.push_back(*row);
    }

    std::ranges::sort(rows, row_less);
    return rows;
}

[[nodiscard]] SourceAssetRegistryDocumentV1
selected_document_from_rows(const std::vector<SourceAssetRegistryRowV1>& rows) {
    SourceAssetRegistryDocumentV1 document;
    document.assets = rows;
    return document;
}

using SourceFileMap = std::unordered_map<std::string, std::string>;

[[nodiscard]] SourceFileMap build_source_file_map(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                                                  const RegisteredSourceAssetCookPackageRequest& request,
                                                  const std::vector<SourceAssetRegistryRowV1>& selected_rows) {
    SourceFileMap files;
    files.reserve(request.source_files.size());
    for (const auto& file : request.source_files) {
        files.emplace(file.path, file.content);
    }

    std::unordered_set<std::string> selected_source_paths;
    selected_source_paths.reserve(selected_rows.size());
    for (const auto& row : selected_rows) {
        selected_source_paths.insert(row.source_path);
        if (files.find(row.source_path) == files.end()) {
            add_diagnostic(diagnostics, "missing_source_file",
                           "source payload is missing for selected registered source asset", row.source_path, row.key);
        }
    }
    for (const auto& file : request.source_files) {
        if (selected_source_paths.find(file.path) == selected_source_paths.end()) {
            add_diagnostic(diagnostics, "unexpected_source_file",
                           "source payload path is not selected for registered source asset cooking", file.path);
        }
    }

    return files;
}

[[nodiscard]] AssetCookedPackageIndex
parse_package_index(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                    const RegisteredSourceAssetCookPackageRequest& request) {
    if (request.package_index_content.empty()) {
        return {};
    }

    try {
        auto index = deserialize_asset_cooked_package_index(request.package_index_content);
        for (const auto& entry : index.entries) {
            if (!is_safe_package_path(entry.path)) {
                add_diagnostic(diagnostics, "invalid_package_index",
                               "package index entry path must be package-relative and must not escape the package",
                               entry.path);
            }
        }
        for (const auto& edge : index.dependencies) {
            if (!is_safe_package_path(edge.path)) {
                add_diagnostic(diagnostics, "invalid_package_index",
                               "package index dependency path must be package-relative and must not escape the package",
                               edge.path);
            }
        }
        return index;
    } catch (const std::exception& error) {
        add_diagnostic(diagnostics, "invalid_package_index",
                       std::string{"failed to parse cooked package index: "} + error.what(),
                       request.package_index_path);
    }
    return {};
}

[[nodiscard]] std::string document_kind(AssetImportActionKind kind) {
    switch (kind) {
    case AssetImportActionKind::texture:
        return "GameEngine.CookedTexture.v1";
    case AssetImportActionKind::mesh:
        return "GameEngine.CookedMesh.v2";
    case AssetImportActionKind::morph_mesh_cpu:
        return "GameEngine.CookedMorphMeshCpu.v1";
    case AssetImportActionKind::animation_float_clip:
        return "GameEngine.CookedAnimationFloatClip.v1";
    case AssetImportActionKind::audio:
        return "GameEngine.CookedAudio.v1";
    case AssetImportActionKind::material:
        return "GameEngine.Material.v1";
    case AssetImportActionKind::scene:
        return "GameEngine.Scene.v1";
    case AssetImportActionKind::unknown:
        break;
    }
    return "GameEngine.Unknown.v1";
}

[[nodiscard]] bool same_edge(const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) noexcept {
    return lhs.asset == rhs.asset && lhs.dependency == rhs.dependency && lhs.kind == rhs.kind && lhs.path == rhs.path;
}

void sort_entries(std::vector<AssetCookedPackageEntry>& entries) {
    std::ranges::sort(entries, [](const AssetCookedPackageEntry& lhs, const AssetCookedPackageEntry& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
}

void sort_edges(std::vector<AssetDependencyEdge>& edges) {
    std::ranges::sort(edges, [](const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        if (lhs.dependency.value != rhs.dependency.value) {
            return lhs.dependency.value < rhs.dependency.value;
        }
        return static_cast<int>(lhs.kind) < static_cast<int>(rhs.kind);
    });
}

[[nodiscard]] AssetCookedPackageIndex
merge_package_indexes(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                      AssetCookedPackageIndex base, const AssetCookedPackageIndex& selected) {
    std::unordered_set<AssetId, AssetIdHash> selected_assets;
    selected_assets.reserve(selected.entries.size());
    for (const auto& entry : selected.entries) {
        selected_assets.insert(entry.asset);
    }

    for (const auto& selected_entry : selected.entries) {
        for (const auto& existing : base.entries) {
            if (existing.asset == selected_entry.asset && existing.kind != selected_entry.kind) {
                add_diagnostic(diagnostics, "package_index_conflict",
                               "package index conflict: existing asset kind conflicts with selected cooked artifact",
                               selected_entry.path);
            }
            if (existing.asset != selected_entry.asset && existing.path == selected_entry.path) {
                add_diagnostic(diagnostics, "package_index_conflict",
                               "package index conflict: cooked output path is already owned by another asset",
                               selected_entry.path);
            }
        }
    }
    if (!diagnostics.empty()) {
        return {};
    }

    const auto removed_entries =
        std::ranges::remove_if(base.entries, [&selected_assets](const AssetCookedPackageEntry& entry) {
            return selected_assets.find(entry.asset) != selected_assets.end();
        });
    base.entries.erase(removed_entries.begin(), removed_entries.end());
    base.entries.insert(base.entries.end(), selected.entries.begin(), selected.entries.end());

    const auto removed_edges =
        std::ranges::remove_if(base.dependencies, [&selected_assets](const AssetDependencyEdge& edge) {
            return selected_assets.find(edge.asset) != selected_assets.end();
        });
    base.dependencies.erase(removed_edges.begin(), removed_edges.end());
    base.dependencies.insert(base.dependencies.end(), selected.dependencies.begin(), selected.dependencies.end());
    sort_entries(base.entries);
    sort_edges(base.dependencies);
    base.dependencies.erase(std::ranges::unique(base.dependencies, same_edge).begin(), base.dependencies.end());
    return base;
}

void append_import_failures(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                            const AssetImportExecutionResult& import_result) {
    for (const auto& failure : import_result.failures) {
        add_diagnostic(diagnostics, "asset_import_failed", "asset import failed: " + failure.diagnostic,
                       failure.source_path);
    }
}

void append_assembly_failures(std::vector<RegisteredSourceAssetCookPackageDiagnostic>& diagnostics,
                              const std::vector<AssetCookedPackageAssemblyFailure>& failures) {
    for (const auto& failure : failures) {
        add_diagnostic(diagnostics, "asset_package_assembly_failed",
                       "asset package assembly failed: " + failure.diagnostic, failure.path);
    }
}

void append_changed_files(RegisteredSourceAssetCookPackageResult& result, IFileSystem& scratch,
                          const AssetImportExecutionResult& import_result, const std::string& package_index_path) {
    std::vector<RegisteredSourceAssetCookPackageChangedFile> files;
    files.reserve(import_result.imported.size() + 1U);
    for (const auto& imported : import_result.imported) {
        const auto content = scratch.read_text(imported.output_path);
        files.push_back(RegisteredSourceAssetCookPackageChangedFile{
            .path = imported.output_path,
            .document_kind = document_kind(imported.kind),
            .content = content,
            .content_hash = hash_asset_cooked_content(content),
        });
    }
    std::ranges::sort(files, [](const auto& lhs, const auto& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.document_kind < rhs.document_kind;
    });
    result.changed_files.insert(result.changed_files.end(), files.begin(), files.end());
    result.changed_files.push_back(RegisteredSourceAssetCookPackageChangedFile{
        .path = package_index_path,
        .document_kind = "GameEngine.CookedPackageIndex.v1",
        .content = result.package_index_content,
        .content_hash = hash_asset_cooked_content(result.package_index_content),
    });
}

void append_model_mutations(RegisteredSourceAssetCookPackageResult& result,
                            const RegisteredSourceAssetCookPackageRequest& request,
                            const std::vector<SourceAssetRegistryRowV1>& selected_rows) {
    for (const auto& row : selected_rows) {
        result.model_mutations.push_back(RegisteredSourceAssetCookPackageModelMutation{
            .kind = "cook_registered_source_asset",
            .target_path = row.imported_path,
            .source_registry_path = request.source_registry_path,
            .package_index_path = request.package_index_path,
            .asset_key = row.key,
            .asset = asset_id_from_key_v2(row.key),
            .asset_kind = row.kind,
            .source_path = row.source_path,
            .source_format = row.source_format,
            .imported_path = row.imported_path,
            .dependency_rows = row.dependencies,
        });
    }
    std::ranges::sort(result.model_mutations, [](const auto& lhs, const auto& rhs) {
        if (lhs.target_path != rhs.target_path) {
            return lhs.target_path < rhs.target_path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
}

[[nodiscard]] RegisteredSourceAssetCookPackageResult make_base_result() {
    RegisteredSourceAssetCookPackageResult result;
    result.validation_recipes = default_validation_recipes();
    result.unsupported_gap_ids = default_unsupported_gap_ids();
    return result;
}

} // namespace

RegisteredSourceAssetCookPackageResult
plan_registered_source_asset_cook_package(const RegisteredSourceAssetCookPackageRequest& request) {
    auto result = make_base_result();
    validate_request_shape(result.diagnostics, request, true);
    if (!result.succeeded()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    auto selected_rows = prepare_selected_rows(result.diagnostics, request);
    if (!result.succeeded()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    auto base_index = parse_package_index(result.diagnostics, request);
    const auto source_files = build_source_file_map(result.diagnostics, request, selected_rows);
    if (!result.succeeded()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    MemoryFileSystem scratch;
    for (const auto& row : selected_rows) {
        scratch.write_text(row.source_path, source_files.at(row.source_path));
    }

    AssetImportPlan import_plan;
    try {
        import_plan = build_asset_import_plan(
            build_source_asset_import_metadata_registry(selected_document_from_rows(selected_rows)));
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "invalid_import_plan",
                       std::string{"failed to build selected source asset import plan: "} + error.what(),
                       request.source_registry_path);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    const auto import_result = execute_asset_import_plan(scratch, import_plan);
    if (!import_result.succeeded()) {
        append_import_failures(result.diagnostics, import_result);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    const auto assembly = assemble_asset_cooked_package(scratch, import_plan, import_result,
                                                        AssetCookedPackageAssemblyDesc{request.source_revision});
    if (!assembly.succeeded()) {
        append_assembly_failures(result.diagnostics, assembly.failures);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    auto merged = merge_package_indexes(result.diagnostics, std::move(base_index), assembly.index);
    if (!result.succeeded()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    try {
        result.package_index_content = serialize_asset_cooked_package_index(merged);
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "package_index_conflict",
                       std::string{"package index conflict: failed to serialize merged package index: "} + error.what(),
                       request.package_index_path);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    append_changed_files(result, scratch, import_result, request.package_index_path);
    append_model_mutations(result, request, selected_rows);
    return result;
}

RegisteredSourceAssetCookPackageResult
apply_registered_source_asset_cook_package(IFileSystem& filesystem,
                                           const RegisteredSourceAssetCookPackageRequest& request) {
    auto apply_request = request;
    auto input_result = make_base_result();
    validate_request_shape(input_result.diagnostics, request, false);
    if (!input_result.succeeded()) {
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    try {
        apply_request.source_registry_content = filesystem.exists(request.source_registry_path)
                                                    ? filesystem.read_text(request.source_registry_path)
                                                    : std::string{};
        apply_request.package_index_content = filesystem.exists(request.package_index_path)
                                                  ? filesystem.read_text(request.package_index_path)
                                                  : std::string{};
    } catch (const std::exception& error) {
        add_diagnostic(input_result.diagnostics, "filesystem_read_failed",
                       std::string{"failed to read registered source asset cook/package inputs: "} + error.what());
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    auto selected_rows = prepare_selected_rows(input_result.diagnostics, apply_request);
    if (!input_result.succeeded()) {
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    apply_request.source_files.clear();
    try {
        for (const auto& row : selected_rows) {
            apply_request.source_files.push_back(RegisteredSourceAssetCookPackageSourceFile{
                .path = row.source_path,
                .content = filesystem.read_text(row.source_path),
            });
        }
    } catch (const std::exception& error) {
        add_diagnostic(input_result.diagnostics, "filesystem_read_failed",
                       std::string{"failed to read registered source asset file: "} + error.what());
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    auto result = plan_registered_source_asset_cook_package(apply_request);
    if (!result.succeeded()) {
        return result;
    }

    try {
        for (const auto& file : result.changed_files) {
            filesystem.write_text(file.path, file.content);
        }
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "filesystem_write_failed",
                       std::string{"failed to write registered source asset cook/package outputs: "} + error.what());
        sort_diagnostics(result.diagnostics);
    }

    return result;
}

} // namespace mirakana
