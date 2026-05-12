// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/upload_staging.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace mirakana::rhi {
namespace {

[[nodiscard]] RhiUploadDiagnostic make_diagnostic(RhiUploadDiagnosticCode code, RhiUploadAllocationHandle handle,
                                                  std::string message) {
    return RhiUploadDiagnostic{.code = code, .handle = handle, .message = std::move(message)};
}

[[nodiscard]] bool add_does_not_overflow(std::uint64_t offset, std::uint64_t size) noexcept {
    return offset <= offset + size;
}

[[nodiscard]] bool range_fits(std::uint64_t allocation_size, std::uint64_t offset, std::uint64_t size) noexcept {
    return size > 0 && add_does_not_overflow(offset, size) && offset + size <= allocation_size;
}

const RhiUploadAllocationRecord* find_allocation_record(const RhiUploadStagingPlan& plan,
                                                        RhiUploadAllocationHandle handle) noexcept {
    for (const auto& allocation : plan.allocations()) {
        if (allocation.handle == handle) {
            return &allocation;
        }
    }
    return nullptr;
}

[[nodiscard]] RhiUploadDiagnosticCode inactive_allocation_code(const RhiUploadStagingPlan& plan,
                                                               RhiUploadAllocationHandle handle) noexcept {
    for (const auto& allocation : plan.allocations()) {
        if (allocation.handle.id == handle.id) {
            return RhiUploadDiagnosticCode::stale_generation;
        }
    }
    return RhiUploadDiagnosticCode::invalid_allocation;
}

[[nodiscard]] const char* inactive_allocation_message(RhiUploadDiagnosticCode code) noexcept {
    return code == RhiUploadDiagnosticCode::stale_generation ? "RHI upload staging allocation generation is stale."
                                                             : "RHI upload staging allocation is not active.";
}

} // namespace

RhiUploadAllocationResult RhiUploadStagingPlan::allocate_staging_bytes(RhiUploadAllocationDesc desc) {
    RhiUploadAllocationResult result;
    if (desc.size_bytes == 0) {
        result.diagnostics.push_back(make_diagnostic(RhiUploadDiagnosticCode::invalid_allocation, {},
                                                     "RHI upload staging allocation size must be greater than zero."));
        return result;
    }

    const auto handle = RhiUploadAllocationHandle{.id = RhiUploadAllocationId{next_id_++}, .generation = 1};
    allocations_.push_back(RhiUploadAllocationRecord{
        .handle = handle,
        .size_bytes = desc.size_bytes,
        .debug_name = std::move(desc.debug_name),
        .submitted = false,
        .submitted_fence = FenceValue{},
    });
    result.handle = handle;
    return result;
}

RhiUploadResult RhiUploadStagingPlan::enqueue_buffer_upload(RhiBufferUploadDesc desc) {
    RhiUploadResult result;
    const auto* allocation = find_allocation(desc.staging);
    if (allocation == nullptr) {
        const auto code = inactive_allocation_code(*this, desc.staging);
        result.diagnostics.push_back(make_diagnostic(code, desc.staging, inactive_allocation_message(code)));
        return result;
    }
    if (!range_fits(allocation->size_bytes, desc.staging_offset, desc.size_bytes)) {
        result.diagnostics.push_back(make_diagnostic(RhiUploadDiagnosticCode::invalid_copy_range, desc.staging,
                                                     "RHI buffer upload copy range must fit the staging allocation."));
        return result;
    }

    pending_copies_.push_back(RhiUploadCopyRecord{
        .kind = RhiUploadCopyKind::buffer,
        .staging = desc.staging,
        .staging_offset = desc.staging_offset,
        .size_bytes = desc.size_bytes,
        .texture_format = Format::unknown,
        .texture_region = BufferTextureCopyRegion{},
        .debug_name = std::move(desc.debug_name),
    });
    return result;
}

