// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/scene_authoring.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/editor/io.hpp"
#include "mirakana/editor/prefab_variant_authoring.hpp"
#include "mirakana/editor/scene_edit.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/scene/prefab.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

struct SceneNodeIdHash {
    [[nodiscard]] std::size_t operator()(SceneNodeId id) const noexcept {
        return std::hash<std::uint32_t>{}(id.value);
    }
};

void add_scene_prefab_refresh_subtree_nodes(const Scene& scene, SceneNodeId root,
                                            std::unordered_set<SceneNodeId, SceneNodeIdHash>& nodes);

[[nodiscard]] bool valid_text_field(std::string_view value) noexcept {
    return !value.empty() && value.find_first_of("\r\n=") == std::string_view::npos;
}

[[nodiscard]] bool contains_node(const Scene& scene, SceneNodeId node) noexcept {
    return scene.find_node(node) != nullptr;
}

[[nodiscard]] bool contains_descendant(const Scene& scene, SceneNodeId root, SceneNodeId candidate) {
    const auto* root_node = scene.find_node(root);
    if (root_node == nullptr) {
        return false;
    }

    std::unordered_set<SceneNodeId, SceneNodeIdHash> visited;
    std::vector<SceneNodeId> stack = root_node->children;
    while (!stack.empty()) {
        const auto current = stack.back();
        stack.pop_back();
        if (current == candidate) {
            return true;
        }
        if (!visited.insert(current).second) {
            continue;
        }

        const auto* node = scene.find_node(current);
        if (node != nullptr) {
            stack.insert(stack.end(), node->children.begin(), node->children.end());
        }
    }

    return false;
}

[[nodiscard]] std::size_t scene_node_depth_from_root(const Scene& scene, SceneNodeId id) noexcept {
    std::size_t edges = 0;
    SceneNodeId cur = id;
    while (cur != null_scene_node) {
        const auto* n = scene.find_node(cur);
        if (n == nullptr) {
            return 0;
        }
        cur = n->parent;
        ++edges;
    }
    return edges > 0 ? edges - 1 : 0;
}

void append_hierarchy_row(const Scene& scene, SceneNodeId node, std::size_t depth, SceneNodeId selected,
                          std::vector<SceneAuthoringNodeRow>& rows,
                          std::unordered_set<SceneNodeId, SceneNodeIdHash>& visited) {
    std::vector<std::pair<SceneNodeId, std::size_t>> stack;
    stack.emplace_back(node, depth);
    while (!stack.empty()) {
        const auto [current, current_depth] = stack.back();
        stack.pop_back();
        if (!visited.insert(current).second) {
            continue;
        }

        const auto* scene_node = scene.find_node(current);
        if (scene_node == nullptr) {
            continue;
        }

        rows.push_back(SceneAuthoringNodeRow{
            .node = scene_node->id,
            .parent = scene_node->parent,
            .name = scene_node->name,
            .depth = current_depth,
            .selected = scene_node->id == selected,
            .has_camera = scene_node->components.camera.has_value(),
            .has_light = scene_node->components.light.has_value(),
            .has_mesh_renderer = scene_node->components.mesh_renderer.has_value(),
            .has_sprite_renderer = scene_node->components.sprite_renderer.has_value(),
        });

        for (const auto child : std::views::reverse(scene_node->children)) {
            stack.emplace_back(child, current_depth + 1U);
        }
    }
}

[[nodiscard]] UndoableAction empty_action() {
    return UndoableAction{};
}

[[nodiscard]] UndoableAction make_snapshot_action(std::string label, SceneAuthoringDocument& document, Scene before,
                                                  Scene after, SceneNodeId selected_before,
                                                  SceneNodeId selected_after) {
    return UndoableAction{
        .label = std::move(label),
        .redo = [&document, after = std::move(after),
                 selected_after]() mutable { (void)document.replace_scene(after, selected_after); },
        .undo = [&document, before = std::move(before),
                 selected_before]() mutable { (void)document.replace_scene(before, selected_before); },
    };
}

[[nodiscard]] std::unordered_set<SceneNodeId, SceneNodeIdHash> collect_subtree(const Scene& scene, SceneNodeId root) {
    std::unordered_set<SceneNodeId, SceneNodeIdHash> nodes;
    std::vector<SceneNodeId> stack{root};
    while (!stack.empty()) {
        const auto current = stack.back();
        stack.pop_back();
        if (!nodes.insert(current).second) {
            continue;
        }

        const auto* scene_node = scene.find_node(current);
        if (scene_node != nullptr) {
            stack.insert(stack.end(), scene_node->children.begin(), scene_node->children.end());
        }
    }
    return nodes;
}

// NOLINTNEXTLINE(bugprone-exception-escape) -- aggregate holds Scene; implicit special members may propagate throws.
struct RebuiltScene {
    Scene scene;
    std::unordered_map<SceneNodeId, SceneNodeId, SceneNodeIdHash> remap;
    std::unordered_set<SceneNodeId, SceneNodeIdHash> removed;
};

[[nodiscard]] RebuiltScene rebuild_without_subtree(const Scene& source, SceneNodeId removed_root) {
    const auto removed = collect_subtree(source, removed_root);
    RebuiltScene result{.scene = Scene(std::string(source.name())), .remap = {}, .removed = removed};

    for (const auto& source_node : source.nodes()) {
        if (removed.contains(source_node.id)) {
            continue;
        }
        const auto rebuilt_id = result.scene.create_node(source_node.name);
        auto* rebuilt_node = result.scene.find_node(rebuilt_id);
        rebuilt_node->transform = source_node.transform;
        rebuilt_node->prefab_source = source_node.prefab_source;
        result.scene.set_components(rebuilt_id, source_node.components);
        result.remap.emplace(source_node.id, rebuilt_id);
    }

    for (const auto& source_node : source.nodes()) {
        if (removed.contains(source_node.id) || source_node.parent == null_scene_node ||
            removed.contains(source_node.parent)) {
            continue;
        }

        const auto child = result.remap.find(source_node.id);
        const auto parent = result.remap.find(source_node.parent);
        if (child != result.remap.end() && parent != result.remap.end()) {
            result.scene.set_parent(child->second, parent->second);
        }
    }

    return result;
}

void add_reference_diagnostic(std::vector<SceneAuthoringDiagnostic>& diagnostics, SceneNodeId node, AssetId asset,
                              std::string field, AssetKind expected_kind, const AssetRegistry& registry) {
    const auto* record = registry.find(asset);
    if (record == nullptr) {
        diagnostics.push_back(SceneAuthoringDiagnostic{
            .kind = SceneAuthoringDiagnosticKind::missing_asset,
            .node = node,
            .asset = asset,
            .field = std::move(field),
            .diagnostic = "missing asset reference",
        });
        return;
    }

    if (record->kind != expected_kind) {
        std::string_view expected_label = "asset";
        switch (expected_kind) {
        case AssetKind::texture:
            expected_label = "texture";
            break;
        case AssetKind::mesh:
            expected_label = "mesh";
            break;
        case AssetKind::material:
            expected_label = "material";
            break;
        default:
            break;
        }
        diagnostics.push_back(SceneAuthoringDiagnostic{
            .kind = SceneAuthoringDiagnosticKind::wrong_asset_kind,
            .node = node,
            .asset = asset,
            .field = std::move(field),
            .diagnostic = std::string("asset reference has wrong kind; expected ") + std::string(expected_label),
        });
    }
}

[[nodiscard]] bool is_ascii_drive_path(std::string_view path) noexcept {
    if (path.size() < 2U || path[1] != ':') {
        return false;
    }
    const char drive = path[0];
    return (drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z');
}

[[nodiscard]] std::string normalize_path_separators(std::string_view path) {
    std::string normalized(path);
    std::ranges::replace(normalized, '\\', '/');
    return normalized;
}

void trim_trailing_slashes(std::string& path) {
    while (path.size() > 1U && path.back() == '/') {
        path.pop_back();
    }
}

[[nodiscard]] std::string ascii_path_key(std::string_view path) {
    std::string key(path);
    for (auto& ch : key) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return key;
}

[[nodiscard]] bool starts_with_ascii_path_prefix(std::string_view path, std::string_view prefix) {
    const auto path_key = ascii_path_key(path);
    const auto prefix_key = ascii_path_key(prefix);
    return path_key.size() > prefix_key.size() && path_key.substr(0U, prefix_key.size()) == prefix_key &&
           path_key[prefix_key.size()] == '/';
}

struct RuntimePackagePathReview {
    std::string path;
    bool safe{false};
};

[[nodiscard]] RuntimePackagePathReview make_runtime_package_path(std::string_view raw_path,
                                                                 std::string_view project_root_path);

[[nodiscard]] bool is_json_whitespace(char value) noexcept {
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

[[nodiscard]] std::size_t skip_json_whitespace(std::string_view text, std::size_t offset) noexcept {
    while (offset < text.size() && is_json_whitespace(text[offset])) {
        ++offset;
    }
    return offset;
}

[[nodiscard]] bool is_hex_digit(char value) noexcept {
    return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F');
}

[[nodiscard]] bool is_safe_relative_manifest_path(std::string_view raw_path) {
    auto path = normalize_path_separators(raw_path);
    if (path.empty() || path.front() == '/' || is_ascii_drive_path(path) ||
        path.find_first_of("\r\n;=\"") != std::string::npos) {
        return false;
    }

    std::size_t start = 0U;
    while (start <= path.size()) {
        const auto slash = path.find('/', start);
        const auto end = slash == std::string::npos ? path.size() : slash;
        const auto segment = std::string_view(path).substr(start, end - start);
        if (segment.empty() || segment == "..") {
            return false;
        }
        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1U;
    }
    return true;
}

[[nodiscard]] std::string join_manifest_path(std::string_view project_root_path, std::string_view game_manifest_path) {
    auto root = normalize_path_separators(project_root_path);
    auto manifest = normalize_path_separators(game_manifest_path);
    trim_trailing_slashes(root);
    if (root.empty() || root == ".") {
        return manifest;
    }
    return root + "/" + manifest;
}

[[nodiscard]] bool parse_json_string(std::string_view text, std::size_t offset, std::string& output,
                                     std::size_t& next_offset, std::string& diagnostic) {
    if (offset >= text.size() || text[offset] != '"') {
        diagnostic = "expected JSON string";
        return false;
    }

    output.clear();
    for (std::size_t index = offset + 1U; index < text.size(); ++index) {
        const auto value = text[index];
        if (value == '"') {
            next_offset = index + 1U;
            return true;
        }
        if (static_cast<unsigned char>(value) < 0x20U) {
            diagnostic = "JSON string contains an unescaped control character";
            return false;
        }
        if (value != '\\') {
            output.push_back(value);
            continue;
        }
        if (index + 1U >= text.size()) {
            diagnostic = "JSON string escape is incomplete";
            return false;
        }
        const auto escaped = text[++index];
        switch (escaped) {
        case '"':
        case '\\':
        case '/':
            output.push_back(escaped);
            break;
        case 'b':
            output.push_back('\b');
            break;
        case 'f':
            output.push_back('\f');
            break;
        case 'n':
            output.push_back('\n');
            break;
        case 'r':
            output.push_back('\r');
            break;
        case 't':
            output.push_back('\t');
            break;
        case 'u':
            if (index + 4U >= text.size() || !is_hex_digit(text[index + 1U]) || !is_hex_digit(text[index + 2U]) ||
                !is_hex_digit(text[index + 3U]) || !is_hex_digit(text[index + 4U])) {
                diagnostic = "JSON unicode escape is incomplete";
                return false;
            }
            output.push_back('?');
            index += 4U;
            break;
        default:
            diagnostic = "JSON string escape is unsupported";
            return false;
        }
    }

    diagnostic = "JSON string is unterminated";
    return false;
}

struct RuntimePackageFilesProperty {
    bool found{false};
    bool valid{false};
    std::size_t property_name_begin{0U};
    std::size_t array_begin{0U};
    std::size_t array_end{0U};
    std::size_t object_close{0U};
    std::vector<std::string> entries;
    std::string diagnostic;
};

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

[[nodiscard]] std::string sanitize_element_id(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.') {
            text.push_back(character);
        } else {
            text.push_back('_');
        }
    }
    return text.empty() ? "row" : text;
}

