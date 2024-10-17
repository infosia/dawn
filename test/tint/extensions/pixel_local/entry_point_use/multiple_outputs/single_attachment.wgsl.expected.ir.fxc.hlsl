struct PixelLocal {
  uint a;
};

struct Out {
  float4 x;
  float4 y;
  float4 z;
};

struct f_outputs {
  float4 Out_x : SV_Target0;
  float4 Out_y : SV_Target2;
  float4 Out_z : SV_Target3;
};

struct f_inputs {
  float4 pos : SV_Position;
};


static PixelLocal P = (PixelLocal)0;
RasterizerOrderedTexture2D<uint4> pixel_local_a : register(u1);
Out f_inner() {
  P.a = (P.a + 42u);
  Out v = {(10.0f).xxxx, (20.0f).xxxx, (30.0f).xxxx};
  return v;
}

f_outputs f(f_inputs inputs) {
  P.a = pixel_local_a.Load(uint2(inputs.pos.xy)).x;
  Out v_1 = f_inner();
  Out v_2 = v_1;
  Out v_3 = v_1;
  Out v_4 = v_1;
  f_outputs v_5 = {v_2.x, v_3.y, v_4.z};
  pixel_local_a[uint2(inputs.pos.xy)] = P.a.xxxx;
  f_outputs v_6 = v_5;
  return v_6;
}