RhiUploadResult RhiUploadStagingPlan::enqueue_texture_upload(RhiTextureUploadDesc desc) {
    RhiUploadResult result;
    const auto* allocation = find_allocation(desc.staging);
    if (allocation == nullptr) {
        const auto code = inactive_allocation_code(*this, desc.staging);
        result.diagnostics.push_back(make_diagnostic(code, desc.staging, inactive_allocation_message(code)));
        return result;
    }

    std::uint64_t required_end_offset = 0;
    try {
        required_end_offset = buffer_texture_copy_required_bytes(desc.format, desc.region);
    } catch (const std::exception&) {
        result.diagnostics.push_back(make_diagnostic(RhiUploadDiagnosticCode::invalid_copy_range, desc.staging,
                                                     "RHI texture upload footprint is invalid."));
        return result;
    }

    if (required_end_offset > allocation->size_bytes || required_end_offset <= desc.region.buffer_offset) {
        result.diagnostics.push_back(make_diagnostic(RhiUploadDiagnosticCode::invalid_copy_range, desc.staging,
                                                     "RHI texture upload copy range must fit the staging allocation."));
        return result;
    }
    const auto copy_size = required_end_offset - desc.region.buffer_offset;

    pending_copies_.push_back(RhiUploadCopyRecord{
        .kind = RhiUploadCopyKind::texture,
        .staging = desc.staging,
        .staging_offset = desc.region.buffer_offset,
        .size_bytes = copy_size,
        .texture_format = desc.format,
        .texture_region = desc.region,
        .debug_name = std::move(desc.debug_name),
    });
    return result;
}

RhiUploadResult RhiUploadStagingPlan::mark_submitted(RhiUploadAllocationHandle handle, FenceValue fence) {
    RhiUploadResult result;
    auto* allocation = find_allocation(handle);
    if (allocation == nullptr) {
        const auto code = inactive_allocation_code(*this, handle);
        result.diagnostics.push_back(make_diagnostic(code, handle, inactive_allocation_message(code)));
        return result;
    }
    if (allocation->submitted) {
        result.diagnostics.push_back(make_diagnostic(RhiUploadDiagnosticCode::already_submitted, handle,
                                                     "RHI upload staging allocation was already submitted."));
        return result;
    }

    allocation->submitted = true;
    allocation->submitted_fence = fence;
    return result;
}

std::uint32_t RhiUploadStagingPlan::retire_completed_uploads(FenceValue completed_fence) {
    std::uint32_t retired_count = 0;
    auto allocation = allocations_.begin();
    while (allocation != allocations_.end()) {
        if (allocation->submitted && allocation->submitted_fence.value <= completed_fence.value) {
            const auto handle = allocation->handle;
            const auto removed_copies = std::ranges::remove_if(
                pending_copies_, [handle](const RhiUploadCopyRecord& copy) { return copy.staging == handle; });
            pending_copies_.erase(removed_copies.begin(), removed_copies.end());
            allocation = allocations_.erase(allocation);
            ++retired_count;
            continue;
        }
        ++allocation;
    }
    return retired_count;
}

const std::vector<RhiUploadCopyRecord>& RhiUploadStagingPlan::pending_copies() const noexcept {
    return pending_copies_;
}

const std::vector<RhiUploadAllocationRecord>& RhiUploadStagingPlan::allocations() const noexcept {
    return allocations_;
}

RhiUploadAllocationRecord* RhiUploadStagingPlan::find_allocation(RhiUploadAllocationHandle handle) noexcept {
    const auto allocation = std::ranges::find_if(
        allocations_, [handle](const RhiUploadAllocationRecord& candidate) { return candidate.handle == handle; });
    return allocation == allocations_.end() ? nullptr : &*allocation;
}

const RhiUploadAllocationRecord*
RhiUploadStagingPlan::find_allocation(RhiUploadAllocationHandle handle) const noexcept {
    const auto allocation = std::ranges::find_if(
        allocations_, [handle](const RhiUploadAllocationRecord& candidate) { return candidate.handle == handle; });
    return allocation == allocations_.end() ? nullptr : &*allocation;
}

RhiUploadRing::RhiUploadRing(IRhiDevice& device, RhiUploadRingDesc desc)
    : device_{&device}, size_bytes_{desc.size_bytes},
      min_alignment_{desc.min_alignment == 0 ? 1U : desc.min_alignment} {
    if (desc.size_bytes == 0) {
        throw std::invalid_argument("rhi upload ring size_bytes must be greater than zero");
    }
    buffer_ = device.create_buffer(BufferDesc{
        .size_bytes = desc.size_bytes,
        .usage = BufferUsage::copy_source,
    });
}

