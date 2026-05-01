struct f_inputs {
  uint3 v : SV_DispatchThreadID;
};


uint tint_mod_u32(uint lhs, uint rhs) {
  return (lhs - ((lhs / select((rhs == 0u), 1u, rhs)) * select((rhs == 0u), 1u, rhs)));
}

void f_inner(uint3 v) {
  uint l = (v.x << (tint_mod_u32(v.y, 1u) & 31u));
}

[numthreads(1, 1, 1)]
void f(f_inputs inputs) {
  f_inner(inputs.v);
}

