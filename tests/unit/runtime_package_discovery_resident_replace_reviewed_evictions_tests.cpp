// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/resource_runtime.hpp"

#include <stdexcept>
#include <string>
#include <string_view>
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

    void fail_is_directory(bool value) noexcept {
        fail_is_directory_ = value;
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
        .content_hash = asset.value + 1U,
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

void write_duplicate_asset_package(CountingFileSystem& filesystem, std::string_view index_path,
                                   std::string_view content_root, mirakana::AssetId asset,
                                   std::string_view first_payload_path, std::string_view first_payload,
                                   std::string_view second_payload_path, std::string_view second_payload) {
    const std::string index = "format=GameEngine.CookedPackageIndex.v1\n"
                              "entry.count=2\n"
                              "entry.0.asset=" +
                              std::to_string(asset.value) +
                              "\n"
                              "entry.0.kind=texture\n"
                              "entry.0.path=" +
                              std::string(first_payload_path) +
                              "\n"
                              "entry.0.content_hash=" +
                              std::to_string(mirakana::hash_asset_cooked_content(first_payload)) +
                              "\n"
                              "entry.0.source_revision=31\n"
                              "entry.0.dependencies=\n"
                              "entry.1.asset=" +
                              std::to_string(asset.value) +
                              "\n"
                              "entry.1.kind=texture\n"
                              "entry.1.path=" +
                              std::string(second_payload_path) +
                              "\n"
                              "entry.1.content_hash=" +
                              std::to_string(mirakana::hash_asset_cooked_content(second_payload)) +
                              "\n"
                              "entry.1.source_revision=32\n"
                              "entry.1.dependencies=\n"
                              "dependency.count=0\n";
    filesystem.write_text(index_path, index);
    filesystem.write_text(std::string(content_root) + "/" + std::string(first_payload_path), first_payload);
    filesystem.write_text(std::string(content_root) + "/" + std::string(second_payload_path), second_payload);
}

[[nodiscard]] mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDescV2
make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2 mount_id, std::string selected_package_index_path,
          std::uint64_t max_bytes = 4096,
          std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> eviction_candidates = {},
          std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> protected_mounts = {}) {
    return mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDescV2{
        .discovery =
            mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{
                .root = "runtime/packages",
                .content_root = "runtime",
            },
        .selected_package_index_path = std::move(selected_package_index_path),
        .mount_id = mount_id,
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .budget =
            mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                .max_resident_content_bytes = max_bytes,
                .max_resident_asset_records = {},
            },
        .eviction_candidate_unmount_order = std::move(eviction_candidates),
        .protected_mount_ids = std::move(protected_mounts),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeResidentCatalogCacheV2
make_refreshed_cache(const mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set,
                     std::uint64_t max_bytes = 4096) {
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const auto refresh =
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                              mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                                  .max_resident_content_bytes = max_bytes, .max_resident_asset_records = {}});
    MK_REQUIRE(refresh.succeeded());
    return catalog_cache;
}

} // namespace

MK_TEST(
    "runtime package discovery resident replace with reviewed evictions commits selected candidate after eviction") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto other = mirakana::AssetId::from_name("textures/other");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    const std::string replacement_payload = "new";
    write_package(filesystem, "runtime/packages/base.geindex", "runtime", other, mirakana::AssetKind::texture,
                  "textures/other.texture", "ignored");
    write_package(filesystem, "runtime/packages/replacement.geindex", "runtime", replacement,
                  mirakana::AssetKind::texture, "textures/replacement.texture", replacement_payload);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base,
                  "resident base payload");
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}, "other", other,
                  "resident other payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                  "runtime/packages/replacement.geindex", replacement_payload.size(),
                  {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}}));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::committed);
    MK_REQUIRE(result.discovery.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::discovered);
    MK_REQUIRE(result.selected_candidate.package_index_path == "runtime/packages/replacement.geindex");
    MK_REQUIRE(result.selected_candidate.content_root == "runtime");
    MK_REQUIRE(result.selected_candidate.label == "replacement");
    MK_REQUIRE(result.candidate_resident_replace.status ==
               mirakana::runtime::RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::replaced);
    MK_REQUIRE(result.eviction_plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1);
    MK_REQUIRE(result.evicted_mount_count == 1);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.loaded_record_count == 1);
    MK_REQUIRE(result.loaded_resident_bytes == replacement_payload.size());
    MK_REQUIRE(result.projected_resident_bytes == replacement_payload.size());
    MK_REQUIRE(result.previous_mount_count == 2);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(result.mount_generation != previous_mount_generation);
    MK_REQUIRE(result.invoked_discovery);
    MK_REQUIRE(result.invoked_resident_commit);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(mount_set.mounts()[0].label == "base");
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == replacement_payload);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), replacement).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), other).has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package discovery resident replace with reviewed evictions skips eviction when projected view fits") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    write_package(filesystem, "runtime/packages/replacement.geindex", "runtime", replacement,
                  mirakana::AssetKind::texture, "textures/replacement.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                  "runtime/packages/replacement.geindex", 64));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required);
    MK_REQUIRE(result.eviction_plan.steps.empty());
    MK_REQUIRE(result.evicted_mount_count == 0);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(mount_set.mounts()[0].label == "base");
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), replacement).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
}

