// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/tools/asset_runtime_package_hot_reload_tool.hpp"

#include <cstdint>
#include <sstream>
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
        return filesystem_.is_directory(path);
    }

    [[nodiscard]] std::string read_text(std::string_view path) const override {
        ++read_text_count_;
        return filesystem_.read_text(path);
    }

    [[nodiscard]] std::vector<std::string> list_files(std::string_view root) const override {
        ++list_files_count_;
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

  private:
    mirakana::MemoryFileSystem filesystem_;
    mutable int read_text_count_{0};
    mutable int list_files_count_{0};
    mutable int is_directory_count_{0};
};

[[nodiscard]] std::string texture_source_document();

class TextureSourceExternalImporter final : public mirakana::IExternalAssetImporter {
  public:
    [[nodiscard]] bool supports(const mirakana::AssetImportAction& action) const noexcept override {
        return action.kind == mirakana::AssetImportActionKind::texture;
    }

    [[nodiscard]] std::string import_source_document(mirakana::IFileSystem&,
                                                     const mirakana::AssetImportAction&) override {
        called_ = true;
        return texture_source_document();
    }

    [[nodiscard]] bool called() const noexcept {
        return called_;
    }

  private:
    bool called_{false};
};

[[nodiscard]] std::string texture_source_document() {
    return "format=GameEngine.TextureSource.v1\n"
           "texture.width=8\n"
           "texture.height=4\n"
           "texture.pixel_format=rgba8_unorm\n";
}

[[nodiscard]] std::string cooked_texture_document(mirakana::AssetId asset, std::string_view source_path) {
    std::ostringstream output;
    output << "format=GameEngine.CookedTexture.v1\n";
    output << "asset.id=" << asset.value << '\n';
    output << "asset.kind=texture\n";
    output << "source.path=" << source_path << '\n';
    output << "texture.width=8\n";
    output << "texture.height=4\n";
    output << "texture.pixel_format=rgba8_unorm\n";
    output << "texture.source_bytes=128\n";
    return output.str();
}

[[nodiscard]] mirakana::runtime::RuntimeAssetPackage make_loaded_package(mirakana::AssetId asset, std::string_view path,
                                                                         std::string_view content) {
    return mirakana::runtime::RuntimeAssetPackage({mirakana::runtime::RuntimeAssetRecord{
        .handle = mirakana::runtime::RuntimeAssetHandle{.value = 1},
        .asset = asset,
        .kind = mirakana::AssetKind::texture,
        .path = std::string(path),
        .content_hash = mirakana::hash_asset_cooked_content(content),
        .source_revision = 1,
        .dependencies = {},
        .content = std::string(content),
    }});
}

void mount_package(mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set,
                   mirakana::runtime::RuntimeResidentPackageMountIdV2 id, std::string_view label,
                   mirakana::AssetId asset, std::string_view path, std::string_view content) {
    MK_REQUIRE(mount_set
                   .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                       .id = id,
                       .label = std::string(label),
                       .package = make_loaded_package(asset, path, content),
                   })
                   .succeeded());
}

void write_package_index(CountingFileSystem& filesystem, std::string_view index_path, mirakana::AssetId asset,
                         std::string_view payload_path, std::string_view payload) {
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = asset,
                                                                      .kind = mirakana::AssetKind::texture,
                                                                      .path = std::string(payload_path),
                                                                      .content = std::string(payload),
                                                                      .source_revision = 9,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text(index_path, mirakana::serialize_asset_cooked_package_index(index));
}

[[nodiscard]] mirakana::runtime::RuntimeResidentCatalogCacheV2
make_refreshed_cache(const mirakana::runtime::RuntimeResidentPackageMountSetV2& mount_set) {
    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    const auto refresh =
        catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
                              mirakana::runtime::RuntimeResourceResidencyBudgetV2{
                                  .max_resident_content_bytes = 4096U,
                                  .max_resident_asset_records = {},
                              });
    MK_REQUIRE(refresh.succeeded());
    return catalog_cache;
}

