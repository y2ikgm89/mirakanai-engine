// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/resource_runtime.hpp"

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

  private:
    mirakana::MemoryFileSystem filesystem_;
    mutable int read_text_count_{0};
};

[[nodiscard]] mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2
make_candidate(std::string package_index_path = "runtime/packages/incoming.geindex",
               std::string label = "packages/incoming") {
    return mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
        .package_index_path = std::move(package_index_path),
        .content_root = "runtime",
        .label = std::move(label),
    };
}

void write_package(CountingFileSystem& filesystem, std::string_view index_path, mirakana::AssetId asset,
                   std::string_view payload_path, std::string_view payload, std::uint64_t source_revision = 11) {
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = mirakana::AssetKind::texture,
                                                                      .path = std::string(payload_path),
                                                                      .content = std::string(payload),
                                                                      .source_revision = source_revision,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text(index_path, mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text(std::string("runtime/") + std::string(payload_path), payload);
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_loaded_package(mirakana::AssetId asset, std::string_view path,
                                                                         std::string_view content) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
        .asset = asset,
        .kind = mirakana::AssetKind::texture,
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
                       .package = make_loaded_package(asset, std::string(label) + ".texture", content),
                   })
                   .succeeded());
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

[[nodiscard]] mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsDescV2
make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2 mount_id, std::uint64_t max_bytes,
          std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> eviction_candidates = {},
          std::vector<mirakana::runtime::RuntimeResidentPackageMountIdV2> protected_mounts = {}) {
    return mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsDescV2{
        .candidate = make_candidate(),
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

} // namespace

MK_TEST("runtime package candidate resident mount with reviewed evictions commits selected candidate after eviction") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    const std::string incoming_payload = "new";
    write_package(filesystem, "runtime/packages/incoming.geindex", incoming, "textures/incoming.texture",
                  incoming_payload);
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base,
                  "resident base payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    const auto result = mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, incoming_payload.size(),
                  {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}}));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::mounted);
    MK_REQUIRE(result.candidate_load.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.eviction_plan.status == mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::planned);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1);
    MK_REQUIRE(result.evicted_mount_count == 1);
    MK_REQUIRE(result.resident_mount.status == mirakana::runtime::RuntimeResidentPackageMountStatusV2::mounted);
    MK_REQUIRE(result.catalog_refresh.status == mirakana::runtime::RuntimeResidentCatalogCacheStatusV2::rebuilt);
    MK_REQUIRE(result.loaded_resident_bytes == incoming_payload.size());
    MK_REQUIRE(result.projected_resident_bytes == incoming_payload.size());
    MK_REQUIRE(result.previous_mount_count == 1);
    MK_REQUIRE(result.mounted_package_count == 1);
    MK_REQUIRE(result.mount_generation != previous_mount_generation);
    MK_REQUIRE(result.committed);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(mount_set.mounts()[0].id == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(mount_set.mounts()[0].package.records()[0].content == incoming_payload);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package candidate resident mount with reviewed evictions skips eviction when projected view fits") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, "runtime/packages/incoming.geindex", incoming, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);

    const auto result = mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, 64));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::no_eviction_required);
    MK_REQUIRE(result.eviction_plan.steps.empty());
    MK_REQUIRE(result.evicted_mount_count == 0);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), base).has_value());
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
}

