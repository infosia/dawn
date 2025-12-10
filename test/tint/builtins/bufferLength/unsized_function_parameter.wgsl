@group(0) @binding(0) var<storage, read_write> v : buffer;

fn foo(p : ptr<storage, buffer, read_write>) {
  let p2 = bufferLength(p);
}

@compute @workgroup_size(1)
fn main() {
  foo(&v);
}