[[nodiscard]] bool prefab_refresh_batch_has_duplicate_instance_roots(
    const std::vector<ScenePrefabInstanceRefreshBatchTargetInput>& sorted_targets) {
    for (std::size_t index = 1; index < sorted_targets.size(); ++index) {
        if (sorted_targets[index].instance_root == sorted_targets[index - 1U].instance_root) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool prefab_refresh_batch_has_instance_root_hierarchy_conflict(
    const Scene& scene, const std::vector<ScenePrefabInstanceRefreshBatchTargetInput>& sorted_targets) {
    for (std::size_t i = 0; i < sorted_targets.size(); ++i) {
        for (std::size_t j = 0; j < sorted_targets.size(); ++j) {
            if (i == j) {
                continue;
            }
            if (contains_descendant(scene, sorted_targets[i].instance_root, sorted_targets[j].instance_root)) {
                return true;
            }
        }
    }
    return false;
}

void sort_prefab_refresh_batch_targets_in_place(std::vector<ScenePrefabInstanceRefreshBatchTargetInput>& targets) {
    std::ranges::sort(targets, [](const ScenePrefabInstanceRefreshBatchTargetInput& a,
                                  const ScenePrefabInstanceRefreshBatchTargetInput& b) {
        return a.instance_root.value < b.instance_root.value;
    });
}

void finalize_scene_prefab_refresh_batch_plan(ScenePrefabInstanceRefreshBatchPlan& batch, bool all_prefabs_valid) {
    batch.target_count = batch.targets.size();
    batch.blocking_target_count = 0;
    batch.warning_target_count = 0;
    batch.ready_target_count = 0;
    for (const auto& target_plan : batch.targets) {
        switch (target_plan.status) {
        case ScenePrefabInstanceRefreshStatus::blocked:
            ++batch.blocking_target_count;
            break;
        case ScenePrefabInstanceRefreshStatus::warning:
            ++batch.warning_target_count;
            break;
        case ScenePrefabInstanceRefreshStatus::ready:
            ++batch.ready_target_count;
            break;
        }
    }
    const bool any_blocked_targets = batch.blocking_target_count > 0;
    const bool any_warning_targets = batch.warning_target_count > 0;
    if (!batch.batch_diagnostics.empty() || any_blocked_targets) {
        batch.status = ScenePrefabInstanceRefreshStatus::blocked;
    } else if (any_warning_targets) {
        batch.status = ScenePrefabInstanceRefreshStatus::warning;
    } else {
        batch.status = ScenePrefabInstanceRefreshStatus::ready;
    }
    batch.status_label = std::string(scene_prefab_instance_refresh_status_label(batch.status));
    batch.can_apply = batch.batch_diagnostics.empty() && all_prefabs_valid;
    if (batch.can_apply) {
        for (const auto& target_plan : batch.targets) {
            if (!target_plan.can_apply) {
                batch.can_apply = false;
                break;
            }
        }
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("scene file dialog ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
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
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

void append_button(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                   std::string label, bool enabled) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::button);
    desc.text = make_text(std::move(label));
    desc.enabled = enabled;
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] std::optional<PrefabVariantConflictResolutionKind>
scene_prefab_refresh_row_prefab_variant_alignment_kind(ScenePrefabInstanceRefreshRowKind kind) noexcept {
    switch (kind) {
    case ScenePrefabInstanceRefreshRowKind::preserve_node:
        return PrefabVariantConflictResolutionKind::accept_current_node;
    case ScenePrefabInstanceRefreshRowKind::add_source_node:
        return PrefabVariantConflictResolutionKind::retarget_override;
    case ScenePrefabInstanceRefreshRowKind::remove_stale_node:
        return PrefabVariantConflictResolutionKind::remove_override;
    case ScenePrefabInstanceRefreshRowKind::keep_nested_prefab_instance:
    case ScenePrefabInstanceRefreshRowKind::keep_local_child:
    case ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local:
        return PrefabVariantConflictResolutionKind::accept_current_node;
    case ScenePrefabInstanceRefreshRowKind::unsupported_nested_prefab_instance:
    case ScenePrefabInstanceRefreshRowKind::unsupported_local_child:
    case ScenePrefabInstanceRefreshRowKind::unsupported_stale_source_subtree:
        return PrefabVariantConflictResolutionKind::none;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] bool
scene_prefab_refresh_plan_emits_nested_variant_alignment(const ScenePrefabInstanceRefreshPlan& plan) noexcept {
    return plan.descendant_linked_prefab_instance_root_count > 0 || plan.keep_nested_prefab_instance_count > 0 ||
           plan.unsupported_nested_prefab_instance_count > 0;
}

void append_prefab_instance_refresh_row_variant_alignment_labels(mirakana::ui::UiDocument& document,
                                                                 const mirakana::ui::ElementId& item_root,
                                                                 const std::string& prefix,
                                                                 const ScenePrefabInstanceRefreshRow& row) {
    const auto aligned = scene_prefab_refresh_row_prefab_variant_alignment_kind(row.kind);
    if (!aligned.has_value()) {
        return;
    }
    const auto label = std::string(prefab_variant_conflict_resolution_kind_label(*aligned));
    switch (row.kind) {
    case ScenePrefabInstanceRefreshRowKind::preserve_node:
    case ScenePrefabInstanceRefreshRowKind::add_source_node:
    case ScenePrefabInstanceRefreshRowKind::remove_stale_node:
        append_label(document, item_root, prefix + ".source_node_variant_alignment.resolution_kind", label);
        break;
    case ScenePrefabInstanceRefreshRowKind::keep_nested_prefab_instance:
    case ScenePrefabInstanceRefreshRowKind::unsupported_nested_prefab_instance:
        append_label(document, item_root, prefix + ".nested_variant_alignment.resolution_kind", label);
        break;
    case ScenePrefabInstanceRefreshRowKind::keep_local_child:
    case ScenePrefabInstanceRefreshRowKind::unsupported_local_child:
        append_label(document, item_root, prefix + ".local_child_variant_alignment.resolution_kind", label);
        break;
    case ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local:
    case ScenePrefabInstanceRefreshRowKind::unsupported_stale_source_subtree:
        append_label(document, item_root, prefix + ".stale_source_variant_alignment.resolution_kind", label);
        break;
    default:
        break;
    }
}

void append_prefab_instance_refresh_plan_rows(mirakana::ui::UiDocument& document,
                                              const mirakana::ui::ElementId& rows_list_id,
                                              const std::string& row_id_prefix,
                                              const ScenePrefabInstanceRefreshPlan& plan) {
    for (const auto& row : plan.rows) {
        const auto prefix = row_id_prefix + ".rows." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(prefix, rows_list_id, mirakana::ui::SemanticRole::list_item);
        item.enabled = !row.blocking;
        item.text = make_text(row.source_node_name);
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_root{prefix};
        append_label(document, item_root, prefix + ".status", row.status_label);
        append_label(document, item_root, prefix + ".kind", row.kind_label);
        append_label(document, item_root, prefix + ".current_node",
                     row.current_node == null_scene_node
                         ? "-"
                         : std::to_string(row.current_node.value) + " " + row.current_node_name);
        append_label(document, item_root, prefix + ".source_node", row.source_node_name);
        append_label(document, item_root, prefix + ".refreshed_node",
                     row.refreshed_node_index == 0U ? "-" : std::to_string(row.refreshed_node_index));
        append_label(document, item_root, prefix + ".diagnostic", row.diagnostic.empty() ? "-" : row.diagnostic);
        append_label(document, item_root, prefix + ".blocking", row.blocking ? "true" : "false");
        append_prefab_instance_refresh_row_variant_alignment_labels(document, item_root, prefix, row);
    }
}

[[nodiscard]] bool scene_prefab_refresh_batch_has_nested_variant_alignment_surface(
    const ScenePrefabInstanceRefreshBatchPlan& batch) noexcept {
    return std::ranges::any_of(batch.targets, &scene_prefab_refresh_plan_emits_nested_variant_alignment);
}

[[nodiscard]] bool
scene_prefab_refresh_plan_emits_local_child_variant_alignment(const ScenePrefabInstanceRefreshPlan& plan) noexcept {
    if (plan.keep_local_child_count > 0U) {
        return true;
    }
    return std::ranges::any_of(plan.rows, [](const ScenePrefabInstanceRefreshRow& row) {
        return row.kind == ScenePrefabInstanceRefreshRowKind::keep_local_child ||
               row.kind == ScenePrefabInstanceRefreshRowKind::unsupported_local_child;
    });
}

[[nodiscard]] bool
scene_prefab_refresh_plan_emits_stale_source_variant_alignment(const ScenePrefabInstanceRefreshPlan& plan) noexcept {
    if (plan.keep_stale_source_node_count > 0U) {
        return true;
    }
    return std::ranges::any_of(plan.rows, [](const ScenePrefabInstanceRefreshRow& row) {
        return row.kind == ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local ||
               row.kind == ScenePrefabInstanceRefreshRowKind::unsupported_stale_source_subtree;
    });
}

[[nodiscard]] bool
scene_prefab_refresh_plan_emits_source_node_variant_alignment(const ScenePrefabInstanceRefreshPlan& plan) noexcept {
    if (plan.preserve_count > 0U || plan.add_count > 0U || plan.remove_count > 0U) {
        return true;
    }
    return std::ranges::any_of(plan.rows, [](const ScenePrefabInstanceRefreshRow& row) {
        return row.kind == ScenePrefabInstanceRefreshRowKind::preserve_node ||
               row.kind == ScenePrefabInstanceRefreshRowKind::add_source_node ||
               row.kind == ScenePrefabInstanceRefreshRowKind::remove_stale_node;
    });
}

[[nodiscard]] bool scene_prefab_refresh_batch_has_local_child_variant_alignment_surface(
    const ScenePrefabInstanceRefreshBatchPlan& batch) noexcept {
    return std::ranges::any_of(batch.targets, &scene_prefab_refresh_plan_emits_local_child_variant_alignment);
}

[[nodiscard]] bool scene_prefab_refresh_batch_has_stale_source_variant_alignment_surface(
    const ScenePrefabInstanceRefreshBatchPlan& batch) noexcept {
    return std::ranges::any_of(batch.targets, &scene_prefab_refresh_plan_emits_stale_source_variant_alignment);
}

[[nodiscard]] bool scene_prefab_refresh_batch_has_source_node_variant_alignment_surface(
    const ScenePrefabInstanceRefreshBatchPlan& batch) noexcept {
    return std::ranges::any_of(batch.targets, &scene_prefab_refresh_plan_emits_source_node_variant_alignment);
}

[[nodiscard]] std::string_view scene_file_dialog_mode_id(EditorSceneFileDialogMode mode) noexcept {
    switch (mode) {
    case EditorSceneFileDialogMode::open:
        return "open";
    case EditorSceneFileDialogMode::save:
        return "save";
    }
    return "open";
}

[[nodiscard]] std::string scene_file_dialog_action(EditorSceneFileDialogMode mode) {
    return mode == EditorSceneFileDialogMode::open ? "open" : "save";
}

[[nodiscard]] std::string scene_file_dialog_status(EditorSceneFileDialogMode mode, std::string_view state) {
    return "Scene " + scene_file_dialog_action(mode) + " dialog " + std::string(state);
}

[[nodiscard]] std::string selected_filter_value(int selected_filter) {
    return selected_filter < 0 ? "-" : std::to_string(selected_filter);
}

void append_scene_file_dialog_rows(EditorSceneFileDialogModel& model, std::size_t selected_count, int selected_filter) {
    model.rows = {
        EditorSceneFileDialogRow{.id = "status", .label = "Status", .value = model.status_label},
        EditorSceneFileDialogRow{
            .id = "selected_path", .label = "Selected path", .value = sanitize_text(model.selected_path)},
        EditorSceneFileDialogRow{
            .id = "selected_count", .label = "Selected count", .value = std::to_string(selected_count)},
        EditorSceneFileDialogRow{
            .id = "selected_filter", .label = "Selected filter", .value = selected_filter_value(selected_filter)},
    };
}

[[nodiscard]] mirakana::FileDialogRequest make_scene_dialog_request(mirakana::FileDialogKind kind, std::string title,
                                                                    std::string accept_label,
                                                                    std::string_view default_location) {
    mirakana::FileDialogRequest request;
    request.kind = kind;
    request.title = std::move(title);
    request.filters = {mirakana::FileDialogFilter{.name = "Scene", .pattern = "scene"}};
    request.default_location = std::string(default_location);
    request.allow_many = false;
    request.accept_label = std::move(accept_label);
    request.cancel_label = "Cancel";
    return request;
}

[[nodiscard]] EditorSceneFileDialogModel make_scene_file_dialog_model(const mirakana::FileDialogResult& result,
                                                                      EditorSceneFileDialogMode mode) {
    EditorSceneFileDialogModel model;
    model.mode = mode;
    model.status_label = scene_file_dialog_status(mode, "idle");

    if (auto diagnostic = mirakana::validate_file_dialog_result(result); !diagnostic.empty()) {
        model.status_label = scene_file_dialog_status(mode, "blocked");
        model.diagnostics.push_back(std::move(diagnostic));
        append_scene_file_dialog_rows(model, result.paths.size(), result.selected_filter);
        return model;
    }

    switch (result.status) {
    case mirakana::FileDialogStatus::canceled:
        model.status_label = scene_file_dialog_status(mode, "canceled");
        break;
    case mirakana::FileDialogStatus::failed:
        model.status_label = scene_file_dialog_status(mode, "failed");
        model.diagnostics.push_back(result.error);
        break;
    case mirakana::FileDialogStatus::accepted:
        if (result.paths.size() != 1U) {
            model.status_label = scene_file_dialog_status(mode, "blocked");
            model.diagnostics.push_back("scene " + scene_file_dialog_action(mode) +
                                        " dialog requires exactly one selected path");
            break;
        }

        model.selected_path = result.paths.front();
        if (!model.selected_path.ends_with(".scene")) {
            model.status_label = scene_file_dialog_status(mode, "blocked");
            model.diagnostics.push_back("scene " + scene_file_dialog_action(mode) +
                                        " dialog selection must end with .scene");
            break;
        }

        model.accepted = true;
        model.status_label = scene_file_dialog_status(mode, "accepted");
        break;
    }

    append_scene_file_dialog_rows(model, result.paths.size(), result.selected_filter);
    return model;
}

[[nodiscard]] bool parse_runtime_package_files_array(std::string_view text, std::size_t offset,
                                                     RuntimePackageFilesProperty& property) {
    if (offset >= text.size() || text[offset] != '[') {
        property.diagnostic = "runtimePackageFiles must be an array";
        return false;
    }

    std::unordered_set<std::string> seen;
    auto cursor = skip_json_whitespace(text, offset + 1U);
    if (cursor < text.size() && text[cursor] == ']') {
        property.array_begin = offset;
        property.array_end = cursor + 1U;
        property.valid = true;
        return true;
    }

    for (;;) {
        std::string value;
        std::size_t after_string = 0U;
        if (!parse_json_string(text, cursor, value, after_string, property.diagnostic)) {
            property.diagnostic = "runtimePackageFiles entries must be strings";
            return false;
        }
        auto reviewed = make_runtime_package_path(value, {});
        if (!reviewed.safe) {
            property.diagnostic = "runtimePackageFiles entries must be safe game-relative paths";
            return false;
        }
        const auto key = ascii_path_key(reviewed.path);
        if (!seen.insert(key).second) {
            property.diagnostic = "runtimePackageFiles entries must be unique";
            return false;
        }
        property.entries.push_back(std::move(reviewed.path));

        cursor = skip_json_whitespace(text, after_string);
        if (cursor >= text.size()) {
            property.diagnostic = "runtimePackageFiles array is unterminated";
            return false;
        }
        if (text[cursor] == ',') {
            cursor = skip_json_whitespace(text, cursor + 1U);
            continue;
        }
        if (text[cursor] == ']') {
            property.array_begin = offset;
            property.array_end = cursor + 1U;
            property.valid = true;
            return true;
        }
        property.diagnostic = "runtimePackageFiles must contain only string entries";
        return false;
    }
}

[[nodiscard]] RuntimePackageFilesProperty find_runtime_package_files_property(std::string_view text) {
    RuntimePackageFilesProperty property;
    int depth = 0;
    for (std::size_t cursor = 0U; cursor < text.size();) {
        const auto value = text[cursor];
        if (value == '"') {
            std::string parsed;
            std::size_t after_string = 0U;
            if (!parse_json_string(text, cursor, parsed, after_string, property.diagnostic)) {
                return property;
            }
            const auto after_key = skip_json_whitespace(text, after_string);
            if (depth == 1 && after_key < text.size() && text[after_key] == ':' && parsed == "runtimePackageFiles") {
                property.found = true;
                property.property_name_begin = cursor;
                const auto value_begin = skip_json_whitespace(text, after_key + 1U);
                (void)parse_runtime_package_files_array(text, value_begin, property);
                return property;
            }
            cursor = after_string;
            continue;
        }

        if (value == '{' || value == '[') {
            ++depth;
        } else if (value == '}' || value == ']') {
            if (value == '}' && depth == 1) {
                property.object_close = cursor;
            }
            --depth;
            if (depth < 0) {
                property.diagnostic = "game manifest JSON has unbalanced braces";
                return property;
            }
        }
        ++cursor;
    }

    if (property.object_close == 0U && !text.contains('{')) {
        property.diagnostic = "game manifest must be a JSON object";
    }
    return property;
}

[[nodiscard]] std::string json_escape_string(std::string_view value) {
    std::string escaped;
    for (const auto ch : value) {
        switch (ch) {
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            escaped.push_back(ch);
            break;
        }
    }
    return escaped;
}

[[nodiscard]] std::string line_indent_before(std::string_view text, std::size_t offset) {
    auto line_begin = offset;
    while (line_begin > 0U && text[line_begin - 1U] != '\n' && text[line_begin - 1U] != '\r') {
        --line_begin;
    }
    std::string indent;
    while (line_begin < offset && (text[line_begin] == ' ' || text[line_begin] == '\t')) {
        indent.push_back(text[line_begin]);
        ++line_begin;
    }
    return indent.empty() ? std::string("  ") : indent;
}

[[nodiscard]] std::string format_runtime_package_files_array(const std::vector<std::string>& paths,
                                                             std::string_view key_indent) {
    const auto item_indent = std::string(key_indent) + "  ";
    std::string output = "[\n";
    for (std::size_t index = 0U; index < paths.size(); ++index) {
        output += item_indent;
        output += '"';
        output += json_escape_string(paths[index]);
        output += '"';
        if (index + 1U < paths.size()) {
            output += ',';
        }
        output += '\n';
    }
    output += key_indent;
    output += ']';
    return output;
}

[[nodiscard]] std::string upsert_runtime_package_files_property(std::string_view manifest_text,
                                                                const RuntimePackageFilesProperty& property,
                                                                const std::vector<std::string>& paths) {
    if (property.found) {
        auto updated = std::string(manifest_text);
        const auto indent = line_indent_before(manifest_text, property.property_name_begin);
        updated.replace(property.array_begin, property.array_end - property.array_begin,
                        format_runtime_package_files_array(paths, indent));
        return updated;
    }

    auto insert_begin = property.object_close;
    while (insert_begin > 0U && is_json_whitespace(manifest_text[insert_begin - 1U])) {
        --insert_begin;
    }

    const auto has_existing_properties = insert_begin > 0U && manifest_text[insert_begin - 1U] != '{';
    std::string updated(manifest_text.substr(0U, insert_begin));
    updated += has_existing_properties ? ",\n" : "\n";
    updated += "  \"runtimePackageFiles\": ";
    updated += format_runtime_package_files_array(paths, "  ");
    updated += '\n';
    updated += manifest_text.substr(property.object_close);
    return updated;
}

[[nodiscard]] RuntimePackagePathReview make_runtime_package_path(std::string_view raw_path,
                                                                 std::string_view project_root_path) {
    auto path = normalize_path_separators(raw_path);
    auto project_root = normalize_path_separators(project_root_path);
    trim_trailing_slashes(project_root);

    if (!project_root.empty() && project_root != "." && starts_with_ascii_path_prefix(path, project_root)) {
        path.erase(0U, project_root.size() + 1U);
    }

    const auto path_key = ascii_path_key(path);
    if (path.empty() || path.front() == '/' || is_ascii_drive_path(path) || path_key.starts_with("games/") ||
        path.find_first_of("\r\n;=\"") != std::string::npos) {
        return RuntimePackagePathReview{.path = std::move(path), .safe = false};
    }

    std::vector<std::string> segments;
    std::size_t start = 0U;
    while (start <= path.size()) {
        const auto slash = path.find('/', start);
        const auto end = slash == std::string::npos ? path.size() : slash;
        const auto segment = std::string_view(path).substr(start, end - start);
        if (segment.empty() || segment == "..") {
            return RuntimePackagePathReview{.path = std::move(path), .safe = false};
        }
        if (segment != ".") {
            segments.emplace_back(segment);
        }
        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1U;
    }

    if (segments.empty()) {
        return RuntimePackagePathReview{.path = std::move(path), .safe = false};
    }

    std::string normalized;
    for (std::size_t index = 0U; index < segments.size(); ++index) {
        if (index > 0U) {
            normalized.push_back('/');
        }
        normalized += segments[index];
    }
    return RuntimePackagePathReview{.path = std::move(normalized), .safe = true};
}

[[nodiscard]] std::string scene_prefab_refresh_row_id(std::string_view source_node_name) {
    return "source." + sanitize_element_id(source_node_name);
}

[[nodiscard]] std::string scene_prefab_refresh_nested_row_id(SceneNodeId node) {
    return "node." + std::to_string(node.value) + ".nested_prefab";
}

[[nodiscard]] std::size_t prefab_root_count(const PrefabDefinition& prefab) noexcept {
    return static_cast<std::size_t>(std::count_if(prefab.nodes.begin(), prefab.nodes.end(),
                                                  [](const auto& node) { return node.parent_index == 0U; }));
}

[[nodiscard]] bool same_prefab_source_identity(const ScenePrefabSourceLink& lhs,
                                               const ScenePrefabSourceLink& rhs) noexcept {
    return lhs.prefab_name == rhs.prefab_name && lhs.prefab_path == rhs.prefab_path;
}

[[nodiscard]] bool scene_prefab_refresh_matches_source(const SceneNode& node,
                                                       const ScenePrefabSourceLink& root_link) noexcept {
    return node.prefab_source.has_value() && is_valid_scene_prefab_source_link(*node.prefab_source) &&
           same_prefab_source_identity(*node.prefab_source, root_link);
}

[[nodiscard]] bool
scene_prefab_refresh_has_nonmatching_ancestor(const Scene& scene, SceneNodeId node,
                                              const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
                                              const ScenePrefabSourceLink& root_link) {
    const auto* current = scene.find_node(node);
    while (current != nullptr && current->parent != null_scene_node && subtree.contains(current->parent)) {
        const auto* parent = scene.find_node(current->parent);
        if (parent == nullptr) {
            return true;
        }
        if (!scene_prefab_refresh_matches_source(*parent, root_link)) {
            return true;
        }
        current = parent;
    }
    return false;
}

[[nodiscard]] bool scene_prefab_refresh_local_parent_anchor_available(
    const Scene& scene, const SceneNode& local_root, const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
    const ScenePrefabSourceLink& root_link, const std::unordered_set<std::string>& refreshed_names) {
    if (local_root.parent == null_scene_node || !subtree.contains(local_root.parent)) {
        return false;
    }

    const auto* parent = scene.find_node(local_root.parent);
    return parent != nullptr && parent->prefab_source.has_value() &&
           scene_prefab_refresh_matches_source(*parent, root_link) &&
           refreshed_names.contains(parent->prefab_source->source_node_name);
}

[[nodiscard]] bool scene_prefab_refresh_stale_parent_anchor_available(
    const Scene& scene, const SceneNode& stale_root, const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
    const ScenePrefabSourceLink& root_link, const std::unordered_set<std::string>& refreshed_names,
    const std::unordered_set<std::string>& ambiguous_refreshed_names) {
    if (stale_root.parent == null_scene_node || !subtree.contains(stale_root.parent)) {
        return false;
    }

    const auto* parent = scene.find_node(stale_root.parent);
    if (parent == nullptr || !parent->prefab_source.has_value() ||
        !scene_prefab_refresh_matches_source(*parent, root_link)) {
        return false;
    }

    const auto& parent_source_name = parent->prefab_source->source_node_name;
    return refreshed_names.contains(parent_source_name) && !ambiguous_refreshed_names.contains(parent_source_name);
}

[[nodiscard]] bool scene_prefab_refresh_has_stale_source_ancestor(
    const Scene& scene, SceneNodeId node, const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
    const ScenePrefabSourceLink& root_link, const std::unordered_set<std::string>& refreshed_names,
    const std::unordered_set<std::string>& ambiguous_instance_names) {
    const auto* current = scene.find_node(node);
    while (current != nullptr && current->parent != null_scene_node && subtree.contains(current->parent)) {
        const auto* parent = scene.find_node(current->parent);
        if (parent == nullptr) {
            return false;
        }
        if (scene_prefab_refresh_matches_source(*parent, root_link) && parent->prefab_source.has_value()) {
            const auto& parent_source_name = parent->prefab_source->source_node_name;
            if (!refreshed_names.contains(parent_source_name) &&
                !ambiguous_instance_names.contains(parent_source_name)) {
                return true;
            }
        }
        current = parent;
    }
    return false;
}

[[nodiscard]] bool scene_prefab_refresh_stale_subtree_has_refreshed_source_descendant(
    const Scene& scene, const SceneNode& stale_root, const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
    const ScenePrefabSourceLink& root_link, const std::unordered_set<std::string>& refreshed_names) {
    return std::ranges::any_of(scene.nodes(), [&](const auto& node) {
        if (node.id == stale_root.id || !subtree.contains(node.id) ||
            !contains_descendant(scene, stale_root.id, node.id)) {
            return false;
        }
        return scene_prefab_refresh_matches_source(node, root_link) && node.prefab_source.has_value() &&
               refreshed_names.contains(node.prefab_source->source_node_name);
    });
}

[[nodiscard]] bool scene_prefab_refresh_stale_subtree_has_nested_source_link(
    const Scene& scene, const SceneNode& stale_root, const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
    const ScenePrefabSourceLink& root_link) {
    return std::ranges::any_of(scene.nodes(), [&](const auto& node) {
        if (node.id == stale_root.id || !subtree.contains(node.id) ||
            !contains_descendant(scene, stale_root.id, node.id) || !node.prefab_source.has_value() ||
            !is_valid_scene_prefab_source_link(*node.prefab_source)) {
            return false;
        }
        return !same_prefab_source_identity(*node.prefab_source, root_link);
    });
}

[[nodiscard]] bool scene_prefab_refresh_is_nested_prefab_root(const SceneNode& node,
                                                              const ScenePrefabSourceLink& root_link) noexcept {
    return node.prefab_source.has_value() && is_valid_scene_prefab_source_link(*node.prefab_source) &&
           !same_prefab_source_identity(*node.prefab_source, root_link) && node.prefab_source->source_node_index == 1U;
}

void collect_nested_prefab_propagation_preview(const Scene& scene, SceneNodeId instance_root,
                                               const ScenePrefabSourceLink& root_link,
                                               const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
                                               ScenePrefabInstanceRefreshPlan& plan) {
    plan.nested_prefab_propagation_preview.clear();
    plan.descendant_linked_prefab_instance_root_count = 0;
    plan.distinct_nested_prefab_asset_count = 0;
    std::set<std::string> distinct_assets;
    std::size_t order = 0;
    for (const auto& node : scene.nodes()) {
        if (!subtree.contains(node.id) || node.id == instance_root) {
            continue;
        }
        if (!scene_prefab_refresh_is_nested_prefab_root(node, root_link)) {
            continue;
        }
        if (!node.prefab_source.has_value()) {
            continue;
        }
        const auto& nested_ps = *node.prefab_source;
        SceneNestedPrefabPropagationPreviewRow preview_row;
        preview_row.preview_order = order++;
        preview_row.instance_root = node.id;
        preview_row.node_name = node.name;
        preview_row.prefab_name = nested_ps.prefab_name;
        preview_row.prefab_path = nested_ps.prefab_path;
        plan.nested_prefab_propagation_preview.push_back(std::move(preview_row));
        ++plan.descendant_linked_prefab_instance_root_count;
        distinct_assets.insert(nested_ps.prefab_name + std::string{'\x1f'} + nested_ps.prefab_path);
    }
    plan.distinct_nested_prefab_asset_count = distinct_assets.size();
}

[[nodiscard]] bool scene_prefab_refresh_nested_parent_anchor_available(
    const Scene& scene, const SceneNode& nested_root, const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
    const ScenePrefabSourceLink& root_link, const std::unordered_set<std::string>& refreshed_names,
    const std::unordered_set<std::string>& ambiguous_refreshed_names) {
    if (nested_root.parent == null_scene_node || !subtree.contains(nested_root.parent)) {
        return false;
    }

    const auto* parent = scene.find_node(nested_root.parent);
    if (parent == nullptr || !parent->prefab_source.has_value() ||
        !scene_prefab_refresh_matches_source(*parent, root_link)) {
        return false;
    }

    const auto& parent_source_name = parent->prefab_source->source_node_name;
    return refreshed_names.contains(parent_source_name) && !ambiguous_refreshed_names.contains(parent_source_name);
}

[[nodiscard]] bool scene_prefab_refresh_nested_subtree_has_parent_source_link(
    const Scene& scene, const SceneNode& nested_root, const std::unordered_set<SceneNodeId, SceneNodeIdHash>& subtree,
    const ScenePrefabSourceLink& root_link) {
    return std::ranges::any_of(scene.nodes(), [&](const auto& node) {
        if (node.id == nested_root.id || !subtree.contains(node.id) ||
            !contains_descendant(scene, nested_root.id, node.id)) {
            return false;
        }
        return scene_prefab_refresh_matches_source(node, root_link);
    });
}

void add_scene_prefab_refresh_subtree_nodes(const Scene& scene, SceneNodeId root,
                                            std::unordered_set<SceneNodeId, SceneNodeIdHash>& nodes) {
    const auto subtree = collect_subtree(scene, root);
    nodes.insert(subtree.begin(), subtree.end());
}

void finalize_scene_prefab_refresh_row(ScenePrefabInstanceRefreshRow& row) {
    row.status_label = std::string(scene_prefab_instance_refresh_status_label(row.status));
    row.kind_label = std::string(scene_prefab_instance_refresh_row_kind_label(row.kind));
    row.blocking = row.status == ScenePrefabInstanceRefreshStatus::blocked;
}

[[nodiscard]] ScenePrefabInstanceRefreshRow
make_scene_prefab_refresh_row(std::string id, SceneNodeId current_node, const std::string& current_node_name,
                              std::uint32_t refreshed_node_index, const std::string& source_node_name,
                              ScenePrefabInstanceRefreshStatus status, ScenePrefabInstanceRefreshRowKind kind,
                              std::string diagnostic) {
    ScenePrefabInstanceRefreshRow row;
    row.id = std::move(id);
    row.current_node = current_node;
    row.current_node_name = sanitize_text(current_node_name);
    row.refreshed_node_index = refreshed_node_index;
    row.source_node_name = sanitize_text(source_node_name);
    row.status = status;
    row.kind = kind;
    row.diagnostic = std::move(diagnostic);
    finalize_scene_prefab_refresh_row(row);
    return row;
}

void add_scene_prefab_refresh_row(ScenePrefabInstanceRefreshPlan& plan, ScenePrefabInstanceRefreshRow row) {
    switch (row.status) {
    case ScenePrefabInstanceRefreshStatus::blocked:
        ++plan.blocking_count;
        break;
    case ScenePrefabInstanceRefreshStatus::warning:
        ++plan.warning_count;
        break;
    case ScenePrefabInstanceRefreshStatus::ready:
        break;
    }

    switch (row.kind) {
    case ScenePrefabInstanceRefreshRowKind::preserve_node:
        ++plan.preserve_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::add_source_node:
        ++plan.add_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::remove_stale_node:
        ++plan.remove_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::keep_local_child:
        ++plan.keep_local_child_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local:
        ++plan.keep_stale_source_node_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::keep_nested_prefab_instance:
        ++plan.keep_nested_prefab_instance_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::unsupported_nested_prefab_instance:
        ++plan.unsupported_nested_prefab_instance_count;
        break;
    case ScenePrefabInstanceRefreshRowKind::invalid_instance_root:
    case ScenePrefabInstanceRefreshRowKind::ambiguous_source_node:
    case ScenePrefabInstanceRefreshRowKind::unsupported_local_child:
    case ScenePrefabInstanceRefreshRowKind::unsupported_stale_source_subtree:
        break;
    }

    plan.rows.push_back(std::move(row));
    plan.row_count = plan.rows.size();
}

[[nodiscard]] std::unordered_map<std::string, std::size_t>
count_prefab_source_node_names(const PrefabDefinition& prefab) {
    std::unordered_map<std::string, std::size_t> counts;
    for (const auto& node : prefab.nodes) {
        ++counts[node.name];
    }
    return counts;
}

[[nodiscard]] std::string refreshed_prefab_path_or_link(std::string_view refreshed_prefab_path,
                                                        const ScenePrefabSourceLink& link) {
    return refreshed_prefab_path.empty() ? link.prefab_path : std::string(refreshed_prefab_path);
}

void finalize_scene_prefab_refresh_plan(ScenePrefabInstanceRefreshPlan& plan, bool valid_refreshed_prefab) {
    if (plan.blocking_count > 0U || !valid_refreshed_prefab) {
        plan.status = ScenePrefabInstanceRefreshStatus::blocked;
    } else if (plan.warning_count > 0U) {
        plan.status = ScenePrefabInstanceRefreshStatus::warning;
    } else {
        plan.status = ScenePrefabInstanceRefreshStatus::ready;
    }
    plan.status_label = std::string(scene_prefab_instance_refresh_status_label(plan.status));
    plan.can_apply = plan.status != ScenePrefabInstanceRefreshStatus::blocked && valid_refreshed_prefab;
    plan.mutates = plan.can_apply && !plan.rows.empty();
    plan.executes = false;
}

void block_scene_prefab_refresh_plan_after_validation(ScenePrefabInstanceRefreshPlan& plan, std::string diagnostic) {
    plan.can_apply = false;
    plan.mutates = false;
    plan.status = ScenePrefabInstanceRefreshStatus::blocked;
    plan.status_label = std::string(scene_prefab_instance_refresh_status_label(plan.status));
    plan.diagnostics.emplace_back(std::move(diagnostic));
}

} // namespace

SceneAuthoringDocument SceneAuthoringDocument::from_scene(Scene scene, std::string scene_path) {
    return {std::move(scene), std::move(scene_path)};
}

SceneAuthoringDocument::SceneAuthoringDocument(Scene scene, std::string scene_path)
    : scene_(std::move(scene)), scene_path_(std::move(scene_path)), saved_scene_text_(serialize_scene(scene_)) {}

const Scene& SceneAuthoringDocument::scene() const noexcept {
    return scene_;
}

std::string_view SceneAuthoringDocument::scene_path() const noexcept {
    return scene_path_;
}

bool SceneAuthoringDocument::dirty() const {
    try {
        return serialize_scene(scene_) != saved_scene_text_;
    } catch (const std::exception&) {
        return true;
    }
}

SceneNodeId SceneAuthoringDocument::selected_node() const noexcept {
    return selected_node_;
}

std::vector<SceneAuthoringNodeRow> SceneAuthoringDocument::hierarchy_rows() const {
    std::vector<SceneAuthoringNodeRow> rows;
    rows.reserve(scene_.nodes().size());
    std::unordered_set<SceneNodeId, SceneNodeIdHash> visited;
    for (const auto& node : scene_.nodes()) {
        if (node.parent == null_scene_node) {
            append_hierarchy_row(scene_, node.id, 0, selected_node_, rows, visited);
        }
    }
    return rows;
}

bool SceneAuthoringDocument::select_node(SceneNodeId node) noexcept {
    if (!contains_node(scene_, node)) {
        return false;
    }
    selected_node_ = node;
    return true;
}

bool SceneAuthoringDocument::replace_scene(Scene scene, SceneNodeId selected_node) {
    if (selected_node != null_scene_node && scene.find_node(selected_node) == nullptr) {
        selected_node = null_scene_node;
    }
    scene_ = std::move(scene);
    selected_node_ = selected_node;
    return true;
}

void SceneAuthoringDocument::set_scene_path(std::string scene_path) {
    scene_path_ = std::move(scene_path);
}

void SceneAuthoringDocument::mark_saved() {
    saved_scene_text_ = serialize_scene(scene_);
}

UndoableAction make_scene_authoring_rename_node_action(SceneAuthoringDocument& document, SceneNodeId node,
                                                       std::string name) {
    if (!valid_text_field(name) || document.scene().find_node(node) == nullptr) {
        return empty_action();
    }

    auto after = document.scene();
    after.find_node(node)->name = std::move(name);
    return make_snapshot_action("Rename Scene Node", document, document.scene(), std::move(after),
                                document.selected_node(), node);
}

UndoableAction make_scene_authoring_create_node_action(SceneAuthoringDocument& document, std::string name,
                                                       SceneNodeId parent) {
    if (!valid_text_field(name) || (parent != null_scene_node && document.scene().find_node(parent) == nullptr)) {
        return empty_action();
    }

    auto after = document.scene();
    const auto created = after.create_node(std::move(name));
    if (parent != null_scene_node) {
        after.set_parent(created, parent);
    }
    return make_snapshot_action("Create Scene Node", document, document.scene(), std::move(after),
                                document.selected_node(), created);
}

UndoableAction make_scene_authoring_reparent_node_action(SceneAuthoringDocument& document, SceneNodeId node,
                                                         SceneNodeId parent) {
    const auto& scene = document.scene();
    if (node == null_scene_node || parent == null_scene_node || scene.find_node(node) == nullptr ||
        scene.find_node(parent) == nullptr || node == parent || contains_descendant(scene, node, parent)) {
        return empty_action();
    }

    auto after = scene;
    after.set_parent(node, parent);
    return make_snapshot_action("Reparent Scene Node", document, scene, std::move(after), document.selected_node(),
                                node);
}

std::vector<SceneReparentParentOption>
make_scene_authoring_reparent_parent_options(const SceneAuthoringDocument& document, SceneNodeId node) {
    const auto& scene = document.scene();
    const auto* self = scene.find_node(node);
    if (self == nullptr) {
        return {};
    }

    const SceneNodeId current_parent = self->parent;

    std::vector<SceneReparentParentOption> out;
    out.reserve(scene.nodes().size());

    for (const auto& n : scene.nodes()) {
        const SceneNodeId candidate = n.id;
        if (candidate == node) {
            continue;
        }
        if (candidate == current_parent) {
            continue;
        }
        if (contains_descendant(scene, node, candidate)) {
            continue;
        }

        const auto depth = scene_node_depth_from_root(scene, candidate);
        std::string label;
        label.assign(depth * 2U, ' ');
        label += n.name;
        label += "##reparent_parent_";
        label += std::to_string(candidate.value);
        out.push_back(SceneReparentParentOption{.parent = candidate, .label = std::move(label)});
    }

    std::ranges::sort(out, [](const SceneReparentParentOption& a, const SceneReparentParentOption& b) {
        return a.parent.value < b.parent.value;
    });
    return out;
}

UndoableAction make_scene_authoring_delete_node_action(SceneAuthoringDocument& document, SceneNodeId node) {
    if (document.scene().find_node(node) == nullptr) {
        return empty_action();
    }

    auto rebuilt = rebuild_without_subtree(document.scene(), node);
    auto selected_after = document.selected_node();
    if (rebuilt.removed.contains(selected_after)) {
        selected_after = null_scene_node;
    } else if (selected_after != null_scene_node) {
        const auto remapped = rebuilt.remap.find(selected_after);
        selected_after = remapped == rebuilt.remap.end() ? null_scene_node : remapped->second;
    }
    return make_snapshot_action("Delete Scene Node", document, document.scene(), std::move(rebuilt.scene),
                                document.selected_node(), selected_after);
}

UndoableAction make_scene_authoring_duplicate_subtree_action(SceneAuthoringDocument& document, SceneNodeId root,
                                                             std::string root_name) {
    auto prefab = build_prefab_from_scene_subtree(document.scene(), root, "duplicate.prefab");
    if (!prefab.has_value() || !valid_text_field(root_name)) {
        return empty_action();
    }

    prefab->nodes[0].name = std::move(root_name);
    auto after = document.scene();
    const auto instance = instantiate_prefab(after, *prefab);
    const auto selected_after = instance.nodes.empty() ? null_scene_node : instance.nodes.front();
    return make_snapshot_action("Duplicate Scene Subtree", document, document.scene(), std::move(after),
                                document.selected_node(), selected_after);
}

UndoableAction make_scene_authoring_transform_edit_action(SceneAuthoringDocument& document,
                                                          const SceneNodeTransformDraft& draft) {
    auto after = document.scene();
    if (!apply_scene_node_transform_draft(after, draft)) {
        return empty_action();
    }
    return make_snapshot_action("Edit Scene Transform", document, document.scene(), std::move(after),
                                document.selected_node(), draft.node);
}

UndoableAction make_scene_authoring_component_edit_action(SceneAuthoringDocument& document,
                                                          const SceneNodeComponentDraft& draft) {
    auto after = document.scene();
    if (!apply_scene_node_component_draft(after, draft)) {
        return empty_action();
    }
    return make_snapshot_action("Edit Scene Components", document, document.scene(), std::move(after),
                                document.selected_node(), draft.node);
}

UndoableAction make_scene_authoring_instantiate_prefab_action(SceneAuthoringDocument& document,
                                                              PrefabDefinition prefab) {
    return make_scene_authoring_instantiate_prefab_action(document, std::move(prefab), {});
}

UndoableAction make_scene_authoring_instantiate_prefab_action(SceneAuthoringDocument& document, PrefabDefinition prefab,
                                                              std::string prefab_path) {
    if (!is_valid_prefab_definition(prefab)) {
        return empty_action();
    }

    auto after = document.scene();
    const auto instance = instantiate_prefab(
        after, PrefabInstantiateDesc{.prefab = std::move(prefab), .source_path = std::move(prefab_path)});
    const auto selected_after = instance.nodes.empty() ? null_scene_node : instance.nodes.front();
    return make_snapshot_action("Instantiate Prefab", document, document.scene(), std::move(after),
                                document.selected_node(), selected_after);
}

std::optional<PrefabDefinition> build_prefab_from_selected_node(const SceneAuthoringDocument& document,
                                                                std::string name) {
    if (document.selected_node() == null_scene_node) {
        return std::nullopt;
    }
    return build_prefab_from_scene_subtree(document.scene(), document.selected_node(), std::move(name));
}

std::string_view scene_prefab_instance_source_link_status_label(ScenePrefabInstanceSourceLinkStatus status) noexcept {
    switch (status) {
    case ScenePrefabInstanceSourceLinkStatus::linked:
        return "Linked";
    case ScenePrefabInstanceSourceLinkStatus::stale:
        return "Stale";
    }
    return "Stale";
}

ScenePrefabInstanceSourceLinkModel
make_scene_prefab_instance_source_link_model(const SceneAuthoringDocument& document) {
    ScenePrefabInstanceSourceLinkModel model;
    for (const auto& node : document.scene().nodes()) {
        if (!node.prefab_source.has_value()) {
            continue;
        }

        const bool valid = is_valid_scene_prefab_source_link(*node.prefab_source);
        ScenePrefabInstanceSourceLinkRow row;
        row.id = std::to_string(node.id.value);
        row.node = node.id;
        row.node_name = node.name;
        row.status = valid ? ScenePrefabInstanceSourceLinkStatus::linked : ScenePrefabInstanceSourceLinkStatus::stale;
        row.status_label = std::string(scene_prefab_instance_source_link_status_label(row.status));
        row.prefab_name = node.prefab_source->prefab_name;
        row.prefab_path = node.prefab_source->prefab_path;
        row.source_node_index = node.prefab_source->source_node_index;
        row.source_node_name = node.prefab_source->source_node_name;
        row.diagnostic = valid ? "" : "prefab source link is incomplete";
        row.selected = node.id == document.selected_node();
        if (valid) {
            ++model.linked_count;
        } else {
            ++model.stale_count;
            model.has_diagnostics = true;
        }
        model.rows.push_back(std::move(row));
    }
    model.has_links = !model.rows.empty();
    return model;
}

mirakana::ui::UiDocument
make_scene_prefab_instance_source_link_ui_model(const ScenePrefabInstanceSourceLinkModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("scene_prefab_source_links", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"scene_prefab_source_links"};
    append_label(document, root, "scene_prefab_source_links.linked_count", std::to_string(model.linked_count));
    append_label(document, root, "scene_prefab_source_links.stale_count", std::to_string(model.stale_count));

    add_or_throw(document, make_child("scene_prefab_source_links.rows", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_root{"scene_prefab_source_links.rows"};
    for (const auto& row : model.rows) {
        const auto prefix = "scene_prefab_source_links.rows." + sanitize_element_id(row.id);
        mirakana::ui::ElementDesc item = make_child(prefix, rows_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.node_name);
        item.enabled = row.status == ScenePrefabInstanceSourceLinkStatus::linked;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId row_root{prefix};
        append_label(document, row_root, prefix + ".status", row.status_label);
        append_label(document, row_root, prefix + ".node", std::to_string(row.node.value));
        append_label(document, row_root, prefix + ".prefab_name", row.prefab_name);
        append_label(document, row_root, prefix + ".prefab_path", row.prefab_path.empty() ? "-" : row.prefab_path);
        append_label(document, row_root, prefix + ".source_node",
                     std::to_string(row.source_node_index) + ":" + row.source_node_name);
        append_label(document, row_root, prefix + ".diagnostic", row.diagnostic.empty() ? "-" : row.diagnostic);
    }

    return document;
}

std::string_view scene_prefab_instance_refresh_status_label(ScenePrefabInstanceRefreshStatus status) noexcept {
    switch (status) {
    case ScenePrefabInstanceRefreshStatus::ready:
        return "ready";
    case ScenePrefabInstanceRefreshStatus::warning:
        return "warning";
    case ScenePrefabInstanceRefreshStatus::blocked:
        return "blocked";
    }
    return "blocked";
}

std::string_view scene_prefab_instance_refresh_row_kind_label(ScenePrefabInstanceRefreshRowKind kind) noexcept {
    switch (kind) {
    case ScenePrefabInstanceRefreshRowKind::preserve_node:
        return "preserve_node";
    case ScenePrefabInstanceRefreshRowKind::add_source_node:
        return "add_source_node";
    case ScenePrefabInstanceRefreshRowKind::remove_stale_node:
        return "remove_stale_node";
    case ScenePrefabInstanceRefreshRowKind::keep_local_child:
        return "keep_local_child";
    case ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local:
        return "keep_stale_source_node_as_local";
    case ScenePrefabInstanceRefreshRowKind::keep_nested_prefab_instance:
        return "keep_nested_prefab_instance";
    case ScenePrefabInstanceRefreshRowKind::invalid_instance_root:
        return "invalid_instance_root";
    case ScenePrefabInstanceRefreshRowKind::ambiguous_source_node:
        return "ambiguous_source_node";
    case ScenePrefabInstanceRefreshRowKind::unsupported_local_child:
        return "unsupported_local_child";
    case ScenePrefabInstanceRefreshRowKind::unsupported_stale_source_subtree:
        return "unsupported_stale_source_subtree";
    case ScenePrefabInstanceRefreshRowKind::unsupported_nested_prefab_instance:
        return "unsupported_nested_prefab_instance";
    }
    return "invalid_instance_root";
}

namespace {

[[nodiscard]] std::optional<std::string> validate_nested_prefab_propagation_apply(
    const SceneAuthoringDocument& document, const ScenePrefabInstanceRefreshPlan& root_plan, SceneNodeId instance_root,
    const PrefabDefinition& refreshed_prefab, std::string_view refreshed_prefab_path,
    const ScenePrefabInstanceRefreshPolicy& policy);

[[nodiscard]] ScenePrefabInstanceRefreshPlan plan_scene_prefab_instance_refresh_without_nested_propagation_simulation(
    const SceneAuthoringDocument& document, SceneNodeId instance_root, const PrefabDefinition& refreshed_prefab,
    std::string_view refreshed_prefab_path, const ScenePrefabInstanceRefreshPolicy& policy) {
    ScenePrefabInstanceRefreshPlan plan;
    plan.instance_root = instance_root;
    plan.prefab_name = refreshed_prefab.name;
    plan.keep_local_children = policy.keep_local_children;
    plan.keep_stale_source_nodes_as_local = policy.keep_stale_source_nodes_as_local;
    plan.keep_nested_prefab_instances = policy.keep_nested_prefab_instances;
    plan.apply_reviewed_nested_prefab_propagation_requested = policy.apply_reviewed_nested_prefab_propagation;

    const bool valid_refreshed_prefab = is_valid_prefab_definition(refreshed_prefab);
    const auto* root_node = document.scene().find_node(instance_root);
    if (root_node == nullptr || !root_node->prefab_source.has_value() ||
        !is_valid_scene_prefab_source_link(*root_node->prefab_source) ||
        root_node->prefab_source->source_node_index != 1U) {
        add_scene_prefab_refresh_row(
            plan,
            make_scene_prefab_refresh_row("root.invalid", instance_root, root_node == nullptr ? "" : root_node->name, 0,
                                          "", ScenePrefabInstanceRefreshStatus::blocked,
                                          ScenePrefabInstanceRefreshRowKind::invalid_instance_root,
                                          "selected node is not a linked prefab instance root"));
        finalize_scene_prefab_refresh_plan(plan, valid_refreshed_prefab);
        return plan;
    }

    const auto root_link = *root_node->prefab_source;
    plan.prefab_name = root_link.prefab_name;
    plan.prefab_path = refreshed_prefab_path_or_link(refreshed_prefab_path, root_link);

    const ScenePrefabSourceLink refreshed_root_link{
        .prefab_name = refreshed_prefab.name,
        .prefab_path = plan.prefab_path,
        .source_node_index = 1U,
        .source_node_name = refreshed_prefab.nodes.empty() ? std::string{} : refreshed_prefab.nodes.front().name,
    };
    if (!valid_refreshed_prefab || root_link.prefab_name != refreshed_prefab.name ||
        !is_valid_scene_prefab_source_link(refreshed_root_link)) {
        plan.diagnostics.emplace_back("refreshed prefab does not match the selected source link");
        add_scene_prefab_refresh_row(
            plan, make_scene_prefab_refresh_row("root.invalid", instance_root, root_node->name, 0,
                                                root_link.source_node_name, ScenePrefabInstanceRefreshStatus::blocked,
                                                ScenePrefabInstanceRefreshRowKind::invalid_instance_root,
                                                "refreshed prefab is invalid or has a mismatched source identity"));
        finalize_scene_prefab_refresh_plan(plan, valid_refreshed_prefab);
        return plan;
    }

    const auto subtree = collect_subtree(document.scene(), instance_root);
    collect_nested_prefab_propagation_preview(document.scene(), instance_root, root_link, subtree, plan);
    const auto refreshed_name_counts = count_prefab_source_node_names(refreshed_prefab);
    std::unordered_set<std::string> ambiguous_refreshed_names;
    std::unordered_set<std::string> refreshed_names;
    for (const auto& [name, count] : refreshed_name_counts) {
        refreshed_names.insert(name);
        if (count > 1U) {
            ambiguous_refreshed_names.insert(name);
        }
    }

    std::unordered_map<std::string, std::size_t> instance_source_name_counts;
    std::vector<std::string> instance_source_names_ordered;
    for (const auto& source_node : document.scene().nodes()) {
        if (!subtree.contains(source_node.id)) {
            continue;
        }

        if (source_node.id != instance_root &&
            scene_prefab_refresh_has_nonmatching_ancestor(document.scene(), source_node.id, subtree, root_link)) {
            continue;
        }

        if (!scene_prefab_refresh_matches_source(source_node, root_link)) {
            continue;
        }

        const auto& source_name = source_node.prefab_source->source_node_name;
        if (!instance_source_name_counts.contains(source_name)) {
            instance_source_names_ordered.push_back(source_name);
        }
        ++instance_source_name_counts[source_name];
    }

    std::unordered_set<std::string> ambiguous_instance_names;
    std::vector<std::string> ambiguous_instance_names_ordered;
    for (const auto& name : instance_source_names_ordered) {
        if (instance_source_name_counts[name] > 1U) {
            ambiguous_instance_names.insert(name);
            ambiguous_instance_names_ordered.push_back(name);
        }
    }

    std::unordered_set<SceneNodeId, SceneNodeIdHash> stale_keep_roots;
    std::unordered_set<SceneNodeId, SceneNodeIdHash> stale_keep_subtree_nodes;
    if (policy.keep_stale_source_nodes_as_local) {
        for (const auto& source_node : document.scene().nodes()) {
            if (!subtree.contains(source_node.id) || !scene_prefab_refresh_matches_source(source_node, root_link) ||
                scene_prefab_refresh_has_nonmatching_ancestor(document.scene(), source_node.id, subtree, root_link)) {
                continue;
            }

            const auto& source_name = source_node.prefab_source->source_node_name;
            if (refreshed_names.contains(source_name) || ambiguous_instance_names.contains(source_name) ||
                scene_prefab_refresh_has_stale_source_ancestor(document.scene(), source_node.id, subtree, root_link,
                                                               refreshed_names, ambiguous_instance_names)) {
                continue;
            }

            if (!scene_prefab_refresh_stale_parent_anchor_available(document.scene(), source_node, subtree, root_link,
                                                                    refreshed_names, ambiguous_refreshed_names)) {
                continue;
            }

            if (scene_prefab_refresh_stale_subtree_has_refreshed_source_descendant(
                    document.scene(), source_node, subtree, root_link, refreshed_names) ||
                scene_prefab_refresh_stale_subtree_has_nested_source_link(document.scene(), source_node, subtree,
                                                                          root_link)) {
                add_scene_prefab_refresh_row(
                    plan, make_scene_prefab_refresh_row(
                              scene_prefab_refresh_row_id(source_name) + ".stale_subtree_conflict", source_node.id,
                              source_node.name, 0, source_name, ScenePrefabInstanceRefreshStatus::blocked,
                              ScenePrefabInstanceRefreshRowKind::unsupported_stale_source_subtree,
                              "stale source subtree cannot be kept as local safely"));
                continue;
            }

            stale_keep_roots.insert(source_node.id);
            add_scene_prefab_refresh_subtree_nodes(document.scene(), source_node.id, stale_keep_subtree_nodes);
        }
    }

    std::unordered_map<std::string, SceneNodeId> instance_by_source_name;
    std::unordered_map<std::string, std::string> instance_node_name_by_source_name;
    for (const auto& source_node : document.scene().nodes()) {
        if (!subtree.contains(source_node.id)) {
            continue;
        }

        if (source_node.id != instance_root &&
            scene_prefab_refresh_has_nonmatching_ancestor(document.scene(), source_node.id, subtree, root_link)) {
            continue;
        }

        if (stale_keep_subtree_nodes.contains(source_node.id)) {
            continue;
        }

        if (!scene_prefab_refresh_matches_source(source_node, root_link)) {
            if (scene_prefab_refresh_is_nested_prefab_root(source_node, root_link)) {
                const bool has_anchor = scene_prefab_refresh_nested_parent_anchor_available(
                    document.scene(), source_node, subtree, root_link, refreshed_names, ambiguous_refreshed_names);
                const bool has_parent_source_descendant = scene_prefab_refresh_nested_subtree_has_parent_source_link(
                    document.scene(), source_node, subtree, root_link);
                const bool can_keep_nested =
                    policy.keep_nested_prefab_instances && has_anchor && !has_parent_source_descendant;
                std::string diagnostic;
                if (can_keep_nested) {
                    diagnostic = "keep reviewed nested prefab instance under refreshed parent";
                } else if (!policy.keep_nested_prefab_instances) {
                    diagnostic = "nested prefab instance requires explicit keep policy";
                } else if (!has_anchor) {
                    diagnostic = "nested prefab instance cannot be anchored under a refreshed source parent";
                } else {
                    diagnostic = "nested prefab subtree contains parent prefab source links";
                }
                add_scene_prefab_refresh_row(
                    plan, make_scene_prefab_refresh_row(
                              scene_prefab_refresh_nested_row_id(source_node.id), source_node.id, source_node.name, 0,
                              source_node.prefab_source->prefab_name,
                              can_keep_nested ? ScenePrefabInstanceRefreshStatus::warning
                                              : ScenePrefabInstanceRefreshStatus::blocked,
                              can_keep_nested ? ScenePrefabInstanceRefreshRowKind::keep_nested_prefab_instance
                                              : ScenePrefabInstanceRefreshRowKind::unsupported_nested_prefab_instance,
                              std::move(diagnostic)));
                continue;
            }

            const bool can_keep_local =
                policy.keep_local_children && scene_prefab_refresh_local_parent_anchor_available(
                                                  document.scene(), source_node, subtree, root_link, refreshed_names);
            add_scene_prefab_refresh_row(
                plan, make_scene_prefab_refresh_row(
                          "node." + std::to_string(source_node.id.value) + ".local_child", source_node.id,
                          source_node.name, 0, source_node.name,
                          can_keep_local ? ScenePrefabInstanceRefreshStatus::warning
                                         : ScenePrefabInstanceRefreshStatus::blocked,
                          can_keep_local ? ScenePrefabInstanceRefreshRowKind::keep_local_child
                                         : ScenePrefabInstanceRefreshRowKind::unsupported_local_child,
                          can_keep_local ? "keep reviewed local child subtree under refreshed instance"
                                         : "prefab refresh would remove an unlinked or differently linked child"));
            continue;
        }

        const auto [it, inserted] =
            instance_by_source_name.emplace(source_node.prefab_source->source_node_name, source_node.id);
        if (inserted) {
            instance_node_name_by_source_name.emplace(source_node.prefab_source->source_node_name, source_node.name);
        }
    }

    for (const auto& name : ambiguous_instance_names_ordered) {
        add_scene_prefab_refresh_row(
            plan, make_scene_prefab_refresh_row(scene_prefab_refresh_row_id(name) + ".instance_ambiguous",
                                                instance_by_source_name[name], instance_node_name_by_source_name[name],
                                                0, name, ScenePrefabInstanceRefreshStatus::blocked,
                                                ScenePrefabInstanceRefreshRowKind::ambiguous_source_node,
                                                "prefab instance has duplicate source node names"));
    }

    std::vector<std::string> ambiguous_refreshed_names_ordered;
    for (const auto& refreshed_node : refreshed_prefab.nodes) {
        if (ambiguous_refreshed_names.contains(refreshed_node.name) &&
            std::ranges::find(ambiguous_refreshed_names_ordered, refreshed_node.name) ==
                ambiguous_refreshed_names_ordered.end()) {
            ambiguous_refreshed_names_ordered.push_back(refreshed_node.name);
        }
    }
    for (const auto& name : ambiguous_refreshed_names_ordered) {
        add_scene_prefab_refresh_row(
            plan, make_scene_prefab_refresh_row(scene_prefab_refresh_row_id(name) + ".refreshed_ambiguous",
                                                instance_by_source_name[name], instance_node_name_by_source_name[name],
                                                0, name, ScenePrefabInstanceRefreshStatus::blocked,
                                                ScenePrefabInstanceRefreshRowKind::ambiguous_source_node,
                                                "refreshed prefab has duplicate source node names"));
    }

    if (prefab_root_count(refreshed_prefab) != 1U) {
        add_scene_prefab_refresh_row(
            plan,
            make_scene_prefab_refresh_row("root.multi_root", instance_root, root_node->name, 0,
                                          root_link.source_node_name, ScenePrefabInstanceRefreshStatus::blocked,
                                          ScenePrefabInstanceRefreshRowKind::invalid_instance_root,
                                          "selected prefab instance refresh requires a single-root source prefab"));
    }

    for (std::size_t index = 0; index < refreshed_prefab.nodes.size(); ++index) {
        const auto& refreshed_node = refreshed_prefab.nodes[index];
        if (ambiguous_refreshed_names.contains(refreshed_node.name)) {
            continue;
        }

        const auto existing = instance_by_source_name.find(refreshed_node.name);
        if (existing != instance_by_source_name.end() && !ambiguous_instance_names.contains(refreshed_node.name)) {
            add_scene_prefab_refresh_row(
                plan, make_scene_prefab_refresh_row(scene_prefab_refresh_row_id(refreshed_node.name), existing->second,
                                                    instance_node_name_by_source_name[refreshed_node.name],
                                                    static_cast<std::uint32_t>(index + 1U), refreshed_node.name,
                                                    ScenePrefabInstanceRefreshStatus::ready,
                                                    ScenePrefabInstanceRefreshRowKind::preserve_node,
                                                    "preserve existing scene node state"));
        } else {
            add_scene_prefab_refresh_row(
                plan, make_scene_prefab_refresh_row(scene_prefab_refresh_row_id(refreshed_node.name), null_scene_node,
                                                    "", static_cast<std::uint32_t>(index + 1U), refreshed_node.name,
                                                    ScenePrefabInstanceRefreshStatus::ready,
                                                    ScenePrefabInstanceRefreshRowKind::add_source_node,
                                                    "add refreshed source node"));
        }
    }

    for (const auto& source_node : document.scene().nodes()) {
        if (!subtree.contains(source_node.id) || !scene_prefab_refresh_matches_source(source_node, root_link) ||
            scene_prefab_refresh_has_nonmatching_ancestor(document.scene(), source_node.id, subtree, root_link)) {
            continue;
        }

        const auto& source_name = source_node.prefab_source->source_node_name;
        const auto node_id = source_node.id;
        if (stale_keep_roots.contains(node_id)) {
            add_scene_prefab_refresh_row(
                plan, make_scene_prefab_refresh_row(scene_prefab_refresh_row_id(source_name), node_id, source_node.name,
                                                    0, source_name, ScenePrefabInstanceRefreshStatus::warning,
                                                    ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local,
                                                    "keep reviewed stale source node subtree as local"));
            continue;
        }
        if (stale_keep_subtree_nodes.contains(node_id)) {
            continue;
        }
        if (!refreshed_names.contains(source_name) && !ambiguous_instance_names.contains(source_name)) {
            add_scene_prefab_refresh_row(
                plan, make_scene_prefab_refresh_row(scene_prefab_refresh_row_id(source_name), node_id,
                                                    instance_node_name_by_source_name[source_name], 0, source_name,
                                                    ScenePrefabInstanceRefreshStatus::warning,
                                                    ScenePrefabInstanceRefreshRowKind::remove_stale_node,
                                                    "remove stale source node from instance"));
        }
    }

    finalize_scene_prefab_refresh_plan(plan, valid_refreshed_prefab);
    return plan;
}

} // namespace

ScenePrefabInstanceRefreshPlan plan_scene_prefab_instance_refresh(const SceneAuthoringDocument& document,
                                                                  SceneNodeId instance_root,
                                                                  const PrefabDefinition& refreshed_prefab,
                                                                  std::string_view refreshed_prefab_path,
                                                                  const ScenePrefabInstanceRefreshPolicy& policy) {
    auto plan = plan_scene_prefab_instance_refresh_without_nested_propagation_simulation(
        document, instance_root, refreshed_prefab, refreshed_prefab_path, policy);
    if (policy.apply_reviewed_nested_prefab_propagation && !plan.nested_prefab_propagation_preview.empty() &&
        plan.can_apply) {
        if (!policy.load_prefab_for_nested_propagation) {
            block_scene_prefab_refresh_plan_after_validation(
                plan, "nested prefab propagation apply requires load_prefab_for_nested_propagation");
        } else if (const auto propagation_error = validate_nested_prefab_propagation_apply(
                       document, plan, instance_root, refreshed_prefab, refreshed_prefab_path, policy)) {
            block_scene_prefab_refresh_plan_after_validation(plan, *propagation_error);
        }
    }
    return plan;
}

namespace {

void merge_propagation_id_map(std::unordered_map<SceneNodeId, SceneNodeId, SceneAuthoringSceneNodeIdHash>& id_map,
                              const ScenePrefabInstanceRefreshResult& nested_result) {
    for (auto& entry : id_map) {
        const auto it = nested_result.source_to_result_node_id.find(entry.second);
        if (it != nested_result.source_to_result_node_id.end()) {
            entry.second = it->second;
        }
    }
}

[[nodiscard]] ScenePrefabInstanceRefreshResult
execute_scene_prefab_instance_refresh_plan(const SceneAuthoringDocument& document,
                                           const ScenePrefabInstanceRefreshPlan& plan, SceneNodeId instance_root,
                                           const PrefabDefinition& refreshed_prefab) {
    ScenePrefabInstanceRefreshResult result;
    const auto& source_scene = document.scene();
    const auto* root_node = source_scene.find_node(instance_root);
    if (root_node == nullptr || !root_node->prefab_source.has_value()) {
        result.diagnostic = "scene prefab instance refresh is blocked";
        return result;
    }
    const auto root_link = *root_node->prefab_source;
    const auto subtree = collect_subtree(source_scene, instance_root);
    std::unordered_set<SceneNodeId, SceneNodeIdHash> stale_as_local_nodes;
    std::unordered_set<SceneNodeId, SceneNodeIdHash> local_preserve_nodes;
    std::unordered_set<SceneNodeId, SceneNodeIdHash> nested_prefab_nodes;
    for (const auto& row : plan.rows) {
        if (row.current_node == null_scene_node) {
            continue;
        }
        if (row.kind == ScenePrefabInstanceRefreshRowKind::keep_local_child) {
            add_scene_prefab_refresh_subtree_nodes(source_scene, row.current_node, local_preserve_nodes);
        } else if (row.kind == ScenePrefabInstanceRefreshRowKind::keep_stale_source_node_as_local) {
            add_scene_prefab_refresh_subtree_nodes(source_scene, row.current_node, stale_as_local_nodes);
        } else if (row.kind == ScenePrefabInstanceRefreshRowKind::keep_nested_prefab_instance) {
            add_scene_prefab_refresh_subtree_nodes(source_scene, row.current_node, nested_prefab_nodes);
        }
    }

    std::unordered_set<SceneNodeId, SceneNodeIdHash> preserved_nodes = local_preserve_nodes;
    preserved_nodes.insert(stale_as_local_nodes.begin(), stale_as_local_nodes.end());
    preserved_nodes.insert(nested_prefab_nodes.begin(), nested_prefab_nodes.end());

    std::unordered_map<std::string, const SceneNode*> instance_by_source_name;
    for (const auto node_id : subtree) {
        const auto* node = source_scene.find_node(node_id);
        if (node != nullptr && !preserved_nodes.contains(node_id) && node->prefab_source.has_value() &&
            is_valid_scene_prefab_source_link(*node->prefab_source) &&
            same_prefab_source_identity(*node->prefab_source, root_link)) {
            instance_by_source_name.emplace(node->prefab_source->source_node_name, node);
        }
    }

    Scene next(std::string(source_scene.name()));
    std::unordered_map<SceneNodeId, SceneNodeId, SceneNodeIdHash> outside_remap;
    for (const auto& node : source_scene.nodes()) {
        if (subtree.contains(node.id)) {
            continue;
        }
        const auto rebuilt = next.create_node(node.name);
        auto* rebuilt_node = next.find_node(rebuilt);
        rebuilt_node->transform = node.transform;
        rebuilt_node->prefab_source = node.prefab_source;
        next.set_components(rebuilt, node.components);
        outside_remap.emplace(node.id, rebuilt);
    }

    for (const auto& node : source_scene.nodes()) {
        if (subtree.contains(node.id) || node.parent == null_scene_node || subtree.contains(node.parent)) {
            continue;
        }
        const auto child = outside_remap.find(node.id);
        const auto parent = outside_remap.find(node.parent);
        if (child != outside_remap.end() && parent != outside_remap.end()) {
            next.set_parent(child->second, parent->second);
        }
    }

    std::vector<SceneNodeId> refreshed_nodes;
    refreshed_nodes.reserve(refreshed_prefab.nodes.size());
    std::unordered_map<std::string, SceneNodeId> refreshed_by_source_name;
    for (std::size_t index = 0; index < refreshed_prefab.nodes.size(); ++index) {
        const auto& refreshed_node = refreshed_prefab.nodes[index];
        const auto existing = instance_by_source_name.find(refreshed_node.name);
        const SceneNode* preserved = existing == instance_by_source_name.end() ? nullptr : existing->second;
        const auto created = next.create_node(preserved == nullptr ? refreshed_node.name : preserved->name);
        auto* created_node = next.find_node(created);
        created_node->transform = preserved == nullptr ? refreshed_node.transform : preserved->transform;
        created_node->prefab_source = ScenePrefabSourceLink{
            .prefab_name = refreshed_prefab.name,
            .prefab_path = plan.prefab_path,
            .source_node_index = static_cast<std::uint32_t>(index + 1U),
            .source_node_name = refreshed_node.name,
        };
        next.set_components(created, preserved == nullptr ? refreshed_node.components : preserved->components);
        refreshed_nodes.push_back(created);
        refreshed_by_source_name.emplace(refreshed_node.name, created);
        if (preserved == nullptr) {
            ++result.added_count;
        } else {
            ++result.preserved_count;
        }
    }

    for (std::size_t index = 0; index < refreshed_prefab.nodes.size(); ++index) {
        const auto parent_index = refreshed_prefab.nodes[index].parent_index;
        if (parent_index == 0U) {
            if (root_node->parent != null_scene_node) {
                const auto parent = outside_remap.find(root_node->parent);
                if (parent != outside_remap.end()) {
                    next.set_parent(refreshed_nodes[index], parent->second);
                }
            }
            continue;
        }
        next.set_parent(refreshed_nodes[index], refreshed_nodes[parent_index - 1U]);
    }

    std::unordered_map<SceneNodeId, SceneNodeId, SceneNodeIdHash> local_remap;
    for (const auto& source_node : source_scene.nodes()) {
        if (!preserved_nodes.contains(source_node.id)) {
            continue;
        }
        const auto copied = next.create_node(source_node.name);
        auto* copied_node = next.find_node(copied);
        copied_node->transform = source_node.transform;
        copied_node->prefab_source =
            stale_as_local_nodes.contains(source_node.id) && scene_prefab_refresh_matches_source(source_node, root_link)
                ? std::nullopt
                : source_node.prefab_source;
        next.set_components(copied, source_node.components);
        local_remap.emplace(source_node.id, copied);
    }

    for (const auto& source_node : source_scene.nodes()) {
        if (!preserved_nodes.contains(source_node.id) || source_node.parent == null_scene_node) {
            continue;
        }

        const auto child = local_remap.find(source_node.id);
        if (child == local_remap.end()) {
            continue;
        }

        if (const auto local_parent = local_remap.find(source_node.parent); local_parent != local_remap.end()) {
            next.set_parent(child->second, local_parent->second);
            continue;
        }

        if (const auto* source_parent = source_scene.find_node(source_node.parent);
            source_parent != nullptr && source_parent->prefab_source.has_value()) {
            if (const auto refreshed_parent =
                    refreshed_by_source_name.find(source_parent->prefab_source->source_node_name);
                refreshed_parent != refreshed_by_source_name.end()) {
                next.set_parent(child->second, refreshed_parent->second);
                continue;
            }
        }

        if (const auto outside_parent = outside_remap.find(source_node.parent); outside_parent != outside_remap.end()) {
            next.set_parent(child->second, outside_parent->second);
        }
    }

    result.removed_count = plan.remove_count;
    result.kept_local_child_count = plan.keep_local_child_count;
    result.kept_stale_source_node_count = plan.keep_stale_source_node_count;
    result.kept_nested_prefab_instance_count = plan.keep_nested_prefab_instance_count;
    if (document.selected_node() != null_scene_node) {
        if (!subtree.contains(document.selected_node())) {
            const auto selected = outside_remap.find(document.selected_node());
            result.selected_node = selected == outside_remap.end() ? null_scene_node : selected->second;
        } else if (const auto selected_local = local_remap.find(document.selected_node());
                   selected_local != local_remap.end()) {
            result.selected_node = selected_local->second;
        } else if (const auto* selected_node = source_scene.find_node(document.selected_node());
                   selected_node != nullptr && selected_node->prefab_source.has_value()) {
            const auto selected = refreshed_by_source_name.find(selected_node->prefab_source->source_node_name);
            result.selected_node =
                selected == refreshed_by_source_name.end() ? refreshed_nodes.front() : selected->second;
        }
    }
    if (result.selected_node == null_scene_node && !refreshed_nodes.empty()) {
        result.selected_node = refreshed_nodes.front();
    }

    result.source_to_result_node_id.clear();
    result.source_to_result_node_id.reserve(outside_remap.size() + local_remap.size() + instance_by_source_name.size());
    for (const auto& entry : outside_remap) {
        result.source_to_result_node_id.emplace(entry.first, entry.second);
    }
    for (const auto& entry : local_remap) {
        result.source_to_result_node_id.emplace(entry.first, entry.second);
    }
    for (const auto& entry : instance_by_source_name) {
        const auto* old_ptr = entry.second;
        if (old_ptr == nullptr) {
            continue;
        }
        const auto mapped = refreshed_by_source_name.find(entry.first);
        if (mapped != refreshed_by_source_name.end()) {
            result.source_to_result_node_id.emplace(old_ptr->id, mapped->second);
        }
    }

    result.scene = std::move(next);
    result.applied = true;
    result.diagnostic = "scene prefab instance refresh applied";
    return result;
}

[[nodiscard]] std::optional<std::string> validate_nested_prefab_propagation_apply(
    const SceneAuthoringDocument& document, const ScenePrefabInstanceRefreshPlan& root_plan, SceneNodeId instance_root,
    const PrefabDefinition& refreshed_prefab, std::string_view /*refreshed_prefab_path*/,
    const ScenePrefabInstanceRefreshPolicy& policy) {
    auto work = SceneAuthoringDocument::from_scene(document.scene(), std::string(document.scene_path()));
    (void)work.select_node(document.selected_node());

    auto r0 = execute_scene_prefab_instance_refresh_plan(work, root_plan, instance_root, refreshed_prefab);
    if (!r0.applied || !r0.scene.has_value()) {
        return "nested prefab propagation simulation failed for root refresh";
    }
    (void)work.replace_scene(std::move(*r0.scene), r0.selected_node);

    auto id_map = r0.source_to_result_node_id;
    ScenePrefabInstanceRefreshPolicy nested_policy = policy;
    nested_policy.apply_reviewed_nested_prefab_propagation = false;

    for (const auto& preview : root_plan.nested_prefab_propagation_preview) {
        if (!policy.load_prefab_for_nested_propagation) {
            return "nested prefab propagation apply requires load_prefab_for_nested_propagation";
        }
        const auto nested_prefab = policy.load_prefab_for_nested_propagation(preview.prefab_path);
        if (!nested_prefab.has_value()) {
            return "nested prefab propagation could not load prefab: " + preview.prefab_path;
        }
        const auto mapped = id_map.find(preview.instance_root);
        if (mapped == id_map.end()) {
            return "nested prefab propagation lost instance root mapping";
        }
        const auto nested_plan = plan_scene_prefab_instance_refresh_without_nested_propagation_simulation(
            work, mapped->second, *nested_prefab, preview.prefab_path, nested_policy);
        if (!nested_plan.can_apply) {
            return "nested prefab propagation blocked for " + preview.prefab_path;
        }
        auto rn = execute_scene_prefab_instance_refresh_plan(work, nested_plan, mapped->second, *nested_prefab);
        if (!rn.applied || !rn.scene.has_value()) {
            return "nested prefab propagation apply failed for " + preview.prefab_path;
        }
        (void)work.replace_scene(std::move(*rn.scene), rn.selected_node);
        merge_propagation_id_map(id_map, rn);
    }
    return std::nullopt;
}

} // namespace

ScenePrefabInstanceRefreshResult apply_scene_prefab_instance_refresh(const SceneAuthoringDocument& document,
                                                                     SceneNodeId instance_root,
                                                                     const PrefabDefinition& refreshed_prefab,
                                                                     std::string_view refreshed_prefab_path,
                                                                     const ScenePrefabInstanceRefreshPolicy& policy) {
    ScenePrefabInstanceRefreshResult result;
    const auto plan =
        plan_scene_prefab_instance_refresh(document, instance_root, refreshed_prefab, refreshed_prefab_path, policy);
    if (!plan.can_apply) {
        result.diagnostic = "scene prefab instance refresh is blocked";
        return result;
    }

    if (!policy.apply_reviewed_nested_prefab_propagation || plan.nested_prefab_propagation_preview.empty()) {
        return execute_scene_prefab_instance_refresh_plan(document, plan, instance_root, refreshed_prefab);
    }

    ScenePrefabInstanceRefreshPolicy root_policy = policy;
    root_policy.apply_reviewed_nested_prefab_propagation = false;

    auto work = SceneAuthoringDocument::from_scene(document.scene(), std::string(document.scene_path()));
    (void)work.select_node(document.selected_node());

    auto r0 = execute_scene_prefab_instance_refresh_plan(work, plan, instance_root, refreshed_prefab);
    if (!r0.applied || !r0.scene.has_value()) {
        result.diagnostic = r0.diagnostic.empty() ? "scene prefab instance refresh is blocked" : r0.diagnostic;
        return result;
    }
    (void)work.replace_scene(std::move(*r0.scene), r0.selected_node);

    auto combined_map = r0.source_to_result_node_id;
    ScenePrefabInstanceRefreshResult combined = r0;

    ScenePrefabInstanceRefreshPolicy nested_policy = policy;
    nested_policy.apply_reviewed_nested_prefab_propagation = false;

    for (const auto& preview : plan.nested_prefab_propagation_preview) {
        if (!policy.load_prefab_for_nested_propagation) {
            result.diagnostic = "nested prefab propagation apply requires load_prefab_for_nested_propagation";
            return result;
        }
        const auto nested_prefab = policy.load_prefab_for_nested_propagation(preview.prefab_path);
        if (!nested_prefab.has_value()) {
            result.diagnostic = "nested prefab propagation could not load prefab: " + preview.prefab_path;
            return result;
        }
        const auto mapped = combined_map.find(preview.instance_root);
        if (mapped == combined_map.end()) {
            result.diagnostic = "nested prefab propagation lost instance root mapping";
            return result;
        }
        const auto nested_plan = plan_scene_prefab_instance_refresh_without_nested_propagation_simulation(
            work, mapped->second, *nested_prefab, preview.prefab_path, nested_policy);
        if (!nested_plan.can_apply) {
            result.diagnostic = "nested prefab propagation blocked for " + preview.prefab_path;
            return result;
        }
        auto rn = execute_scene_prefab_instance_refresh_plan(work, nested_plan, mapped->second, *nested_prefab);
        if (!rn.applied || !rn.scene.has_value()) {
            result.diagnostic = rn.diagnostic.empty()
                                    ? ("nested prefab propagation apply failed for " + preview.prefab_path)
                                    : rn.diagnostic;
            return result;
        }
        (void)work.replace_scene(std::move(*rn.scene), rn.selected_node);
        combined.preserved_count += rn.preserved_count;
        combined.added_count += rn.added_count;
        combined.removed_count += rn.removed_count;
        combined.kept_local_child_count += rn.kept_local_child_count;
        combined.kept_stale_source_node_count += rn.kept_stale_source_node_count;
        combined.kept_nested_prefab_instance_count += rn.kept_nested_prefab_instance_count;
        combined.selected_node = rn.selected_node;
        merge_propagation_id_map(combined_map, rn);
    }

    combined.source_to_result_node_id = std::move(combined_map);
    combined.scene.emplace(work.scene());
    combined.applied = true;
    combined.diagnostic = "scene prefab instance refresh applied with nested prefab propagation";
    return combined;
}

ScenePrefabInstanceRefreshBatchPlan
plan_scene_prefab_instance_refresh_batch(const SceneAuthoringDocument& document,
                                         std::vector<ScenePrefabInstanceRefreshBatchTargetInput> targets,
                                         const ScenePrefabInstanceRefreshPolicy& policy) {
    ScenePrefabInstanceRefreshBatchPlan batch;
    batch.apply_reviewed_nested_prefab_propagation_requested = policy.apply_reviewed_nested_prefab_propagation;
    sort_prefab_refresh_batch_targets_in_place(targets);
    if (targets.empty()) {
        batch.batch_diagnostics.emplace_back("prefab instance refresh batch requires at least one target");
        batch.ordered_targets = std::move(targets);
        finalize_scene_prefab_refresh_batch_plan(batch, false);
        return batch;
    }
    if (prefab_refresh_batch_has_duplicate_instance_roots(targets)) {
        batch.batch_diagnostics.emplace_back("prefab instance refresh batch contains duplicate instance roots");
        batch.ordered_targets = std::move(targets);
        finalize_scene_prefab_refresh_batch_plan(batch, false);
        return batch;
    }
    if (prefab_refresh_batch_has_instance_root_hierarchy_conflict(document.scene(), targets)) {
        batch.batch_diagnostics.emplace_back(
            "prefab instance refresh batch cannot include nested instance roots in one batch");
        batch.ordered_targets = std::move(targets);
        finalize_scene_prefab_refresh_batch_plan(batch, false);
        return batch;
    }

    bool all_prefabs_valid = true;
    batch.targets.reserve(targets.size());
    for (const auto& target : targets) {
        if (!is_valid_prefab_definition(target.refreshed_prefab)) {
            all_prefabs_valid = false;
        }
        batch.targets.push_back(plan_scene_prefab_instance_refresh(
            document, target.instance_root, target.refreshed_prefab, target.refreshed_prefab_path, policy));
    }
    batch.ordered_targets = std::move(targets);
    finalize_scene_prefab_refresh_batch_plan(batch, all_prefabs_valid);
    return batch;
}

ScenePrefabInstanceRefreshBatchResult
apply_scene_prefab_instance_refresh_batch(const SceneAuthoringDocument& document,
                                          std::vector<ScenePrefabInstanceRefreshBatchTargetInput> targets,
                                          const ScenePrefabInstanceRefreshPolicy& policy) {
    ScenePrefabInstanceRefreshBatchResult result;
    auto batch_plan = plan_scene_prefab_instance_refresh_batch(document, std::move(targets), policy);
    if (!batch_plan.can_apply) {
        result.diagnostic = "scene prefab instance refresh batch is blocked";
        return result;
    }

    auto work = SceneAuthoringDocument::from_scene(document.scene(), std::string(document.scene_path()));
    (void)work.select_node(document.selected_node());
    auto work_targets = std::move(batch_plan.ordered_targets);

    for (std::size_t index = 0; index < work_targets.size(); ++index) {
        auto refresh_result = apply_scene_prefab_instance_refresh(work, work_targets[index].instance_root,
                                                                  work_targets[index].refreshed_prefab,
                                                                  work_targets[index].refreshed_prefab_path, policy);
        if (!refresh_result.applied || !refresh_result.scene.has_value()) {
            result.diagnostic = refresh_result.diagnostic.empty() ? "scene prefab instance refresh batch failed"
                                                                  : refresh_result.diagnostic;
            return result;
        }
        for (std::size_t later = index + 1; later < work_targets.size(); ++later) {
            const auto mapped = refresh_result.source_to_result_node_id.find(work_targets[later].instance_root);
            if (mapped == refresh_result.source_to_result_node_id.end()) {
                result.diagnostic = "scene prefab instance refresh batch lost instance root mapping";
                return result;
            }
            work_targets[later].instance_root = mapped->second;
        }
        (void)work.replace_scene(std::move(*refresh_result.scene), refresh_result.selected_node);
    }

    result.applied = true;
    result.targets_applied = work_targets.size();
    result.scene = work.scene();
    result.selected_node = work.selected_node();
    result.diagnostic = "scene prefab instance refresh batch applied";
    return result;
}

UndoableAction make_scene_prefab_instance_refresh_action(SceneAuthoringDocument& document, SceneNodeId instance_root,
                                                         const PrefabDefinition& refreshed_prefab,
                                                         const std::string& refreshed_prefab_path,
                                                         const ScenePrefabInstanceRefreshPolicy& policy) {
    auto result =
        apply_scene_prefab_instance_refresh(document, instance_root, refreshed_prefab, refreshed_prefab_path, policy);
    if (!result.applied || !result.scene.has_value()) {
        return empty_action();
    }

    return make_snapshot_action("Refresh Prefab Instance", document, document.scene(), std::move(*result.scene),
                                document.selected_node(), result.selected_node);
}

UndoableAction
make_scene_prefab_instance_refresh_batch_action(SceneAuthoringDocument& document,
                                                std::vector<ScenePrefabInstanceRefreshBatchTargetInput> targets,
                                                const ScenePrefabInstanceRefreshPolicy& policy) {
    Scene before = document.scene();
    const SceneNodeId selected_before = document.selected_node();
    auto batch_result = apply_scene_prefab_instance_refresh_batch(document, std::move(targets), policy);
    if (!batch_result.applied || !batch_result.scene.has_value()) {
        return empty_action();
    }
    return make_snapshot_action("Refresh Prefab Instances (Batch)", document, std::move(before),
                                std::move(*batch_result.scene), selected_before, batch_result.selected_node);
}

namespace {

void append_nested_prefab_propagation_preview_ui(mirakana::ui::UiDocument& document,
                                                 const mirakana::ui::ElementId& root, std::string_view id_prefix,
                                                 const std::vector<SceneNestedPrefabPropagationPreviewRow>& preview,
                                                 std::string_view propagation_operator_policy) {
    if (preview.empty()) {
        return;
    }
    const std::string p{id_prefix};
    append_label(document, root, p + ".propagation_preview.contract",
                 "ge.editor.scene_nested_prefab_propagation_preview.v1");
    append_label(document, root, p + ".propagation_preview.operator_policy", std::string(propagation_operator_policy));
    add_or_throw(document, make_child(p + ".propagation_preview.rows", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_root{p + ".propagation_preview.rows"};
    for (const auto& row : preview) {
        const std::string item_prefix = p + ".propagation_preview.rows." + std::to_string(row.preview_order);
        mirakana::ui::ElementDesc item = make_child(item_prefix, rows_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.node_name + " (" + row.prefab_name + ")");
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_root{item_prefix};
        append_label(document, item_root, item_prefix + ".preview_order", std::to_string(row.preview_order));
        append_label(document, item_root, item_prefix + ".instance_root", std::to_string(row.instance_root.value));
        append_label(document, item_root, item_prefix + ".node_name", row.node_name);
        append_label(document, item_root, item_prefix + ".prefab_name", row.prefab_name);
        append_label(document, item_root, item_prefix + ".prefab_path",
                     row.prefab_path.empty() ? "-" : row.prefab_path);
    }
}

} // namespace

mirakana::ui::UiDocument make_scene_prefab_instance_refresh_ui_model(const ScenePrefabInstanceRefreshPlan& plan) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("scene_prefab_instance_refresh", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"scene_prefab_instance_refresh"};
    append_button(document, root, "scene_prefab_instance_refresh.apply", "Apply Prefab Refresh", plan.can_apply);
    append_label(document, root, "scene_prefab_instance_refresh.status", plan.status_label);
    append_label(document, root, "scene_prefab_instance_refresh.prefab_name", plan.prefab_name);
    append_label(document, root, "scene_prefab_instance_refresh.prefab_path",
                 plan.prefab_path.empty() ? "-" : plan.prefab_path);
    append_label(document, root, "scene_prefab_instance_refresh.summary.rows", std::to_string(plan.row_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.preserve", std::to_string(plan.preserve_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.add", std::to_string(plan.add_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.remove", std::to_string(plan.remove_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.keep_local",
                 std::to_string(plan.keep_local_child_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.keep_stale",
                 std::to_string(plan.keep_stale_source_node_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.keep_nested",
                 std::to_string(plan.keep_nested_prefab_instance_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.unsupported_nested",
                 std::to_string(plan.unsupported_nested_prefab_instance_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.descendant_linked_prefab_roots",
                 std::to_string(plan.descendant_linked_prefab_instance_root_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.distinct_nested_prefab_assets",
                 std::to_string(plan.distinct_nested_prefab_asset_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.blocking", std::to_string(plan.blocking_count));
    append_label(document, root, "scene_prefab_instance_refresh.summary.warnings", std::to_string(plan.warning_count));
    append_label(document, root, "scene_prefab_instance_refresh.mutates", plan.mutates ? "true" : "false");
    append_label(document, root, "scene_prefab_instance_refresh.executes", plan.executes ? "true" : "false");
    append_label(document, root, "scene_prefab_instance_refresh.keep_local_children",
                 plan.keep_local_children ? "true" : "false");
    append_label(document, root, "scene_prefab_instance_refresh.keep_stale_source_nodes_as_local",
                 plan.keep_stale_source_nodes_as_local ? "true" : "false");
    append_label(document, root, "scene_prefab_instance_refresh.keep_nested_prefab_instances",
                 plan.keep_nested_prefab_instances ? "true" : "false");
    append_label(document, root, "scene_prefab_instance_refresh.apply_reviewed_nested_prefab_propagation",
                 plan.apply_reviewed_nested_prefab_propagation_requested ? "true" : "false");

    if (scene_prefab_refresh_plan_emits_nested_variant_alignment(plan)) {
        append_label(document, root, "scene_prefab_instance_refresh.nested_prefab_variant_alignment.contract",
                     "ge.editor.scene_prefab_nested_variant_alignment.v1");
        append_label(document, root, "scene_prefab_instance_refresh.nested_prefab_variant_alignment.review_pattern",
                     "prefab_variant_conflicts.rows");
    }

    if (scene_prefab_refresh_plan_emits_local_child_variant_alignment(plan)) {
        append_label(document, root, "scene_prefab_instance_refresh.local_child_variant_alignment.contract",
                     "ge.editor.scene_prefab_local_child_variant_alignment.v1");
        append_label(document, root, "scene_prefab_instance_refresh.local_child_variant_alignment.review_pattern",
                     "prefab_variant_conflicts.rows");
    }

    if (scene_prefab_refresh_plan_emits_stale_source_variant_alignment(plan)) {
        append_label(document, root, "scene_prefab_instance_refresh.stale_source_variant_alignment.contract",
                     "ge.editor.scene_prefab_stale_source_variant_alignment.v1");
        append_label(document, root, "scene_prefab_instance_refresh.stale_source_variant_alignment.review_pattern",
                     "prefab_variant_conflicts.rows");
    }

    if (scene_prefab_refresh_plan_emits_source_node_variant_alignment(plan)) {
        append_label(document, root, "scene_prefab_instance_refresh.source_node_variant_alignment.contract",
                     "ge.editor.scene_prefab_source_node_variant_alignment.v1");
        append_label(document, root, "scene_prefab_instance_refresh.source_node_variant_alignment.review_pattern",
                     "prefab_variant_conflicts.rows");
    }

    append_nested_prefab_propagation_preview_ui(
        document, root, "scene_prefab_instance_refresh", plan.nested_prefab_propagation_preview,
        plan.apply_reviewed_nested_prefab_propagation_requested ? "reviewed_chained_prefab_refresh_after_root"
                                                                : "preview_only_no_chained_refresh");

    add_or_throw(document, make_child("scene_prefab_instance_refresh.rows", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId rows_root{"scene_prefab_instance_refresh.rows"};
    append_prefab_instance_refresh_plan_rows(document, rows_root, "scene_prefab_instance_refresh", plan);

    add_or_throw(document,
                 make_child("scene_prefab_instance_refresh.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{"scene_prefab_instance_refresh.diagnostics"};
    for (std::size_t index = 0; index < plan.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, "scene_prefab_instance_refresh.diagnostics." + std::to_string(index),
                     plan.diagnostics[index]);
    }

    return document;
}

mirakana::ui::UiDocument
make_scene_prefab_instance_refresh_batch_ui_model(const ScenePrefabInstanceRefreshBatchPlan& plan) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("scene_prefab_instance_refresh_batch", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"scene_prefab_instance_refresh_batch"};
    append_button(document, root, "scene_prefab_instance_refresh_batch.apply", "Apply Batch Prefab Refresh",
                  plan.can_apply);
    append_label(document, root, "scene_prefab_instance_refresh_batch.status", plan.status_label);
    append_label(document, root, "scene_prefab_instance_refresh_batch.target_count", std::to_string(plan.target_count));
    append_label(document, root, "scene_prefab_instance_refresh_batch.blocking_target_count",
                 std::to_string(plan.blocking_target_count));
    append_label(document, root, "scene_prefab_instance_refresh_batch.warning_target_count",
                 std::to_string(plan.warning_target_count));
    append_label(document, root, "scene_prefab_instance_refresh_batch.ready_target_count",
                 std::to_string(plan.ready_target_count));
    append_label(document, root, "scene_prefab_instance_refresh_batch.apply_reviewed_nested_prefab_propagation",
                 plan.apply_reviewed_nested_prefab_propagation_requested ? "true" : "false");

    if (scene_prefab_refresh_batch_has_nested_variant_alignment_surface(plan)) {
        append_label(document, root, "scene_prefab_instance_refresh_batch.nested_prefab_variant_alignment.contract",
                     "ge.editor.scene_prefab_nested_variant_alignment.v1");
        append_label(document, root,
                     "scene_prefab_instance_refresh_batch.nested_prefab_variant_alignment.review_pattern",
                     "prefab_variant_conflicts.rows");
    }

    if (scene_prefab_refresh_batch_has_local_child_variant_alignment_surface(plan)) {
        append_label(document, root, "scene_prefab_instance_refresh_batch.local_child_variant_alignment.contract",
                     "ge.editor.scene_prefab_local_child_variant_alignment.v1");
        append_label(document, root, "scene_prefab_instance_refresh_batch.local_child_variant_alignment.review_pattern",
                     "prefab_variant_conflicts.rows");
    }

    if (scene_prefab_refresh_batch_has_stale_source_variant_alignment_surface(plan)) {
        append_label(document, root, "scene_prefab_instance_refresh_batch.stale_source_variant_alignment.contract",
                     "ge.editor.scene_prefab_stale_source_variant_alignment.v1");
        append_label(document, root,
                     "scene_prefab_instance_refresh_batch.stale_source_variant_alignment.review_pattern",
                     "prefab_variant_conflicts.rows");
    }

    if (scene_prefab_refresh_batch_has_source_node_variant_alignment_surface(plan)) {
        append_label(document, root, "scene_prefab_instance_refresh_batch.source_node_variant_alignment.contract",
                     "ge.editor.scene_prefab_source_node_variant_alignment.v1");
        append_label(document, root, "scene_prefab_instance_refresh_batch.source_node_variant_alignment.review_pattern",
                     "prefab_variant_conflicts.rows");
    }

    add_or_throw(document,
                 make_child("scene_prefab_instance_refresh_batch.diagnostics", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId batch_diag_root{"scene_prefab_instance_refresh_batch.diagnostics"};
    for (std::size_t index = 0; index < plan.batch_diagnostics.size(); ++index) {
        append_label(document, batch_diag_root,
                     "scene_prefab_instance_refresh_batch.diagnostics." + std::to_string(index),
                     plan.batch_diagnostics[index]);
    }

    add_or_throw(document,
                 make_child("scene_prefab_instance_refresh_batch.targets", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId targets_root{"scene_prefab_instance_refresh_batch.targets"};
    for (std::size_t ti = 0; ti < plan.targets.size(); ++ti) {
        const std::string tgt_prefix = "scene_prefab_instance_refresh_batch.targets." + std::to_string(ti);
        const auto& sub_plan = plan.targets[ti];
        mirakana::ui::ElementDesc item = make_child(tgt_prefix, targets_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(sub_plan.prefab_name + " @" + std::to_string(sub_plan.instance_root.value));
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId tgt_root{tgt_prefix};
        append_label(document, tgt_root, tgt_prefix + ".status", sub_plan.status_label);
        append_label(document, tgt_root, tgt_prefix + ".instance_root", std::to_string(sub_plan.instance_root.value));
        append_label(document, tgt_root, tgt_prefix + ".prefab_name", sub_plan.prefab_name);
        append_label(document, tgt_root, tgt_prefix + ".prefab_path",
                     sub_plan.prefab_path.empty() ? "-" : sub_plan.prefab_path);
        append_label(document, tgt_root, tgt_prefix + ".can_apply", sub_plan.can_apply ? "true" : "false");
        add_or_throw(document, make_child(tgt_prefix + ".rows", tgt_root, mirakana::ui::SemanticRole::list));
        const mirakana::ui::ElementId rows_id{tgt_prefix + ".rows"};
        append_prefab_instance_refresh_plan_rows(document, rows_id, tgt_prefix, sub_plan);
        append_nested_prefab_propagation_preview_ui(
            document, tgt_root, tgt_prefix, sub_plan.nested_prefab_propagation_preview,
            sub_plan.apply_reviewed_nested_prefab_propagation_requested ? "reviewed_chained_prefab_refresh_after_root"
                                                                        : "preview_only_no_chained_refresh");
    }

    return document;
}

void save_scene_authoring_document(ITextStore& store, std::string_view path, SceneAuthoringDocument& document) {
    store.write_text(path, serialize_scene(document.scene()));
    document.mark_saved();
}

SceneAuthoringDocument load_scene_authoring_document(ITextStore& store, std::string_view path) {
    return SceneAuthoringDocument::from_scene(deserialize_scene(store.read_text(path)), std::string(path));
}

mirakana::FileDialogRequest make_scene_open_dialog_request(std::string_view default_location) {
    return make_scene_dialog_request(mirakana::FileDialogKind::open_file, "Open Scene", "Open", default_location);
}

mirakana::FileDialogRequest make_scene_save_dialog_request(std::string_view default_location) {
    return make_scene_dialog_request(mirakana::FileDialogKind::save_file, "Save Scene", "Save", default_location);
}

EditorSceneFileDialogModel make_scene_open_dialog_model(const mirakana::FileDialogResult& result) {
    return make_scene_file_dialog_model(result, EditorSceneFileDialogMode::open);
}

EditorSceneFileDialogModel make_scene_save_dialog_model(const mirakana::FileDialogResult& result) {
    return make_scene_file_dialog_model(result, EditorSceneFileDialogMode::save);
}

mirakana::ui::UiDocument make_scene_file_dialog_ui_model(const EditorSceneFileDialogModel& model) {
    mirakana::ui::UiDocument document;
    const auto mode_id = std::string(scene_file_dialog_mode_id(model.mode));
    const std::string root_prefix = "scene_file_dialog." + mode_id;
    add_or_throw(document, make_root(root_prefix, mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{root_prefix};

    for (const auto& row : model.rows) {
        const std::string row_prefix = root_prefix + "." + sanitize_element_id(row.id);
        add_or_throw(document, make_child(row_prefix, root, mirakana::ui::SemanticRole::list_item));
        const mirakana::ui::ElementId row_root{row_prefix};
        append_label(document, row_root, row_prefix + ".label", row.label);
        append_label(document, row_root, row_prefix + ".value", row.value);
    }

    const std::string diagnostics_prefix = root_prefix + ".diagnostics";
    add_or_throw(document, make_child(diagnostics_prefix, root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{diagnostics_prefix};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_label(document, diagnostics_root, diagnostics_prefix + "." + std::to_string(index),
                     model.diagnostics[index]);
    }

    return document;
}

void save_prefab_authoring_document(ITextStore& store, std::string_view path, const PrefabDefinition& prefab) {
    store.write_text(path, serialize_prefab_definition(prefab));
}

PrefabDefinition load_prefab_authoring_document(ITextStore& store, std::string_view path) {
    return deserialize_prefab_definition(store.read_text(path));
}

std::vector<SceneAuthoringDiagnostic> validate_scene_authoring_references(const Scene& scene,
                                                                          const AssetRegistry& registry) {
    std::vector<SceneAuthoringDiagnostic> diagnostics;
    for (const auto& node : scene.nodes()) {
        if (node.components.mesh_renderer.has_value()) {
            add_reference_diagnostic(diagnostics, node.id, node.components.mesh_renderer->mesh, "mesh_renderer.mesh",
                                     AssetKind::mesh, registry);
            add_reference_diagnostic(diagnostics, node.id, node.components.mesh_renderer->material,
                                     "mesh_renderer.material", AssetKind::material, registry);
        }
        if (node.components.sprite_renderer.has_value()) {
            add_reference_diagnostic(diagnostics, node.id, node.components.sprite_renderer->sprite,
                                     "sprite_renderer.sprite", AssetKind::texture, registry);
            add_reference_diagnostic(diagnostics, node.id, node.components.sprite_renderer->material,
                                     "sprite_renderer.material", AssetKind::material, registry);
        }
    }

    std::ranges::sort(diagnostics, [](const SceneAuthoringDiagnostic& lhs, const SceneAuthoringDiagnostic& rhs) {
        if (lhs.node.value != rhs.node.value) {
            return lhs.node.value < rhs.node.value;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        return lhs.field < rhs.field;
    });
    return diagnostics;
}

std::vector<ScenePackageCandidateRow>
make_scene_package_candidate_rows(const SceneAuthoringDocument& document, std::string_view cooked_scene_path,
                                  std::string_view package_index_path,
                                  std::initializer_list<std::string_view> prefab_source_paths) {
    std::vector<ScenePackageCandidateRow> rows;
    rows.push_back(ScenePackageCandidateRow{.kind = ScenePackageCandidateKind::scene_source,
                                            .path = std::string(document.scene_path()),
                                            .runtime_file = false});
    rows.push_back(ScenePackageCandidateRow{
        .kind = ScenePackageCandidateKind::scene_cooked, .path = std::string(cooked_scene_path), .runtime_file = true});
    rows.push_back(ScenePackageCandidateRow{.kind = ScenePackageCandidateKind::package_index,
                                            .path = std::string(package_index_path),
                                            .runtime_file = true});
    for (const auto path : prefab_source_paths) {
        rows.push_back(ScenePackageCandidateRow{
            .kind = ScenePackageCandidateKind::prefab_source, .path = std::string(path), .runtime_file = false});
    }
    return rows;
}

std::string_view scene_package_candidate_kind_label(ScenePackageCandidateKind kind) noexcept {
    switch (kind) {
    case ScenePackageCandidateKind::scene_source:
        return "scene_source";
    case ScenePackageCandidateKind::scene_cooked:
        return "scene_cooked";
    case ScenePackageCandidateKind::package_index:
        return "package_index";
    case ScenePackageCandidateKind::prefab_source:
        return "prefab_source";
    }
    return "unknown";
}

std::string_view scene_package_registration_draft_status_label(ScenePackageRegistrationDraftStatus status) noexcept {
    switch (status) {
    case ScenePackageRegistrationDraftStatus::add_runtime_file:
        return "add_runtime_file";
    case ScenePackageRegistrationDraftStatus::already_registered:
        return "already_registered";
    case ScenePackageRegistrationDraftStatus::rejected_source_file:
        return "rejected_source_file";
    case ScenePackageRegistrationDraftStatus::rejected_unsafe_path:
        return "rejected_unsafe_path";
    case ScenePackageRegistrationDraftStatus::rejected_duplicate:
        return "rejected_duplicate";
    }
    return "unknown";
}

namespace {

template <typename ExistingRuntimePackageFiles>
std::vector<ScenePackageRegistrationDraftRow> make_scene_package_registration_draft_rows_from_range(
    const std::vector<ScenePackageCandidateRow>& candidates, std::string_view project_root_path,
    const ExistingRuntimePackageFiles& existing_runtime_package_files) {
    std::unordered_set<std::string> existing_paths;
    for (const auto& package_file : existing_runtime_package_files) {
        const auto reviewed = make_runtime_package_path(package_file, {});
        if (reviewed.safe) {
            existing_paths.insert(ascii_path_key(reviewed.path));
        }
    }

    std::vector<ScenePackageRegistrationDraftRow> rows;
    rows.reserve(candidates.size());
    std::unordered_set<std::string> proposed_paths;
    for (const auto& candidate : candidates) {
        auto reviewed = make_runtime_package_path(candidate.path, project_root_path);
        ScenePackageRegistrationDraftRow row{
            .kind = candidate.kind,
            .candidate_path = candidate.path,
            .runtime_package_path = reviewed.path,
            .runtime_file = candidate.runtime_file,
            .status = ScenePackageRegistrationDraftStatus::rejected_source_file,
            .diagnostic = {},
        };

        if (!reviewed.safe) {
            row.status = ScenePackageRegistrationDraftStatus::rejected_unsafe_path;
            row.diagnostic = "unsafe runtimePackageFiles path";
            rows.push_back(std::move(row));
            continue;
        }

        if (!candidate.runtime_file) {
            row.status = ScenePackageRegistrationDraftStatus::rejected_source_file;
            row.diagnostic = "source asset is not a runtime package file";
            rows.push_back(std::move(row));
            continue;
        }

        const auto key = ascii_path_key(row.runtime_package_path);
        if (existing_paths.contains(key)) {
            row.status = ScenePackageRegistrationDraftStatus::already_registered;
            row.diagnostic = "runtime file is already registered";
            rows.push_back(std::move(row));
            continue;
        }

        if (proposed_paths.contains(key)) {
            row.status = ScenePackageRegistrationDraftStatus::rejected_duplicate;
            row.diagnostic = "duplicate runtime package addition";
            rows.push_back(std::move(row));
            continue;
        }

        proposed_paths.insert(key);
        row.status = ScenePackageRegistrationDraftStatus::add_runtime_file;
        row.diagnostic = "add runtimePackageFiles entry";
        rows.push_back(std::move(row));
    }

    return rows;
}

} // namespace

std::vector<ScenePackageRegistrationDraftRow>
make_scene_package_registration_draft_rows(const std::vector<ScenePackageCandidateRow>& candidates,
                                           std::string_view project_root_path,
                                           std::initializer_list<std::string_view> existing_runtime_package_files) {
    return make_scene_package_registration_draft_rows_from_range(candidates, project_root_path,
                                                                 existing_runtime_package_files);
}

std::vector<ScenePackageRegistrationDraftRow>
make_scene_package_registration_draft_rows(const std::vector<ScenePackageCandidateRow>& candidates,
                                           std::string_view project_root_path,
                                           const std::vector<std::string>& existing_runtime_package_files) {
    return make_scene_package_registration_draft_rows_from_range(candidates, project_root_path,
                                                                 existing_runtime_package_files);
}

ScenePackageRegistrationApplyPlan
make_scene_package_registration_apply_plan(const std::vector<ScenePackageRegistrationDraftRow>& draft_rows,
                                           std::string_view project_root_path, std::string_view game_manifest_path) {
    ScenePackageRegistrationApplyPlan plan;
    const auto manifest_path = join_manifest_path(project_root_path, game_manifest_path);
    if (!is_safe_relative_manifest_path(manifest_path)) {
        plan.game_manifest_path = manifest_path;
        plan.diagnostic = "unsafe game manifest path";
        return plan;
    }
    plan.game_manifest_path = manifest_path;

    std::unordered_set<std::string> seen;
    for (const auto& row : draft_rows) {
        if (row.status != ScenePackageRegistrationDraftStatus::add_runtime_file) {
            continue;
        }
        auto reviewed = make_runtime_package_path(row.runtime_package_path, {});
        if (!reviewed.safe) {
            plan.diagnostic = "unsafe runtimePackageFiles path";
            plan.runtime_package_files.clear();
            return plan;
        }
        const auto key = ascii_path_key(reviewed.path);
        if (seen.insert(key).second) {
            plan.runtime_package_files.push_back(std::move(reviewed.path));
        }
    }

    plan.can_apply = !plan.runtime_package_files.empty();
    plan.diagnostic =
        plan.can_apply ? "ready to apply runtimePackageFiles entries" : "no runtime package files to apply";
    return plan;
}

ScenePackageRegistrationApplyResult
apply_scene_package_registration_to_manifest(ITextStore& store, const ScenePackageRegistrationApplyPlan& plan) {
    ScenePackageRegistrationApplyResult result;
    if (!plan.can_apply) {
        result.diagnostic = plan.diagnostic.empty() ? "package registration apply plan is not ready" : plan.diagnostic;
        return result;
    }
    if (!is_safe_relative_manifest_path(plan.game_manifest_path)) {
        result.diagnostic = "unsafe game manifest path";
        return result;
    }

    std::string manifest_text;
    try {
        manifest_text = store.read_text(plan.game_manifest_path);
    } catch (const std::exception& error) {
        result.diagnostic = std::string("game manifest read failed: ") + error.what();
        return result;
    }

    auto property = find_runtime_package_files_property(manifest_text);
    if (!property.diagnostic.empty() && !property.found && property.object_close == 0U) {
        result.diagnostic = property.diagnostic;
        return result;
    }
    if (property.found && !property.valid) {
        result.diagnostic = property.diagnostic.empty() ? "runtimePackageFiles is malformed" : property.diagnostic;
        return result;
    }
    if (!property.found && property.object_close == 0U) {
        result.diagnostic = "game manifest must be a JSON object";
        return result;
    }

    auto merged = property.entries;
    std::unordered_set<std::string> seen;
    for (const auto& entry : merged) {
        seen.insert(ascii_path_key(entry));
    }

    std::size_t added = 0U;
    for (const auto& entry : plan.runtime_package_files) {
        auto reviewed = make_runtime_package_path(entry, {});
        if (!reviewed.safe) {
            result.diagnostic = "runtimePackageFiles apply entries must be safe game-relative paths";
            return result;
        }
        const auto key = ascii_path_key(reviewed.path);
        if (seen.insert(key).second) {
            merged.push_back(std::move(reviewed.path));
            ++added;
        }
    }

    const auto updated = upsert_runtime_package_files_property(manifest_text, property, merged);
    if (added > 0U || !property.found) {
        try {
            store.write_text(plan.game_manifest_path, updated);
        } catch (const std::exception& error) {
            result.diagnostic = std::string("game manifest write failed: ") + error.what();
            return result;
        }
    }

    result.applied = true;
    result.runtime_package_files = std::move(merged);
    result.diagnostic = added == 0U ? "runtimePackageFiles already up to date" : "runtimePackageFiles entries applied";
    return result;
}

} // namespace mirakana::editor
