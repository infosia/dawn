// Copyright 2022 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "src/tint/utils/text/unicode.h"

#include <algorithm>
#include <array>

#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/macros/compiler.h"

namespace tint {
namespace {

struct CodePointRange {
    uint32_t first;  // First code point in the interval
    uint32_t last;   // Last code point in the interval (inclusive)
};

inline bool operator<(CodePoint code_point, CodePointRange range) {
    return code_point < range.first;
}
inline bool operator<(CodePointRange range, CodePoint code_point) {
    return range.last < code_point;
}

// Interval ranges of all code points in the Unicode 14 XID_Start set
// This array needs to be in ascending order.
constexpr std::array<CodePointRange, 655> kXIDStartRanges = {
    CodePointRange{0x00041, 0x0005a}, CodePointRange{0x00061, 0x0007a},
    CodePointRange{0x000aa, 0x000aa}, CodePointRange{0x000b5, 0x000b5},
    CodePointRange{0x000ba, 0x000ba}, CodePointRange{0x000c0, 0x000d6},
    CodePointRange{0x000d8, 0x000f6}, CodePointRange{0x000f8, 0x002c1},
    CodePointRange{0x002c6, 0x002d1}, CodePointRange{0x002e0, 0x002e4},
    CodePointRange{0x002ec, 0x002ec}, CodePointRange{0x002ee, 0x002ee},
    CodePointRange{0x00370, 0x00374}, CodePointRange{0x00376, 0x00377},
    CodePointRange{0x0037b, 0x0037d}, CodePointRange{0x0037f, 0x0037f},
    CodePointRange{0x00386, 0x00386}, CodePointRange{0x00388, 0x0038a},
    CodePointRange{0x0038c, 0x0038c}, CodePointRange{0x0038e, 0x003a1},
    CodePointRange{0x003a3, 0x003f5}, CodePointRange{0x003f7, 0x00481},
    CodePointRange{0x0048a, 0x0052f}, CodePointRange{0x00531, 0x00556},
    CodePointRange{0x00559, 0x00559}, CodePointRange{0x00560, 0x00588},
    CodePointRange{0x005d0, 0x005ea}, CodePointRange{0x005ef, 0x005f2},
    CodePointRange{0x00620, 0x0064a}, CodePointRange{0x0066e, 0x0066f},
    CodePointRange{0x00671, 0x006d3}, CodePointRange{0x006d5, 0x006d5},
    CodePointRange{0x006e5, 0x006e6}, CodePointRange{0x006ee, 0x006ef},
    CodePointRange{0x006fa, 0x006fc}, CodePointRange{0x006ff, 0x006ff},
    CodePointRange{0x00710, 0x00710}, CodePointRange{0x00712, 0x0072f},
    CodePointRange{0x0074d, 0x007a5}, CodePointRange{0x007b1, 0x007b1},
    CodePointRange{0x007ca, 0x007ea}, CodePointRange{0x007f4, 0x007f5},
    CodePointRange{0x007fa, 0x007fa}, CodePointRange{0x00800, 0x00815},
    CodePointRange{0x0081a, 0x0081a}, CodePointRange{0x00824, 0x00824},
    CodePointRange{0x00828, 0x00828}, CodePointRange{0x00840, 0x00858},
    CodePointRange{0x00860, 0x0086a}, CodePointRange{0x00870, 0x00887},
    CodePointRange{0x00889, 0x0088e}, CodePointRange{0x008a0, 0x008c9},
    CodePointRange{0x00904, 0x00939}, CodePointRange{0x0093d, 0x0093d},
    CodePointRange{0x00950, 0x00950}, CodePointRange{0x00958, 0x00961},
    CodePointRange{0x00971, 0x00980}, CodePointRange{0x00985, 0x0098c},
    CodePointRange{0x0098f, 0x00990}, CodePointRange{0x00993, 0x009a8},
    CodePointRange{0x009aa, 0x009b0}, CodePointRange{0x009b2, 0x009b2},
    CodePointRange{0x009b6, 0x009b9}, CodePointRange{0x009bd, 0x009bd},
    CodePointRange{0x009ce, 0x009ce}, CodePointRange{0x009dc, 0x009dd},
    CodePointRange{0x009df, 0x009e1}, CodePointRange{0x009f0, 0x009f1},
    CodePointRange{0x009fc, 0x009fc}, CodePointRange{0x00a05, 0x00a0a},
    CodePointRange{0x00a0f, 0x00a10}, CodePointRange{0x00a13, 0x00a28},
    CodePointRange{0x00a2a, 0x00a30}, CodePointRange{0x00a32, 0x00a33},
    CodePointRange{0x00a35, 0x00a36}, CodePointRange{0x00a38, 0x00a39},
    CodePointRange{0x00a59, 0x00a5c}, CodePointRange{0x00a5e, 0x00a5e},
    CodePointRange{0x00a72, 0x00a74}, CodePointRange{0x00a85, 0x00a8d},
    CodePointRange{0x00a8f, 0x00a91}, CodePointRange{0x00a93, 0x00aa8},
    CodePointRange{0x00aaa, 0x00ab0}, CodePointRange{0x00ab2, 0x00ab3},
    CodePointRange{0x00ab5, 0x00ab9}, CodePointRange{0x00abd, 0x00abd},
    CodePointRange{0x00ad0, 0x00ad0}, CodePointRange{0x00ae0, 0x00ae1},
    CodePointRange{0x00af9, 0x00af9}, CodePointRange{0x00b05, 0x00b0c},
    CodePointRange{0x00b0f, 0x00b10}, CodePointRange{0x00b13, 0x00b28},
    CodePointRange{0x00b2a, 0x00b30}, CodePointRange{0x00b32, 0x00b33},
    CodePointRange{0x00b35, 0x00b39}, CodePointRange{0x00b3d, 0x00b3d},
    CodePointRange{0x00b5c, 0x00b5d}, CodePointRange{0x00b5f, 0x00b61},
    CodePointRange{0x00b71, 0x00b71}, CodePointRange{0x00b83, 0x00b83},
    CodePointRange{0x00b85, 0x00b8a}, CodePointRange{0x00b8e, 0x00b90},
    CodePointRange{0x00b92, 0x00b95}, CodePointRange{0x00b99, 0x00b9a},
    CodePointRange{0x00b9c, 0x00b9c}, CodePointRange{0x00b9e, 0x00b9f},
    CodePointRange{0x00ba3, 0x00ba4}, CodePointRange{0x00ba8, 0x00baa},
    CodePointRange{0x00bae, 0x00bb9}, CodePointRange{0x00bd0, 0x00bd0},
    CodePointRange{0x00c05, 0x00c0c}, CodePointRange{0x00c0e, 0x00c10},
    CodePointRange{0x00c12, 0x00c28}, CodePointRange{0x00c2a, 0x00c39},
    CodePointRange{0x00c3d, 0x00c3d}, CodePointRange{0x00c58, 0x00c5a},
    CodePointRange{0x00c5d, 0x00c5d}, CodePointRange{0x00c60, 0x00c61},
    CodePointRange{0x00c80, 0x00c80}, CodePointRange{0x00c85, 0x00c8c},
    CodePointRange{0x00c8e, 0x00c90}, CodePointRange{0x00c92, 0x00ca8},
    CodePointRange{0x00caa, 0x00cb3}, CodePointRange{0x00cb5, 0x00cb9},
    CodePointRange{0x00cbd, 0x00cbd}, CodePointRange{0x00cdd, 0x00cde},
    CodePointRange{0x00ce0, 0x00ce1}, CodePointRange{0x00cf1, 0x00cf2},
    CodePointRange{0x00d04, 0x00d0c}, CodePointRange{0x00d0e, 0x00d10},
    CodePointRange{0x00d12, 0x00d3a}, CodePointRange{0x00d3d, 0x00d3d},
    CodePointRange{0x00d4e, 0x00d4e}, CodePointRange{0x00d54, 0x00d56},
    CodePointRange{0x00d5f, 0x00d61}, CodePointRange{0x00d7a, 0x00d7f},
    CodePointRange{0x00d85, 0x00d96}, CodePointRange{0x00d9a, 0x00db1},
    CodePointRange{0x00db3, 0x00dbb}, CodePointRange{0x00dbd, 0x00dbd},
    CodePointRange{0x00dc0, 0x00dc6}, CodePointRange{0x00e01, 0x00e30},
    CodePointRange{0x00e32, 0x00e32}, CodePointRange{0x00e40, 0x00e46},
    CodePointRange{0x00e81, 0x00e82}, CodePointRange{0x00e84, 0x00e84},
    CodePointRange{0x00e86, 0x00e8a}, CodePointRange{0x00e8c, 0x00ea3},
    CodePointRange{0x00ea5, 0x00ea5}, CodePointRange{0x00ea7, 0x00eb0},
    CodePointRange{0x00eb2, 0x00eb2}, CodePointRange{0x00ebd, 0x00ebd},
    CodePointRange{0x00ec0, 0x00ec4}, CodePointRange{0x00ec6, 0x00ec6},
    CodePointRange{0x00edc, 0x00edf}, CodePointRange{0x00f00, 0x00f00},
    CodePointRange{0x00f40, 0x00f47}, CodePointRange{0x00f49, 0x00f6c},
    CodePointRange{0x00f88, 0x00f8c}, CodePointRange{0x01000, 0x0102a},
    CodePointRange{0x0103f, 0x0103f}, CodePointRange{0x01050, 0x01055},
    CodePointRange{0x0105a, 0x0105d}, CodePointRange{0x01061, 0x01061},
    CodePointRange{0x01065, 0x01066}, CodePointRange{0x0106e, 0x01070},
    CodePointRange{0x01075, 0x01081}, CodePointRange{0x0108e, 0x0108e},
    CodePointRange{0x010a0, 0x010c5}, CodePointRange{0x010c7, 0x010c7},
    CodePointRange{0x010cd, 0x010cd}, CodePointRange{0x010d0, 0x010fa},
    CodePointRange{0x010fc, 0x01248}, CodePointRange{0x0124a, 0x0124d},
    CodePointRange{0x01250, 0x01256}, CodePointRange{0x01258, 0x01258},
    CodePointRange{0x0125a, 0x0125d}, CodePointRange{0x01260, 0x01288},
    CodePointRange{0x0128a, 0x0128d}, CodePointRange{0x01290, 0x012b0},
    CodePointRange{0x012b2, 0x012b5}, CodePointRange{0x012b8, 0x012be},
    CodePointRange{0x012c0, 0x012c0}, CodePointRange{0x012c2, 0x012c5},
    CodePointRange{0x012c8, 0x012d6}, CodePointRange{0x012d8, 0x01310},
    CodePointRange{0x01312, 0x01315}, CodePointRange{0x01318, 0x0135a},
    CodePointRange{0x01380, 0x0138f}, CodePointRange{0x013a0, 0x013f5},
    CodePointRange{0x013f8, 0x013fd}, CodePointRange{0x01401, 0x0166c},
    CodePointRange{0x0166f, 0x0167f}, CodePointRange{0x01681, 0x0169a},
    CodePointRange{0x016a0, 0x016ea}, CodePointRange{0x016ee, 0x016f8},
    CodePointRange{0x01700, 0x01711}, CodePointRange{0x0171f, 0x01731},
    CodePointRange{0x01740, 0x01751}, CodePointRange{0x01760, 0x0176c},
    CodePointRange{0x0176e, 0x01770}, CodePointRange{0x01780, 0x017b3},
    CodePointRange{0x017d7, 0x017d7}, CodePointRange{0x017dc, 0x017dc},
    CodePointRange{0x01820, 0x01878}, CodePointRange{0x01880, 0x018a8},
    CodePointRange{0x018aa, 0x018aa}, CodePointRange{0x018b0, 0x018f5},
    CodePointRange{0x01900, 0x0191e}, CodePointRange{0x01950, 0x0196d},
    CodePointRange{0x01970, 0x01974}, CodePointRange{0x01980, 0x019ab},
    CodePointRange{0x019b0, 0x019c9}, CodePointRange{0x01a00, 0x01a16},
    CodePointRange{0x01a20, 0x01a54}, CodePointRange{0x01aa7, 0x01aa7},
    CodePointRange{0x01b05, 0x01b33}, CodePointRange{0x01b45, 0x01b4c},
    CodePointRange{0x01b83, 0x01ba0}, CodePointRange{0x01bae, 0x01baf},
    CodePointRange{0x01bba, 0x01be5}, CodePointRange{0x01c00, 0x01c23},
    CodePointRange{0x01c4d, 0x01c4f}, CodePointRange{0x01c5a, 0x01c7d},
    CodePointRange{0x01c80, 0x01c88}, CodePointRange{0x01c90, 0x01cba},
    CodePointRange{0x01cbd, 0x01cbf}, CodePointRange{0x01ce9, 0x01cec},
    CodePointRange{0x01cee, 0x01cf3}, CodePointRange{0x01cf5, 0x01cf6},
    CodePointRange{0x01cfa, 0x01cfa}, CodePointRange{0x01d00, 0x01dbf},
    CodePointRange{0x01e00, 0x01f15}, CodePointRange{0x01f18, 0x01f1d},
    CodePointRange{0x01f20, 0x01f45}, CodePointRange{0x01f48, 0x01f4d},
    CodePointRange{0x01f50, 0x01f57}, CodePointRange{0x01f59, 0x01f59},
    CodePointRange{0x01f5b, 0x01f5b}, CodePointRange{0x01f5d, 0x01f5d},
    CodePointRange{0x01f5f, 0x01f7d}, CodePointRange{0x01f80, 0x01fb4},
    CodePointRange{0x01fb6, 0x01fbc}, CodePointRange{0x01fbe, 0x01fbe},
    CodePointRange{0x01fc2, 0x01fc4}, CodePointRange{0x01fc6, 0x01fcc},
    CodePointRange{0x01fd0, 0x01fd3}, CodePointRange{0x01fd6, 0x01fdb},
    CodePointRange{0x01fe0, 0x01fec}, CodePointRange{0x01ff2, 0x01ff4},
    CodePointRange{0x01ff6, 0x01ffc}, CodePointRange{0x02071, 0x02071},
    CodePointRange{0x0207f, 0x0207f}, CodePointRange{0x02090, 0x0209c},
    CodePointRange{0x02102, 0x02102}, CodePointRange{0x02107, 0x02107},
    CodePointRange{0x0210a, 0x02113}, CodePointRange{0x02115, 0x02115},
    CodePointRange{0x02118, 0x0211d}, CodePointRange{0x02124, 0x02124},
    CodePointRange{0x02126, 0x02126}, CodePointRange{0x02128, 0x02128},
    CodePointRange{0x0212a, 0x02139}, CodePointRange{0x0213c, 0x0213f},
    CodePointRange{0x02145, 0x02149}, CodePointRange{0x0214e, 0x0214e},
    CodePointRange{0x02160, 0x02188}, CodePointRange{0x02c00, 0x02ce4},
    CodePointRange{0x02ceb, 0x02cee}, CodePointRange{0x02cf2, 0x02cf3},
    CodePointRange{0x02d00, 0x02d25}, CodePointRange{0x02d27, 0x02d27},
    CodePointRange{0x02d2d, 0x02d2d}, CodePointRange{0x02d30, 0x02d67},
    CodePointRange{0x02d6f, 0x02d6f}, CodePointRange{0x02d80, 0x02d96},
    CodePointRange{0x02da0, 0x02da6}, CodePointRange{0x02da8, 0x02dae},
    CodePointRange{0x02db0, 0x02db6}, CodePointRange{0x02db8, 0x02dbe},
    CodePointRange{0x02dc0, 0x02dc6}, CodePointRange{0x02dc8, 0x02dce},
    CodePointRange{0x02dd0, 0x02dd6}, CodePointRange{0x02dd8, 0x02dde},
    CodePointRange{0x03005, 0x03007}, CodePointRange{0x03021, 0x03029},
    CodePointRange{0x03031, 0x03035}, CodePointRange{0x03038, 0x0303c},
    CodePointRange{0x03041, 0x03096}, CodePointRange{0x0309d, 0x0309f},
    CodePointRange{0x030a1, 0x030fa}, CodePointRange{0x030fc, 0x030ff},
    CodePointRange{0x03105, 0x0312f}, CodePointRange{0x03131, 0x0318e},
    CodePointRange{0x031a0, 0x031bf}, CodePointRange{0x031f0, 0x031ff},
    CodePointRange{0x03400, 0x04dbf}, CodePointRange{0x04e00, 0x0a48c},
    CodePointRange{0x0a4d0, 0x0a4fd}, CodePointRange{0x0a500, 0x0a60c},
    CodePointRange{0x0a610, 0x0a61f}, CodePointRange{0x0a62a, 0x0a62b},
    CodePointRange{0x0a640, 0x0a66e}, CodePointRange{0x0a67f, 0x0a69d},
    CodePointRange{0x0a6a0, 0x0a6ef}, CodePointRange{0x0a717, 0x0a71f},
    CodePointRange{0x0a722, 0x0a788}, CodePointRange{0x0a78b, 0x0a7ca},
    CodePointRange{0x0a7d0, 0x0a7d1}, CodePointRange{0x0a7d3, 0x0a7d3},
    CodePointRange{0x0a7d5, 0x0a7d9}, CodePointRange{0x0a7f2, 0x0a801},
    CodePointRange{0x0a803, 0x0a805}, CodePointRange{0x0a807, 0x0a80a},
    CodePointRange{0x0a80c, 0x0a822}, CodePointRange{0x0a840, 0x0a873},
    CodePointRange{0x0a882, 0x0a8b3}, CodePointRange{0x0a8f2, 0x0a8f7},
    CodePointRange{0x0a8fb, 0x0a8fb}, CodePointRange{0x0a8fd, 0x0a8fe},
    CodePointRange{0x0a90a, 0x0a925}, CodePointRange{0x0a930, 0x0a946},
    CodePointRange{0x0a960, 0x0a97c}, CodePointRange{0x0a984, 0x0a9b2},
    CodePointRange{0x0a9cf, 0x0a9cf}, CodePointRange{0x0a9e0, 0x0a9e4},
    CodePointRange{0x0a9e6, 0x0a9ef}, CodePointRange{0x0a9fa, 0x0a9fe},
    CodePointRange{0x0aa00, 0x0aa28}, CodePointRange{0x0aa40, 0x0aa42},
    CodePointRange{0x0aa44, 0x0aa4b}, CodePointRange{0x0aa60, 0x0aa76},
    CodePointRange{0x0aa7a, 0x0aa7a}, CodePointRange{0x0aa7e, 0x0aaaf},
    CodePointRange{0x0aab1, 0x0aab1}, CodePointRange{0x0aab5, 0x0aab6},
    CodePointRange{0x0aab9, 0x0aabd}, CodePointRange{0x0aac0, 0x0aac0},
    CodePointRange{0x0aac2, 0x0aac2}, CodePointRange{0x0aadb, 0x0aadd},
    CodePointRange{0x0aae0, 0x0aaea}, CodePointRange{0x0aaf2, 0x0aaf4},
    CodePointRange{0x0ab01, 0x0ab06}, CodePointRange{0x0ab09, 0x0ab0e},
    CodePointRange{0x0ab11, 0x0ab16}, CodePointRange{0x0ab20, 0x0ab26},
    CodePointRange{0x0ab28, 0x0ab2e}, CodePointRange{0x0ab30, 0x0ab5a},
    CodePointRange{0x0ab5c, 0x0ab69}, CodePointRange{0x0ab70, 0x0abe2},
    CodePointRange{0x0ac00, 0x0d7a3}, CodePointRange{0x0d7b0, 0x0d7c6},
    CodePointRange{0x0d7cb, 0x0d7fb}, CodePointRange{0x0f900, 0x0fa6d},
    CodePointRange{0x0fa70, 0x0fad9}, CodePointRange{0x0fb00, 0x0fb06},
    CodePointRange{0x0fb13, 0x0fb17}, CodePointRange{0x0fb1d, 0x0fb1d},
    CodePointRange{0x0fb1f, 0x0fb28}, CodePointRange{0x0fb2a, 0x0fb36},
    CodePointRange{0x0fb38, 0x0fb3c}, CodePointRange{0x0fb3e, 0x0fb3e},
    CodePointRange{0x0fb40, 0x0fb41}, CodePointRange{0x0fb43, 0x0fb44},
    CodePointRange{0x0fb46, 0x0fbb1}, CodePointRange{0x0fbd3, 0x0fc5d},
    CodePointRange{0x0fc64, 0x0fd3d}, CodePointRange{0x0fd50, 0x0fd8f},
    CodePointRange{0x0fd92, 0x0fdc7}, CodePointRange{0x0fdf0, 0x0fdf9},
    CodePointRange{0x0fe71, 0x0fe71}, CodePointRange{0x0fe73, 0x0fe73},
    CodePointRange{0x0fe77, 0x0fe77}, CodePointRange{0x0fe79, 0x0fe79},
    CodePointRange{0x0fe7b, 0x0fe7b}, CodePointRange{0x0fe7d, 0x0fe7d},
    CodePointRange{0x0fe7f, 0x0fefc}, CodePointRange{0x0ff21, 0x0ff3a},
    CodePointRange{0x0ff41, 0x0ff5a}, CodePointRange{0x0ff66, 0x0ff9d},
    CodePointRange{0x0ffa0, 0x0ffbe}, CodePointRange{0x0ffc2, 0x0ffc7},
    CodePointRange{0x0ffca, 0x0ffcf}, CodePointRange{0x0ffd2, 0x0ffd7},
    CodePointRange{0x0ffda, 0x0ffdc}, CodePointRange{0x10000, 0x1000b},
    CodePointRange{0x1000d, 0x10026}, CodePointRange{0x10028, 0x1003a},
    CodePointRange{0x1003c, 0x1003d}, CodePointRange{0x1003f, 0x1004d},
    CodePointRange{0x10050, 0x1005d}, CodePointRange{0x10080, 0x100fa},
    CodePointRange{0x10140, 0x10174}, CodePointRange{0x10280, 0x1029c},
    CodePointRange{0x102a0, 0x102d0}, CodePointRange{0x10300, 0x1031f},
    CodePointRange{0x1032d, 0x1034a}, CodePointRange{0x10350, 0x10375},
    CodePointRange{0x10380, 0x1039d}, CodePointRange{0x103a0, 0x103c3},
    CodePointRange{0x103c8, 0x103cf}, CodePointRange{0x103d1, 0x103d5},
    CodePointRange{0x10400, 0x1049d}, CodePointRange{0x104b0, 0x104d3},
    CodePointRange{0x104d8, 0x104fb}, CodePointRange{0x10500, 0x10527},
    CodePointRange{0x10530, 0x10563}, CodePointRange{0x10570, 0x1057a},
    CodePointRange{0x1057c, 0x1058a}, CodePointRange{0x1058c, 0x10592},
    CodePointRange{0x10594, 0x10595}, CodePointRange{0x10597, 0x105a1},
    CodePointRange{0x105a3, 0x105b1}, CodePointRange{0x105b3, 0x105b9},
    CodePointRange{0x105bb, 0x105bc}, CodePointRange{0x10600, 0x10736},
    CodePointRange{0x10740, 0x10755}, CodePointRange{0x10760, 0x10767},
    CodePointRange{0x10780, 0x10785}, CodePointRange{0x10787, 0x107b0},
    CodePointRange{0x107b2, 0x107ba}, CodePointRange{0x10800, 0x10805},
    CodePointRange{0x10808, 0x10808}, CodePointRange{0x1080a, 0x10835},
    CodePointRange{0x10837, 0x10838}, CodePointRange{0x1083c, 0x1083c},
    CodePointRange{0x1083f, 0x10855}, CodePointRange{0x10860, 0x10876},
    CodePointRange{0x10880, 0x1089e}, CodePointRange{0x108e0, 0x108f2},
    CodePointRange{0x108f4, 0x108f5}, CodePointRange{0x10900, 0x10915},
    CodePointRange{0x10920, 0x10939}, CodePointRange{0x10980, 0x109b7},
    CodePointRange{0x109be, 0x109bf}, CodePointRange{0x10a00, 0x10a00},
    CodePointRange{0x10a10, 0x10a13}, CodePointRange{0x10a15, 0x10a17},
    CodePointRange{0x10a19, 0x10a35}, CodePointRange{0x10a60, 0x10a7c},
    CodePointRange{0x10a80, 0x10a9c}, CodePointRange{0x10ac0, 0x10ac7},
    CodePointRange{0x10ac9, 0x10ae4}, CodePointRange{0x10b00, 0x10b35},
    CodePointRange{0x10b40, 0x10b55}, CodePointRange{0x10b60, 0x10b72},
    CodePointRange{0x10b80, 0x10b91}, CodePointRange{0x10c00, 0x10c48},
    CodePointRange{0x10c80, 0x10cb2}, CodePointRange{0x10cc0, 0x10cf2},
    CodePointRange{0x10d00, 0x10d23}, CodePointRange{0x10e80, 0x10ea9},
    CodePointRange{0x10eb0, 0x10eb1}, CodePointRange{0x10f00, 0x10f1c},
    CodePointRange{0x10f27, 0x10f27}, CodePointRange{0x10f30, 0x10f45},
    CodePointRange{0x10f70, 0x10f81}, CodePointRange{0x10fb0, 0x10fc4},
    CodePointRange{0x10fe0, 0x10ff6}, CodePointRange{0x11003, 0x11037},
    CodePointRange{0x11071, 0x11072}, CodePointRange{0x11075, 0x11075},
    CodePointRange{0x11083, 0x110af}, CodePointRange{0x110d0, 0x110e8},
    CodePointRange{0x11103, 0x11126}, CodePointRange{0x11144, 0x11144},
    CodePointRange{0x11147, 0x11147}, CodePointRange{0x11150, 0x11172},
    CodePointRange{0x11176, 0x11176}, CodePointRange{0x11183, 0x111b2},
    CodePointRange{0x111c1, 0x111c4}, CodePointRange{0x111da, 0x111da},
    CodePointRange{0x111dc, 0x111dc}, CodePointRange{0x11200, 0x11211},
    CodePointRange{0x11213, 0x1122b}, CodePointRange{0x11280, 0x11286},
    CodePointRange{0x11288, 0x11288}, CodePointRange{0x1128a, 0x1128d},
    CodePointRange{0x1128f, 0x1129d}, CodePointRange{0x1129f, 0x112a8},
    CodePointRange{0x112b0, 0x112de}, CodePointRange{0x11305, 0x1130c},
    CodePointRange{0x1130f, 0x11310}, CodePointRange{0x11313, 0x11328},
    CodePointRange{0x1132a, 0x11330}, CodePointRange{0x11332, 0x11333},
    CodePointRange{0x11335, 0x11339}, CodePointRange{0x1133d, 0x1133d},
    CodePointRange{0x11350, 0x11350}, CodePointRange{0x1135d, 0x11361},
    CodePointRange{0x11400, 0x11434}, CodePointRange{0x11447, 0x1144a},
    CodePointRange{0x1145f, 0x11461}, CodePointRange{0x11480, 0x114af},
    CodePointRange{0x114c4, 0x114c5}, CodePointRange{0x114c7, 0x114c7},
    CodePointRange{0x11580, 0x115ae}, CodePointRange{0x115d8, 0x115db},
    CodePointRange{0x11600, 0x1162f}, CodePointRange{0x11644, 0x11644},
    CodePointRange{0x11680, 0x116aa}, CodePointRange{0x116b8, 0x116b8},
    CodePointRange{0x11700, 0x1171a}, CodePointRange{0x11740, 0x11746},
    CodePointRange{0x11800, 0x1182b}, CodePointRange{0x118a0, 0x118df},
    CodePointRange{0x118ff, 0x11906}, CodePointRange{0x11909, 0x11909},
    CodePointRange{0x1190c, 0x11913}, CodePointRange{0x11915, 0x11916},
    CodePointRange{0x11918, 0x1192f}, CodePointRange{0x1193f, 0x1193f},
    CodePointRange{0x11941, 0x11941}, CodePointRange{0x119a0, 0x119a7},
    CodePointRange{0x119aa, 0x119d0}, CodePointRange{0x119e1, 0x119e1},
    CodePointRange{0x119e3, 0x119e3}, CodePointRange{0x11a00, 0x11a00},
    CodePointRange{0x11a0b, 0x11a32}, CodePointRange{0x11a3a, 0x11a3a},
    CodePointRange{0x11a50, 0x11a50}, CodePointRange{0x11a5c, 0x11a89},
    CodePointRange{0x11a9d, 0x11a9d}, CodePointRange{0x11ab0, 0x11af8},
    CodePointRange{0x11c00, 0x11c08}, CodePointRange{0x11c0a, 0x11c2e},
    CodePointRange{0x11c40, 0x11c40}, CodePointRange{0x11c72, 0x11c8f},
    CodePointRange{0x11d00, 0x11d06}, CodePointRange{0x11d08, 0x11d09},
    CodePointRange{0x11d0b, 0x11d30}, CodePointRange{0x11d46, 0x11d46},
    CodePointRange{0x11d60, 0x11d65}, CodePointRange{0x11d67, 0x11d68},
    CodePointRange{0x11d6a, 0x11d89}, CodePointRange{0x11d98, 0x11d98},
    CodePointRange{0x11ee0, 0x11ef2}, CodePointRange{0x11fb0, 0x11fb0},
    CodePointRange{0x12000, 0x12399}, CodePointRange{0x12400, 0x1246e},
    CodePointRange{0x12480, 0x12543}, CodePointRange{0x12f90, 0x12ff0},
    CodePointRange{0x13000, 0x1342e}, CodePointRange{0x14400, 0x14646},
    CodePointRange{0x16800, 0x16a38}, CodePointRange{0x16a40, 0x16a5e},
    CodePointRange{0x16a70, 0x16abe}, CodePointRange{0x16ad0, 0x16aed},
    CodePointRange{0x16b00, 0x16b2f}, CodePointRange{0x16b40, 0x16b43},
    CodePointRange{0x16b63, 0x16b77}, CodePointRange{0x16b7d, 0x16b8f},
    CodePointRange{0x16e40, 0x16e7f}, CodePointRange{0x16f00, 0x16f4a},
    CodePointRange{0x16f50, 0x16f50}, CodePointRange{0x16f93, 0x16f9f},
    CodePointRange{0x16fe0, 0x16fe1}, CodePointRange{0x16fe3, 0x16fe3},
    CodePointRange{0x17000, 0x187f7}, CodePointRange{0x18800, 0x18cd5},
    CodePointRange{0x18d00, 0x18d08}, CodePointRange{0x1aff0, 0x1aff3},
    CodePointRange{0x1aff5, 0x1affb}, CodePointRange{0x1affd, 0x1affe},
    CodePointRange{0x1b000, 0x1b122}, CodePointRange{0x1b150, 0x1b152},
    CodePointRange{0x1b164, 0x1b167}, CodePointRange{0x1b170, 0x1b2fb},
    CodePointRange{0x1bc00, 0x1bc6a}, CodePointRange{0x1bc70, 0x1bc7c},
    CodePointRange{0x1bc80, 0x1bc88}, CodePointRange{0x1bc90, 0x1bc99},
    CodePointRange{0x1d400, 0x1d454}, CodePointRange{0x1d456, 0x1d49c},
    CodePointRange{0x1d49e, 0x1d49f}, CodePointRange{0x1d4a2, 0x1d4a2},
    CodePointRange{0x1d4a5, 0x1d4a6}, CodePointRange{0x1d4a9, 0x1d4ac},
    CodePointRange{0x1d4ae, 0x1d4b9}, CodePointRange{0x1d4bb, 0x1d4bb},
    CodePointRange{0x1d4bd, 0x1d4c3}, CodePointRange{0x1d4c5, 0x1d505},
    CodePointRange{0x1d507, 0x1d50a}, CodePointRange{0x1d50d, 0x1d514},
    CodePointRange{0x1d516, 0x1d51c}, CodePointRange{0x1d51e, 0x1d539},
    CodePointRange{0x1d53b, 0x1d53e}, CodePointRange{0x1d540, 0x1d544},
    CodePointRange{0x1d546, 0x1d546}, CodePointRange{0x1d54a, 0x1d550},
    CodePointRange{0x1d552, 0x1d6a5}, CodePointRange{0x1d6a8, 0x1d6c0},
    CodePointRange{0x1d6c2, 0x1d6da}, CodePointRange{0x1d6dc, 0x1d6fa},
    CodePointRange{0x1d6fc, 0x1d714}, CodePointRange{0x1d716, 0x1d734},
    CodePointRange{0x1d736, 0x1d74e}, CodePointRange{0x1d750, 0x1d76e},
    CodePointRange{0x1d770, 0x1d788}, CodePointRange{0x1d78a, 0x1d7a8},
    CodePointRange{0x1d7aa, 0x1d7c2}, CodePointRange{0x1d7c4, 0x1d7cb},
    CodePointRange{0x1df00, 0x1df1e}, CodePointRange{0x1e100, 0x1e12c},
    CodePointRange{0x1e137, 0x1e13d}, CodePointRange{0x1e14e, 0x1e14e},
    CodePointRange{0x1e290, 0x1e2ad}, CodePointRange{0x1e2c0, 0x1e2eb},
    CodePointRange{0x1e7e0, 0x1e7e6}, CodePointRange{0x1e7e8, 0x1e7eb},
    CodePointRange{0x1e7ed, 0x1e7ee}, CodePointRange{0x1e7f0, 0x1e7fe},
    CodePointRange{0x1e800, 0x1e8c4}, CodePointRange{0x1e900, 0x1e943},
    CodePointRange{0x1e94b, 0x1e94b}, CodePointRange{0x1ee00, 0x1ee03},
    CodePointRange{0x1ee05, 0x1ee1f}, CodePointRange{0x1ee21, 0x1ee22},
    CodePointRange{0x1ee24, 0x1ee24}, CodePointRange{0x1ee27, 0x1ee27},
    CodePointRange{0x1ee29, 0x1ee32}, CodePointRange{0x1ee34, 0x1ee37},
    CodePointRange{0x1ee39, 0x1ee39}, CodePointRange{0x1ee3b, 0x1ee3b},
    CodePointRange{0x1ee42, 0x1ee42}, CodePointRange{0x1ee47, 0x1ee47},
    CodePointRange{0x1ee49, 0x1ee49}, CodePointRange{0x1ee4b, 0x1ee4b},
    CodePointRange{0x1ee4d, 0x1ee4f}, CodePointRange{0x1ee51, 0x1ee52},
    CodePointRange{0x1ee54, 0x1ee54}, CodePointRange{0x1ee57, 0x1ee57},
    CodePointRange{0x1ee59, 0x1ee59}, CodePointRange{0x1ee5b, 0x1ee5b},
    CodePointRange{0x1ee5d, 0x1ee5d}, CodePointRange{0x1ee5f, 0x1ee5f},
    CodePointRange{0x1ee61, 0x1ee62}, CodePointRange{0x1ee64, 0x1ee64},
    CodePointRange{0x1ee67, 0x1ee6a}, CodePointRange{0x1ee6c, 0x1ee72},
    CodePointRange{0x1ee74, 0x1ee77}, CodePointRange{0x1ee79, 0x1ee7c},
    CodePointRange{0x1ee7e, 0x1ee7e}, CodePointRange{0x1ee80, 0x1ee89},
    CodePointRange{0x1ee8b, 0x1ee9b}, CodePointRange{0x1eea1, 0x1eea3},
    CodePointRange{0x1eea5, 0x1eea9}, CodePointRange{0x1eeab, 0x1eebb},
    CodePointRange{0x20000, 0x2a6df}, CodePointRange{0x2a700, 0x2b738},
    CodePointRange{0x2b740, 0x2b81d}, CodePointRange{0x2b820, 0x2cea1},
    CodePointRange{0x2ceb0, 0x2ebe0}, CodePointRange{0x2f800, 0x2fa1d},
    CodePointRange{0x30000, 0x3134a},
};

// The additional code point interval ranges for the Unicode 14 XID_Continue
// set. This extends the values in kXIDStartRanges.
// This array needs to be in ascending order.
constexpr std::array<CodePointRange, 363> kXIDContinueRanges = {
    CodePointRange{0x00030, 0x00039}, CodePointRange{0x0005f, 0x0005f},
    CodePointRange{0x000b7, 0x000b7}, CodePointRange{0x00300, 0x0036f},
    CodePointRange{0x00387, 0x00387}, CodePointRange{0x00483, 0x00487},
    CodePointRange{0x00591, 0x005bd}, CodePointRange{0x005bf, 0x005bf},
    CodePointRange{0x005c1, 0x005c2}, CodePointRange{0x005c4, 0x005c5},
    CodePointRange{0x005c7, 0x005c7}, CodePointRange{0x00610, 0x0061a},
    CodePointRange{0x0064b, 0x00669}, CodePointRange{0x00670, 0x00670},
    CodePointRange{0x006d6, 0x006dc}, CodePointRange{0x006df, 0x006e4},
    CodePointRange{0x006e7, 0x006e8}, CodePointRange{0x006ea, 0x006ed},
    CodePointRange{0x006f0, 0x006f9}, CodePointRange{0x00711, 0x00711},
    CodePointRange{0x00730, 0x0074a}, CodePointRange{0x007a6, 0x007b0},
    CodePointRange{0x007c0, 0x007c9}, CodePointRange{0x007eb, 0x007f3},
    CodePointRange{0x007fd, 0x007fd}, CodePointRange{0x00816, 0x00819},
    CodePointRange{0x0081b, 0x00823}, CodePointRange{0x00825, 0x00827},
    CodePointRange{0x00829, 0x0082d}, CodePointRange{0x00859, 0x0085b},
    CodePointRange{0x00898, 0x0089f}, CodePointRange{0x008ca, 0x008e1},
    CodePointRange{0x008e3, 0x00903}, CodePointRange{0x0093a, 0x0093c},
    CodePointRange{0x0093e, 0x0094f}, CodePointRange{0x00951, 0x00957},
    CodePointRange{0x00962, 0x00963}, CodePointRange{0x00966, 0x0096f},
    CodePointRange{0x00981, 0x00983}, CodePointRange{0x009bc, 0x009bc},
    CodePointRange{0x009be, 0x009c4}, CodePointRange{0x009c7, 0x009c8},
    CodePointRange{0x009cb, 0x009cd}, CodePointRange{0x009d7, 0x009d7},
    CodePointRange{0x009e2, 0x009e3}, CodePointRange{0x009e6, 0x009ef},
    CodePointRange{0x009fe, 0x009fe}, CodePointRange{0x00a01, 0x00a03},
    CodePointRange{0x00a3c, 0x00a3c}, CodePointRange{0x00a3e, 0x00a42},
    CodePointRange{0x00a47, 0x00a48}, CodePointRange{0x00a4b, 0x00a4d},
    CodePointRange{0x00a51, 0x00a51}, CodePointRange{0x00a66, 0x00a71},
    CodePointRange{0x00a75, 0x00a75}, CodePointRange{0x00a81, 0x00a83},
    CodePointRange{0x00abc, 0x00abc}, CodePointRange{0x00abe, 0x00ac5},
    CodePointRange{0x00ac7, 0x00ac9}, CodePointRange{0x00acb, 0x00acd},
    CodePointRange{0x00ae2, 0x00ae3}, CodePointRange{0x00ae6, 0x00aef},
    CodePointRange{0x00afa, 0x00aff}, CodePointRange{0x00b01, 0x00b03},
    CodePointRange{0x00b3c, 0x00b3c}, CodePointRange{0x00b3e, 0x00b44},
    CodePointRange{0x00b47, 0x00b48}, CodePointRange{0x00b4b, 0x00b4d},
    CodePointRange{0x00b55, 0x00b57}, CodePointRange{0x00b62, 0x00b63},
    CodePointRange{0x00b66, 0x00b6f}, CodePointRange{0x00b82, 0x00b82},
    CodePointRange{0x00bbe, 0x00bc2}, CodePointRange{0x00bc6, 0x00bc8},
    CodePointRange{0x00bca, 0x00bcd}, CodePointRange{0x00bd7, 0x00bd7},
    CodePointRange{0x00be6, 0x00bef}, CodePointRange{0x00c00, 0x00c04},
    CodePointRange{0x00c3c, 0x00c3c}, CodePointRange{0x00c3e, 0x00c44},
    CodePointRange{0x00c46, 0x00c48}, CodePointRange{0x00c4a, 0x00c4d},
    CodePointRange{0x00c55, 0x00c56}, CodePointRange{0x00c62, 0x00c63},
    CodePointRange{0x00c66, 0x00c6f}, CodePointRange{0x00c81, 0x00c83},
    CodePointRange{0x00cbc, 0x00cbc}, CodePointRange{0x00cbe, 0x00cc4},
    CodePointRange{0x00cc6, 0x00cc8}, CodePointRange{0x00cca, 0x00ccd},
    CodePointRange{0x00cd5, 0x00cd6}, CodePointRange{0x00ce2, 0x00ce3},
    CodePointRange{0x00ce6, 0x00cef}, CodePointRange{0x00d00, 0x00d03},
    CodePointRange{0x00d3b, 0x00d3c}, CodePointRange{0x00d3e, 0x00d44},
    CodePointRange{0x00d46, 0x00d48}, CodePointRange{0x00d4a, 0x00d4d},
    CodePointRange{0x00d57, 0x00d57}, CodePointRange{0x00d62, 0x00d63},
    CodePointRange{0x00d66, 0x00d6f}, CodePointRange{0x00d81, 0x00d83},
    CodePointRange{0x00dca, 0x00dca}, CodePointRange{0x00dcf, 0x00dd4},
    CodePointRange{0x00dd6, 0x00dd6}, CodePointRange{0x00dd8, 0x00ddf},
    CodePointRange{0x00de6, 0x00def}, CodePointRange{0x00df2, 0x00df3},
    CodePointRange{0x00e31, 0x00e31}, CodePointRange{0x00e33, 0x00e3a},
    CodePointRange{0x00e47, 0x00e4e}, CodePointRange{0x00e50, 0x00e59},
    CodePointRange{0x00eb1, 0x00eb1}, CodePointRange{0x00eb3, 0x00ebc},
    CodePointRange{0x00ec8, 0x00ecd}, CodePointRange{0x00ed0, 0x00ed9},
    CodePointRange{0x00f18, 0x00f19}, CodePointRange{0x00f20, 0x00f29},
    CodePointRange{0x00f35, 0x00f35}, CodePointRange{0x00f37, 0x00f37},
    CodePointRange{0x00f39, 0x00f39}, CodePointRange{0x00f3e, 0x00f3f},
    CodePointRange{0x00f71, 0x00f84}, CodePointRange{0x00f86, 0x00f87},
    CodePointRange{0x00f8d, 0x00f97}, CodePointRange{0x00f99, 0x00fbc},
    CodePointRange{0x00fc6, 0x00fc6}, CodePointRange{0x0102b, 0x0103e},
    CodePointRange{0x01040, 0x01049}, CodePointRange{0x01056, 0x01059},
    CodePointRange{0x0105e, 0x01060}, CodePointRange{0x01062, 0x01064},
    CodePointRange{0x01067, 0x0106d}, CodePointRange{0x01071, 0x01074},
    CodePointRange{0x01082, 0x0108d}, CodePointRange{0x0108f, 0x0109d},
    CodePointRange{0x0135d, 0x0135f}, CodePointRange{0x01369, 0x01371},
    CodePointRange{0x01712, 0x01715}, CodePointRange{0x01732, 0x01734},
    CodePointRange{0x01752, 0x01753}, CodePointRange{0x01772, 0x01773},
    CodePointRange{0x017b4, 0x017d3}, CodePointRange{0x017dd, 0x017dd},
    CodePointRange{0x017e0, 0x017e9}, CodePointRange{0x0180b, 0x0180d},
    CodePointRange{0x0180f, 0x01819}, CodePointRange{0x018a9, 0x018a9},
    CodePointRange{0x01920, 0x0192b}, CodePointRange{0x01930, 0x0193b},
    CodePointRange{0x01946, 0x0194f}, CodePointRange{0x019d0, 0x019da},
    CodePointRange{0x01a17, 0x01a1b}, CodePointRange{0x01a55, 0x01a5e},
    CodePointRange{0x01a60, 0x01a7c}, CodePointRange{0x01a7f, 0x01a89},
    CodePointRange{0x01a90, 0x01a99}, CodePointRange{0x01ab0, 0x01abd},
    CodePointRange{0x01abf, 0x01ace}, CodePointRange{0x01b00, 0x01b04},
    CodePointRange{0x01b34, 0x01b44}, CodePointRange{0x01b50, 0x01b59},
    CodePointRange{0x01b6b, 0x01b73}, CodePointRange{0x01b80, 0x01b82},
    CodePointRange{0x01ba1, 0x01bad}, CodePointRange{0x01bb0, 0x01bb9},
    CodePointRange{0x01be6, 0x01bf3}, CodePointRange{0x01c24, 0x01c37},
    CodePointRange{0x01c40, 0x01c49}, CodePointRange{0x01c50, 0x01c59},
    CodePointRange{0x01cd0, 0x01cd2}, CodePointRange{0x01cd4, 0x01ce8},
    CodePointRange{0x01ced, 0x01ced}, CodePointRange{0x01cf4, 0x01cf4},
    CodePointRange{0x01cf7, 0x01cf9}, CodePointRange{0x01dc0, 0x01dff},
    CodePointRange{0x0203f, 0x02040}, CodePointRange{0x02054, 0x02054},
    CodePointRange{0x020d0, 0x020dc}, CodePointRange{0x020e1, 0x020e1},
    CodePointRange{0x020e5, 0x020f0}, CodePointRange{0x02cef, 0x02cf1},
    CodePointRange{0x02d7f, 0x02d7f}, CodePointRange{0x02de0, 0x02dff},
    CodePointRange{0x0302a, 0x0302f}, CodePointRange{0x03099, 0x0309a},
    CodePointRange{0x0a620, 0x0a629}, CodePointRange{0x0a66f, 0x0a66f},
    CodePointRange{0x0a674, 0x0a67d}, CodePointRange{0x0a69e, 0x0a69f},
    CodePointRange{0x0a6f0, 0x0a6f1}, CodePointRange{0x0a802, 0x0a802},
    CodePointRange{0x0a806, 0x0a806}, CodePointRange{0x0a80b, 0x0a80b},
    CodePointRange{0x0a823, 0x0a827}, CodePointRange{0x0a82c, 0x0a82c},
    CodePointRange{0x0a880, 0x0a881}, CodePointRange{0x0a8b4, 0x0a8c5},
    CodePointRange{0x0a8d0, 0x0a8d9}, CodePointRange{0x0a8e0, 0x0a8f1},
    CodePointRange{0x0a8ff, 0x0a909}, CodePointRange{0x0a926, 0x0a92d},
    CodePointRange{0x0a947, 0x0a953}, CodePointRange{0x0a980, 0x0a983},
    CodePointRange{0x0a9b3, 0x0a9c0}, CodePointRange{0x0a9d0, 0x0a9d9},
    CodePointRange{0x0a9e5, 0x0a9e5}, CodePointRange{0x0a9f0, 0x0a9f9},
    CodePointRange{0x0aa29, 0x0aa36}, CodePointRange{0x0aa43, 0x0aa43},
    CodePointRange{0x0aa4c, 0x0aa4d}, CodePointRange{0x0aa50, 0x0aa59},
    CodePointRange{0x0aa7b, 0x0aa7d}, CodePointRange{0x0aab0, 0x0aab0},
    CodePointRange{0x0aab2, 0x0aab4}, CodePointRange{0x0aab7, 0x0aab8},
    CodePointRange{0x0aabe, 0x0aabf}, CodePointRange{0x0aac1, 0x0aac1},
    CodePointRange{0x0aaeb, 0x0aaef}, CodePointRange{0x0aaf5, 0x0aaf6},
    CodePointRange{0x0abe3, 0x0abea}, CodePointRange{0x0abec, 0x0abed},
    CodePointRange{0x0abf0, 0x0abf9}, CodePointRange{0x0fb1e, 0x0fb1e},
    CodePointRange{0x0fe00, 0x0fe0f}, CodePointRange{0x0fe20, 0x0fe2f},
    CodePointRange{0x0fe33, 0x0fe34}, CodePointRange{0x0fe4d, 0x0fe4f},
    CodePointRange{0x0ff10, 0x0ff19}, CodePointRange{0x0ff3f, 0x0ff3f},
    CodePointRange{0x0ff9e, 0x0ff9f}, CodePointRange{0x101fd, 0x101fd},
    CodePointRange{0x102e0, 0x102e0}, CodePointRange{0x10376, 0x1037a},
    CodePointRange{0x104a0, 0x104a9}, CodePointRange{0x10a01, 0x10a03},
    CodePointRange{0x10a05, 0x10a06}, CodePointRange{0x10a0c, 0x10a0f},
    CodePointRange{0x10a38, 0x10a3a}, CodePointRange{0x10a3f, 0x10a3f},
    CodePointRange{0x10ae5, 0x10ae6}, CodePointRange{0x10d24, 0x10d27},
    CodePointRange{0x10d30, 0x10d39}, CodePointRange{0x10eab, 0x10eac},
    CodePointRange{0x10f46, 0x10f50}, CodePointRange{0x10f82, 0x10f85},
    CodePointRange{0x11000, 0x11002}, CodePointRange{0x11038, 0x11046},
    CodePointRange{0x11066, 0x11070}, CodePointRange{0x11073, 0x11074},
    CodePointRange{0x1107f, 0x11082}, CodePointRange{0x110b0, 0x110ba},
    CodePointRange{0x110c2, 0x110c2}, CodePointRange{0x110f0, 0x110f9},
    CodePointRange{0x11100, 0x11102}, CodePointRange{0x11127, 0x11134},
    CodePointRange{0x11136, 0x1113f}, CodePointRange{0x11145, 0x11146},
    CodePointRange{0x11173, 0x11173}, CodePointRange{0x11180, 0x11182},
    CodePointRange{0x111b3, 0x111c0}, CodePointRange{0x111c9, 0x111cc},
    CodePointRange{0x111ce, 0x111d9}, CodePointRange{0x1122c, 0x11237},
    CodePointRange{0x1123e, 0x1123e}, CodePointRange{0x112df, 0x112ea},
    CodePointRange{0x112f0, 0x112f9}, CodePointRange{0x11300, 0x11303},
    CodePointRange{0x1133b, 0x1133c}, CodePointRange{0x1133e, 0x11344},
    CodePointRange{0x11347, 0x11348}, CodePointRange{0x1134b, 0x1134d},
    CodePointRange{0x11357, 0x11357}, CodePointRange{0x11362, 0x11363},
    CodePointRange{0x11366, 0x1136c}, CodePointRange{0x11370, 0x11374},
    CodePointRange{0x11435, 0x11446}, CodePointRange{0x11450, 0x11459},
    CodePointRange{0x1145e, 0x1145e}, CodePointRange{0x114b0, 0x114c3},
    CodePointRange{0x114d0, 0x114d9}, CodePointRange{0x115af, 0x115b5},
    CodePointRange{0x115b8, 0x115c0}, CodePointRange{0x115dc, 0x115dd},
    CodePointRange{0x11630, 0x11640}, CodePointRange{0x11650, 0x11659},
    CodePointRange{0x116ab, 0x116b7}, CodePointRange{0x116c0, 0x116c9},
    CodePointRange{0x1171d, 0x1172b}, CodePointRange{0x11730, 0x11739},
    CodePointRange{0x1182c, 0x1183a}, CodePointRange{0x118e0, 0x118e9},
    CodePointRange{0x11930, 0x11935}, CodePointRange{0x11937, 0x11938},
    CodePointRange{0x1193b, 0x1193e}, CodePointRange{0x11940, 0x11940},
    CodePointRange{0x11942, 0x11943}, CodePointRange{0x11950, 0x11959},
    CodePointRange{0x119d1, 0x119d7}, CodePointRange{0x119da, 0x119e0},
    CodePointRange{0x119e4, 0x119e4}, CodePointRange{0x11a01, 0x11a0a},
    CodePointRange{0x11a33, 0x11a39}, CodePointRange{0x11a3b, 0x11a3e},
    CodePointRange{0x11a47, 0x11a47}, CodePointRange{0x11a51, 0x11a5b},
    CodePointRange{0x11a8a, 0x11a99}, CodePointRange{0x11c2f, 0x11c36},
    CodePointRange{0x11c38, 0x11c3f}, CodePointRange{0x11c50, 0x11c59},
    CodePointRange{0x11c92, 0x11ca7}, CodePointRange{0x11ca9, 0x11cb6},
    CodePointRange{0x11d31, 0x11d36}, CodePointRange{0x11d3a, 0x11d3a},
    CodePointRange{0x11d3c, 0x11d3d}, CodePointRange{0x11d3f, 0x11d45},
    CodePointRange{0x11d47, 0x11d47}, CodePointRange{0x11d50, 0x11d59},
    CodePointRange{0x11d8a, 0x11d8e}, CodePointRange{0x11d90, 0x11d91},
    CodePointRange{0x11d93, 0x11d97}, CodePointRange{0x11da0, 0x11da9},
    CodePointRange{0x11ef3, 0x11ef6}, CodePointRange{0x16a60, 0x16a69},
    CodePointRange{0x16ac0, 0x16ac9}, CodePointRange{0x16af0, 0x16af4},
    CodePointRange{0x16b30, 0x16b36}, CodePointRange{0x16b50, 0x16b59},
    CodePointRange{0x16f4f, 0x16f4f}, CodePointRange{0x16f51, 0x16f87},
    CodePointRange{0x16f8f, 0x16f92}, CodePointRange{0x16fe4, 0x16fe4},
    CodePointRange{0x16ff0, 0x16ff1}, CodePointRange{0x1bc9d, 0x1bc9e},
    CodePointRange{0x1cf00, 0x1cf2d}, CodePointRange{0x1cf30, 0x1cf46},
    CodePointRange{0x1d165, 0x1d169}, CodePointRange{0x1d16d, 0x1d172},
    CodePointRange{0x1d17b, 0x1d182}, CodePointRange{0x1d185, 0x1d18b},
    CodePointRange{0x1d1aa, 0x1d1ad}, CodePointRange{0x1d242, 0x1d244},
    CodePointRange{0x1d7ce, 0x1d7ff}, CodePointRange{0x1da00, 0x1da36},
    CodePointRange{0x1da3b, 0x1da6c}, CodePointRange{0x1da75, 0x1da75},
    CodePointRange{0x1da84, 0x1da84}, CodePointRange{0x1da9b, 0x1da9f},
    CodePointRange{0x1daa1, 0x1daaf}, CodePointRange{0x1e000, 0x1e006},
    CodePointRange{0x1e008, 0x1e018}, CodePointRange{0x1e01b, 0x1e021},
    CodePointRange{0x1e023, 0x1e024}, CodePointRange{0x1e026, 0x1e02a},
    CodePointRange{0x1e130, 0x1e136}, CodePointRange{0x1e140, 0x1e149},
    CodePointRange{0x1e2ae, 0x1e2ae}, CodePointRange{0x1e2ec, 0x1e2f9},
    CodePointRange{0x1e8d0, 0x1e8d6}, CodePointRange{0x1e944, 0x1e94a},
    CodePointRange{0x1e950, 0x1e959}, CodePointRange{0x1fbf0, 0x1fbf9},
    CodePointRange{0xe0100, 0xe01ef},
};

}  // namespace

bool CodePoint::IsXIDStart() const {
    // Short circuit ASCII. The binary search will find these last, but most
    // of our current source is ASCII, so handle them quicker.
    if ((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z')) {
        return true;
    }
    // With [a-zA-Z] handled, nothing less then the next sequence start can be
    // XIDStart, so filter them all out. This catches most of the common symbols
    // that are used in ASCII.
    if (value < 0x000aa) {
        return false;
    }
    return std::binary_search(kXIDStartRanges.cbegin(), kXIDStartRanges.cend(), *this);
}

bool CodePoint::IsXIDContinue() const {
    // Short circuit ASCII. The binary search will find these last, but most
    // of our current source is ASCII, so handle them quicker.
    if ((value >= '0' && value <= '9') || value == '_') {
        return true;
    }
    return IsXIDStart() ||
           std::binary_search(kXIDContinueRanges.cbegin(), kXIDContinueRanges.cend(), *this);
}

namespace utf8 {

// This is a C-style API that will always trigger -Wunsafe-buffer-usage
TINT_BEGIN_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);
std::pair<CodePoint, size_t> Decode(const uint8_t* ptr, size_t len) {
    if (len < 1) {
        return {};
    }
    // Fast-path ASCII characters as they're always valid
    if (ptr[0] <= 0x7f) {
        return {CodePoint{ptr[0]}, 1};
    }

    // Lookup table for the first byte of a UTF-8 sequence.
    // 0 indicates an invalid length.
    // Note that bit encodings that can fit in a smaller number of bytes are
    // invalid (e.g. 0xc0). Code points that exceed the unicode maximum of
    // 0x10FFFF are also invalid (0xf5+).
    // See: https://en.wikipedia.org/wiki/UTF-8#Encoding and
    //      https://datatracker.ietf.org/doc/html/rfc3629#section-3
    static constexpr std::array<uint8_t, 256> kSequenceLength = {
        //         0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
        /* 0x00 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0x10 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0x20 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0x30 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0x40 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0x50 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0x60 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0x70 */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        /* 0x80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 0x90 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 0xa0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 0xb0 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        /* 0xc0 */ 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* 0xd0 */ 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        /* 0xe0 */ 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        /* 0xf0 */ 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    uint8_t n = kSequenceLength[ptr[0]];
    if (n > len) {
        return {};
    }

    CodePoint c;

    uint8_t top_bits = 0b11000000;
    switch (n) {
        // Note: n=0 (invalid) is correctly handled without a case.
        case 1:
            c = CodePoint{ptr[0]};
            break;
        case 2:
            top_bits &= ptr[1] ^ 0b01000000;
            c = CodePoint{(static_cast<uint32_t>(ptr[0] & 0b00011111) << 6) |
                          (static_cast<uint32_t>(ptr[1] & 0b00111111))};
            break;
        case 3:
            top_bits &= (ptr[1] ^ 0b01000000) & (ptr[2] ^ 0b01000000);
            c = CodePoint{(static_cast<uint32_t>(ptr[0] & 0b00001111) << 12) |
                          (static_cast<uint32_t>(ptr[1] & 0b00111111) << 6) |
                          (static_cast<uint32_t>(ptr[2] & 0b00111111))};
            break;
        case 4:
            top_bits &= (ptr[1] ^ 0b01000000) & (ptr[2] ^ 0b01000000) & (ptr[3] ^ 0b01000000);
            c = CodePoint{(static_cast<uint32_t>(ptr[0] & 0b00000111) << 18) |
                          (static_cast<uint32_t>(ptr[1] & 0b00111111) << 12) |
                          (static_cast<uint32_t>(ptr[2] & 0b00111111) << 6) |
                          (static_cast<uint32_t>(ptr[3] & 0b00111111))};
            break;
    }
    if (top_bits != 0b11000000) {
        // Check that the two most significant bits of all the code units after the first code point
        // are all [1, 0].
        n = 0;
        c = 0;
    }

    // Validate code point range. After decoding, each byte count utf8 character must fall within
    // the available range of code points.
    uint32_t v = c;
    if ((n == 1 && (v > 0x0000'007f)) ||                     //
        (n == 2 && (v < 0x0000'0080 || v > 0x0000'07ff)) ||  //
        (n == 3 && (v < 0x0000'0800 || v > 0x0000'ffff)) ||  //
        (n == 4 && (v < 0x0001'0000 || v > 0x0010'ffff))) {
        return {};
    }

    return {c, n};
}
TINT_END_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);

std::pair<CodePoint, size_t> Decode(std::string_view utf8_string) {
    return Decode(reinterpret_cast<const uint8_t*>(utf8_string.data()), utf8_string.size());
}

// This is a C-style API that will always trigger -Wunsafe-buffer-usage
TINT_BEGIN_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);
size_t Encode(CodePoint code_point, uint8_t* ptr) {
    if (code_point <= 0x7f) {
        if (ptr) {
            ptr[0] = static_cast<uint8_t>(code_point);
        }
        return 1;
    }
    if (code_point <= 0x7ff) {
        if (ptr) {
            ptr[0] = static_cast<uint8_t>(code_point >> 6) | 0b11000000;
            ptr[1] = static_cast<uint8_t>(code_point & 0b00111111) | 0b10000000;
        }
        return 2;
    }
    if (code_point <= 0xffff) {
        if (ptr) {
            ptr[0] = static_cast<uint8_t>(code_point >> 12) | 0b11100000;
            ptr[1] = static_cast<uint8_t>((code_point >> 6) & 0b00111111) | 0b10000000;
            ptr[2] = static_cast<uint8_t>(code_point & 0b00111111) | 0b10000000;
        }
        return 3;
    }
    if (code_point <= 0x10ffff) {
        if (ptr) {
            ptr[0] = static_cast<uint8_t>(code_point >> 18) | 0b11110000;
            ptr[1] = static_cast<uint8_t>((code_point >> 12) & 0b00111111) | 0b10000000;
            ptr[2] = static_cast<uint8_t>((code_point >> 6) & 0b00111111) | 0b10000000;
            ptr[3] = static_cast<uint8_t>(code_point & 0b00111111) | 0b10000000;
        }
        return 4;
    }
    return 0;  // invalid code point
}
TINT_END_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);

bool IsASCII(std::string_view str) {
    for (auto c : str) {
        if (c & 0x80) {
            return false;
        }
    }
    return true;
}

}  // namespace utf8

namespace utf16 {

// This is a C-style API that will always trigger -Wunsafe-buffer-usage
TINT_BEGIN_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);
std::pair<CodePoint, size_t> Decode(const uint16_t* ptr, size_t len) {
    if (len < 1) {
        return {};
    }
    uint16_t a = ptr[0];
    if (a <= 0xd7ff || a >= 0xe000) {
        return {CodePoint{static_cast<uint32_t>(a)}, 1};
    }
    if (len < 2) {
        return {};
    }
    uint32_t b = ptr[1];
    if (b <= 0xd7ff || b >= 0xe000) {
        return {};
    }
    uint32_t high = a - 0xd800;
    uint32_t low = b - 0xdc00;
    return {CodePoint{0x10000 + ((high << 10) | low)}, 2};
}
TINT_END_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);

std::pair<CodePoint, size_t> Decode(std::string_view utf16_string) {
    return Decode(reinterpret_cast<const uint16_t*>(utf16_string.data()), utf16_string.size() / 2);
}

// This is a C-style API that will always trigger -Wunsafe-buffer-usage
TINT_BEGIN_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);
size_t Encode(CodePoint code_point, uint16_t* ptr) {
    if (code_point <= 0xd7ff || (code_point >= 0xe000 && code_point <= 0xffff)) {
        if (ptr) {
            ptr[0] = static_cast<uint16_t>(code_point);
        }
        return 1;
    }
    if (code_point >= 0x10000 && code_point <= 0x10ffff) {
        if (ptr) {
            auto biased = code_point - 0x10000;
            ptr[0] = static_cast<uint16_t>((biased >> 10) + 0xd800);
            ptr[1] = static_cast<uint16_t>((biased & 0b1111111111) + 0xdc00);
        }
        return 2;
    }
    return 0;  // invalid code point
}
TINT_END_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);

}  // namespace utf16
}  // namespace tint
