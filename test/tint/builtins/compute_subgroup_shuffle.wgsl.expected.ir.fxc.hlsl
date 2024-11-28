SKIP: FAILED

<dawn>/test/tint/builtins/compute_subgroup_shuffle.wgsl:6:45 error: the sourceLaneIndex argument of subgroupShuffle must be greater than or equal to zero
    let subadd : f32 = subgroupShuffle(val, -128);
                                            ^^^^


tint executable returned error: exit status 1
