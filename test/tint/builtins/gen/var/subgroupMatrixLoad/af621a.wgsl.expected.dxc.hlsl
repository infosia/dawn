#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave> subgroupMatrixLoad_af621a() {
  uint arg_1 = 1u;
  uint arg_3 = 8u;
  uint v = arg_1;
  uint v_1 = max(arg_3, 8u);
  Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave> v_2 = Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave>::Splat(0u);
  if ((((v + (v_1 * 7u)) + 8u) <= 4096u)) {
    v_2 = Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave>::Load(sb_ro, (0u + (v * 1u)), (v_1 * 1u), MatrixLayout::ColMajor);
  }
  Matrix<ComponentType::U8, 8, 8, MatrixUse::A, MatrixScope::Wave> res = v_2;
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_af621a().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

