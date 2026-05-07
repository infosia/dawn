#include <dx/linalg.h>
using namespace dx::linalg;
struct S {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> l;
  Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave> r;
};

struct S_Nested {
  S s;
};


RWByteAddressBuffer buffer : register(u0);
Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> make_matrix() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m = Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f);
  return m;
}

typedef Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> ary_ret[4];
ary_ret make_array() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_array[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v[4] = m_array;
  return v;
}

typedef Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> ary_ret_1[4][4];
ary_ret_1 make_nested_array() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_1[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_nested_array[4][4] = {v_1, v_1, v_1, v_1};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_2[4][4] = m_nested_array;
  return v_2;
}

S make_struct() {
  S m_struct = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S v_3 = m_struct;
  return v_3;
}

S_Nested make_nested_struct() {
  S v_4 = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S_Nested m_nested_struct = {v_4};
  S_Nested v_5 = m_nested_struct;
  return v_5;
}

[numthreads(64, 1, 1)]
void main() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_6 = make_matrix();
  uint v_7 = 0u;
  buffer.GetDimensions(v_7);
  if ((((0u + (64u * 7u)) + 8u) <= (v_7 / 4u))) {
    v_6.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_8[4] = make_array();
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_9 = v_8[0u];
  uint v_10 = 0u;
  buffer.GetDimensions(v_10);
  if ((((0u + (64u * 7u)) + 8u) <= (v_10 / 4u))) {
    v_9.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_11[4][4] = make_nested_array();
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_12 = v_11[1u][2u];
  uint v_13 = 0u;
  buffer.GetDimensions(v_13);
  if ((((0u + (64u * 7u)) + 8u) <= (v_13 / 4u))) {
    v_12.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  S v_14 = make_struct();
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_15 = v_14.l;
  uint v_16 = 0u;
  buffer.GetDimensions(v_16);
  if ((((0u + (64u * 7u)) + 8u) <= (v_16 / 4u))) {
    v_15.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  S_Nested v_17 = make_nested_struct();
  Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave> v_18 = v_17.s.r;
  uint v_19 = 0u;
  buffer.GetDimensions(v_19);
  if ((((0u + (64u * 7u)) + 8u) <= (v_19 / 4u))) {
    v_18.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
}

