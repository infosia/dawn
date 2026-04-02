struct S {
  int4 arr[4];
};

struct main_inputs {
  uint tint_local_index : SV_GroupIndex;
};


static int4 src_private[4] = (int4[4])0;
groupshared int4 src_workgroup[4];
cbuffer cbuffer_src_uniform : register(b0) {
  uint4 src_uniform[4];
};
RWByteAddressBuffer src_storage : register(u1);
RWByteAddressBuffer v : register(u2);
RWByteAddressBuffer dst_nested : register(u3);
typedef int4 ary_ret[4];
ary_ret ret_arr() {
  return (int4[4])0;
}

S ret_struct_arr() {
  return (S)0;
}

void v_1(uint offset, int obj[2]) {
  {
    uint v_2 = 0u;
    v_2 = 0u;
    while(true) {
      uint v_3 = v_2;
      if ((v_3 >= 2u)) {
        break;
      }
      dst_nested.Store((offset + (v_3 * 4u)), asuint(obj[v_3]));
      {
        v_2 = (v_3 + 1u);
      }
    }
  }
}

void v_4(uint offset, int obj[3][2]) {
  {
    uint v_5 = 0u;
    v_5 = 0u;
    while(true) {
      uint v_6 = v_5;
      if ((v_6 >= 3u)) {
        break;
      }
      int v_7[2] = obj[v_6];
      v_1((offset + (v_6 * 8u)), v_7);
      {
        v_5 = (v_6 + 1u);
      }
    }
  }
}

void v_8(uint offset, int obj[4][3][2]) {
  {
    uint v_9 = 0u;
    v_9 = 0u;
    while(true) {
      uint v_10 = v_9;
      if ((v_10 >= 4u)) {
        break;
      }
      int v_11[3][2] = obj[v_10];
      v_4((offset + (v_10 * 24u)), v_11);
      {
        v_9 = (v_10 + 1u);
      }
    }
  }
}

void v_12(uint offset, int4 obj[4]) {
  {
    uint v_13 = 0u;
    v_13 = 0u;
    while(true) {
      uint v_14 = v_13;
      if ((v_14 >= 4u)) {
        break;
      }
      v.Store4((offset + (v_14 * 16u)), asuint(obj[v_14]));
      {
        v_13 = (v_14 + 1u);
      }
    }
  }
}

typedef int4 ary_ret_1[4];
ary_ret_1 v_15(uint offset) {
  int4 a[4] = (int4[4])0;
  {
    uint v_16 = 0u;
    v_16 = 0u;
    while(true) {
      uint v_17 = v_16;
      if ((v_17 >= 4u)) {
        break;
      }
      a[v_17] = asint(src_storage.Load4((offset + (v_17 * 16u))));
      {
        v_16 = (v_17 + 1u);
      }
    }
  }
  int4 v_18[4] = a;
  return v_18;
}

typedef int4 ary_ret_2[4];
ary_ret_2 v_19(uint start_byte_offset) {
  int4 a[4] = (int4[4])0;
  {
    uint v_20 = 0u;
    v_20 = 0u;
    while(true) {
      uint v_21 = v_20;
      if ((v_21 >= 4u)) {
        break;
      }
      a[v_21] = asint(src_uniform[((start_byte_offset + (v_21 * 16u)) / 16u)]);
      {
        v_20 = (v_21 + 1u);
      }
    }
  }
  int4 v_22[4] = a;
  return v_22;
}

void foo(int4 src_param[4]) {
  int4 src_function[4] = (int4[4])0;
  int4 v_23[4] = {(int(1)).xxxx, (int(2)).xxxx, (int(3)).xxxx, (int(3)).xxxx};
  v_12(0u, v_23);
  v_12(0u, src_param);
  int4 v_24[4] = ret_arr();
  v_12(0u, v_24);
  int4 src_let[4] = (int4[4])0;
  v_12(0u, src_let);
  int4 v_25[4] = src_function;
  v_12(0u, v_25);
  int4 v_26[4] = src_private;
  v_12(0u, v_26);
  int4 v_27[4] = src_workgroup;
  v_12(0u, v_27);
  S v_28 = ret_struct_arr();
  int4 v_29[4] = v_28.arr;
  v_12(0u, v_29);
  int4 v_30[4] = v_19(0u);
  v_12(0u, v_30);
  int4 v_31[4] = v_15(0u);
  v_12(0u, v_31);
  int src_nested[4][3][2] = (int[4][3][2])0;
  int v_32[4][3][2] = src_nested;
  v_8(0u, v_32);
}

void main_inner(uint tint_local_index) {
  {
    uint v_33 = 0u;
    v_33 = tint_local_index;
    while(true) {
      uint v_34 = v_33;
      if ((v_34 >= 4u)) {
        break;
      }
      src_workgroup[v_34] = (int(0)).xxxx;
      {
        v_33 = (v_34 + 1u);
      }
    }
  }
  GroupMemoryBarrierWithGroupSync();
  int4 ary[4] = (int4[4])0;
  foo(ary);
}

[numthreads(1, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.tint_local_index);
}

