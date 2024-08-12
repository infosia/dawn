SKIP: FAILED


enable subgroups;
enable subgroups_f16;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : f16;

fn subgroupBroadcast_7ae924() -> f16 {
  var arg_0 = 1.0h;
  var arg_1 = 1i;
  var res : f16 = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_7ae924();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_7ae924();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/7ae924.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;
enable subgroups_f16;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : f16;

fn subgroupBroadcast_7ae924() -> f16 {
  var arg_0 = 1.0h;
  var arg_1 = 1i;
  var res : f16 = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_7ae924();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_7ae924();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/7ae924.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

