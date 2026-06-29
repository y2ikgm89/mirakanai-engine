// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/asset_import_review.hpp"

#include "mirakana/assets/source_asset_registry.hpp"

#include <algorithm>
#include <deque>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] bool is_safe_import_repository_path(std::string_view path) noexcept;

[[nodiscard]] char lower_ascii(char value) noexcept {
    if (value >= 'A' && value <= 'Z') {
        return static_cast<char>(value - 'A' + 'a');
    }
    return value;
}

[[nodiscard]] std::string to_lower_ascii(std::string_view value) {
    std::string lowered;
    lowered.reserve(value.size());
    for (const char character : value) {
        lowered.push_back(lower_ascii(character));
    }
    return lowered;
}

[[nodiscard]] std::string_view filename_view(std::string_view path) noexcept {
    const auto slash = path.find_last_of("/\\");
    if (slash == std::string_view::npos) {
        return path;
    }
    return path.substr(slash + 1U);
}

[[nodiscard]] std::string_view extension_view(std::string_view path) noexcept {
    const auto filename = filename_view(path);
    const auto dot = filename.find_last_of('.');
    if (dot == std::string_view::npos) {
        return {};
    }
    return filename.substr(dot);
}

[[nodiscard]] std::string stem_from_path(std::string_view path) {
    const auto filename = filename_view(path);
    const auto dot = filename.find_last_of('.');
    if (dot == std::string_view::npos) {
        return std::string{filename};
    }
    return std::string{filename.substr(0U, dot)};
}

[[nodiscard]] std::string sanitize_import_stem(std::string_view stem) {
    std::string sanitized;
    sanitized.reserve(stem.size());
    bool last_was_underscore = false;
    for (const char character : stem) {
        const auto lowered = lower_ascii(character);
        const auto safe = (lowered >= 'a' && lowered <= 'z') || (lowered >= '0' && lowered <= '9') || lowered == '-' ||
                          lowered == '_' || lowered == '.';
        const char output = character == ' ' ? '_' : (safe ? lowered : '_');
        if (output == '_' && last_was_underscore) {
            continue;
        }
        sanitized.push_back(output);
        last_was_underscore = output == '_';
    }
    while (!sanitized.empty() && sanitized.back() == '_') {
        sanitized.pop_back();
    }
    while (!sanitized.empty() && sanitized.front() == '_') {
        sanitized.erase(sanitized.begin());
    }
    if (sanitized.empty()) {
        return "asset";
    }
    return sanitized;
}

[[nodiscard]] std::string trim_trailing_separators(std::string_view path) {
    std::string trimmed{path};
    while (!trimmed.empty() && (trimmed.back() == '/' || trimmed.back() == '\\')) {
        trimmed.pop_back();
    }
    return trimmed;
}

[[nodiscard]] std::string output_extension_for_kind(AssetKind kind) {
    switch (kind) {
    case AssetKind::texture:
        return ".texture";
    case AssetKind::mesh:
        return ".mesh";
    case AssetKind::audio:
        return ".audio";
    case AssetKind::unknown:
    case AssetKind::morph_mesh_cpu:
    case AssetKind::animation_float_clip:
    case AssetKind::animation_quaternion_clip:
    case AssetKind::sprite_animation:
    case AssetKind::skinned_mesh:
    case AssetKind::material:
    case AssetKind::scene:
    case AssetKind::script:
    case AssetKind::shader:
    case AssetKind::ui_atlas:
    case AssetKind::tilemap:
    case AssetKind::physics_collision_scene:
    case AssetKind::environment_profile:
    case AssetKind::environment_preset_pack:
    case AssetKind::mavg_cluster_graph:
        break;
    }
    return {};
}

[[nodiscard]] std::string reviewed_imported_path_extension_for_kind(AssetKind kind) {
    switch (kind) {
    case AssetKind::material:
        return ".material";
    case AssetKind::scene:
        return ".scene";
    case AssetKind::texture:
    case AssetKind::mesh:
    case AssetKind::audio:
        return output_extension_for_kind(kind);
    case AssetKind::unknown:
    case AssetKind::morph_mesh_cpu:
    case AssetKind::animation_float_clip:
    case AssetKind::animation_quaternion_clip:
    case AssetKind::sprite_animation:
    case AssetKind::skinned_mesh:
    case AssetKind::script:
    case AssetKind::shader:
    case AssetKind::ui_atlas:
    case AssetKind::tilemap:
    case AssetKind::physics_collision_scene:
    case AssetKind::environment_profile:
    case AssetKind::environment_preset_pack:
    case AssetKind::mavg_cluster_graph:
        break;
    }
    return {};
}

[[nodiscard]] bool is_imported_root_constrained_kind(AssetKind kind) noexcept {
    return kind == AssetKind::texture || kind == AssetKind::mesh || kind == AssetKind::audio;
}

[[nodiscard]] bool is_strict_child_repository_path(std::string_view root, std::string_view path) {
    const auto normalized_root = trim_trailing_separators(root);
    return !normalized_root.empty() && path.size() > normalized_root.size() + 1U &&
           path.substr(0U, normalized_root.size()) == normalized_root && path[normalized_root.size()] == '/';
}

