struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) uv: vec2<f32>,
};

@vertex
fn main(
    @location(0) pos: vec2<f32>,
    @location(1) uv: vec2<f32>
) -> VertexOutput {
    var out: VertexOutput;

    // Convert 2D position (X, Y) into 4D screen space (X, Y, Z, W)
    // Z is 0.0 (flat), W is 1.0
    out.position = vec4<f32>(pos, 0.0, 1.0);
    out.uv = uv;

    return out;
}