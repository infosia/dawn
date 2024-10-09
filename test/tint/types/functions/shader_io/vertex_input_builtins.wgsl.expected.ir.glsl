SKIP: FAILED


@vertex
fn tint_symbol(@builtin(vertex_index) vertex_index : u32, @builtin(instance_index) instance_index : u32) -> @builtin(position) vec4<f32> {
  let foo : u32 = (vertex_index + instance_index);
  return vec4<f32>();
}

Failed to generate: :20:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

:20:1 error: error: instance_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
tint_push_constant_struct = struct @align(4), @block {
  tint_first_instance:u32 @offset(0)
}

$B1: {  # root
  %tint_push_constants:ptr<push_constant, tint_push_constant_struct, read> = var
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_InstanceID:ptr<__in, i32, read> = var @builtin(instance_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%tint_symbol_inner = func(%vertex_index:u32, %instance_index:u32):vec4<f32> {
  $B2: {
    %9:u32 = add %vertex_index, %instance_index
    %foo:u32 = let %9
    ret vec4<f32>(0.0f)
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %12:i32 = load %gl_VertexID
    %13:u32 = convert %12
    %14:i32 = load %gl_InstanceID
    %15:u32 = convert %14
    %16:vec4<f32> = call %tint_symbol_inner, %13, %15
    store %gl_Position, %16
    %17:f32 = swizzle %gl_Position, y
    %18:f32 = negation %17
    store_vector_element %gl_Position, 1u, %18
    %19:f32 = swizzle %gl_Position, z
    %20:f32 = swizzle %gl_Position, w
    %21:f32 = mul 2.0f, %19
    %22:f32 = sub %21, %20
    store_vector_element %gl_Position, 2u, %22
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
