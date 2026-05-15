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

    void fail_read_text(bool value) noexcept {
        fail_read_text_ = value;
    }

  private:
    mirakana::MemoryFileSystem filesystem_;
    mutable int read_text_count_{0};
    bool fail_read_text_{false};
};

[[nodiscard]] mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2 make_candidate() {
    return mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
        .package_index_path = "runtime/packages/main.geindex",
        .content_root = "runtime",
        .label = "packages/main",
    };
}

[[nodiscard]] mirakana::runtime::RuntimePackageCandidateResidentMountDescV2
make_mount_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2 mount_id) {
    return mirakana::runtime::RuntimePackageCandidateResidentMountDescV2{
        .candidate = make_candidate(),
        .mount_id = mount_id,
        .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
        .budget = {},
    };
}

void write_package(CountingFileSystem& filesystem, mirakana::AssetId asset, mirakana::AssetKind kind,
                   std::string_view payload) {
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = kind,
                                                                      .path = "textures/player.texture",
                                                                      .content = std::string(payload),
                                                                      .source_revision = 11,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text("runtime/textures/player.texture", payload);
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_loaded_package(mirakana::AssetId asset,
                                                                         std::string_view content) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
        .asset = asset,
        .kind = mirakana::AssetKind::texture,
        .path = "runtime/textures/base.texture",
        .content_hash = asset.value + 1U,
        .source_revision = 1,
        .dependencies = {},
        .content = std::string(content),
    }});
}

} // namespace

MK_TEST("runtime package candidate resident mount loads selected candidate and refreshes cache") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";
    write_package(filesystem, texture, mirakana::AssetKind::texture, payload);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;

    const auto result = mirakana::runtime::commit_runtime_package_candidate_resident_mount_v2(
        filesystem, mount_set, catalog_cache,
        make_mount_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::mounted);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.resident_mount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.invoked_candidate_load);
    MK_REQUIRE(result.invoked_catalog_refresh);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(result.loaded_record_count == 1);
    MK_REQUIRE(result.loaded_resident_bytes == payload.size());
    MK_REQUIRE(result.projected_resident_bytes == payload.size());
    MK_REQUIRE(result.previous_mount_count == 0);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(result.mount_generation == mount_set.generation());
    MK_REQUIRE(result.mount_generation != result.previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4});
    MK_REQUIRE(mount_set.mounts()[0].label == "packages/main");
    MK_REQUIRE(catalog_cache.has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture).has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package candidate resident mount rejects invalid and duplicate mount ids before reads") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
                       .label = "base",
                       .package = make_loaded_package(texture, "base"),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    MK_REQUIRE(catalog_cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                            mirakana::runtime::RuntimeResourceResidencyBudgetV2{})
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid_id = mirakana::runtime::commit_runtime_package_candidate_resident_mount_v2(
        filesystem, mount_set, catalog_cache, make_mount_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{}));
    const auto duplicate_id = mirakana::runtime::commit_runtime_package_candidate_resident_mount_v2(
        filesystem, mount_set, catalog_cache,
        make_mount_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7}));

    MK_REQUIRE(invalid_id.status == mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::invalid_mount_id);
    MK_REQUIRE(duplicate_id.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::duplicate_mount_id);
    MK_REQUIRE(!invalid_id.invoked_candidate_load);
    MK_REQUIRE(!duplicate_id.invoked_candidate_load);
    MK_REQUIRE(!invalid_id.invoked_catalog_refresh);
    MK_REQUIRE(!duplicate_id.invoked_catalog_refresh);
    MK_REQUIRE(!invalid_id.committed);
    MK_REQUIRE(!duplicate_id.committed);
    MK_REQUIRE(invalid_id.diagnostics[0].code == "invalid-mount-id");
    MK_REQUIRE(duplicate_id.diagnostics[0].code == "duplicate-mount-id");
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package candidate resident mount preserves state on candidate load failures") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/base");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_loaded_package(texture, "base"),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    MK_REQUIRE(catalog_cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                            mirakana::runtime::RuntimeResourceResidencyBudgetV2{})
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto desc = make_mount_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    desc.candidate.package_index_path = "runtime/../escape.geindex";
    const auto invalid_candidate = mirakana::runtime::commit_runtime_package_candidate_resident_mount_v2(
        filesystem, mount_set, catalog_cache, desc);

    filesystem.write_text("runtime/packages/main.geindex", "not a cooked package index");
    const auto invalid_index = mirakana::runtime::commit_runtime_package_candidate_resident_mount_v2(
        filesystem, mount_set, catalog_cache,
        make_mount_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3}));

    MK_REQUIRE(invalid_candidate.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::candidate_load_failed);
    MK_REQUIRE(invalid_candidate.candidate_load.status ==
               mirakana::runtime::RuntimePackageCandidateLoadStatusV2::invalid_candidate);
    MK_REQUIRE(invalid_index.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::candidate_load_failed);
    MK_REQUIRE(invalid_index.candidate_load.status ==
               mirakana::runtime::RuntimePackageCandidateLoadStatusV2::package_load_failed);
    MK_REQUIRE(invalid_candidate.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageCandidateResidentMountDiagnosticPhaseV2::candidate_load);
    MK_REQUIRE(invalid_index.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(invalid_candidate.invoked_candidate_load);
    MK_REQUIRE(invalid_index.invoked_candidate_load);
    MK_REQUIRE(!invalid_candidate.invoked_catalog_refresh);
    MK_REQUIRE(!invalid_index.invoked_catalog_refresh);
    MK_REQUIRE(!invalid_candidate.committed);
    MK_REQUIRE(!invalid_index.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package candidate resident mount preserves state on read exceptions") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/main.geindex", "not read");
    filesystem.fail_read_text(true);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;

    const auto result = mirakana::runtime::commit_runtime_package_candidate_resident_mount_v2(
        filesystem, mount_set, catalog_cache,
        make_mount_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}));

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::candidate_load_failed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::read_failed);
    MK_REQUIRE(result.diagnostics[0].code == "package-read-failed");
    MK_REQUIRE(result.invoked_candidate_load);
    MK_REQUIRE(!result.invoked_catalog_refresh);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.mounts().empty());
    MK_REQUIRE(!catalog_cache.has_value());
    MK_REQUIRE(filesystem.read_text_count() == 1);
}

MK_TEST("runtime package candidate resident mount preserves state on projected budget failure") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";
    write_package(filesystem, texture, mirakana::AssetKind::texture, payload);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_loaded_package(base, "base"),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    MK_REQUIRE(catalog_cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                            mirakana::runtime::RuntimeResourceResidencyBudgetV2{})
                   .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto base_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base);
    MK_REQUIRE(base_handle.has_value());
    auto desc = make_mount_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    desc.budget.max_resident_content_bytes = 4;

    const auto result = mirakana::runtime::commit_runtime_package_candidate_resident_mount_v2(filesystem, mount_set,
                                                                                              catalog_cache, desc);

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageCandidateResidentMountStatusV2::budget_failed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.resident_mount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(result.invoked_candidate_load);
    MK_REQUIRE(result.invoked_catalog_refresh);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.loaded_record_count == 1);
    MK_REQUIRE(result.projected_resident_bytes > *desc.budget.max_resident_content_bytes);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::runtime::RuntimePackageCandidateResidentMountDiagnosticPhaseV2::resident_budget);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog_cache.catalog(), *base_handle));
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture).has_value());
}

int main() {
    return mirakana::test::run_all();
}
