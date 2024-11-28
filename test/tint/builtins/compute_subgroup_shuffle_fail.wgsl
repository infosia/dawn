enable subgroups;

const lane = -128;
@compute @workgroup_size(1)
fn main() {
    let val : f32 = 2.0;
    let subadd : f32 = subgroupShuffle(val, lane);
}
