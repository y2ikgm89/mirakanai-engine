// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "../../shaders/environment/cloud_layer.hlsl"

CloudLayerVertexOut cloud_layer_vs_main(uint vertex_id : SV_VertexID) {
    return cloud_layer_vs_contract(vertex_id);
}

float4 cloud_layer_ps_main(CloudLayerVertexOut input) : SV_Target0 {
    return cloud_layer_ps_contract(input);
}
