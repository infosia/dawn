struct A {
  b : array<vec2u, 2>,
}

@compute @workgroup_size(1u)
fn main() {
  var q = 0u;
  var w = A(array(vec2u(1, 2), vec2u(3, 4))).b[q].x;
}
