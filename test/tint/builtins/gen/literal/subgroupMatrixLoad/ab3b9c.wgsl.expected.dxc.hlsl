#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> subgroupMatrixLoad_ab3b9c() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave> res = Matrix<ComponentType::F32, 8, 8, MatrixUse::A, MatrixScope::Wave>::Load(sb_ro, 4u, 32u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_ab3b9c().Store(prevent_dce, 0u, 256u, MatrixLayout::RowMajor);
}

