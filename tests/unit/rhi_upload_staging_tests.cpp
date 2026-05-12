// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/rhi/upload_staging.hpp"

MK_TEST("rhi upload staging plan batches copies and retires by fence") {
    mirakana::rhi::RhiUploadStagingPlan plan;
    const auto allocation = plan.allocate_staging_bytes(
        mirakana::rhi::RhiUploadAllocationDesc{.size_bytes = 128, .debug_name = "mesh-upload"});
    MK_REQUIRE(allocation.succeeded());
    MK_REQUIRE(allocation.handle.id.value == 1);

    const auto copy = plan.enqueue_buffer_upload(mirakana::rhi::RhiBufferUploadDesc{
        .staging = allocation.handle, .staging_offset = 0, .size_bytes = 64, .debug_name = "mesh-vertices"});
    MK_REQUIRE(copy.succeeded());
    const auto texture_copy = plan.enqueue_texture_upload(mirakana::rhi::RhiTextureUploadDesc{
        .staging = allocation.handle,
        .format = mirakana::rhi::Format::rgba8_unorm,
        .region =
            mirakana::rhi::BufferTextureCopyRegion{
                .buffer_offset = 64,
                .buffer_row_length = 4,
                .buffer_image_height = 4,
                .texture_offset = mirakana::rhi::Offset3D{},
                .texture_extent = mirakana::rhi::Extent3D{.width = 4, .height = 4, .depth = 1},
            },
        .debug_name = "albedo-mip0",
    });
    MK_REQUIRE(texture_copy.succeeded());
    MK_REQUIRE(plan.pending_copies().size() == 2);
    MK_REQUIRE(plan.pending_copies()[0].kind == mirakana::rhi::RhiUploadCopyKind::buffer);
    MK_REQUIRE(plan.pending_copies()[1].kind == mirakana::rhi::RhiUploadCopyKind::texture);

    MK_REQUIRE(plan.retire_completed_uploads(mirakana::rhi::FenceValue{4}) == 0);
    MK_REQUIRE(plan.mark_submitted(allocation.handle, mirakana::rhi::FenceValue{5}).succeeded());
    MK_REQUIRE(plan.retire_completed_uploads(mirakana::rhi::FenceValue{5}) == 1);
    MK_REQUIRE(plan.pending_copies().empty());
}

MK_TEST("rhi upload staging plan rejects invalid allocation and copy ranges") {
    mirakana::rhi::RhiUploadStagingPlan plan;

    const auto invalid_allocation =
        plan.allocate_staging_bytes(mirakana::rhi::RhiUploadAllocationDesc{.size_bytes = 0, .debug_name = "empty"});
    MK_REQUIRE(!invalid_allocation.succeeded());
    MK_REQUIRE(invalid_allocation.diagnostics[0].code == mirakana::rhi::RhiUploadDiagnosticCode::invalid_allocation);

    const auto allocation = plan.allocate_staging_bytes(
        mirakana::rhi::RhiUploadAllocationDesc{.size_bytes = 32, .debug_name = "small-upload"});
    MK_REQUIRE(allocation.succeeded());

    const auto invalid_copy = plan.enqueue_buffer_upload(mirakana::rhi::RhiBufferUploadDesc{
        .staging = allocation.handle,
        .staging_offset = 16,
        .size_bytes = 32,
        .debug_name = "overflow",
    });
    MK_REQUIRE(!invalid_copy.succeeded());
    MK_REQUIRE(invalid_copy.diagnostics[0].code == mirakana::rhi::RhiUploadDiagnosticCode::invalid_copy_range);
    MK_REQUIRE(plan.pending_copies().empty());
}

