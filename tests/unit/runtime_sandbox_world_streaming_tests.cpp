// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/sandbox_world_streaming.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::RuntimeSandboxCellCoord;
using mirakana::runtime::RuntimeSandboxWorldAddressableDependencyKind;
using mirakana::runtime::RuntimeSandboxWorldStreamingDiagnosticCode;
using mirakana::runtime::RuntimeSandboxWorldStreamingRangeKind;
using mirakana::runtime::RuntimeSandboxWorldStreamingSafePointStatus;
using mirakana::runtime::RuntimeSandboxWorldStreamingStatus;
using mirakana::runtime::RuntimeSandboxWorldStreamingTargetState;

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

[[nodiscard]] RuntimeSandboxCellCoord cell(std::int32_t x, std::int32_t y, std::int32_t z = 0) {
    return RuntimeSandboxCellCoord{.x = x, .y = y, .z = z};
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxChunkRow chunk(std::string id, std::string region, std::int32_t origin_x,
                                                              bool resident, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxChunkRow{
        .chunk_id = std::move(id),
        .region_id = std::move(region),
        .origin_x = origin_x,
        .origin_y = 0,
        .origin_z = 0,
        .size_x = 4U,
        .size_y = 4U,
        .size_z = 1U,
        .resident = resident,
        .persistent = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxWorld make_world(bool spawn_resident = true) {
    const auto result = mirakana::runtime::build_runtime_sandbox_world(mirakana::runtime::RuntimeSandboxWorldDesc{
        .world_id = "sandbox.streaming",
        .world_tick = 100U,
        .chunk_rows =
            {
                chunk("chunk.spawn", "region.spawn", 0, spawn_resident, 1U),
                chunk("chunk.forest", "region.forest", 4, false, 2U),
                chunk("chunk.cave", "region.cave", 8, false, 3U),
            },
        .row_budget = 16U,
    });
    MK_REQUIRE(result.succeeded());
    return result.world;
}

[[nodiscard]] mirakana::runtime::RuntimeWorldRegionPackageDesc region(std::string region_id, std::uint32_t mount_id,
                                                                      std::uint64_t resident_bytes) {
    const auto package_index_path = "runtime/regions/" + region_id + ".geindex";
    const auto scene_asset_id = mirakana::AssetId::from_name(region_id + "/scene");
    return mirakana::runtime::RuntimeWorldRegionPackageDesc{
        .region_id = std::move(region_id),
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = package_index_path,
                .content_root = "runtime",
                .label = "sandbox-region",
            },
        .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
        .estimated_resident_bytes = resident_bytes,
        .estimated_asset_records = 1U,
        .required_preload_assets = {scene_asset_id},
        .resident_resource_kinds = {mirakana::AssetKind::scene},
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxWorldStreamingSourceRow
source(std::string source_id, RuntimeSandboxCellCoord min_coord, RuntimeSandboxCellCoord max_coord_exclusive,
       RuntimeSandboxWorldStreamingTargetState target_state, std::int32_t priority, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxWorldStreamingSourceRow{
        .source_id = std::move(source_id),
        .position = min_coord,
        .range_kind = RuntimeSandboxWorldStreamingRangeKind::rectangle,
        .radius = 0U,
        .min_coord = min_coord,
        .max_coord_exclusive = max_coord_exclusive,
        .target_state = target_state,
        .priority = priority,
        .player_source = source_index == 1U,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord make_record(mirakana::AssetId asset, mirakana::AssetKind kind,
                                                                std::string path, std::string content) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = static_cast<std::uint32_t>(asset.value & 0xFFFFU)},
        .asset = asset,
        .kind = kind,
        .path = std::move(path),
        .content_hash = asset.value + 41U,
        .source_revision = 1U,
        .dependencies = {},
        .content = std::move(content),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAddressableAssetRow address(std::string address_id, mirakana::AssetId asset,
                                                                    std::uint32_t source_index) {
    return mirakana::runtime::RuntimeAddressableAssetRow{
        .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = std::move(address_id)},
        .asset = asset,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxWorldAddressableDependencyRow
dependency(std::string chunk_id, RuntimeSandboxWorldAddressableDependencyKind kind, std::string address_id,
           mirakana::AssetId asset, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxWorldAddressableDependencyRow{
        .chunk_id = std::move(chunk_id),
        .kind = kind,
        .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = std::move(address_id)},
        .asset = asset,
        .required = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAddressableContentStreamingRequest addressable_request_for_forest() {
    const auto atlas = mirakana::AssetId::from_name("atlas/forest_tiles");
    const auto material = mirakana::AssetId::from_name("materials/forest_biome");
    const auto audio = mirakana::AssetId::from_name("audio/forest_ambience");
    const auto prefab = mirakana::AssetId::from_name("prefabs/tree");

    return mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "sandbox-chunk-stream",
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(atlas, mirakana::AssetKind::texture, "assets/atlas/forest_tiles.texture", "atlas-bytes"),
            make_record(material, mirakana::AssetKind::material, "assets/materials/forest.material", "material-bytes"),
            make_record(audio, mirakana::AssetKind::audio, "assets/audio/forest.audio", "audio-bytes"),
            make_record(prefab, mirakana::AssetKind::scene, "assets/prefabs/tree.scene", "prefab-bytes"),
        }),
        .addressable_assets =
            {
                address("atlas/forest_tiles", atlas, 1U),
                address("materials/forest_biome", material, 2U),
                address("audio/forest_ambience", audio, 3U),
                address("prefabs/tree", prefab, 4U),
            },
        .budget = mirakana::runtime::RuntimeAddressableResidentBudget{.max_resident_bytes = 256U},
    };
}

[[nodiscard]] std::vector<mirakana::runtime::RuntimeSandboxWorldAddressableDependencyRow> forest_dependencies() {
    return {
        dependency("chunk.forest", RuntimeSandboxWorldAddressableDependencyKind::tile_atlas, "atlas/forest_tiles",
                   mirakana::AssetId::from_name("atlas/forest_tiles"), 1U),
        dependency("chunk.forest", RuntimeSandboxWorldAddressableDependencyKind::biome_material,
                   "materials/forest_biome", mirakana::AssetId::from_name("materials/forest_biome"), 2U),
        dependency("chunk.forest", RuntimeSandboxWorldAddressableDependencyKind::audio, "audio/forest_ambience",
                   mirakana::AssetId::from_name("audio/forest_ambience"), 3U),
        dependency("chunk.forest", RuntimeSandboxWorldAddressableDependencyKind::prefab_object, "prefabs/tree",
                   mirakana::AssetId::from_name("prefabs/tree"), 4U),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxWorldStreamingPlanRequest make_streaming_request() {
    return mirakana::runtime::RuntimeSandboxWorldStreamingPlanRequest{
        .world = make_world(),
        .sources =
            {
                source("player", cell(4, 0), cell(8, 4), RuntimeSandboxWorldStreamingTargetState::active, 10, 1U),
                source("preload", cell(0, 0), cell(4, 4), RuntimeSandboxWorldStreamingTargetState::loaded, 1, 2U),
            },
        .dirty_chunk_ids = {},
        .region_packages =
            {
                region("region.spawn", 1U, 24U),
                region("region.forest", 2U, 32U),
                region("region.cave", 3U, 32U),
            },
        .addressable_content = addressable_request_for_forest(),
        .addressable_dependencies = forest_dependencies(),
        .budget =
            mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                .max_resident_content_bytes = 128U,
                .max_resident_asset_records = 8U,
            },
        .max_resident_chunks = 2U,
        .required_dependency_kinds = {},
        .allow_automatic_lru_eviction = false,
        .row_budget = 32U,
    };
}

void write_region_package(CountingFileSystem& filesystem, std::string_view region_id, mirakana::AssetId asset,
                          mirakana::AssetKind kind, std::string_view path, std::string_view payload) {
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = kind,
                                                                      .path = std::string(path),
                                                                      .content = std::string(payload),
                                                                      .source_revision = 11U,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/regions/" + std::string(region_id) + ".geindex",
                          mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text("runtime/" + std::string(path), payload);
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_package(mirakana::runtime::RuntimeAssetRecord record) {
    return mirakana::runtime::RuntimeAssetPackage({std::move(record)});
}

void mount_spawn(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set) {
    const auto scene = mirakana::AssetId::from_name("region.spawn/scene");
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                       .label = "region.spawn",
                       .package = make_package(
                           make_record(scene, mirakana::AssetKind::scene, "assets/region.spawn.scene", "spawn scene")),
                   })
                   .succeeded());
}

[[nodiscard]] mirakana::runtime::RuntimeResidentCatalogCacheV2
make_refreshed_cache(const mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set) {
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    MK_REQUIRE(catalog_cache
                   .refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                            mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                                .max_resident_content_bytes = 128U,
                                .max_resident_asset_records = 8U,
                            })
                   .succeeded());
    return catalog_cache;
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeSandboxWorldStreamingPlan& plan,
                                           RuntimeSandboxWorldStreamingDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime sandbox world streaming plans source selected chunks and addressable dependencies") {
    using Action = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;

    const auto plan = mirakana::runtime::plan_runtime_sandbox_world_streaming(make_streaming_request());

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStreamingStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.chunk_rows.size() == 2U);
    MK_REQUIRE(plan.chunk_rows[0].chunk_id == "chunk.forest");
    MK_REQUIRE(plan.chunk_rows[0].region_id == "region.forest");
    MK_REQUIRE(plan.chunk_rows[0].source_id == "player");
    MK_REQUIRE(plan.chunk_rows[0].target_state == RuntimeSandboxWorldStreamingTargetState::active);
    MK_REQUIRE(plan.chunk_rows[0].priority == 10);
    MK_REQUIRE(plan.chunk_rows[0].world_action == Action::load_region);
    MK_REQUIRE(plan.chunk_rows[0].dirty_pinned == false);
    MK_REQUIRE(plan.chunk_rows[1].chunk_id == "chunk.spawn");
    MK_REQUIRE(plan.chunk_rows[1].source_id == "preload");
    MK_REQUIRE(plan.chunk_rows[1].target_state == RuntimeSandboxWorldStreamingTargetState::loaded);
    MK_REQUIRE(plan.chunk_rows[1].world_action == Action::keep_resident);
    MK_REQUIRE(plan.world_region_plan.status == mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus::planned);
    MK_REQUIRE(plan.world_region_plan.load_count == 1U);
    MK_REQUIRE(plan.world_region_plan.keep_count == 1U);
    MK_REQUIRE(plan.addressable_dependency_rows.size() == 4U);
    MK_REQUIRE(plan.tile_atlas_dependency_count == 1U);
    MK_REQUIRE(plan.biome_material_dependency_count == 1U);
    MK_REQUIRE(plan.audio_dependency_count == 1U);
    MK_REQUIRE(plan.prefab_object_dependency_count == 1U);
    MK_REQUIRE(plan.addressable_plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::ready);
    MK_REQUIRE(plan.addressable_plan.planned_load_count == 4U);
    MK_REQUIRE(!plan.addressable_plan.invoked_package_io);
    MK_REQUIRE(!plan.addressable_plan.invoked_async_execution);
    MK_REQUIRE(!plan.invoked_package_io);
    MK_REQUIRE(!plan.invoked_threading);
    MK_REQUIRE(!plan.invoked_native_handle);
    MK_REQUIRE(!plan.invoked_automatic_lru_eviction);
}

