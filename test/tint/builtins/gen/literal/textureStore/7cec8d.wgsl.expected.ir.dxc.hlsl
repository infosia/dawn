
RWTexture2DArray<int4> arg_0 : register(u0, space1);
void textureStore_7cec8d() {
  arg_0[int3((1).xx, int(1))] = (1).xxxx;
}

void fragment_main() {
  textureStore_7cec8d();
}

[numthreads(1, 1, 1)]
void compute_main() {
  textureStore_7cec8d();
}

