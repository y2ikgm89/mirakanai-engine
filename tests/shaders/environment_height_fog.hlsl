// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "../../shaders/environment/height_fog.hlsl"

HeightFogVertexOutput vs_main(uint vertex_id : SV_VertexID) {
    return height_fog_vs_main(vertex_id);
}

float4 ps_main(HeightFogVertexOutput input) : SV_Target0 {
    return height_fog_ps_main(input);
}
