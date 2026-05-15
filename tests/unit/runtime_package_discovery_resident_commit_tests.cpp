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

void write_package(CountingFileSystem& filesystem, std::string_view index_path, std::string_view content_root,
                   mirakana::AssetId asset, mirakana::AssetKind kind, std::string_view payload_path,
                   std::string_view payload, std::uint64_t source_revision = 17) {
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

[[nodiscard]] mirakana::runtime::RuntimePackageDiscoveryResidentCommitDescV2
make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2 mode,
          mirakana::runtime::RuntimeResidentPackageMountIdV2 mount_id, std::string selected_package_index_path,
          std::uint64_t max_bytes = 4096) {
    return mirakana::runtime::RuntimePackageDiscoveryResidentCommitDescV2{
        .discovery =
            mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{
                .root = "runtime/packages",
                .content_root = "runtime",
            },
        .selected_package_index_path = std::move(selected_package_index_path),
        .mode = mode,
        .mount_id = mount_id,
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .budget =
            mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                .max_resident_content_bytes = max_bytes,
                .max_resident_asset_records = {},
            },
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

MK_TEST("runtime package discovery resident commit mounts selected discovered candidate") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/base");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";
    write_package(filesystem, "runtime/packages/base.geindex", "runtime", material, mirakana::AssetKind::material,
                  "materials/base.material", "base material");
    write_package(filesystem, "runtime/packages/characters/player.geindex", "runtime", texture,
                  mirakana::AssetKind::texture, "textures/player.texture", payload);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::mount,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
                  "runtime/packages/characters/player.geindex"));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::committed);
    MK_REQUIRE(result.discovery.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::discovered);
    MK_REQUIRE(result.selected_candidate.package_index_path == "runtime/packages/characters/player.geindex");
    MK_REQUIRE(result.selected_candidate.content_root == "runtime");
    MK_REQUIRE(result.selected_candidate.label == "characters/player");
    MK_REQUIRE(result.resident_mount.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::mounted);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.loaded_record_count == 1);
    MK_REQUIRE(result.loaded_resident_bytes == payload.size());
    MK_REQUIRE(result.projected_resident_bytes == payload.size());
    MK_REQUIRE(result.previous_mount_count == 0);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(result.invoked_discovery);
    MK_REQUIRE(result.invoked_resident_commit);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7});
    MK_REQUIRE(mount_set.mounts()[0].label == "characters/player");
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == payload);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package discovery resident commit replaces selected discovered candidate") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, "runtime/packages/characters/player.geindex", "runtime", texture,
                  mirakana::AssetKind::texture, "textures/player.texture", "replacement texture", 29);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                       .label = "old",
                       .package = make_loaded_package(texture, mirakana::AssetKind::texture, "old", "old texture"),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::replace,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                  "runtime/packages/characters/player.geindex"));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::committed);
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimePackageCandidateResidentReplaceStatusV2::replaced);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.previous_mount_count == 1);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(mount_set.generation() != previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3});
    MK_REQUIRE(mount_set.mounts()[0].label == "old");
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "replacement texture");
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package discovery resident commit rejects mount ids before discovery or reads") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, "runtime/packages/characters/player.geindex", "runtime", texture,
                  mirakana::AssetKind::texture, "textures/player.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
                       .label = "base",
                       .package = make_loaded_package(texture, mirakana::AssetKind::texture, "base", "base"),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::mount,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{}, "runtime/packages/characters/player.geindex"));
    const auto duplicate = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::mount,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
                  "runtime/packages/characters/player.geindex"));
    const auto missing = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::replace,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9},
                  "runtime/packages/characters/player.geindex"));

    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::invalid_mount_id);
    MK_REQUIRE(duplicate.status ==
               mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::duplicate_mount_id);
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::missing_mount_id);
    MK_REQUIRE(invalid.diagnostics.size() == 1);
    MK_REQUIRE(duplicate.diagnostics.size() == 1);
    MK_REQUIRE(missing.diagnostics.size() == 1);
    MK_REQUIRE(invalid.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_mount);
    MK_REQUIRE(duplicate.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_mount);
    MK_REQUIRE(missing.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::resident_replace);
    MK_REQUIRE(!invalid.invoked_discovery);
    MK_REQUIRE(!duplicate.invoked_discovery);
    MK_REQUIRE(!missing.invoked_discovery);
    MK_REQUIRE(!invalid.invoked_resident_commit);
    MK_REQUIRE(!duplicate.invoked_resident_commit);
    MK_REQUIRE(!missing.invoked_resident_commit);
    MK_REQUIRE(filesystem.list_files_count() == 0);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package discovery resident commit reports missing candidate before package reads") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, "runtime/packages/characters/player.geindex", "runtime", texture,
                  mirakana::AssetKind::texture, "textures/player.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::mount,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6}, "runtime/packages/missing.geindex"));

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::candidate_not_found);
    MK_REQUIRE(result.discovery.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::discovered);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "candidate-not-found");
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(mount_set.mounts().empty());
    MK_REQUIRE(!catalog_cache.has_value());
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package discovery resident commit reports invalid discovery descriptor before scanning") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/characters/player.geindex", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    auto desc = make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::mount,
                          mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 12},
                          "runtime/packages/characters/player.geindex");
    desc.discovery.root = "";

    const auto result =
        mirakana::runtime::commit_runtime_package_discovery_resident_v2(filesystem, mount_set, catalog_cache, desc);

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::invalid_descriptor);
    MK_REQUIRE(result.discovery.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageDiscoveryResidentCommitDiagnosticPhaseV2::descriptor);
    MK_REQUIRE(result.diagnostics[0].code == "invalid-root");
    MK_REQUIRE(result.invoked_discovery);
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(mount_set.mounts().empty());
    MK_REQUIRE(!catalog_cache.has_value());
    MK_REQUIRE(filesystem.list_files_count() == 0);
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package discovery resident commit preserves state on discovery scan failure") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/characters/player.geindex", "not read");
    filesystem.fail_list_files(true);
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                       .label = "base",
                       .package = make_loaded_package(texture, mirakana::AssetKind::texture, "base", "base"),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::mount,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 8},
                  "runtime/packages/characters/player.geindex"));

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::discovery_failed);
    MK_REQUIRE(result.discovery.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::scan_failed);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "scan-failed");
    MK_REQUIRE(!result.invoked_resident_commit);
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package discovery resident commit preserves state on delegated failures") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, "runtime/packages/characters/player.geindex", "runtime", texture,
                  mirakana::AssetKind::texture, "textures/player.texture",
                  "candidate content that exceeds the projected resident budget");
    filesystem.write_text("runtime/packages/broken.geindex", "not a cooked package index");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                       .label = "base",
                       .package = make_loaded_package(texture, mirakana::AssetKind::texture, "base", "base"),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto budget = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::mount,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 10},
                  "runtime/packages/characters/player.geindex", 8));
    const auto load = mirakana::runtime::commit_runtime_package_discovery_resident_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimePackageDiscoveryResidentCommitModeV2::mount,
                  mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 11}, "runtime/packages/broken.geindex"));

    MK_REQUIRE(budget.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::budget_failed);
    MK_REQUIRE(budget.invoked_resident_commit);
    MK_REQUIRE(budget.resident_mount.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::budget_failed);
    MK_REQUIRE(budget.diagnostics.size() == 1);
    MK_REQUIRE(budget.diagnostics[0].code == "resident-content-bytes-exceed-budget");
    MK_REQUIRE(load.status == mirakana::runtime::RuntimePackageDiscoveryResidentCommitStatusV2::candidate_load_failed);
    MK_REQUIRE(load.invoked_resident_commit);
    MK_REQUIRE(load.resident_mount.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::candidate_load_failed);
    MK_REQUIRE(load.diagnostics.size() == 1);
    MK_REQUIRE(load.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

int main() {
    return mirakana::test::run_all();
}
