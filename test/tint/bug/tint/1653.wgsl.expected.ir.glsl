SKIP: FAILED


const TRI_VERTICES = array(vec4(0.0, 0.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), vec4(1.0, 1.0, 0.0, 1.0));

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index : u32) -> @builtin(position) vec4<f32> {
  return TRI_VERTICES[in_vertex_index];
}

Failed to generate: :13:1 error: error: vertex_index must be an u32
%vs_main = @vertex func():void {
^^^^^^^^

note: # Disassembly
$B1: {  # root
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%vs_main_inner = func(%in_vertex_index:u32):vec4<f32> {
  $B2: {
    %6:vec4<f32> = access array<vec4<f32>, 3>(vec4<f32>(0.0f, 0.0f, 0.0f, 1.0f), vec4<f32>(0.0f, 1.0f, 0.0f, 1.0f), vec4<f32>(1.0f, 1.0f, 0.0f, 1.0f)), %in_vertex_index
    ret %6
  }
}
%vs_main = @vertex func():void {
  $B3: {
    %8:i32 = load %gl_VertexID
    %9:u32 = convert %8
    %10:vec4<f32> = call %vs_main_inner, %9
    store %gl_Position, %10
    %11:f32 = swizzle %gl_Position, y
    %12:f32 = negation %11
    store_vector_element %gl_Position, 1u, %12
    %13:f32 = swizzle %gl_Position, z
    %14:f32 = swizzle %gl_Position, w
    %15:f32 = mul 2.0f, %13
    %16:f32 = sub %15, %14
    store_vector_element %gl_Position, 2u, %16
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
