// Copyright 2026 The Dawn & Tint Authors
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

#include "dawn/common/ExternalTextureParams.h"

#include "dawn/common/ColorSpace.h"

namespace dawn {

wgpu::Status ComputeExternalTextureParams(const wgpu::ColorSpaceDawn& srcColorSpace,
                                          wgpu::PredefinedColorSpace dstColorSpace,
                                          ExternalTextureColorSpaceParams* out) {
    if (srcColorSpace.nextInChain != nullptr) {
        return wgpu::Status::Error;
    }

    math::Mat4x3f rangeMatrix;
    switch (srcColorSpace.yCbCrRange) {
        case wgpu::ColorSpaceYCbCrRangeDawn::Narrow:
            rangeMatrix = kYCbCrRange_Narrow;
            break;
        case wgpu::ColorSpaceYCbCrRangeDawn::Full:
            rangeMatrix = kYCbCrRange_Full;
            break;
        default:
            return wgpu::Status::Error;
    }

    math::Mat3x3f ycbcrToRgb;
    switch (srcColorSpace.yCbCrMatrix) {
        case wgpu::ColorSpaceYCbCrMatrixDawn::Identity:
            ycbcrToRgb = math::Mat3x3f::Identity();
            break;
        case wgpu::ColorSpaceYCbCrMatrixDawn::Rec601:
            ycbcrToRgb = kYCbCrToRGB_Rec601;
            break;
        case wgpu::ColorSpaceYCbCrMatrixDawn::Rec709:
            ycbcrToRgb = kYCbCrToRGB_Rec709;
            break;
        case wgpu::ColorSpaceYCbCrMatrixDawn::Rec2020:
            ycbcrToRgb = kYCbCrToRGB_Rec2020;
            break;
        default:
            return wgpu::Status::Error;
    }

    math::Mat3x3f srcRGBToXYZ;
    switch (srcColorSpace.primaries) {
        case wgpu::ColorSpacePrimariesDawn::Rec601:
            srcRGBToXYZ = kRGBToXYZ_Rec601;
            break;
        case wgpu::ColorSpacePrimariesDawn::SRGB:
            srcRGBToXYZ = kRGBToXYZ_Rec709;
            break;
        case wgpu::ColorSpacePrimariesDawn::Rec2020:
            srcRGBToXYZ = kRGBToXYZ_Rec2020;
            break;
        case wgpu::ColorSpacePrimariesDawn::DisplayP3:
            srcRGBToXYZ = kRGBToXYZ_DisplayP3;
            break;
        default:
            return wgpu::Status::Error;
    }

    TransferFunction srcEOTF;
    switch (srcColorSpace.transfer) {
        case wgpu::ColorSpaceTransferDawn::SRGB:
            srcEOTF = kEOTF_sRGB;
            break;
        case wgpu::ColorSpaceTransferDawn::DisplayP3:
            srcEOTF = kEOTF_DisplayP3;
            break;
        default:
            return wgpu::Status::Error;
    }

    math::Mat3x3f dstXYZToRGB;
    TransferFunction dstOETF;
    switch (dstColorSpace) {
        case wgpu::PredefinedColorSpace::SRGB:
            dstXYZToRGB = kXYZToRGB_sRGB;
            dstOETF = kEOTFInverse_sRGB;
            break;
        case wgpu::PredefinedColorSpace::DisplayP3:
            dstXYZToRGB = kXYZToRGB_DisplayP3;
            dstOETF = kEOTFInverse_DisplayP3;
            break;
        case wgpu::PredefinedColorSpace::SRGBLinear:
            dstXYZToRGB = kXYZToRGB_sRGB;
            dstOETF = kEOTF_Linear;
            break;
        case wgpu::PredefinedColorSpace::DisplayP3Linear:
            dstXYZToRGB = kXYZToRGB_DisplayP3;
            dstOETF = kEOTF_Linear;
            break;
        default:
            return wgpu::Status::Error;
    }

    // The YCbCr to RGB matrix is passed transposed.
    math::Mat4x3f yuvToRgbConversionMatrix = math::Mul(ycbcrToRgb, rangeMatrix);
    for (size_t x = 0; x < 4; x++) {
        for (size_t y = 0; y < 3; y++) {
            out->yuvToRgbConversionMatrix[x + 4 * y] = yuvToRgbConversionMatrix[x][y];
        }
    }

    math::Mat3x3f gamutConversionMatrix = math::Mul(dstXYZToRGB, srcRGBToXYZ);
    for (size_t x = 0; x < 3; x++) {
        for (size_t y = 0; y < 3; y++) {
            out->gamutConversionMatrix[y + 3 * x] = gamutConversionMatrix[x][y];
        }
    }

    auto TranferFunctionToArray = [](const TransferFunction& tf) -> std::array<float, 7> {
        return {tf.g, tf.a, tf.b, tf.c, tf.d, tf.e, tf.f};
    };
    out->srcTransferFunction = TranferFunctionToArray(srcEOTF);
    out->dstTransferFunction = TranferFunctionToArray(dstOETF);

    return wgpu::Status::Success;
}

}  // namespace dawn
