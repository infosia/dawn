
enable subgroups;

const wgsize : i32 = 128i;

const lane = -(wgsize);

@compute @workgroup_size(1)
fn main() {
  let val : f32 = 2.0;
  let subadd : f32 = subgroupShuffle(val, lane);
}

<dawn>/test/tint/builtins/compute_subgroup_shuffle_fail_override.wgsl:11:45 error: the sourceLaneIndex argument of subgroupShuffle must be greater than or equal to zero
    let subadd : f32 = subgroupShuffle(val, lane);
                                            ^^^^

