// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/scene_prefab_authoring_tool.hpp"

#include "mirakana/assets/asset_package.hpp"

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
    return {"scene-component-prefab-schema-v2", "editor-productization", "3d-playable-vertical-slice"};
}

[[nodiscard]] std::string command_kind_name(ScenePrefabAuthoringCommandKind kind) {
    switch (kind) {
    case ScenePrefabAuthoringCommandKind::create_scene:
        return "create_scene";
    case ScenePrefabAuthoringCommandKind::add_node:
        return "add_node";
    case ScenePrefabAuthoringCommandKind::add_or_update_component:
        return "add_or_update_component";
    case ScenePrefabAuthoringCommandKind::create_prefab:
        return "create_prefab";
    case ScenePrefabAuthoringCommandKind::instantiate_prefab:
        return "instantiate_prefab";
    case ScenePrefabAuthoringCommandKind::free_form_edit:
        return "free_form_edit";
    }
    return "unknown";
}

void add_diagnostic(std::vector<ScenePrefabAuthoringDiagnostic>& diagnostics, std::string code, std::string message,
                    std::string path = {}, AuthoringId node = {}, AuthoringId component = {},
                    std::string unsupported_gap_id = {}, std::string validation_recipe = {}) {
    diagnostics.push_back(ScenePrefabAuthoringDiagnostic{
        .severity = "error",
        .code = std::move(code),
        .message = std::move(message),
        .path = std::move(path),
        .node = std::move(node),
        .component = std::move(component),
        .unsupported_gap_id = std::move(unsupported_gap_id),
        .validation_recipe = std::move(validation_recipe),
    });
}

void sort_diagnostics(std::vector<ScenePrefabAuthoringDiagnostic>& diagnostics) {
    std::ranges::sort(diagnostics,
                      [](const ScenePrefabAuthoringDiagnostic& lhs, const ScenePrefabAuthoringDiagnostic& rhs) {
                          if (lhs.path != rhs.path) {
                              return lhs.path < rhs.path;
                          }
                          if (lhs.code != rhs.code) {
                              return lhs.code < rhs.code;
                          }
                          if (lhs.node.value != rhs.node.value) {
                              return lhs.node.value < rhs.node.value;
                          }
                          if (lhs.component.value != rhs.component.value) {
                              return lhs.component.value < rhs.component.value;
                          }
                          return lhs.message < rhs.message;
                      });
}

[[nodiscard]] bool has_control_character(std::string_view value) noexcept {
    return value.find('\0') != std::string_view::npos || value.find('\n') != std::string_view::npos ||
           value.find('\r') != std::string_view::npos;
}

