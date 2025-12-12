requires fragment_depth;

@fragment
fn any(@builtin(position) position : vec4f) -> @builtin(frag_depth, any) f32 { return 1.0; }

@fragment
fn less(@builtin(position) position : vec4f) -> @builtin(frag_depth, less) f32 { return 1.0; }

@fragment
fn greater(@builtin(position) position : vec4f) -> @builtin(frag_depth, greater) f32 { return 1.0; }
