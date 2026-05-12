// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/content_browser.hpp"

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/source_asset_registry.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string display_name_from_path(std::string_view path) {
    const auto separator = path.find_last_of("/\\");
    if (separator == std::string_view::npos) {
        return std::string(path);
    }
    return std::string(path.substr(separator + 1U));
}

[[nodiscard]] std::string directory_from_path(std::string_view path) {
    const auto separator = path.find_last_of("/\\");
    if (separator == std::string_view::npos) {
        return {};
    }
    return std::string(path.substr(0, separator));
}

[[nodiscard]] char ascii_lower(char value) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
}

[[nodiscard]] bool contains_case_insensitive(std::string_view text, std::string_view needle) {
    if (needle.empty()) {
        return true;
    }
    if (needle.size() > text.size()) {
        return false;
    }

    return !std::ranges::search(text, needle, [](char lhs, char rhs) {
                return ascii_lower(lhs) == ascii_lower(rhs);
            }).empty();
}

[[nodiscard]] const AssetIdentityRowV2* find_identity_row(const AssetIdentityDocumentV2& identity, AssetId id,
                                                          AssetKind kind) noexcept {
    const auto it = std::ranges::find_if(identity.assets, [id, kind](const AssetIdentityRowV2& row) {
        return row.kind == kind && asset_id_from_key_v2(row.key) == id;
    });
    return it == identity.assets.end() ? nullptr : &*it;
}

[[nodiscard]] ContentBrowserItem make_content_browser_item(const AssetRecord& record,
                                                           const AssetIdentityRowV2* identity_row) {
    ContentBrowserItem item{
        .id = record.id,
        .kind = record.kind,
        .path = record.path,
        .display_name = display_name_from_path(record.path),
        .directory = directory_from_path(record.path),
    };
    if (identity_row != nullptr) {
        item.asset_key = identity_row->key;
        item.asset_key_label = identity_row->key.value;
        item.identity_source_path = identity_row->source_path;
        item.identity_backed = true;
    }
    return item;
}

[[nodiscard]] ContentBrowserItem make_content_browser_item(const SourceAssetRegistryRowV1& row) {
    ContentBrowserItem item{
        .id = asset_id_from_key_v2(row.key),
        .kind = row.kind,
        .path = row.imported_path,
        .display_name = display_name_from_path(row.imported_path),
        .directory = directory_from_path(row.imported_path),
        .asset_key = row.key,
        .asset_key_label = row.key.value,
        .identity_source_path = row.source_path,
        .identity_backed = true,
    };
    return item;
}

void sort_items(std::vector<ContentBrowserItem>& items) {
    std::ranges::sort(items,
                      [](const ContentBrowserItem& lhs, const ContentBrowserItem& rhs) { return lhs.path < rhs.path; });
}

void refresh_items(std::vector<ContentBrowserItem>& items, const AssetRegistry& registry,
                   const AssetIdentityDocumentV2* identity) {
    items.clear();
    for (const auto& record : registry.records()) {
        const auto* identity_row = identity == nullptr ? nullptr : find_identity_row(*identity, record.id, record.kind);
        items.push_back(make_content_browser_item(record, identity_row));
    }

    sort_items(items);
}

void refresh_items(std::vector<ContentBrowserItem>& items, const SourceAssetRegistryDocumentV1& registry) {
    items.clear();
    for (const auto& row : registry.assets) {
        items.push_back(make_content_browser_item(row));
    }

    sort_items(items);
}

} // namespace

void ContentBrowserState::refresh_from(const AssetRegistry& registry) {
    refresh_items(items_, registry, nullptr);
    clear_missing_selection();
}

void ContentBrowserState::refresh_from(const AssetRegistry& registry, const AssetIdentityDocumentV2& identity) {
    refresh_items(items_, registry, &identity);
    clear_missing_selection();
}

void ContentBrowserState::refresh_from(const SourceAssetRegistryDocumentV1& registry) {
    refresh_items(items_, registry);
    clear_missing_selection();
}

std::size_t ContentBrowserState::item_count() const noexcept {
    return items_.size();
}

const std::vector<ContentBrowserItem>& ContentBrowserState::items() const noexcept {
    return items_;
}

std::vector<ContentBrowserItem> ContentBrowserState::visible_items() const {
    std::vector<ContentBrowserItem> result;
    for (const auto& item : items_) {
        if (kind_filter_ != AssetKind::unknown && item.kind != kind_filter_) {
            continue;
        }
        if (!contains_case_insensitive(item.path, text_filter_) &&
            !contains_case_insensitive(item.display_name, text_filter_) &&
            !contains_case_insensitive(item.asset_key_label, text_filter_) &&
            !contains_case_insensitive(item.identity_source_path, text_filter_)) {
            continue;
        }
        result.push_back(item);
    }
    return result;
}

std::string_view ContentBrowserState::text_filter() const noexcept {
    return text_filter_;
}

AssetKind ContentBrowserState::kind_filter() const noexcept {
    return kind_filter_;
}

const ContentBrowserItem* ContentBrowserState::selected_asset() const noexcept {
    return find_item(selected_);
}

void ContentBrowserState::set_text_filter(std::string filter) {
    text_filter_ = std::move(filter);
}

void ContentBrowserState::set_kind_filter(AssetKind kind) noexcept {
    kind_filter_ = kind;
}

bool ContentBrowserState::select(AssetId id) noexcept {
    if (find_item(id) == nullptr) {
        return false;
    }
    selected_ = id;
    return true;
}

bool ContentBrowserState::select(const AssetKeyV2& key) noexcept {
    return select(asset_id_from_key_v2(key));
}

void ContentBrowserState::clear_selection() noexcept {
    selected_ = AssetId{};
}

const ContentBrowserItem* ContentBrowserState::find_item(AssetId id) const noexcept {
    const auto it = std::ranges::find_if(items_, [id](const ContentBrowserItem& item) { return item.id == id; });
    return it == items_.end() ? nullptr : &*it;
}

void ContentBrowserState::clear_missing_selection() noexcept {
    if (selected_.value != 0 && find_item(selected_) == nullptr) {
        selected_ = AssetId{};
    }
}

std::string_view asset_kind_label(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::texture:
        return "Texture";
    case AssetKind::mesh:
        return "Mesh";
    case AssetKind::morph_mesh_cpu:
        return "Morph Mesh (CPU)";
    case AssetKind::animation_float_clip:
        return "Animation Float Clip";
    case AssetKind::animation_quaternion_clip:
        return "Animation Quaternion Clip";
    case AssetKind::skinned_mesh:
        return "Skinned Mesh";
    case AssetKind::material:
        return "Material";
    case AssetKind::scene:
        return "Scene";
    case AssetKind::audio:
        return "Audio";
    case AssetKind::script:
        return "Script";
    case AssetKind::shader:
        return "Shader";
    case AssetKind::ui_atlas:
        return "UI Atlas";
    case AssetKind::tilemap:
        return "Tilemap";
    case AssetKind::unknown:
        break;
    }
    return "Unknown";
}

} // namespace mirakana::editor
