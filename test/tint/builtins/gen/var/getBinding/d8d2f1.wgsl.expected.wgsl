SKIP: FAILED

<dawn>/test/tint/builtins/gen/var/getBinding/d8d2f1.wgsl:40:62 error: type 'texture_3d<u32>' cannot be used in address space 'storage' as it is non-host-shareable
@group(0) @binding(0) var<storage, read_write> prevent_dce : texture_3d<u32>;
                                                             ^^^^^^^^^^^^^^^

<dawn>/test/tint/builtins/gen/var/getBinding/d8d2f1.wgsl:40:23 note: while instantiating 'var' prevent_dce
@group(0) @binding(0) var<storage, read_write> prevent_dce : texture_3d<u32>;
                      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