MK_TEST("runtime sandbox world streaming pins dirty resident chunks and fails closed on resident budgets") {
    auto request = make_streaming_request();
    request.sources = {
        source("player", cell(4, 0), cell(8, 4), RuntimeSandboxWorldStreamingTargetState::active, 10, 1U),
    };
    request.dirty_chunk_ids = {"chunk.spawn"};
    request.max_resident_chunks = 1U;

    const auto plan = mirakana::runtime::plan_runtime_sandbox_world_streaming(request);

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStreamingStatus::budget_exceeded);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.chunk_rows.size() == 2U);
    MK_REQUIRE(plan.dirty_pinned_chunk_count == 1U);
    MK_REQUIRE(plan.eviction_review_required);
    MK_REQUIRE(!plan.invoked_automatic_lru_eviction);
    MK_REQUIRE(diagnostic_count(plan, RuntimeSandboxWorldStreamingDiagnosticCode::resident_chunk_count_exceeded) == 1U);
    MK_REQUIRE(plan.world_region_plan.status ==
               mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus::budget_exceeded);
}

MK_TEST("runtime sandbox world streaming rejects missing required addressable dependencies") {
    auto request = make_streaming_request();
    request.required_dependency_kinds = {RuntimeSandboxWorldAddressableDependencyKind::tile_atlas,
                                         RuntimeSandboxWorldAddressableDependencyKind::audio};
    request.addressable_dependencies = {
        dependency("chunk.forest", RuntimeSandboxWorldAddressableDependencyKind::tile_atlas, "atlas/forest_tiles",
                   mirakana::AssetId::from_name("atlas/forest_tiles"), 1U),
    };

    const auto plan = mirakana::runtime::plan_runtime_sandbox_world_streaming(request);

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStreamingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RuntimeSandboxWorldStreamingDiagnosticCode::missing_addressable_dependency) ==
               1U);
    MK_REQUIRE(plan.diagnostics.size() == 1U);
    MK_REQUIRE(plan.diagnostics[0].world_id == "sandbox.streaming");
    MK_REQUIRE(plan.addressable_plan.load_rows.empty());
    MK_REQUIRE(!plan.invoked_package_io);
    MK_REQUIRE(!plan.invoked_threading);
}

