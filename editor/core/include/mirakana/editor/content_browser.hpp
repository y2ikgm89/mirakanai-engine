// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/source_asset_registry.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

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

} // namespace mirakana::editor
