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

#ifndef INCLUDE_DAWN_REPLAY_JSONWALKER_H_
#define INCLUDE_DAWN_REPLAY_JSONWALKER_H_

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "dawn/replay/dawn_replay_export.h"
#include "src/dawn/replay/CaptureWalker.h"

namespace dawn::replay {

class ResourceVisitor;

class DAWN_REPLAY_EXPORT JSONWalker : public RootCommandVisitor {
  public:
    JSONWalker();
    ~JSONWalker() override;

    std::string GetJSON() const;

    // RootCommandVisitor
    void SetContentReadHead(ReadHead* readHead) override;
    ResourceVisitor& GetResourceVisitor() override;

    struct RootCommandInvalidCmdData {};

    template <typename T>
    struct RootCommandDataType {
        using Type = T;
    };

    template <>
    struct RootCommandDataType<schema::RootCommandCreateResourceCmdData> {
        using Type = CreateResourceData;
    };

    template <>
    struct RootCommandDataType<RootCommandInvalidCmdData> {
        using Type = RootCommandInvalidCmdData;
    };

#define DAWN_REPLAY_GET_X_MACRO(_1, _2, NAME, ...) NAME

#define DECLARE_ROOT_COMMAND_OPERATOR_INVALID(NAME, ...) \
    MaybeError operator()(const RootCommandDataType<RootCommandInvalidCmdData>::Type& data);
#define DECLARE_ROOT_COMMAND_OPERATOR_VALID(NAME, ...) \
    MaybeError operator()(                             \
        const RootCommandDataType<schema::RootCommand##NAME##CmdData>::Type& data) override;

#define DECLARE_ROOT_COMMAND_OPERATOR(...)                                      \
    DAWN_REPLAY_GET_X_MACRO(__VA_ARGS__, DECLARE_ROOT_COMMAND_OPERATOR_INVALID, \
                            DECLARE_ROOT_COMMAND_OPERATOR_VALID)                \
    (__VA_ARGS__)

    DAWN_REPLAY_ROOT_COMMANDS(DECLARE_ROOT_COMMAND_OPERATOR)
#undef DECLARE_ROOT_COMMAND_OPERATOR
#undef DECLARE_ROOT_COMMAND_OPERATOR_VALID
#undef DECLARE_ROOT_COMMAND_OPERATOR_INVALID
#undef DAWN_REPLAY_GET_X_MACRO

  public:
    class JSONWriter;

  private:
    std::unique_ptr<JSONWriter> mWriter;
};

}  // namespace dawn::replay

#endif  // INCLUDE_DAWN_REPLAY_JSONWALKER_H_