[[nodiscard]] bool is_valid_reviewed_imported_path(AssetKind kind, std::string_view imported_output_root,
                                                   std::string_view path) {
    const auto expected_extension = reviewed_imported_path_extension_for_kind(kind);
    if (expected_extension.empty() || extension_view(path) != expected_extension) {
        return false;
    }
    if (is_imported_root_constrained_kind(kind)) {
        const auto normalized_root = trim_trailing_separators(imported_output_root);
        return is_safe_import_repository_path(normalized_root) &&
               is_strict_child_repository_path(normalized_root, path);
    }
    return true;
}

[[nodiscard]] std::string asset_key_from_imported_path(std::string_view imported_path) {
    const auto extension = extension_view(imported_path);
    if (extension.empty()) {
        return std::string{imported_path};
    }
    return std::string{imported_path.substr(0U, imported_path.size() - extension.size())};
}

[[nodiscard]] bool existing_source_reuse_target_valid(const EditorAssetImportExistingSourceRow& existing,
                                                      AssetKind candidate_kind, std::string_view imported_output_root) {
    return is_safe_import_repository_path(existing.imported_path) &&
           asset_key_from_imported_path(existing.imported_path) == existing.asset_key.value &&
           is_valid_reviewed_imported_path(candidate_kind, imported_output_root, existing.imported_path);
}

[[nodiscard]] std::string parent_path_with_separator(std::string_view path) {
    const auto slash = path.find_last_of("/\\");
    if (slash == std::string_view::npos) {
        return {};
    }
    return std::string{path.substr(0U, slash + 1U)};
}

[[nodiscard]] std::string append_imported_path_suffix(std::string_view imported_path, int suffix) {
    const auto filename = filename_view(imported_path);
    const auto extension = extension_view(imported_path);
    const auto stem_size = filename.size() - extension.size();
    std::string candidate = parent_path_with_separator(imported_path);
    candidate.append(filename.substr(0U, stem_size));
    candidate.push_back('_');
    candidate.append(std::to_string(suffix));
    candidate.append(extension);
    return candidate;
}

[[nodiscard]] std::string next_imported_path_suggestion(std::string_view imported_path,
                                                        const std::unordered_set<std::string>& occupied_paths,
                                                        const std::unordered_set<std::string>& occupied_asset_keys) {
    if (imported_path.empty()) {
        return {};
    }
    for (int suffix = 2; suffix <= 99; ++suffix) {
        auto candidate = append_imported_path_suffix(imported_path, suffix);
        if (!occupied_paths.contains(candidate) &&
            !occupied_asset_keys.contains(asset_key_from_imported_path(candidate))) {
            return candidate;
        }
    }
    return {};
}

[[nodiscard]] std::string candidate_row_id(std::size_t index, std::string_view source_path) {
    return "asset_import.candidate." + std::to_string(index) + "." + to_lower_ascii(stem_from_path(source_path));
}

