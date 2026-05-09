#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::I8, 8, 8, MatrixUse::A, MatrixScope::Wave> subgroupMatrixLoad_e87fb2() {
  Matrix<ComponentType::I8, 8, 8, MatrixUse::A, MatrixScope::Wave> res = Matrix<ComponentType::I8, 8, 8, MatrixUse::A, MatrixScope::Wave>::Load(sb_ro, 1u, 8u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_e87fb2().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

