
RWByteAddressBuffer prevent_dce : register(u0);
int2 subgroupOr_3f60e0() {
  int2 arg = (1).xx;
  int2 res = asint(WaveActiveBitOr(asuint(arg)));
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  prevent_dce.Store2(0u, asuint(subgroupOr_3f60e0()));
}

