SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : i32;

fn subgroupBroadcast_26c534() -> i32 {
  var res : i32 = subgroupBroadcast(1i, 1i);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_26c534();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_26c534();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/26c534.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : i32;

fn subgroupBroadcast_26c534() -> i32 {
  var res : i32 = subgroupBroadcast(1i, 1i);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_26c534();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_26c534();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/26c534.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

