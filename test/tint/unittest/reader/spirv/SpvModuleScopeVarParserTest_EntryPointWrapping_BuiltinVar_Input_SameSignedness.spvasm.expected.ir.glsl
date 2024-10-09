SKIP: FAILED


var<private> x_1 : u32;

var<private> x_4 : vec4f;

fn main_1() {
  let x_2 = x_1;
  return;
}

struct main_out {
  @builtin(position)
  x_4_1 : vec4f,
}

@vertex
fn tint_symbol(@builtin(instance_index) x_1_param : u32) -> main_out {
  x_1 = x_1_param;
  main_1();
  return main_out(x_4);
}

Failed to generate: :34:1 error: error: instance_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
main_out = struct @align(16) {
  x_4_1:vec4<f32> @offset(0)
}

tint_push_constant_struct = struct @align(4), @block {
  tint_first_instance:u32 @offset(0)
}

$B1: {  # root
  %x_1:ptr<private, u32, read_write> = var
  %x_4:ptr<private, vec4<f32>, read_write> = var
  %tint_push_constants:ptr<push_constant, tint_push_constant_struct, read> = var
  %gl_InstanceID:ptr<__in, i32, read> = var @builtin(instance_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%main_1 = func():void {
  $B2: {
    %8:u32 = load %x_1
    %x_2:u32 = let %8
    ret
  }
}
%tint_symbol_inner = func(%x_1_param:u32):main_out {
  $B3: {
    store %x_1, %x_1_param
    %12:void = call %main_1
    %13:vec4<f32> = load %x_4
    %14:main_out = construct %13
    ret %14
  }
}
%tint_symbol = @vertex func():void {
  $B4: {
    %16:i32 = load %gl_InstanceID
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
