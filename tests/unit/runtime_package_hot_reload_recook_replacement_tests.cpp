// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_hot_reload.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class CountingFileSystem final : public mirakana::IFileSystem {
  public:
    [[nodiscard]] bool exists(std::string_view path) const override {
        return filesystem_.exists(path);
    }

    [[nodiscard]] bool is_directory(std::string_view path) const override {
        ++is_directory_count_;
        if (fail_is_directory_) {
            throw std::runtime_error("is_directory failed");
        }
        return filesystem_.is_directory(path);
    }

    [[nodiscard]] std::string read_text(std::string_view path) const override {
        ++read_text_count_;
        if (fail_read_text_) {
            throw std::runtime_error("read failed");
        }
        return filesystem_.read_text(path);
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view root) const override {
        ++list_files_count_;
        if (fail_list_files_) {
            throw std::runtime_error("list failed");
        }
        return filesystem_.list_files(root);
    }

    void write_text(std::string_view path, std::string_view text) override {
        filesystem_.write_text(path, text);
    }

    void remove(std::string_view path) override {
        filesystem_.remove(path);
    }

    void remove_empty_directory(std::string_view path) override {
        filesystem_.remove_empty_directory(path);
    }

    [[nodiscard]] int read_text_count() const noexcept {
        return read_text_count_;
    }

    [[nodiscard]] int list_files_count() const noexcept {
        return list_files_count_;
    }

    [[nodiscard]] int is_directory_count() const noexcept {
        return is_directory_count_;
    }

    void fail_list_files(bool value) noexcept {
        fail_list_files_ = value;
    }

    void fail_read_text(bool value) noexcept {
        fail_read_text_ = value;
    }

  private:
    mirakana::MemoryFileSystem filesystem_;
    mutable int read_text_count_{0};
    mutable int list_files_count_{0};
    mutable int is_directory_count_{0};
    bool fail_is_directory_{false};
    bool fail_list_files_{false};
    bool fail_read_text_{false};
};

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_loaded_package(mirakana::AssetId asset,
                                                                         mirakana::AssetKind kind,
                                                                         std::string_view path,
                                                                         std::string_view content) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
        .asset = asset,
        .kind = kind,
        .path = std::string(path),
        .content_hash = mirakana::hash_asset_cooked_content(content),
        .source_revision = 1,
        .dependencies = {},
        .content = std::string(content),
    }});
}

void mount_package(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set,
                   mirakana::runtime::RuntimeResidentPackageMountIdV2 id, std::string_view label,
                   mirakana::AssetId asset, std::string_view content) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = id,
                       .label = std::string(label),
                       .package = make_loaded_package(asset, mirakana::AssetKind::texture,
                                                      std::string(label) + ".texture", content),
                   })
                   .succeeded());
}

void write_package(CountingFileSystem& filesystem, std::string_view index_path, std::string_view content_root,
                   mirakana::AssetId asset, mirakana::AssetKind kind, std::string_view payload_path,
                   std::string_view payload, std::uint64_t source_revision = 31) {
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = kind,
                                                                      .path = std::string(payload_path),
                                                                      .content = std::string(payload),
                                                                      .source_revision = source_revision,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text(index_path, mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text(std::string(content_root) + "/" + std::string(payload_path), payload);
}

void write_package_index_only(CountingFileSystem& filesystem, std::string_view index_path, mirakana::AssetId asset,
                              mirakana::AssetKind kind, std::string_view payload_path,
                              std::string_view expected_payload) {
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = kind,
                                                                      .path = std::string(payload_path),
                                                                      .content = std::string(expected_payload),
                                                                      .source_revision = 31,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text(index_path, mirakana::serialize_asset_cooked_package_index(index));
}

[[nodiscard]] mirakana::runtime::RuntimeResidentCatalogCacheV2
make_refreshed_cache(const mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set,
                     std::uint64_t max_bytes = 4096) {
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const auto refresh =
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                              mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                                  .max_resident_content_bytes = max_bytes,
                                  .max_resident_asset_records = {},
                              });
    MK_REQUIRE(refresh.succeeded());
    return catalog_cache;
}

[[nodiscard]] mirakana::AssetHotReloadApplyResult make_recook_result(mirakana::AssetHotReloadApplyResultKind kind,
                                                                     mirakana::AssetId asset, std::string path,
                                                                     std::string diagnostic = {}) {
    return mirakana::AssetHotReloadApplyResult{
        .kind = kind,
        .asset = asset,
        .path = std::move(path),
        .requested_revision = 8,
        .active_revision = kind == mirakana::AssetHotReloadApplyResultKind::failed_rolled_back ? 7U : 9U,
        .diagnostic = std::move(diagnostic),
    };
}

