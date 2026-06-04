// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "../../shaders/environment/volumetric_clouds.hlsl"

VolumetricCloudVertexOut volumetric_cloud_vs_main(uint vertex_id : SV_VertexID) {
    return volumetric_cloud_vs_contract(vertex_id);
}

float4 volumetric_cloud_ps_main(VolumetricCloudVertexOut input) : SV_Target0 {
    return volumetric_cloud_ps_contract(input);
}
