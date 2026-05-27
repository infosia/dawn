


A bug was found in the tint_ir_fuzzer where a crash occurred during the conversion to WGSL.
The crash is an InternalCompilerError (ICE) in tint::wgsl::ir::transform::State::Replace, specifically related to AtomicVec2uToFromU64.
The root cause identified is the double usage of %5 in the provided IR.
The suspected regression change list is e271ac9c6038365abbb79ede3cba994dccf0581a.
No workaround or recommendation was provided.


The IR code (which cannot be represented in wgsl) appears below:


$B1: {  # root
  %sb_rw0:ptr<storage, array<atomic<u64>, 4>, read_write> = var undef @binding_point(0, 0)
  %sb_rw1:ptr<storage, array<atomic<u64>>, read_write> = var undef @binding_point(0, 1)
}

%sb_rw0_1 = @compute @workgroup_size(1i, 1i, 1i) func():void {  # %sb_rw0_1: 'sb_rw0'
  $B2: {
    %4:ptr<storage, atomic<u64>, read_write> = access %sb_rw0, 0i
    %5:u64 = bitcast<u64> vec2<u32>(1u)
    %6:void = atomicStoreMin %4, %5
    %7:ptr<storage, atomic<u64>, read_write> = access %sb_rw1, %5
    %8:u64 = bitcast<u64> vec2<u32>(1u)
    %9:void = atomicStoreMin %7, %8
    ret
  }
}



end of code:


This is crashing in tint::wgsl::writer::raise::RaiseFuzzer, specifically in transform::AtomicVec2uToFromU64. The ICE is due to the double usage of %5