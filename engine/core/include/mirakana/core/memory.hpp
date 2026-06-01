// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/diagnostics.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class ScratchLeaseStatus : std::uint8_t {
    ready,
    released,
    invalid_request,
    invalid_alignment,
    capacity_exceeded,
    invalid_owner,
    stale_generation
};

struct ScratchLease {
    ScratchLeaseStatus status{ScratchLeaseStatus::invalid_request};
    std::span<std::byte> bytes;
    MemoryLifetimeClass lifetime_class{MemoryLifetimeClass::frame_temporary};
    std::uint32_t owner_id{0};
    std::uint64_t generation{0};

    [[nodiscard]] bool valid() const noexcept {
        return status == ScratchLeaseStatus::ready && !bytes.empty();
    }
};

class ScratchArena {
  public:
    [[nodiscard]] static ScratchArena make_frame(std::string name, std::size_t capacity_bytes);
    [[nodiscard]] static ScratchArena make_worker(std::string name, std::size_t capacity_bytes, std::uint32_t owner_id);

    ScratchArena(const ScratchArena&) = delete;
    ScratchArena& operator=(const ScratchArena&) = delete;
    ScratchArena(ScratchArena&&) noexcept = delete;
    ScratchArena& operator=(ScratchArena&&) noexcept = delete;
    ~ScratchArena() = default;

    [[nodiscard]] ScratchLease acquire(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t),
                                       std::uint32_t requester_id = 0);
    [[nodiscard]] ScratchLeaseStatus release(const ScratchLease& lease, std::uint32_t requester_id = 0) noexcept;

    void reset_at_safe_point() noexcept;

    [[nodiscard]] MemoryCounterRow memory_counter_row(std::uint64_t frame_index) const;
    [[nodiscard]] std::size_t capacity_bytes() const noexcept;
    [[nodiscard]] std::size_t used_bytes() const noexcept;
    [[nodiscard]] MemoryLifetimeClass lifetime_class() const noexcept;
    [[nodiscard]] std::uint32_t owner_id() const noexcept;
    [[nodiscard]] std::uint64_t generation() const noexcept;

  private:
    ScratchArena(MemoryLifetimeClass lifetime_class, std::string name, std::size_t capacity_bytes,
                 std::uint32_t owner_id);

    [[nodiscard]] bool contains_lease_bytes(const ScratchLease& lease) const noexcept;

    MemoryLifetimeClass lifetime_class_{MemoryLifetimeClass::frame_temporary};
    std::string name_;
    std::uint32_t owner_id_{0};
    std::size_t capacity_bytes_{0};
    std::vector<std::byte> storage_;
    std::size_t base_offset_bytes_{0};
    std::size_t cursor_bytes_{0};
    std::size_t used_bytes_{0};
    std::uint64_t allocation_count_{0};
    std::uint64_t reuse_count_{0};
    std::uint64_t reset_count_{0};
    std::uint64_t high_water_bytes_{0};
    std::uint64_t generation_{0};
    std::uint64_t safe_point_generation_{0};
    std::uint64_t cross_thread_free_count_{0};
    std::uint64_t use_after_safe_point_count_{0};
};

[[nodiscard]] std::string_view scratch_lease_status_label(ScratchLeaseStatus status) noexcept;

} // namespace mirakana
