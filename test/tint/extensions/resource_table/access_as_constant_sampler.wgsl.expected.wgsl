enable chromium_experimental_resource_table;

@fragment
fn fs() {
  let t = getResource<texture_2d<f32>>(0);
  let s = getResource<sampler>(1);
  _ = textureSample(t, s, vec2f(0));
}
