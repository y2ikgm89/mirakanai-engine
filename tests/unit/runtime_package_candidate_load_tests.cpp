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

} // namespace

MK_TEST("runtime package candidate load reads selected candidate into typed loaded package") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/player");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = texture,
                                                                      .kind = mirakana::AssetKind::texture,
                                                                      .path = "textures/player.texture",
                                                                      .content = payload,
                                                                      .source_revision = 7,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));
    filesystem.write_text("runtime/textures/player.texture", payload);

    const auto result = mirakana::runtime::load_runtime_package_candidate_v2(filesystem, make_candidate());

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::loaded);
    MK_REQUIRE(result.candidate.package_index_path == "runtime/packages/main.geindex");
    MK_REQUIRE(result.package_desc.index_path == "runtime/packages/main.geindex");
    MK_REQUIRE(result.package_desc.content_root == "runtime");
    MK_REQUIRE(result.invoked_load);
    MK_REQUIRE(result.loaded_package.succeeded());
    MK_REQUIRE(result.loaded_package.package.records().size() == 1);
    MK_REQUIRE(result.loaded_record_count == 1);
    MK_REQUIRE(result.estimated_resident_bytes == payload.size());
    MK_REQUIRE(result.diagnostics.empty());
    const auto* record = result.loaded_package.package.find(texture);
    MK_REQUIRE(record != nullptr);
    MK_REQUIRE(record->path == "runtime/textures/player.texture");
    MK_REQUIRE(filesystem.read_text_count() == 2);
}

MK_TEST("runtime package candidate load rejects unsafe candidates before reading filesystem") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/main.geindex", "not read");

    const auto bad_index = mirakana::runtime::load_runtime_package_candidate_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                        .package_index_path = "runtime/../escape.geindex",
                        .content_root = "runtime",
                        .label = "escape",
                    });
    const auto bad_content_root = mirakana::runtime::load_runtime_package_candidate_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                        .package_index_path = "runtime/packages/main.geindex",
                        .content_root = "../runtime",
                        .label = "packages/main",
                    });
    const auto bad_label = mirakana::runtime::load_runtime_package_candidate_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                        .package_index_path = "runtime/packages/main.geindex",
                        .content_root = "runtime",
                        .label = "",
                    });

    MK_REQUIRE(bad_index.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::invalid_candidate);
    MK_REQUIRE(bad_content_root.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::invalid_candidate);
    MK_REQUIRE(bad_label.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::invalid_candidate);
    MK_REQUIRE(bad_index.diagnostics.size() == 1);
    MK_REQUIRE(bad_index.diagnostics[0].code == "invalid-package-index-path");
    MK_REQUIRE(bad_content_root.diagnostics[0].code == "invalid-content-root");
    MK_REQUIRE(bad_label.diagnostics[0].code == "invalid-label");
    MK_REQUIRE(!bad_index.invoked_load);
    MK_REQUIRE(!bad_content_root.invoked_load);
    MK_REQUIRE(!bad_label.invoked_load);
    MK_REQUIRE(bad_index.package_desc.index_path.empty());
    MK_REQUIRE(bad_content_root.package_desc.index_path.empty());
    MK_REQUIRE(bad_label.package_desc.index_path.empty());
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package candidate load reports package load failures without partial package") {
    CountingFileSystem filesystem;
    const auto texture = mirakana::AssetId::from_name("textures/missing");
    const std::string payload = "format=GameEngine.CookedTexture.v1\ntexture.width=4\n";
    const auto index = mirakana::build_asset_cooked_package_index({mirakana::AssetCookedArtifact{
                                                                      .asset = texture,
                                                                      .kind = mirakana::AssetKind::texture,
                                                                      .path = "textures/missing.texture",
                                                                      .content = payload,
                                                                      .source_revision = 1,
                                                                      .dependencies = {},
                                                                  }},
                                                                  {});
    filesystem.write_text("runtime/packages/main.geindex", mirakana::serialize_asset_cooked_package_index(index));

    const auto result = mirakana::runtime::load_runtime_package_candidate_v2(filesystem, make_candidate());

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::package_load_failed);
    MK_REQUIRE(result.invoked_load);
    MK_REQUIRE(result.loaded_package.package.empty());
    MK_REQUIRE(result.loaded_package.failures.size() == 1);
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].asset == texture);
    MK_REQUIRE(result.diagnostics[0].path == "runtime/textures/missing.texture");
    MK_REQUIRE(result.diagnostics[0].code == "package-load-failed");
}

MK_TEST("runtime package candidate load converts invalid package index text to diagnostics") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/main.geindex", "not a cooked package index");

    const auto result = mirakana::runtime::load_runtime_package_candidate_v2(filesystem, make_candidate());

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::package_load_failed);
    MK_REQUIRE(result.invoked_load);
    MK_REQUIRE(result.loaded_package.package.empty());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].path == "runtime/packages/main.geindex");
    MK_REQUIRE(result.diagnostics[0].code == "package-index-invalid");
}

MK_TEST("runtime package candidate load reports filesystem read exceptions without throwing") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/main.geindex", "not read");
    filesystem.fail_read_text(true);

    const auto result = mirakana::runtime::load_runtime_package_candidate_v2(filesystem, make_candidate());

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageCandidateLoadStatusV2::read_failed);
    MK_REQUIRE(result.invoked_load);
    MK_REQUIRE(result.loaded_package.package.empty());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].path == "runtime/packages/main.geindex");
    MK_REQUIRE(result.diagnostics[0].code == "package-read-failed");
}

int main() {
    return mirakana::test::run_all();
}
