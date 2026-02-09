SKIP: FAILED

Failed to generate: :21:5 error: convert: 64-bit integer types are not permitted
    %5:u64 = convert vec2<u32>(1u)
    ^^^^^^

:18:3 note: in block
  $B2: {
  ^^^

:21:14 error: convert: not defined for result type, 'u64'
    %5:u64 = convert vec2<u32>(1u)
             ^^^^^^^

:18:3 note: in block
  $B2: {
  ^^^

note: # Disassembly
SB_RW = struct @align(8) {
  arg_0:atomic<vec2<u32>> @offset(0)
}

SB_RW_a64 = struct @align(8) {
  arg_0:atomic<u64> @offset(0)
}

sb_rw_block = struct @align(8), @block {
  inner:SB_RW_a64 @offset(0)
}

$B1: {  # root
  %1:ptr<storage, sb_rw_block, read_write> = var undef @binding_point(0, 0)
}

%atomicStoreMin_f5f229 = func():void {
  $B2: {
    %3:ptr<storage, SB_RW_a64, read_write> = access %1, 0u
    %4:ptr<storage, atomic<u64>, read_write> = access %3, 0u
    %5:u64 = convert vec2<u32>(1u)
    %6:void = atomicStoreMin %4, %5
    ret
  }
}
%fragment_main = @fragment func():void {
  $B3: {
    %8:void = call %atomicStoreMin_f5f229
    ret
  }
}

Failed to generate: :21:5 error: convert: 64-bit integer types are not permitted
    %5:u64 = convert vec2<u32>(1u)
    ^^^^^^

:18:3 note: in block
  $B2: {
  ^^^

:21:14 error: convert: not defined for result type, 'u64'
    %5:u64 = convert vec2<u32>(1u)
             ^^^^^^^

:18:3 note: in block
  $B2: {
  ^^^

note: # Disassembly
SB_RW = struct @align(8) {
  arg_0:atomic<vec2<u32>> @offset(0)
}

SB_RW_a64 = struct @align(8) {
  arg_0:atomic<u64> @offset(0)
}

sb_rw_block = struct @align(8), @block {
  inner:SB_RW_a64 @offset(0)
}

$B1: {  # root
  %1:ptr<storage, sb_rw_block, read_write> = var undef @binding_point(0, 0)
}

%atomicStoreMin_f5f229 = func():void {
  $B2: {
    %3:ptr<storage, SB_RW_a64, read_write> = access %1, 0u
    %4:ptr<storage, atomic<u64>, read_write> = access %3, 0u
    %5:u64 = convert vec2<u32>(1u)
    %6:void = atomicStoreMin %4, %5
    ret
  }
}
%compute_main = @compute @workgroup_size(1i, 1i, 1i) func():void {
  $B3: {
    %8:void = call %atomicStoreMin_f5f229
    ret
  }
}

//
// fragment_main
//
//
// compute_main
//

tint executable returned error: exit status 1
