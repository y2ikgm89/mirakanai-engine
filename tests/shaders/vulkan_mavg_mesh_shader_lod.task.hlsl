// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct Payload {
    uint color_index;
};

[[vk::binding(0, 0)]] StructuredBuffer<uint> shader_payload : register(t0, space0);

groupshared Payload payload;

[numthreads(1, 1, 1)]
void task_main(uint3 dispatch_id : SV_DispatchThreadID) {
    payload.color_index = dispatch_id.x + (shader_payload[0] & 1u);
    DispatchMesh(1, 1, 1, payload);
}
