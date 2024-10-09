SKIP: FAILED


var<private> x_4 : i32;

var<private> x_1 : vec4f;

fn main_1() {
  let x_2 = x_4;
  return;
}

struct main_out {
  @builtin(position)
  x_1_1 : vec4f,
}

@vertex
fn tint_symbol(@builtin(vertex_index) x_4_param : u32) -> main_out {
  x_4 = bitcast<i32>(x_4_param);
  main_1();
  return main_out(x_1);
}

Failed to generate: :30:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
main_out = struct @align(16) {
  x_1_1:vec4<f32> @offset(0)
}

$B1: {  # root
  %x_4:ptr<private, i32, read_write> = var
  %x_1:ptr<private, vec4<f32>, read_write> = var
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%main_1 = func():void {
  $B2: {
    %7:i32 = load %x_4
    %x_2:i32 = let %7
    ret
  }
}
%tint_symbol_inner = func(%x_4_param:u32):main_out {
  $B3: {
    %11:i32 = convert %x_4_param
    store %x_4, %11
    %12:void = call %main_1
    %13:vec4<f32> = load %x_1
    %14:main_out = construct %13
    ret %14
  }
}
%tint_symbol = @vertex func():void {
  $B4: {
    %16:i32 = load %gl_VertexID
    %17:u32 = convert %16
    %18:main_out = call %tint_symbol_inner, %17
    %19:vec4<f32> = access %18, 0u
    store %gl_Position, %19
    %20:f32 = swizzle %gl_Position, y
    %21:f32 = negation %20
    store_vector_element %gl_Position, 1u, %21
    %22:f32 = swizzle %gl_Position, z
    %23:f32 = swizzle %gl_Position, w
    %24:f32 = mul 2.0f, %22
    %25:f32 = sub %24, %23
    store_vector_element %gl_Position, 2u, %25
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