[[nodiscard]] mirakana::AssetHotReloadRecookRequest
make_recook_request(mirakana::AssetId asset,
                    std::string_view trigger_path = "source/textures/replacement.texture_source",
                    std::uint64_t previous_revision = 7, std::uint64_t current_revision = 9) {
    return mirakana::AssetHotReloadRecookRequest{
        .asset = asset,
        .source_asset = asset,
        .trigger_path = std::string(trigger_path),
        .trigger_event_kind = mirakana::AssetHotReloadEventKind::modified,
        .reason = mirakana::AssetHotReloadRecookReason::source_modified,
        .previous_revision = previous_revision,
        .current_revision = current_revision,
        .ready_tick = 12,
    };
}

[[nodiscard]] mirakana::AssetRuntimePackageHotReloadReplacementDesc make_desc(mirakana::AssetId replacement) {
    mirakana::AssetRuntimePackageHotReloadReplacementDesc desc;
    desc.import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = replacement,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "source/textures/replacement.texture_source",
        .output_path = "runtime/textures/replacement.texture",
        .dependencies = {},
    });
    desc.recook_requests.push_back(make_recook_request(replacement));
    desc.runtime_replacement.candidates.push_back(mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
        .package_index_path = "runtime/packages/replacement.geindex",
        .content_root = "runtime",
        .label = "replacement",
    });
    desc.runtime_replacement.discovery = mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{
        .root = "runtime/packages",
        .content_root = "runtime",
    };
    desc.runtime_replacement.selected_package_index_path = "runtime/packages/replacement.geindex";
    desc.runtime_replacement.mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7};
    desc.runtime_replacement.reviewed_existing_mount_ids = {
        mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
    };
    desc.runtime_replacement.overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins;
    desc.runtime_replacement.budget = mirakana::runtime::RuntimeResourceResidencyBudgetV2{
        .max_resident_content_bytes = 4096U,
        .max_resident_asset_records = {},
    };
    desc.runtime_replacement.protected_mount_ids = {
        mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7},
    };
    return desc;
}

struct HotReloadFixture {
    CountingFileSystem filesystem;
    mirakana::AssetId base{mirakana::AssetId::from_name("textures/base")};
    mirakana::AssetId replacement{mirakana::AssetId::from_name("textures/replacement")};
    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    mirakana::AssetRuntimeReplacementState replacements;

    HotReloadFixture() {
        mount_package(mount_set, mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 7}, "base", base,
                      "textures/base.texture", "old");
        replacements.seed({
            mirakana::AssetFileSnapshot{
                .asset = replacement,
                .path = "runtime/textures/replacement.texture",
                .revision = 7,
                .size_bytes = 32,
            },
        });
    }
};

} // namespace

MK_TEST("asset runtime package hot reload replacement commits recook and resident safe point") {
    HotReloadFixture fixture;
    fixture.filesystem.write_text("source/textures/replacement.texture_source", texture_source_document());
    const auto cooked = cooked_texture_document(fixture.replacement, "source/textures/replacement.texture_source");
    write_package_index(fixture.filesystem, "runtime/packages/replacement.geindex", fixture.replacement,
                        "textures/replacement.texture", cooked);
    auto catalog_cache = make_refreshed_cache(fixture.mount_set);
    const auto previous_generation = fixture.mount_set.generation();

    const auto result = mirakana::execute_asset_runtime_package_hot_reload_replacement_safe_point(
        fixture.filesystem, fixture.replacements, fixture.mount_set, catalog_cache, make_desc(fixture.replacement));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::AssetRuntimePackageHotReloadReplacementStatus::committed);
    MK_REQUIRE(result.recook.succeeded());
    MK_REQUIRE(result.runtime_replacement.succeeded());
    MK_REQUIRE(result.committed_apply_results.size() == 1);
    MK_REQUIRE(result.committed_apply_results[0].kind == mirakana::AssetHotReloadApplyResultKind::applied);
    MK_REQUIRE(result.committed_apply_results[0].active_revision == 9);
    MK_REQUIRE(fixture.replacements.pending_count() == 0);
    MK_REQUIRE(fixture.replacements.find_active(fixture.replacement)->revision == 9);
    MK_REQUIRE(fixture.mount_set.generation() > previous_generation);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), fixture.replacement).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), fixture.base).has_value());
    MK_REQUIRE(result.invoked_recook);
    MK_REQUIRE(result.invoked_runtime_replacement);
    MK_REQUIRE(!result.invoked_file_watch);
    MK_REQUIRE(result.committed);
}

