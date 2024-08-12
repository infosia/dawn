SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec4<f32>;

fn subgroupBroadcast_9a72b2() -> vec4<f32> {
  var arg_0 = vec4<f32>(1.0f);
  var arg_1 = 1u;
  var res : vec4<f32> = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_9a72b2();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_9a72b2();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/9a72b2.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec4<f32>;

fn subgroupBroadcast_9a72b2() -> vec4<f32> {
  var arg_0 = vec4<f32>(1.0f);
  var arg_1 = 1u;
  var res : vec4<f32> = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_9a72b2();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_9a72b2();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/9a72b2.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

