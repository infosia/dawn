enable chromium_internal_input_attachments;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec4<f32>;

@input_attachment_index(3) @group(1) @binding(0) var arg_0 : input_attachment<f32>;

fn inputAttachmentLoad_d8c8ff() -> vec4<f32> {
  var arg_1 = 1u;
  var res : vec4<f32> = inputAttachmentLoad(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = inputAttachmentLoad_d8c8ff();
}
