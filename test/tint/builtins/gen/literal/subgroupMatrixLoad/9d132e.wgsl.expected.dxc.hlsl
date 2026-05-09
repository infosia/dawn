#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::I8, 8, 8, MatrixUse::B, MatrixScope::Wave> subgroupMatrixLoad_9d132e() {
  Matrix<ComponentType::I8, 8, 8, MatrixUse::B, MatrixScope::Wave> res = Matrix<ComponentType::I8, 8, 8, MatrixUse::B, MatrixScope::Wave>::Load(sb_ro, 1u, 8u, MatrixLayout::ColMajor);
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_9d132e().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

