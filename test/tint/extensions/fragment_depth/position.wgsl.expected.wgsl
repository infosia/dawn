requires fragment_depth;

@fragment
fn any(@builtin(position) position : vec4f) -> @builtin(frag_depth) f32 {
  return 1.0;
}

@fragment
fn less(@builtin(position) position : vec4f) -> @builtin(frag_depth) f32 {
  return 1.0;
}

@fragment
fn greater(@builtin(position) position : vec4f) -> @builtin(frag_depth) f32 {
  return 1.0;
}
