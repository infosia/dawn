
[numthreads(1, 1, 1)]
void main() {
  uint q = 0u;
  uint2 v[2] = {uint2(1u, 2u), uint2(3u, 4u)};
  uint w = v[min(q, 1u)].x;
}