[[nodiscard]] bool is_safe_repository_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || has_control_character(path)) {
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

void validate_authoring_path(std::vector<ScenePrefabAuthoringDiagnostic>& diagnostics, std::string_view path,
                             std::string_view extension, std::string_view label) {
    if (!is_safe_repository_path(path) || !ends_with(path, extension)) {
        add_diagnostic(diagnostics, "unsafe_path",
                       std::string{label} + " must be a safe repository-relative authored " + std::string{extension} +
                           " path",
                       std::string{path});
    }
}

void validate_claim(std::vector<ScenePrefabAuthoringDiagnostic>& diagnostics, std::string_view value, std::string code,
                    std::string message, std::string unsupported_gap_id) {
    if (value != "unsupported") {
        add_diagnostic(diagnostics, std::move(code), std::move(message), {}, {}, {}, std::move(unsupported_gap_id));
    }
}

void validate_unsupported_claims(std::vector<ScenePrefabAuthoringDiagnostic>& diagnostics,
                                 const ScenePrefabAuthoringRequest& request) {
    validate_claim(diagnostics, request.runtime_package_migration, "unsupported_runtime_package_migration",
                   "Scene v2 runtime package migration is not supported by Scene/Prefab v2 authoring tooling",
                   "scene-component-prefab-schema-v2");
    validate_claim(diagnostics, request.source_asset_import, "unsupported_source_asset_import",
                   "source asset import is not supported by Scene/Prefab v2 authoring tooling",
                   "production-ui-importer-platform-adapters");
    validate_claim(diagnostics, request.package_cooking, "unsupported_package_cooking",
                   "package cooking is not supported by Scene/Prefab v2 authoring tooling", "runtime-resource-v2");
    validate_claim(diagnostics, request.editor_productization, "unsupported_editor_productization",
                   "editor productization is not supported by Scene/Prefab v2 authoring tooling",
                   "editor-productization");
    validate_claim(diagnostics, request.nested_prefab_conflict_ux, "unsupported_nested_prefab_conflict_ux",
                   "nested prefab conflict UX is not supported by Scene/Prefab v2 authoring tooling",
                   "editor-productization");
    validate_claim(diagnostics, request.renderer_rhi_residency, "unsupported_renderer_rhi_residency",
                   "renderer/RHI residency is not supported by Scene/Prefab v2 authoring tooling",
                   "renderer-rhi-resource-foundation");
    validate_claim(diagnostics, request.package_streaming, "unsupported_package_streaming",
                   "package streaming is not supported by Scene/Prefab v2 authoring tooling", "runtime-resource-v2");
    validate_claim(diagnostics, request.material_graph, "unsupported_material_graph",
                   "material graph is not supported by Scene/Prefab v2 authoring tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.shader_graph, "unsupported_shader_graph",
                   "shader graph is not supported by Scene/Prefab v2 authoring tooling", "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.live_shader_generation, "unsupported_live_shader_generation",
                   "live shader generation is not supported by Scene/Prefab v2 authoring tooling",
                   "3d-playable-vertical-slice");
    validate_claim(diagnostics, request.arbitrary_shell, "unsupported_arbitrary_shell",
                   "arbitrary shell execution is not supported by Scene/Prefab v2 authoring tooling",
                   "editor-productization");
    validate_claim(diagnostics, request.free_form_edit, "unsupported_free_form_edit",
                   "free-form edits are not supported by Scene/Prefab v2 authoring tooling", "editor-productization");
}

void append_changed_file(std::vector<ScenePrefabAuthoringChangedFile>& files, std::string path,
                         std::string document_kind, std::string content) {
    ScenePrefabAuthoringChangedFile file;
    file.path = std::move(path);
    file.document_kind = std::move(document_kind);
    file.content = std::move(content);
    file.content_hash = hash_asset_cooked_content(file.content);
    files.push_back(std::move(file));
}

void append_model_mutation(std::vector<ScenePrefabAuthoringModelMutation>& mutations, std::string kind,
                           std::string target_path, AuthoringId node = {}, AuthoringId component = {},
                           std::string prefab_path = {}) {
    mutations.push_back(ScenePrefabAuthoringModelMutation{
        .kind = std::move(kind),
        .target_path = std::move(target_path),
        .node = std::move(node),
        .component = std::move(component),
        .prefab_path = std::move(prefab_path),
    });
}

[[nodiscard]] std::string scene_schema_code_name(SceneSchemaV2DiagnosticCode code) {
    switch (code) {
    case SceneSchemaV2DiagnosticCode::invalid_scene_name:
        return "invalid_scene_name";
    case SceneSchemaV2DiagnosticCode::invalid_authoring_id:
        return "invalid_authoring_id";
    case SceneSchemaV2DiagnosticCode::duplicate_node_id:
        return "duplicate_node_id";
    case SceneSchemaV2DiagnosticCode::duplicate_component_id:
        return "duplicate_component_id";
    case SceneSchemaV2DiagnosticCode::missing_parent_node:
        return "missing_parent_node";
    case SceneSchemaV2DiagnosticCode::missing_component_node:
        return "missing_component_node";
    case SceneSchemaV2DiagnosticCode::invalid_component_type:
        return "invalid_component_type";
    case SceneSchemaV2DiagnosticCode::duplicate_component_property:
        return "duplicate_component_property";
    case SceneSchemaV2DiagnosticCode::invalid_component_property:
        return "invalid_component_property";
    case SceneSchemaV2DiagnosticCode::invalid_text_value:
        return "invalid_text_value";
    case SceneSchemaV2DiagnosticCode::invalid_transform:
        return "invalid_transform";
    case SceneSchemaV2DiagnosticCode::missing_override_target:
        return "missing_override_target";
    case SceneSchemaV2DiagnosticCode::duplicate_override_path:
        return "duplicate_override_path";
    }
    return "unknown_scene_schema_v2_diagnostic";
}

[[nodiscard]] std::string scene_schema_message(const SceneSchemaV2Diagnostic& diagnostic) {
    switch (diagnostic.code) {
    case SceneSchemaV2DiagnosticCode::duplicate_node_id:
    case SceneSchemaV2DiagnosticCode::duplicate_component_id:
        return "duplicate authoring id";
    case SceneSchemaV2DiagnosticCode::missing_parent_node:
        return "missing parent authoring id";
    case SceneSchemaV2DiagnosticCode::missing_component_node:
        return "missing component node authoring id";
    case SceneSchemaV2DiagnosticCode::invalid_component_type:
        return "unsupported component type";
    case SceneSchemaV2DiagnosticCode::duplicate_component_property:
        return "duplicate component property";
    case SceneSchemaV2DiagnosticCode::invalid_component_property:
        return "invalid component property";
    case SceneSchemaV2DiagnosticCode::invalid_text_value:
        return "invalid Scene/Prefab v2 line-oriented text value";
    case SceneSchemaV2DiagnosticCode::invalid_transform:
        return "invalid Scene/Prefab v2 transform";
    case SceneSchemaV2DiagnosticCode::invalid_authoring_id:
        return "invalid authoring id";
    case SceneSchemaV2DiagnosticCode::invalid_scene_name:
        return "invalid Scene/Prefab v2 name";
    case SceneSchemaV2DiagnosticCode::missing_override_target:
        return "missing prefab override target";
    case SceneSchemaV2DiagnosticCode::duplicate_override_path:
        return "duplicate prefab override path";
    }
    return scene_schema_code_name(diagnostic.code);
}

void append_scene_schema_diagnostics(std::vector<ScenePrefabAuthoringDiagnostic>& diagnostics,
                                     const std::vector<SceneSchemaV2Diagnostic>& schema_diagnostics,
                                     std::string_view path) {
    for (const auto& diagnostic : schema_diagnostics) {
        add_diagnostic(diagnostics, scene_schema_code_name(diagnostic.code), scene_schema_message(diagnostic),
                       std::string{path}, diagnostic.node, diagnostic.component);
    }
}

[[nodiscard]] const SceneNodeDocumentV2* find_node(const SceneDocumentV2& scene, std::string_view id) noexcept {
    const auto it =
        std::ranges::find_if(scene.nodes, [id](const SceneNodeDocumentV2& node) { return node.id.value == id; });
    return it == scene.nodes.end() ? nullptr : &*it;
}

[[nodiscard]] SceneComponentDocumentV2* find_component(SceneDocumentV2& scene, std::string_view id) noexcept {
    const auto it = std::ranges::find_if(
        scene.components, [id](const SceneComponentDocumentV2& component) { return component.id.value == id; });
    return it == scene.components.end() ? nullptr : &*it;
}

[[nodiscard]] bool component_payload_is_supported(const ScenePrefabAuthoringRequest& request) noexcept {
    if (request.component_payload_format != "properties") {
        return false;
    }
    return std::ranges::all_of(request.component_properties, [](const auto& property) {
        return !has_control_character(property.name) && !has_control_character(property.value);
    });
}

[[nodiscard]] SceneDocumentV2 parse_scene_content(const ScenePrefabAuthoringRequest& request,
                                                  ScenePrefabAuthoringResult& result) {
    try {
        return deserialize_scene_document_v2(request.scene_content);
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "invalid_scene_v2_document",
                       std::string{"Scene v2 document is invalid: "} + error.what(), request.scene_path);
        return {};
    }
}

