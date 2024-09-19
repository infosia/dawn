
RWTexture2DArray<uint4> arg_0 : register(u0, space1);
void textureStore_6da692() {
  arg_0[int3((1).xx, int(1))] = (1u).xxxx;
}

void fragment_main() {
  textureStore_6da692();
}

[numthreads(1, 1, 1)]
void compute_main() {
  textureStore_6da692();
}

