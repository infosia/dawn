#version 310 es


struct LightData {
  vec4 position;
  vec3 color;
  float radius;
};

struct TileLightIdData {
  uint count;
  uint lightId[64];
};

struct Tiles {
  TileLightIdData data[4];
};

layout(binding = 0, std430)
buffer LightsBuffer_1_ssbo {
  LightData lights[];
} lightsBuffer;
layout(binding = 1, std430)
buffer tileLightId_block_1_ssbo {
  Tiles inner;
} v;
layout(binding = 2, std140)
uniform config_block_1_ubo {
  uvec4 inner[2];
} v_1;
layout(binding = 3, std140)
uniform uniforms_block_1_ubo {
  uvec4 inner[11];
} v_2;
mat4 v_3(uint start_byte_offset) {
  return mat4(uintBitsToFloat(v_2.inner[(start_byte_offset / 16u)]), uintBitsToFloat(v_2.inner[((16u + start_byte_offset) / 16u)]), uintBitsToFloat(v_2.inner[((32u + start_byte_offset) / 16u)]), uintBitsToFloat(v_2.inner[((48u + start_byte_offset) / 16u)]));
}
void main_inner(uvec3 GlobalInvocationID) {
  uint index = GlobalInvocationID.x;
  if ((index >= v_1.inner[0u].x)) {
    return;
  }
  uint v_4 = index;
  uint v_5 = min(v_4, (uint(lightsBuffer.lights.length()) - 1u));
  uint v_6 = index;
  uint v_7 = min(v_6, (uint(lightsBuffer.lights.length()) - 1u));
  float v_8 = (lightsBuffer.lights[v_7].position.y - 0.10000000149011611938f);
  float v_9 = float(index);
  lightsBuffer.lights[v_5].position.y = (v_8 + (0.00100000004749745131f * (v_9 - (64.0f * floor((float(index) / 64.0f))))));
  uint v_10 = index;
  uint v_11 = min(v_10, (uint(lightsBuffer.lights.length()) - 1u));
  if ((lightsBuffer.lights[v_11].position.y < uintBitsToFloat(v_2.inner[0u].y))) {
    uint v_12 = index;
    uint v_13 = min(v_12, (uint(lightsBuffer.lights.length()) - 1u));
    lightsBuffer.lights[v_13].position.y = uintBitsToFloat(v_2.inner[1u].y);
  }
  mat4 M = v_3(96u);
  float viewNear = (-(M[3u].z) / (-1.0f + M[2u].z));
  float viewFar = (-(M[3u].z) / (1.0f + M[2u].z));
  uint v_14 = index;
  uint v_15 = min(v_14, (uint(lightsBuffer.lights.length()) - 1u));
  vec4 lightPos = lightsBuffer.lights[v_15].position;
  mat4 v_16 = v_3(32u);
  lightPos = (v_16 * lightPos);
  lightPos = (lightPos / lightPos.w);
  uint v_17 = index;
  uint v_18 = min(v_17, (uint(lightsBuffer.lights.length()) - 1u));
  float lightRadius = lightsBuffer.lights[v_18].radius;
  vec4 v_19 = lightPos;
  vec4 boxMin = (v_19 - vec4(vec3(lightRadius), 0.0f));
  vec4 v_20 = lightPos;
  vec4 boxMax = (v_20 + vec4(vec3(lightRadius), 0.0f));
  vec4 frustumPlanes[6] = vec4[6](vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f));
  frustumPlanes[4u] = vec4(0.0f, 0.0f, -1.0f, viewNear);
  frustumPlanes[5u] = vec4(0.0f, 0.0f, 1.0f, -(viewFar));
  int TILE_SIZE = 16;
  int TILE_COUNT_X = 2;
  int TILE_COUNT_Y = 2;
  {
    int y = 0;
    while(true) {
      if ((y < TILE_COUNT_Y)) {
      } else {
        break;
      }
      {
        int x = 0;
        while(true) {
          if ((x < TILE_COUNT_X)) {
          } else {
            break;
          }
          uint v_21 = uint(x);
          int v_22 = int((v_21 * uint(TILE_SIZE)));
          uint v_23 = uint(y);
          ivec2 tilePixel0Idx = ivec2(v_22, int((v_23 * uint(TILE_SIZE))));
          vec2 v_24 = (2.0f * vec2(tilePixel0Idx));
          vec2 floorCoord = ((v_24 / uintBitsToFloat(v_2.inner[10u]).xy) - vec2(1.0f));
          ivec2 v_25 = tilePixel0Idx;
          ivec2 v_26 = ivec2(TILE_SIZE);
          uvec2 v_27 = uvec2(v_25);
          vec2 v_28 = (2.0f * vec2(ivec2((v_27 + uvec2(v_26)))));
          vec2 ceilCoord = ((v_28 / uintBitsToFloat(v_2.inner[10u]).xy) - vec2(1.0f));
          vec2 viewFloorCoord = vec2((((-(viewNear) * floorCoord.x) - (M[2u].x * viewNear)) / M[0u].x), (((-(viewNear) * floorCoord.y) - (M[2u].y * viewNear)) / M[1u].y));
          vec2 viewCeilCoord = vec2((((-(viewNear) * ceilCoord.x) - (M[2u].x * viewNear)) / M[0u].x), (((-(viewNear) * ceilCoord.y) - (M[2u].y * viewNear)) / M[1u].y));
          frustumPlanes[0u] = vec4(1.0f, 0.0f, (-(viewFloorCoord.x) / viewNear), 0.0f);
          frustumPlanes[1u] = vec4(-1.0f, 0.0f, (viewCeilCoord.x / viewNear), 0.0f);
          frustumPlanes[2u] = vec4(0.0f, 1.0f, (-(viewFloorCoord.y) / viewNear), 0.0f);
          frustumPlanes[3u] = vec4(0.0f, -1.0f, (viewCeilCoord.y / viewNear), 0.0f);
          float dp = 0.0f;
          {
            uint i = 0u;
            while(true) {
              if ((i < 6u)) {
              } else {
                break;
              }
              vec4 p = vec4(0.0f);
              uint v_29 = min(i, 5u);
              if ((frustumPlanes[v_29].x > 0.0f)) {
                p.x = boxMax.x;
              } else {
                p.x = boxMin.x;
              }
              uint v_30 = min(i, 5u);
              if ((frustumPlanes[v_30].y > 0.0f)) {
                p.y = boxMax.y;
              } else {
                p.y = boxMin.y;
              }
              uint v_31 = min(i, 5u);
              if ((frustumPlanes[v_31].z > 0.0f)) {
                p.z = boxMax.z;
              } else {
                p.z = boxMin.z;
              }
              p.w = 1.0f;
              float v_32 = dp;
              vec4 v_33 = p;
              uint v_34 = min(i, 5u);
              dp = (v_32 + min(0.0f, dot(v_33, frustumPlanes[v_34])));
              {
                i = (i + 1u);
              }
              continue;
            }
          }
          if ((dp >= 0.0f)) {
            int v_35 = x;
            uint v_36 = uint(y);
            int v_37 = int((v_36 * uint(TILE_COUNT_X)));
            uint v_38 = uint(v_35);
            uint tileId = uint(int((v_38 + uint(v_37))));
            bool v_39 = false;
            if ((tileId < 0u)) {
              v_39 = true;
            } else {
              v_39 = (tileId >= v_1.inner[0u].y);
            }
            if (v_39) {
              {
                uint v_40 = uint(x);
                x = int((v_40 + uint(1)));
              }
              continue;
            }
            uint v_41 = min(tileId, 3u);
            uint offset = atomicAdd(v.inner.data[v_41].count, 1u);
            if ((offset >= v_1.inner[1u].x)) {
              {
                uint v_40 = uint(x);
                x = int((v_40 + uint(1)));
              }
              continue;
            }
            uint v_42 = min(tileId, 3u);
            uint v_43 = min(offset, 63u);
            v.inner.data[v_42].lightId[v_43] = GlobalInvocationID.x;
          }
          {
            uint v_40 = uint(x);
            x = int((v_40 + uint(1)));
          }
          continue;
        }
      }
      {
        uint v_44 = uint(y);
        y = int((v_44 + uint(1)));
      }
      continue;
    }
  }
}
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
  main_inner(gl_GlobalInvocationID);
}
