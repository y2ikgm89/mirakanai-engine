// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "native_editor_launch.hpp"
#include "win32_imgui_descriptor_allocator.hpp"

#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::editor::NativeEditorLaunchParseResult parse_args(std::vector<std::string> args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    return mirakana::editor::parse_native_editor_launch(static_cast<int>(argv.size()), argv.data());
}

} // namespace

MK_TEST("editor native shell launch options default to interactive window") {
    const auto launch = parse_args({"MK_editor"});
    const auto& options = launch.options;

    MK_REQUIRE(options.width == 1280U);
    MK_REQUIRE(options.height == 720U);
    MK_REQUIRE(options.smoke_frames == -1);
    MK_REQUIRE(!options.no_user_config);

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(validation.valid);
    MK_REQUIRE(validation.diagnostic.empty());
}

MK_TEST("editor native shell launch options accept bounded smoke frames") {
    const auto launch =
        parse_args({"MK_editor", "--width", "1024", "--height", "768", "--smoke-frames", "3", "--no-user-config"});
    const auto& options = launch.options;

    MK_REQUIRE(options.width == 1024U);
    MK_REQUIRE(options.height == 768U);
    MK_REQUIRE(options.smoke_frames == 3);
    MK_REQUIRE(!options.smoke_resize);
    MK_REQUIRE(options.no_user_config);

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(validation.valid);
}

MK_TEST("editor native shell launch options accept deterministic smoke resize") {
    const auto launch = parse_args({"MK_editor", "--smoke-frames", "2", "--smoke-resize", "--no-user-config"});

    MK_REQUIRE(launch.options.smoke_frames == 2);
    MK_REQUIRE(launch.options.smoke_resize);

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(validation.valid);
}