MK_TEST("rhi upload staging plan diagnoses unknown allocations and duplicate submission") {
    mirakana::rhi::RhiUploadStagingPlan plan;
    const mirakana::rhi::RhiUploadAllocationHandle unknown{.id = mirakana::rhi::RhiUploadAllocationId{99},
                                                           .generation = 1};

    const auto unknown_copy = plan.enqueue_buffer_upload(mirakana::rhi::RhiBufferUploadDesc{
        .staging = unknown, .staging_offset = 0, .size_bytes = 16, .debug_name = "unknown"});
    MK_REQUIRE(!unknown_copy.succeeded());
    MK_REQUIRE(unknown_copy.diagnostics[0].code == mirakana::rhi::RhiUploadDiagnosticCode::invalid_allocation);

    const auto allocation = plan.allocate_staging_bytes(
        mirakana::rhi::RhiUploadAllocationDesc{.size_bytes = 64, .debug_name = "constants"});
    MK_REQUIRE(allocation.succeeded());
    MK_REQUIRE(plan.mark_submitted(allocation.handle, mirakana::rhi::FenceValue{7}).succeeded());

    const auto duplicate = plan.mark_submitted(allocation.handle, mirakana::rhi::FenceValue{.value = 8});
    MK_REQUIRE(!duplicate.succeeded());
    MK_REQUIRE(duplicate.diagnostics[0].code == mirakana::rhi::RhiUploadDiagnosticCode::already_submitted);
}

MK_TEST("rhi upload staging diagnoses stale allocation generations") {
    mirakana::rhi::RhiUploadStagingPlan plan;
    const auto allocation = plan.allocate_staging_bytes(
        mirakana::rhi::RhiUploadAllocationDesc{.size_bytes = 64, .debug_name = "stale-source"});
    MK_REQUIRE(allocation.succeeded());

    auto stale = allocation.handle;
    ++stale.generation;

    const auto buffer_copy = plan.enqueue_buffer_upload(mirakana::rhi::RhiBufferUploadDesc{
        .staging = stale, .staging_offset = 0, .size_bytes = 16, .debug_name = "stale-buffer"});
    MK_REQUIRE(!buffer_copy.succeeded());
    MK_REQUIRE(buffer_copy.diagnostics[0].code == mirakana::rhi::RhiUploadDiagnosticCode::stale_generation);
    MK_REQUIRE(buffer_copy.diagnostics[0].handle == stale);

    const auto texture_copy = plan.enqueue_texture_upload(mirakana::rhi::RhiTextureUploadDesc{
        .staging = stale,
        .format = mirakana::rhi::Format::rgba8_unorm,
        .region =
            mirakana::rhi::BufferTextureCopyRegion{
                .buffer_offset = 0,
                .buffer_row_length = 2,
                .buffer_image_height = 2,
                .texture_offset = mirakana::rhi::Offset3D{},
                .texture_extent = mirakana::rhi::Extent3D{.width = 2, .height = 2, .depth = 1},
            },
        .debug_name = "stale-texture",
    });
    MK_REQUIRE(!texture_copy.succeeded());
    MK_REQUIRE(texture_copy.diagnostics[0].code == mirakana::rhi::RhiUploadDiagnosticCode::stale_generation);
    MK_REQUIRE(texture_copy.diagnostics[0].handle == stale);

    const auto submitted = plan.mark_submitted(stale, mirakana::rhi::FenceValue{.value = 9});
    MK_REQUIRE(!submitted.succeeded());
    MK_REQUIRE(submitted.diagnostics[0].code == mirakana::rhi::RhiUploadDiagnosticCode::stale_generation);
    MK_REQUIRE(submitted.diagnostics[0].handle == stale);
    MK_REQUIRE(plan.pending_copies().empty());
}

MK_TEST("rhi upload ring ownership requires matching allocation generation") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadStagingPlan plan;
    const auto allocation = plan.allocate_staging_bytes(
        mirakana::rhi::RhiUploadAllocationDesc{.size_bytes = 128, .debug_name = "ring-generation"});
    MK_REQUIRE(allocation.succeeded());

    mirakana::rhi::RhiUploadRing ring(device,
                                      mirakana::rhi::RhiUploadRingDesc{.size_bytes = 512, .min_alignment = 256});
    MK_REQUIRE(ring.reserve_for_allocation(plan, allocation.handle, mirakana::rhi::FenceValue{}).succeeded());
    MK_REQUIRE(ring.owns_allocation(allocation.handle));
    MK_REQUIRE(ring.byte_offset_for(allocation.handle).has_value());

    auto stale = allocation.handle;
    ++stale.generation;

    MK_REQUIRE(!ring.owns_allocation(stale));
    MK_REQUIRE(!ring.byte_offset_for(stale).has_value());

    const auto stale_reserve = ring.reserve_for_allocation(plan, stale, mirakana::rhi::FenceValue{});
    MK_REQUIRE(!stale_reserve.succeeded());
    MK_REQUIRE(stale_reserve.diagnostics[0].code == mirakana::rhi::RhiUploadDiagnosticCode::stale_generation);
    MK_REQUIRE(stale_reserve.diagnostics[0].handle == stale);
}

