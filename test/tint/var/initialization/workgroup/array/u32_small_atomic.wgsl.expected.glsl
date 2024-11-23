#version 310 es

shared int zero[3];
void tint_symbol_inner(uint tint_local_index) {
  atomicExchange(zero[(tint_local_index % 3u)], 0);
  barrier();
  int v = atomicOr(zero[0], 0);
}
layout(local_size_x = 10, local_size_y = 1, local_size_z = 1) in;
void main() {
  tint_symbol_inner(gl_LocalInvocationIndex);
}
