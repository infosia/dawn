SKIP: FAILED


@vertex
fn tint_symbol(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4f {
  return vec4f(vec2f(vec2i()), 0.0, 1.0);
}

Failed to generate: :12:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
$B1: {  # root
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%tint_symbol_inner = func(%VertexIndex:u32):vec4<f32> {
  $B2: {
    ret vec4<f32>(0.0f, 0.0f, 0.0f, 1.0f)
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %7:i32 = load %gl_VertexID
    %8:u32 = convert %7
    %9:vec4<f32> = call %tint_symbol_inner, %8
    store %gl_Position, %9
    %10:f32 = swizzle %gl_Position, y
    %11:f32 = negation %10
    store_vector_element %gl_Position, 1u, %11
    %12:f32 = swizzle %gl_Position, z
    %13:f32 = swizzle %gl_Position, w
    %14:f32 = mul 2.0f, %12
    %15:f32 = sub %14, %13
    store_vector_element %gl_Position, 2u, %15
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
