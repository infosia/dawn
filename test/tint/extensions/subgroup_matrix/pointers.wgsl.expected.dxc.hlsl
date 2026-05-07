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
void foo(inout Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m, inout Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_array[4], inout Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_nested_array[4][4], inout S m_struct, inout S_Nested m_nested_struct) {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v = m;
  uint v_1 = 0u;
  buffer.GetDimensions(v_1);
  if ((((0u + (64u * 7u)) + 8u) <= (v_1 / 4u))) {
    v.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_2 = m_array[0u];
  uint v_3 = 0u;
  buffer.GetDimensions(v_3);
  if ((((0u + (64u * 7u)) + 8u) <= (v_3 / 4u))) {
    v_2.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_4 = m_nested_array[1u][2u];
  uint v_5 = 0u;
  buffer.GetDimensions(v_5);
  if ((((0u + (64u * 7u)) + 8u) <= (v_5 / 4u))) {
    v_4.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_6 = m_struct.l;
  uint v_7 = 0u;
  buffer.GetDimensions(v_7);
  if ((((0u + (64u * 7u)) + 8u) <= (v_7 / 4u))) {
    v_6.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave> v_8 = m_nested_struct.s.r;
  uint v_9 = 0u;
  buffer.GetDimensions(v_9);
  if ((((0u + (64u * 7u)) + 8u) <= (v_9 / 4u))) {
    v_8.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
}

[numthreads(64, 1, 1)]
void main() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m = Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f);
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_array[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_10[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_nested_array[4][4] = {v_10, v_10, v_10, v_10};
  S m_struct = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S v_11 = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S_Nested m_nested_struct = {v_11};
  foo(m, m_array, m_nested_array, m_struct, m_nested_struct);
}

