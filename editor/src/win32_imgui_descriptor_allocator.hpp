// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::editor {

struct Win32ImguiDescriptorAllocatorDesc {
    std::uintptr_t cpu_descriptor_base{0};
    std::uint64_t gpu_descriptor_base{0};
    std::uint32_t descriptor_size{0};
    std::uint32_t capacity{0};
};

struct Win32ImguiDescriptorAllocatorPlan {
    bool valid{false};
    std::string diagnostic;
};

struct Win32ImguiDescriptorLease {
    bool valid{false};
    std::uint32_t index{0};
    std::uintptr_t cpu_descriptor{0};
    std::uint64_t gpu_descriptor{0};
    std::string diagnostic;
};

[[nodiscard]] Win32ImguiDescriptorAllocatorPlan
plan_win32_imgui_descriptor_allocator(const Win32ImguiDescriptorAllocatorDesc& desc);

class Win32ImguiDescriptorAllocator final {
  public:
    explicit Win32ImguiDescriptorAllocator(Win32ImguiDescriptorAllocatorDesc desc);

    [[nodiscard]] Win32ImguiDescriptorLease allocate();
    void release(const Win32ImguiDescriptorLease& lease) noexcept;
    void release_cpu_descriptor(std::uintptr_t cpu_descriptor) noexcept;

    [[nodiscard]] std::uint32_t capacity() const noexcept;
    [[nodiscard]] std::uint32_t leased_count() const noexcept;

  private:
    Win32ImguiDescriptorAllocatorDesc desc_{};
    std::vector<std::uint32_t> free_indices_;
    std::vector<bool> leased_;
};

} // namespace mirakana::editor
