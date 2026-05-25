// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/addressable_content_streaming.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord make_record(mirakana::AssetId asset, mirakana::AssetKind kind,
                                                                std::string path, std::string content) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = static_cast<std::uint32_t>(asset.value & 0xFFFFU)},
        .asset = asset,
        .kind = kind,
        .path = std::move(path),
        .content_hash = asset.value + 17U,
        .source_revision = 1U,
        .dependencies = {},
        .content = std::move(content),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAddressableAssetRow make_address(std::string address, mirakana::AssetId asset,
                                                                         std::uint32_t source_index) {
    return mirakana::runtime::RuntimeAddressableAssetRow{
        .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = std::move(address)},
        .asset = asset,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAddressableLoadRequest make_load(std::string address,
                                                                         std::uint32_t source_index) {
    return mirakana::runtime::RuntimeAddressableLoadRequest{
        .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = std::move(address)},
        .include_dependencies = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAddressableReleaseRequest
make_release(std::string address, std::uint32_t release_count, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeAddressableReleaseRequest{
        .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = std::move(address)},
        .release_count = release_count,
        .include_dependencies = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeAddressableReleaseRequest
make_release_with_dependencies(std::string address, std::uint32_t release_count, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeAddressableReleaseRequest{
        .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = std::move(address)},
        .release_count = release_count,
        .include_dependencies = true,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeAddressableContentStreamingPlan& plan,
                                           mirakana::runtime::RuntimeAddressableContentDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime addressable content streaming resolves dependency closure and load refcounts") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");
    const auto material = mirakana::AssetId::from_name("materials/player");
    const auto texture = mirakana::AssetId::from_name("textures/player");

    const auto request =
        mirakana::runtime::RuntimeAddressableContentStreamingRequest{
            .stream_id = "level-stream",
            .package = mirakana::runtime::RuntimeAssetPackage(
                {
                    make_record(texture, mirakana::AssetKind::texture, "assets/textures/player.texture", "texture"),
                    make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
                    make_record(material, mirakana::AssetKind::material, "assets/materials/player.material",
                                "material-data"),
                },
                {
                    mirakana::AssetDependencyEdge{.asset = scene,
                                                  .dependency = material,
                                                  .kind = mirakana::AssetDependencyKind::scene_material,
                                                  .path = "assets/materials/player.material"},
                    mirakana::AssetDependencyEdge{.asset = material,
                                                  .dependency = texture,
                                                  .kind = mirakana::AssetDependencyKind::material_texture,
                                                  .path = "assets/textures/player.texture"},
                }),
            .addressable_assets =
                {
                    make_address("textures/player", texture, 3U),
                    make_address("scenes/playable", scene, 1U),
                    make_address("materials/player", material, 2U),
                },
            .resident_assets =
                {
                    mirakana::runtime::RuntimeAddressableResidentAssetRow{
                        .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = "materials/player"},
                        .ref_count = 1U,
                        .source_index = 1U,
                    },
                },
            .load_requests = {make_load("scenes/playable", 1U)},
            .budget = mirakana::runtime::RuntimeAddressableResidentBudget{.max_resident_bytes = 128U},
        };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.address_rows.size() == 3U);
    MK_REQUIRE(plan.dependency_rows.size() == 2U);
    MK_REQUIRE(plan.load_rows.size() == 3U);
    MK_REQUIRE(plan.release_rows.empty());
    MK_REQUIRE(plan.address_rows[0].address_id.value == "materials/player");
    MK_REQUIRE(plan.address_rows[1].address_id.value == "scenes/playable");
    MK_REQUIRE(plan.address_rows[2].address_id.value == "textures/player");
    MK_REQUIRE(plan.dependency_rows[0].address_id.value == "materials/player");
    MK_REQUIRE(plan.dependency_rows[0].dependency_address_id.value == "textures/player");
    MK_REQUIRE(plan.dependency_rows[1].address_id.value == "scenes/playable");
    MK_REQUIRE(plan.dependency_rows[1].dependency_address_id.value == "materials/player");
    MK_REQUIRE(plan.load_rows[0].address_id.value == "materials/player");
    MK_REQUIRE(plan.load_rows[0].action == mirakana::runtime::RuntimeAddressableLoadAction::load_dependency);
    MK_REQUIRE(plan.load_rows[0].ref_count_after == 2U);
    MK_REQUIRE(plan.load_rows[1].address_id.value == "scenes/playable");
    MK_REQUIRE(plan.load_rows[1].action == mirakana::runtime::RuntimeAddressableLoadAction::load_asset);
    MK_REQUIRE(plan.load_rows[1].ref_count_after == 1U);
    MK_REQUIRE(plan.load_rows[2].address_id.value == "textures/player");
    MK_REQUIRE(plan.load_rows[2].action == mirakana::runtime::RuntimeAddressableLoadAction::load_dependency);
    MK_REQUIRE(plan.load_rows[2].ref_count_after == 1U);
    MK_REQUIRE(plan.address_count == 3U);
    MK_REQUIRE(plan.dependency_count == 2U);
    MK_REQUIRE(plan.planned_load_count == 3U);
    MK_REQUIRE(plan.planned_release_count == 0U);
    MK_REQUIRE(plan.final_resident_bytes == 30U);
    MK_REQUIRE(plan.resident_budget_bytes == 128U);
    MK_REQUIRE(plan.committed == false);
    MK_REQUIRE(plan.invoked_package_io == false);
    MK_REQUIRE(plan.invoked_async_execution == false);
}

