#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
RWByteAddressBuffer sb_rw : register(u1);
Matrix<ComponentType::I32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> subgroupMatrixLoad_6c7422() {
  Matrix<ComponentType::I32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> res = Matrix<ComponentType::I32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave>::Load(sb_rw, 4u, 32u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_6c7422().Store(prevent_dce, 0u, 256u, MatrixLayout::RowMajor);
}

