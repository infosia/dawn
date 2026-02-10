@vertex
fn vert(@builtin(vertex_index) vertex_idx: u32) -> @builtin(position) vec4f {
  const kVertices = array(
    vec2f( 3, -1),
    vec2f(-1,  3),
    vec2f(-1, -1),
  );
  return vec4(kVertices[vertex_idx], 0, 1);
}

@group(0) @binding(0) var<uniform> inputs : array<vec4f, 1024>;

@fragment
fn frag(@builtin(position) position: vec4f) -> @location(0) vec4u {
  let t = vec2u(position.xy);
  let inv_idx = t.x % 2 + (t.y % 2) * 2;
  let q = t / 2;
  let quad_idx = q.y * 256 + q.x;
  let index = quad_idx * 4 + inv_idx;
  let input = inputs[index];
  if inv_idx == 0 { discard; }
  let v = fwidthFine(input.x);
  return bitcast<vec4u>(vec4f(v, 0, 0, 0));
}