[[nodiscard]] mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2 make_candidate(std::string index_path) {
    return mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
        .package_index_path = std::move(index_path),
        .content_root = "runtime",
        .label = "replacement",
    };
}

[[nodiscard]] mirakana::runtime::RuntimePackageHotReloadRecookReplacementDescV2
make_desc(mirakana::AssetId replacement, std::string selected_package_index_path) {
    return mirakana::runtime::RuntimePackageHotReloadRecookReplacementDescV2{
        .recook_apply_results =
            {
                make_recook_result(mirakana::AssetHotReloadApplyResultKind::staged, replacement,
                                   selected_package_index_path),
            },
        .candidates =
            {
                make_candidate(selected_package_index_path),
            },
        .discovery =
            mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{
                .root = "runtime/packages",
                .content_root = "runtime",
            },
        .selected_package_index_path = std::move(selected_package_index_path),
        .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
        .reviewed_existing_mount_ids =
            {
                mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
            },
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .budget =
            mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                .max_resident_content_bytes = 4096U,
                .max_resident_asset_records = {},
            },
        .eviction_candidate_unmount_order = {},
        .protected_mount_ids =
            {
                mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
            },
    };
}

} // namespace

MK_TEST("runtime package hot reload recook replacement commits reviewed selected package at safe point") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    write_package(filesystem, "runtime/packages/replacement.geindex", "runtime", replacement,
                  mirakana::AssetKind::texture, "textures/replacement.texture", "new");

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7}, "base", base, "old");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_generation = mount_set.generation();

    auto desc = make_desc(replacement, "runtime/packages/replacement.geindex");
    desc.recook_apply_results = {
        make_recook_result(mirakana::AssetHotReloadApplyResultKind::staged, replacement,
                           "runtime/textures/replacement.texture"),
    };
    const auto result = mirakana::runtime::commit_runtime_package_hot_reload_recook_replacement_v2(
        filesystem, mount_set, catalog_cache, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageHotReloadRecookReplacementStatusV2::committed);
    MK_REQUIRE(result.recook_change_review.succeeded());
    MK_REQUIRE(result.replacement_intent_review.succeeded());
    MK_REQUIRE(result.replacement_commit.succeeded());
    MK_REQUIRE(result.selected_candidate.candidate.package_index_path == "runtime/packages/replacement.geindex");
    MK_REQUIRE(result.selected_candidate_count == 1);
    MK_REQUIRE(result.loaded_record_count == 1);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(result.previous_mount_generation == previous_generation);
    MK_REQUIRE(result.mount_generation == mount_set.generation());
    MK_REQUIRE(result.mount_generation > previous_generation);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), replacement).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(result.invoked_candidate_review);
    MK_REQUIRE(result.invoked_replacement_intent_review);
    MK_REQUIRE(result.invoked_resident_commit);
    MK_REQUIRE(!result.invoked_file_watch);
    MK_REQUIRE(!result.invoked_recook);
    MK_REQUIRE(result.committed);
}

MK_TEST("runtime package hot reload recook replacement blocks failed recook rows before package reads") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7}, "base", base, "old");
    auto catalog_cache = make_refreshed_cache(mount_set);
    auto desc = make_desc(replacement, "runtime/packages/replacement.geindex");
    desc.recook_apply_results = {
        make_recook_result(mirakana::AssetHotReloadApplyResultKind::failed_rolled_back, replacement,
                           "runtime/packages/replacement.geindex", "recook failed"),
    };
    const auto previous_generation = mount_set.generation();

    const auto result = mirakana::runtime::commit_runtime_package_hot_reload_recook_replacement_v2(
        filesystem, mount_set, catalog_cache, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementStatusV2::recook_change_review_failed);
    MK_REQUIRE(!result.recook_change_review.succeeded());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementDiagnosticPhaseV2::recook_change_review);
    MK_REQUIRE(result.diagnostics[0].code == "recook-failed");
    MK_REQUIRE(!result.invoked_replacement_intent_review);
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(filesystem.list_files_count() == 0);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_generation);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
}

