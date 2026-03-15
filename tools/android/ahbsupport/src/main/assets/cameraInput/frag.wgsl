@group(0) @binding(0) var samp: sampler;
@group(0) @binding(1) var tex: texture_2d<f32>;

@fragment
fn main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {

    // Simply read the pixel color from the texture at the current U/V coordinate
    let color = textureSample(tex, samp, uv);

    return vec4<f32>(color.rgb, 1.0);
}