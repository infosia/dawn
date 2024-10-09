SKIP: FAILED


enable f16;

struct VertexInputs0 {
  @builtin(vertex_index)
  vertex_index : u32,
  @location(0)
  loc0 : i32,
}

struct VertexInputs1 {
  @location(2)
  loc2 : f32,
  @location(3)
  loc3 : vec4<f32>,
  @location(5)
  loc5 : vec3<f16>,
}

@vertex
fn tint_symbol(inputs0 : VertexInputs0, @location(1) loc1 : u32, @builtin(instance_index) instance_index : u32, inputs1 : VertexInputs1, @location(4) loc4 : f16) -> @builtin(position) vec4<f32> {
  let foo : u32 = (inputs0.vertex_index + instance_index);
  let i : i32 = inputs0.loc0;
  let u : u32 = loc1;
  let f : f32 = inputs1.loc2;
  let v : vec4<f32> = inputs1.loc3;
  let x : f16 = loc4;
  let y : vec3<f16> = inputs1.loc5;
  return vec4<f32>();
}

Failed to generate: :48:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

:48:1 error: error: instance_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
VertexInputs0 = struct @align(4) {
  vertex_index:u32 @offset(0)
  loc0:i32 @offset(4)
}

VertexInputs1 = struct @align(16) {
  loc2:f32 @offset(0)
  loc3:vec4<f32> @offset(16)
  loc5:vec3<f16> @offset(32)
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
  %tint_symbol_loc2_Input:ptr<__in, f32, read> = var @location(2)
  %tint_symbol_loc3_Input:ptr<__in, vec4<f32>, read> = var @location(3)
  %tint_symbol_loc5_Input:ptr<__in, vec3<f16>, read> = var @location(5)
  %tint_symbol_loc4_Input:ptr<__in, f16, read> = var @location(4)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%tint_symbol_inner = func(%inputs0:VertexInputs0, %loc1:u32, %instance_index:u32, %inputs1:VertexInputs1, %loc4:f16):vec4<f32> {
  $B2: {
    %18:u32 = access %inputs0, 0u
    %19:u32 = add %18, %instance_index
    %foo:u32 = let %19
    %21:i32 = access %inputs0, 1u
    %i:i32 = let %21
    %u:u32 = let %loc1
    %24:f32 = access %inputs1, 0u
    %f:f32 = let %24
    %26:vec4<f32> = access %inputs1, 1u
    %v:vec4<f32> = let %26
    %x:f16 = let %loc4
    %29:vec3<f16> = access %inputs1, 2u
    %y:vec3<f16> = let %29
    ret vec4<f32>(0.0f)
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %32:i32 = load %gl_VertexID
    %33:u32 = convert %32
    %34:i32 = load %tint_symbol_loc0_Input
    %35:VertexInputs0 = construct %33, %34
    %36:u32 = load %tint_symbol_loc1_Input
    %37:i32 = load %gl_InstanceID
    %38:u32 = convert %37
    %39:f32 = load %tint_symbol_loc2_Input
    %40:vec4<f32> = load %tint_symbol_loc3_Input
    %41:vec3<f16> = load %tint_symbol_loc5_Input
    %42:VertexInputs1 = construct %39, %40, %41
    %43:f16 = load %tint_symbol_loc4_Input
    %44:vec4<f32> = call %tint_symbol_inner, %35, %36, %38, %42, %43
    store %gl_Position, %44
    %45:f32 = swizzle %gl_Position, y
    %46:f32 = negation %45
    store_vector_element %gl_Position, 1u, %46
    %47:f32 = swizzle %gl_Position, z
    %48:f32 = swizzle %gl_Position, w
    %49:f32 = mul 2.0f, %47
    %50:f32 = sub %49, %48
    store_vector_element %gl_Position, 2u, %50
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
