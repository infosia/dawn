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
[numthreads(64, 1, 1)]
void main() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m = Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f);
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_array[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_nested_array[4][4] = {v, v, v, v};
  S m_struct = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S v_1 = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S_Nested m_nested_struct = {v_1};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_let = m;
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_array_let[4] = m_array;
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> m_nested_array_let[4][4] = m_nested_array;
  S m_struct_let = m_struct;
  S_Nested m_nested_struct_let = m_nested_struct;
  uint v_2 = 0u;
  buffer.GetDimensions(v_2);
  if ((((0u + (64u * 7u)) + 8u) <= (v_2 / 4u))) {
    m_let.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_3 = 0u;
  buffer.GetDimensions(v_3);
  if ((((0u + (64u * 7u)) + 8u) <= (v_3 / 4u))) {
    m_array_let[0u].Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_4 = 0u;
  buffer.GetDimensions(v_4);
  if ((((0u + (64u * 7u)) + 8u) <= (v_4 / 4u))) {
    m_nested_array_let[1u][2u].Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_5 = 0u;
  buffer.GetDimensions(v_5);
  if ((((0u + (64u * 7u)) + 8u) <= (v_5 / 4u))) {
    m_struct_let.l.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_6 = 0u;
  buffer.GetDimensions(v_6);
  if ((((0u + (64u * 7u)) + 8u) <= (v_6 / 4u))) {
    m_nested_struct_let.s.r.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
}

