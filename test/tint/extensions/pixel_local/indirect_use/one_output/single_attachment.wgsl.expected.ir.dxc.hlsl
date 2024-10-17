struct PixelLocal {
  uint a;
};

struct f_outputs {
  float4 tint_symbol : SV_Target0;
};

struct f_inputs {
  float4 pos : SV_Position;
};


static PixelLocal P = (PixelLocal)0;
RasterizerOrderedTexture2D<uint4> pixel_local_a : register(u1);
void f0() {
  P.a = (P.a + 9u);
}

void f1() {
  f0();
  P.a = (P.a + 8u);
}

void f2() {
  P.a = (P.a + 7u);
  f1();
}

float4 f_inner() {
  f2();
  return (2.0f).xxxx;
}

f_outputs f(f_inputs inputs) {
  P.a = pixel_local_a.Load(uint2(inputs.pos.xy)).x;
  f_outputs v = {f_inner()};
  pixel_local_a[uint2(inputs.pos.xy)] = P.a.xxxx;
  f_outputs v_1 = v;
  return v_1;
}