std::uint64_t RhiUploadRing::align_up(std::uint64_t value, std::uint64_t alignment) noexcept {
    if (alignment == 0) {
        return value;
    }
    const auto mask = alignment - 1;
    return (value + mask) & ~mask;
}

bool RhiUploadRing::try_place(std::uint64_t size_bytes, std::uint64_t* out_begin) {
    std::vector<std::pair<std::uint64_t, std::uint64_t>> intervals;
    intervals.reserve(spans_.size());
    for (const auto& span : spans_) {
        intervals.emplace_back(span.begin, span.end);
    }
    std::ranges::sort(intervals);
    std::uint64_t gap_start = 0;
    for (const auto& interval : intervals) {
        const auto aligned = align_up(gap_start, min_alignment_);
        if (aligned + size_bytes <= interval.first && aligned + size_bytes <= size_bytes_) {
            *out_begin = aligned;
            return true;
        }
        gap_start = std::max(gap_start, interval.second);
    }
    const auto aligned = align_up(gap_start, min_alignment_);
    if (aligned + size_bytes <= size_bytes_) {
        *out_begin = aligned;
        return true;
    }
    return false;
}

RhiUploadResult RhiUploadRing::reserve_for_allocation(const RhiUploadStagingPlan& plan,
                                                      RhiUploadAllocationHandle handle, FenceValue completed_fence) {
    RhiUploadResult result;
    release_completed(completed_fence);
    if (owns_allocation(handle)) {
        result.diagnostics.push_back(make_diagnostic(RhiUploadDiagnosticCode::allocation_already_bound, handle,
                                                     "RHI upload ring already reserved this allocation handle."));
        return result;
    }
    const auto* record = find_allocation_record(plan, handle);
    if (record == nullptr) {
        const auto code = inactive_allocation_code(plan, handle);
        result.diagnostics.push_back(
            make_diagnostic(code, handle,
                            code == RhiUploadDiagnosticCode::stale_generation
                                ? "RHI upload ring reserve requires the current staging allocation generation."
                                : "RHI upload ring reserve requires an active staging allocation."));
        return result;
    }
    if (record->size_bytes == 0) {
        result.diagnostics.push_back(make_diagnostic(RhiUploadDiagnosticCode::invalid_allocation, handle,
                                                     "RHI upload ring reserve requires a non-zero allocation size."));
        return result;
    }

    std::uint64_t begin = 0;
    if (!try_place(record->size_bytes, &begin)) {
        result.diagnostics.push_back(
            make_diagnostic(RhiUploadDiagnosticCode::ring_exhausted, handle,
                            "RHI upload ring has no aligned gap for the requested staging size."));
        return result;
    }
    const auto end = begin + record->size_bytes;
    spans_.push_back(Span{.begin = begin,
                          .end = end,
                          .allocation_id_value = handle.id.value,
                          .allocation_generation = handle.generation,
                          .has_fence = false,
                          .fence = FenceValue{}});
    return result;
}

std::optional<std::uint64_t> RhiUploadRing::byte_offset_for(RhiUploadAllocationHandle handle) const noexcept {
    for (const auto& span : spans_) {
        if (span.allocation_id_value == handle.id.value && span.allocation_generation == handle.generation) {
            return span.begin;
        }
    }
    return std::nullopt;
}

bool RhiUploadRing::owns_allocation(RhiUploadAllocationHandle handle) const noexcept {
    return byte_offset_for(handle).has_value();
}

void RhiUploadRing::notify_submit(FenceValue fence) {
    for (auto& span : spans_) {
        if (!span.has_fence) {
            span.has_fence = true;
            span.fence = fence;
        }
    }
}

void RhiUploadRing::release_completed(FenceValue completed_fence) {
    const auto removed = std::ranges::remove_if(spans_, [&](const Span& span) {
        if (!span.has_fence) {
            return false;
        }
        if (span.fence.value > completed_fence.value) {
            return false;
        }
        return true;
    });
    spans_.erase(removed.begin(), removed.end());
}

