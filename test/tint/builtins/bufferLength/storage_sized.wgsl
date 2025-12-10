@group(0) @binding(0) var<storage, read> v : buffer<128>;

@compute @workgroup_size(1)
fn main() {
  let l = bufferLength(&v);
}

