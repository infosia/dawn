
RWByteAddressBuffer b : register(u0);
uint tint_mod_u32(uint lhs, uint rhs) {
  return (lhs - ((lhs / select((rhs == 0u), 1u, rhs)) * select((rhs == 0u), 1u, rhs)));
}

[numthreads(1, 1, 1)]
void main() {
  uint i = 0u;
  {
    uint2 tint_loop_idx = (4294967295u).xx;
    while(true) {
      if (all((tint_loop_idx == (0u).xx))) {
        break;
      }
      if ((i >= b.Load(0u))) {
        break;
      }
      uint v = (min(i, 49u) * 4u);
      if ((tint_mod_u32(i, 2u) == 0u)) {
        {
          uint tint_low_inc = (tint_loop_idx.x - 1u);
          tint_loop_idx.x = tint_low_inc;
          uint tint_carry = uint((tint_low_inc == 4294967295u));
          tint_loop_idx.y = (tint_loop_idx.y - tint_carry);
          b.Store((4u + v), (b.Load((4u + v)) * 2u));
          i = (i + 1u);
        }
        continue;
      }
      b.Store((4u + v), 0u);
      {
        uint tint_low_inc = (tint_loop_idx.x - 1u);
        tint_loop_idx.x = tint_low_inc;
        uint tint_carry = uint((tint_low_inc == 4294967295u));
        tint_loop_idx.y = (tint_loop_idx.y - tint_carry);
        b.Store((4u + v), (b.Load((4u + v)) * 2u));
        i = (i + 1u);
      }
    }
  }
}

