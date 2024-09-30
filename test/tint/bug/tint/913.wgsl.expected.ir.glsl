#version 310 es


struct Uniforms {
  uint dstTextureFlipY;
  uint channelCount;
  uvec2 srcCopyOrigin;
  uvec2 dstCopyOrigin;
  uvec2 copySize;
};

uniform highp sampler2D src;
uniform highp sampler2D dst;
layout(binding = 2, std430)
buffer OutputBuf_1_ssbo {
  uint result[];
} tint_symbol;
layout(binding = 3, std140)
uniform tint_symbol_3_1_ubo {
  Uniforms tint_symbol_2;
} v;
bool aboutEqual(float value, float expect) {
  return (abs((value - expect)) < 0.00100000004749745131f);
}
void tint_symbol_1_inner(uvec3 GlobalInvocationID) {
  uvec2 srcSize = uvec2(textureSize(src, 0));
  uvec2 dstSize = uvec2(textureSize(dst, 0));
  uvec2 dstTexCoord = uvec2(GlobalInvocationID.xy);
  vec4 nonCoveredColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
  bool success = true;
  bool v_1 = false;
  if ((dstTexCoord[0u] < v.tint_symbol_2.dstCopyOrigin.x)) {
    v_1 = true;
  } else {
    v_1 = (dstTexCoord[1u] < v.tint_symbol_2.dstCopyOrigin.y);
  }
  bool v_2 = false;
  if (v_1) {
    v_2 = true;
  } else {
    uint v_3 = v.tint_symbol_2.dstCopyOrigin.x;
    v_2 = (dstTexCoord[0u] >= (v_3 + v.tint_symbol_2.copySize.x));
  }
  bool v_4 = false;
  if (v_2) {
    v_4 = true;
  } else {
    uint v_5 = v.tint_symbol_2.dstCopyOrigin.y;
    v_4 = (dstTexCoord[1u] >= (v_5 + v.tint_symbol_2.copySize.y));
  }
  if (v_4) {
    bool v_6 = false;
    if (success) {
      ivec2 v_7 = ivec2(ivec2(dstTexCoord));
      v_6 = all(equal(texelFetch(dst, v_7, int(0)), nonCoveredColor));
    } else {
      v_6 = false;
    }
    success = v_6;
  } else {
    uvec2 v_8 = (dstTexCoord - v.tint_symbol_2.dstCopyOrigin);
    uvec2 srcTexCoord = (v_8 + v.tint_symbol_2.srcCopyOrigin);
    if ((v.tint_symbol_2.dstTextureFlipY == 1u)) {
      srcTexCoord[1u] = ((srcSize[1u] - srcTexCoord.y) - 1u);
    }
    ivec2 v_9 = ivec2(ivec2(srcTexCoord));
    vec4 srcColor = texelFetch(src, v_9, int(0));
    ivec2 v_10 = ivec2(ivec2(dstTexCoord));
    vec4 dstColor = texelFetch(dst, v_10, int(0));
    if ((v.tint_symbol_2.channelCount == 2u)) {
      bool v_11 = false;
      if (success) {
        v_11 = aboutEqual(dstColor[0u], srcColor[0u]);
      } else {
        v_11 = false;
      }
      bool v_12 = false;
      if (v_11) {
        v_12 = aboutEqual(dstColor[1u], srcColor[1u]);
      } else {
        v_12 = false;
      }
      success = v_12;
    } else {
      bool v_13 = false;
      if (success) {
        v_13 = aboutEqual(dstColor[0u], srcColor[0u]);
      } else {
        v_13 = false;
      }
      bool v_14 = false;
      if (v_13) {
        v_14 = aboutEqual(dstColor[1u], srcColor[1u]);
      } else {
        v_14 = false;
      }
      bool v_15 = false;
      if (v_14) {
        v_15 = aboutEqual(dstColor[2u], srcColor[2u]);
      } else {
        v_15 = false;
      }
      bool v_16 = false;
      if (v_15) {
        v_16 = aboutEqual(dstColor[3u], srcColor[3u]);
      } else {
        v_16 = false;
      }
      success = v_16;
    }
  }
  uint outputIndex = ((GlobalInvocationID[1u] * dstSize[0u]) + GlobalInvocationID[0u]);
  if (success) {
    tint_symbol.result[outputIndex] = 1u;
  } else {
    tint_symbol.result[outputIndex] = 0u;
  }
}
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
void main() {
  tint_symbol_1_inner(gl_GlobalInvocationID);
}