MK_TEST("runtime package candidate resident mount with reviewed evictions rejects mount ids before reads") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, "runtime/packages/incoming.geindex", incoming, "textures/incoming.texture", "not read");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid = mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{}, 64,
                  {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3}}));
    const auto duplicate =
        mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3}, 64,
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 3}}));

    MK_REQUIRE(invalid.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::invalid_mount_id);
    MK_REQUIRE(duplicate.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::duplicate_mount_id);
    MK_REQUIRE(!invalid.invoked_candidate_load);
    MK_REQUIRE(!duplicate.invoked_candidate_load);
    MK_REQUIRE(!invalid.invoked_eviction_plan);
    MK_REQUIRE(!duplicate.invoked_eviction_plan);
    MK_REQUIRE(!invalid.committed);
    MK_REQUIRE(!duplicate.committed);
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package candidate resident mount with reviewed evictions preserves state on candidate load failure") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base, "base");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();
    auto desc = make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, 64,
                          {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}});
    desc.candidate.package_index_path = "runtime/../escape.geindex";

    const auto result = mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache, desc);

    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::candidate_load_failed);
    MK_REQUIRE(result.candidate_load.status ==
               mirakana::runtime::RuntimePackageCandidateLoadStatusV2::invalid_candidate);
    MK_REQUIRE(result.invoked_candidate_load);
    MK_REQUIRE(!result.invoked_eviction_plan);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package candidate resident mount with reviewed evictions rejects protected candidates") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, "runtime/packages/incoming.geindex", incoming, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base,
                  "resident base payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto protected_new_mount =
        mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, 3,
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}}));
    const auto protected_existing_mount =
        mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, 3,
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}},
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}}));

    MK_REQUIRE(protected_new_mount.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::
                   protected_eviction_candidate_mount_id);
    MK_REQUIRE(protected_new_mount.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id);
    MK_REQUIRE(protected_new_mount.diagnostics.size() == 1);
    MK_REQUIRE(protected_new_mount.diagnostics[0].mount ==
               mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2});
    MK_REQUIRE(protected_existing_mount.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::
                   protected_eviction_candidate_mount_id);
    MK_REQUIRE(protected_existing_mount.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::protected_candidate_mount_id);
    MK_REQUIRE(protected_existing_mount.diagnostics.size() == 1);
    MK_REQUIRE(
        protected_existing_mount.diagnostics[0].phase ==
        mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::eviction_plan);
    MK_REQUIRE(protected_existing_mount.diagnostics[0].code == "protected-candidate-mount-id");
    MK_REQUIRE(!protected_new_mount.committed);
    MK_REQUIRE(!protected_existing_mount.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package candidate resident mount with reviewed evictions rejects invalid and duplicate eviction "
        "candidates") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, "runtime/packages/incoming.geindex", incoming, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base,
                  "resident base payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto invalid = mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, 3,
                  {mirakana::runtime::RuntimeResidentPackageMountIdV2{}}));
    const auto duplicate =
        mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
            filesystem, mount_set, catalog_cache,
            make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, 3,
                      {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1},
                       mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}}));

    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::
                                     invalid_eviction_candidate_mount_id);
    MK_REQUIRE(invalid.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::invalid_candidate_mount_id);
    MK_REQUIRE(invalid.diagnostics.size() == 1);
    MK_REQUIRE(
        invalid.diagnostics[0].phase ==
        mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::eviction_plan);
    MK_REQUIRE(invalid.diagnostics[0].mount == mirakana::runtime::RuntimeResidentPackageMountIdV2{});
    MK_REQUIRE(invalid.diagnostics[0].code == "invalid-candidate-mount-id");
    MK_REQUIRE(invalid.invoked_candidate_load);
    MK_REQUIRE(invalid.invoked_eviction_plan);
    MK_REQUIRE(!invalid.invoked_catalog_refresh);
    MK_REQUIRE(!invalid.committed);

    MK_REQUIRE(duplicate.status == mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::
                                       duplicate_eviction_candidate_mount_id);
    MK_REQUIRE(duplicate.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::duplicate_candidate_mount_id);
    MK_REQUIRE(duplicate.diagnostics.size() == 1);
    MK_REQUIRE(
        duplicate.diagnostics[0].phase ==
        mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsDiagnosticPhaseV2::eviction_plan);
    MK_REQUIRE(duplicate.diagnostics[0].mount == mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1});
    MK_REQUIRE(duplicate.diagnostics[0].code == "duplicate-candidate-mount-id");
    MK_REQUIRE(duplicate.invoked_candidate_load);
    MK_REQUIRE(duplicate.invoked_eviction_plan);
    MK_REQUIRE(!duplicate.invoked_catalog_refresh);
    MK_REQUIRE(!duplicate.committed);

    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
}

MK_TEST("runtime package candidate resident mount with reviewed evictions rejects missing candidates") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, "runtime/packages/incoming.geindex", incoming, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base,
                  "resident base payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, 3,
                  {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 99}}));

    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::
                                    missing_eviction_candidate_mount_id);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::missing_candidate_mount_id);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "missing-candidate-mount-id");
    MK_REQUIRE(result.invoked_candidate_load);
    MK_REQUIRE(result.invoked_eviction_plan);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 1);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime package candidate resident mount with reviewed evictions preserves state when candidates are "
        "insufficient") {
    CountingFileSystem filesystem;
    const auto base = mirakana::AssetId::from_name("textures/base");
    const auto other = mirakana::AssetId::from_name("textures/other");
    const auto incoming = mirakana::AssetId::from_name("textures/incoming");
    write_package(filesystem, "runtime/packages/incoming.geindex", incoming, "textures/incoming.texture", "new");
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}, "base", base,
                  "resident base payload");
    mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 4}, "other", other,
                  "resident other payload");
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    const auto result = mirakana::runtime::commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2(
        filesystem, mount_set, catalog_cache,
        make_desc(mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2}, 4,
                  {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1}}));

    MK_REQUIRE(result.status ==
               mirakana::runtime::RuntimePackageCandidateResidentMountReviewedEvictionsStatusV2::budget_failed);
    MK_REQUIRE(result.eviction_plan.status ==
               mirakana::runtime::RuntimeResidentPackageEvictionPlanStatusV2::budget_unreachable);
    MK_REQUIRE(result.eviction_plan.steps.size() == 1);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), incoming).has_value());
}

int main() {
    return mirakana::test::run_all();
}
