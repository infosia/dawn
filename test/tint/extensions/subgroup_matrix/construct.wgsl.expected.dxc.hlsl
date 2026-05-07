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
  uint v = 0u;
  buffer.GetDimensions(v);
  if ((((0u + (64u * 7u)) + 8u) <= (v / 4u))) {
    Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f).Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_1[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_2 = v_1[1u];
  uint v_3 = 0u;
  buffer.GetDimensions(v_3);
  if ((((0u + (64u * 7u)) + 8u) <= (v_3 / 4u))) {
    v_2.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_4[4] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_5[4][4] = {v_4, v_4, v_4, v_4};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_6 = v_5[2u][3u];
  uint v_7 = 0u;
  buffer.GetDimensions(v_7);
  if ((((0u + (64u * 7u)) + 8u) <= (v_7 / 4u))) {
    v_6.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  S v_8 = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_9 = v_8.l;
  uint v_10 = 0u;
  buffer.GetDimensions(v_10);
  if ((((0u + (64u * 7u)) + 8u) <= (v_10 / 4u))) {
    v_9.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  S v_11 = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(0.0f)};
  S_Nested v_12 = {v_11};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave> v_13 = v_12.s.r;
  uint v_14 = 0u;
  buffer.GetDimensions(v_14);
  if ((((0u + (64u * 7u)) + 8u) <= (v_14 / 4u))) {
    v_13.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  uint v_15 = 0u;
  buffer.GetDimensions(v_15);
  if ((((0u + (64u * 7u)) + 8u) <= (v_15 / 4u))) {
    Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(42.0f).Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_16[2] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(42.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(100.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_17 = v_16[1u];
  uint v_18 = 0u;
  buffer.GetDimensions(v_18);
  if ((((0u + (64u * 7u)) + 8u) <= (v_18 / 4u))) {
    v_17.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_19[2] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(42.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(100.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_20[2] = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(-7.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(-42.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_21[2][2] = {v_19, v_20};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_22 = v_21[1u][0u];
  uint v_23 = 0u;
  buffer.GetDimensions(v_23);
  if ((((0u + (64u * 7u)) + 8u) <= (v_23 / 4u))) {
    v_22.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  S v_24 = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(42.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(100.0f)};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> v_25 = v_24.l;
  uint v_26 = 0u;
  buffer.GetDimensions(v_26);
  if ((((0u + (64u * 7u)) + 8u) <= (v_26 / 4u))) {
    v_25.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
  S v_27 = {Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(42.0f), Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(100.0f)};
  S_Nested v_28 = {v_27};
  Matrix<ComponentType::F32, 8, 8, MatrixUse::B, MatrixScope::Wave> v_29 = v_28.s.r;
  uint v_30 = 0u;
  buffer.GetDimensions(v_30);
  if ((((0u + (64u * 7u)) + 8u) <= (v_30 / 4u))) {
    v_29.Store(buffer, 0u, 256u, MatrixLayout::RowMajor);
  }
}

