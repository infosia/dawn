struct str {
  int arr[4];
};


void func(inout int pointer[4]) {
  pointer = (int[4])0;
}

[numthreads(1, 1, 1)]
void main() {
  str F = (str)0;
  func(F.arr);
}

