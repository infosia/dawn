// flags: --overrides wgsize=128

enable subgroups;

override wgsize : i32;
override lane = -wgsize;

@compute @workgroup_size(1)
fn main() {
    let val : f32 = 2.0;
    let subadd : f32 = subgroupShuffle(val, lane);
}
