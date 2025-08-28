SKIP: FAILED

<dawn>/test/tint/builtins/gen/var/getBinding/b33bd5.wgsl:40:62 error: type 'texture_2d_array<u32>' cannot be used in address space 'storage' as it is non-host-shareable
@group(0) @binding(0) var<storage, read_write> prevent_dce : texture_2d_array<u32>;
                                                             ^^^^^^^^^^^^^^^^^^^^^

<dawn>/test/tint/builtins/gen/var/getBinding/b33bd5.wgsl:40:23 note: while instantiating 'var' prevent_dce
@group(0) @binding(0) var<storage, read_write> prevent_dce : texture_2d_array<u32>;
                      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


tint executable returned error: exit status 1