[[nodiscard]] PrefabDocumentV2 parse_prefab_content(const ScenePrefabAuthoringRequest& request,
                                                    ScenePrefabAuthoringResult& result) {
    try {
        return deserialize_prefab_document_v2(request.prefab_content);
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "stale_prefab_reference",
                       std::string{"stale prefab reference: "} + error.what(), request.prefab_path);
        return {};
    }
}

void finalize_scene_change(ScenePrefabAuthoringResult& result, const SceneDocumentV2& scene, std::string_view path,
                           std::string mutation_kind, AuthoringId node = {}, AuthoringId component = {},
                           std::string prefab_path = {}) {
    append_scene_schema_diagnostics(result.diagnostics, validate_scene_document_v2(scene), path);
    if (!result.succeeded()) {
        return;
    }

    try {
        result.scene_content = serialize_scene_document_v2(scene);
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "invalid_scene_v2_document",
                       std::string{"Scene v2 document could not be serialized: "} + error.what(), std::string{path});
        return;
    }

    append_changed_file(result.changed_files, std::string{path}, "GameEngine.Scene.v2", result.scene_content);
    append_model_mutation(result.model_mutations, std::move(mutation_kind), std::string{path}, std::move(node),
                          std::move(component), std::move(prefab_path));
}

void finalize_prefab_change(ScenePrefabAuthoringResult& result, const PrefabDocumentV2& prefab, std::string_view path,
                            std::string mutation_kind) {
    append_scene_schema_diagnostics(result.diagnostics, validate_prefab_document_v2(prefab), path);
    if (!result.succeeded()) {
        return;
    }

    try {
        result.prefab_content = serialize_prefab_document_v2(prefab);
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "invalid_prefab_v2_document",
                       std::string{"Prefab v2 document could not be serialized: "} + error.what(), std::string{path});
        return;
    }

    append_changed_file(result.changed_files, std::string{path}, "GameEngine.Prefab.v2", result.prefab_content);
    append_model_mutation(result.model_mutations, std::move(mutation_kind), std::string{path}, {}, {},
                          std::string{path});
}

