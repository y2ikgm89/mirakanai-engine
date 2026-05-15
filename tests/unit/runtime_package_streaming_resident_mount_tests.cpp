// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <cstdint>
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

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord make_record(mirakana::AssetId asset, mirakana::AssetKind kind,
                                                                mirakana::runtime::RuntimeAssetHandle handle,
                                                                std::string content) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset,
        .kind = kind,
        .path = "assets/" + std::to_string(asset.value) + ".geasset",
        .content_hash = asset.value + handle.value,
        .source_revision = handle.value,
        .dependencies = {},
        .content = std::move(content),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_package(mirakana::runtime::RuntimeAssetRecord record) {
    return mirakana::runtime::RuntimeAssetPackage({std::move(record)});
}

void write_package(CountingFileSystem& filesystem, mirakana::AssetId asset, mirakana::AssetKind kind,
                   std::string_view path, std::string_view payload, std::uint64_t source_revision = 11) {
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = kind,
                                                                      .path = std::string(path),
                                                                      .content = std::string(payload),
                                                                      .source_revision = source_revision,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/game.geindex", mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text("runtime/" + std::string(path), payload);
}

[[nodiscard]] mirakana::runtime::RuntimePackageStreamingExecutionDesc make_valid_desc(std::uint64_t budget_bytes) {
    return mirakana::runtime::RuntimePackageStreamingExecutionDesc{
        .target_id = "packaged-scene-streaming",
        .package_index_path = "runtime/game.geindex",
        .content_root = "runtime",
        .runtime_scene_validation_target_id = "packaged-scene",
        .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
        .resident_budget_bytes = budget_bytes,
        .safe_point_required = true,
        .runtime_scene_validation_succeeded = true,
        .required_preload_assets = {},
        .resident_resource_kinds = {},
        .max_resident_packages = 0,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeResidentCatalogCacheV2
make_refreshed_cache(const mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set,
                     mirakana::runtime::RuntimePackageMountOverlay overlay =
                         mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                     mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget = {}) {
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    MK_REQUIRE(catalog_cache.refresh(mount_set, overlay, budget).succeeded());
    return catalog_cache;
}

} // namespace

MK_TEST("runtime package streaming resident mount commit mounts package and refreshes resident catalog") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {texture, material};
    desc.resident_resource_kinds = {mirakana::AssetKind::texture, mirakana::AssetKind::material};
    desc.max_resident_packages = 1;

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(texture, mirakana::AssetKind::texture, mirakana::runtime::RuntimeAssetHandle{.value = 1},
                        "replacement texture"),
            make_record(material, mirakana::AssetKind::material, mirakana::runtime::RuntimeAssetHandle{.value = 2},
                        "replacement material"),
        }),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, std::move(loaded_package));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.resident_mount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.resident_catalog_refresh.succeeded());
    MK_REQUIRE(result.resident_catalog_refresh.invoked_catalog_build);
    MK_REQUIRE(result.resident_catalog_refresh.mount_generation == mount_set.generation());
    MK_REQUIRE(result.resident_package_count == 1);
    MK_REQUIRE(result.resident_mount_generation == mount_set.generation());
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime package streaming resident mount commit rejects duplicate mount id before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{};
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = make_package(make_record(material, mirakana::AssetKind::material,
                                            mirakana::runtime::RuntimeAssetHandle{.value = 2}, "material")),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
        std::move(loaded_package));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_mount_failed);
    MK_REQUIRE(result.resident_mount.status ==
               mirakana::runtime::RuntimeResidentPackageMountStatusV2::duplicate_mount_id);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "duplicate-mount-id");
    MK_REQUIRE(result.resident_mount_generation == previous_mount_generation);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime package streaming resident mount commit rejects duplicate records before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{};
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(material, mirakana::AssetKind::material, mirakana::runtime::RuntimeAssetHandle{.value = 2},
                        "material"),
            make_record(material, mirakana::AssetKind::material, mirakana::runtime::RuntimeAssetHandle{.value = 3},
                        "duplicate material"),
        }),
        .failures = {},
    };

    auto desc = make_valid_desc(4096);
    desc.max_resident_packages = 2;

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, std::move(loaded_package));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::catalog_build_failed);
    MK_REQUIRE(result.resident_catalog_refresh.invoked_catalog_build);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "catalog-build-failed");
    MK_REQUIRE(result.resident_mount_generation == previous_mount_generation);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime package streaming resident mount commit preserves catalog on projected budget failure") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto cached_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture);
    MK_REQUIRE(cached_handle.has_value());

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = make_package(make_record(material, mirakana::AssetKind::material,
                                            mirakana::runtime::RuntimeAssetHandle{.value = 2},
                                            "material content that exceeds the projected resident budget")),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(8), std::move(loaded_package));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.resident_package_count == 2);
    MK_REQUIRE(result.estimated_resident_bytes > result.resident_budget_bytes);
    MK_REQUIRE(result.resident_mount_generation == previous_mount_generation);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(mirakana::runtime::is_runtime_resource_handle_live_v2(catalog_cache.catalog(), *cached_handle));
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime package streaming candidate resident mount loads selected package and refreshes resident catalog") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/base");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", payload, 19);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(material, mirakana::AssetKind::material,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {texture};
    desc.resident_resource_kinds = {mirakana::AssetKind::texture};
    desc.max_resident_packages = 2;

    const auto result =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.candidate_load.candidate.package_index_path == "runtime/game.geindex");
    MK_REQUIRE(result.candidate_load.candidate.content_root == "runtime");
    MK_REQUIRE(result.candidate_load.candidate.label == "packaged-scene-streaming");
    MK_REQUIRE(result.candidate_load.loaded_record_count == 1);
    MK_REQUIRE(result.candidate_load.estimated_resident_bytes == payload.size());
    MK_REQUIRE(result.resident_mount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.resident_package_count == 2);
    MK_REQUIRE(result.resident_mount_generation == mount_set.generation());
    MK_REQUIRE(result.resident_mount_generation != previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() != previous_catalog_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(mount_set.mounts()[1].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(mount_set.mounts()[1].label == "packaged-scene-streaming");
    MK_REQUIRE(mount_set.mounts()[1].package.records()[0].content == payload);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package streaming candidate resident mount rejects invalid and duplicate ids before reads") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096));
    const auto duplicate =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096));

    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_mount_failed);
    MK_REQUIRE(duplicate.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_mount_failed);
    MK_REQUIRE(invalid.resident_mount.status ==
               mirakana::runtime::RuntimeResidentPackageMountStatusV2::invalid_mount_id);
    MK_REQUIRE(duplicate.resident_mount.status ==
               mirakana::runtime::RuntimeResidentPackageMountStatusV2::duplicate_mount_id);
    MK_REQUIRE(!invalid.candidate_load.invoked_load);
    MK_REQUIRE(!duplicate.candidate_load.invoked_load);
    MK_REQUIRE(invalid.diagnostics[0].code == "invalid-mount-id");
    MK_REQUIRE(duplicate.diagnostics[0].code == "duplicate-mount-id");
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident mount preserves state on candidate load failure") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/game.geindex", "not a cooked package index");
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096));

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::package_load_failed);
    MK_REQUIRE(result.candidate_load.status ==
               mirakana::runtime::RuntimePackageCandidateLoadStatusV2::package_load_failed);
    MK_REQUIRE(result.candidate_load.invoked_load);
    MK_REQUIRE(result.candidate_load.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(result.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident mount preserves state on residency hint failure") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", "candidate");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6},
                       .label = "base",
                       .package = make_package(make_record(material, mirakana::AssetKind::material,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {material};
    desc.max_resident_packages = 2;

    const auto result =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc);

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "preload-asset-missing");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident mount preserves state on projected budget failure") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture",
                  "candidate content that exceeds the projected resident budget");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 8},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_mount_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(8));

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(!result.diagnostics.empty());
    MK_REQUIRE(result.diagnostics[0].code == "resident-budget-intent-exceeded");
    MK_REQUIRE(result.estimated_resident_bytes > result.resident_budget_bytes);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident mount with reviewed evictions commits after eviction") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    const std::string incoming_payload = "new";
    write_package(filesystem, incoming, mirakana::AssetKind::texture, "textures/incoming.texture", incoming_payload);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(base, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1},
                                                           "resident base payload")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    auto desc = make_valid_desc(incoming_payload.size());
    desc.required_preload_assets = {incoming};
    desc.resident_resource_kinds = {mirakana::AssetKind::texture};
    desc.max_resident_packages = 1;

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}}, {});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.eviction_plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1);
    MK_REQUIRE(result.invoked_eviction_plan);
    MK_REQUIRE(result.evicted_mount_count == 1);
    MK_REQUIRE(result.resident_mount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.estimated_resident_bytes == incoming_payload.size());
    MK_REQUIRE(result.resident_package_count == 1);
    MK_REQUIRE(result.resident_mount_generation != previous_mount_generation);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == incoming_payload);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package streaming candidate resident mount with reviewed evictions skips eviction when projected view "
        "fits") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, incoming, mirakana::AssetKind::texture, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(base, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    auto desc = make_valid_desc(64);
    desc.max_resident_packages = 2;

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, {}, {});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required);
    MK_REQUIRE(result.evicted_mount_count == 0);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
}

