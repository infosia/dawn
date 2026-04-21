// AAAAAAAAAAAAAAAAAAAAAAAAAAAABAAAAAAA
@group(0) @binding(0) var<storage, read_write> out_buf: u32;

@compute @workgroup_size(1)
fn main() {
    out_buf = 42u;
}
