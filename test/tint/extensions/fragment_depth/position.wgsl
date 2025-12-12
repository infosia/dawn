requires fragment_depth;

@vertex
fn main() -> @builtin(position) vec4f { return vec4f(0.5, 0.5, 0.5, 0.5); }

@fragment
fn any(@builtin(position) position : vec4f) -> @builtin(frag_depth, any) f32 { return 1.0; }

@fragment
fn less(@builtin(position) position : vec4f) -> @builtin(frag_depth, less) f32 { return 1.0; }

@fragment
fn greater(@builtin(position) position : vec4f) -> @builtin(frag_depth, greater) f32 { return 0.0; }
