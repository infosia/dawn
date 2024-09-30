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

struct Config {
  uint numLights;
  uint numTiles;
  uint tileCountX;
  uint tileCountY;
  uint numTileLightSlot;
  uint tileSize;
};

struct Uniforms {
  vec4 tint_symbol;
  vec4 tint_symbol_1;
  mat4 viewMatrix;
  mat4 projectionMatrix;
  vec4 fullScreenSize;
};

layout(binding = 0, std430)
buffer LightsBuffer_1_ssbo {
  LightData lights[];
} lightsBuffer;
layout(binding = 0, std430)
buffer tint_symbol_4_1_ssbo {
  Tiles tint_symbol_3;
} v;
layout(binding = 0, std140)
uniform tint_symbol_6_1_ubo {
  Config tint_symbol_5;
} v_1;
layout(binding = 0, std140)
uniform tint_symbol_8_1_ubo {
  Uniforms tint_symbol_7;
} v_2;
void tint_symbol_2_inner(uvec3 GlobalInvocationID) {
  uint index = GlobalInvocationID[0u];
  uint v_3 = index;
  if ((v_3 >= v_1.tint_symbol_5.numLights)) {
    return;
  }
  uint v_4 = index;
  uint v_5 = index;
  float v_6 = (lightsBuffer.lights[v_5].position.y - 0.10000000149011611938f);
  float v_7 = float(index);
  lightsBuffer.lights[v_4].position[1u] = (v_6 + (0.00100000004749745131f * (v_7 - (64.0f * floor((float(index) / 64.0f))))));
  uint v_8 = index;
  float v_9 = lightsBuffer.lights[v_8].position.y;
  if ((v_9 < v_2.tint_symbol_7.tint_symbol.y)) {
    uint v_10 = index;
    lightsBuffer.lights[v_10].position[1u] = v_2.tint_symbol_7.tint_symbol_1.y;
  }
  mat4 M = v_2.tint_symbol_7.projectionMatrix;
  float v_11 = -(M[3].z);
  float viewNear = (v_11 / (-1.0f + M[2].z));
  float v_12 = -(M[3].z);
  float viewFar = (v_12 / (1.0f + M[2].z));
  uint v_13 = index;
  vec4 lightPos = lightsBuffer.lights[v_13].position;
  lightPos = (v_2.tint_symbol_7.viewMatrix * lightPos);
  lightPos = (lightPos / lightPos.w);
  uint v_14 = index;
  float lightRadius = lightsBuffer.lights[v_14].radius;
  vec4 v_15 = lightPos;
  vec4 boxMin = (v_15 - vec4(vec3(lightRadius), 0.0f));
  vec4 v_16 = lightPos;
  vec4 boxMax = (v_16 + vec4(vec3(lightRadius), 0.0f));
  vec4 frustumPlanes[6] = vec4[6](vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f), vec4(0.0f));
  frustumPlanes[4] = vec4(0.0f, 0.0f, -1.0f, viewNear);
  frustumPlanes[5] = vec4(0.0f, 0.0f, 1.0f, -(viewFar));
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
          ivec2 tilePixel0Idx = ivec2((x * TILE_SIZE), (y * TILE_SIZE));
          vec2 v_17 = (2.0f * vec2(tilePixel0Idx));
          vec2 floorCoord = ((v_17 / v_2.tint_symbol_7.fullScreenSize.xy) - vec2(1.0f));
          ivec2 v_18 = tilePixel0Idx;
          vec2 v_19 = (2.0f * vec2((v_18 + ivec2(TILE_SIZE))));
          vec2 ceilCoord = ((v_19 / v_2.tint_symbol_7.fullScreenSize.xy) - vec2(1.0f));
          float v_20 = (-(viewNear) * floorCoord.x);
          float v_21 = (v_20 - (M[2].x * viewNear));
          float v_22 = (v_21 / M[0].x);
          float v_23 = (-(viewNear) * floorCoord.y);
          float v_24 = (v_23 - (M[2].y * viewNear));
          vec2 viewFloorCoord = vec2(v_22, (v_24 / M[1].y));
          float v_25 = (-(viewNear) * ceilCoord.x);
          float v_26 = (v_25 - (M[2].x * viewNear));
          float v_27 = (v_26 / M[0].x);
          float v_28 = (-(viewNear) * ceilCoord.y);
          float v_29 = (v_28 - (M[2].y * viewNear));
          vec2 viewCeilCoord = vec2(v_27, (v_29 / M[1].y));
          frustumPlanes[0] = vec4(1.0f, 0.0f, (-(viewFloorCoord.x) / viewNear), 0.0f);
          frustumPlanes[1] = vec4(-1.0f, 0.0f, (viewCeilCoord.x / viewNear), 0.0f);
          frustumPlanes[2] = vec4(0.0f, 1.0f, (-(viewFloorCoord.y) / viewNear), 0.0f);
          frustumPlanes[3] = vec4(0.0f, -1.0f, (viewCeilCoord.y / viewNear), 0.0f);
          float dp = 0.0f;
          {
            uint i = 0u;
            while(true) {
              if ((i < 6u)) {
              } else {
                break;
              }
              vec4 p = vec4(0.0f);
              uint v_30 = i;
              if ((frustumPlanes[v_30].x > 0.0f)) {
                p[0u] = boxMax.x;
              } else {
                p[0u] = boxMin.x;
              }
              uint v_31 = i;
              if ((frustumPlanes[v_31].y > 0.0f)) {
                p[1u] = boxMax.y;
              } else {
                p[1u] = boxMin.y;
              }
              uint v_32 = i;
              if ((frustumPlanes[v_32].z > 0.0f)) {
                p[2u] = boxMax.z;
              } else {
                p[2u] = boxMin.z;
              }
              p[3u] = 1.0f;
              float v_33 = dp;
              vec4 v_34 = p;
              uint v_35 = i;
              dp = (v_33 + min(0.0f, dot(v_34, frustumPlanes[v_35])));
              {
                i = (i + 1u);
              }
              continue;
            }
          }
          if ((dp >= 0.0f)) {
            uint tileId = uint((x + (y * TILE_COUNT_X)));
            bool v_36 = false;
            if ((tileId < 0u)) {
              v_36 = true;
            } else {
              uint v_37 = tileId;
              v_36 = (v_37 >= v_1.tint_symbol_5.numTiles);
            }
            if (v_36) {
              {
                x = (x + 1);
              }
              continue;
            }
            uint v_38 = tileId;
            uint offset = atomicAdd(v.tint_symbol_3.data[v_38].count, 1u);
            uint v_39 = offset;
            if ((v_39 >= v_1.tint_symbol_5.numTileLightSlot)) {
              {
                x = (x + 1);
              }
              continue;
            }
            uint v_40 = tileId;
            uint v_41 = offset;
            v.tint_symbol_3.data[v_40].lightId[v_41] = GlobalInvocationID[0u];
          }
          {
            x = (x + 1);
          }
          continue;
        }
      }
      {
        y = (y + 1);
      }
      continue;
    }
  }
}
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;
void main() {
  tint_symbol_2_inner(gl_GlobalInvocationID);
}
