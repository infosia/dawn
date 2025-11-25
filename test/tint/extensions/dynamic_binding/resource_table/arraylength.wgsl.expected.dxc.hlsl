SKIP: FAILED

Failed to generate IR: failure: :2:57 error: var: a resource variable is missing binding point
  %sampled_textures:ptr<handle, resource_table, read> = var undef
                                                        ^^^

:1:1 note: in block
$B1: {  # root
^^^

note: # Disassembly
$B1: {  # root
  %sampled_textures:ptr<handle, resource_table, read> = var undef
}

%fs = @fragment func():void {
  $B2: {
    %3:resource_table = load %sampled_textures
    %4:u32 = wgsl.arrayLength %3
    %t:u32 = let %4
    ret
  }
}


tint executable returned error: exit status 1
