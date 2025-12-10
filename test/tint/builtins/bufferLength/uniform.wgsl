@group(0) @binding(0) var<uniform> v : buffer<64>;

@compute @workgroup_size(1)
fn main() {
  let p = bufferLength(&v);
}

