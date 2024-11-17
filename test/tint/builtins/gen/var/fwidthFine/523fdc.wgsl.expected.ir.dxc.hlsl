
RWByteAddressBuffer prevent_dce : register(u0);
float3 fwidthFine_523fdc() {
  float3 arg_0 = (1.0f).xxx;
  float3 v = arg_0;
  float3 res = (abs(ddx_fine(v)) + abs(ddy_fine(v)));
  return res;
}

void fragment_main() {
  prevent_dce.Store3(0u, asuint(fwidthFine_523fdc()));
}

