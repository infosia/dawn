SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec2<u32>;

fn subgroupBroadcast_126ecc() -> vec2<u32> {
  var res : vec2<u32> = subgroupBroadcast(vec2<u32>(1u), 1i);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_126ecc();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_126ecc();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/126ecc.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec2<u32>;

fn subgroupBroadcast_126ecc() -> vec2<u32> {
  var res : vec2<u32> = subgroupBroadcast(vec2<u32>(1u), 1i);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_126ecc();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_126ecc();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/126ecc.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