MK_TEST("runtime addressable content streaming plans explicit releases without underflow") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
        }),
        .addressable_assets = {make_address("scenes/playable", scene, 1U)},
        .resident_assets =
            {
                mirakana::runtime::RuntimeAddressableResidentAssetRow{
                    .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = "scenes/playable"},
                    .ref_count = 2U,
                    .source_index = 1U,
                },
            },
        .release_requests = {make_release("scenes/playable", 2U, 1U)},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.release_rows.size() == 1U);
    MK_REQUIRE(plan.release_rows[0].address_id.value == "scenes/playable");
    MK_REQUIRE(plan.release_rows[0].ref_count_after == 0U);
    MK_REQUIRE(plan.release_rows[0].resident_bytes_released == 10U);
    MK_REQUIRE(plan.planned_release_count == 1U);
    MK_REQUIRE(plan.final_resident_bytes == 0U);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime addressable content streaming does not reload the root address through dependency cycles") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");
    const auto material = mirakana::AssetId::from_name("materials/player");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage(
            {
                make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
                make_record(material, mirakana::AssetKind::material, "assets/materials/player.material",
                            "material-data"),
            },
            {
                mirakana::AssetDependencyEdge{.asset = scene,
                                              .dependency = material,
                                              .kind = mirakana::AssetDependencyKind::scene_material,
                                              .path = "assets/materials/player.material"},
                mirakana::AssetDependencyEdge{.asset = material,
                                              .dependency = scene,
                                              .kind = mirakana::AssetDependencyKind::scene_material,
                                              .path = "assets/scenes/playable.scene"},
            }),
        .addressable_assets =
            {
                make_address("scenes/playable", scene, 1U),
                make_address("materials/player", material, 2U),
            },
        .load_requests = {make_load("scenes/playable", 1U)},
        .budget = mirakana::runtime::RuntimeAddressableResidentBudget{.max_resident_bytes = 128U},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.load_rows.size() == 2U);
    MK_REQUIRE(plan.load_rows[0].address_id.value == "materials/player");
    MK_REQUIRE(plan.load_rows[1].address_id.value == "scenes/playable");
    MK_REQUIRE(plan.load_rows[1].action == mirakana::runtime::RuntimeAddressableLoadAction::load_asset);
    MK_REQUIRE(plan.load_rows[1].ref_count_after == 1U);
}

MK_TEST("runtime addressable content streaming rejects asset aliases before dependency planning") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
        }),
        .addressable_assets =
            {
                make_address("scenes/playable", scene, 1U),
                make_address("scenes/alias", scene, 2U),
            },
        .load_requests = {make_load("scenes/alias", 2U)},
        .budget = mirakana::runtime::RuntimeAddressableResidentBudget{.max_resident_bytes = 128U},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.load_rows.empty());
    MK_REQUIRE(plan.release_rows.empty());
    MK_REQUIRE(plan.diagnostics.size() == 1U);
    MK_REQUIRE(plan.diagnostics[0].code ==
               mirakana::runtime::RuntimeAddressableContentDiagnosticCode::duplicate_address_asset);
    MK_REQUIRE(plan.diagnostics[0].asset == scene);
    MK_REQUIRE(plan.diagnostics[0].address_id.value == "scenes/alias");
}

MK_TEST("runtime addressable content streaming preserves stream id on release underflow diagnostics") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
        }),
        .addressable_assets = {make_address("scenes/playable", scene, 1U)},
        .resident_assets =
            {
                mirakana::runtime::RuntimeAddressableResidentAssetRow{
                    .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = "scenes/playable"},
                    .ref_count = 1U,
                    .source_index = 1U,
                },
            },
        .release_requests = {make_release("scenes/playable", 2U, 1U)},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.release_rows.empty());
    MK_REQUIRE(plan.diagnostics.size() == 1U);
    MK_REQUIRE(plan.diagnostics[0].code ==
               mirakana::runtime::RuntimeAddressableContentDiagnosticCode::release_underflow);
    MK_REQUIRE(plan.diagnostics[0].stream_id == "level-stream");
    MK_REQUIRE(plan.diagnostics[0].address_id.value == "scenes/playable");
}