MK_TEST("runtime package streaming candidate resident mount with reviewed evictions rejects descriptors and ids before "
        "reads") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto invalid_desc = make_valid_desc(4096);
    invalid_desc.target_id.clear();
    const auto invalid_descriptor = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, invalid_desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5}}, {});

    auto missing_preflight = make_valid_desc(4096);
    missing_preflight.runtime_scene_validation_succeeded = false;
    const auto validation_required = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, missing_preflight,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5}}, {});

    const auto invalid_id = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5}}, {});
    const auto duplicate_id = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5}}, {});

    MK_REQUIRE(invalid_descriptor.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::invalid_descriptor);
    MK_REQUIRE(validation_required.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required);
    MK_REQUIRE(invalid_id.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_mount_failed);
    MK_REQUIRE(duplicate_id.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_mount_failed);
    MK_REQUIRE(!invalid_descriptor.candidate_load.invoked_load);
    MK_REQUIRE(!validation_required.candidate_load.invoked_load);
    MK_REQUIRE(!invalid_id.candidate_load.invoked_load);
    MK_REQUIRE(!duplicate_id.candidate_load.invoked_load);
    MK_REQUIRE(!invalid_id.invoked_eviction_plan);
    MK_REQUIRE(!duplicate_id.invoked_eviction_plan);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident mount with reviewed evictions preserves state on candidate load "
        "failure") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/game.geindex", "not a cooked package index");
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3}}, {});

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::package_load_failed);
    MK_REQUIRE(result.candidate_load.status ==
               mirakana::runtime::RuntimePackageCandidateLoadStatusV2::package_load_failed);
    MK_REQUIRE(result.candidate_load.invoked_load);
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident mount with reviewed evictions preserves state on residency hint "
        "failure") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", "candidate");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6},
                       .label = "base",
                       .package = make_package(make_record(material, mirakana::AssetKind::material,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {material};
    desc.max_resident_packages = 2;

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, {}, {});

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.diagnostics[0].code == "preload-asset-missing");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident mount with reviewed evictions rejects reviewed eviction "
        "candidates") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, incoming, mirakana::AssetKind::texture, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(base, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1},
                                                           "resident base payload")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto desc = make_valid_desc(3);

    const auto invalid = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{}}, {});
    const auto duplicate = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
             mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}},
            {});
    const auto missing = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99}}, {});
    const auto protected_new = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}}, {});

    MK_REQUIRE(invalid.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed);
    MK_REQUIRE(duplicate.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed);
    MK_REQUIRE(missing.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed);
    MK_REQUIRE(protected_new.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed);
    MK_REQUIRE(invalid.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::invalid_candidate_mount_id);
    MK_REQUIRE(duplicate.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::duplicate_candidate_mount_id);
    MK_REQUIRE(missing.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::missing_candidate_mount_id);
    MK_REQUIRE(protected_new.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id);
    MK_REQUIRE(!invalid.committed);
    MK_REQUIRE(!duplicate.committed);
    MK_REQUIRE(!missing.committed);
    MK_REQUIRE(!protected_new.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST(
    "runtime package streaming candidate resident mount with reviewed evictions preserves state when candidates are "
    "insufficient") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto other = mirakana::AssetId::from_name("textures/other");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, incoming, mirakana::AssetKind::texture, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(base, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1},
                                                           "resident base payload")),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                       .label = "other",
                       .package = make_package(make_record(other, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 2},
                                                           "resident other payload")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_mount_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4),
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}}, {});

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
}

