// <dawn>/test/tint/validation/extensions/atomic_vec2u_min_max/atomic_vec2u_min_max_missing_extension.error.wgsl:6:36 error: The type 'atomic<vec2u>' cannot be used without the extension 'atomic_vec2u_min_max'
// var<storage, read_write> a: atomic<vec2u>;
//                                    ^^^^^

@group(0) @binding(0)
var<storage, read_write> a: atomic<vec2u>;

@fragment
fn main() {
  atomicStoreMin(&a, vec2u(0, 0));
}
