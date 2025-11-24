enable chromium_experimental_dynamic_binding;

var sampled_textures : resource_table;

@fragment fn fs() {
    let t = hasBinding<texture_2d<i32>>(sampled_textures, 4);
}
