SKIP: FAILED


fn vec4f() -> i32 {
  return 0;
}

fn vec2f(i : i32) -> f32 {
  return f32(i);
}

fn vec2i(f : f32) -> bool {
  return bool(f);
}

@vertex
fn tint_symbol(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4<f32> {
  return select(vec4<f32>(), vec4<f32>(1), vec2i(vec2f(vec4f())));
}

Failed to generate: :34:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
$B1: {  # root
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%vec4f = func():i32 {
  $B2: {
    ret 0i
  }
}
%vec2f = func(%i:i32):f32 {
  $B3: {
    %7:f32 = convert %i
    ret %7
  }
}
%vec2i = func(%f:f32):bool {
  $B4: {
    %10:bool = convert %f
    ret %10
  }
}
%tint_symbol_inner = func(%VertexIndex:u32):vec4<f32> {
  $B5: {
    %13:i32 = call %vec4f
    %14:f32 = call %vec2f, %13
    %15:bool = call %vec2i, %14
    %16:vec4<bool> = construct %15
    %17:vec4<f32> = glsl.mix vec4<f32>(0.0f), vec4<f32>(1.0f), %16
    ret %17
  }
}
%tint_symbol = @vertex func():void {
  $B6: {
    %19:i32 = load %gl_VertexID
    %20:u32 = convert %19
    %21:vec4<f32> = call %tint_symbol_inner, %20
    store %gl_Position, %21
    %22:f32 = swizzle %gl_Position, y
    %23:f32 = negation %22
    store_vector_element %gl_Position, 1u, %23
    %24:f32 = swizzle %gl_Position, z
    %25:f32 = swizzle %gl_Position, w
    %26:f32 = mul 2.0f, %24
    %27:f32 = sub %26, %25
    store_vector_element %gl_Position, 2u, %27
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
