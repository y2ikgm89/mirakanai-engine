// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/runtime_scene_package_validation_tool.hpp"

#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <exception>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] std::vector<std::string> default_validation_recipes() {
    return {"agent-contract", "public-api-boundary", "default"};
}

[[nodiscard]] std::vector<std::string> default_unsupported_gap_ids() {
    return {
        "runtime-resource-v2",   "renderer-rhi-resource-foundation", "production-ui-importer-platform-adapters",
        "editor-productization", "3d-playable-vertical-slice",
    };
}

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto byte = static_cast<unsigned char>(character);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] bool has_ascii_space(std::string_view value) noexcept {
    return std::ranges::any_of(value,
                               [](char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; });
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
        key.find(':') != std::string_view::npos || has_ascii_space(key) || has_control_character(key)) {
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

[[nodiscard]] bool is_safe_package_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || path.find(';') != std::string_view::npos ||
        has_control_character(path)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto segment = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

[[nodiscard]] bool is_safe_optional_package_root(std::string_view path) noexcept {
    return path.empty() || is_safe_package_path(path);
}

[[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

void add_diagnostic(std::vector<RuntimeScenePackageValidationDiagnostic>& diagnostics, std::string code,
                    std::string message, std::string path = {}, AssetKeyV2 scene_asset_key = {}, AssetId asset = {},
                    SceneNodeId node = {}, std::string reference_kind = {},
                    AssetKind expected_kind = AssetKind::unknown, AssetKind actual_kind = AssetKind::unknown,
                    std::string unsupported_gap_id = {}, std::string validation_recipe = {}) {
    diagnostics.push_back(RuntimeScenePackageValidationDiagnostic{
        .severity = "error",
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .scene_asset_key = std::move(scene_asset_key),
        .asset = asset,
        .node = node,
        .reference_kind = std::move(reference_kind),
        .expected_kind = expected_kind,
        .actual_kind = actual_kind,
        .unsupported_gap_id = std::move(unsupported_gap_id),
        .validation_recipe = std::move(validation_recipe),
    });
}

void sort_diagnostics(std::vector<RuntimeScenePackageValidationDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics, [](const RuntimeScenePackageValidationDiagnostic& lhs,
                                      const RuntimeScenePackageValidationDiagnostic& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.code != rhs.code) {
            return lhs.code < rhs.code;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        if (lhs.node.value != rhs.node.value) {
            return lhs.node.value < rhs.node.value;
        }
        if (lhs.reference_kind != rhs.reference_kind) {
            return lhs.reference_kind < rhs.reference_kind;
        }
        if (lhs.expected_kind != rhs.expected_kind) {
            return static_cast<int>(lhs.expected_kind) < static_cast<int>(rhs.expected_kind);
        }
        if (lhs.actual_kind != rhs.actual_kind) {
            return static_cast<int>(lhs.actual_kind) < static_cast<int>(rhs.actual_kind);
        }
        if (lhs.scene_asset_key.value != rhs.scene_asset_key.value) {
            return lhs.scene_asset_key.value < rhs.scene_asset_key.value;
        }
        return lhs.message < rhs.message;
    });
}

void validate_claim(std::vector<RuntimeScenePackageValidationDiagnostic>& diagnostics, std::string_view value,
                    std::string code, std::string message, std::string unsupported_gap_id) {
    if (value != "unsupported") {
        add_diagnostic(diagnostics, std::move(code), std::move(message), {}, {}, {}, {}, {}, AssetKind::unknown,
                       AssetKind::unknown, std::move(unsupported_gap_id));
    }
}

void validate_unsupported_claims(std::vector<RuntimeScenePackageValidationDiagnostic>& diagnostics,
                                 const RuntimeScenePackageValidationRequest& request) {
    validate_claim(diagnostics, request.package_cooking, "unsupported_package_cooking",
                   "package cooking is not supported by runtime scene package validation", "runtime-resource-v2");
    validate_claim(diagnostics, request.runtime_source_parsing, "unsupported_runtime_source_parsing",
                   "runtime source parsing is not supported by runtime scene package validation",
                   "runtime-resource-v2");
    validate_claim(diagnostics, request.external_importer_execution, "unsupported_external_importer_execution",
                   "external importer execution is not supported by runtime scene package validation",
                   "production-ui-importer-platform-adapters");
    validate_claim(diagnostics, request.renderer_rhi_residency, "unsupported_renderer_rhi_residency",
                   "renderer/RHI residency is not supported by runtime scene package validation",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.package_streaming, "unsupported_package_streaming",
                   "package streaming is not supported by runtime scene package validation", "runtime-resource-v2");
    validate_claim(diagnostics, request.material_graph, "unsupported_material_graph",
                   "material graph is not supported by runtime scene package validation", "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.shader_graph, "unsupported_shader_graph",
                   "shader graph is not supported by runtime scene package validation", "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.live_shader_generation, "unsupported_live_shader_generation",
                   "live shader generation is not supported by runtime scene package validation",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.editor_productization, "unsupported_editor_productization",
                   "editor productization is not supported by runtime scene package validation",
                   "editor-productization");
    validate_claim(diagnostics, request.metal_readiness, "unsupported_metal_readiness",
                   "Metal readiness is host-gated and is not supported by runtime scene package validation",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.public_native_rhi_handles, "unsupported_public_native_rhi_handles",
                   "public native/RHI handles are not supported by runtime scene package validation",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.general_production_renderer_quality,
                   "unsupported_general_production_renderer_quality",
                   "general production renderer quality is not supported by runtime scene package validation",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.arbitrary_shell, "unsupported_arbitrary_shell",
                   "arbitrary shell execution is not supported by runtime scene package validation",
                   "editor-productization");
    validate_claim(diagnostics, request.free_form_edit, "unsupported_free_form_edit",
                   "free-form edits are not supported by runtime scene package validation", "editor-productization");
}

void validate_request_shape(std::vector<RuntimeScenePackageValidationDiagnostic>& diagnostics,
                            const RuntimeScenePackageValidationRequest& request) {
    if (!is_safe_package_path(request.package_index_path) || !ends_with(request.package_index_path, ".geindex")) {
        add_diagnostic(diagnostics, "unsafe_package_index_path",
                       "package index path must be a safe package-relative .geindex path", request.package_index_path,
                       request.scene_asset_key);
    }
    if (!is_safe_optional_package_root(request.content_root)) {
        add_diagnostic(diagnostics, "unsafe_content_root", "content root must be empty or a safe package-relative path",
                       request.content_root, request.scene_asset_key);
    }

    validate_unsupported_claims(diagnostics, request);

    if (request.kind == RuntimeScenePackageValidationCommandKind::free_form_edit) {
        add_diagnostic(diagnostics, "unsupported_free_form_edit",
                       "free-form edits are not supported by runtime scene package validation",
                       request.package_index_path, request.scene_asset_key, {}, {}, {}, AssetKind::unknown,
                       AssetKind::unknown, "editor-productization");
        return;
    }
    if (request.kind != RuntimeScenePackageValidationCommandKind::validate_runtime_scene_package) {
        add_diagnostic(diagnostics, "unsupported_operation",
                       "only validate_runtime_scene_package is supported by runtime scene package validation",
                       request.package_index_path, request.scene_asset_key);
    }
    if (request.scene_asset_key.value.empty()) {
        add_diagnostic(diagnostics, "invalid_scene_asset_key",
                       "scene asset key must be non-empty for runtime scene package validation",
                       request.package_index_path, request.scene_asset_key);
    } else if (!is_valid_asset_key(request.scene_asset_key.value)) {
        add_diagnostic(diagnostics, "invalid_scene_asset_key",
                       "scene asset key must be a valid AssetKeyV2 for runtime scene package validation",
                       request.package_index_path, request.scene_asset_key);
    }
}

[[nodiscard]] std::string reference_kind_name(runtime_scene::RuntimeSceneReferenceKind kind) {
    switch (kind) {
    case runtime_scene::RuntimeSceneReferenceKind::mesh:
        return "mesh";
    case runtime_scene::RuntimeSceneReferenceKind::material:
        return "material";
    case runtime_scene::RuntimeSceneReferenceKind::sprite:
        return "sprite";
    }
    return "reference";
}

[[nodiscard]] std::string scene_diagnostic_code(runtime_scene::RuntimeSceneDiagnosticCode code) {
    switch (code) {
    case runtime_scene::RuntimeSceneDiagnosticCode::none:
        return "none";
    case runtime_scene::RuntimeSceneDiagnosticCode::missing_scene_asset:
        return "missing_scene_asset";
    case runtime_scene::RuntimeSceneDiagnosticCode::wrong_asset_kind:
        return "wrong_asset_kind";
    case runtime_scene::RuntimeSceneDiagnosticCode::malformed_scene_payload:
        return "malformed_scene_payload";
    case runtime_scene::RuntimeSceneDiagnosticCode::missing_referenced_asset:
        return "missing_referenced_asset";
    case runtime_scene::RuntimeSceneDiagnosticCode::referenced_asset_kind_mismatch:
        return "referenced_asset_kind_mismatch";
    case runtime_scene::RuntimeSceneDiagnosticCode::duplicate_node_name:
        return "duplicate_node_name";
    }
    return "runtime_scene_diagnostic";
}

void append_package_load_failures(std::vector<RuntimeScenePackageValidationDiagnostic>& diagnostics,
                                  const std::vector<runtime::RuntimeAssetPackageLoadFailure>& failures,
                                  const RuntimeScenePackageValidationRequest& request) {
    for (const auto& failure : failures) {
        add_diagnostic(diagnostics, "runtime_package_load_failed", "runtime package load failed: " + failure.diagnostic,
                       failure.path, request.scene_asset_key, failure.asset);
    }
}

void append_scene_diagnostics(std::vector<RuntimeScenePackageValidationDiagnostic>& diagnostics,
                              const runtime_scene::RuntimeSceneLoadResult& scene_result,
                              const RuntimeScenePackageValidationRequest& request) {
    for (const auto& diagnostic : scene_result.diagnostics) {
        add_diagnostic(diagnostics, scene_diagnostic_code(diagnostic.code), diagnostic.message, {},
                       request.scene_asset_key, diagnostic.asset, diagnostic.node,
                       reference_kind_name(diagnostic.reference_kind), diagnostic.expected_kind,
                       diagnostic.actual_kind);
    }
}

void populate_summary_for_runtime_scene(RuntimeScenePackageValidationSummary& summary,
                                        const runtime::RuntimeAssetPackage& package,
                                        const runtime_scene::RuntimeSceneLoadResult& scene_result) {
    summary.package_record_count = static_cast<std::uint64_t>(package.records().size());
    if (!scene_result.instance.has_value()) {
        return;
    }

    const auto& instance = *scene_result.instance;
    summary.scene_name = instance.scene.name();
    summary.scene_node_count = static_cast<std::uint32_t>(instance.scene.nodes().size());
    summary.references.clear();
    summary.references.reserve(instance.references.size());
    for (const auto& reference : instance.references) {
        const auto* record = package.find(reference.asset);
        summary.references.push_back(RuntimeScenePackageValidationReference{
            .node = reference.node,
            .asset = reference.asset,
            .reference_kind = reference_kind_name(reference.kind),
            .expected_kind = reference.expected_kind,
            .actual_kind = record == nullptr ? AssetKind::unknown : record->kind,
        });
    }
}

[[nodiscard]] RuntimeScenePackageValidationResult
make_base_result(const RuntimeScenePackageValidationRequest& request) {
    RuntimeScenePackageValidationResult result;
    result.validation_recipes = default_validation_recipes();
    result.unsupported_gap_ids = default_unsupported_gap_ids();
    result.summary.package_index_path = request.package_index_path;
    result.summary.content_root = request.content_root;
    result.summary.scene_asset_key = request.scene_asset_key;
    if (!request.scene_asset_key.value.empty() && is_valid_asset_key(request.scene_asset_key.value)) {
        result.summary.scene_asset = asset_id_from_key_v2(request.scene_asset_key);
    }
    return result;
}

} // namespace

RuntimeScenePackageValidationResult
plan_runtime_scene_package_validation(const RuntimeScenePackageValidationRequest& request) {
    auto result = make_base_result(request);
    validate_request_shape(result.diagnostics, request);
    sort_diagnostics(result.diagnostics);
    return result;
}

RuntimeScenePackageValidationResult
execute_runtime_scene_package_validation(IFileSystem& filesystem, const RuntimeScenePackageValidationRequest& request) {
    auto result = plan_runtime_scene_package_validation(request);
    if (!result.succeeded()) {
        return result;
    }

    runtime::RuntimeAssetPackageLoadResult package_result;
    try {
        package_result = runtime::load_runtime_asset_package(
            filesystem, runtime::RuntimeAssetPackageDesc{.index_path = request.package_index_path,
                                                         .content_root = request.content_root});
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "runtime_package_load_failed",
                       std::string{"runtime package load failed: "} + error.what(), request.package_index_path,
                       request.scene_asset_key, result.summary.scene_asset);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    if (!package_result.succeeded()) {
        append_package_load_failures(result.diagnostics, package_result.failures, request);
        sort_diagnostics(result.diagnostics);
        return result;
    }

    const auto scene_result = runtime_scene::instantiate_runtime_scene(
        package_result.package, result.summary.scene_asset,
        runtime_scene::RuntimeSceneLoadOptions{.validate_asset_references = request.validate_asset_references,
                                               .require_unique_node_names = request.require_unique_node_names});
    populate_summary_for_runtime_scene(result.summary, package_result.package, scene_result);
    append_scene_diagnostics(result.diagnostics, scene_result, request);
    sort_diagnostics(result.diagnostics);
    return result;
}

} // namespace mirakana
