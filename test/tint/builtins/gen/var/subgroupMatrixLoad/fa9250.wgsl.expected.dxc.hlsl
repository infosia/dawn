#include <dx/linalg.h>
using namespace dx::linalg;

RWByteAddressBuffer prevent_dce : register(u0);
RWByteAddressBuffer sb_rw : register(u1);
Matrix<ComponentType::I32, 8, 8, MatrixUse::B, MatrixScope::Wave> subgroupMatrixLoad_fa9250() {
  uint arg_1 = 1u;
  uint arg_3 = 8u;
  uint v = arg_1;
  uint v_1 = max(arg_3, 8u);
  Matrix<ComponentType::I32, 8, 8, MatrixUse::B, MatrixScope::Wave> v_2 = Matrix<ComponentType::I32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Splat(int(0));
  if ((((v + (v_1 * 7u)) + 8u) <= 1024u)) {
    v_2 = Matrix<ComponentType::I32, 8, 8, MatrixUse::B, MatrixScope::Wave>::Load(sb_rw, (0u + (v * 4u)), (v_1 * 4u), MatrixLayout::ColMajor);
  }
  Matrix<ComponentType::I32, 8, 8, MatrixUse::B, MatrixScope::Wave> res = v_2;
  return res;
}

[numthreads(1, 1, 1)]
void compute_main() {
  subgroupMatrixLoad_fa9250().Store(prevent_dce, 0u, 256u, MatrixLayout::RowMajor);
}

