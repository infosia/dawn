SKIP: FAILED


struct VertexInputs {
  @builtin(vertex_index)
  vertex_index : u32,
  @builtin(instance_index)
  instance_index : u32,
}

@vertex
fn tint_symbol(inputs : VertexInputs) -> @builtin(position) vec4<f32> {
  let foo : u32 = (inputs.vertex_index + inputs.instance_index);
  return vec4<f32>();
}

Failed to generate: :27:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

:27:1 error: error: instance_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
VertexInputs = struct @align(4) {
  vertex_index:u32 @offset(0)
  instance_index:u32 @offset(4)
}

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

%tint_symbol_inner = func(%inputs:VertexInputs):vec4<f32> {
  $B2: {
    %8:u32 = access %inputs, 0u
    %9:u32 = access %inputs, 1u
    %10:u32 = add %8, %9
    %foo:u32 = let %10
    ret vec4<f32>(0.0f)
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %13:i32 = load %gl_VertexID
    %14:u32 = convert %13
    %15:i32 = load %gl_InstanceID
    %16:u32 = convert %15
    %17:VertexInputs = construct %14, %16
    %18:vec4<f32> = call %tint_symbol_inner, %17
    store %gl_Position, %18
    %19:f32 = swizzle %gl_Position, y
    %20:f32 = negation %19
    store_vector_element %gl_Position, 1u, %20
    %21:f32 = swizzle %gl_Position, z
    %22:f32 = swizzle %gl_Position, w
    %23:f32 = mul 2.0f, %21
    %24:f32 = sub %23, %22
    store_vector_element %gl_Position, 2u, %24
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
