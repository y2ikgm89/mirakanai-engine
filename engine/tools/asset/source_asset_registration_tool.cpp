// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/source_asset_registration_tool.hpp"

#include <algorithm>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] std::vector<std::string> default_validation_recipes() {
    return {"agent-contract", "public-api-boundary", "default"};
}

[[nodiscard]] std::vector<std::string> default_unsupported_gap_ids() {
    return {"asset-identity-v2",
            "runtime-resource-v2",
            "renderer-rhi-resource-foundation",
            "production-ui-importer-platform-adapters",
            "editor-productization",
            "3d-playable-vertical-slice"};
}

[[nodiscard]] std::uint64_t hash_source_asset_registry_content(std::string_view content) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char character : content) {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ULL;
    }
    return hash == 0 ? 1 : hash;
}

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto value = static_cast<unsigned char>(character);
        return value < 0x20U || value == 0x7FU;
    });
}

[[nodiscard]] bool is_safe_repository_path(std::string_view path) noexcept {
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

[[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] AssetImportActionKind import_action_kind_from_asset_kind(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::texture:
        return AssetImportActionKind::texture;
    case AssetKind::mesh:
        return AssetImportActionKind::mesh;
    case AssetKind::morph_mesh_cpu:
        return AssetImportActionKind::morph_mesh_cpu;
    case AssetKind::animation_float_clip:
        return AssetImportActionKind::animation_float_clip;
    case AssetKind::audio:
        return AssetImportActionKind::audio;
    case AssetKind::material:
        return AssetImportActionKind::material;
    case AssetKind::scene:
        return AssetImportActionKind::scene;
    case AssetKind::unknown:
    case AssetKind::script:
    case AssetKind::shader:
    case AssetKind::ui_atlas:
    case AssetKind::tilemap:
    case AssetKind::physics_collision_scene:
        break;
    }
    return AssetImportActionKind::unknown;
}

void add_diagnostic(std::vector<SourceAssetRegistrationDiagnostic>& diagnostics, std::string code, std::string message,
                    std::string path = {}, AssetKeyV2 asset_key = {}, std::string unsupported_gap_id = {},
                    std::string validation_recipe = {}) {
    diagnostics.push_back(SourceAssetRegistrationDiagnostic{
        .severity = "error",
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .asset_key = std::move(asset_key),
        .unsupported_gap_id = std::move(unsupported_gap_id),
        .validation_recipe = std::move(validation_recipe),
    });
}

void sort_diagnostics(std::vector<SourceAssetRegistrationDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics,
                      [](const SourceAssetRegistrationDiagnostic& lhs, const SourceAssetRegistrationDiagnostic& rhs) {
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

void validate_claim(std::vector<SourceAssetRegistrationDiagnostic>& diagnostics, std::string_view value,
                    std::string code, std::string message, std::string unsupported_gap_id) {
    if (value != "unsupported") {
        add_diagnostic(diagnostics, std::move(code), std::move(message), {}, {}, std::move(unsupported_gap_id));
    }
}

void validate_unsupported_claims(std::vector<SourceAssetRegistrationDiagnostic>& diagnostics,
                                 const SourceAssetRegistrationRequest& request) {
    if (request.import_settings != "default-only") {
        add_diagnostic(diagnostics, "unsupported_import_settings",
                       "custom import settings are not supported by source asset registration tooling", {}, {},
                       "production-ui-importer-platform-adapters");
    }
    validate_claim(diagnostics, request.external_importer, "unsupported_external_importer",
                   "external importer execution is not supported by source asset registration tooling",
                   "production-ui-importer-platform-adapters");
    validate_claim(diagnostics, request.package_cooking, "unsupported_package_cooking",
                   "package cooking is not supported by source asset registration tooling", "runtime-resource-v2");
    validate_claim(diagnostics, request.renderer_rhi_residency, "unsupported_renderer_rhi_residency",
                   "renderer/RHI residency is not supported by source asset registration tooling",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.package_streaming, "unsupported_package_streaming",
                   "package streaming is not supported by source asset registration tooling", "runtime-resource-v2");
    validate_claim(diagnostics, request.material_graph, "unsupported_material_graph",
                   "material graph is not supported by source asset registration tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.shader_graph, "unsupported_shader_graph",
                   "shader graph is not supported by source asset registration tooling", "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.live_shader_generation, "unsupported_live_shader_generation",
                   "live shader generation is not supported by source asset registration tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.shader_compilation_execution, "unsupported_shader_compilation_execution",
                   "shader compilation execution is not supported by source asset registration tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.editor_productization, "unsupported_editor_productization",
                   "editor productization is not supported by source asset registration tooling",
                   "editor-productization");
    validate_claim(diagnostics, request.metal_readiness, "unsupported_metal_readiness",
                   "Metal readiness is host-gated and is not supported by source asset registration tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.public_native_rhi_handles, "unsupported_public_native_rhi_handles",
                   "public native/RHI handles are not supported by source asset registration tooling",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.arbitrary_shell, "unsupported_arbitrary_shell",
                   "arbitrary shell execution is not supported by source asset registration tooling",
                   "editor-productization");
    validate_claim(diagnostics, request.free_form_edit, "unsupported_free_form_edit",
                   "free-form edits are not supported by source asset registration tooling", "editor-productization");
}

void validate_request_shape(std::vector<SourceAssetRegistrationDiagnostic>& diagnostics,
                            const SourceAssetRegistrationRequest& request) {
    if (!is_safe_repository_path(request.source_registry_path) ||
        !ends_with(request.source_registry_path, ".geassets")) {
        add_diagnostic(diagnostics, "unsafe_path",
                       "source registry path must be a safe repository-relative .geassets path",
                       request.source_registry_path, request.asset_key);
    }

    validate_unsupported_claims(diagnostics, request);

    if (request.kind == SourceAssetRegistrationCommandKind::free_form_edit) {
        add_diagnostic(diagnostics, "unsupported_free_form_edit",
                       "free-form edits are not supported by source asset registration tooling",
                       request.source_registry_path, request.asset_key, "editor-productization");
        return;
    }

    if (request.kind != SourceAssetRegistrationCommandKind::register_source_asset) {
        add_diagnostic(diagnostics, "unsupported_operation",
                       "only register_source_asset is supported by source asset registration tooling",
                       request.source_registry_path, request.asset_key);
    }

    if (!is_safe_repository_path(request.source_path)) {
        add_diagnostic(diagnostics, "unsafe_source_path", "source path must be a safe repository-relative path",
                       request.source_path, request.asset_key);
    }
    if (!is_safe_repository_path(request.imported_path)) {
        add_diagnostic(diagnostics, "unsafe_imported_path", "imported path must be a safe repository-relative path",
                       request.imported_path, request.asset_key);
    }
    if (!is_supported_source_asset_kind_v1(request.asset_kind) ||
        request.source_format != expected_source_asset_format_v1(request.asset_kind)) {
        add_diagnostic(diagnostics, "unsupported_source_format",
                       "unsupported source format for source asset registration", request.source_path,
                       request.asset_key);
    }
}

void append_changed_file(std::vector<SourceAssetRegistrationChangedFile>& files, std::string path,
                         std::string document_kind, std::string content) {
    SourceAssetRegistrationChangedFile file;
    file.path = std::move(path);
    file.document_kind = std::move(document_kind);
    file.content = std::move(content);
    file.content_hash = hash_source_asset_registry_content(file.content);
    files.push_back(std::move(file));
}

void append_model_mutation(std::vector<SourceAssetRegistrationModelMutation>& mutations,
                           const std::string& source_registry_path, const SourceAssetRegistryRowV1& row) {
    mutations.push_back(SourceAssetRegistrationModelMutation{
        .kind = "register_source_asset",
        .target_path = source_registry_path,
        .asset_key = row.key,
        .asset = asset_id_from_key_v2(row.key),
        .asset_kind = row.kind,
        .source_path = row.source_path,
        .source_format = row.source_format,
        .imported_path = row.imported_path,
        .dependency_rows = row.dependencies,
    });
}

void append_import_metadata(std::vector<SourceAssetRegistrationImportMetadata>& metadata,
                            const SourceAssetRegistryRowV1& row) {
    metadata.push_back(SourceAssetRegistrationImportMetadata{
        .asset = asset_id_from_key_v2(row.key),
        .kind = import_action_kind_from_asset_kind(row.kind),
        .source_path = row.source_path,
        .imported_path = row.imported_path,
        .dependency_rows = row.dependencies,
    });
}

[[nodiscard]] bool dependency_less(const SourceAssetDependencyRowV1& lhs,
                                   const SourceAssetDependencyRowV1& rhs) noexcept {
    const auto lhs_kind = source_asset_dependency_kind_name_v1(lhs.kind);
    const auto rhs_kind = source_asset_dependency_kind_name_v1(rhs.kind);
    if (lhs_kind != rhs_kind) {
        return lhs_kind < rhs_kind;
    }
    return lhs.key.value < rhs.key.value;
}

void canonicalize(SourceAssetRegistryDocumentV1& document) {
    for (auto& row : document.assets) {
        std::ranges::sort(row.dependencies, dependency_less);
    }
    std::ranges::sort(document.assets, [](const SourceAssetRegistryRowV1& lhs, const SourceAssetRegistryRowV1& rhs) {
        return lhs.key.value < rhs.key.value;
    });
}

[[nodiscard]] bool row_equals_request(const SourceAssetRegistryRowV1& row,
                                      const SourceAssetRegistrationRequest& request) {
    auto row_dependencies = row.dependencies;
    auto request_dependencies = request.dependency_rows;
    std::ranges::sort(row_dependencies, dependency_less);
    std::ranges::sort(request_dependencies, dependency_less);
    return row.key.value == request.asset_key.value && row.kind == request.asset_kind &&
           row.source_path == request.source_path && row.source_format == request.source_format &&
           row.imported_path == request.imported_path && row_dependencies == request_dependencies;
}

[[nodiscard]] SourceAssetRegistryDocumentV1
parse_source_registry_content(std::vector<SourceAssetRegistrationDiagnostic>& diagnostics,
                              const SourceAssetRegistrationRequest& request) {
    if (request.source_registry_content.empty()) {
        return {};
    }

    try {
        return deserialize_source_asset_registry_document(request.source_registry_content);
    } catch (const std::exception& error) {
        add_diagnostic(diagnostics, "invalid_source_registry",
                       std::string{"failed to parse source asset registry: "} + error.what(),
                       request.source_registry_path, request.asset_key);
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
        return "invalid_asset_kind";
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
        return "unsupported asset kind";
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

void append_registry_diagnostics(std::vector<SourceAssetRegistrationDiagnostic>& diagnostics,
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

} // namespace

SourceAssetRegistrationResult plan_source_asset_registration(const SourceAssetRegistrationRequest& request) {
    SourceAssetRegistrationResult result;
    result.validation_recipes = default_validation_recipes();
    result.unsupported_gap_ids = default_unsupported_gap_ids();

    validate_request_shape(result.diagnostics, request);
    if (!result.succeeded()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    auto document = parse_source_registry_content(result.diagnostics, request);
    if (!result.succeeded()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }
    canonicalize(document);

    bool registered_new_asset = false;
    if (const auto* existing = find_row_by_key(document, request.asset_key); existing != nullptr) {
        if (!row_equals_request(*existing, request)) {
            add_diagnostic(result.diagnostics, "duplicate_asset_key",
                           "duplicate asset key conflicts with existing source asset registration",
                           request.source_registry_path, request.asset_key);
            sort_diagnostics(result.diagnostics);
            return result;
        }
    } else {
        document.assets.push_back(SourceAssetRegistryRowV1{
            .key = request.asset_key,
            .kind = request.asset_kind,
            .source_path = request.source_path,
            .source_format = request.source_format,
            .imported_path = request.imported_path,
            .dependencies = request.dependency_rows,
        });
        registered_new_asset = true;
    }

    canonicalize(document);
    const auto registry_diagnostics = validate_source_asset_registry_document(document);
    if (!registry_diagnostics.empty()) {
        append_registry_diagnostics(result.diagnostics, registry_diagnostics, request.source_registry_path);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    result.source_registry_content = serialize_source_asset_registry_document(document);
    result.asset_identity_projection = project_source_asset_registry_identity_v2(document);
    if (registered_new_asset) {
        if (const auto* registered = find_row_by_key(document, request.asset_key); registered != nullptr) {
            append_model_mutation(result.model_mutations, request.source_registry_path, *registered);
            append_import_metadata(result.import_metadata, *registered);
        }
    }

    const auto previous_content =
        request.source_registry_content.empty()
            ? std::string{}
            : serialize_source_asset_registry_document(
                  deserialize_source_asset_registry_document(request.source_registry_content));
    if (result.source_registry_content != previous_content) {
        append_changed_file(result.changed_files, request.source_registry_path,
                            std::string{source_asset_registry_format_v1()}, result.source_registry_content);
    }

    return result;
}

SourceAssetRegistrationResult apply_source_asset_registration(IFileSystem& filesystem,
                                                              const SourceAssetRegistrationRequest& request) {
    auto apply_request = request;
    SourceAssetRegistrationResult input_result;
    input_result.validation_recipes = default_validation_recipes();
    input_result.unsupported_gap_ids = default_unsupported_gap_ids();
    validate_request_shape(input_result.diagnostics, request);
    if (!input_result.succeeded()) {
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    try {
        if (filesystem.exists(request.source_registry_path)) {
            apply_request.source_registry_content = filesystem.read_text(request.source_registry_path);
        } else {
            apply_request.source_registry_content.clear();
        }
    } catch (const std::exception& error) {
        add_diagnostic(input_result.diagnostics, "filesystem_read_failed",
                       std::string{"failed to read source asset registry: "} + error.what(),
                       request.source_registry_path, request.asset_key);
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    auto result = plan_source_asset_registration(apply_request);
    if (!result.succeeded()) {
        return result;
    }

    try {
        for (const auto& file : result.changed_files) {
            filesystem.write_text(file.path, file.content);
        }
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "filesystem_write_failed",
                       std::string{"failed to write source asset registry update: "} + error.what(),
                       request.source_registry_path, request.asset_key);
        sort_diagnostics(result.diagnostics);
    }

    return result;
}

} // namespace mirakana
