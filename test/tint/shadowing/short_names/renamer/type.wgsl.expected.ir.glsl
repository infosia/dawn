SKIP: FAILED


struct vec4f {
  i : i32,
}

alias vec2f = f32;

alias vec2i = bool;

@vertex
fn tint_symbol(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4<f32> {
  let s = vec4f(1);
  let f : f32 = vec2f(s.i);
  let b : bool = vec2i(f);
  return select(vec4<f32>(), vec4<f32>(1), b);
}

Failed to generate: :24:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
vec4f = struct @align(4) {
  i:i32 @offset(0)
}

$B1: {  # root
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%tint_symbol_inner = func(%VertexIndex:u32):vec4<f32> {
  $B2: {
    %s:vec4f = let vec4f(1i)
    %7:i32 = access %s, 0u
    %8:f32 = convert %7
    %f:f32 = let %8
    %10:bool = convert %f
    %b:bool = let %10
    %12:vec4<bool> = construct %b
    %13:vec4<f32> = glsl.mix vec4<f32>(0.0f), vec4<f32>(1.0f), %12
    ret %13
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %15:i32 = load %gl_VertexID
    %16:u32 = convert %15
    %17:vec4<f32> = call %tint_symbol_inner, %16
    store %gl_Position, %17
    %18:f32 = swizzle %gl_Position, y
    %19:f32 = negation %18
    store_vector_element %gl_Position, 1u, %19
    %20:f32 = swizzle %gl_Position, z
    %21:f32 = swizzle %gl_Position, w
    %22:f32 = mul 2.0f, %20
    %23:f32 = sub %22, %21
    store_vector_element %gl_Position, 2u, %23
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
