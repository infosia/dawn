// Copyright 2020 The Dawn & Tint Authors
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

#include "src/tint/lang/spirv/writer/writer.h"

#include <memory>
#include <utility>

#include "src/tint/lang/spirv/writer/common/option_helpers.h"
#include "src/tint/lang/spirv/writer/printer/printer.h"
#include "src/tint/lang/spirv/writer/raise/raise.h"
#include "src/utils/trace_event.h"

// Included by 'ast_printer.h', included again here for './tools/run gen' track the dependency.
#include "spirv/unified1/spirv.h"

namespace tint::spirv::writer {

Result<Output> Generate(core::ir::Module& ir,
                        const Options& options,
                        dawn::utils::TracePlatform* platform) {
    TRACE_EVENT_BEGIN0(platform, Tint, "Generate SPIR-V");

    {
        TRACE_EVENT_BEGIN0(platform, Tint, "Validate");
        auto res = ValidateBindingOptions(options);
        TRACE_EVENT_END0(platform, Tint, "Validate");
        if (res != Success) {
            return res.Failure();
        }
    }

    Output output;

    // Raise from core-dialect to SPIR-V-dialect.
    if (auto res = Raise(ir, options, platform); res != Success) {
        return std::move(res.Failure());
    }

    // Generate the SPIR-V code.
    TRACE_EVENT_BEGIN0(platform, Tint, "Print");
    auto spirv = Print(ir, options);
    TRACE_EVENT_END0(platform, Tint, "Print");
    if (spirv != Success) {
        return std::move(spirv.Failure());
    }
    output.spirv = std::move(spirv.Get());

    TRACE_EVENT_END0(platform, Tint, "Generate SPIR-V");

    return output;
}

}  // namespace tint::spirv::writer
