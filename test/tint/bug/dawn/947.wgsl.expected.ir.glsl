SKIP: FAILED


struct Uniforms {
  u_scale : vec2<f32>,
  u_offset : vec2<f32>,
}

@binding(0) @group(0) var<uniform> uniforms : Uniforms;

struct VertexOutputs {
  @location(0)
  texcoords : vec2<f32>,
  @builtin(position)
  position : vec4<f32>,
}

@vertex
fn vs_main(@builtin(vertex_index) VertexIndex : u32) -> VertexOutputs {
  var texcoord = array<vec2<f32>, 3>(vec2<f32>(-(0.5), 0.0), vec2<f32>(1.5, 0.0), vec2<f32>(0.5, 2.0));
  var tint_symbol : VertexOutputs;
  tint_symbol.position = vec4<f32>(((texcoord[VertexIndex] * 2.0) - vec2<f32>(1.0, 1.0)), 0.0, 1.0);
  var flipY = (uniforms.u_scale.y < 0.0);
  if (flipY) {
    tint_symbol.texcoords = ((((texcoord[VertexIndex] * uniforms.u_scale) + uniforms.u_offset) * vec2<f32>(1.0, -(1.0))) + vec2<f32>(0.0, 1.0));
  } else {
    tint_symbol.texcoords = ((((texcoord[VertexIndex] * vec2<f32>(1.0, -(1.0))) + vec2<f32>(0.0, 1.0)) * uniforms.u_scale) + uniforms.u_offset);
  }
  return tint_symbol;
}

Failed to generate: :80:1 error: error: vertex_index must be an u32
%vs_main = @vertex func():void {
^^^^^^^^

note: # Disassembly
Uniforms = struct @align(8) {
  u_scale:vec2<f32> @offset(0)
  u_offset:vec2<f32> @offset(8)
}

VertexOutputs = struct @align(16) {
  texcoords:vec2<f32> @offset(0)
  position:vec4<f32> @offset(16)
}

tint_symbol_2 = struct @align(8), @block {
  tint_symbol_1:Uniforms @offset(0)
}

$B1: {  # root
  %1:ptr<uniform, tint_symbol_2, read> = var @binding_point(0, 0)
  %gl_VertexID:ptr<__in, i32, read> = var @builtin(vertex_index)
  %vs_main_loc0_Output:ptr<__out, vec2<f32>, write> = var @location(0)
  %gl_Position:ptr<__out, vec4<f32>, write> = var @builtin(position)
  %gl_PointSize:ptr<__out, f32, write> = var @builtin(__point_size)
}

%vs_main_inner = func(%VertexIndex:u32):VertexOutputs {
  $B2: {
    %texcoord:ptr<function, array<vec2<f32>, 3>, read_write> = var, array<vec2<f32>, 3>(vec2<f32>(-0.5f, 0.0f), vec2<f32>(1.5f, 0.0f), vec2<f32>(0.5f, 2.0f))
    %tint_symbol:ptr<function, VertexOutputs, read_write> = var
    %10:ptr<function, vec4<f32>, read_write> = access %tint_symbol, 1u
    %11:ptr<function, vec2<f32>, read_write> = access %texcoord, %VertexIndex
    %12:vec2<f32> = load %11
    %13:vec2<f32> = mul %12, 2.0f
    %14:vec2<f32> = sub %13, vec2<f32>(1.0f)
    %15:vec4<f32> = construct %14, 0.0f, 1.0f
    store %10, %15
    %16:ptr<uniform, Uniforms, read> = access %1, 0u
    %17:ptr<uniform, vec2<f32>, read> = access %16, 0u
    %18:f32 = load_vector_element %17, 1u
    %19:bool = lt %18, 0.0f
    %flipY:ptr<function, bool, read_write> = var, %19
    %21:bool = load %flipY
    if %21 [t: $B3, f: $B4] {  # if_1
      $B3: {  # true
        %22:ptr<function, vec2<f32>, read_write> = access %tint_symbol, 0u
        %23:ptr<function, vec2<f32>, read_write> = access %texcoord, %VertexIndex
        %24:vec2<f32> = load %23
        %25:ptr<uniform, Uniforms, read> = access %1, 0u
        %26:ptr<uniform, vec2<f32>, read> = access %25, 0u
        %27:vec2<f32> = load %26
        %28:vec2<f32> = mul %24, %27
        %29:ptr<uniform, Uniforms, read> = access %1, 0u
        %30:ptr<uniform, vec2<f32>, read> = access %29, 1u
        %31:vec2<f32> = load %30
        %32:vec2<f32> = add %28, %31
        %33:vec2<f32> = mul %32, vec2<f32>(1.0f, -1.0f)
        %34:vec2<f32> = add %33, vec2<f32>(0.0f, 1.0f)
        store %22, %34
        exit_if  # if_1
      }
      $B4: {  # false
        %35:ptr<function, vec2<f32>, read_write> = access %tint_symbol, 0u
        %36:ptr<function, vec2<f32>, read_write> = access %texcoord, %VertexIndex
        %37:vec2<f32> = load %36
        %38:vec2<f32> = mul %37, vec2<f32>(1.0f, -1.0f)
        %39:vec2<f32> = add %38, vec2<f32>(0.0f, 1.0f)
        %40:ptr<uniform, Uniforms, read> = access %1, 0u
        %41:ptr<uniform, vec2<f32>, read> = access %40, 0u
        %42:vec2<f32> = load %41
        %43:vec2<f32> = mul %39, %42
        %44:ptr<uniform, Uniforms, read> = access %1, 0u
        %45:ptr<uniform, vec2<f32>, read> = access %44, 1u
        %46:vec2<f32> = load %45
        %47:vec2<f32> = add %43, %46
        store %35, %47
        exit_if  # if_1
      }
    }
    %48:VertexOutputs = load %tint_symbol
    ret %48
  }
}
%vs_main = @vertex func():void {
  $B5: {
    %50:i32 = load %gl_VertexID
    %51:u32 = convert %50
    %52:VertexOutputs = call %vs_main_inner, %51
    %53:vec2<f32> = access %52, 0u
    store %vs_main_loc0_Output, %53
    %54:vec4<f32> = access %52, 1u
    store %gl_Position, %54
    %55:f32 = swizzle %gl_Position, y
    %56:f32 = negation %55
    store_vector_element %gl_Position, 1u, %56
    %57:f32 = swizzle %gl_Position, z
    %58:f32 = swizzle %gl_Position, w
    %59:f32 = mul 2.0f, %57
    %60:f32 = sub %59, %58
    store_vector_element %gl_Position, 2u, %60
    store %gl_PointSize, 1.0f
    ret
  }
}

#version 310 es
precision highp float;
precision highp int;

bool continue_execution = true;
layout(location = 0) in vec2 fs_main_loc0_Input;
layout(location = 0) out vec4 fs_main_loc0_Output;
vec4 fs_main_inner(vec2 texcoord) {
  vec2 clampedTexcoord = clamp(texcoord, vec2(0.0f), vec2(1.0f));
  if (!(all(equal(clampedTexcoord, texcoord)))) {
    continue_execution = false;
  }
  vec4 srcColor = vec4(0.0f);
  if (!(continue_execution)) {
    discard;
  }
  return srcColor;
}
void main() {
  fs_main_loc0_Output = fs_main_inner(fs_main_loc0_Input);
}

tint executable returned error: exit status 1