void validate_request_paths(std::vector<ScenePrefabAuthoringDiagnostic>& diagnostics,
                            const ScenePrefabAuthoringRequest& request) {
    switch (request.kind) {
    case ScenePrefabAuthoringCommandKind::create_scene:
    case ScenePrefabAuthoringCommandKind::add_node:
    case ScenePrefabAuthoringCommandKind::add_or_update_component:
        validate_authoring_path(diagnostics, request.scene_path, ".scene", "scene_path");
        break;
    case ScenePrefabAuthoringCommandKind::create_prefab:
        validate_authoring_path(diagnostics, request.prefab_path, ".prefab", "prefab_path");
        break;
    case ScenePrefabAuthoringCommandKind::instantiate_prefab:
        validate_authoring_path(diagnostics, request.scene_path, ".scene", "scene_path");
        validate_authoring_path(diagnostics, request.prefab_path, ".prefab", "prefab_path");
        break;
    case ScenePrefabAuthoringCommandKind::free_form_edit:
        if (!request.scene_path.empty()) {
            validate_authoring_path(diagnostics, request.scene_path, ".scene", "scene_path");
        }
        break;
    }
}

void validate_request_shape(std::vector<ScenePrefabAuthoringDiagnostic>& diagnostics,
                            const ScenePrefabAuthoringRequest& request) {
    validate_request_paths(diagnostics, request);
    validate_unsupported_claims(diagnostics, request);

    if (request.kind == ScenePrefabAuthoringCommandKind::free_form_edit) {
        add_diagnostic(diagnostics, "unsupported_free_form_edit",
                       "free-form edits are not supported by Scene/Prefab v2 authoring tooling", request.scene_path, {},
                       {}, "editor-productization");
    }

    if (request.kind == ScenePrefabAuthoringCommandKind::add_or_update_component &&
        !component_payload_is_supported(request)) {
        add_diagnostic(diagnostics, "unsupported_component_payload",
                       "unsupported component payload; use typed component property rows", request.scene_path,
                       request.component_node_id, request.component_id);
    }

    if (request.kind == ScenePrefabAuthoringCommandKind::instantiate_prefab && request.instance_id_prefix.empty()) {
        add_diagnostic(diagnostics, "invalid_instance_prefix",
                       "prefab instantiation requires a deterministic non-empty instance id prefix",
                       request.scene_path);
    }
}