MK_TEST("runtime package hot reload recook replacement requires selected reviewed candidate") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7}, "base", base, "old");
    auto catalog_cache = make_refreshed_cache(mount_set);
    auto desc = make_desc(replacement, "runtime/packages/replacement.geindex");
    desc.selected_package_index_path = "runtime/packages/missing.geindex";
    const auto previous_generation = mount_set.generation();

    const auto result = mirakana::runtime::commit_runtime_package_hot_reload_recook_replacement_v2(
        filesystem, mount_set, catalog_cache, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementStatusV2::candidate_not_found);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementDiagnosticPhaseV2::candidate_selection);
    MK_REQUIRE(result.diagnostics[0].path == "runtime/packages/missing.geindex");
    MK_REQUIRE(result.diagnostics[0].code == "candidate-not-found");
    MK_REQUIRE(result.invoked_candidate_review);
    MK_REQUIRE(!result.invoked_replacement_intent_review);
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(filesystem.list_files_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_generation);
}

MK_TEST("runtime package hot reload recook replacement blocks invalid intent before discovery") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7}, "base", base, "old");
    auto catalog_cache = make_refreshed_cache(mount_set);
    auto desc = make_desc(replacement, "runtime/packages/replacement.geindex");
    desc.mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99};
    const auto previous_generation = mount_set.generation();

    const auto result = mirakana::runtime::commit_runtime_package_hot_reload_recook_replacement_v2(
        filesystem, mount_set, catalog_cache, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementStatusV2::replacement_intent_review_failed);
    MK_REQUIRE(!result.replacement_intent_review.succeeded());
    MK_REQUIRE(result.replacement_intent_review.status ==
               mirakana::runtime::RuntimePackageHotReloadReplacementIntentReviewStatusV2::missing_mount_id);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementDiagnosticPhaseV2::replacement_intent_review);
    MK_REQUIRE(result.diagnostics[0].code == "missing-mount-id");
    MK_REQUIRE(result.invoked_candidate_review);
    MK_REQUIRE(result.invoked_replacement_intent_review);
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(filesystem.list_files_count() == 0);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_generation);
}

MK_TEST("runtime package hot reload recook replacement preserves state when commit stage fails") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7}, "base", base, "old");
    auto catalog_cache = make_refreshed_cache(mount_set);
    write_package_index_only(filesystem, "runtime/packages/replacement.geindex", replacement,
                             mirakana::AssetKind::texture, "textures/replacement.texture", "new");
    auto desc = make_desc(replacement, "runtime/packages/replacement.geindex");
    const auto previous_generation = mount_set.generation();

    const auto result = mirakana::runtime::commit_runtime_package_hot_reload_recook_replacement_v2(
        filesystem, mount_set, catalog_cache, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementStatusV2::replacement_commit_failed);
    MK_REQUIRE(result.replacement_intent_review.succeeded());
    MK_REQUIRE(!result.replacement_commit.succeeded());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementDiagnosticPhaseV2::candidate_load);
    MK_REQUIRE(result.diagnostics[0].code == "package-load-failed");
    MK_REQUIRE(result.diagnostics[0].path == "runtime/textures/replacement.texture");
    MK_REQUIRE(result.invoked_candidate_review);
    MK_REQUIRE(result.invoked_replacement_intent_review);
    MK_REQUIRE(result.invoked_resident_commit);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(filesystem.list_files_count() > 0);
    MK_REQUIRE(filesystem.read_text_count() > 0);
    MK_REQUIRE(mount_set.generation() == previous_generation);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), replacement).has_value());
}

MK_TEST("runtime package hot reload recook replacement preserves state when reviewed evictions are insufficient") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    write_package(filesystem, "runtime/packages/replacement.geindex", "runtime", replacement,
                  mirakana::AssetKind::texture, "textures/replacement.texture", "replacement payload");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7}, "base", base,
                  "base payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    auto desc = make_desc(replacement, "runtime/packages/replacement.geindex");
    desc.budget.max_resident_content_bytes = 1U;
    const auto previous_generation = mount_set.generation();

    const auto result = mirakana::runtime::commit_runtime_package_hot_reload_recook_replacement_v2(
        filesystem, mount_set, catalog_cache, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementStatusV2::replacement_commit_failed);
    MK_REQUIRE(result.replacement_commit.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::budget_failed);
    MK_REQUIRE(result.replacement_commit.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable);
    MK_REQUIRE(!result.diagnostics.empty());
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementDiagnosticPhaseV2::resident_budget);
    MK_REQUIRE(result.diagnostics[0].code == "resident-content-bytes-exceed-budget");
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_generation);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), replacement).has_value());
}

int main() {
    return mirakana::test::run_all();
}
