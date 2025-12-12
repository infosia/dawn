//
// any
//
struct any_outputs {
  float tint_symbol : SV_Depth;
};

struct any_inputs {
  float4 position : SV_Position;
};


float any_inner(float4 position) {
  return 1.0f;
}

any_outputs any(any_inputs inputs) {
  any_outputs v = {any_inner(float4(inputs.position.xyz, (1.0f / inputs.position.w)))};
  return v;
}

//
// less
//
struct less_outputs {
  float tint_symbol : SV_DepthLessEqual;
};

struct less_inputs {
  float4 position : SV_Position;
};


float less_inner(float4 position) {
  return 1.0f;
}

less_outputs less(less_inputs inputs) {
  less_outputs v = {less_inner(float4(inputs.position.xyz, (1.0f / inputs.position.w)))};
  return v;
}

//
// greater
//
struct greater_outputs {
  float tint_symbol : SV_DepthGreaterEqual;
};

struct greater_inputs {
  float4 position : SV_Position;
};


float greater_inner(float4 position) {
  return 1.0f;
}

greater_outputs greater(greater_inputs inputs) {
  greater_outputs v = {greater_inner(float4(inputs.position.xyz, (1.0f / inputs.position.w)))};
  return v;
}