MK_TEST("runtime package streaming resident replace commit replaces mounted package and refreshes resident catalog") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto stale_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture);
    MK_REQUIRE(stale_handle.has_value());

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {texture};
    desc.resident_resource_kinds = {mirakana::AssetKind::texture};
    desc.max_resident_packages = 1;

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                            mirakana::runtime::RuntimeAssetHandle{.value = 2}, "base v2")),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_resident_replace_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, std::move(loaded_package));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::replaced);
    MK_REQUIRE(result.resident_replace.catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.resident_package_count == 1);
    MK_REQUIRE(result.resident_mount_generation == mount_set.generation());
    MK_REQUIRE(result.resident_mount_generation != previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() != previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog_cache.catalog(), *stale_handle));
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9});
    MK_REQUIRE(mount_set.mounts()[0].label == "base");
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v2");
}

MK_TEST("runtime package streaming resident replace commit rejects invalid and missing ids before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    mirakana::runtime::RuntimeAssetPackageLoadResult invalid_loaded_package{
        .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                            mirakana::runtime::RuntimeAssetHandle{.value = 2}, "invalid")),
        .failures = {},
    };
    const auto invalid = mirakana::runtime::execute_selected_runtime_package_streaming_resident_replace_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
        std::move(invalid_loaded_package));

    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_replace_failed);
    MK_REQUIRE(invalid.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id);
    MK_REQUIRE(!invalid.resident_replace.invoked_candidate_catalog_build);
    MK_REQUIRE(invalid.diagnostics.size() == 1);
    MK_REQUIRE(invalid.diagnostics[0].code == "invalid-mount-id");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);

    mirakana::runtime::RuntimeAssetPackageLoadResult missing_loaded_package{
        .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                            mirakana::runtime::RuntimeAssetHandle{.value = 3}, "missing")),
        .failures = {},
    };
    const auto missing = mirakana::runtime::execute_selected_runtime_package_streaming_resident_replace_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
        std::move(missing_loaded_package));

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_replace_failed);
    MK_REQUIRE(missing.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id);
    MK_REQUIRE(!missing.resident_replace.invoked_candidate_catalog_build);
    MK_REQUIRE(missing.diagnostics.size() == 1);
    MK_REQUIRE(missing.diagnostics[0].code == "missing-mount-id");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming resident replace commit preserves state on candidate catalog failure") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(texture, mirakana::AssetKind::texture, mirakana::runtime::RuntimeAssetHandle{.value = 2},
                        "base v2"),
            make_record(texture, mirakana::AssetKind::texture, mirakana::runtime::RuntimeAssetHandle{.value = 3},
                        "duplicate"),
        }),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_resident_replace_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
        std::move(loaded_package));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed);
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::catalog_build_failed);
    MK_REQUIRE(result.resident_replace.invoked_candidate_catalog_build);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "catalog-build-failed");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming resident replace commit preserves state on projected budget failure") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                            mirakana::runtime::RuntimeAssetHandle{.value = 2},
                                            "replacement content that exceeds the projected resident budget")),
        .failures = {},
    };

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_resident_replace_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(8), std::move(loaded_package));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::budget_failed);
    MK_REQUIRE(result.resident_replace.invoked_candidate_catalog_build);
    MK_REQUIRE(result.resident_replace.catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(!result.resident_replace.catalog_refresh.invoked_catalog_build);
    MK_REQUIRE(!result.diagnostics.empty());
    MK_REQUIRE(result.diagnostics[0].code == "resident-budget-intent-exceeded");
    MK_REQUIRE(result.estimated_resident_bytes > result.resident_budget_bytes);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace loads selected package and preserves mount slot") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", payload, 17);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto stale_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture);
    MK_REQUIRE(stale_handle.has_value());

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {texture};
    desc.resident_resource_kinds = {mirakana::AssetKind::texture};
    desc.max_resident_packages = 1;

    const auto result =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_replace_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.candidate_load.candidate.package_index_path == "runtime/game.geindex");
    MK_REQUIRE(result.candidate_load.candidate.content_root == "runtime");
    MK_REQUIRE(result.candidate_load.candidate.label == "packaged-scene-streaming");
    MK_REQUIRE(result.candidate_load.loaded_record_count == 1);
    MK_REQUIRE(result.candidate_load.estimated_resident_bytes == payload.size());
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::replaced);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.resident_package_count == 1);
    MK_REQUIRE(result.resident_mount_generation == mount_set.generation());
    MK_REQUIRE(result.resident_mount_generation != previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() != previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog_cache.catalog(), *stale_handle));
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 9});
    MK_REQUIRE(mount_set.mounts()[0].label == "base");
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == payload);
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package streaming candidate resident replace rejects invalid and missing ids before reads") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_replace_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096));
    const auto missing =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_replace_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096));

    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_replace_failed);
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_replace_failed);
    MK_REQUIRE(invalid.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::invalid_mount_id);
    MK_REQUIRE(missing.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id);
    MK_REQUIRE(!invalid.candidate_load.invoked_load);
    MK_REQUIRE(!missing.candidate_load.invoked_load);
    MK_REQUIRE(invalid.diagnostics[0].code == "invalid-mount-id");
    MK_REQUIRE(missing.diagnostics[0].code == "missing-mount-id");
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace preserves state on candidate load failure") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/game.geindex", "not a cooked package index");
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_replace_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096));

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::package_load_failed);
    MK_REQUIRE(result.candidate_load.status ==
               mirakana::runtime::RuntimePackageCandidateLoadStatusV2::package_load_failed);
    MK_REQUIRE(result.candidate_load.invoked_load);
    MK_REQUIRE(result.candidate_load.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(result.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace preserves state on residency hint failure") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", "replacement");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6},
                .label = "base",
                .package = make_package(make_record(material, mirakana::AssetKind::material,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {material};
    desc.max_resident_packages = 1;

    const auto result =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_replace_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc);

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "preload-asset-missing");
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::missing_mount_id);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace preserves state on projected budget failure") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture",
                  "replacement content that exceeds the projected resident budget");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 8},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result =
        mirakana::runtime::execute_selected_runtime_package_streaming_candidate_resident_replace_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 8},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(8));

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::budget_failed);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::budget_failed);
    MK_REQUIRE(!result.diagnostics.empty());
    MK_REQUIRE(result.diagnostics[0].code == "resident-budget-intent-exceeded");
    MK_REQUIRE(result.estimated_resident_bytes > result.resident_budget_bytes);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace with reviewed evictions commits after eviction") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto other = mirakana::AssetId::from_name("textures/other");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    const std::string incoming_payload = "new";
    write_package(filesystem, incoming, mirakana::AssetKind::texture, "textures/incoming.texture", incoming_payload);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(base, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1},
                                                           "resident base payload")),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                       .label = "other",
                       .package = make_package(make_record(other, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 2},
                                                           "resident other payload")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    auto desc = make_valid_desc(incoming_payload.size());
    desc.required_preload_assets = {incoming};
    desc.resident_resource_kinds = {mirakana::AssetKind::texture};
    desc.max_resident_packages = 1;

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}}, {});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.eviction_plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1);
    MK_REQUIRE(result.invoked_eviction_plan);
    MK_REQUIRE(result.evicted_mount_count == 1);
    MK_REQUIRE(result.resident_replace.status ==
               mirakana::runtime::RuntimeResidentPackageReplaceCommitStatusV2::replaced);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.estimated_resident_bytes == incoming_payload.size());
    MK_REQUIRE(result.resident_package_count == 1);
    MK_REQUIRE(result.resident_mount_generation != previous_mount_generation);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(mount_set.mounts()[0].label == "base");
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == incoming_payload);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), other).has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package streaming candidate resident replace with reviewed evictions skips eviction when projected "
        "view fits") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, incoming, mirakana::AssetKind::texture, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(base, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    auto desc = make_valid_desc(64);
    desc.max_resident_packages = 1;

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, {}, {});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required);
    MK_REQUIRE(result.evicted_mount_count == 0);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
}

