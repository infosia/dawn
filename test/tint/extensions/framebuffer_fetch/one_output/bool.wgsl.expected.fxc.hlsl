SKIP: FAILED

<dawn>/test/tint/extensions/framebuffer_fetch/one_output/bool.wgsl:3:26 error: cannot apply '@color' to declaration of type 'vec4<bool>'
@fragment fn f(@color(0) fbf : vec4<bool>) -> @location(0) vec4<f32> {
                         ^^^

<dawn>/test/tint/extensions/framebuffer_fetch/one_output/bool.wgsl:3:17 note: '@color' must only be applied to declarations of numeric scalar or numeric vector type
@fragment fn f(@color(0) fbf : vec4<bool>) -> @location(0) vec4<f32> {
                ^^^^^


tint executable returned error: exit status 1
