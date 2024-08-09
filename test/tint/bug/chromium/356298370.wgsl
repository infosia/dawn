@group(0) @binding(1) var texture1 : texture_2d<f32>;
@group(0) @binding(2) var texture2 : texture_2d<f32>;
@group(0) @binding(3) var sampler1 : sampler;

fn sample(t : texture_2d<f32>, s : sampler, coords : vec2<f32>, unused : texture_2d<f32>) -> vec4<f32> {
  return textureSample(t, s, coords);
}

@fragment
fn main(@location(0) coords : vec2<f32>) -> @location(0) vec4<f32> {
  var a = sample(texture1, sampler1, coords, texture1);
  var b = sample(texture1, sampler1, coords, texture2);
  return a + b;
}