MK_TEST("runtime package streaming candidate resident replace with reviewed evictions rejects descriptors and ids "
        "before reads") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto invalid_desc = make_valid_desc(4096);
    invalid_desc.target_id.clear();
    const auto invalid_descriptor = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, invalid_desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5}}, {});

    auto missing_preflight = make_valid_desc(4096);
    missing_preflight.runtime_scene_validation_succeeded = false;
    const auto validation_required = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, missing_preflight,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5}}, {});

    const auto invalid_id = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5}}, {});
    const auto missing_id = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5}}, {});

    MK_REQUIRE(invalid_descriptor.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::invalid_descriptor);
    MK_REQUIRE(validation_required.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required);
    MK_REQUIRE(invalid_id.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_replace_failed);
    MK_REQUIRE(missing_id.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_replace_failed);
    MK_REQUIRE(!invalid_descriptor.candidate_load.invoked_load);
    MK_REQUIRE(!validation_required.candidate_load.invoked_load);
    MK_REQUIRE(!invalid_id.candidate_load.invoked_load);
    MK_REQUIRE(!missing_id.candidate_load.invoked_load);
    MK_REQUIRE(!invalid_id.invoked_eviction_plan);
    MK_REQUIRE(!missing_id.invoked_eviction_plan);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace with reviewed evictions preserves state on candidate "
        "load failure") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/game.geindex", "not a cooked package index");
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
                .label = "base",
                .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096),
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3}}, {});

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::package_load_failed);
    MK_REQUIRE(result.candidate_load.status ==
               mirakana::runtime::RuntimePackageCandidateLoadStatusV2::package_load_failed);
    MK_REQUIRE(result.candidate_load.invoked_load);
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.diagnostics[0].code == "package-index-invalid");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace with reviewed evictions preserves state on residency "
        "hint failure") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    write_package(filesystem, texture, mirakana::AssetKind::texture, "textures/player.texture", "replacement");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6},
                .label = "base",
                .package = make_package(make_record(material, mirakana::AssetKind::material,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base v1")),
            })
            .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {material};
    desc.max_resident_packages = 1;

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 6},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc, {}, {});

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.diagnostics[0].code == "preload-asset-missing");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "base v1");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace with reviewed evictions rejects reviewed eviction "
        "candidates") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto other = mirakana::AssetId::from_name("textures/other");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, incoming, mirakana::AssetKind::texture, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(base, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1},
                                                           "resident base payload")),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                       .label = "other",
                       .package = make_package(make_record(other, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 2},
                                                           "resident other payload")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto desc = make_valid_desc(3);

    const auto invalid = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{}}, {});
    const auto duplicate = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
             mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}},
            {});
    const auto missing = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99}}, {});
    const auto protected_replacement = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc,
            {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}}, {});

    MK_REQUIRE(invalid.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed);
    MK_REQUIRE(duplicate.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed);
    MK_REQUIRE(missing.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed);
    MK_REQUIRE(protected_replacement.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed);
    MK_REQUIRE(invalid.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::invalid_candidate_mount_id);
    MK_REQUIRE(duplicate.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::duplicate_candidate_mount_id);
    MK_REQUIRE(missing.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::missing_candidate_mount_id);
    MK_REQUIRE(protected_replacement.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id);
    MK_REQUIRE(!invalid.committed);
    MK_REQUIRE(!duplicate.committed);
    MK_REQUIRE(!missing.committed);
    MK_REQUIRE(!protected_replacement.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming candidate resident replace with reviewed evictions preserves state when candidates "
        "are insufficient") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto other = mirakana::AssetId::from_name("textures/other");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, incoming, mirakana::AssetKind::texture, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(base, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1},
                                                           "resident base payload")),
                   })
                   .succeeded());
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4},
                       .label = "other",
                       .package = make_package(make_record(other, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 2},
                                                           "resident other payload")),
                   })
                   .succeeded());
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::
        execute_selected_runtime_package_streaming_candidate_resident_replace_with_reviewed_evictions_safe_point(
            filesystem, mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4), {}, {});

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable);
    MK_REQUIRE(result.eviction_plan.steps.empty());
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == "resident base payload");
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
}

