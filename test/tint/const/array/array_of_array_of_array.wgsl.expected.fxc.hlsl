[numthreads(1, 1, 1)]
void main() {
  uint q = 0u;
  uint tint_symbol[2][2][2] = {{{0u, 1u}, {2u, 3u}}, {{4u, 5u}, {6u, 7u}}};
  uint w = tint_symbol[min(q, 1u)][min(q, 1u)][min(q, 1u)];
  return;
}
