// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/material_asset_preview_panel.hpp"

#include <array>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace mirakana::editor {

[[nodiscard]] static bool material_preview_ascii_iequals(std::string_view left, std::string_view right) noexcept {
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index) {
        const auto left_byte = static_cast<unsigned char>(left[index]);
        const auto right_byte = static_cast<unsigned char>(right[index]);
        const auto left_lower = (left_byte >= 'A' && left_byte <= 'Z') ? left_byte + 32U : left_byte;
        const auto right_lower = (right_byte >= 'A' && right_byte <= 'Z') ? right_byte + 32U : right_byte;
        if (left_lower != right_lower) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] static std::string_view
gpu_preview_visible_refresh_evidence_for_backend(const EditorMaterialGpuPreviewExecutionSnapshot& snapshot,
                                                 std::string_view backend_token) noexcept {
    if (!material_preview_ascii_iequals(snapshot.backend_label, backend_token)) {
        return "not-applicable";
    }
    if (snapshot.status != EditorMaterialGpuPreviewStatus::ready) {
        return "pending";
    }
    if (snapshot.display_path_label != "cpu-readback") {
        return "pending";
    }
    if (snapshot.frames_rendered == 0) {
        return "pending";
    }
    if (!snapshot.diagnostic.empty()) {
        return "pending";
    }
    return "complete";
}

namespace {

[[nodiscard]] std::string_view parity_checklist_row_caption(std::string_view row_id) noexcept {
    if (row_id == "execution_contract") {
        return "GPU execution contract";
    }
    if (row_id == "parity_checklist_contract") {
        return "Display parity checklist contract";
    }
    if (row_id == "backend_scope") {
        return "Active backend reported";
    }
    if (row_id == "display_path_scope") {
        return "Display path classified";
    }
    if (row_id == "vulkan_visible_refresh_gate") {
        return "Vulkan visible refresh gate";
    }
    if (row_id == "metal_visible_refresh_gate") {
        return "Metal visible refresh gate";
    }
    return "Display parity checklist row";
}

void populate_editor_material_gpu_preview_display_parity_checklist(EditorMaterialAssetPreviewPanelModel& model) {
    model.gpu_display_parity_checklist_rows.clear();
    auto push_row = [&](std::string id, std::string status, std::string detail) {
        EditorMaterialGpuPreviewDisplayParityChecklistRow row;
        row.id = std::move(id);
        row.status_label = std::move(status);
        row.detail_label = std::move(detail);
        model.gpu_display_parity_checklist_rows.push_back(std::move(row));
    };
    push_row("execution_contract", "complete", "ge.editor.material_gpu_preview_execution.v1");
    push_row("parity_checklist_contract", "complete", "ge.editor.material_gpu_preview_display_parity_checklist.v1");

    const bool backend_reported = model.gpu_execution_backend_label != "-";
    push_row("backend_scope", backend_reported ? "complete" : "pending", model.gpu_execution_backend_label);

    const auto& display_path = model.gpu_execution_display_path_label;
    const bool display_path_classified = display_path == "cpu-readback" || display_path == "d3d12-shared-texture";
    push_row("display_path_scope", display_path_classified ? "complete" : "pending", display_path);

    push_row("vulkan_visible_refresh_gate", model.gpu_execution_vulkan_visible_refresh_evidence,
             "Vulkan-scope visible refresh (snapshot-only)");
    push_row("metal_visible_refresh_gate", model.gpu_execution_metal_visible_refresh_evidence,
             "Metal-scope visible refresh (snapshot-only)");
}

[[nodiscard]] std::string sanitize_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r' || character == '=') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text.empty() ? "-" : text;
}

[[nodiscard]] std::string asset_id_string(AssetId asset) {
    return std::to_string(asset.value);
}

