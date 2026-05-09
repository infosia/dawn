#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
RWByteAddressBuffer sb_rw : register(u1);
Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> subgroupMatrixLoad_bf26bb() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> res = Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Load(sb_rw, 4u, 32u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_bf26bb().Store(prevent_dce, 0u, 256u, MatrixLayout::RowMajor);
}

