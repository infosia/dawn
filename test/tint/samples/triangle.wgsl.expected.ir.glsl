SKIP: FAILED


const pos = array<vec2<f32>, 3>(vec2(0.0, 0.5), vec2(-(0.5), -(0.5)), vec2(0.5, -(0.5)));

@vertex
fn vtx_main(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4<f32> {
  return vec4<f32>(pos[VertexIndex], 0.0, 1.0);
}

Failed to generate: :14:1 error: error: vertex_index must be an u32
%vtx_main = @vertex func():void {
^^^^^^^^^

note: # Disassembly
$B1: {  # root
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%vtx_main_inner = func(%VertexIndex:u32):vec4<f32> {
  $B2: {
    %6:vec2<f32> = access array<vec2<f32>, 3>(vec2<f32>(0.0f, 0.5f), vec2<f32>(-0.5f), vec2<f32>(0.5f, -0.5f)), %VertexIndex
    %7:vec4<f32> = construct %6, 0.0f, 1.0f
    ret %7
  }
}
%vtx_main = @vertex func():void {
  $B3: {
    %9:i32 = load %gl_VertexID
    %10:u32 = convert %9
    %11:vec4<f32> = call %vtx_main_inner, %10
    store %gl_Position, %11
    %12:f32 = swizzle %gl_Position, y
    %13:f32 = negation %12
    store_vector_element %gl_Position, 1u, %13
    %14:f32 = swizzle %gl_Position, z
    %15:f32 = swizzle %gl_Position, w
    %16:f32 = mul 2.0f, %14
    %17:f32 = sub %16, %15
    store_vector_element %gl_Position, 2u, %17
    store %gl_PointSize, 1.0f
    ret
  }
}

#version 310 es
precision highp float;
precision highp int;

layout(location = 0) out vec4 frag_main_loc0_Output;
vec4 frag_main_inner() {
  return vec4(1.0f, 0.0f, 0.0f, 1.0f);
}
void main() {
  frag_main_loc0_Output = frag_main_inner();
}

tint executable returned error: exit status 1
