struct A {
  uint2 b;
  uint c;
};

[numthreads(1, 1, 1)]
void main() {
  uint q = 0u;
  A tint_symbol[2] = {{uint2(1u, 2u), 3u}, {uint2(4u, 5u), 6u}};
  uint w = tint_symbol[min(q, 1u)].b.x;
  return;
}