RhiStagingBufferPool::RhiStagingBufferPool(IRhiDevice& device, RhiStagingBufferPoolDesc desc) : device_{&device} {
    if (desc.buffer_count == 0 || desc.chunk_size_bytes == 0) {
        throw std::invalid_argument("rhi staging buffer pool requires non-zero buffer_count and chunk_size_bytes");
    }
    buffers_.reserve(desc.buffer_count);
    free_.assign(desc.buffer_count, true);
    for (std::uint32_t i = 0; i < desc.buffer_count; ++i) {
        buffers_.push_back(device.create_buffer(BufferDesc{
            .size_bytes = desc.chunk_size_bytes,
            .usage = BufferUsage::copy_source,
        }));
    }
}

std::optional<BufferHandle> RhiStagingBufferPool::try_acquire() noexcept {
    for (std::size_t i = 0; i < buffers_.size(); ++i) {
        if (free_[i]) {
            free_[i] = false;
            return buffers_[i];
        }
    }
    return std::nullopt;
}

void RhiStagingBufferPool::release(BufferHandle buffer) {
    for (std::size_t i = 0; i < buffers_.size(); ++i) {
        if (buffers_[i].value == buffer.value) {
            free_[i] = true;
            return;
        }
    }
    throw std::invalid_argument("rhi staging buffer pool release handle is not owned by this pool");
}

RhiUploadResult validate_upload_gpu_batch(const RhiUploadStagingPlan& plan,
                                          const std::vector<RhiGpuBufferCopyTarget>& buffer_targets,
                                          const std::vector<RhiGpuTextureCopyTarget>& texture_targets) {
    RhiUploadResult result;
    std::uint32_t buffer_copies = 0;
    std::uint32_t texture_copies = 0;
    for (const auto& copy : plan.pending_copies()) {
        if (copy.kind == RhiUploadCopyKind::buffer) {
            ++buffer_copies;
        } else {
            ++texture_copies;
        }
    }
    if (buffer_copies != buffer_targets.size() || texture_copies != texture_targets.size()) {
        result.diagnostics.push_back(
            make_diagnostic(RhiUploadDiagnosticCode::gpu_batch_mismatch, {},
                            "RHI upload GPU batch target counts must match pending buffer and texture copy rows."));
    }
    return result;
}

RhiUploadResult record_upload_gpu_batch(IRhiCommandList& list, const RhiUploadStagingPlan& plan,
                                        const RhiUploadRing& ring,
                                        const std::vector<RhiGpuBufferCopyTarget>& buffer_targets,
                                        const std::vector<RhiGpuTextureCopyTarget>& texture_targets) {
    auto validated = validate_upload_gpu_batch(plan, buffer_targets, texture_targets);
    if (!validated.succeeded()) {
        return validated;
    }

    std::size_t buffer_index = 0;
    std::size_t texture_index = 0;
    for (const auto& copy : plan.pending_copies()) {
        const auto base = ring.byte_offset_for(copy.staging);
        if (!base.has_value()) {
            RhiUploadResult missing;
            missing.diagnostics.push_back(make_diagnostic(
                RhiUploadDiagnosticCode::invalid_allocation, copy.staging,
                "RHI upload GPU batch recording requires reserve_for_allocation for every staging handle."));
            return missing;
        }
        if (copy.kind == RhiUploadCopyKind::buffer) {
            const auto& target = buffer_targets[buffer_index++];
            list.copy_buffer(ring.buffer(), target.destination,
                             BufferCopyRegion{
                                 .source_offset = *base + copy.staging_offset,
                                 .destination_offset = target.destination_offset,
                                 .size_bytes = copy.size_bytes,
                             });
        } else {
            const auto& target = texture_targets[texture_index++];
            auto region = copy.texture_region;
            region.buffer_offset = *base + copy.texture_region.buffer_offset;
            list.copy_buffer_to_texture(ring.buffer(), target.destination, region);
        }
    }
    return RhiUploadResult{};
}

RhiUploadResult mark_pending_allocations_submitted(RhiUploadStagingPlan& plan, RhiUploadRing& ring, FenceValue fence) {
    RhiUploadResult result;
    std::unordered_set<std::uint64_t> seen;
    for (const auto& copy : plan.pending_copies()) {
        if (seen.insert(copy.staging.id.value).second) {
            const auto marked = plan.mark_submitted(copy.staging, fence);
            if (!marked.succeeded()) {
                return marked;
            }
        }
    }
    ring.notify_submit(fence);
    return result;
}

} // namespace mirakana::rhi
