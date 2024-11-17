
RWByteAddressBuffer prevent_dce : register(u0);
float2 fwidthFine_ff6aa0() {
  float2 res = (abs(ddx_fine((1.0f).xx)) + abs(ddy_fine((1.0f).xx)));
  return res;
}

void fragment_main() {
  prevent_dce.Store2(0u, asuint(fwidthFine_ff6aa0()));
}