MK_TEST("rhi upload ring reserve fails when the ring cannot fit the staging allocation") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadStagingPlan plan;
    const auto allocation =
        plan.allocate_staging_bytes(mirakana::rhi::RhiUploadAllocationDesc{.size_bytes = 512, .debug_name = "big"});
    MK_REQUIRE(allocation.succeeded());

    mirakana::rhi::RhiUploadRing ring(device,
                                      mirakana::rhi::RhiUploadRingDesc{.size_bytes = 256, .min_alignment = 256});
    const auto reserved = ring.reserve_for_allocation(plan, allocation.handle, mirakana::rhi::FenceValue{});
    MK_REQUIRE(!reserved.succeeded());
    MK_REQUIRE(reserved.diagnostics.front().code == mirakana::rhi::RhiUploadDiagnosticCode::ring_exhausted);
}

MK_TEST("rhi upload ring and gpu batch record buffer copies on null rhi") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiUploadStagingPlan plan;
    const auto allocation =
        plan.allocate_staging_bytes(mirakana::rhi::RhiUploadAllocationDesc{.size_bytes = 256, .debug_name = "staging"});
    MK_REQUIRE(allocation.succeeded());

    mirakana::rhi::RhiUploadRing ring(device,
                                      mirakana::rhi::RhiUploadRingDesc{.size_bytes = 1024, .min_alignment = 256});
    MK_REQUIRE(ring.reserve_for_allocation(plan, allocation.handle, mirakana::rhi::FenceValue{}).succeeded());
    const auto base = ring.byte_offset_for(allocation.handle);
    MK_REQUIRE(base.has_value());

    std::vector<std::uint8_t> payload(64, std::uint8_t{0x5A});
    device.write_buffer(ring.buffer(), *base, payload);

    MK_REQUIRE(plan.enqueue_buffer_upload(mirakana::rhi::RhiBufferUploadDesc{allocation.handle, 0, 64, "vertex-slice"})
                   .succeeded());

    const auto destination = device.create_buffer(mirakana::rhi::BufferDesc{
        .size_bytes = 256, .usage = mirakana::rhi::BufferUsage::copy_destination | mirakana::rhi::BufferUsage::vertex});

    auto commands = device.begin_command_list(mirakana::rhi::QueueKind::graphics);
    const auto recorded = mirakana::rhi::record_upload_gpu_batch(
        *commands, plan, ring,
        std::vector<mirakana::rhi::RhiGpuBufferCopyTarget>{{.destination = destination, .destination_offset = 0}}, {});
    MK_REQUIRE(recorded.succeeded());
    commands->close();

    const auto fence = device.submit(*commands);
    MK_REQUIRE(mirakana::rhi::mark_pending_allocations_submitted(plan, ring, fence).succeeded());
    device.wait(fence);
    MK_REQUIRE(plan.retire_completed_uploads(fence) == 1);
    ring.release_completed(fence);

    const auto copied = device.read_buffer(destination, 0, 64);
    MK_REQUIRE(copied.size() == 64);
    MK_REQUIRE(copied.front() == 0x5A);
}

MK_TEST("rhi staging buffer pool hands out buffers until exhausted then release") {
    mirakana::rhi::NullRhiDevice device;
    mirakana::rhi::RhiStagingBufferPool pool(
        device, mirakana::rhi::RhiStagingBufferPoolDesc{.buffer_count = 2, .chunk_size_bytes = 4096});
    const auto a = pool.try_acquire();
    const auto b = pool.try_acquire();
    MK_REQUIRE(a.has_value());
    MK_REQUIRE(b.has_value());
    MK_REQUIRE(a->value != b->value);
    MK_REQUIRE(!pool.try_acquire().has_value());
    pool.release(*a);
    const auto c = pool.try_acquire();
    MK_REQUIRE(c.has_value());
    MK_REQUIRE(c->value == a->value);
}

int main() {
    return mirakana::test::run_all();
}
