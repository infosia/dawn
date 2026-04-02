struct str {
  int arr[4];
};

struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


groupshared str S;
void func() {
  S.arr = (int[4])0;
}

void main_inner(uint tint_local_index) {
  {
    uint v = 0u;
    v = tint_local_index;
    while(true) {
      uint v_1 = v;
      if ((v_1 >= 4u)) {
        break;
      }
      S.arr[v_1] = int(0);
      {
        v = (v_1 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  func();
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

