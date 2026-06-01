// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/memory.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

inline constexpr std::size_t k_max_scratch_alignment = 4096;

[[nodiscard]] bool is_power_of_two(std::size_t value) noexcept {
    return value != 0 && (value & (value - 1U)) == 0;
}

[[nodiscard]] bool valid_scratch_alignment(std::size_t alignment) noexcept {
    return is_power_of_two(alignment) && alignment <= k_max_scratch_alignment;
}

[[nodiscard]] std::size_t align_forward(std::uintptr_t base_address, std::size_t offset,
                                        std::size_t alignment) noexcept {
    const auto address = base_address + offset;
    const auto mask = alignment - 1U;
    const auto aligned_address = (address + mask) & ~mask;
    return static_cast<std::size_t>(aligned_address - base_address);
}

} // namespace

ScratchArena ScratchArena::make_frame(std::string name, std::size_t capacity_bytes) {
    return ScratchArena(MemoryLifetimeClass::frame_temporary, std::move(name), capacity_bytes, 0);
}

ScratchArena ScratchArena::make_worker(std::string name, std::size_t capacity_bytes, std::uint32_t owner_id) {
    return ScratchArena(MemoryLifetimeClass::worker_scratch, std::move(name), capacity_bytes, owner_id);
}

ScratchArena::ScratchArena(MemoryLifetimeClass lifetime_class, std::string name, std::size_t capacity_bytes,
                           std::uint32_t owner_id)
    : lifetime_class_(lifetime_class), name_(std::move(name)), owner_id_(owner_id), capacity_bytes_(capacity_bytes) {
    if (name_.empty()) {
        throw std::invalid_argument("ScratchArena name must not be empty");
    }
    if (capacity_bytes_ == 0) {
        throw std::invalid_argument("ScratchArena capacity must be greater than zero");
    }
    if (capacity_bytes_ > std::numeric_limits<std::size_t>::max() - k_max_scratch_alignment) {
        throw std::invalid_argument("ScratchArena capacity is too large");
    }
    storage_.resize(capacity_bytes_ + k_max_scratch_alignment);
    const auto raw_base_address = reinterpret_cast<std::uintptr_t>(storage_.data());
    base_offset_bytes_ = align_forward(raw_base_address, 0, k_max_scratch_alignment);
}

ScratchLease ScratchArena::acquire(std::size_t bytes, std::size_t alignment, std::uint32_t requester_id) {
    if (lifetime_class_ == MemoryLifetimeClass::worker_scratch && requester_id != owner_id_) {
        return ScratchLease{.status = ScratchLeaseStatus::invalid_owner,
                            .lifetime_class = lifetime_class_,
                            .owner_id = owner_id_,
                            .generation = generation_};
    }
    if (bytes == 0) {
        return ScratchLease{.status = ScratchLeaseStatus::invalid_request,
                            .lifetime_class = lifetime_class_,
                            .owner_id = owner_id_,
                            .generation = generation_};
    }
    if (!valid_scratch_alignment(alignment)) {
        return ScratchLease{.status = ScratchLeaseStatus::invalid_alignment,
                            .lifetime_class = lifetime_class_,
                            .owner_id = owner_id_,
                            .generation = generation_};
    }
    if (bytes > capacity_bytes_) {
        return ScratchLease{.status = ScratchLeaseStatus::capacity_exceeded,
                            .lifetime_class = lifetime_class_,
                            .owner_id = owner_id_,
                            .generation = generation_};
    }

    const auto base_address = reinterpret_cast<std::uintptr_t>(storage_.data() + base_offset_bytes_);
    const auto aligned_offset = align_forward(base_address, cursor_bytes_, alignment);
    if (aligned_offset > capacity_bytes_ || bytes > capacity_bytes_ - aligned_offset) {
        return ScratchLease{.status = ScratchLeaseStatus::capacity_exceeded,
                            .lifetime_class = lifetime_class_,
                            .owner_id = owner_id_,
                            .generation = generation_};
    }

    auto lease_bytes = std::span<std::byte>(storage_.data() + base_offset_bytes_ + aligned_offset, bytes);
    cursor_bytes_ = aligned_offset + bytes;
    used_bytes_ = cursor_bytes_;
    ++allocation_count_;
    if (reset_count_ > 0) {
        ++reuse_count_;
    }
    high_water_bytes_ = std::max(high_water_bytes_, static_cast<std::uint64_t>(used_bytes_));

    return ScratchLease{.status = ScratchLeaseStatus::ready,
                        .bytes = lease_bytes,
                        .lifetime_class = lifetime_class_,
                        .owner_id = owner_id_,
                        .generation = generation_};
}

