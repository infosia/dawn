@group(1u) @binding(1u) var tex : texture_2d<f32>;

@group(2u) @binding(1u) var samp : sampler;

@fragment
fn main() {
  helper(tex, samp);
}

fn helper(t : texture_2d<f32>, s : sampler) {
}
