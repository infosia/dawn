@group(0) @binding(0) var<storage, read> v : buffer;

@fragment
fn main() {
  let offset = 16u;
  let p = bufferView<vec4u>(&v, offset);
}