MK_TEST("runtime sandbox world streaming rejects unsupported automatic lru eviction") {
    auto request = make_streaming_request();
    request.allow_automatic_lru_eviction = true;

    const auto plan = mirakana::runtime::plan_runtime_sandbox_world_streaming(request);

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStreamingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, RuntimeSandboxWorldStreamingDiagnosticCode::unsupported_automatic_lru_eviction) ==
               1U);
    MK_REQUIRE(!plan.invoked_automatic_lru_eviction);
}

MK_TEST("runtime sandbox world streaming safe point rejects invalid plans without package reads or live mutation") {
    CountingFileSystem filesystem;
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_spawn(mount_set);
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();
    const auto previous_catalog_generation = catalog_cache.catalog().generation();

    auto request = make_streaming_request();
    request.allow_automatic_lru_eviction = true;
    const auto plan = mirakana::runtime::plan_runtime_sandbox_world_streaming(request);

    const auto result = mirakana::runtime::execute_runtime_sandbox_world_streaming_safe_point(
        filesystem, mount_set, catalog_cache,
        mirakana::runtime::RuntimeSandboxWorldStreamingSafePointDesc{
            .plan = plan,
            .target_id = "sandbox-streaming",
            .runtime_scene_validation_target_id = "sandbox-runtime-scene",
            .budget = request.budget,
            .max_resident_packages = 2U,
            .runtime_scene_validation_succeeded = true,
        });

    MK_REQUIRE(result.status == RuntimeSandboxWorldStreamingSafePointStatus::invalid_plan);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.world_region_result.rows.empty());
    MK_REQUIRE(filesystem.read_text_count() == 0);
    MK_REQUIRE(mount_set.generation() == previous_mount_generation);
    MK_REQUIRE(catalog_cache.catalog().generation() == previous_catalog_generation);
}

