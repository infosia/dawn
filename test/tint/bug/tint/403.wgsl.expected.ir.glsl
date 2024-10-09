SKIP: FAILED


struct vertexUniformBuffer1 {
  transform1 : mat2x2<f32>,
}

struct vertexUniformBuffer2 {
  transform2 : mat2x2<f32>,
}

@group(0) @binding(0) var<uniform> x_20 : vertexUniformBuffer1;

@group(1) @binding(0) var<uniform> x_26 : vertexUniformBuffer2;

@vertex
fn tint_symbol(@builtin(vertex_index) tint_symbol_1 : u32) -> @builtin(position) vec4<f32> {
  var indexable : array<vec2<f32>, 3>;
  let x_23 : mat2x2<f32> = x_20.transform1;
  let x_28 : mat2x2<f32> = x_26.transform2;
  let x_46 : u32 = tint_symbol_1;
  indexable = array<vec2<f32>, 3>(vec2<f32>(-(1.0), 1.0), vec2<f32>(1.0, 1.0), vec2<f32>(-(1.0), -(1.0)));
  let x_51 : vec2<f32> = indexable[x_46];
  let x_52 : vec2<f32> = (mat2x2<f32>((x_23[0u] + x_28[0u]), (x_23[1u] + x_28[1u])) * x_51);
  return vec4<f32>(x_52.x, x_52.y, 0.0, 1.0);
}

Failed to generate: :56:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
vertexUniformBuffer1 = struct @align(8) {
  transform1:mat2x2<f32> @offset(0)
}

vertexUniformBuffer2 = struct @align(8) {
  transform2:mat2x2<f32> @offset(0)
}

tint_symbol_3 = struct @align(8), @block {
  tint_symbol_2:vertexUniformBuffer1 @offset(0)
}

tint_symbol_5 = struct @align(8), @block {
  tint_symbol_4:vertexUniformBuffer2 @offset(0)
}

$B1: {  # root
  %1:ptr<uniform, tint_symbol_3, read> = var @binding_point(0, 0)
  %2:ptr<uniform, tint_symbol_5, read> = var @binding_point(0, 0)
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%tint_symbol_inner = func(%tint_symbol_1:u32):vec4<f32> {
  $B2: {
    %indexable:ptr<function, array<vec2<f32>, 3>, read_write> = var
    %9:ptr<uniform, vertexUniformBuffer1, read> = access %1, 0u
    %10:ptr<uniform, mat2x2<f32>, read> = access %9, 0u
    %11:mat2x2<f32> = load %10
    %x_23:mat2x2<f32> = let %11
    %13:ptr<uniform, vertexUniformBuffer2, read> = access %2, 0u
    %14:ptr<uniform, mat2x2<f32>, read> = access %13, 0u
    %15:mat2x2<f32> = load %14
    %x_28:mat2x2<f32> = let %15
    %x_46:u32 = let %tint_symbol_1
    store %indexable, array<vec2<f32>, 3>(vec2<f32>(-1.0f, 1.0f), vec2<f32>(1.0f), vec2<f32>(-1.0f))
    %18:ptr<function, vec2<f32>, read_write> = access %indexable, %x_46
    %19:vec2<f32> = load %18
    %x_51:vec2<f32> = let %19
    %21:vec2<f32> = access %x_23, 0u
    %22:vec2<f32> = access %x_28, 0u
    %23:vec2<f32> = add %21, %22
    %24:vec2<f32> = access %x_23, 1u
    %25:vec2<f32> = access %x_28, 1u
    %26:vec2<f32> = add %24, %25
    %27:mat2x2<f32> = construct %23, %26
    %28:vec2<f32> = mul %27, %x_51
    %x_52:vec2<f32> = let %28
    %30:f32 = access %x_52, 0u
    %31:f32 = access %x_52, 1u
    %32:vec4<f32> = construct %30, %31, 0.0f, 1.0f
    ret %32
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %34:i32 = load %gl_VertexID
    %35:u32 = convert %34
    %36:vec4<f32> = call %tint_symbol_inner, %35
    store %gl_Position, %36
    %37:f32 = swizzle %gl_Position, y
    %38:f32 = negation %37
    store_vector_element %gl_Position, 1u, %38
    %39:f32 = swizzle %gl_Position, z
    %40:f32 = swizzle %gl_Position, w
    %41:f32 = mul 2.0f, %39
    %42:f32 = sub %41, %40
    store_vector_element %gl_Position, 2u, %42
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