[[nodiscard]] std::string slot_id(MaterialTextureSlot slot) {
    switch (slot) {
    case MaterialTextureSlot::base_color:
        return "base_color";
    case MaterialTextureSlot::normal:
        return "normal";
    case MaterialTextureSlot::metallic_roughness:
        return "metallic_roughness";
    case MaterialTextureSlot::emissive:
        return "emissive";
    case MaterialTextureSlot::occlusion:
        return "occlusion";
    case MaterialTextureSlot::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] std::string target_id(ShaderCompileTarget target) {
    switch (target) {
    case ShaderCompileTarget::d3d12_dxil:
        return "d3d12";
    case ShaderCompileTarget::vulkan_spirv:
        return "vulkan";
    case ShaderCompileTarget::metal_ir:
        return "metal_ir";
    case ShaderCompileTarget::metal_library:
        return "metal_library";
    case ShaderCompileTarget::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] std::string target_label(ShaderCompileTarget target) {
    switch (target) {
    case ShaderCompileTarget::d3d12_dxil:
        return "D3D12 DXIL";
    case ShaderCompileTarget::vulkan_spirv:
        return "Vulkan SPIR-V";
    case ShaderCompileTarget::metal_ir:
        return "Metal IR";
    case ShaderCompileTarget::metal_library:
        return "Metal Library";
    case ShaderCompileTarget::unknown:
        break;
    }
    return "Unknown";
}

[[nodiscard]] std::string format_float(float value) {
    std::array<char, 32> buffer{};
    const auto count = std::snprintf(buffer.data(), buffer.size(), "%.3f", static_cast<double>(value));
    if (count <= 0) {
        return "0.000";
    }
    return std::string(buffer.data(), static_cast<std::size_t>(count));
}

[[nodiscard]] std::string format_vec3(const std::array<float, 3>& value) {
    return format_float(value[0]) + ", " + format_float(value[1]) + ", " + format_float(value[2]);
}

[[nodiscard]] std::string format_vec4(const std::array<float, 4>& value) {
    return format_float(value[0]) + ", " + format_float(value[1]) + ", " + format_float(value[2]) + ", " +
           format_float(value[3]);
}

[[nodiscard]] bool material_preview_usable(EditorMaterialPreviewStatus status) noexcept {
    return status == EditorMaterialPreviewStatus::ready || status == EditorMaterialPreviewStatus::warning;
}

[[nodiscard]] AssetId factor_shader_id() {
    return AssetId::from_name("editor.material_preview.factor.shader");
}

[[nodiscard]] AssetId textured_shader_id() {
    return AssetId::from_name("editor.material_preview.textured.shader");
}

[[nodiscard]] std::string shader_variant_id(AssetId shader) {
    return shader == textured_shader_id() ? "textured" : "factor";
}

[[nodiscard]] std::string shader_variant_label(AssetId shader) {
    return shader == textured_shader_id() ? "Textured" : "Factor";
}

[[nodiscard]] EditorMaterialAssetPreviewShaderRow make_shader_row(const ViewportShaderArtifactState& shader_artifacts,
                                                                  AssetId shader, ShaderCompileTarget target,
                                                                  std::string_view required_id) {
    const auto* vertex = shader_artifacts.find(shader, ShaderSourceStage::vertex, target);
    const auto* fragment = shader_artifacts.find(shader, ShaderSourceStage::fragment, target);

    const auto variant = shader_variant_id(shader);
    EditorMaterialAssetPreviewShaderRow row;
    row.id = variant + "." + target_id(target);
    row.label = shader_variant_label(shader) + " " + target_label(target);
    row.shader = shader;
    row.target = target;
    row.target_label = target_label(target);
    row.vertex_status_label = std::string(viewport_shader_artifact_status_label(
        vertex == nullptr ? ViewportShaderArtifactStatus::missing : vertex->status));
    row.fragment_status_label = std::string(viewport_shader_artifact_status_label(
        fragment == nullptr ? ViewportShaderArtifactStatus::missing : fragment->status));
    row.vertex_ready = vertex != nullptr && vertex->status == ViewportShaderArtifactStatus::ready;
    row.fragment_ready = fragment != nullptr && fragment->status == ViewportShaderArtifactStatus::ready;
    row.ready = row.vertex_ready && row.fragment_ready;
    row.required = row.id == required_id;
    row.status_label = row.ready ? "ready" : "missing";
    return row;
}

[[nodiscard]] std::vector<EditorMaterialAssetPreviewShaderRow>
make_shader_rows(const ViewportShaderArtifactState& shader_artifacts, std::string_view required_id) {
    return {
        make_shader_row(shader_artifacts, factor_shader_id(), ShaderCompileTarget::d3d12_dxil, required_id),
        make_shader_row(shader_artifacts, factor_shader_id(), ShaderCompileTarget::vulkan_spirv, required_id),
        make_shader_row(shader_artifacts, textured_shader_id(), ShaderCompileTarget::d3d12_dxil, required_id),
        make_shader_row(shader_artifacts, textured_shader_id(), ShaderCompileTarget::vulkan_spirv, required_id),
    };
}

[[nodiscard]] std::vector<EditorMaterialAssetPreviewTexturePayloadRow>
make_texture_payload_rows(const EditorMaterialGpuPreviewPlan& plan) {
    std::vector<EditorMaterialAssetPreviewTexturePayloadRow> rows;
    rows.reserve(plan.textures.size());
    for (const auto& texture : plan.textures) {
        EditorMaterialAssetPreviewTexturePayloadRow row;
        row.id = slot_id(texture.slot);
        row.slot_label = std::string(editor_material_texture_slot_label(texture.slot));
        row.texture = texture.texture;
        row.artifact_path = texture.artifact_path;
        row.width = texture.payload.width;
        row.height = texture.payload.height;
        row.byte_count = texture.payload.bytes.size();
        row.diagnostic = texture.diagnostic;
        row.ready = row.diagnostic.empty() && row.width > 0 && row.height > 0 && row.byte_count > 0;
        rows.push_back(std::move(row));
    }
    return rows;
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("material asset preview ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::ElementDesc make_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  const std::string& label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(sanitize_text(label));
    add_or_throw(document, std::move(desc));
}

void append_section(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root, std::string id,
                    std::string label, mirakana::ui::SemanticRole role = mirakana::ui::SemanticRole::panel) {
    mirakana::ui::ElementDesc section = make_child(std::move(id), root, role);
    section.text = make_text(std::move(label));
    add_or_throw(document, std::move(section));
}

} // namespace

std::string_view editor_material_gpu_preview_vulkan_visible_refresh_evidence(
    const EditorMaterialGpuPreviewExecutionSnapshot& snapshot) noexcept {
    return gpu_preview_visible_refresh_evidence_for_backend(snapshot, "vulkan");
}

std::string_view editor_material_gpu_preview_metal_visible_refresh_evidence(
    const EditorMaterialGpuPreviewExecutionSnapshot& snapshot) noexcept {
    return gpu_preview_visible_refresh_evidence_for_backend(snapshot, "metal");
}

std::string_view
editor_material_asset_preview_panel_status_label(EditorMaterialAssetPreviewPanelStatus status) noexcept {
    switch (status) {
    case EditorMaterialAssetPreviewPanelStatus::empty:
        return "empty";
    case EditorMaterialAssetPreviewPanelStatus::ready:
        return "ready";
    case EditorMaterialAssetPreviewPanelStatus::attention:
        return "attention";
    }
    return "unknown";
}

EditorMaterialAssetPreviewPanelModel
make_editor_material_asset_preview_panel_model(IFileSystem& filesystem, const AssetRegistry& registry, AssetId material,
                                               const ViewportShaderArtifactState& shader_artifacts) {
    EditorMaterialAssetPreviewPanelModel model;
    model.material = material;
    model.material_id = asset_id_string(material);

    const auto* record = registry.find(material);
    model.has_material_record = record != nullptr;
    if (record != nullptr) {
        model.material_path = record->path;
    }

    model.preview = make_editor_selected_material_preview(filesystem, registry, material);
    model.material_name = model.preview.name;
    model.gpu_plan = make_editor_material_gpu_preview_plan(filesystem, registry, material);
    model.gpu_status_label = std::string(editor_material_gpu_preview_status_label(model.gpu_plan.status));
    model.gpu_execution_status_label = "Host Required";
    model.gpu_execution_diagnostic = "material GPU preview execution is host-owned by the optional editor shell";
    model.gpu_execution_backend_label = "-";
    model.gpu_execution_display_path_label = "-";
    const EditorMaterialGpuPreviewExecutionSnapshot default_execution{};
    model.gpu_execution_vulkan_visible_refresh_evidence =
        std::string(editor_material_gpu_preview_vulkan_visible_refresh_evidence(default_execution));
    model.gpu_execution_metal_visible_refresh_evidence =
        std::string(editor_material_gpu_preview_metal_visible_refresh_evidence(default_execution));
    populate_editor_material_gpu_preview_display_parity_checklist(model);
    model.texture_payload_rows = make_texture_payload_rows(model.gpu_plan);
    model.material_preview_ready = material_preview_usable(model.preview.status);
    model.gpu_payload_ready = model.gpu_plan.ready();

    const bool textured = !model.preview.texture_rows.empty();
    model.required_shader_row_id = textured ? "textured.d3d12" : "factor.d3d12";
    model.shader_rows = make_shader_rows(shader_artifacts, model.required_shader_row_id);
    for (const auto& row : model.shader_rows) {
        if (row.required) {
            model.required_shader_ready = row.ready;
            break;
        }
    }

    if (material.value == 0U) {
        model.status = EditorMaterialAssetPreviewPanelStatus::empty;
    } else if (model.material_preview_ready && model.gpu_payload_ready && model.required_shader_ready) {
        model.status = EditorMaterialAssetPreviewPanelStatus::ready;
    } else {
        model.status = EditorMaterialAssetPreviewPanelStatus::attention;
    }
    model.status_label = std::string(editor_material_asset_preview_panel_status_label(model.status));
    return model;
}

void apply_editor_material_gpu_preview_execution_snapshot(EditorMaterialAssetPreviewPanelModel& model,
                                                          const EditorMaterialGpuPreviewExecutionSnapshot& snapshot) {
    model.gpu_execution_status_label = std::string(editor_material_gpu_preview_status_label(snapshot.status));
    model.gpu_execution_diagnostic = snapshot.diagnostic;
    model.gpu_execution_backend_label = snapshot.backend_label.empty() ? "-" : snapshot.backend_label;
    model.gpu_execution_display_path_label = snapshot.display_path_label.empty() ? "-" : snapshot.display_path_label;
    model.gpu_execution_frames_rendered = snapshot.frames_rendered;
    model.gpu_execution_host_owned = true;
    model.gpu_execution_ready = snapshot.status == EditorMaterialGpuPreviewStatus::ready;
    model.gpu_execution_rendered = model.gpu_execution_ready && snapshot.frames_rendered > 0;
    model.gpu_execution_vulkan_visible_refresh_evidence =
        std::string(editor_material_gpu_preview_vulkan_visible_refresh_evidence(snapshot));
    model.gpu_execution_metal_visible_refresh_evidence =
        std::string(editor_material_gpu_preview_metal_visible_refresh_evidence(snapshot));
    populate_editor_material_gpu_preview_display_parity_checklist(model);
}

mirakana::ui::UiDocument make_material_asset_preview_panel_ui_model(const EditorMaterialAssetPreviewPanelModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("material_asset_preview", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"material_asset_preview"};

    append_label(document, root, "material_asset_preview.status", model.status_label);

    append_section(document, root, "material_asset_preview.material", "Material");
    const mirakana::ui::ElementId material_root{"material_asset_preview.material"};
    append_label(document, material_root, "material_asset_preview.material.id", model.material_id);
    append_label(document, material_root, "material_asset_preview.material.path", model.material_path);
    append_label(document, material_root, "material_asset_preview.material.name", model.material_name);
    append_label(document, material_root, "material_asset_preview.material.preview_status",
                 std::string(editor_material_preview_status_label(model.preview.status)));

    append_section(document, root, "material_asset_preview.factors", "Factors");
    const mirakana::ui::ElementId factor_root{"material_asset_preview.factors"};
    append_label(document, factor_root, "material_asset_preview.factors.base_color",
                 format_vec4(model.preview.base_color));
    append_label(document, factor_root, "material_asset_preview.factors.emissive", format_vec3(model.preview.emissive));
    append_label(document, factor_root, "material_asset_preview.factors.metallic",
                 format_float(model.preview.metallic));
    append_label(document, factor_root, "material_asset_preview.factors.roughness",
                 format_float(model.preview.roughness));
    append_label(document, factor_root, "material_asset_preview.factors.uniform_bytes",
                 std::to_string(model.preview.material_uniform_bytes));

    append_section(document, root, "material_asset_preview.textures", "Textures", mirakana::ui::SemanticRole::list);
    const mirakana::ui::ElementId textures_root{"material_asset_preview.textures"};
    for (const auto& row : model.preview.texture_rows) {
        const auto id = slot_id(row.slot);
        const auto prefix = "material_asset_preview.textures." + id;
        mirakana::ui::ElementDesc item = make_child(prefix, textures_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(std::string(editor_material_texture_slot_label(row.slot)));
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status",
                     std::string(editor_material_preview_texture_status_label(row.status)));
        append_label(document, item_root, prefix + ".artifact", row.artifact_path);
    }

    append_section(document, root, "material_asset_preview.gpu", "GPU Payload");
    const mirakana::ui::ElementId gpu_root{"material_asset_preview.gpu"};
    append_label(document, gpu_root, "material_asset_preview.gpu.status", model.gpu_status_label);
    append_label(document, gpu_root, "material_asset_preview.gpu.diagnostic", model.gpu_plan.diagnostic);
    append_section(document, gpu_root, "material_asset_preview.gpu.execution", "GPU Execution");
    const mirakana::ui::ElementId execution_root{"material_asset_preview.gpu.execution"};
    append_label(document, execution_root, "material_asset_preview.gpu.execution.status",
                 model.gpu_execution_status_label);
    append_label(document, execution_root, "material_asset_preview.gpu.execution.contract",
                 "ge.editor.material_gpu_preview_execution.v1");
    append_label(document, execution_root, "material_asset_preview.gpu.execution.diagnostic",
                 model.gpu_execution_diagnostic);
    append_label(document, execution_root, "material_asset_preview.gpu.execution.backend",
                 model.gpu_execution_backend_label);
    append_label(document, execution_root, "material_asset_preview.gpu.execution.display_path",
                 model.gpu_execution_display_path_label);
    append_label(document, execution_root, "material_asset_preview.gpu.execution.frames",
                 std::to_string(model.gpu_execution_frames_rendered));
    append_label(document, execution_root, "material_asset_preview.gpu.execution.rendered",
                 model.gpu_execution_rendered ? "rendered" : "not-rendered");
    append_label(document, execution_root, "material_asset_preview.gpu.execution.vulkan_visible_refresh",
                 model.gpu_execution_vulkan_visible_refresh_evidence);
    append_label(document, execution_root, "material_asset_preview.gpu.execution.metal_visible_refresh",
                 model.gpu_execution_metal_visible_refresh_evidence);

    append_section(document, execution_root, "material_asset_preview.gpu.execution.parity_checklist",
                   "Display parity checklist", mirakana::ui::SemanticRole::list);
    const mirakana::ui::ElementId parity_checklist_root{"material_asset_preview.gpu.execution.parity_checklist"};
    append_label(document, parity_checklist_root, "material_asset_preview.gpu.execution.parity_checklist.contract",
                 "ge.editor.material_gpu_preview_display_parity_checklist.v1");
    for (const auto& row : model.gpu_display_parity_checklist_rows) {
        const auto prefix = "material_asset_preview.gpu.execution.parity_checklist.rows." + row.id;
        mirakana::ui::ElementDesc item =
            make_child(prefix, parity_checklist_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(std::string(parity_checklist_row_caption(row.id)));
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status", row.status_label);
        append_label(document, item_root, prefix + ".detail", row.detail_label);
    }

    append_section(document, root, "material_asset_preview.gpu.textures", "GPU Textures",
                   mirakana::ui::SemanticRole::list);
    const mirakana::ui::ElementId gpu_textures_root{"material_asset_preview.gpu.textures"};
    for (const auto& row : model.texture_payload_rows) {
        const auto prefix = "material_asset_preview.gpu.textures." + row.id;
        mirakana::ui::ElementDesc item = make_child(prefix, gpu_textures_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.slot_label);
        item.enabled = row.ready;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".artifact", row.artifact_path);
        append_label(document, item_root, prefix + ".bytes", std::to_string(row.byte_count));
        append_label(document, item_root, prefix + ".status", row.ready ? "ready" : "attention");
    }

    append_section(document, root, "material_asset_preview.shaders", "Shaders", mirakana::ui::SemanticRole::list);
    const mirakana::ui::ElementId shaders_root{"material_asset_preview.shaders"};
    for (const auto& row : model.shader_rows) {
        const auto prefix = "material_asset_preview.shaders." + row.id;
        mirakana::ui::ElementDesc item = make_child(prefix, shaders_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.label);
        item.enabled = row.ready;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status", row.status_label);
        append_label(document, item_root, prefix + ".vertex", row.vertex_status_label);
        append_label(document, item_root, prefix + ".fragment", row.fragment_status_label);
        append_label(document, item_root, prefix + ".required", row.required ? "required" : "optional");
    }

    return document;
}

} // namespace mirakana::editor
