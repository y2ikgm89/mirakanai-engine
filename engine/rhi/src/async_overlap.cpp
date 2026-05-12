// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/rhi.hpp"

#include <cstdint>

namespace mirakana::rhi {

RhiAsyncOverlapReadinessDiagnostics
diagnose_compute_graphics_async_overlap_readiness(const RhiStats& stats,
                                                  std::uint64_t gpu_timestamp_ticks_per_second) noexcept {
    RhiAsyncOverlapReadinessDiagnostics diagnostics{};
    diagnostics.compute_queue_submitted =
        stats.compute_queue_submits > 0 && stats.last_compute_submitted_fence_value > 0;
    const bool graphics_queue_wait_source_is_compute = stats.last_graphics_queue_wait_fence_queue == QueueKind::compute;
    diagnostics.graphics_queue_waited_for_compute =
        diagnostics.compute_queue_submitted && graphics_queue_wait_source_is_compute &&
        stats.last_graphics_queue_wait_fence_value > 0 &&
        stats.last_graphics_queue_wait_fence_value == stats.last_compute_submitted_fence_value;
    const bool graphics_submit_order_is_known =
        stats.last_graphics_submit_sequence > 0 && stats.last_graphics_queue_wait_sequence > 0;
    diagnostics.graphics_queue_submitted_after_wait =
        stats.graphics_queue_submits > 0 && stats.last_graphics_submitted_fence_value > 0 &&
        (!diagnostics.graphics_queue_waited_for_compute ||
         (graphics_submit_order_is_known
              ? stats.last_graphics_submit_sequence > stats.last_graphics_queue_wait_sequence
              : stats.last_graphics_submitted_fence_value > stats.last_graphics_queue_wait_fence_value));
    diagnostics.same_frame_graphics_wait_serializes_compute = diagnostics.compute_queue_submitted &&
                                                              diagnostics.graphics_queue_waited_for_compute &&
                                                              diagnostics.graphics_queue_submitted_after_wait;
    diagnostics.gpu_timestamps_available = gpu_timestamp_ticks_per_second > 0;
    diagnostics.last_compute_submitted_fence_value = stats.last_compute_submitted_fence_value;
    diagnostics.last_graphics_queue_wait_fence_value = stats.last_graphics_queue_wait_fence_value;
    diagnostics.last_graphics_queue_wait_fence_queue = stats.last_graphics_queue_wait_fence_queue;
    diagnostics.last_graphics_submitted_fence_value = stats.last_graphics_submitted_fence_value;
    diagnostics.last_graphics_queue_wait_sequence = stats.last_graphics_queue_wait_sequence;
    diagnostics.last_graphics_submit_sequence = stats.last_graphics_submit_sequence;

    if (stats.compute_queue_submits == 0 && stats.graphics_queue_submits == 0 && stats.queue_waits == 0) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::not_requested;
        return diagnostics;
    }

    if (diagnostics.same_frame_graphics_wait_serializes_compute) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::not_proven_serial_dependency;
        return diagnostics;
    }

    if (!diagnostics.compute_queue_submitted || !diagnostics.graphics_queue_submitted_after_wait) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::missing_queue_evidence;
        return diagnostics;
    }

    if (!diagnostics.gpu_timestamps_available) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::unsupported_missing_timestamp_support;
        return diagnostics;
    }

    diagnostics.status = RhiAsyncOverlapReadinessStatus::ready_for_backend_private_timing;
    return diagnostics;
}

