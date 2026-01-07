
@group(0) @binding(0) var tex: texture_2d<f32>;
@group(0) @binding(1) var sam: sampler;

@fragment fn main() {
    let a = sam;
    let b = a;
    let c = b;
    var res: vec4f = textureSampleLevel(tex, c, vec2f(1.f), 0.f, vec2i(1i));
}
