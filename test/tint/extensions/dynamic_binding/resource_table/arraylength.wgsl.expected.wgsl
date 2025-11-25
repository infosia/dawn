enable chromium_experimental_dynamic_binding;

var sampled_textures : resource_table;

@fragment
fn fs() {
  let t = arrayLength(sampled_textures);
}
