//
// main1
//

cbuffer cbuffer_a : register(b0) {
  int a;
};
void uses_a() {
  int foo = a;
}

[numthreads(1, 1, 1)]
void main1() {
  uses_a();
}

//
// main2
//

cbuffer cbuffer_a : register(b0) {
  int a;
};
void uses_a() {
  int foo = a;
}

void uses_uses_a() {
  uses_a();
}

[numthreads(1, 1, 1)]
void main2() {
  uses_uses_a();
}

//
// main3
//

cbuffer cbuffer_b : register(b0) {
  int b;
};
void uses_b() {
  int foo = b;
}

[numthreads(1, 1, 1)]
void main3() {
  uses_b();
}

//
// main4
//

[numthreads(1, 1, 1)]
void main4() {
}

