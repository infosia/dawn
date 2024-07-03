enable chromium_experimental_push_constant;
var<push_constant> a : i32;
var<push_constant> c : i32; // unuseunpack2x16snormd
fn uses_a() {
  let foo = a;
}

fn uses_uses_a() {
  uses_a();
}

fn uses_b() {
}

@compute @workgroup_size(1)
fn main2() {
  uses_uses_a();
}
