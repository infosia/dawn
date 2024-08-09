SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : i32;

fn subgroupBroadcast_9dbb9b() -> i32 {
  var res : i32 = subgroupBroadcast(1i, 1u);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_9dbb9b();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_9dbb9b();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/9dbb9b.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : i32;

fn subgroupBroadcast_9dbb9b() -> i32 {
  var res : i32 = subgroupBroadcast(1i, 1u);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_9dbb9b();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_9dbb9b();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/9dbb9b.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

