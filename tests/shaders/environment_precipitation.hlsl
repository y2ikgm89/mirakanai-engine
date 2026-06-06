// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "../../shaders/environment/precipitation.hlsl"

PrecipitationVertexOut precipitation_vs_main(float2 quad_position : POSITION,
                                             float2 quad_uv : TEXCOORD0,
                                             uint instance_id : SV_InstanceID,
                                             float4 instance_center_radius : TEXCOORD1,
                                             float4 instance_motion_kind : TEXCOORD2) {
    return precipitation_vs_contract(quad_position, quad_uv, instance_id, instance_center_radius, instance_motion_kind);
}

float4 precipitation_ps_main(PrecipitationVertexOut input) : SV_Target0 {
    return precipitation_ps_contract(input);
}
