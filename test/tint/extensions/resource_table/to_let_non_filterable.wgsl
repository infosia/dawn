enable chromium_experimental_resource_table;

@fragment fn fs() -> @location(0) vec4f {
    let tex: texture_2d<f32, unfilterable> = getResource<texture_2d<f32, filterable>>(1);
    let samp: sampler<non_filtering> = getResource<sampler<filtering>>(2);
    return textureSample(tex, samp, vec2f(0));
}
