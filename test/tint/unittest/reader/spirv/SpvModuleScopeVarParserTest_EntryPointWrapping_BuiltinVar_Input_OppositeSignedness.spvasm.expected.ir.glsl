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
fn tint_symbol(@builtin(instance_index) x_4_param : u32) -> main_out {
  x_4 = bitcast<i32>(x_4_param);
  main_1();
  return main_out(x_1);
}

Failed to generate: :35:1 error: error: instance_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
main_out = struct @align(16) {
  x_1_1:vec4<f32> @offset(0)
}

tint_push_constant_struct = struct @align(4), @block {
  tint_first_instance:u32 @offset(0)
}

$B1: {  # root
  %x_4:ptr<private, i32, read_write> = var
  %x_1:ptr<private, vec4<f32>, read_write> = var
  %tint_push_constants:ptr<push_constant, tint_push_constant_struct, read> = var
  %gl_InstanceID:ptr<__in, i32, read> = var @builtin(instance_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%main_1 = func():void {
  $B2: {
    %8:i32 = load %x_4
    %x_2:i32 = let %8
    ret
  }
}
%tint_symbol_inner = func(%x_4_param:u32):main_out {
  $B3: {
    %12:i32 = convert %x_4_param
    store %x_4, %12
    %13:void = call %main_1
    %14:vec4<f32> = load %x_1
    %15:main_out = construct %14
    ret %15
  }
}
%tint_symbol = @vertex func():void {
  $B4: {
    %17:i32 = load %gl_InstanceID
    %18:u32 = convert %17
    %19:main_out = call %tint_symbol_inner, %18
    %20:vec4<f32> = access %19, 0u
    store %gl_Position, %20
    %21:f32 = swizzle %gl_Position, y
    %22:f32 = negation %21
    store_vector_element %gl_Position, 1u, %22
    %23:f32 = swizzle %gl_Position, z
    %24:f32 = swizzle %gl_Position, w
    %25:f32 = mul 2.0f, %23
    %26:f32 = sub %25, %24
    store_vector_element %gl_Position, 2u, %26
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