MK_TEST("runtime package streaming resident unmount commit removes mounted package and refreshes resident catalog") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                .label = "overlay",
                .package = make_package(make_record(material, mirakana::AssetKind::material,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 2}, "overlay")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 128,
        .max_resident_asset_records = 2,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    const auto material_handle = mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material);
    MK_REQUIRE(material_handle.has_value());

    auto desc = make_valid_desc(4096);
    desc.required_preload_assets = {texture};
    desc.resident_resource_kinds = {mirakana::AssetKind::texture};
    desc.max_resident_packages = 1;

    const auto result = mirakana::runtime::execute_selected_runtime_package_streaming_resident_unmount_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed);
    MK_REQUIRE(result.resident_unmount.status ==
               mirakana::runtime::RuntimeResidentPackageUnmountCommitStatusV2::unmounted);
    MK_REQUIRE(result.resident_catalog_refresh.status ==
               mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.resident_package_count == 1);
    MK_REQUIRE(result.resident_mount_generation == mount_set.generation());
    MK_REQUIRE(result.resident_mount_generation != previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() != previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::is_runtime_resource_handle_live_v2(catalog_cache.catalog(), *material_handle));
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), texture).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), material).has_value());
}

MK_TEST("runtime package streaming resident unmount commit rejects invalid and missing ids before mutation") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 5},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 64,
        .max_resident_asset_records = 1,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid = mirakana::runtime::execute_selected_runtime_package_streaming_resident_unmount_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096));

    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_unmount_failed);
    MK_REQUIRE(invalid.resident_unmount.status ==
               mirakana::runtime::RuntimeResidentPackageUnmountCommitStatusV2::invalid_mount_id);
    MK_REQUIRE(invalid.diagnostics.size() == 1);
    MK_REQUIRE(invalid.diagnostics[0].code == "invalid-mount-id");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);

    const auto missing = mirakana::runtime::execute_selected_runtime_package_streaming_resident_unmount_safe_point(
        mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99},
        mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, make_valid_desc(4096));

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_unmount_failed);
    MK_REQUIRE(missing.resident_unmount.status ==
               mirakana::runtime::RuntimeResidentPackageUnmountCommitStatusV2::missing_mount_id);
    MK_REQUIRE(missing.diagnostics.size() == 1);
    MK_REQUIRE(missing.diagnostics[0].code == "missing-mount-id");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package streaming resident unmount commit preserves state on projected residency hint failure") {
    const auto texture = mirakana::AssetId::from_name("textures/player/albedo");
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       .label = "base",
                       .package = make_package(make_record(texture, mirakana::AssetKind::texture,
                                                           mirakana::runtime::RuntimeAssetHandle{.value = 1}, "base")),
                   })
                   .succeeded());
    MK_REQUIRE(
        mount_set
            .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
                .label = "overlay",
                .package = make_package(make_record(material, mirakana::AssetKind::material,
                                                    mirakana::runtime::RuntimeAssetHandle{.value = 2}, "overlay")),
            })
            .succeeded());
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 initial_budget{
        .max_resident_content_bytes = 128,
        .max_resident_asset_records = 2,
    };
    MK_REQUIRE(
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, initial_budget)
            .succeeded());
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto missing_preload_desc = make_valid_desc(4096);
    missing_preload_desc.required_preload_assets = {material};
    const auto missing_preload =
        mirakana::runtime::execute_selected_runtime_package_streaming_resident_unmount_safe_point(
            mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, missing_preload_desc);

    MK_REQUIRE(!missing_preload.succeeded());
    MK_REQUIRE(missing_preload.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed);
    MK_REQUIRE(missing_preload.diagnostics.size() == 1);
    MK_REQUIRE(missing_preload.diagnostics[0].code == "preload-asset-missing");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);

    auto disallowed_kind_desc = make_valid_desc(4096);
    disallowed_kind_desc.resident_resource_kinds = {mirakana::AssetKind::texture};
    const auto disallowed_kind =
        mirakana::runtime::execute_selected_runtime_package_streaming_resident_unmount_safe_point(
            mount_set, catalog_cache, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
            mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, disallowed_kind_desc);

    MK_REQUIRE(!disallowed_kind.succeeded());
    MK_REQUIRE(disallowed_kind.status ==
               mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed);
    MK_REQUIRE(disallowed_kind.diagnostics.size() == 1);
    MK_REQUIRE(disallowed_kind.diagnostics[0].code == "resident-resource-kind-disallowed");
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

int main() {
    return mirakana::test::run_all();
}
