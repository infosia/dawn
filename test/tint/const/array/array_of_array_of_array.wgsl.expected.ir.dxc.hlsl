
[numthreads(1, 1, 1)]
void main() {
  uint q = 0u;
  uint v[2][2][2] = {{{0u, 1u}, {2u, 3u}}, {{4u, 5u}, {6u, 7u}}};
  uint w = v[min(q, 1u)][min(q, 1u)][min(q, 1u)];
}

