enable chromium_experimental_dynamic_binding;

var sampled_textures : resource_table;

@fragment fn fs() {
    let texture_load = textureLoad(getBinding<texture_1d<f32>>(sampled_textures, 2), 0, 0);
}
