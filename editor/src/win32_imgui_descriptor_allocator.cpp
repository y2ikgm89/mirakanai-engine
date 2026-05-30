// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "win32_imgui_descriptor_allocator.hpp"

#include <algorithm>
#include <stdexcept>

namespace mirakana::editor {

Win32ImguiDescriptorAllocatorPlan plan_win32_imgui_descriptor_allocator(const Win32ImguiDescriptorAllocatorDesc& desc) {
    if (desc.capacity == 0) {
        return Win32ImguiDescriptorAllocatorPlan{.valid = false,
                                                 .diagnostic = "imgui descriptor allocator capacity must be positive"};
    }
    if (desc.descriptor_size == 0) {
        return Win32ImguiDescriptorAllocatorPlan{
            .valid = false, .diagnostic = "imgui descriptor allocator descriptor size must be positive"};
    }
    return Win32ImguiDescriptorAllocatorPlan{.valid = true};
}

Win32ImguiDescriptorAllocator::Win32ImguiDescriptorAllocator(Win32ImguiDescriptorAllocatorDesc desc) : desc_(desc) {
    const auto plan = plan_win32_imgui_descriptor_allocator(desc_);
    if (!plan.valid) {
        throw std::invalid_argument(plan.diagnostic);
    }

    free_indices_.reserve(desc_.capacity);
    for (std::uint32_t index = desc_.capacity; index > 0; --index) {
        free_indices_.push_back(index - 1U);
    }
    leased_.assign(desc_.capacity, false);
}

Win32ImguiDescriptorLease Win32ImguiDescriptorAllocator::allocate() {
    if (free_indices_.empty()) {
        return Win32ImguiDescriptorLease{.valid = false, .diagnostic = "imgui descriptor allocator exhausted"};
    }

    const auto index = free_indices_.back();
    free_indices_.pop_back();
    leased_[index] = true;

    const auto offset = static_cast<std::uint64_t>(index) * desc_.descriptor_size;
    return Win32ImguiDescriptorLease{
        .valid = true,
        .index = index,
        .cpu_descriptor = desc_.cpu_descriptor_base + static_cast<std::uintptr_t>(offset),
        .gpu_descriptor = desc_.gpu_descriptor_base + offset,
    };
}

void Win32ImguiDescriptorAllocator::release(const Win32ImguiDescriptorLease& lease) noexcept {
    if (!lease.valid || lease.index >= leased_.size() || !leased_[lease.index]) {
        return;
    }

    leased_[lease.index] = false;
    free_indices_.push_back(lease.index);
}

void Win32ImguiDescriptorAllocator::release_cpu_descriptor(std::uintptr_t cpu_descriptor) noexcept {
    if (cpu_descriptor < desc_.cpu_descriptor_base || desc_.descriptor_size == 0) {
        return;
    }

    const auto offset = cpu_descriptor - desc_.cpu_descriptor_base;
    if ((offset % desc_.descriptor_size) != 0) {
        return;
    }

    const auto index = static_cast<std::uint32_t>(offset / desc_.descriptor_size);
    release(Win32ImguiDescriptorLease{
        .valid = true,
        .index = index,
        .cpu_descriptor = cpu_descriptor,
        .gpu_descriptor = desc_.gpu_descriptor_base + static_cast<std::uint64_t>(offset),
    });
}

std::uint32_t Win32ImguiDescriptorAllocator::capacity() const noexcept {
    return desc_.capacity;
}

std::uint32_t Win32ImguiDescriptorAllocator::leased_count() const noexcept {
    return static_cast<std::uint32_t>(std::ranges::count(leased_, true));
}

} // namespace mirakana::editor
