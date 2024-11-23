var<workgroup> zero : array<atomic<i32>, 3>;

@compute @workgroup_size(10)
fn main() {
    var v = atomicLoad(&zero[0]);
}
