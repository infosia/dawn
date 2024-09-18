
Texture1D<float4> t_f : register(t0);
Texture1D<int4> t_i : register(t1);
Texture1D<uint4> t_u : register(t2);
[numthreads(1, 1, 1)]
void main() {
  Texture1D<float4> v = t_f;
  uint2 v_1 = (0u).xx;
  v.GetDimensions(uint(1), v_1[0u], v_1[1u]);
  uint fdims = v_1.x;
  Texture1D<int4> v_2 = t_i;
  uint2 v_3 = (0u).xx;
  v_2.GetDimensions(uint(1), v_3[0u], v_3[1u]);
  uint idims = v_3.x;
  Texture1D<uint4> v_4 = t_u;
  uint2 v_5 = (0u).xx;
  v_4.GetDimensions(uint(1), v_5[0u], v_5[1u]);
  uint udims = v_5.x;
}

