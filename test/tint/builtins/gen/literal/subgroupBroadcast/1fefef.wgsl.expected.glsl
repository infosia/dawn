SKIP: FAILED


enable subgroups;
enable subgroups_f16;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec4<f16>;

fn subgroupBroadcast_1fefef() -> vec4<f16> {
  var res : vec4<f16> = subgroupBroadcast(vec4<f16>(1.0h), 1i);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_1fefef();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_1fefef();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/1fefef.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;
enable subgroups_f16;
enable f16;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec4<f16>;

fn subgroupBroadcast_1fefef() -> vec4<f16> {
  var res : vec4<f16> = subgroupBroadcast(vec4<f16>(1.0h), 1i);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_1fefef();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_1fefef();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/1fefef.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

