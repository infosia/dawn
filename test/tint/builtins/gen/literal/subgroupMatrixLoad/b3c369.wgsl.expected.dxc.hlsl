#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::F32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> subgroupMatrixLoad_b3c369() {
  Matrix<ComponentType::F32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave> res = Matrix<ComponentType::F32, 8, 8, MatrixUse::Accumulator, MatrixScope::Wave>::Load(sb_ro, 4u, 32u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_b3c369().Store(prevent_dce, 0u, 256u, MatrixLayout::RowMajor);
}

