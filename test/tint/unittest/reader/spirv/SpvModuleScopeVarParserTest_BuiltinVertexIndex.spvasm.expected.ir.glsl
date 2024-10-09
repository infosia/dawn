SKIP: FAILED


var<private> x_2 : u32;

var<private> position_1 : vec4f;

fn main_1() {
  return;
}

struct main_out {
  @builtin(position)
  position_1_1 : vec4f,
}

@vertex
fn tint_symbol(@builtin(vertex_index) x_2_param : u32) -> main_out {
  x_2 = x_2_param;
  main_1();
  return main_out(position_1);
}

Failed to generate: :27:1 error: error: vertex_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
main_out = struct @align(16) {
  position_1_1:vec4<f32> @offset(0)
}

$B1: {  # root
  %x_2:ptr<private, u32, read_write> = var
  %position_1:ptr<private, vec4<f32>, read_write> = var
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%main_1 = func():void {
  $B2: {
    ret
  }
}
%tint_symbol_inner = func(%x_2_param:u32):main_out {
  $B3: {
    store %x_2, %x_2_param
    %9:void = call %main_1
    %10:vec4<f32> = load %position_1
    %11:main_out = construct %10
    ret %11
  }
}
%tint_symbol = @vertex func():void {
  $B4: {
    %13:i32 = load %gl_VertexID
    %14:u32 = convert %13
    %15:main_out = call %tint_symbol_inner, %14
    %16:vec4<f32> = access %15, 0u
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
