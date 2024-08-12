SKIP: FAILED


enable subgroups;
enable subgroups_f16;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec3<f16>;

fn subgroupBroadcast_6cc8ce() -> vec3<f16> {
  var arg_0 = vec3<f16>(1.0h);
  var arg_1 = 1i;
  var res : vec3<f16> = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_6cc8ce();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_6cc8ce();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/6cc8ce.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;
enable subgroups_f16;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec3<f16>;

fn subgroupBroadcast_6cc8ce() -> vec3<f16> {
  var arg_0 = vec3<f16>(1.0h);
  var arg_1 = 1i;
  var res : vec3<f16> = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_6cc8ce();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_6cc8ce();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/6cc8ce.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

