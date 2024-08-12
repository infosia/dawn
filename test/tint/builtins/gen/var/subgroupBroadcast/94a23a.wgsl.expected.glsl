SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : f32;

fn subgroupBroadcast_94a23a() -> f32 {
  var arg_0 = 1.0f;
  var arg_1 = 1u;
  var res : f32 = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_94a23a();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_94a23a();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/94a23a.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : f32;

fn subgroupBroadcast_94a23a() -> f32 {
  var arg_0 = 1.0f;
  var arg_1 = 1u;
  var res : f32 = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_94a23a();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_94a23a();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/94a23a.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