ScratchLeaseStatus ScratchArena::release(const ScratchLease& lease, std::uint32_t requester_id) noexcept {
    if (lease.status != ScratchLeaseStatus::ready) {
        return ScratchLeaseStatus::invalid_request;
    }
    if (lease.lifetime_class != lifetime_class_) {
        return ScratchLeaseStatus::invalid_request;
    }
    if (!contains_lease_bytes(lease)) {
        return ScratchLeaseStatus::invalid_request;
    }
    if (requester_id != owner_id_) {
        ++cross_thread_free_count_;
        return ScratchLeaseStatus::invalid_owner;
    }
    if (lease.generation != generation_) {
        ++use_after_safe_point_count_;
        return ScratchLeaseStatus::stale_generation;
    }
    return ScratchLeaseStatus::released;
}

bool ScratchArena::contains_lease_bytes(const ScratchLease& lease) const noexcept {
    if (lease.bytes.empty()) {
        return false;
    }

    const auto lease_begin = reinterpret_cast<std::uintptr_t>(lease.bytes.data());
    const auto arena_begin = reinterpret_cast<std::uintptr_t>(storage_.data() + base_offset_bytes_);
    if (lease_begin < arena_begin) {
        return false;
    }

    const auto lease_offset = static_cast<std::size_t>(lease_begin - arena_begin);
    return lease_offset <= capacity_bytes_ && lease.bytes.size() <= capacity_bytes_ - lease_offset;
}

void ScratchArena::reset_at_safe_point() noexcept {
    cursor_bytes_ = 0;
    used_bytes_ = 0;
    ++generation_;
    safe_point_generation_ = generation_;
    ++reset_count_;
}

MemoryCounterRow ScratchArena::memory_counter_row(std::uint64_t frame_index) const {
    return MemoryCounterRow{.lifetime_class = lifetime_class_,
                            .name = name_,
                            .bytes = static_cast<std::uint64_t>(used_bytes_),
                            .allocation_count = allocation_count_,
                            .high_water_bytes = high_water_bytes_,
                            .budget_bytes = static_cast<std::uint64_t>(capacity_bytes_),
                            .generation = generation_,
                            .safe_point_generation = safe_point_generation_,
                            .frame_index = frame_index,
                            .reuse_count = reuse_count_,
                            .reset_count = reset_count_,
                            .cross_thread_free_count = cross_thread_free_count_,
                            .use_after_safe_point_count = use_after_safe_point_count_};
}

std::size_t ScratchArena::capacity_bytes() const noexcept {
    return capacity_bytes_;
}

std::size_t ScratchArena::used_bytes() const noexcept {
    return used_bytes_;
}

MemoryLifetimeClass ScratchArena::lifetime_class() const noexcept {
    return lifetime_class_;
}

std::uint32_t ScratchArena::owner_id() const noexcept {
    return owner_id_;
}

std::uint64_t ScratchArena::generation() const noexcept {
    return generation_;
}

std::string_view scratch_lease_status_label(ScratchLeaseStatus status) noexcept {
    switch (status) {
    case ScratchLeaseStatus::ready:
        return "ready";
    case ScratchLeaseStatus::released:
        return "released";
    case ScratchLeaseStatus::invalid_request:
        return "invalid_request";
    case ScratchLeaseStatus::invalid_alignment:
        return "invalid_alignment";
    case ScratchLeaseStatus::capacity_exceeded:
        return "capacity_exceeded";
    case ScratchLeaseStatus::invalid_owner:
        return "invalid_owner";
    case ScratchLeaseStatus::stale_generation:
        return "stale_generation";
    }
    return "unknown";
}

} // namespace mirakana
