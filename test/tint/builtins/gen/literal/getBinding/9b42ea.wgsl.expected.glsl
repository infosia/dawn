SKIP: FAILED

<dawn>/test/tint/builtins/gen/literal/getBinding/9b42ea.wgsl:40:62 error: type 'texture_depth_2d' cannot be used in address space 'storage' as it is non-host-shareable
@group(0) @binding(0) var<storage, read_write> prevent_dce : texture_depth_2d;
                                                             ^^^^^^^^^^^^^^^^

<dawn>/test/tint/builtins/gen/literal/getBinding/9b42ea.wgsl:40:23 note: while instantiating 'var' prevent_dce
@group(0) @binding(0) var<storage, read_write> prevent_dce : texture_depth_2d;
                      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
