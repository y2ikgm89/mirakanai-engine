#include <metal_stdlib>

using namespace metal;

kernel void mirakanai_ios_evidence_kernel(device uint* output [[buffer(0)]]) {
    output[0] = 0x4D4B494FU;
}