[[nodiscard]] bool has_line_separator_or_control(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        const auto byte = static_cast<unsigned char>(character);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] bool ends_with(std::string_view value, std::string_view suffix) noexcept {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

[[nodiscard]] bool is_safe_import_repository_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || path.find(':') != std::string_view::npos ||
        path.find('\\') != std::string_view::npos || path.find('=') != std::string_view::npos ||
        path.find(';') != std::string_view::npos || has_line_separator_or_control(path)) {
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

[[nodiscard]] bool is_safe_source_registry_path(std::string_view path) noexcept {
    return is_safe_import_repository_path(path) && ends_with(path, ".geassets");
}

void block_row(EditorAssetImportCandidateRow& row, std::string diagnostic) {
    row.can_register = false;
    row.can_import = false;
    row.status_label = "blocked";
    row.diagnostic = std::move(diagnostic);
}

void assign_rename_suggestion(EditorAssetImportCandidateRow& row, std::unordered_set<std::string>& occupied_paths,
                              std::unordered_set<std::string>& occupied_asset_keys) {
    if (!row.suggested_imported_path.empty() || row.imported_path.empty()) {
        return;
    }

    auto suggestion = next_imported_path_suggestion(row.imported_path, occupied_paths, occupied_asset_keys);
    if (suggestion.empty()) {
        return;
    }

    row.suggested_imported_path = std::move(suggestion);
    row.suggested_asset_key = AssetKeyV2{asset_key_from_imported_path(row.suggested_imported_path)};
    occupied_paths.insert(row.suggested_imported_path);
    occupied_asset_keys.insert(row.suggested_asset_key.value);
}

void block_row_for_collision(EditorAssetImportCandidateRow& row, std::string diagnostic,
                             std::unordered_set<std::string>& occupied_paths,
                             std::unordered_set<std::string>& occupied_asset_keys) {
    assign_rename_suggestion(row, occupied_paths, occupied_asset_keys);
    if (row.suggested_imported_path.empty() && !row.imported_path.empty()) {
        block_row(row, "asset_import_rename_suggestion_exhausted");
        return;
    }
    block_row(row, std::move(diagnostic));
}

[[nodiscard]] SourceAssetRegistrationRequest
make_registration_request(const EditorAssetImportReviewRequest& review_request,
                          const EditorAssetImportCandidateRow& row, std::string source_registry_content) {
    SourceAssetRegistrationRequest request;
    request.source_registry_path = review_request.source_registry_path;
    request.source_registry_content = std::move(source_registry_content);
    request.asset_key = row.asset_key;
    request.asset_kind = row.asset_kind;
    request.source_path = row.source_path;
    request.source_format = row.source_format;
    request.imported_path = row.imported_path;
    request.dependency_rows = {};
    return request;
}

[[nodiscard]] AssetImportAction make_import_action(const EditorAssetImportCandidateRow& row) {
    return AssetImportAction{
        .id = row.asset,
        .kind = row.action_kind,
        .source_path = row.source_path,
        .output_path = row.imported_path,
        .dependencies = {},
        .preset_metadata = row.preset_metadata,
    };
}

[[nodiscard]] SourceAssetRegistryRowV1 make_source_registry_row(const EditorAssetImportCandidateRow& row) {
    return SourceAssetRegistryRowV1{
        .key = row.asset_key,
        .kind = row.asset_kind,
        .source_path = row.source_path,
        .source_format = row.source_format,
        .imported_path = row.imported_path,
        .dependencies = {},
    };
}

[[nodiscard]] bool source_registry_row_equals(const SourceAssetRegistryRowV1& lhs,
                                              const SourceAssetRegistryRowV1& rhs) {
    return lhs.key.value == rhs.key.value && lhs.kind == rhs.kind && lhs.source_path == rhs.source_path &&
           lhs.source_format == rhs.source_format && lhs.imported_path == rhs.imported_path &&
           lhs.dependencies == rhs.dependencies;
}

[[nodiscard]] const SourceAssetRegistryRowV1*
find_source_registry_row_by_key(const SourceAssetRegistryDocumentV1& document, const AssetKeyV2& key) noexcept {
    const auto it = std::ranges::find_if(
        document.assets, [&key](const SourceAssetRegistryRowV1& row) { return row.key.value == key.value; });
    return it == document.assets.end() ? nullptr : &(*it);
}

[[nodiscard]] const SourceAssetRegistryRowV1*
find_source_registry_row_by_source_path(const SourceAssetRegistryDocumentV1& document,
                                        std::string_view source_path) noexcept {
    const auto it = std::ranges::find_if(
        document.assets, [source_path](const SourceAssetRegistryRowV1& row) { return row.source_path == source_path; });
    return it == document.assets.end() ? nullptr : &(*it);
}

[[nodiscard]] const SourceAssetRegistryRowV1*
find_source_registry_row_by_imported_path(const SourceAssetRegistryDocumentV1& document,
                                          std::string_view imported_path) noexcept {
    const auto it = std::ranges::find_if(document.assets, [imported_path](const SourceAssetRegistryRowV1& row) {
        return row.imported_path == imported_path;
    });
    return it == document.assets.end() ? nullptr : &(*it);
}

struct ExistingSourceContentHashMatches {
    const EditorAssetImportExistingSourceRow* first{nullptr};
    std::size_t count{0};
};

[[nodiscard]] ExistingSourceContentHashMatches
find_existing_sources_by_content_hash(std::span<const EditorAssetImportExistingSourceRow> rows,
                                      std::string_view content_hash) noexcept {
    ExistingSourceContentHashMatches matches;
    if (content_hash.empty()) {
        return matches;
    }
    for (const auto& row : rows) {
        if (row.source_content_hash == content_hash) {
            if (matches.first == nullptr) {
                matches.first = &row;
            }
            ++matches.count;
        }
    }
    return matches;
}

[[nodiscard]] bool provenance_rows_match(const EditorAssetBrowserLegalProvenanceRow& lhs,
                                         const EditorAssetBrowserLegalProvenanceRow& rhs) noexcept {
    return lhs.id == rhs.id && lhs.asset_key_label == rhs.asset_key_label && lhs.source_url == rhs.source_url &&
           lhs.retrieved_date == rhs.retrieved_date && lhs.version_or_commit == rhs.version_or_commit &&
           lhs.copyright_holder == rhs.copyright_holder && lhs.license_id == rhs.license_id &&
           lhs.modification_status == rhs.modification_status && lhs.distribution_target == rhs.distribution_target &&
           lhs.notice_complete == rhs.notice_complete && lhs.external_engine_material == rhs.external_engine_material;
}

[[nodiscard]] bool parse_existing_source_registry(std::string_view content,
                                                  SourceAssetRegistryDocumentV1& document) noexcept {
    if (content.empty()) {
        document = {};
        return true;
    }

    try {
        document = deserialize_source_asset_registry_document(content);
        return true;
    } catch (const std::exception&) {
        document = {};
        return false;
    }
}

[[nodiscard]] std::string reviewed_row_id(std::string_view prefix, AssetId asset) {
    return std::string{prefix} + "." + std::to_string(asset.value);
}

[[nodiscard]] const AssetImportAction* find_import_action(const AssetImportPlan& plan, AssetId asset) noexcept {
    const auto it =
        std::ranges::find_if(plan.actions, [asset](const AssetImportAction& action) { return action.id == asset; });
    return it == plan.actions.end() ? nullptr : &(*it);
}

[[nodiscard]] const SourceAssetRegistryRowV1*
find_source_registry_row_by_asset(const SourceAssetRegistryDocumentV1& document, AssetId asset) noexcept {
    const auto it = std::ranges::find_if(document.assets, [asset](const SourceAssetRegistryRowV1& row) {
        return asset_id_from_key_v2(row.key) == asset;
    });
    return it == document.assets.end() ? nullptr : &(*it);
}

[[nodiscard]] AssetKeyV2 fallback_key_for_action(const AssetImportAction& action) {
    return AssetKeyV2{asset_key_from_imported_path(action.output_path)};
}

[[nodiscard]] EditorAssetImportReviewedActionRow make_reviewed_action_row(std::string_view prefix,
                                                                          const AssetImportAction& action,
                                                                          AssetKeyV2 key, bool selected,
                                                                          bool dependency_expanded) {
    return EditorAssetImportReviewedActionRow{
        .id = reviewed_row_id(prefix, action.id),
        .asset_key = std::move(key),
        .asset = action.id,
        .action_kind = action.kind,
        .source_path = action.source_path,
        .output_path = action.output_path,
        .status_label = "ready",
        .diagnostic = {},
        .selected = selected,
        .dependency_expanded = dependency_expanded,
        .stale = true,
        .can_reimport = true,
        .can_recook = true,
    };
}

[[nodiscard]] bool recook_request_less(const AssetHotReloadRecookRequest& lhs,
                                       const AssetHotReloadRecookRequest& rhs) noexcept {
    if (lhs.ready_tick != rhs.ready_tick) {
        return lhs.ready_tick < rhs.ready_tick;
    }
    if (lhs.trigger_path != rhs.trigger_path) {
        return lhs.trigger_path < rhs.trigger_path;
    }
    return lhs.asset.value < rhs.asset.value;
}

[[nodiscard]] std::unordered_map<AssetId, std::vector<AssetId>, AssetIdHash>
make_dependents_by_dependency(const AssetImportPlan& plan) {
    std::unordered_map<AssetId, std::vector<AssetId>, AssetIdHash> dependents;
    for (const auto& edge : plan.dependencies) {
        dependents[edge.dependency].push_back(edge.asset);
    }
    for (auto& [_, assets] : dependents) {
        std::ranges::sort(assets, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
        assets.erase(std::ranges::unique(assets).begin(), assets.end());
    }
    return dependents;
}

struct PlannedRecookAsset {
    AssetId asset;
    AssetId source_asset;
    std::string trigger_path;
    bool selected{false};
    bool dependency_expanded{false};
};

[[nodiscard]] AssetHotReloadRecookRequest
make_reviewed_recook_request(const PlannedRecookAsset& planned, const AssetImportAction& action,
                             const EditorAssetReimportReviewRequest& request) {
    return AssetHotReloadRecookRequest{
        .asset = planned.asset,
        .source_asset = planned.source_asset,
        .trigger_path = planned.trigger_path.empty() ? action.source_path : planned.trigger_path,
        .trigger_event_kind = AssetHotReloadEventKind::modified,
        .reason = planned.dependency_expanded ? AssetHotReloadRecookReason::dependency_invalidated
                                              : AssetHotReloadRecookReason::source_modified,
        .previous_revision = request.previous_revision,
        .current_revision = request.current_revision,
        .ready_tick = request.ready_tick,
    };
}

[[nodiscard]] bool valid_content_hash(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool valid_review_recook_event_kind(AssetHotReloadEventKind kind) noexcept {
    switch (kind) {
    case AssetHotReloadEventKind::added:
    case AssetHotReloadEventKind::modified:
    case AssetHotReloadEventKind::removed:
        return true;
    case AssetHotReloadEventKind::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_review_recook_reason(AssetHotReloadRecookReason reason) noexcept {
    switch (reason) {
    case AssetHotReloadRecookReason::source_added:
    case AssetHotReloadRecookReason::source_modified:
    case AssetHotReloadRecookReason::source_removed:
    case AssetHotReloadRecookReason::dependency_invalidated:
        return true;
    case AssetHotReloadRecookReason::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_review_recook_path(std::string_view path) noexcept {
    return !path.empty() && path.find('\n') == std::string_view::npos && path.find('\r') == std::string_view::npos &&
           path.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool is_valid_review_recook_request(const AssetHotReloadRecookRequest& request) noexcept {
    return request.asset.value != 0 && request.source_asset.value != 0 &&
           valid_review_recook_path(request.trigger_path) &&
           valid_review_recook_event_kind(request.trigger_event_kind) && valid_review_recook_reason(request.reason);
}

[[nodiscard]] std::unordered_map<AssetId, EditorAssetRecookContentHashRow, AssetIdHash>
make_content_hashes_by_asset(std::span<const EditorAssetRecookContentHashRow> rows) {
    std::unordered_map<AssetId, EditorAssetRecookContentHashRow, AssetIdHash> hashes;
    hashes.reserve(rows.size());
    for (const auto& row : rows) {
        const auto asset = asset_id_from_key_v2(row.asset_key);
        if (asset.value == 0 || !valid_content_hash(row.source_content_hash) ||
            !valid_content_hash(row.output_content_hash)) {
            continue;
        }
        hashes.emplace(asset, row);
    }
    return hashes;
}

[[nodiscard]] EditorAssetImportReviewedActionRow make_recook_hash_row(const AssetImportAction& action,
                                                                      const EditorAssetRecookContentHashRow& hash) {
    const bool stale = hash.source_content_hash != hash.output_content_hash;
    return EditorAssetImportReviewedActionRow{
        .id = reviewed_row_id("asset_import.recook", action.id),
        .asset_key = hash.asset_key,
        .asset = action.id,
        .action_kind = action.kind,
        .source_path = action.source_path,
        .output_path = action.output_path,
        .source_content_hash = hash.source_content_hash,
        .output_content_hash = hash.output_content_hash,
        .status_label = stale ? "stale" : "current",
        .diagnostic = stale ? "source and output content hashes differ" : "source and output content hashes match",
        .selected = false,
        .dependency_expanded = false,
        .stale = stale,
        .can_reimport = stale,
        .can_recook = stale,
    };
}

[[nodiscard]] std::unordered_set<std::uint64_t> make_selected_asset_values(std::span<const AssetKeyV2> keys) {
    std::unordered_set<std::uint64_t> selected;
    selected.reserve(keys.size());
    for (const auto& key : keys) {
        const auto asset = asset_id_from_key_v2(key);
        if (asset.value != 0) {
            selected.insert(asset.value);
        }
    }
    return selected;
}

} // namespace

AssetImportActionKind editor_asset_import_action_kind_for_asset_kind(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::texture:
        return AssetImportActionKind::texture;
    case AssetKind::mesh:
        return AssetImportActionKind::mesh;
    case AssetKind::audio:
        return AssetImportActionKind::audio;
    case AssetKind::material:
        return AssetImportActionKind::material;
    case AssetKind::scene:
        return AssetImportActionKind::scene;
    case AssetKind::unknown:
    case AssetKind::morph_mesh_cpu:
    case AssetKind::animation_float_clip:
    case AssetKind::animation_quaternion_clip:
    case AssetKind::sprite_animation:
    case AssetKind::skinned_mesh:
    case AssetKind::script:
    case AssetKind::shader:
    case AssetKind::ui_atlas:
    case AssetKind::tilemap:
    case AssetKind::physics_collision_scene:
    case AssetKind::environment_profile:
    case AssetKind::environment_preset_pack:
    case AssetKind::mavg_cluster_graph:
        break;
    }
    return AssetImportActionKind::unknown;
}

AssetKind editor_asset_import_asset_kind_for_source_path(std::string_view path) noexcept {
    const auto extension = to_lower_ascii(extension_view(path));
    if (extension == ".texture" || extension == ".png") {
        return AssetKind::texture;
    }
    if (extension == ".mesh" || extension == ".gltf" || extension == ".glb") {
        return AssetKind::mesh;
    }
    if (extension == ".audio_source" || extension == ".wav" || extension == ".mp3" || extension == ".flac") {
        return AssetKind::audio;
    }
    if (extension == ".material") {
        return AssetKind::material;
    }
    if (extension == ".scene") {
        return AssetKind::scene;
    }
    return AssetKind::unknown;
}

std::string editor_asset_import_source_format_for_path(std::string_view path) {
    const auto kind = editor_asset_import_asset_kind_for_source_path(path);
    return std::string{expected_source_asset_format_v1(kind)};
}

std::string editor_asset_import_output_path_for_source_path(std::string_view output_root, std::string_view source_path,
                                                            AssetKind kind) {
    if (kind == AssetKind::material || kind == AssetKind::scene) {
        return std::string{source_path};
    }

    const auto extension = output_extension_for_kind(kind);
    if (extension.empty()) {
        return {};
    }

    auto root = trim_trailing_separators(output_root);
    if (root.empty()) {
        root = "assets/imported";
    }
    return root + "/" + sanitize_import_stem(stem_from_path(source_path)) + extension;
}

EditorAssetImportReviewModel review_editor_asset_import_candidates(const EditorAssetImportReviewRequest& request) {
    EditorAssetImportReviewModel model;
    model.rows.reserve(request.sources.size());

    SourceAssetRegistryDocumentV1 planned_registry;
    const bool source_registry_content_valid =
        parse_existing_source_registry(request.source_registry_content, planned_registry);
    std::string planned_registry_content = source_registry_content_valid && !request.source_registry_content.empty()
                                               ? serialize_source_asset_registry_document(planned_registry)
                                               : request.source_registry_content;
    const bool source_registry_path_safe = is_safe_source_registry_path(request.source_registry_path);

    for (std::size_t index = 0; index < request.sources.size(); ++index) {
        const auto& source = request.sources[index];
        EditorAssetImportCandidateRow row;
        row.id = candidate_row_id(index, source.source_path);
        row.source_path = source.source_path;
        row.asset_kind = editor_asset_import_asset_kind_for_source_path(source.source_path);
        row.action_kind = editor_asset_import_action_kind_for_asset_kind(row.asset_kind);
        row.source_format = editor_asset_import_source_format_for_path(source.source_path);
        row.imported_path = source.reviewed_imported_path.empty()
                                ? editor_asset_import_output_path_for_source_path(request.imported_output_root,
                                                                                  source.source_path, row.asset_kind)
                                : source.reviewed_imported_path;
        row.source_content_hash = source.source_content_hash;
        row.asset_key = AssetKeyV2{asset_key_from_imported_path(row.imported_path)};
        row.asset = asset_id_from_key_v2(row.asset_key);
        row.status_label = "ready";
        row.reuse_existing_source_selected = source.reuse_existing_source;

        if (row.asset_kind == AssetKind::unknown || row.action_kind == AssetImportActionKind::unknown ||
            row.source_format.empty() || row.imported_path.empty()) {
            block_row(row, "unsupported_import_source");
        } else if (!is_safe_import_repository_path(row.source_path)) {
            block_row(row, "unsafe_import_source_path");
        } else if (!is_safe_import_repository_path(row.imported_path)) {
            block_row(row, "unsafe_import_target_path");
        } else if (!source.reviewed_imported_path.empty() &&
                   !is_valid_reviewed_imported_path(row.asset_kind, request.imported_output_root,
                                                    source.reviewed_imported_path)) {
            block_row(row, "invalid_reviewed_imported_path");
        } else if (!source_registry_path_safe) {
            block_row(row, "unsafe_source_registry_path");
        } else if (!source.source_exists) {
            block_row(row, "missing_import_source");
        } else {
            const auto legal_review = review_editor_asset_browser_legal_provenance(source.provenance);
            if (legal_review.blocked || !legal_review.accepted_for_package) {
                row.blocked_by_legal = true;
                block_row(row, "asset_import_provenance_blocked");
            } else {
                const auto preset_review =
                    review_asset_import_preset_for_asset(request.import_presets, row.asset_key, row.asset_kind);
                row.preset_metadata = preset_review.metadata;
                if (!preset_review.ready) {
                    row.blocked_by_preset = true;
                    block_row(row, "asset_import_preset_unsupported");
                } else {
                    row.can_register = true;
                    row.can_import = true;
                }
            }
        }

        model.rows.push_back(std::move(row));
    }

    std::unordered_map<std::string, std::size_t> source_path_counts;
    std::unordered_map<std::string, std::size_t> imported_path_counts;
    std::unordered_map<std::string, std::size_t> asset_key_counts;
    std::unordered_map<std::string, std::size_t> content_hash_counts;
    source_path_counts.reserve(model.rows.size());
    imported_path_counts.reserve(model.rows.size());
    asset_key_counts.reserve(model.rows.size());
    content_hash_counts.reserve(model.rows.size());

    std::unordered_set<std::string> occupied_imported_paths;
    occupied_imported_paths.reserve(planned_registry.assets.size() + model.rows.size());
    std::unordered_set<std::string> occupied_asset_keys;
    occupied_asset_keys.reserve(planned_registry.assets.size() + model.rows.size());
    for (const auto& existing : planned_registry.assets) {
        if (!existing.imported_path.empty()) {
            occupied_imported_paths.insert(existing.imported_path);
        }
        if (!existing.key.value.empty()) {
            occupied_asset_keys.insert(existing.key.value);
        }
    }
    for (const auto& row : model.rows) {
        if (!row.source_path.empty()) {
            ++source_path_counts[row.source_path];
        }
        if (!row.imported_path.empty()) {
            ++imported_path_counts[row.imported_path];
            occupied_imported_paths.insert(row.imported_path);
        }
        if (!row.asset_key.value.empty()) {
            ++asset_key_counts[row.asset_key.value];
            occupied_asset_keys.insert(row.asset_key.value);
        }
        if (!row.source_content_hash.empty()) {
            ++content_hash_counts[row.source_content_hash];
        }
    }

    for (std::size_t index = 0; index < model.rows.size(); ++index) {
        auto& row = model.rows[index];
        if (!row.diagnostic.empty()) {
            continue;
        }

        if (const auto existing_matches =
                find_existing_sources_by_content_hash(request.existing_sources, row.source_content_hash);
            existing_matches.count > 0U) {
            if (existing_matches.count > 1U) {
                block_row(row, "duplicate_import_content_hash_ambiguous");
                continue;
            }

            const auto& existing = *existing_matches.first;
            row.reusable_existing_source_path = existing.source_path;
            row.reusable_existing_imported_path = existing.imported_path;
            row.reusable_existing_asset_key = existing.asset_key;
            if (!row.reuse_existing_source_selected) {
                block_row(row, "duplicate_import_content_hash");
                continue;
            }
            if (!existing_source_reuse_target_valid(existing, row.asset_kind, request.imported_output_root)) {
                block_row(row, "duplicate_import_content_hash_existing_source_invalid");
                continue;
            }
            if (!provenance_rows_match(request.sources[index].provenance, existing.provenance)) {
                block_row(row, "duplicate_import_content_hash_provenance_mismatch");
                continue;
            }

            row.can_register = false;
            row.can_import = false;
            row.reuses_existing_source = true;
            continue;
        }

        if (const auto count = content_hash_counts.find(row.source_content_hash);
            count != content_hash_counts.end() && count->second > 1U) {
            block_row(row, "duplicate_import_content_hash");
            continue;
        }

        if (const auto count = source_path_counts.find(row.source_path);
            (count != source_path_counts.end() && count->second > 1U) ||
            find_source_registry_row_by_source_path(planned_registry, row.source_path) != nullptr) {
            block_row(row, "duplicate_import_source_path");
            continue;
        }

        if (const auto count = imported_path_counts.find(row.imported_path);
            count != imported_path_counts.end() && count->second > 1U) {
            block_row_for_collision(row, "duplicate_import_target_path", occupied_imported_paths, occupied_asset_keys);
            continue;
        }

        if (const auto count = asset_key_counts.find(row.asset_key.value);
            (count != asset_key_counts.end() && count->second > 1U) ||
            find_source_registry_row_by_key(planned_registry, row.asset_key) != nullptr) {
            block_row_for_collision(row, "duplicate_import_asset_key", occupied_imported_paths, occupied_asset_keys);
            continue;
        }

        if (find_source_registry_row_by_imported_path(planned_registry, row.imported_path) != nullptr) {
            block_row_for_collision(row, "duplicate_imported_path", occupied_imported_paths, occupied_asset_keys);
        }
    }

    std::vector<SourceAssetRegistrationRequest> registration_requests;
    AssetImportPlan import_plan;
    for (auto& row : model.rows) {
        if (row.reuses_existing_source) {
            continue;
        }
        if (row.can_register && row.can_import) {
            if (!source_registry_content_valid) {
                block_row(row, "source_registry_preflight_failed");
            } else {
                auto candidate_registry = planned_registry;
                const auto source_registry_row = make_source_registry_row(row);
                bool registry_preflight_ok = true;
                if (const auto* existing = find_source_registry_row_by_key(candidate_registry, row.asset_key);
                    existing != nullptr) {
                    registry_preflight_ok = source_registry_row_equals(*existing, source_registry_row);
                } else {
                    candidate_registry.assets.push_back(source_registry_row);
                }

                if (registry_preflight_ok && validate_source_asset_registry_document(candidate_registry).empty()) {
                    registration_requests.push_back(make_registration_request(request, row, planned_registry_content));
                    import_plan.actions.push_back(make_import_action(row));
                    planned_registry = std::move(candidate_registry);
                    planned_registry_content = serialize_source_asset_registry_document(planned_registry);
                } else {
                    block_row(row, "source_registry_preflight_failed");
                }
            }
        }

        if (!row.diagnostic.empty()) {
            model.diagnostics.push_back(row.diagnostic);
        }
    }

    if (model.diagnostics.empty()) {
        model.registration_requests = std::move(registration_requests);
        model.import_plan = std::move(import_plan);
    }

    model.ready = !model.rows.empty() && model.diagnostics.empty();
    return model;
}

EditorAssetImportReviewedActionPlan
review_editor_asset_reimport_request(const EditorAssetReimportReviewRequest& request) {
    EditorAssetImportReviewedActionPlan plan;
    plan.command_id = "asset_browser.import.reimport_selected";
    plan.executes_import_tools = true;

    if (request.selected_asset_keys.empty()) {
        plan.diagnostics.push_back("asset reimport requires at least one selected asset key");
        return plan;
    }
    if (request.previous_revision == 0 || request.current_revision == 0) {
        plan.diagnostics.push_back("asset reimport requires non-zero source revisions");
        return plan;
    }

    const auto dependents_by_dependency = make_dependents_by_dependency(request.import_plan);
    std::deque<PlannedRecookAsset> queue;
    std::unordered_map<AssetId, PlannedRecookAsset, AssetIdHash> planned_by_asset;
    planned_by_asset.reserve(request.selected_asset_keys.size());

    for (const auto& key : request.selected_asset_keys) {
        const auto asset = asset_id_from_key_v2(key);
        const auto* action = find_import_action(request.import_plan, asset);
        if (asset.value == 0 || action == nullptr) {
            plan.diagnostics.push_back("selected asset key has no import action: " + key.value);
            continue;
        }

        PlannedRecookAsset planned{
            .asset = asset,
            .source_asset = asset,
            .trigger_path = action->source_path,
            .selected = true,
            .dependency_expanded = false,
        };
        if (planned_by_asset.emplace(asset, planned).second) {
            queue.push_back(std::move(planned));
        }
    }

    while (request.include_dependency_closure && !queue.empty()) {
        const auto source = queue.front();
        queue.pop_front();
        const auto dependents = dependents_by_dependency.find(source.asset);
        if (dependents == dependents_by_dependency.end()) {
            continue;
        }

        for (const auto dependent : dependents->second) {
            if (planned_by_asset.find(dependent) != planned_by_asset.end()) {
                continue;
            }
            PlannedRecookAsset planned{
                .asset = dependent,
                .source_asset = source.source_asset,
                .trigger_path = source.trigger_path,
                .selected = false,
                .dependency_expanded = true,
            };
            planned_by_asset.emplace(dependent, planned);
            queue.push_back(std::move(planned));
        }
    }

    std::vector<PlannedRecookAsset> planned_assets;
    planned_assets.reserve(planned_by_asset.size());
    for (const auto& [_, planned] : planned_by_asset) {
        planned_assets.push_back(planned);
    }
    std::ranges::sort(planned_assets, [](const PlannedRecookAsset& lhs, const PlannedRecookAsset& rhs) {
        if (lhs.dependency_expanded != rhs.dependency_expanded) {
            return !lhs.dependency_expanded;
        }
        return lhs.asset.value < rhs.asset.value;
    });

    for (const auto& planned : planned_assets) {
        const auto* action = find_import_action(request.import_plan, planned.asset);
        if (action == nullptr) {
            plan.diagnostics.push_back("planned asset has no import action");
            continue;
        }
        const auto* registry_row = find_source_registry_row_by_asset(request.source_registry, planned.asset);
        const AssetKeyV2 key = registry_row != nullptr ? registry_row->key : fallback_key_for_action(*action);
        plan.rows.push_back(make_reviewed_action_row("asset_import.reimport", *action, key, planned.selected,
                                                     planned.dependency_expanded));
        plan.recook_requests.push_back(make_reviewed_recook_request(planned, *action, request));
    }

    std::ranges::sort(plan.recook_requests, recook_request_less);
    try {
        plan.import_plan = build_asset_recook_plan(request.import_plan, plan.recook_requests);
    } catch (const std::exception& error) {
        plan.diagnostics.push_back(std::string{"asset reimport recook plan failed: "} + error.what());
    }

    plan.ready = !plan.recook_requests.empty() && plan.diagnostics.empty();
    return plan;
}

EditorAssetImportReviewedActionPlan review_editor_asset_recook_request(const EditorAssetRecookReviewRequest& request) {
    EditorAssetImportReviewedActionPlan plan;
    plan.command_id = "asset_browser.import.recook_stale";
    plan.executes_import_tools = true;

    const auto hashes_by_asset = make_content_hashes_by_asset(request.content_hash_rows);
    for (const auto& recook_request : request.ready_recook_requests) {
        const auto* action = find_import_action(request.import_plan, recook_request.asset);
        if (action == nullptr) {
            plan.diagnostics.push_back("ready recook request has no import action");
            continue;
        }
        if (!is_valid_review_recook_request(recook_request)) {
            EditorAssetImportReviewedActionRow row =
                make_reviewed_action_row("asset_import.recook", *action, fallback_key_for_action(*action), false,
                                         recook_request.reason == AssetHotReloadRecookReason::dependency_invalidated);
            row.status_label = "blocked";
            row.diagnostic = "invalid runtime recook request";
            row.stale = false;
            row.can_reimport = false;
            row.can_recook = false;
            plan.rows.push_back(std::move(row));
            plan.diagnostics.push_back("invalid runtime recook request");
            continue;
        }
        const auto hash = hashes_by_asset.find(recook_request.asset);
        if (hash == hashes_by_asset.end()) {
            EditorAssetImportReviewedActionRow row =
                make_reviewed_action_row("asset_import.recook", *action, fallback_key_for_action(*action), false,
                                         recook_request.reason == AssetHotReloadRecookReason::dependency_invalidated);
            row.status_label = "blocked";
            row.diagnostic = "missing reviewed content hashes";
            row.stale = false;
            row.can_reimport = false;
            row.can_recook = false;
            plan.rows.push_back(std::move(row));
            plan.diagnostics.push_back("ready recook request is missing reviewed content hashes");
            continue;
        }

        auto row = make_recook_hash_row(*action, hash->second);
        row.dependency_expanded = recook_request.reason == AssetHotReloadRecookReason::dependency_invalidated;
        if (row.stale) {
            plan.recook_requests.push_back(recook_request);
        }
        plan.rows.push_back(std::move(row));
    }

    std::ranges::sort(plan.rows,
                      [](const EditorAssetImportReviewedActionRow& lhs, const EditorAssetImportReviewedActionRow& rhs) {
                          if (lhs.output_path != rhs.output_path) {
                              return lhs.output_path < rhs.output_path;
                          }
                          return lhs.asset.value < rhs.asset.value;
                      });
    std::ranges::sort(plan.recook_requests, recook_request_less);

    if (plan.recook_requests.empty() && plan.diagnostics.empty()) {
        plan.diagnostics.push_back("no stale asset recook requests are ready");
    }

    if (!plan.recook_requests.empty()) {
        try {
            plan.import_plan = build_asset_recook_plan(request.import_plan, plan.recook_requests);
        } catch (const std::exception& error) {
            plan.diagnostics.push_back(std::string{"asset recook plan failed: "} + error.what());
        }
    }

    plan.ready = !plan.recook_requests.empty() && plan.diagnostics.empty();
    return plan;
}

EditorAssetImportReviewedActionPlan
review_editor_asset_hot_reload_stage_request(const EditorAssetHotReloadStageReviewRequest& request) {
    EditorAssetImportReviewedActionPlan plan;
    plan.command_id = "asset_browser.import.stage_hot_reload";
    plan.mutates_project_files = false;
    plan.executes_import_tools = false;

    const auto selected_assets = make_selected_asset_values(request.selected_asset_keys);
    for (const auto& result : request.staged_results) {
        if (result.kind != AssetHotReloadApplyResultKind::staged || result.asset.value == 0) {
            plan.diagnostics.push_back("hot reload stage review accepts only staged runtime replacements");
            continue;
        }
        const bool selected = selected_assets.empty() || selected_assets.contains(result.asset.value);
        if (!selected) {
            continue;
        }

        plan.runtime_stage_pending = true;
        plan.staged_results.push_back(result);
        plan.rows.push_back(EditorAssetImportReviewedActionRow{
            .id = reviewed_row_id("asset_import.hot_reload", result.asset),
            .asset_key = AssetKeyV2{result.path},
            .asset = result.asset,
            .action_kind = AssetImportActionKind::unknown,
            .source_path = {},
            .output_path = result.path,
            .status_label = "staged",
            .diagnostic = "runtime replacement staged until caller-owned safe point",
            .selected = selected,
            .dependency_expanded = false,
            .stale = true,
            .can_reimport = false,
            .can_recook = false,
        });
        if (request.commit_at_safe_point) {
            plan.safe_point_assets.push_back(result.asset);
        }
    }

    std::ranges::sort(plan.safe_point_assets, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
    plan.safe_point_assets.erase(std::ranges::unique(plan.safe_point_assets).begin(), plan.safe_point_assets.end());
    plan.commits_runtime_replacements = request.commit_at_safe_point && !plan.safe_point_assets.empty();
    if (!plan.runtime_stage_pending) {
        plan.diagnostics.push_back("no staged runtime replacements are ready for hot reload");
    } else if (request.commit_at_safe_point && plan.safe_point_assets.empty()) {
        plan.diagnostics.push_back("safe point commit has no selected staged assets");
    }
    plan.ready = plan.runtime_stage_pending && plan.diagnostics.empty();
    return plan;
}

} // namespace mirakana::editor
