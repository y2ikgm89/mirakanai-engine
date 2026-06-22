// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

struct Payload {
    uint color_index;
};

groupshared Payload payload;

[numthreads(1, 1, 1)]
void task_main(uint3 dispatch_id : SV_DispatchThreadID) {
    payload.color_index = dispatch_id.x;
    DispatchMesh(1, 1, 1, payload);
}