MK_TEST("runtime sandbox world streaming safe point adopts reviewed package candidates") {
    CountingFileSystem filesystem;
    const auto forest_scene = mirakana::AssetId::from_name("region.forest/scene");
    write_region_package(filesystem, "region.forest", forest_scene, mirakana::AssetKind::scene,
                         "regions/region.forest/scene.scene", "forest scene");

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mount_spawn(mount_set);
    auto catalog_cache = make_refreshed_cache(mount_set);
    const auto previous_mount_generation = mount_set.generation();

    const auto request = make_streaming_request();
    const auto plan = mirakana::runtime::plan_runtime_sandbox_world_streaming(request);
    const auto result = mirakana::runtime::execute_runtime_sandbox_world_streaming_safe_point(
        filesystem, mount_set, catalog_cache,
        mirakana::runtime::RuntimeSandboxWorldStreamingSafePointDesc{
            .plan = plan,
            .target_id = "sandbox-streaming",
            .runtime_scene_validation_target_id = "sandbox-runtime-scene",
            .budget = request.budget,
            .max_resident_packages = 2U,
            .runtime_scene_validation_succeeded = true,
        });

    MK_REQUIRE(result.status == RuntimeSandboxWorldStreamingSafePointStatus::completed);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.committed);
    MK_REQUIRE(result.world_region_result.committed_count == 1U);
    MK_REQUIRE(result.addressable_plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::ready);
    MK_REQUIRE(!result.addressable_plan.committed);
    MK_REQUIRE(!result.invoked_async_execution);
    MK_REQUIRE(!result.invoked_threading);
    MK_REQUIRE(mount_set.generation() != previous_mount_generation);
    MK_REQUIRE(mount_set.mounts().size() == 2U);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), forest_scene).has_value());
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

int main() {
    return mirakana::test::run_all();
}
