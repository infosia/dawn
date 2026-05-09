#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
ByteAddressBuffer sb_ro : register(t1);
Matrix<ComponentType::I8, 8, 8, MatrixUse::B, MatrixScope::Wave> subgroupMatrixLoad_9d132e() {
  uint arg_1 = 1u;
  uint arg_3 = 8u;
  uint v = arg_1;
  uint v_1 = max(arg_3, 8u);
  Matrix<ComponentType::I8, 8, 8, MatrixUse::B, MatrixScope::Wave> v_2 = Matrix<ComponentType::I8, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(int(0));
  if ((((v + (v_1 * 7u)) + 8u) <= 4096u)) {
    v_2 = Matrix<ComponentType::I8, 8, 8, MatrixUse::B, MatrixScope::Wave>::Load(sb_ro, (0u + (v * 1u)), (v_1 * 1u), MatrixLayout::ColMajor);
  }
  Matrix<ComponentType::I8, 8, 8, MatrixUse::B, MatrixScope::Wave> res = v_2;
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_9d132e().Store(prevent_dce, 0u, 64u, MatrixLayout::RowMajor);
}

