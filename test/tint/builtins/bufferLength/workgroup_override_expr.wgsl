override o1 : u32;
override o2 : u32;

var<workgroup> v : buffer<o1 + o2>;

@compute @workgroup_size(1)
fn main() {
  let p = bufferLength(&v);
}

