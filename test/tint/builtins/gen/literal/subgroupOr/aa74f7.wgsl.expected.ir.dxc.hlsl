
RWByteAddressBuffer prevent_dce : register(u0);
uint2 subgroupOr_aa74f7() {
  uint2 res = WaveActiveBitOr((1u).xx);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, subgroupOr_aa74f7());
}

