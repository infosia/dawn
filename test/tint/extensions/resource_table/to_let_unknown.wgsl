enable chromium_experimental_resource_table;

@fragment fn fs() -> @location(0) vec4f {
    let tex1: texture_2d<f32> = getResource<texture_2d<f32, filterable>>(3);
    let samp1: sampler = getResource<sampler<filtering>>(4);
    let val = textureSample(tex2, samp2, vec2f(0));

    let tex2: texture_2d<f32> = getResource<texture_2d<f32, unfilterable>>(3);
    let samp2: sampler = getResource<sampler<non_filtering>>(4);
    return val + textureSample(tex2, samp2, vec2f(0));
}
