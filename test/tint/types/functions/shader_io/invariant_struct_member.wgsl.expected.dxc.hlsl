struct Out {
  float4 pos;
};

struct main_outputs {
  precise float4 Out_pos : SV_Position;
};


Out main_inner() {
  return (Out)0;
}

main_outputs main() {
  Out v = main_inner();
  main_outputs v_1 = {v.pos};
  return v_1;
}

