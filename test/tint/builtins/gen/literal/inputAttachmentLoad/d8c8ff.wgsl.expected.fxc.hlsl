SKIP: FAILED

$B1: {  # root
  %prevent_dce:ptr<storage, vec4<f32>, read_write> = var @binding_point(0, 0)
  %arg_0:ptr<handle, input_attachment<f32>, read> = var @binding_point(1, 0) @input_attachment_index(3)
}

%inputAttachmentLoad_d8c8ff = func():vec4<f32> {
  $B2: {
    %4:input_attachment<f32> = load %arg_0
    %5:vec4<f32> = inputAttachmentLoad %4, 1u
    %res:ptr<function, vec4<f32>, read_write> = var %5
    %7:vec4<f32> = load %res
    ret %7
  }
}
%fragment_main = @fragment func():void {
  $B3: {
    %9:vec4<f32> = call %inputAttachmentLoad_d8c8ff
    store %prevent_dce, %9
    ret
  }
}
Failed to generate: input attachments are not supported by the HLSL backend

tint executable returned error: exit status 1
