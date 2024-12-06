#version 310 es
precision highp float;
precision highp int;

layout(binding = 0, std430)
buffer tint_symbol_block_1_ssbo {
  int inner[4];
} f_v;
void foo() {
  {
    int i = 0;
    while(true) {
      if ((i < 4)) {
      } else {
        break;
      }
      uint v_1 = min(uint(i), 3u);
      bool tint_continue = false;
      switch(f_v.inner[v_1]) {
        case 1:
        {
          tint_continue = true;
          break;
        }
        default:
        {
          uint v_2 = min(uint(i), 3u);
          f_v.inner[v_2] = 2;
          break;
        }
      }
      if (tint_continue) {
        {
          i = (i + 1);
        }
        continue;
      }
      {
        i = (i + 1);
      }
      continue;
    }
  }
}
void main() {
  foo();
}
