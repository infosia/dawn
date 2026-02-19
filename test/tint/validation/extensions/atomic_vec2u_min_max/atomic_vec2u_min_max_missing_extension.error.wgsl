// <dawn>/test/tint/validation/atomic_vec2u_min_max_missing_extension.error.wgsl:6:36 error: 'atomic' only supports 'i32' or 'u32' types
// var<storage, read_write> a: atomic<vec2u>;
//                                    ^^^^^

@group(0) @binding(0)
var<storage, read_write> a: atomic<vec2u>;

@fragment
fn main() {
  atomicStoreMin(&a, vec2u(0, 0));
}
