enable subgroups;

override wgsize : u32;

@compute @workgroup_size(1)
fn main() {
  let val : f32 = 2.0;
  let subadd : f32 = subgroupShuffle(val, wgsize);
}
