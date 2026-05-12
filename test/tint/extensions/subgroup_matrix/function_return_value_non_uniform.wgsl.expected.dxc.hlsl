#include <dx/linalg.h>
using namespace dx::linalg;
using Matrix_left_f32_8x8 = Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>;
using Matrix_right_f32_8x8 = Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>;
struct S {
  Matrix_left_f32_8x8 l;
  Matrix_right_f32_8x8 r;
};

struct S_Nested {
  S s;
};

struct main_inputs {
  uint idx : SV_GroupIndex;
};


RWByteAddressBuffer buffer : register(u0);
static bool non_uniform_condition = false;
Matrix_left_f32_8x8 make_matrix() {
  if (non_uniform_condition) {
    return Matrix_left_f32_8x8::Splat(1.0f);
  } else {
    return Matrix_left_f32_8x8::Splat(2.0f);
  }
  return Matrix_left_f32_8x8::Splat(0.0f);
}

typedef Matrix_left_f32_8x8 ary_ret[2];
ary_ret make_array() {
  if (non_uniform_condition) {
    Matrix_left_f32_8x8 v[2] = {Matrix_left_f32_8x8::Splat(42.0f), Matrix_left_f32_8x8::Splat(100.0f)};
    return v;
  } else {
    Matrix_left_f32_8x8 v_1[2] = {Matrix_left_f32_8x8::Splat(-7.0f), Matrix_left_f32_8x8::Splat(-42.0f)};
    return v_1;
  }
  Matrix_left_f32_8x8 v_2[2] = {Matrix_left_f32_8x8::Splat(0.0f), Matrix_left_f32_8x8::Splat(0.0f)};
  return v_2;
}

typedef Matrix_left_f32_8x8 ary_ret_1[2][2];
ary_ret_1 make_nested_array() {
  Matrix_left_f32_8x8 v_3[2] = {Matrix_left_f32_8x8::Splat(0.0f), Matrix_left_f32_8x8::Splat(0.0f)};
  Matrix_left_f32_8x8 a[2][2] = {v_3, v_3};
  Matrix_left_f32_8x8 v_4[2] = {Matrix_left_f32_8x8::Splat(0.0f), Matrix_left_f32_8x8::Splat(0.0f)};
  Matrix_left_f32_8x8 b[2][2] = {v_4, v_4};
  if (non_uniform_condition) {
    Matrix_left_f32_8x8 v_5[2] = {Matrix_left_f32_8x8::Splat(42.0f), Matrix_left_f32_8x8::Splat(100.0f)};
    Matrix_left_f32_8x8 v_6[2] = {Matrix_left_f32_8x8::Splat(-7.0f), Matrix_left_f32_8x8::Splat(-42.0f)};
    Matrix_left_f32_8x8 v_7[2][2] = {v_5, v_6};
    return v_7;
  } else {
    Matrix_left_f32_8x8 v_8[2] = {Matrix_left_f32_8x8::Splat(7.0f), Matrix_left_f32_8x8::Splat(42.0f)};
    Matrix_left_f32_8x8 v_9[2] = {Matrix_left_f32_8x8::Splat(-100.0f), Matrix_left_f32_8x8::Splat(-1.0f)};
    Matrix_left_f32_8x8 v_10[2][2] = {v_8, v_9};
    return v_10;
  }
  Matrix_left_f32_8x8 v_11[2] = {Matrix_left_f32_8x8::Splat(0.0f), Matrix_left_f32_8x8::Splat(0.0f)};
  Matrix_left_f32_8x8 v_12[2][2] = {v_11, v_11};
  return v_12;
}

S make_struct() {
  if (non_uniform_condition) {
    S v_13 = {Matrix_left_f32_8x8::Splat(42.0f), Matrix_right_f32_8x8::Splat(100.0f)};
    return v_13;
  } else {
    S v_14 = {Matrix_left_f32_8x8::Splat(-7.0f), Matrix_right_f32_8x8::Splat(-42.0f)};
    return v_14;
  }
  S v_15 = {Matrix_left_f32_8x8::Splat(0.0f), Matrix_right_f32_8x8::Splat(0.0f)};
  return v_15;
}

S_Nested make_nested_struct() {
  if (non_uniform_condition) {
    S v_16 = {Matrix_left_f32_8x8::Splat(42.0f), Matrix_right_f32_8x8::Splat(100.0f)};
    S_Nested v_17 = {v_16};
    return v_17;
  } else {
    S v_18 = {Matrix_left_f32_8x8::Splat(-7.0f), Matrix_right_f32_8x8::Splat(-42.0f)};
    S_Nested v_19 = {v_18};
    return v_19;
  }
  S v_20 = {Matrix_left_f32_8x8::Splat(0.0f), Matrix_right_f32_8x8::Splat(0.0f)};
  S_Nested v_21 = {v_20};
  return v_21;
}

void main_inner(uint idx) {
  uint v_22 = 0u;
  buffer.GetDimensions(v_22);
  non_uniform_condition = (asfloat(buffer.Load((0u + (min(idx, ((v_22 / 4u) - 1u)) * 4u)))) == 0.0f);
  Matrix_left_f32_8x8 v_23 = make_matrix();
  uint v_24 = 0u;
  buffer.GetDimensions(v_24);
  if ((((0u + (64u * 7u)) + 8u) <= (v_24 / 4u))) {
    v_23.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix_left_f32_8x8 v_25[2] = make_array();
  Matrix_left_f32_8x8 v_26 = v_25[0u];
  uint v_27 = 0u;
  buffer.GetDimensions(v_27);
  if ((((0u + (64u * 7u)) + 8u) <= (v_27 / 4u))) {
    v_26.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix_left_f32_8x8 v_28[2][2] = make_nested_array();
  Matrix_left_f32_8x8 v_29 = v_28[1u][0u];
  uint v_30 = 0u;
  buffer.GetDimensions(v_30);
  if ((((0u + (64u * 7u)) + 8u) <= (v_30 / 4u))) {
    v_29.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  S v_31 = make_struct();
  Matrix_left_f32_8x8 v_32 = v_31.l;
  uint v_33 = 0u;
  buffer.GetDimensions(v_33);
  if ((((0u + (64u * 7u)) + 8u) <= (v_33 / 4u))) {
    v_32.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  S_Nested v_34 = make_nested_struct();
  Matrix_right_f32_8x8 v_35 = v_34.s.r;
  uint v_36 = 0u;
  buffer.GetDimensions(v_36);
  if ((((0u + (64u * 7u)) + 8u) <= (v_36 / 4u))) {
    v_35.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
}

[numthreads(64, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.idx);
}

