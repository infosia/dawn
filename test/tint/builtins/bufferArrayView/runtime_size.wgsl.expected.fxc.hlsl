
ByteAddressBuffer v : register(t0);
RWByteAddressBuffer v_1 : register(u1);
uint tint_div_u32(uint lhs, uint rhs) {
  return (lhs / (((rhs == 0u)) ? (1u) : (rhs)));
}

void main() {
  uint size = 16u;
  uint v_2 = (tint_div_u32(size, 16u) * 16u);
  uint v_3 = 0u;
  v.GetDimensions(v_3);
  bool v_4 = (v_3 < max(v_2, 16u));
  uint v_5 = ((v_4) ? (0u) : (0u));
  uint v_6 = ((((v_4) ? (16u) : (max(v_2, 16u))) / 16u) - 1u);
  v_1.Store4(0u, v.Load4(((0u + (v_5 * 1u)) + (min(uint(int(0)), v_6) * 16u))));
}