MK_TEST("asset runtime package hot reload replacement commits only the selected package recook assets") {
    HotReloadFixture fixture;
    const auto stale = mirakana::AssetId::from_name("textures/stale");
    fixture.replacements.seed({
        mirakana::AssetFileSnapshot{
            .asset = fixture.replacement,
            .path = "runtime/textures/replacement.texture",
            .revision = 7,
            .size_bytes = 32,
        },
        mirakana::AssetFileSnapshot{
            .asset = stale,
            .path = "runtime/textures/stale.texture",
            .revision = 3,
            .size_bytes = 16,
        },
    });
    const auto stale_stage = fixture.replacements.stage(
        make_recook_request(stale, "source/textures/stale.texture_source", 3, 4), "runtime/textures/stale.texture", 4);
    MK_REQUIRE(stale_stage.kind == mirakana::AssetHotReloadApplyResultKind::staged);
    fixture.filesystem.write_text("source/textures/replacement.texture_source", texture_source_document());
    const auto cooked = cooked_texture_document(fixture.replacement, "source/textures/replacement.texture_source");
    write_package_index(fixture.filesystem, "runtime/packages/replacement.geindex", fixture.replacement,
                        "textures/replacement.texture", cooked);
    auto catalog_cache = make_refreshed_cache(fixture.mount_set);

    const auto result = mirakana::execute_asset_runtime_package_hot_reload_replacement_safe_point(
        fixture.filesystem, fixture.replacements, fixture.mount_set, catalog_cache, make_desc(fixture.replacement));

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.committed_apply_results.size() == 1);
    MK_REQUIRE(result.committed_apply_results[0].asset == fixture.replacement);
    MK_REQUIRE(fixture.replacements.pending_count() == 1);
    MK_REQUIRE(fixture.replacements.find_pending(fixture.replacement) == nullptr);
    MK_REQUIRE(fixture.replacements.find_pending(stale) != nullptr);
    MK_REQUIRE(fixture.replacements.find_active(fixture.replacement)->revision == 9);
    MK_REQUIRE(fixture.replacements.find_active(stale)->revision == 3);
}

MK_TEST("asset runtime package hot reload replacement passes external import options into recook") {
    HotReloadFixture fixture;
    fixture.filesystem.write_text("source/textures/replacement.texture_source", "not a first-party texture source");
    const auto cooked = cooked_texture_document(fixture.replacement, "source/textures/replacement.texture_source");
    write_package_index(fixture.filesystem, "runtime/packages/replacement.geindex", fixture.replacement,
                        "textures/replacement.texture", cooked);
    auto catalog_cache = make_refreshed_cache(fixture.mount_set);
    auto desc = make_desc(fixture.replacement);
    TextureSourceExternalImporter importer;
    desc.import_options.external_importers = {&importer};

    const auto result = mirakana::execute_asset_runtime_package_hot_reload_replacement_safe_point(
        fixture.filesystem, fixture.replacements, fixture.mount_set, catalog_cache, desc);

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(importer.called());
    MK_REQUIRE(result.recook.import_result.imported.size() == 1);
    MK_REQUIRE(result.committed_apply_results.size() == 1);
    MK_REQUIRE(fixture.replacements.find_active(fixture.replacement)->revision == 9);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), fixture.replacement).has_value());
}