MK_TEST("runtime package discovery resident replace with reviewed evictions rejects descriptors before scans") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    write_package(filesystem, "runtime/packages/replacement.geindex", "runtime", replacement,
                  mirakana::AssetKind::texture, "textures/replacement.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    filesystem.fail_is_directory(true);

    const auto invalid_path =
        mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}, "runtime/../escape.geindex"));
    const auto invalid_mount =
        mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{}, "runtime/packages/replacement.geindex"));
    const auto missing_mount =
        mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99},
                      "runtime/packages/replacement.geindex"));

    MK_REQUIRE(invalid_path.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::invalid_descriptor);
    MK_REQUIRE(invalid_mount.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::invalid_mount_id);
    MK_REQUIRE(missing_mount.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::missing_mount_id);
    MK_REQUIRE(!invalid_path.invoked_discovery);
    MK_REQUIRE(!invalid_mount.invoked_discovery);
    MK_REQUIRE(!missing_mount.invoked_discovery);
    MK_REQUIRE(!invalid_path.invoked_resident_commit);
    MK_REQUIRE(!invalid_mount.invoked_resident_commit);
    MK_REQUIRE(!missing_mount.invoked_resident_commit);
    MK_REQUIRE(filesystem.is_directory_count() == 0);
    MK_REQUIRE(filesystem.list_files_count() == 0);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package discovery resident replace with reviewed evictions reports missing candidate before package "
        "reads") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    write_package(filesystem, "runtime/packages/replacement.geindex", "runtime", replacement,
                  mirakana::AssetKind::texture, "textures/replacement.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "runtime/packages/missing.geindex"));

    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::candidate_not_found);
    MK_REQUIRE(result.discovery.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::discovered);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "candidate-not-found");
    MK_REQUIRE(result.invoked_discovery);
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package discovery resident replace with reviewed evictions preserves state on discovery failures") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/replacement.geindex", "not read");
    const auto base = mirakana::AssetId::from_name("textures/base");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto invalid_desc = make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                                  "runtime/packages/replacement.geindex");
    invalid_desc.discovery.root = "";
    const auto invalid =
        mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache, invalid_desc);

    filesystem.fail_list_files(true);
    const auto scan = mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                  "runtime/packages/replacement.geindex"));

    MK_REQUIRE(invalid.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::invalid_descriptor);
    MK_REQUIRE(invalid.discovery.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(invalid.diagnostics.size() == 1);
    MK_REQUIRE(invalid.diagnostics[0].code == "invalid-root");
    MK_REQUIRE(scan.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::discovery_failed);
    MK_REQUIRE(scan.discovery.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::scan_failed);
    MK_REQUIRE(scan.diagnostics.size() == 1);
    MK_REQUIRE(scan.diagnostics[0].code == "scan-failed");
    MK_REQUIRE(!invalid.invoked_resident_commit);
    MK_REQUIRE(!scan.invoked_resident_commit);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST(
    "runtime package discovery resident replace with reviewed evictions preserves state on delegated load failure") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/broken.geindex", "not a cooked package index");
    const auto base = mirakana::AssetId::from_name("textures/base");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "runtime/packages/broken.geindex"));

    MK_REQUIRE(
        result.status ==
        mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::candidate_load_failed);
    MK_REQUIRE(
        result.candidate_resident_replace.status ==
        mirakana::runtime::RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::candidate_load_failed);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(result.invoked_discovery);
    MK_REQUIRE(result.invoked_resident_commit);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 1);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package discovery resident replace with reviewed evictions rejects duplicate-asset indexes before "
        "replacement") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    write_duplicate_asset_package(filesystem, "runtime/packages/duplicate.geindex", "runtime", replacement,
                                  "textures/replacement-a.texture", "new-a", "textures/replacement-b.texture", "new-b");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                  "runtime/packages/duplicate.geindex"));

    MK_REQUIRE(
        result.status ==
        mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::candidate_load_failed);
    MK_REQUIRE(
        result.candidate_resident_replace.status ==
        mirakana::runtime::RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::candidate_load_failed);
    MK_REQUIRE(result.candidate_resident_replace.candidate_load.status ==
               mirakana::runtime::RuntimePackageCandidateLoadStatusV2::package_load_failed);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(
        result.diagnostics[0].phase ==
        mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::candidate_load);
    MK_REQUIRE(result.diagnostics[0].asset == mirakana::AssetId{});
    MK_REQUIRE(result.diagnostics[0].mount == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(result.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(result.diagnostics[0].message == "asset cooked package entry asset is duplicated");
    MK_REQUIRE(result.invoked_discovery);
    MK_REQUIRE(result.invoked_resident_commit);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 1);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), replacement).has_value());
}

