struct str {
  int arr[4];
};


static str P = (str)0;
void func(inout int pointer[4]) {
  pointer = (int[4])0;
}

[numthreads(1, 1, 1)]
void main() {
  func(P.arr);
}

