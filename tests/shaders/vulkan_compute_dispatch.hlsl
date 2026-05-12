[[vk::binding(0, 0)]] RWByteAddressBuffer output_buffer : register(u0, space0);

[numthreads(4, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID) {
    output_buffer.Store(dispatch_thread_id.x * 4, 0x41u + dispatch_thread_id.x);
}
