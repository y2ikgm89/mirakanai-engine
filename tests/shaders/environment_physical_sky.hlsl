// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "../../shaders/environment/physical_sky.hlsl"

PhysicalSkyVertexOutput vs_main(uint vertex_id : SV_VertexID) {
    return physical_sky_vs_main(vertex_id);
}

float4 ps_main(PhysicalSkyVertexOutput input) : SV_Target0 {
    return physical_sky_ps_main(input);
}
