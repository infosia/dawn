SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : u32;

fn subgroupBroadcast_92b8ba() -> u32 {
  var arg_0 = 1u;
  var arg_1 = 1u;
  var res : u32 = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_92b8ba();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_92b8ba();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/92b8ba.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : u32;

fn subgroupBroadcast_92b8ba() -> u32 {
  var arg_0 = 1u;
  var arg_1 = 1u;
  var res : u32 = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_92b8ba();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_92b8ba();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/92b8ba.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

