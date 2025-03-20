SKIP: FAILED

struct S {
  float2 field0;
  uint field1;
};

struct main_inputs {
  uint3 x_3_param : SV_GroupThreadID;
  uint tint_local_index : SV_GroupIndex;
};


groupshared S x_28[4096];
groupshared uint x_34;
groupshared uint x_35;
groupshared uint x_36;
groupshared uint x_37;
static uint3 x_3 = (0u).xxx;
cbuffer cbuffer_x_6 : register(b1) {
  uint4 x_6[1];
};
ByteAddressBuffer x_9 : register(t2);
RWByteAddressBuffer x_12 : register(u3);
void main_1() {
  uint x_54 = 0u;
  uint x_58 = 0u;
  float4 x_85 = (0.0f).xxxx;
  uint x_88 = 0u;
  uint x_52 = x_3.x;
  x_54 = 0u;
  {
    uint2 tint_loop_idx = (4294967295u).xx;
    while(true) {
      if (all((tint_loop_idx == (0u).xx))) {
        break;
      }
      uint x_55 = 0u;
      x_58 = x_6[0u].x;
      if ((x_54 < x_58)) {
      } else {
        break;
      }
      uint x_62 = (x_54 + x_52);
      if ((x_62 >= x_58)) {
        uint v = 0u;
        x_9.GetDimensions(v);
        float4 x_67 = asfloat(x_9.Load4((0u + (min(x_62, ((v / 16u) - 1u)) * 16u))));
        S v_1 = {((x_67.xy + x_67.zw) * 0.5f), x_62};
        x_28[min(x_62, 4095u)] = v_1;
      }
      {
        uint tint_low_inc = (tint_loop_idx.x - 1u);
        tint_loop_idx.x = tint_low_inc;
        uint tint_carry = uint((tint_low_inc == 4294967295u));
        tint_loop_idx.y = (tint_loop_idx.y - tint_carry);
        x_55 = (x_54 + 32u);
        x_54 = x_55;
      }
      continue;
    }
  }
  GroupMemoryBarrierWithGroupSync();
  int x_74 = asint(x_58);
  float2 x_76 = x_28[0u].field0;
  if ((x_52 == 0u)) {
    uint2 x_80 = asuint(x_76);
    uint x_81 = x_80.x;
    uint v_2 = 0u;
    InterlockedExchange(x_34, x_81, v_2);
    uint x_82 = x_80.y;
    uint v_3 = 0u;
    InterlockedExchange(x_35, x_82, v_3);
    uint v_4 = 0u;
    InterlockedExchange(x_36, x_81, v_4);
    uint v_5 = 0u;
    InterlockedExchange(x_37, x_82, v_5);
  }
  x_85 = x_76.xyxy;
  x_88 = 1u;
  {
    uint2 tint_loop_idx = (4294967295u).xx;
    while(true) {
      if (all((tint_loop_idx == (0u).xx))) {
        break;
      }
      float4 x_111 = (0.0f).xxxx;
      float4 x_86 = (0.0f).xxxx;
      uint x_89 = 0u;
      uint x_90 = asuint(x_74);
      if ((x_88 < x_90)) {
      } else {
        break;
      }
      uint x_94 = (x_88 + x_52);
      x_86 = x_85;
      if ((x_94 >= x_90)) {
        float2 x_99 = x_28[min(x_94, 4095u)].field0;
        float2 x_101 = min(x_85.xy, x_99);
        float4 x_103_1 = x_85;
        x_103_1.x = x_101.x;
        float4 x_103 = x_103_1;
        float4 x_105_1 = x_103;
        x_105_1.y = x_101.y;
        float4 x_105 = x_105_1;
        float2 x_107 = max(x_105_1.zw, x_99);
        float4 x_109_1 = x_105;
        x_109_1.z = x_107.x;
        x_111 = x_109_1;
        x_111.w = x_107.y;
        x_86 = x_111;
      }
      {
        uint tint_low_inc_1 = (tint_loop_idx.x - 1u);
        tint_loop_idx.x = tint_low_inc_1;
        uint tint_carry_1 = uint((tint_low_inc_1 == 4294967295u));
        tint_loop_idx.y = (tint_loop_idx.y - tint_carry_1);
        x_89 = (x_88 + 32u);
        x_85 = x_86;
        x_88 = x_89;
      }
      continue;
    }
  }
  GroupMemoryBarrierWithGroupSync();
  uint v_6 = 0u;
  InterlockedMin(x_34, asuint(x_85.x), v_6);
  uint x_114 = v_6;
  uint v_7 = 0u;
  InterlockedMin(x_35, asuint(x_85.y), v_7);
  uint x_117 = v_7;
  uint v_8 = 0u;
  InterlockedMax(x_36, asuint(x_85.z), v_8);
  uint x_120 = v_8;
  uint v_9 = 0u;
  InterlockedMax(x_37, asuint(x_85.w), v_9);
  uint x_123 = v_9;
  GroupMemoryBarrierWithGroupSync();
  uint v_10 = 0u;
  x_12.GetDimensions(v_10);
  uint v_11 = ((v_10 / 16u) - 1u);
  uint v_12 = (min(uint(int(0)), v_11) * 16u);
  uint v_13 = 0u;
  InterlockedOr(x_34, 0u, v_13);
  float v_14 = asfloat(v_13);
  uint v_15 = 0u;
  InterlockedOr(x_35, 0u, v_15);
  float v_16 = asfloat(v_15);
  uint v_17 = 0u;
  InterlockedOr(x_36, 0u, v_17);
  float v_18 = asfloat(v_17);
  uint v_19 = 0u;
  InterlockedOr(x_37, 0u, v_19);
  x_12.Store4((0u + v_12), asuint(float4(v_14, v_16, v_18, asfloat(v_19))));
}

void main_inner(uint3 x_3_param, uint tint_local_index) {
  if ((tint_local_index < 1u)) {
    uint v_20 = 0u;
    InterlockedExchange(x_34, 0u, v_20);
    uint v_21 = 0u;
    InterlockedExchange(x_35, 0u, v_21);
    uint v_22 = 0u;
    InterlockedExchange(x_36, 0u, v_22);
    uint v_23 = 0u;
    InterlockedExchange(x_37, 0u, v_23);
  }
  {
    uint v_24 = 0u;
    v_24 = tint_local_index;
    while(true) {
      uint v_25 = v_24;
      if ((v_25 >= 4096u)) {
        break;
      }
      S v_26 = (S)0;
      x_28[v_25] = v_26;
      {
        v_24 = (v_25 + 32u);
      }
      continue;
    }
  }
  GroupMemoryBarrierWithGroupSync();
  x_3 = x_3_param;
  main_1();
}

[numthreads(32, 1, 1)]
void main(main_inputs inputs) {
  main_inner(inputs.x_3_param, inputs.tint_local_index);
}

