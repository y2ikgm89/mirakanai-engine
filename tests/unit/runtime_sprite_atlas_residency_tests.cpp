// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/sprite_atlas_residency.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::AssetId asset(std::string_view name) noexcept {
    return mirakana::AssetId::from_name(name);
}

[[nodiscard]] std::string cooked_texture(mirakana::AssetId texture_asset, std::uint32_t width, std::uint32_t height) {
    const auto bytes = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) * 4U;
    return "format=GameEngine.CookedTexture.v1\n"
           "asset.kind=texture\n"
           "asset.id=" +
           std::to_string(texture_asset.value) +
           "\n"
           "texture.width=" +
           std::to_string(width) +
           "\n"
           "texture.height=" +
           std::to_string(height) +
           "\n"
           "texture.pixel_format=rgba8_unorm\n"
           "texture.source_bytes=" +
           std::to_string(bytes) + "\n";
}

[[nodiscard]] mirakana::runtime::RuntimeAssetRecord texture_record(std::string_view name,
                                                                   mirakana::runtime::RuntimeAssetHandle handle,
                                                                   std::uint32_t width, std::uint32_t height) {
    return mirakana::runtime::RuntimeAssetRecord{
        .handle = handle,
        .asset = asset(name),
        .kind = mirakana::AssetKind::texture,
        .path = std::string{name} + ".texture.geasset",
        .content_hash = 1U,
        .source_revision = 1U,
        .dependencies = {},
        .content = cooked_texture(asset(name), width, height),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSpriteAtlasPageRow page(std::string_view page_id, std::string_view texture_name,
                                                                mirakana::runtime::RuntimeResourceHandleV2 handle,
                                                                std::uint32_t width = 64U, std::uint32_t height = 32U) {
    return mirakana::runtime::RuntimeSpriteAtlasPageRow{
        .page_id = std::string{page_id},
        .texture_asset = asset(texture_name),
        .texture_resource = handle,
        .texture_uri = std::string{texture_name} + ".texture.geasset",
        .width = width,
        .height = height,
        .padding_pixels = 2U,
        .u0 = 0.03125F,
        .v0 = 0.0625F,
        .u1 = 0.96875F,
        .v1 = 0.9375F,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSpriteAtlasResidencyRequest
make_ready_request(mirakana::runtime::RuntimeAssetPackage& package,
                   mirakana::runtime::RuntimeResourceCatalogV2& catalog) {
    const auto build = mirakana::runtime::build_runtime_resource_catalog_v2(catalog, package);
    MK_REQUIRE(build.succeeded());

    return mirakana::runtime::RuntimeSpriteAtlasResidencyRequest{
        .atlas_id = "player_runtime_atlas",
        .package = &package,
        .catalog = &catalog,
        .pages = {page("page-0", "runtime/atlas/player/page0",
                       mirakana::runtime::RuntimeResourceHandleV2{.index = 1U, .generation = catalog.generation()}),
                  page("page-1", "runtime/atlas/player/page1",
                       mirakana::runtime::RuntimeResourceHandleV2{.index = 2U, .generation = catalog.generation()},
                       128U, 64U)},
        .resident_byte_budget = 40960U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeSpriteAtlasResidencyPlan& plan,
                                           mirakana::runtime::RuntimeSpriteAtlasResidencyDiagnosticCode code) {
    std::size_t count = 0;
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime sprite atlas residency plans multi-page cooked texture upload handoff rows") {
    mirakana::runtime::RuntimeAssetPackage package{
        {texture_record("runtime/atlas/player/page0", mirakana::runtime::RuntimeAssetHandle{.value = 1U}, 64U, 32U),
         texture_record("runtime/atlas/player/page1", mirakana::runtime::RuntimeAssetHandle{.value = 2U}, 128U, 64U)}};
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto request = make_ready_request(package, catalog);

    const auto plan = mirakana::runtime::plan_runtime_sprite_atlas_residency(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeSpriteAtlasResidencyStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.page_rows.size() == 2U);
    MK_REQUIRE(plan.upload_handoff_rows.size() == 2U);
    MK_REQUIRE(plan.resident_bytes == 40960U);
    MK_REQUIRE(plan.page_rows[0].resident_bytes == 8192U);
    MK_REQUIRE(plan.page_rows[1].resident_bytes == 32768U);
    MK_REQUIRE(plan.upload_handoff_rows[1].handoff_id == "player_runtime_atlas/page-1/upload");
    MK_REQUIRE(plan.invoked_runtime_source_decode == false);
    MK_REQUIRE(plan.requested_renderer_residency_ownership == false);
    MK_REQUIRE(plan.requested_public_native_handle == false);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime sprite atlas residency rejects duplicate page ids") {
    mirakana::runtime::RuntimeAssetPackage package{
        {texture_record("runtime/atlas/player/page0", mirakana::runtime::RuntimeAssetHandle{.value = 1U}, 64U, 32U),
         texture_record("runtime/atlas/player/page1", mirakana::runtime::RuntimeAssetHandle{.value = 2U}, 128U, 64U)}};
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    auto request = make_ready_request(package, catalog);
    request.pages[1].page_id = "page-0";

    const auto plan = mirakana::runtime::plan_runtime_sprite_atlas_residency(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeSpriteAtlasResidencyStatus::diagnostics);
    MK_REQUIRE(
        diagnostic_count(plan, mirakana::runtime::RuntimeSpriteAtlasResidencyDiagnosticCode::duplicate_page_id) == 1U);
    MK_REQUIRE(plan.upload_handoff_rows.empty());
}

MK_TEST("runtime sprite atlas residency rejects missing cooked texture payloads") {
    mirakana::runtime::RuntimeAssetPackage package{
        {texture_record("runtime/atlas/player/page0", mirakana::runtime::RuntimeAssetHandle{.value = 1U}, 64U, 32U)}};
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    const auto build = mirakana::runtime::build_runtime_resource_catalog_v2(catalog, package);
    MK_REQUIRE(build.succeeded());

    const auto request = mirakana::runtime::RuntimeSpriteAtlasResidencyRequest{
        .atlas_id = "player_runtime_atlas",
        .package = &package,
        .catalog = &catalog,
        .pages = {page("page-0", "runtime/atlas/player/page1",
                       mirakana::runtime::RuntimeResourceHandleV2{.index = 2U, .generation = catalog.generation()},
                       128U, 64U)},
        .resident_byte_budget = 40960U,
    };

    const auto plan = mirakana::runtime::plan_runtime_sprite_atlas_residency(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeSpriteAtlasResidencyStatus::diagnostics);
    MK_REQUIRE(
        diagnostic_count(
            plan, mirakana::runtime::RuntimeSpriteAtlasResidencyDiagnosticCode::missing_cooked_texture_payload) == 1U);
    MK_REQUIRE(plan.upload_handoff_rows.empty());
}

MK_TEST("runtime sprite atlas residency rejects invalid uv and padding metadata") {
    mirakana::runtime::RuntimeAssetPackage package{
        {texture_record("runtime/atlas/player/page0", mirakana::runtime::RuntimeAssetHandle{.value = 1U}, 64U, 32U)}};
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    auto request = make_ready_request(package, catalog);
    request.pages.resize(1U);
    request.pages[0].u1 = request.pages[0].u0;
    request.pages[0].padding_pixels = 64U;

    const auto plan = mirakana::runtime::plan_runtime_sprite_atlas_residency(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeSpriteAtlasResidencyStatus::diagnostics);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSpriteAtlasResidencyDiagnosticCode::invalid_uv_rect) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSpriteAtlasResidencyDiagnosticCode::invalid_padding) ==
               1U);
    MK_REQUIRE(plan.upload_handoff_rows.empty());
}

MK_TEST("runtime sprite atlas residency rejects stale resource handles") {
    mirakana::runtime::RuntimeAssetPackage package{
        {texture_record("runtime/atlas/player/page0", mirakana::runtime::RuntimeAssetHandle{.value = 1U}, 64U, 32U)}};
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    auto request = make_ready_request(package, catalog);
    request.pages.resize(1U);
    request.pages[0].texture_resource.generation += 1U;

    const auto plan = mirakana::runtime::plan_runtime_sprite_atlas_residency(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeSpriteAtlasResidencyStatus::diagnostics);
    MK_REQUIRE(diagnostic_count(
                   plan, mirakana::runtime::RuntimeSpriteAtlasResidencyDiagnosticCode::stale_resource_handle) == 1U);
    MK_REQUIRE(plan.upload_handoff_rows.empty());
}

MK_TEST("runtime sprite atlas residency fails closed on resident byte budget") {
    mirakana::runtime::RuntimeAssetPackage package{
        {texture_record("runtime/atlas/player/page0", mirakana::runtime::RuntimeAssetHandle{.value = 1U}, 64U, 32U),
         texture_record("runtime/atlas/player/page1", mirakana::runtime::RuntimeAssetHandle{.value = 2U}, 128U, 64U)}};
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    auto request = make_ready_request(package, catalog);
    request.resident_byte_budget = 40959U;

    const auto plan = mirakana::runtime::plan_runtime_sprite_atlas_residency(request);

    MK_REQUIRE(plan.status == mirakana::runtime::RuntimeSpriteAtlasResidencyStatus::budget_exceeded);
    MK_REQUIRE(diagnostic_count(
                   plan, mirakana::runtime::RuntimeSpriteAtlasResidencyDiagnosticCode::resident_byte_budget_exceeded) ==
               1U);
    MK_REQUIRE(plan.upload_handoff_rows.empty());
}

int main() {
    return mirakana::test::run_all();
}
