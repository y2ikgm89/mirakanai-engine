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

    void fail_is_directory(bool value) noexcept {
        fail_is_directory_ = value;
    }

  private:
    mirakana::MemoryFileSystem filesystem_;
    mutable int read_text_count_{0};
    mutable int list_files_count_{0};
    bool fail_is_directory_{false};
    bool fail_list_files_{false};
};

} // namespace

MK_TEST("runtime package index discovery returns sorted reviewed geindex candidates") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/z_overlay.geindex", "overlay");
    filesystem.write_text("runtime/base/main.geindex", "base");
    filesystem.write_text("runtime/overlays/main.geindex", "overlay main");
    filesystem.write_text("runtime/readme.txt", "ignored");
    filesystem.write_text("runtime2/not-under-root.geindex", "ignored");

    const auto result = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime/"});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::discovered);
    MK_REQUIRE(result.root == "runtime");
    MK_REQUIRE(result.candidates.size() == 3);
    MK_REQUIRE(result.candidates[0].package_index_path == "runtime/base/main.geindex");
    MK_REQUIRE(result.candidates[0].content_root.empty());
    MK_REQUIRE(result.candidates[0].label == "base/main");
    MK_REQUIRE(result.candidates[1].package_index_path == "runtime/overlays/main.geindex");
    MK_REQUIRE(result.candidates[1].content_root.empty());
    MK_REQUIRE(result.candidates[1].label == "overlays/main");
    MK_REQUIRE(result.candidates[2].package_index_path == "runtime/z_overlay.geindex");
    MK_REQUIRE(result.candidates[2].content_root.empty());
    MK_REQUIRE(result.candidates[2].label == "z_overlay");
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package index discovery supports explicit content root for later load requests") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/packages/scene.geindex", "index");

    const auto result = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem,
        mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime/packages", .content_root = "runtime"});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.candidates.size() == 1);
    MK_REQUIRE(result.candidates[0].package_index_path == "runtime/packages/scene.geindex");
    MK_REQUIRE(result.candidates[0].content_root == "runtime");
    MK_REQUIRE(result.candidates[0].label == "scene");
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package index discovery reports no candidates for a valid empty package root") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/readme.txt", "not a package index");

    const auto result = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime"});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::no_candidates);
    MK_REQUIRE(result.candidates.empty());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(filesystem.list_files_count() == 1);
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package index discovery rejects invalid or missing roots before scanning") {
    CountingFileSystem filesystem;
    filesystem.write_text("other/game.geindex", "index");

    const auto invalid = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = ""});

    MK_REQUIRE(!invalid.succeeded());
    MK_REQUIRE(invalid.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(invalid.diagnostics.size() == 1);
    MK_REQUIRE(invalid.diagnostics[0].code == "invalid-root");
    MK_REQUIRE(invalid.candidates.empty());

    const auto missing = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime"});

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::missing_root);
    MK_REQUIRE(missing.diagnostics.size() == 1);
    MK_REQUIRE(missing.diagnostics[0].code == "missing-root");
    MK_REQUIRE(missing.diagnostics[0].path == "runtime");
    MK_REQUIRE(missing.candidates.empty());
}

MK_TEST("runtime package index discovery rejects unsafe descriptor paths before scanning") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/game.geindex", "index");

    const auto absolute = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "/runtime"});
    const auto drive = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "C:/runtime"});
    const auto parent = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime/../escape"});
    const auto backslash = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime\\packages"});
    const auto control = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime\tpackages"});
    const auto content_root = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem,
        mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime", .content_root = "../content"});

    MK_REQUIRE(absolute.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(drive.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(parent.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(backslash.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(control.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(content_root.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::invalid_descriptor);
    MK_REQUIRE(content_root.diagnostics.size() == 1);
    MK_REQUIRE(content_root.diagnostics[0].code == "invalid-content-root");
    MK_REQUIRE(filesystem.list_files_count() == 0);
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package index discovery filters invalid geindex paths with diagnostics") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/valid.geindex", "index");
    filesystem.write_text("runtime/../escape.geindex", "invalid");

    const auto result = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime"});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::discovered);
    MK_REQUIRE(result.candidates.size() == 1);
    MK_REQUIRE(result.candidates[0].package_index_path == "runtime/valid.geindex");
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "invalid-package-index-path");
    MK_REQUIRE(result.diagnostics[0].path == "runtime/../escape.geindex");
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package index discovery reports directory probe failures without throwing") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/game.geindex", "index");
    filesystem.fail_is_directory(true);

    const auto result = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime"});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::scan_failed);
    MK_REQUIRE(result.candidates.empty());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "scan-failed");
    MK_REQUIRE(result.diagnostics[0].path == "runtime");
    MK_REQUIRE(filesystem.list_files_count() == 0);
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

MK_TEST("runtime package index discovery reports list failures without throwing") {
    CountingFileSystem filesystem;
    filesystem.write_text("runtime/game.geindex", "index");
    filesystem.fail_list_files(true);

    const auto result = mirakana::runtime::discover_runtime_package_indexes_v2(
        filesystem, mirakana::runtime::RuntimePackageIndexDiscoveryDescV2{.root = "runtime"});

    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(result.status == mirakana::runtime::RuntimePackageIndexDiscoveryStatusV2::scan_failed);
    MK_REQUIRE(result.candidates.empty());
    MK_REQUIRE(result.diagnostics.size() == 1);
    MK_REQUIRE(result.diagnostics[0].code == "scan-failed");
    MK_REQUIRE(result.diagnostics[0].path == "runtime");
    MK_REQUIRE(filesystem.read_text_count() == 0);
}

int main() {
    return mirakana::test::run_all();
}
