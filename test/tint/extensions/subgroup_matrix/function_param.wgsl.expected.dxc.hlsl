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
void foo(Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m, Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_array[4], Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_nested_array[4][4], S m_struct, S_Nested m_nested_struct) {
  uint v = 0u;
  buffer.GetDimensions(v);
  if ((((0u + (64u * 7u)) + 8u) <= (v / 4u))) {
    m.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_1 = 0u;
  buffer.GetDimensions(v_1);
  if ((((0u + (64u * 7u)) + 8u) <= (v_1 / 4u))) {
    m_array[0u].Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_2 = 0u;
  buffer.GetDimensions(v_2);
  if ((((0u + (64u * 7u)) + 8u) <= (v_2 / 4u))) {
    m_nested_array[1u][2u].Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_3 = 0u;
  buffer.GetDimensions(v_3);
  if ((((0u + (64u * 7u)) + 8u) <= (v_3 / 4u))) {
    m_struct.l.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_4 = 0u;
  buffer.GetDimensions(v_4);
  if ((((0u + (64u * 7u)) + 8u) <= (v_4 / 4u))) {
    m_nested_struct.s.r.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
}

[numthreads(64, 1, 1)]
void main() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m = Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f);
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_array[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_5[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_nested_array[4][4] = {v_5, v_5, v_5, v_5};
  S m_struct = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S v_6 = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S_Nested m_nested_struct = {v_6};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_7[4] = m_array;
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_8[4][4] = m_nested_array;
  S v_9 = m_struct;
  S_Nested v_10 = m_nested_struct;
  foo(m, v_7, v_8, v_9, v_10);
}

