error: 
enable subgroups;

const wgsize : u32 = 1000u;

@compute @workgroup_size(1)
fn main() {
  let val : f32 = 2.0;
  let subadd : f32 = subgroupShuffle(val, wgsize);
}

<dawn>/test/tint/builtins/compute_subgroup_shuffle.wgsl:8:45 error: the sourceLaneIndex argument of subgroupShuffle must be less than 128
    let subadd : f32 = subgroupShuffle(val, wgsize);
                                            ^^^^^^


