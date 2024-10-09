SKIP: FAILED


@vertex
fn tint_symbol(@builtin(instance_index) b : u32) -> @builtin(position) vec4<f32> {
  return vec4<f32>(f32(b));
}

Failed to generate: :19:1 error: error: instance_index must be an u32
%tint_symbol = @vertex func():void {
^^^^^^^^^^^^

note: # Disassembly
tint_push_constant_struct = struct @align(4), @block {
  tint_first_instance:u32 @offset(0)
}

$B1: {  # root
  %tint_push_constants:ptr<push_constant, tint_push_constant_struct, read> = var
  %gl_InstanceID:ptr<__in, i32, read> = var @builtin(instance_index)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%tint_symbol_inner = func(%b:u32):vec4<f32> {
  $B2: {
    %7:f32 = convert %b
    %8:vec4<f32> = construct %7
    ret %8
  }
}
%tint_symbol = @vertex func():void {
  $B3: {
    %10:i32 = load %gl_InstanceID
    %11:u32 = convert %10
    %12:vec4<f32> = call %tint_symbol_inner, %11
    store %gl_Position, %12
    %13:f32 = swizzle %gl_Position, y
    %14:f32 = negation %13
    store_vector_element %gl_Position, 1u, %14
    %15:f32 = swizzle %gl_Position, z
    %16:f32 = swizzle %gl_Position, w
    %17:f32 = mul 2.0f, %15
    %18:f32 = sub %17, %16
    store_vector_element %gl_Position, 2u, %18
    store %gl_PointSize, 1.0f
    ret
  }
}


tint executable returned error: exit status 1
