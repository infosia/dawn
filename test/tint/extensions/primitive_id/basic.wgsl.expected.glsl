SKIP: FAILED

Error parsing GLSL shader:
ERROR: 0:10: 'gl_PrimitiveId' : undeclared identifier 
ERROR: 0:10: 'main_inner' : no matching overloaded function found 
ERROR: 0:10: 'assign' :  cannot convert from ' const float' to 'layout( location=0) out highp 4-component vector of float'
ERROR: 0:10: '' : compilation terminated 
ERROR: 4 compilation errors.  No code generated.



#version 310 es
precision highp float;
precision highp int;

layout(location = 0) out vec4 main_loc0_Output;
vec4 main_inner(uint prim_id) {
  return vec4(float(prim_id));
}
void main() {
  main_loc0_Output = main_inner(gl_PrimitiveId);
}

tint executable returned error: exit status 1
