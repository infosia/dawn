SKIP: FAILED


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec2<u32>;

fn subgroupBroadcast_0d287f() -> vec2<u32> {
  var res : vec2<u32> = subgroupBroadcast(vec2<u32>(1u), 1u);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_0d287f();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_0d287f();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/0d287f.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^


enable subgroups;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec2<u32>;

fn subgroupBroadcast_0d287f() -> vec2<u32> {
  var res : vec2<u32> = subgroupBroadcast(vec2<u32>(1u), 1u);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = subgroupBroadcast_0d287f();
}

@compute @workgroup_size(1)
fn compute_main() {
  prevent_dce = subgroupBroadcast_0d287f();
}

Failed to generate: <dawn>/test/tint/builtins/gen/literal/subgroupBroadcast/0d287f.wgsl:41:8 error: GLSL backend does not support extension 'subgroups'
enable subgroups;
       ^^^^^^^^^

