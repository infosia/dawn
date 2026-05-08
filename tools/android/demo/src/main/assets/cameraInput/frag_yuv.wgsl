@group(0) @binding(0) var samp: sampler;
@group(0) @binding(1) var myVideoTexture: texture_external; // Must be texture_external!

@fragment
fn main(@location(0) uv: vec2<f32>) -> @location(0) vec4<f32> {
    // textureSampleBaseClampToEdge is specifically required for external textures
    let color = textureSampleBaseClampToEdge(myVideoTexture, samp, uv);
    return vec4<f32>(color.rgb, 1.0);
}