void plan_create_scene(const ScenePrefabAuthoringRequest& request, ScenePrefabAuthoringResult& result) {
    finalize_scene_change(result, request.scene, request.scene_path, "create_scene");
}

void plan_add_node(const ScenePrefabAuthoringRequest& request, ScenePrefabAuthoringResult& result) {
    auto scene = parse_scene_content(request, result);
    if (!result.succeeded()) {
        return;
    }

    scene.nodes.push_back(SceneNodeDocumentV2{
        .id = request.node_id,
        .name = request.node_name,
        .transform = request.node_transform,
        .parent = request.parent_id,
    });
    finalize_scene_change(result, scene, request.scene_path, "add_node", request.node_id);
}

void plan_add_or_update_component(const ScenePrefabAuthoringRequest& request, ScenePrefabAuthoringResult& result) {
    auto scene = parse_scene_content(request, result);
    if (!result.succeeded()) {
        return;
    }

    SceneComponentDocumentV2 component{
        .id = request.component_id,
        .node = request.component_node_id,
        .type = request.component_type,
        .properties = request.component_properties,
    };

    if (auto* existing = find_component(scene, request.component_id.value); existing != nullptr) {
        *existing = std::move(component);
    } else {
        scene.components.push_back(std::move(component));
    }

    finalize_scene_change(result, scene, request.scene_path, "add_or_update_component", request.component_node_id,
                          request.component_id);
}

void plan_create_prefab(const ScenePrefabAuthoringRequest& request, ScenePrefabAuthoringResult& result) {
    finalize_prefab_change(result, request.prefab, request.prefab_path, "create_prefab");
}

void plan_instantiate_prefab(const ScenePrefabAuthoringRequest& request, ScenePrefabAuthoringResult& result) {
    auto scene = parse_scene_content(request, result);
    if (!result.succeeded()) {
        return;
    }
    auto prefab = parse_prefab_content(request, result);
    if (!result.succeeded()) {
        return;
    }

    const auto prefab_diagnostics = validate_prefab_document_v2(prefab);
    if (!prefab_diagnostics.empty()) {
        add_diagnostic(result.diagnostics, "stale_prefab_reference",
                       "stale prefab reference: Prefab v2 document has invalid internal references",
                       request.prefab_path);
        return;
    }
    if (!request.parent_id.value.empty() && find_node(scene, request.parent_id.value) == nullptr) {
        add_diagnostic(result.diagnostics, "stale_prefab_reference",
                       "stale prefab reference: prefab instance parent does not exist", request.scene_path,
                       request.parent_id);
        return;
    }

    for (auto node : prefab.scene.nodes) {
        const auto was_prefab_root = node.parent.value.empty();
        node.id.value = request.instance_id_prefix + node.id.value;
        node.name = request.instance_name_prefix + node.name;
        if (was_prefab_root) {
            node.parent = request.parent_id;
            node.transform = request.node_transform;
        } else {
            node.parent.value = request.instance_id_prefix + node.parent.value;
        }
        scene.nodes.push_back(std::move(node));
    }

    for (auto component : prefab.scene.components) {
        component.id.value = request.instance_id_prefix + component.id.value;
        component.node.value = request.instance_id_prefix + component.node.value;
        scene.components.push_back(std::move(component));
    }

    finalize_scene_change(result, scene, request.scene_path, "instantiate_prefab", request.parent_id, {},
                          request.prefab_path);
}

