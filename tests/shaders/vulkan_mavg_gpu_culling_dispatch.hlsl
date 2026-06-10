// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct ClusterRow {
    uint index_count_per_instance;
    uint instance_count;
    uint start_index_location;
    int base_vertex_location;
    uint start_instance_location;
    uint visible;
    uint padding0;
    uint padding1;
};

struct Constants {
    uint cluster_count;
    uint max_command_count;
    uint record_stride_bytes;
    uint pad;
};

[[vk::binding(0, 0)]] StructuredBuffer<ClusterRow> cluster_rows : register(t0, space0);
[[vk::binding(1, 0)]] RWByteAddressBuffer argument_buffer : register(u0, space0);
[[vk::binding(2, 0)]] RWByteAddressBuffer count_buffer : register(u1, space0);
[[vk::binding(3, 0)]] ConstantBuffer<Constants> constants : register(b0, space0);

[numthreads(1, 1, 1)]
void main(uint3 dispatch_id : SV_DispatchThreadID) {
    if (dispatch_id.x != 0u) {
        return;
    }

    uint write_slot = 0u;
    for (uint cluster_index = 0u; cluster_index < constants.cluster_count; ++cluster_index) {
        ClusterRow row = cluster_rows[cluster_index];
        if (row.visible == 0u) {
            continue;
        }
        if (write_slot >= constants.max_command_count) {
            break;
        }

        const uint write_offset = write_slot * constants.record_stride_bytes;
        argument_buffer.Store(write_offset + 0u, row.index_count_per_instance);
        argument_buffer.Store(write_offset + 4u, row.instance_count);
        argument_buffer.Store(write_offset + 8u, row.start_index_location);
        argument_buffer.Store(write_offset + 12u, asuint(row.base_vertex_location));
        argument_buffer.Store(write_offset + 16u, row.start_instance_location);
        write_slot += 1u;
    }

    count_buffer.Store(0u, write_slot);
}
