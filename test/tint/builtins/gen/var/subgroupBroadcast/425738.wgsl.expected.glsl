SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec3<u32>;

fn subgroupBroadcast_425738() -> vec3<u32> {
  var arg_0 = vec3<u32>(1u);
  var arg_1 = 1i;
  var res : vec3<u32> = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_425738();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_425738();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/425738.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec3<u32>;

fn subgroupBroadcast_425738() -> vec3<u32> {
  var arg_0 = vec3<u32>(1u);
  var arg_1 = 1i;
  var res : vec3<u32> = subgroupBroadcast(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_425738();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_425738();
}

Failed to generate: <dawn>/test/tint/builtins/gen/var/subgroupBroadcast/425738.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

