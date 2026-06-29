// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/asset_import_review.hpp"

#include "mirakana/assets/source_asset_registry.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace mirakana::editor {
namespace {

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

[[nodiscard]] std::string asset_key_from_imported_path(std::string_view imported_path) {
    const auto extension = extension_view(imported_path);
    if (extension.empty()) {
        return std::string{imported_path};
    }
    return std::string{imported_path.substr(0U, imported_path.size() - extension.size())};
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
        row.imported_path = editor_asset_import_output_path_for_source_path(request.imported_output_root,
                                                                            source.source_path, row.asset_kind);
        row.asset_key = AssetKeyV2{asset_key_from_imported_path(row.imported_path)};
        row.asset = asset_id_from_key_v2(row.asset_key);
        row.status_label = "ready";

        if (row.asset_kind == AssetKind::unknown || row.action_kind == AssetImportActionKind::unknown ||
            row.source_format.empty() || row.imported_path.empty()) {
            block_row(row, "unsupported_import_source");
        } else if (!is_safe_import_repository_path(row.source_path)) {
            block_row(row, "unsafe_import_source_path");
        } else if (!is_safe_import_repository_path(row.imported_path)) {
            block_row(row, "unsafe_import_target_path");
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
                row.can_register = true;
                row.can_import = true;
            }
        }

        model.rows.push_back(std::move(row));
    }

    std::unordered_map<std::string, std::size_t> output_path_counts;
    output_path_counts.reserve(model.rows.size());
    for (const auto& row : model.rows) {
        if (!row.imported_path.empty()) {
            ++output_path_counts[row.imported_path];
        }
    }
    for (auto& row : model.rows) {
        const auto count = output_path_counts.find(row.imported_path);
        if (count != output_path_counts.end() && count->second > 1U && !row.blocked_by_legal &&
            row.diagnostic != "unsupported_import_source") {
            block_row(row, "duplicate_import_target_path");
        }
    }

    std::vector<SourceAssetRegistrationRequest> registration_requests;
    AssetImportPlan import_plan;
    for (auto& row : model.rows) {
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

} // namespace mirakana::editor
