struct PixelLocal {
  uint a;
};

struct f_inputs {
  float4 pos : SV_Position;
};


static PixelLocal P = (PixelLocal)0;
RasterizerOrderedTexture2D<uint4> pixel_local_a : register(u1);
void f_inner() {
  P.a = (P.a + 42u);
}

void f(f_inputs inputs) {
  P.a = pixel_local_a.Load(uint2(inputs.pos.xy)).x;
  f_inner();
  pixel_local_a[uint2(inputs.pos.xy)] = P.a.xxxx;
}

