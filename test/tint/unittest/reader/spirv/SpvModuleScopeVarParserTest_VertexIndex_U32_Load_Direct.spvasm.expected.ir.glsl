SKIP: FAILED


var<private> x_4 : u32;

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
  x_4 = x_4_param;
  main_1();
  return main_out(x_1);
}

Failed to generate: :29:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
main_out = struct @align(16) {
  x_1_1:vec4<f32> @offset(0)
}

$B1: {  # root
  %x_4:ptr<private, u32, read_write> = var
  %x_1:ptr<private, vec4<f32>, read_write> = var
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%main_1 = func():void {
  $B2: {
    %7:u32 = load %x_4
    %x_2:u32 = let %7
    ret
  }
}
%tint_symbol_inner = func(%x_4_param:u32):main_out {
  $B3: {
    store %x_4, %x_4_param
    %11:void = call %main_1
    %12:vec4<f32> = load %x_1
    %13:main_out = construct %12
    ret %13
  }
}
%tint_symbol = @vertex func():void {
  $B4: {
    %15:i32 = load %gl_VertexID
    %16:u32 = convert %15
    %17:main_out = call %tint_symbol_inner, %16
    %18:vec4<f32> = access %17, 0u
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
