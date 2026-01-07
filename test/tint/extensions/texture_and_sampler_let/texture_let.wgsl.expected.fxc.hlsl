
Texture2D<float4> tex : register(t0);
void main() {
  float4 res = tex.Load(int3((int(1)).xx, int(0)));
}

