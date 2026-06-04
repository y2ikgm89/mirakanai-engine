// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "../../shaders/environment/volumetric_fog.hlsl"

[numthreads(4, 4, 4)]
void cs_main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    volumetric_fog_cs_contract(dispatch_thread_id);
}
