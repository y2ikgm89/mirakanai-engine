// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/content_browser.hpp"

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/source_asset_registry.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <limits>
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

[[nodiscard]] std::size_t page_count_for(std::size_t visible_item_count, std::size_t page_size) noexcept {
    if (visible_item_count == 0U) {
        return 0U;
    }
    return ((visible_item_count - 1U) / page_size) + 1U;
}

[[nodiscard]] std::size_t last_page_offset_for(std::size_t visible_item_count, std::size_t page_size) noexcept {
    const auto page_count = page_count_for(visible_item_count, page_size);
    return page_count == 0U ? 0U : (page_count - 1U) * page_size;
}

[[nodiscard]] std::size_t clamp_page_offset(std::size_t requested_offset, std::size_t visible_item_count,
                                            std::size_t page_size, bool& clamped) noexcept {
    if (visible_item_count == 0U) {
        if (requested_offset != 0U) {
            clamped = true;
        }
        return 0U;
    }

    const auto aligned = (requested_offset / page_size) * page_size;
    const auto last_offset = last_page_offset_for(visible_item_count, page_size);
    const auto clamped_offset = std::min(aligned, last_offset);
    if (clamped_offset != requested_offset) {
        clamped = true;
    }
    return clamped_offset;
}

[[nodiscard]] std::size_t next_page_offset(std::size_t page_offset, std::size_t page_size, bool& clamped) noexcept {
    const auto max_offset = std::numeric_limits<std::size_t>::max();
    if (page_offset > max_offset - page_size) {
        clamped = true;
        return max_offset;
    }
    return page_offset + page_size;
}

[[nodiscard]] std::size_t selected_visible_index(const std::vector<ContentBrowserItem>& visible_items,
                                                 const ContentBrowserItem* selected) noexcept {
    if (selected == nullptr) {
        return std::numeric_limits<std::size_t>::max();
    }
    const auto it = std::ranges::find_if(
        visible_items, [selected](const ContentBrowserItem& item) { return item.id == selected->id; });
    return it == visible_items.end() ? std::numeric_limits<std::size_t>::max()
                                     : static_cast<std::size_t>(std::distance(visible_items.begin(), it));
}

[[nodiscard]] ContentBrowserNavigationRow make_navigation_row(const ContentBrowserItem& item, std::size_t visible_index,
                                                              bool selected) {
    return ContentBrowserNavigationRow{
        .asset = item.id,
        .kind = item.kind,
        .id = "content_browser.navigation." + std::to_string(visible_index),
        .path = item.path,
        .display_name = item.display_name,
        .asset_key_label = item.asset_key_label,
        .visible_index = visible_index,
        .selected = selected,
    };
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

ContentBrowserNavigationModel plan_content_browser_navigation(const ContentBrowserState& browser,
                                                              const ContentBrowserNavigationRequest& request) {
    auto visible_items = browser.visible_items();
    ContentBrowserNavigationModel model{
        .total_item_count = browser.item_count(),
        .visible_item_count = visible_items.size(),
        .page_offset = request.page_offset,
        .page_size = request.page_size,
        .mutates = false,
        .executes = false,
    };

    if (model.page_size == 0U) {
        model.page_size = 1U;
        model.clamped = true;
        model.diagnostics.push_back("page size must be greater than zero");
    }
    if (model.page_size > kContentBrowserNavigationMaxPageSize) {
        model.page_size = kContentBrowserNavigationMaxPageSize;
        model.clamped = true;
        model.diagnostics.push_back("page size exceeded content browser navigation maximum");
    }

    const auto selected_index = selected_visible_index(visible_items, browser.selected_asset());
    model.has_selected_visible_asset = selected_index != std::numeric_limits<std::size_t>::max();
    if (model.has_selected_visible_asset) {
        model.selected_visible_index = selected_index;
        model.selected_page_index = selected_index / model.page_size;
    }

    switch (request.action) {
    case ContentBrowserNavigationAction::stay:
        break;
    case ContentBrowserNavigationAction::first_page:
        if (model.page_offset != 0U) {
            model.clamped = true;
        }
        model.page_offset = 0U;
        break;
    case ContentBrowserNavigationAction::previous_page:
        if (model.page_offset <= model.page_size) {
            if (model.page_offset != 0U) {
                model.clamped = true;
            }
            model.page_offset = 0U;
        } else {
            model.page_offset -= model.page_size;
        }
        break;
    case ContentBrowserNavigationAction::next_page:
        model.page_offset = next_page_offset(model.page_offset, model.page_size, model.clamped);
        break;
    case ContentBrowserNavigationAction::last_page:
        model.page_offset = last_page_offset_for(model.visible_item_count, model.page_size);
        break;
    case ContentBrowserNavigationAction::selected_page:
        if (model.has_selected_visible_asset) {
            model.page_offset = model.selected_page_index * model.page_size;
        } else {
            model.diagnostics.push_back("selected asset is not visible");
        }
        break;
    }

    model.page_offset = clamp_page_offset(model.page_offset, model.visible_item_count, model.page_size, model.clamped);
    model.page_count = page_count_for(model.visible_item_count, model.page_size);
    model.page_index = model.page_count == 0U ? 0U : model.page_offset / model.page_size;
    model.has_previous_page = model.page_offset > 0U;
    model.has_next_page = model.page_count > 0U && (model.page_offset + model.page_size) < model.visible_item_count;

    if (model.visible_item_count == 0U) {
        return model;
    }

    const auto end = std::min(model.page_offset + model.page_size, model.visible_item_count);
    model.rows.reserve(end - model.page_offset);
    for (std::size_t index = model.page_offset; index < end; ++index) {
        const bool selected = model.has_selected_visible_asset && index == model.selected_visible_index;
        model.rows.push_back(make_navigation_row(visible_items[index], index, selected));
    }
    return model;
}

} // namespace mirakana::editor