void load_apply_inputs(IFileSystem& filesystem, ScenePrefabAuthoringRequest& request,
                       ScenePrefabAuthoringResult& result) {
    try {
        switch (request.kind) {
        case ScenePrefabAuthoringCommandKind::create_scene:
            if (filesystem.exists(request.scene_path)) {
                add_diagnostic(result.diagnostics, "target_exists",
                               "create scene apply refuses to overwrite an existing Scene v2 document",
                               request.scene_path);
            }
            break;
        case ScenePrefabAuthoringCommandKind::add_node:
        case ScenePrefabAuthoringCommandKind::add_or_update_component:
            request.scene_content = filesystem.read_text(request.scene_path);
            break;
        case ScenePrefabAuthoringCommandKind::create_prefab:
            if (filesystem.exists(request.prefab_path)) {
                add_diagnostic(result.diagnostics, "target_exists",
                               "create prefab apply refuses to overwrite an existing Prefab v2 document",
                               request.prefab_path);
            }
            break;
        case ScenePrefabAuthoringCommandKind::instantiate_prefab:
            request.scene_content = filesystem.read_text(request.scene_path);
            request.prefab_content = filesystem.read_text(request.prefab_path);
            break;
        case ScenePrefabAuthoringCommandKind::free_form_edit:
            break;
        }
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "filesystem_read_failed",
                       std::string{"failed to read Scene/Prefab v2 authoring input: "} + error.what());
    }
}

} // namespace

ScenePrefabAuthoringResult plan_scene_prefab_authoring(const ScenePrefabAuthoringRequest& request) {
    ScenePrefabAuthoringResult result;
    result.validation_recipes = default_validation_recipes();
    result.unsupported_gap_ids = default_unsupported_gap_ids();

    validate_request_shape(result.diagnostics, request);
    if (!result.succeeded()) {
        sort_diagnostics(result.diagnostics);
        return result;
    }

    switch (request.kind) {
    case ScenePrefabAuthoringCommandKind::create_scene:
        plan_create_scene(request, result);
        break;
    case ScenePrefabAuthoringCommandKind::add_node:
        plan_add_node(request, result);
        break;
    case ScenePrefabAuthoringCommandKind::add_or_update_component:
        plan_add_or_update_component(request, result);
        break;
    case ScenePrefabAuthoringCommandKind::create_prefab:
        plan_create_prefab(request, result);
        break;
    case ScenePrefabAuthoringCommandKind::instantiate_prefab:
        plan_instantiate_prefab(request, result);
        break;
    case ScenePrefabAuthoringCommandKind::free_form_edit:
        add_diagnostic(result.diagnostics, "unsupported_free_form_edit",
                       "free-form edits are not supported by Scene/Prefab v2 authoring tooling", request.scene_path, {},
                       {}, "editor-productization");
        break;
    }

    sort_diagnostics(result.diagnostics);
    return result;
}

ScenePrefabAuthoringResult apply_scene_prefab_authoring(IFileSystem& filesystem,
                                                        const ScenePrefabAuthoringRequest& request) {
    auto apply_request = request;
    ScenePrefabAuthoringResult input_result;
    input_result.validation_recipes = default_validation_recipes();
    input_result.unsupported_gap_ids = default_unsupported_gap_ids();
    validate_request_shape(input_result.diagnostics, request);
    if (!input_result.succeeded()) {
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    load_apply_inputs(filesystem, apply_request, input_result);
    if (!input_result.succeeded()) {
        sort_diagnostics(input_result.diagnostics);
        return input_result;
    }

    auto result = plan_scene_prefab_authoring(apply_request);
    if (!result.succeeded()) {
        return result;
    }

    try {
        for (const auto& file : result.changed_files) {
            filesystem.write_text(file.path, file.content);
        }
    } catch (const std::exception& error) {
        add_diagnostic(result.diagnostics, "filesystem_write_failed",
                       std::string{"failed to write Scene/Prefab v2 authoring update: "} + error.what());
        sort_diagnostics(result.diagnostics);
    }
    return result;
}

} // namespace mirakana
