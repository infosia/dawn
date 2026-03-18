enable chromium_experimental_framebuffer_fetch;

@fragment fn f(@color(0) fbf : vec4<bool>) -> @location(0) vec4<f32> {
  return vec4f(0.0, 0.0, 0.0, 1.0);
}
