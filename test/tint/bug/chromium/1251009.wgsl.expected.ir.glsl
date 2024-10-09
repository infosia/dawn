SKIP: FAILED


struct VertexInputs0 {
  @builtin(vertex_index)
  vertex_index : u32,
  @location(0)
  loc0 : i32,
}

struct VertexInputs1 {
  @location(2)
  loc1 : u32,
  @location(3)
  loc3 : vec4<f32>,
}

@vertex
fn tint_symbol(inputs0 : VertexInputs0, @location(1) loc1 : u32, @builtin(instance_index) instance_index : u32, inputs1 : VertexInputs1) -> @builtin(position) vec4<f32> {
  let foo : u32 = (inputs0.vertex_index + instance_index);
  return vec4<f32>();
}

Failed to generate: :35:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

:35:1 error: error: instance_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
VertexInputs0 = struct @align(4) {
  vertex_index:u32 @offset(0)
  loc0:i32 @offset(4)
}

VertexInputs1 = struct @align(16) {
  loc1:u32 @offset(0)
  loc3:vec4<f32> @offset(16)
}

tint_push_constant_struct = struct @align(4), @block {
  tint_first_instance:u32 @offset(0)
}

$B1: {  # root
  %tint_push_constants:ptr<push_constant, tint_push_constant_struct, read> = var
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %tint_symbol_loc0_Input:ptr<__in, i32, read> = var @location(0)
  %tint_symbol_loc1_Input:ptr<__in, u32, read> = var @location(1)
  %gl_InstanceID:ptr<__in, i32, read> = var @builtin(instance_index)
  %tint_symbol_loc2_Input:ptr<__in, u32, read> = var @location(2)
  %tint_symbol_loc3_Input:ptr<__in, vec4<f32>, read> = var @location(3)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%tint_symbol_inner = func(%inputs0:VertexInputs0, %loc1:u32, %instance_index:u32, %inputs1:VertexInputs1):vec4<f32> {
  $B2: {
    %15:u32 = access %inputs0, 0u
    %16:u32 = add %15, %instance_index
    %foo:u32 = let %16
    ret vec4<f32>(0.0f)
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %19:i32 = load %gl_VertexID
    %20:u32 = convert %19
    %21:i32 = load %tint_symbol_loc0_Input
    %22:VertexInputs0 = construct %20, %21
    %23:u32 = load %tint_symbol_loc1_Input
    %24:i32 = load %gl_InstanceID
    %25:u32 = convert %24
    %26:u32 = load %tint_symbol_loc2_Input
    %27:vec4<f32> = load %tint_symbol_loc3_Input
    %28:VertexInputs1 = construct %26, %27
    %29:vec4<f32> = call %tint_symbol_inner, %22, %23, %25, %28
    store %gl_Position, %29
    %30:f32 = swizzle %gl_Position, y
    %31:f32 = negation %30
    store_vector_element %gl_Position, 1u, %31
    %32:f32 = swizzle %gl_Position, z
    %33:f32 = swizzle %gl_Position, w
    %34:f32 = mul 2.0f, %32
    %35:f32 = sub %34, %33
    store_vector_element %gl_Position, 2u, %35
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
