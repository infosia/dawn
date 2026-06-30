enable chromium_internal_input_attachments;

@group(0) @binding(0) var<storage, read_write> prevent_dce : vec4<i32>;

@input_attachment_index(3) @group(1) @binding(0) var arg_0 : input_attachment<i32>;

fn inputAttachmentLoad_b7e76b() -> vec4<i32> {
  var arg_1 = 1u;
  var res : vec4<i32> = inputAttachmentLoad(arg_0, arg_1);
  return res;
}

@fragment
fn fragment_main() {
  prevent_dce = inputAttachmentLoad_b7e76b();
}
