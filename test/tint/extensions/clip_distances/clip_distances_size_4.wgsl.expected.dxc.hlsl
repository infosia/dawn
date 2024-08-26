struct VertexOutputs {
  float4 position;
  float clipDistance[4];
};
struct tint_symbol {
  float4 position : SV_Position;

  float4 clipDistance : SV_ClipDistance0;
};

VertexOutputs main_inner() {
  VertexOutputs tint_symbol_1 = {float4(1.0f, 2.0f, 3.0f, 4.0f), (float[4])0};
  return tint_symbol_1;
}

tint_symbol main() {
  VertexOutputs inner_result = main_inner();
  tint_symbol wrapper_result = (tint_symbol)0;
  wrapper_result.position = inner_result.position;
  float tmp_inner_clip_distances[4] = inner_result.clipDistance;
  wrapper_result.clipDistance[0u] = tmp_inner_clip_distances[0u];
  wrapper_result.clipDistance[1u] = tmp_inner_clip_distances[1u];
  wrapper_result.clipDistance[2u] = tmp_inner_clip_distances[2u];
  wrapper_result.clipDistance[3u] = tmp_inner_clip_distances[3u];
  return wrapper_result;
}