MK_TEST("editor native shell launch options reject smoke resize without enough frames") {
    const auto launch = parse_args({"MK_editor", "--smoke-frames", "1", "--smoke-resize"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("smoke resize") != std::string::npos);
}

MK_TEST("editor native shell launch options reject zero window extent") {
    const auto launch = parse_args({"MK_editor", "--width", "0", "--height", "720"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("window extent") != std::string::npos);
}

MK_TEST("editor native shell launch options reject negative smoke frames") {
    const auto launch = parse_args({"MK_editor", "--smoke-frames", "-3"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("smoke frames") != std::string::npos);
}

MK_TEST("editor native shell launch options reject non numeric window extent") {
    const auto launch = parse_args({"MK_editor", "--width", "wide"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("--width") != std::string::npos);
}

MK_TEST("editor native shell launch options reject missing option value") {
    const auto launch = parse_args({"MK_editor", "--height"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("missing value") != std::string::npos);
    MK_REQUIRE(validation.diagnostic.find("--height") != std::string::npos);
}

MK_TEST("editor native shell launch options reject unknown option") {
    const auto launch = parse_args({"MK_editor", "--renderer", "legacy"});

    const auto validation = mirakana::editor::validate_native_editor_launch(launch);
    MK_REQUIRE(!validation.valid);
    MK_REQUIRE(validation.diagnostic.find("unknown option") != std::string::npos);
    MK_REQUIRE(validation.diagnostic.find("--renderer") != std::string::npos);
}

MK_TEST("editor native shell invalid launch exit code stays deterministic") {
    MK_REQUIRE(mirakana::editor::native_editor_launch_usage_error_exit_code() == 2);
}

MK_TEST("editor imgui descriptor allocator rejects zero capacity") {
    const auto plan =
        mirakana::editor::plan_win32_imgui_descriptor_allocator(mirakana::editor::Win32ImguiDescriptorAllocatorDesc{
            .cpu_descriptor_base = 100,
            .gpu_descriptor_base = 200,
            .descriptor_size = 16,
            .capacity = 0,
        });

    MK_REQUIRE(!plan.valid);
    MK_REQUIRE(plan.diagnostic.find("capacity") != std::string::npos);
}

MK_TEST("editor imgui descriptor allocator rejects zero descriptor size") {
    const auto plan =
        mirakana::editor::plan_win32_imgui_descriptor_allocator(mirakana::editor::Win32ImguiDescriptorAllocatorDesc{
            .cpu_descriptor_base = 100,
            .gpu_descriptor_base = 200,
            .descriptor_size = 0,
            .capacity = 2,
        });

    MK_REQUIRE(!plan.valid);
    MK_REQUIRE(plan.diagnostic.find("descriptor size") != std::string::npos);
}

MK_TEST("editor imgui descriptor allocator leases and reuses descriptor slots") {
    mirakana::editor::Win32ImguiDescriptorAllocator allocator(mirakana::editor::Win32ImguiDescriptorAllocatorDesc{
        .cpu_descriptor_base = 100,
        .gpu_descriptor_base = 200,
        .descriptor_size = 16,
        .capacity = 2,
    });

    auto first = allocator.allocate();
    MK_REQUIRE(first.valid);
    MK_REQUIRE(allocator.leased_count() == 1U);
    MK_REQUIRE(first.index == 0U);
    MK_REQUIRE(first.cpu_descriptor == 100U);
    MK_REQUIRE(first.gpu_descriptor == 200U);

    auto second = allocator.allocate();
    MK_REQUIRE(second.valid);
    MK_REQUIRE(allocator.leased_count() == 2U);
    MK_REQUIRE(second.index == 1U);
    MK_REQUIRE(second.cpu_descriptor == 116U);
    MK_REQUIRE(second.gpu_descriptor == 216U);

    allocator.release(first);
    MK_REQUIRE(allocator.leased_count() == 1U);
    auto reused = allocator.allocate();
    MK_REQUIRE(reused.valid);
    MK_REQUIRE(reused.index == first.index);
    MK_REQUIRE(reused.cpu_descriptor == first.cpu_descriptor);
    MK_REQUIRE(reused.gpu_descriptor == first.gpu_descriptor);
}

MK_TEST("editor imgui descriptor allocator reports exhaustion without corrupting free list") {
    mirakana::editor::Win32ImguiDescriptorAllocator allocator(mirakana::editor::Win32ImguiDescriptorAllocatorDesc{
        .cpu_descriptor_base = 8,
        .gpu_descriptor_base = 32,
        .descriptor_size = 8,
        .capacity = 1,
    });

    const auto first = allocator.allocate();
    MK_REQUIRE(first.valid);

    const auto exhausted = allocator.allocate();
    MK_REQUIRE(!exhausted.valid);
    MK_REQUIRE(exhausted.diagnostic.find("exhausted") != std::string::npos);

    allocator.release(first);
    const auto after_release = allocator.allocate();
    MK_REQUIRE(after_release.valid);
    MK_REQUIRE(after_release.index == first.index);

    allocator.release_cpu_descriptor(after_release.cpu_descriptor);
    const auto after_cpu_release = allocator.allocate();
    MK_REQUIRE(after_cpu_release.valid);
    MK_REQUIRE(after_cpu_release.index == first.index);
}

MK_TEST("editor imgui descriptor allocator ignores invalid release rows") {
    mirakana::editor::Win32ImguiDescriptorAllocator allocator(mirakana::editor::Win32ImguiDescriptorAllocatorDesc{
        .cpu_descriptor_base = 100,
        .gpu_descriptor_base = 200,
        .descriptor_size = 16,
        .capacity = 1,
    });

    allocator.release(mirakana::editor::Win32ImguiDescriptorLease{
        .valid = true,
        .index = 7,
        .cpu_descriptor = 212,
        .gpu_descriptor = 312,
    });
    MK_REQUIRE(allocator.leased_count() == 0U);

    const auto first = allocator.allocate();
    MK_REQUIRE(first.valid);
    MK_REQUIRE(allocator.leased_count() == 1U);

    allocator.release_cpu_descriptor(first.cpu_descriptor + 1U);
    MK_REQUIRE(allocator.leased_count() == 1U);

    allocator.release(first);
    allocator.release(first);
    MK_REQUIRE(allocator.leased_count() == 0U);

    const auto reused = allocator.allocate();
    MK_REQUIRE(reused.valid);
    MK_REQUIRE(reused.index == first.index);
}

int main() {
    return mirakana::test::run_all();
}
