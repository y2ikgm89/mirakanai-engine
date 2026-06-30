// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/source_asset_registry.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

inline constexpr std::size_t kContentBrowserNavigationMaxPageSize = 200U;

struct ContentBrowserItem {
    AssetId id;
    AssetKind kind{AssetKind::unknown};
    std::string path;
    std::string display_name;
    std::string directory;
    AssetKeyV2 asset_key;
    std::string asset_key_label;
    std::string identity_source_path;
    bool identity_backed{false};
};

enum class ContentBrowserNavigationAction : std::uint8_t {
    stay,
    first_page,
    previous_page,
    next_page,
    last_page,
    selected_page,
};

struct ContentBrowserNavigationRequest {
    std::size_t page_offset{0U};
    std::size_t page_size{100U};
    ContentBrowserNavigationAction action{ContentBrowserNavigationAction::stay};
};

struct ContentBrowserNavigationRow {
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string id;
    std::string path;
    std::string display_name;
    std::string asset_key_label;
    std::size_t visible_index{0U};
    bool selected{false};
};

struct ContentBrowserNavigationModel {
    std::size_t total_item_count{0U};
    std::size_t visible_item_count{0U};
    std::size_t page_offset{0U};
    std::size_t page_size{100U};
    std::size_t page_index{0U};
    std::size_t page_count{0U};
    std::size_t selected_visible_index{0U};
    std::size_t selected_page_index{0U};
    bool has_previous_page{false};
    bool has_next_page{false};
    bool has_selected_visible_asset{false};
    bool clamped{false};
    bool mutates{false};
    bool executes{false};
    std::vector<ContentBrowserNavigationRow> rows;
    std::vector<std::string> diagnostics;
};

class ContentBrowserState {
  public:
    void refresh_from(const AssetRegistry& registry);
    void refresh_from(const AssetRegistry& registry, const AssetIdentityDocumentV2& identity);
    void refresh_from(const SourceAssetRegistryDocumentV1& registry);

    [[nodiscard]] std::size_t item_count() const noexcept;
    [[nodiscard]] const std::vector<ContentBrowserItem>& items() const noexcept;
    [[nodiscard]] std::vector<ContentBrowserItem> visible_items() const;
    [[nodiscard]] std::string_view text_filter() const noexcept;
    [[nodiscard]] AssetKind kind_filter() const noexcept;
    [[nodiscard]] const ContentBrowserItem* selected_asset() const noexcept;

    void set_text_filter(std::string filter);
    void set_kind_filter(AssetKind kind) noexcept;
    [[nodiscard]] bool select(AssetId id) noexcept;
    [[nodiscard]] bool select(const AssetKeyV2& key) noexcept;
    void clear_selection() noexcept;

  private:
    [[nodiscard]] const ContentBrowserItem* find_item(AssetId id) const noexcept;
    void clear_missing_selection() noexcept;

    std::vector<ContentBrowserItem> items_;
    std::string text_filter_;
    AssetKind kind_filter_{AssetKind::unknown};
    AssetId selected_;
};

[[nodiscard]] std::string_view asset_kind_label(AssetKind kind) noexcept;
[[nodiscard]] ContentBrowserNavigationModel
plan_content_browser_navigation(const ContentBrowserState& browser, const ContentBrowserNavigationRequest& request);

} // namespace mirakana::editor
