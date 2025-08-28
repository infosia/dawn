SKIP: FAILED

<dawn>/test/tint/builtins/gen/literal/getBinding/7691b1.wgsl:40:62 error: type 'texture_depth_2d_array' cannot be used in address space 'storage' as it is non-host-shareable
@group(0) @binding(0) var<storage, read_write> prevent_dce : texture_depth_2d_array;
                                                             ^^^^^^^^^^^^^^^^^^^^^^

<dawn>/test/tint/builtins/gen/literal/getBinding/7691b1.wgsl:40:23 note: while instantiating 'var' prevent_dce
@group(0) @binding(0) var<storage, read_write> prevent_dce : texture_depth_2d_array;
                      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