MK_TEST("runtime addressable content streaming drops dependency releases when the root release underflows") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");
    const auto material = mirakana::AssetId::from_name("materials/player");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage(
            {
                make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
                make_record(material, mirakana::AssetKind::material, "assets/materials/player.material",
                            "material-data"),
            },
            {
                mirakana::AssetDependencyEdge{.asset = scene,
                                              .dependency = material,
                                              .kind = mirakana::AssetDependencyKind::scene_material,
                                              .path = "assets/materials/player.material"},
            }),
        .addressable_assets =
            {
                make_address("scenes/playable", scene, 1U),
                make_address("materials/player", material, 2U),
            },
        .resident_assets =
            {
                mirakana::runtime::RuntimeAddressableResidentAssetRow{
                    .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = "materials/player"},
                    .ref_count = 1U,
                    .source_index = 1U,
                },
            },
        .release_requests = {make_release_with_dependencies("scenes/playable", 1U, 1U)},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.release_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeAddressableContentDiagnosticCode::release_underflow) ==
               1U);
    MK_REQUIRE(plan.diagnostics[0].address_id.value == "scenes/playable");
}

MK_TEST("runtime addressable content streaming drops planned loads when a later release underflows") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
        }),
        .addressable_assets = {make_address("scenes/playable", scene, 1U)},
        .load_requests = {make_load("scenes/playable", 1U)},
        .release_requests = {make_release("scenes/playable", 2U, 2U)},
        .budget = mirakana::runtime::RuntimeAddressableResidentBudget{.max_resident_bytes = 128U},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.load_rows.empty());
    MK_REQUIRE(plan.release_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeAddressableContentDiagnosticCode::release_underflow) ==
               1U);
}

MK_TEST("runtime addressable content streaming drops root releases when dependency release underflows") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");
    const auto material = mirakana::AssetId::from_name("materials/player");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage(
            {
                make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
                make_record(material, mirakana::AssetKind::material, "assets/materials/player.material",
                            "material-data"),
            },
            {
                mirakana::AssetDependencyEdge{.asset = scene,
                                              .dependency = material,
                                              .kind = mirakana::AssetDependencyKind::scene_material,
                                              .path = "assets/materials/player.material"},
            }),
        .addressable_assets =
            {
                make_address("scenes/playable", scene, 1U),
                make_address("materials/player", material, 2U),
            },
        .resident_assets =
            {
                mirakana::runtime::RuntimeAddressableResidentAssetRow{
                    .address_id = mirakana::runtime::RuntimeAddressableAssetId{.value = "scenes/playable"},
                    .ref_count = 1U,
                    .source_index = 1U,
                },
            },
        .release_requests = {make_release_with_dependencies("scenes/playable", 1U, 1U)},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.load_rows.empty());
    MK_REQUIRE(plan.release_rows.empty());
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeAddressableContentDiagnosticCode::release_underflow) ==
               1U);
    MK_REQUIRE(plan.diagnostics[0].address_id.value == "materials/player");
}

MK_TEST("runtime addressable content streaming rejects missing dependency addresses before planning loads") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");
    const auto material = mirakana::AssetId::from_name("materials/player");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage(
            {
                make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
                make_record(material, mirakana::AssetKind::material, "assets/materials/player.material",
                            "material-data"),
            },
            {
                mirakana::AssetDependencyEdge{.asset = scene,
                                              .dependency = material,
                                              .kind = mirakana::AssetDependencyKind::scene_material,
                                              .path = "assets/materials/player.material"},
            }),
        .addressable_assets = {make_address("scenes/playable", scene, 1U)},
        .load_requests = {make_load("scenes/playable", 1U)},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.load_rows.empty());
    MK_REQUIRE(diagnostic_count(
                   plan, mirakana::runtime::RuntimeAddressableContentDiagnosticCode::missing_dependency_address) == 1U);
}

MK_TEST("runtime addressable content streaming reports budget rejection without host side effects") {
    const auto scene = mirakana::AssetId::from_name("scenes/playable");

    const auto request = mirakana::runtime::RuntimeAddressableContentStreamingRequest{
        .stream_id = "level-stream",
        .package = mirakana::runtime::RuntimeAssetPackage({
            make_record(scene, mirakana::AssetKind::scene, "assets/scenes/playable.scene", "scene-data"),
        }),
        .addressable_assets = {make_address("scenes/playable", scene, 1U)},
        .load_requests = {make_load("scenes/playable", 1U)},
        .budget = mirakana::runtime::RuntimeAddressableResidentBudget{.max_resident_bytes = 4U},
    };

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeAddressableContentStreamingStatus::budget_limited);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.final_resident_bytes == 10U);
    MK_REQUIRE(plan.resident_budget_bytes == 4U);
    MK_REQUIRE(plan.budget_rejected_load_count == 1U);
    MK_REQUIRE(plan.committed == false);
    MK_REQUIRE(plan.invoked_package_io == false);
    MK_REQUIRE(plan.invoked_async_execution == false);
    MK_REQUIRE(diagnostic_count(
                   plan, mirakana::runtime::RuntimeAddressableContentDiagnosticCode::resident_budget_exceeded) == 1U);
}

int main() {
    return mirakana::test::run_all();
}
