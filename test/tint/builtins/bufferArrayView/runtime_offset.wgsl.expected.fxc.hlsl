
ByteAddressBuffer v : register(t0);
RWByteAddressBuffer v_1 : register(u1);
void main() {
  uint offset = 16u;
  uint v_2 = 0u;
  v.GetDimensions(v_2);
  bool v_3 = (v_2 < (16u + (offset & 4294967280u)));
  uint v_4 = ((v_3) ? (0u) : ((offset & 4294967280u)));
  uint v_5 = ((((v_3) ? (16u) : (16u)) / 16u) - 1u);
  v_1.Store4(0u, v.Load4(((0u + (v_4 * 1u)) + (min(uint(int(0)), v_5) * 16u))));
}