RhiAsyncOverlapReadinessDiagnostics
diagnose_pipelined_compute_graphics_async_overlap_readiness(const RhiStats& stats,
                                                            const RhiPipelinedComputeGraphicsScheduleEvidence& schedule,
                                                            std::uint64_t gpu_timestamp_ticks_per_second) noexcept {
    RhiAsyncOverlapReadinessDiagnostics diagnostics{};
    diagnostics.gpu_timestamps_available = gpu_timestamp_ticks_per_second > 0;
    diagnostics.output_slot_count = schedule.output_slot_count;
    diagnostics.current_compute_output_slot_index = schedule.current_compute_output_slot_index;
    diagnostics.graphics_consumed_output_slot_index = schedule.graphics_consumed_output_slot_index;
    diagnostics.previous_compute_submitted_fence_value = schedule.previous_compute_fence.value;
    diagnostics.current_compute_fence_value = schedule.current_compute_fence.value;
    diagnostics.last_compute_submitted_fence_value = stats.last_compute_submitted_fence_value;
    diagnostics.last_graphics_queue_wait_fence_value = stats.last_graphics_queue_wait_fence_value;
    diagnostics.last_graphics_queue_wait_fence_queue = stats.last_graphics_queue_wait_fence_queue;
    diagnostics.last_graphics_submitted_fence_value = stats.last_graphics_submitted_fence_value;
    diagnostics.last_graphics_queue_wait_sequence = stats.last_graphics_queue_wait_sequence;
    diagnostics.last_graphics_submit_sequence = stats.last_graphics_submit_sequence;

    if (stats.compute_queue_submits == 0 && stats.graphics_queue_submits == 0 && stats.queue_waits == 0 &&
        schedule.output_slot_count == 0 && schedule.previous_compute_fence.value == 0 &&
        schedule.current_compute_fence.value == 0) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::not_requested;
        return diagnostics;
    }

    diagnostics.output_ring_available = schedule.output_slot_count > 1 &&
                                        schedule.current_compute_output_slot_index < schedule.output_slot_count &&
                                        schedule.graphics_consumed_output_slot_index < schedule.output_slot_count;
    diagnostics.compute_and_graphics_use_distinct_output_slots =
        diagnostics.output_ring_available &&
        schedule.current_compute_output_slot_index != schedule.graphics_consumed_output_slot_index;
    const bool previous_compute_fence_precedes_current =
        schedule.previous_compute_fence.queue == QueueKind::compute &&
        schedule.current_compute_fence.queue == QueueKind::compute && schedule.previous_compute_fence.value > 0 &&
        schedule.current_compute_fence.value > schedule.previous_compute_fence.value;
    const bool graphics_queue_wait_source_is_compute = stats.last_graphics_queue_wait_fence_queue == QueueKind::compute;

    diagnostics.compute_queue_submitted =
        stats.compute_queue_submits > 0 && schedule.current_compute_fence.queue == QueueKind::compute &&
        schedule.current_compute_fence.value > 0 &&
        stats.last_compute_submitted_fence_value == schedule.current_compute_fence.value;
    diagnostics.graphics_queue_waited_for_previous_compute =
        graphics_queue_wait_source_is_compute && schedule.previous_compute_fence.value > 0 &&
        stats.last_graphics_queue_wait_fence_value == schedule.previous_compute_fence.value;
    diagnostics.graphics_queue_waited_for_current_compute =
        graphics_queue_wait_source_is_compute && schedule.current_compute_fence.value > 0 &&
        stats.last_graphics_queue_wait_fence_value == schedule.current_compute_fence.value;
    diagnostics.graphics_queue_waited_for_compute =
        diagnostics.graphics_queue_waited_for_previous_compute || diagnostics.graphics_queue_waited_for_current_compute;
    diagnostics.graphics_queue_submitted_after_wait =
        stats.graphics_queue_submits > 0 && stats.last_graphics_submitted_fence_value > 0 &&
        stats.last_graphics_queue_wait_fence_value > 0 &&
        (stats.last_graphics_submit_sequence > 0 && stats.last_graphics_queue_wait_sequence > 0
             ? stats.last_graphics_submit_sequence > stats.last_graphics_queue_wait_sequence
             : stats.last_graphics_submitted_fence_value > stats.last_graphics_queue_wait_fence_value);
    diagnostics.same_frame_graphics_wait_serializes_compute = diagnostics.compute_queue_submitted &&
                                                              diagnostics.graphics_queue_waited_for_current_compute &&
                                                              diagnostics.graphics_queue_submitted_after_wait;

    if (!diagnostics.output_ring_available || !diagnostics.compute_and_graphics_use_distinct_output_slots ||
        !previous_compute_fence_precedes_current) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::missing_pipelined_slot_evidence;
        return diagnostics;
    }

    if (diagnostics.same_frame_graphics_wait_serializes_compute) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::not_proven_serial_dependency;
        return diagnostics;
    }

    if (!diagnostics.compute_queue_submitted || !diagnostics.graphics_queue_waited_for_previous_compute ||
        !diagnostics.graphics_queue_submitted_after_wait) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::missing_queue_evidence;
        return diagnostics;
    }

    if (!diagnostics.gpu_timestamps_available) {
        diagnostics.status = RhiAsyncOverlapReadinessStatus::unsupported_missing_timestamp_support;
        return diagnostics;
    }

    diagnostics.status = RhiAsyncOverlapReadinessStatus::ready_for_backend_private_timing;
    return diagnostics;
}

} // namespace mirakana::rhi
