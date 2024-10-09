SKIP: FAILED


struct Output {
  @builtin(position)
  Position : vec4<f32>,
  @location(0)
  color : vec4<f32>,
}

@vertex
fn tint_symbol(@builtin(vertex_index) VertexIndex : u32, @builtin(instance_index) InstanceIndex : u32) -> Output {
  let zv : array<vec2<f32>, 4> = array<vec2<f32>, 4>(vec2<f32>(0.2000000000000000111, 0.2000000000000000111), vec2<f32>(0.2999999999999999889, 0.2999999999999999889), vec2<f32>(-(0.10000000000000000555), -(0.10000000000000000555)), vec2<f32>(1.10000000000000008882, 1.10000000000000008882));
  let z : f32 = zv[InstanceIndex].x;
  var tint_symbol_1 : Output;
  tint_symbol_1.Position = vec4<f32>(0.5, 0.5, z, 1.0);
  let colors : array<vec4<f32>, 4> = array<vec4<f32>, 4>(vec4<f32>(1.0, 0.0, 0.0, 1.0), vec4<f32>(0.0, 1.0, 0.0, 1.0), vec4<f32>(0.0, 0.0, 1.0, 1.0), vec4<f32>(1.0, 1.0, 1.0, 1.0));
  tint_symbol_1.color = colors[InstanceIndex];
  return tint_symbol_1;
}

Failed to generate: :36:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

:36:1 error: error: instance_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
Output = struct @align(16) {
  Position:vec4<f32> @offset(0)
  color:vec4<f32> @offset(16)
}

tint_push_constant_struct = struct @align(4), @block {
  tint_first_instance:u32 @offset(0)
}

$B1: {  # root
  %tint_push_constants:ptr<push_constant, tint_push_constant_struct, read> = var
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_InstanceID:ptr<__in, i32, read> = var @builtin(instance_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %tint_symbol_loc0_Output:ptr<__out, vec4<f32>, write> = var @location(0)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%tint_symbol_inner = func(%VertexIndex:u32, %InstanceIndex:u32):Output {
  $B2: {
    %zv:array<vec2<f32>, 4> = let array<vec2<f32>, 4>(vec2<f32>(0.20000000298023223877f), vec2<f32>(0.30000001192092895508f), vec2<f32>(-0.10000000149011611938f), vec2<f32>(1.10000002384185791016f))
    %11:f32 = access %zv, %InstanceIndex, 0u
    %z:f32 = let %11
    %tint_symbol_1:ptr<function, Output, read_write> = var
    %14:ptr<function, vec4<f32>, read_write> = access %tint_symbol_1, 0u
    %15:vec4<f32> = construct 0.5f, 0.5f, %z, 1.0f
    store %14, %15
    %colors:array<vec4<f32>, 4> = let array<vec4<f32>, 4>(vec4<f32>(1.0f, 0.0f, 0.0f, 1.0f), vec4<f32>(0.0f, 1.0f, 0.0f, 1.0f), vec4<f32>(0.0f, 0.0f, 1.0f, 1.0f), vec4<f32>(1.0f))
    %17:ptr<function, vec4<f32>, read_write> = access %tint_symbol_1, 1u
    %18:vec4<f32> = access %colors, %InstanceIndex
    store %17, %18
    %19:Output = load %tint_symbol_1
    ret %19
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %21:i32 = load %gl_VertexID
    %22:u32 = convert %21
    %23:i32 = load %gl_InstanceID
    %24:u32 = convert %23
    %25:Output = call %tint_symbol_inner, %22, %24
    %26:vec4<f32> = access %25, 0u
    store %gl_Position, %26
    %27:f32 = swizzle %gl_Position, y
    %28:f32 = negation %27
    store_vector_element %gl_Position, 1u, %28
    %29:f32 = swizzle %gl_Position, z
    %30:f32 = swizzle %gl_Position, w
    %31:f32 = mul 2.0f, %29
    %32:f32 = sub %31, %30
    store_vector_element %gl_Position, 2u, %32
    %33:vec4<f32> = access %25, 1u
    store %tint_symbol_loc0_Output, %33
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
