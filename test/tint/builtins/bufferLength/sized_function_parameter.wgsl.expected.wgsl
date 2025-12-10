var<workgroup> v : buffer<64>;

fn foo(p : ptr<workgroup, buffer<64>>) {
  let p2 = bufferLength(p);
}

@compute @workgroup_size(1)
fn main() {
  foo(&(v));
}
