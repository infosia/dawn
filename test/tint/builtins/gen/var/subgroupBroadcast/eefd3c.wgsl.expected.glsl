SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec3<i32>;

fn subgroupBroadcast_eefd3c() -> vec3<i32> {
  var arg_0 = vec3<i32>(1i);
  var arg_1 = 1u;
  var res : vec3<i32> = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_eefd3c();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_eefd3c();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/eefd3c.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec3<i32>;

fn subgroupBroadcast_eefd3c() -> vec3<i32> {
  var arg_0 = vec3<i32>(1i);
  var arg_1 = 1u;
  var res : vec3<i32> = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_eefd3c();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_eefd3c();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/eefd3c.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

