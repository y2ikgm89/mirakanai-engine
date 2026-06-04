// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "../../shaders/environment/precipitation.hlsl"

PrecipitationVertexOut precipitation_vs_main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID) {
    return precipitation_vs_contract(vertex_id, instance_id);
}

float4 precipitation_ps_main(PrecipitationVertexOut input) : SV_Target0 {
    return precipitation_ps_contract(input);
}
