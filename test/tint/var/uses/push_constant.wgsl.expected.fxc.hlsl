SKIP: INVALID

//
// main1
//

enable chromium_experimental_push_constant;

var<push_constant> a : i32;

fn uses_a() {
  let foo = a;
}

@compute @workgroup_size(1)
fn main1() {
  uses_a();
}

Failed to generate: error: unhandled address space push_constant
//
// main2
//

enable chromium_experimental_push_constant;

var<push_constant> a : i32;

fn uses_a() {
  let foo = a;
}

fn uses_uses_a() {
  uses_a();
}

@compute @workgroup_size(1)
fn main2() {
  uses_uses_a();
}

Failed to generate: error: unhandled address space push_constant
//
// main3
//

enable chromium_experimental_push_constant;

var<push_constant> b : i32;

fn uses_b() {
  let foo = b;
}

@compute @workgroup_size(1)
fn main3() {
  uses_b();
}

Failed to generate: error: unhandled address space push_constant
//
// main4
//
[numthreads(1, 1, 1)]
void main4() {
  return;
}

tint executable returned error: exit status 1