MK_TEST("asset runtime package hot reload replacement blocks recook failure before runtime package reads") {
    HotReloadFixture fixture;
    auto catalog_cache = make_refreshed_cache(fixture.mount_set);
    const auto previous_generation = fixture.mount_set.generation();

    const auto result = mirakana::execute_asset_runtime_package_hot_reload_replacement_safe_point(
        fixture.filesystem, fixture.replacements, fixture.mount_set, catalog_cache, make_desc(fixture.replacement));

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::AssetRuntimePackageHotReloadReplacementStatus::recook_failed);
    MK_REQUIRE(!result.recook.succeeded());
    MK_REQUIRE(result.runtime_replacement.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementStatusV2::recook_change_review_failed);
    MK_REQUIRE(!result.invoked_runtime_replacement);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::AssetRuntimePackageHotReloadReplacementDiagnosticPhase::recook_execution);
    MK_REQUIRE(result.diagnostics[0].code == "recook-failed");
    MK_REQUIRE(fixture.filesystem.list_files_count() == 0);
    MK_REQUIRE(fixture.mount_set.generation() == previous_generation);
    MK_REQUIRE(fixture.replacements.pending_count() == 0);
    MK_REQUIRE(fixture.replacements.find_active(fixture.replacement)->revision == 7);
}

MK_TEST("asset runtime package hot reload replacement reports recook descriptor exceptions") {
    HotReloadFixture fixture;
    fixture.filesystem.write_text("source/textures/replacement.texture_source", texture_source_document());
    auto catalog_cache = make_refreshed_cache(fixture.mount_set);
    auto desc = make_desc(fixture.replacement);
    desc.recook_requests[0].asset = {};
    const auto previous_generation = fixture.mount_set.generation();

    const auto result = mirakana::execute_asset_runtime_package_hot_reload_replacement_safe_point(
        fixture.filesystem, fixture.replacements, fixture.mount_set, catalog_cache, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::AssetRuntimePackageHotReloadReplacementStatus::recook_failed);
    MK_REQUIRE(!result.invoked_runtime_replacement);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].phase ==
               mirakana::AssetRuntimePackageHotReloadReplacementDiagnosticPhase::recook_execution);
    MK_REQUIRE(result.diagnostics[0].code == "recook-exception");
    MK_REQUIRE(result.diagnostics[0].message == "asset recook request has an invalid asset id");
    MK_REQUIRE(fixture.filesystem.list_files_count() == 0);
    MK_REQUIRE(fixture.mount_set.generation() == previous_generation);
    MK_REQUIRE(fixture.replacements.pending_count() == 0);
    MK_REQUIRE(fixture.replacements.find_active(fixture.replacement)->revision == 7);
}

MK_TEST("asset runtime package hot reload replacement preserves staged recook when runtime commit fails") {
    HotReloadFixture fixture;
    fixture.filesystem.write_text("source/textures/replacement.texture_source", texture_source_document());
    const auto cooked = cooked_texture_document(fixture.replacement, "source/textures/replacement.texture_source");
    write_package_index(fixture.filesystem, "runtime/packages/replacement.geindex", fixture.replacement,
                        "textures/replacement.texture", cooked);
    auto catalog_cache = make_refreshed_cache(fixture.mount_set);
    auto desc = make_desc(fixture.replacement);
    desc.runtime_replacement.selected_package_index_path = "runtime/packages/missing.geindex";
    const auto previous_generation = fixture.mount_set.generation();

    const auto result = mirakana::execute_asset_runtime_package_hot_reload_replacement_safe_point(
        fixture.filesystem, fixture.replacements, fixture.mount_set, catalog_cache, desc);

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::AssetRuntimePackageHotReloadReplacementStatus::runtime_replacement_failed);
    MK_REQUIRE(result.recook.succeeded());
    MK_REQUIRE(!result.runtime_replacement.succeeded());
    MK_REQUIRE(result.runtime_replacement.status ==
               mirakana::runtime::RuntimePackageHotReloadRecookReplacementStatusV2::candidate_not_found);
    MK_REQUIRE(result.committed_apply_results.empty());
    MK_REQUIRE(result.invoked_runtime_replacement);
    MK_REQUIRE(!result.committed);
    MK_REQUIRE(fixture.mount_set.generation() == previous_generation);
    MK_REQUIRE(fixture.replacements.pending_count() == 1);
    MK_REQUIRE(fixture.replacements.find_active(fixture.replacement)->revision == 7);
    MK_REQUIRE(mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), fixture.base).has_value());
    MK_REQUIRE(!mirakana::runtime::find_runtime_resource_v2(catalog_cache.catalog(), fixture.replacement).has_value());
}

int main() {
    return mirakana::test::run_all();
}
