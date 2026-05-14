// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/rhi/rhi.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace mirakana::rhi {

struct RhiUploadAllocationId {
    std::uint64_t value{0};

    friend bool operator==(RhiUploadAllocationId lhs, RhiUploadAllocationId rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

struct RhiUploadAllocationHandle {
    RhiUploadAllocationId id;
    std::uint32_t generation{0};

    friend bool operator==(RhiUploadAllocationHandle lhs, RhiUploadAllocationHandle rhs) noexcept {
        return lhs.id == rhs.id && lhs.generation == rhs.generation;
    }
};

enum class RhiUploadDiagnosticCode : std::uint8_t {
    invalid_allocation = 0,
    invalid_copy_range,
    stale_generation,
    already_submitted,
    ring_exhausted,
    allocation_already_bound,
    gpu_batch_mismatch,
};

struct RhiUploadDiagnostic {
    RhiUploadDiagnosticCode code{RhiUploadDiagnosticCode::invalid_allocation};
    RhiUploadAllocationHandle handle;
    std::string message;
};

struct RhiUploadResult {
    std::vector<RhiUploadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RhiUploadAllocationResult {
    RhiUploadAllocationHandle handle;
    std::vector<RhiUploadDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct RhiUploadAllocationDesc {
    std::uint64_t size_bytes{0};
    std::string debug_name;
};

struct RhiBufferUploadDesc {
    RhiUploadAllocationHandle staging;
    std::uint64_t staging_offset{0};
    std::uint64_t size_bytes{0};
    std::string debug_name;
};

struct RhiTextureUploadDesc {
    RhiUploadAllocationHandle staging;
    Format format{Format::unknown};
    BufferTextureCopyRegion region;
    std::string debug_name;
};

enum class RhiUploadCopyKind : std::uint8_t { buffer = 0, texture };

struct RhiUploadCopyRecord {
    RhiUploadCopyKind kind{RhiUploadCopyKind::buffer};
    RhiUploadAllocationHandle staging;
    std::uint64_t staging_offset{0};
    std::uint64_t size_bytes{0};
    Format texture_format{Format::unknown};
    BufferTextureCopyRegion texture_region;
    std::string debug_name;
};

struct RhiUploadAllocationRecord {
    RhiUploadAllocationHandle handle;
    std::uint64_t size_bytes{0};
    std::string debug_name;
    bool submitted{false};
    FenceValue submitted_fence;
};

class RhiUploadStagingPlan {
  public:
    [[nodiscard]] RhiUploadAllocationResult allocate_staging_bytes(RhiUploadAllocationDesc desc);
    [[nodiscard]] RhiUploadResult enqueue_buffer_upload(RhiBufferUploadDesc desc);
    [[nodiscard]] RhiUploadResult enqueue_texture_upload(RhiTextureUploadDesc desc);
    [[nodiscard]] RhiUploadResult mark_submitted(RhiUploadAllocationHandle handle, FenceValue fence);
    [[nodiscard]] std::uint32_t retire_completed_uploads(FenceValue completed_fence);
    [[nodiscard]] const std::vector<RhiUploadCopyRecord>& pending_copies() const noexcept;
    [[nodiscard]] const std::vector<RhiUploadAllocationRecord>& allocations() const noexcept;

  private:
    [[nodiscard]] RhiUploadAllocationRecord* find_allocation(RhiUploadAllocationHandle handle) noexcept;
    [[nodiscard]] const RhiUploadAllocationRecord* find_allocation(RhiUploadAllocationHandle handle) const noexcept;

    std::vector<RhiUploadAllocationRecord> allocations_;
    std::vector<RhiUploadCopyRecord> pending_copies_;
    std::uint64_t next_id_{1};
};

struct RhiUploadRingDesc {
    std::uint64_t size_bytes{0};
    /// Minimum alignment applied to every sub-range start (use 256 for D3D12 constant buffer alignment defaults).
    std::uint64_t min_alignment{256};
};

/// Device-owned upload ring: one `copy_source` buffer and CPU/GPU lifetime tracking for byte spans keyed by
/// `RhiUploadAllocationId::value`. Callers pair this with `RhiUploadStagingPlan` reservations, then
/// `record_upload_gpu_batch` to issue copies. Buffer lifetime matches `IRhiDevice` (no explicit destroy API yet).
class RhiUploadRing {
  public:
    explicit RhiUploadRing(IRhiDevice& device, RhiUploadRingDesc desc);
    RhiUploadRing(RhiUploadRing&&) noexcept = default;
    RhiUploadRing& operator=(RhiUploadRing&&) noexcept = default;
    RhiUploadRing(const RhiUploadRing&) = delete;
    RhiUploadRing& operator=(const RhiUploadRing&) = delete;
    ~RhiUploadRing() = default;

    [[nodiscard]] BufferHandle buffer() const noexcept {
        return buffer_;
    }
    [[nodiscard]] std::uint64_t capacity_bytes() const noexcept {
        return size_bytes_;
    }

    /// Reserves `size_bytes` for `handle` inside the ring after applying `release_completed(completed_fence)`.
    /// Fails with `ring_exhausted` when no aligned gap fits, or `allocation_already_bound` when the id is mapped twice.
    [[nodiscard]] RhiUploadResult reserve_for_allocation(const RhiUploadStagingPlan& plan,
                                                         RhiUploadAllocationHandle handle, FenceValue completed_fence);

    [[nodiscard]] std::optional<std::uint64_t> byte_offset_for(RhiUploadAllocationHandle handle) const noexcept;
    [[nodiscard]] bool owns_allocation(RhiUploadAllocationHandle handle) const noexcept;

    /// Tags all spans reserved since the last `notify_submit` with `fence` (typically the fence returned from
    /// `IRhiDevice::submit` after recording `record_upload_gpu_batch`).
    void notify_submit(FenceValue fence);

    /// Drops spans whose submitted fence is known completed on the device timeline.
    void release_completed(FenceValue completed_fence);

  private:
    struct Span {
        std::uint64_t begin{0};
        std::uint64_t end{0};
        std::uint64_t allocation_id_value{0};
        std::uint32_t allocation_generation{0};
        bool has_fence{false};
        FenceValue fence{};
    };

    [[nodiscard]] static std::uint64_t align_up(std::uint64_t value, std::uint64_t alignment) noexcept;
    [[nodiscard]] bool try_place(std::uint64_t size_bytes, std::uint64_t* out_begin);

    IRhiDevice* device_{nullptr};
    BufferHandle buffer_{};
    std::uint64_t size_bytes_{0};
    std::uint64_t min_alignment_{256};
    std::vector<Span> spans_;
};

struct RhiStagingBufferPoolDesc {
    std::uint32_t buffer_count{0};
    std::uint64_t chunk_size_bytes{0};
};

/// Fixed-size pool of independent `copy_source` staging chunks (for uploads larger than a ring slot or parallel
/// frames).
class RhiStagingBufferPool {
  public:
    explicit RhiStagingBufferPool(IRhiDevice& device, RhiStagingBufferPoolDesc desc);
    RhiStagingBufferPool(RhiStagingBufferPool&&) noexcept = default;
    RhiStagingBufferPool& operator=(RhiStagingBufferPool&&) noexcept = default;
    RhiStagingBufferPool(const RhiStagingBufferPool&) = delete;
    RhiStagingBufferPool& operator=(const RhiStagingBufferPool&) = delete;
    ~RhiStagingBufferPool() = default;

    [[nodiscard]] std::optional<BufferHandle> try_acquire() noexcept;
    void release(BufferHandle buffer);

  private:
    IRhiDevice* device_{nullptr};
    std::vector<BufferHandle> buffers_;
    std::vector<bool> free_;
};

struct RhiGpuBufferCopyTarget {
    BufferHandle destination{};
    std::uint64_t destination_offset{0};
};

struct RhiGpuTextureCopyTarget {
    TextureHandle destination{};
};

[[nodiscard]] RhiUploadResult validate_upload_gpu_batch(const RhiUploadStagingPlan& plan,
                                                        const std::vector<RhiGpuBufferCopyTarget>& buffer_targets,
                                                        const std::vector<RhiGpuTextureCopyTarget>& texture_targets);

/// Records pending plan copies. Every distinct `RhiUploadAllocationHandle` referenced by pending copies must have been
/// `reserve_for_allocation` on `ring` first. `buffer_targets` / `texture_targets` are consumed in `pending_copies`
/// order.
[[nodiscard]] RhiUploadResult record_upload_gpu_batch(IRhiCommandList& list, const RhiUploadStagingPlan& plan,
                                                      const RhiUploadRing& ring,
                                                      const std::vector<RhiGpuBufferCopyTarget>& buffer_targets,
                                                      const std::vector<RhiGpuTextureCopyTarget>& texture_targets);

/// Marks every allocation touched by `pending_copies` as submitted, then tags ring spans with `fence`.
[[nodiscard]] RhiUploadResult mark_pending_allocations_submitted(RhiUploadStagingPlan& plan, RhiUploadRing& ring,
                                                                 FenceValue fence);

} // namespace mirakana::rhi
