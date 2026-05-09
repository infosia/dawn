#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
RWByteAddressBuffer sb_rw : register(u1);
Matrix<ComponentType::U32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> subgroupMatrixLoad_18e137() {
  uint arg_1 = 1u;
  uint arg_3 = 8u;
  uint v = arg_1;
  uint v_1 = max(arg_3, 8u);
  Matrix<ComponentType::U32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> v_2 = Matrix<ComponentType::U32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave>::Splat(0u);
  if ((((v + (v_1 * 7u)) + 8u) <= 1024u)) {
    v_2 = Matrix<ComponentType::U32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave>::Load(sb_rw, (0u + (v * 4u)), (v_1 * 4u), MatrixLayout::ColMajor);
  }
  Matrix<ComponentType::U32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> res = v_2;
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_18e137().Store(prevent_dce, 0u, 256u, MatrixLayout::RowMajor);
}

