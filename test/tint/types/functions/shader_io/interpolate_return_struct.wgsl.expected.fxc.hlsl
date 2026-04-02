struct Out {
  float4 pos;
  float none;
  float flat;
  float perspective_center;
  float perspective_centroid;
  float perspective_sample;
  float linear_center;
  float linear_centroid;
  float linear_sample;
};

struct main_outputs {
  float Out_none : TEXCOORD0;
  nointerpolation float Out_flat : TEXCOORD1;
  linear float Out_perspective_center : TEXCOORD2;
  linear centroid float Out_perspective_centroid : TEXCOORD3;
  linear sample float Out_perspective_sample : TEXCOORD4;
  noperspective float Out_linear_center : TEXCOORD5;
  noperspective centroid float Out_linear_centroid : TEXCOORD6;
  noperspective sample float Out_linear_sample : TEXCOORD7;
  float4 Out_pos : SV_Position;
};


Out main_inner() {
  return (Out)0;
}

main_outputs main() {
  Out v = main_inner();
  main_outputs v_1 = {v.none, v.flat, v.perspective_center, v.perspective_centroid, v.perspective_sample, v.linear_center, v.linear_centroid, v.linear_sample, v.pos};
  return v_1;
}