MK_TEST("runtime package discovery resident replace with reviewed evictions maps reviewed eviction failures") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto other = mirakana::AssetId::from_name("textures/other");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    write_package(filesystem, "runtime/packages/replacement.geindex", "runtime", replacement,
                  mirakana::AssetKind::texture, "textures/replacement.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base,
                  "resident base payload");
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}, "other", other,
                  "resident other payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid =
        mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                      "runtime/packages/replacement.geindex", 3,
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{}}));
    const auto duplicate =
        mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                      "runtime/packages/replacement.geindex", 3,
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                       mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}}));
    const auto missing =
        mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                      "runtime/packages/replacement.geindex", 3,
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99}}));
    const auto protected_mount =
        mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                      "runtime/packages/replacement.geindex", 3,
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}},
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}}));

    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::
                                     invalid_eviction_candidate_mount_id);
    MK_REQUIRE(duplicate.status == mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::
                                       duplicate_eviction_candidate_mount_id);
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::
                                     missing_eviction_candidate_mount_id);
    MK_REQUIRE(protected_mount.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::
                   protected_eviction_candidate_mount_id);
    MK_REQUIRE(
        invalid.diagnostics[0].phase ==
        mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsDiagnosticPhaseV2::eviction_plan);
    MK_REQUIRE(duplicate.diagnostics[0].code == "duplicate-candidate-mount-id");
    MK_REQUIRE(missing.diagnostics[0].code == "missing-candidate-mount-id");
    MK_REQUIRE(protected_mount.diagnostics[0].code == "protected-candidate-mount-id");
    MK_REQUIRE(!invalid.committed);
    MK_REQUIRE(!duplicate.committed);
    MK_REQUIRE(!missing.committed);
    MK_REQUIRE(!protected_mount.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package discovery resident replace with reviewed evictions preserves state when candidates are "
        "insufficient") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto other = mirakana::AssetId::from_name("textures/other");
    const auto keep = mirakana::AssetId::from_name("textures/keep");
    const auto replacement = mirakana::AssetId::from_name("textures/replacement");
    write_package(filesystem, "runtime/packages/replacement.geindex", "runtime", replacement,
                  mirakana::AssetKind::texture, "textures/replacement.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base,
                  "resident base payload");
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}, "other", other,
                  "resident other payload");
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 8}, "keep", keep,
                  "resident keep payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                  "runtime/packages/replacement.geindex", 4,
                  {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}}));

    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentReplaceReviewedEvictionsStatusV2::budget_failed);
    MK_REQUIRE(result.candidate_resident_replace.status ==
               mirakana::runtime::RuntimePackageCandidateResidentReplaceReviewedEvictionsStatusV2::budget_failed);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 3);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), replacement).has_value());
}

int main() {
    return mirakana::test::run_all();
}
