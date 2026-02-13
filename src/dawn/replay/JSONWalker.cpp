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

#include "dawn/replay/JSONWalker.h"

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace dawn::replay {

namespace {
template <typename T>
struct is_container : std::false_type {};
template <typename T, size_t N>
struct is_container<std::array<T, N>> : std::true_type {};
template <typename T>
struct is_container<std::vector<T>> : std::true_type {};

const char* ObjectTypeToString(schema::ObjectType type) {
    switch (type) {
#define STRINGIFY_ENUM_CASE(NAME, ...) \
    case schema::ObjectType::NAME:     \
        return #NAME;
#define STRINGIFY_ENUM(NAME, MEMBERS) MEMBERS(STRINGIFY_ENUM_CASE)
        DAWN_REPLAY_OBJECT_TYPES_ENUM(STRINGIFY_ENUM)
#undef STRINGIFY_ENUM
#undef STRINGIFY_ENUM_CASE
    }
    return "Unknown";
}
}  // namespace

class JSONWalker::JSONWriter {
  public:
    explicit JSONWriter(bool pretty) : mPretty(pretty) {
        mFirst.push_back(true);
        mSS << "[";
    }

    std::string GetJSON() const {
        if (mPretty) {
            return mSS.str() + "\n]";
        }
        return mSS.str() + "]";
    }

    void BeginObject(const char* name = nullptr) {
        Comma();
        NewLine();
        Indent();
        if (name) {
            WriteJSONString(name);
            mSS << (mPretty ? ": {" : ":{");
        } else {
            mSS << "{";
        }
        mFirst.push_back(true);
    }

    void EndObject() {
        mFirst.pop_back();
        NewLine();
        Indent();
        mSS << "}";
    }

    void BeginArray(const char* name = nullptr) {
        Comma();
        NewLine();
        Indent();
        if (name) {
            WriteJSONString(name);
            mSS << (mPretty ? ": [" : ":[");
        } else {
            mSS << "[";
        }
        mFirst.push_back(true);
    }

    void EndArray() {
        mFirst.pop_back();
        NewLine();
        Indent();
        mSS << "]";
    }

    void BeginCommand(const char* name) {
        BeginObject();
        KeyValue("cmd", name);
        BeginObject("args");
    }

    void EndCommand() {
        EndObject();  // end args
        EndObject();  // end command object
    }

    void Comma() {
        if (!mFirst.back()) {
            mSS << ",";
        }
        mFirst.back() = false;
    }

    void NewLine() {
        if (mPretty) {
            mSS << "\n";
        }
    }

    void Indent() {
        if (mPretty) {
            for (size_t i = 0; i < mFirst.size(); ++i) {
                mSS << "  ";
            }
        }
    }

    void WriteJSONString(const std::string& value) {
        mSS << "\"";
        for (const char c : value) {
            switch (c) {
                case '"':
                    mSS << "\\\"";
                    break;
                case '\\':
                    mSS << "\\\\";
                    break;
                case '\b':
                    mSS << "\\b";
                    break;
                case '\f':
                    mSS << "\\f";
                    break;
                case '\n':
                    mSS << "\\n";
                    break;
                case '\r':
                    mSS << "\\r";
                    break;
                case '\t':
                    mSS << "\\t";
                    break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        static const char hex_digits[] = "0123456789abcdef";
                        mSS << "\\u00";
                        mSS.put(hex_digits[(c >> 4) & 0x0F]);
                        mSS.put(hex_digits[c & 0x0F]);
                    } else {
                        mSS.put(c);
                    }
            }
        }
        mSS << "\"";
    }

    void WriteJSONString(const char* value) { WriteJSONString(std::string(value)); }

    void KeyValue(const char* key, const char* value) {
        Comma();
        NewLine();
        Indent();
        if (key) {
            WriteJSONString(key);
            mSS << (mPretty ? ": " : ":");
        }
        WriteJSONString(value);
    }

    void KeyValue(const char* key, const std::string& value) {
        Comma();
        NewLine();
        Indent();
        if (key) {
            WriteJSONString(key);
            mSS << (mPretty ? ": " : ":");
        }
        WriteJSONString(value);
    }

    template <typename T>
    void Visit(const T& data) {
        if constexpr (requires { data.VisitAllWithNames([](auto&&...) {}); }) {
            data.VisitAllWithNames([&](auto&&... args) { this->EmitKeyValuePairs(args...); });
        }
    }

    void EmitKeyValuePairs() {}

    template <typename... Args>
    void EmitKeyValuePairs(const char* name, const auto& value, Args&&... args) {
        KeyValue(name, value);
        if constexpr (sizeof...(args) > 0) {
            EmitKeyValuePairs(std::forward<Args>(args)...);
        }
    }

    template <typename T>
    void KeyValue(const char* key, const T& value) {
        if constexpr (std::is_same_v<T, schema::ObjectType>) {
            KeyValue(key, ObjectTypeToString(value));
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            Comma();
            NewLine();
            Indent();
            if (key) {
                WriteJSONString(key);
                mSS << (mPretty ? ": " : ":");
            }
            mSS << "null";
        } else if constexpr (requires { value.VisitAllWithNames([](auto&&...) {}); }) {
            BeginObject(key);
            value.VisitAllWithNames([&](auto&&... args) { this->EmitKeyValuePairs(args...); });
            EndObject();
        } else if constexpr (is_container<T>::value) {
            BeginArray(key);
            for (const auto& item : value) {
                KeyValue(nullptr, item);
            }
            EndArray();
        } else if constexpr (std::is_enum_v<T>) {
            Comma();
            NewLine();
            Indent();
            if (key) {
                WriteJSONString(key);
                mSS << (mPretty ? ": " : ":");
            }
            mSS << static_cast<uint32_t>(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            KeyValue(key, value);
        } else {
            Comma();
            NewLine();
            Indent();
            if (key) {
                WriteJSONString(key);
                mSS << (mPretty ? ": " : ":");
            }
            mSS << value;
        }
    }

  private:
    bool mPretty;
    std::stringstream mSS;
    std::vector<bool> mFirst;
};

#define PASS_COMMAND_CASE_LOCAL(NAME)                                             \
    case schema::CommandBufferCommand::NAME: {                                    \
        schema::CommandBufferCommand##NAME##CmdData data;                         \
        DAWN_TRY(Deserialize(*readHead, &data));                                  \
        DAWN_TRY((*visitor)(data));                                               \
        if constexpr (std::is_same_v<schema::CommandBufferCommand##NAME##CmdData, \
                                     schema::CommandBufferCommandEndCmdData>) {   \
            return {};                                                            \
        }                                                                         \
        break;                                                                    \
    }

template <typename Visitor>
MaybeError JSONProcessComputePassCommands(ReadHead* readHead, Visitor* visitor) {
    while (!readHead->IsDone()) {
        schema::CommandBufferCommand cmd;
        DAWN_TRY(Deserialize(*readHead, &cmd));
        switch (cmd) {
            DAWN_REPLAY_COMPUTE_PASS_COMMANDS(PASS_COMMAND_CASE_LOCAL)
            default:
                return DAWN_INTERNAL_ERROR("unhandled ComputePass command");
        }
    }
    return DAWN_INTERNAL_ERROR("Missing ComputePass End command");
}

template <typename Visitor>
MaybeError JSONProcessRenderPassCommands(ReadHead* readHead, Visitor* visitor) {
    while (!readHead->IsDone()) {
        schema::CommandBufferCommand cmd;
        DAWN_TRY(Deserialize(*readHead, &cmd));
        switch (cmd) {
            DAWN_REPLAY_RENDER_PASS_COMMANDS(PASS_COMMAND_CASE_LOCAL)
            default:
                return DAWN_INTERNAL_ERROR("unhandled RenderPass command");
        }
    }
    return DAWN_INTERNAL_ERROR("Missing RenderPass End command");
}

template <typename Visitor>
MaybeError JSONProcessRenderBundleCommands(ReadHead* readHead, Visitor* visitor) {
    while (!readHead->IsDone()) {
        schema::CommandBufferCommand cmd;
        DAWN_TRY(Deserialize(*readHead, &cmd));
        switch (cmd) {
            DAWN_REPLAY_RENDER_BUNDLE_COMMANDS(PASS_COMMAND_CASE_LOCAL)
            default:
                return DAWN_INTERNAL_ERROR("unhandled RenderBundle command");
        }
    }
    return DAWN_INTERNAL_ERROR("Missing RenderBundle End command");
}

class JSONComputePassVisitor {
  public:
    explicit JSONComputePassVisitor(JSONWalker::JSONWriter* writer) : mWriter(writer) {}

#define VISITOR_METHOD(NAME)                                                         \
    MaybeError operator()(const schema::CommandBufferCommand##NAME##CmdData& data) { \
        mWriter->BeginCommand(#NAME);                                                \
        mWriter->Visit(data);                                                        \
        mWriter->EndCommand();                                                       \
        return {};                                                                   \
    }
    DAWN_REPLAY_COMPUTE_PASS_COMMANDS(VISITOR_METHOD)
#undef VISITOR_METHOD

  private:
    JSONWalker::JSONWriter* mWriter;
};

class JSONRenderPassVisitor {
  public:
    explicit JSONRenderPassVisitor(JSONWalker::JSONWriter* writer) : mWriter(writer) {}

#define VISITOR_METHOD(NAME)                                                         \
    MaybeError operator()(const schema::CommandBufferCommand##NAME##CmdData& data) { \
        mWriter->BeginCommand(#NAME);                                                \
        mWriter->Visit(data);                                                        \
        mWriter->EndCommand();                                                       \
        return {};                                                                   \
    }
    DAWN_REPLAY_RENDER_PASS_COMMANDS(VISITOR_METHOD)
#undef VISITOR_METHOD

  private:
    JSONWalker::JSONWriter* mWriter;
};

class JSONRenderBundleVisitor {
  public:
    explicit JSONRenderBundleVisitor(JSONWalker::JSONWriter* writer) : mWriter(writer) {}

#define VISITOR_METHOD(NAME)                                                         \
    MaybeError operator()(const schema::CommandBufferCommand##NAME##CmdData& data) { \
        mWriter->BeginCommand(#NAME);                                                \
        mWriter->Visit(data);                                                        \
        mWriter->EndCommand();                                                       \
        return {};                                                                   \
    }
    DAWN_REPLAY_RENDER_BUNDLE_COMMANDS(VISITOR_METHOD)
#undef VISITOR_METHOD

  private:
    JSONWalker::JSONWriter* mWriter;
};

class JSONEncoderVisitor {
  public:
    explicit JSONEncoderVisitor(JSONWalker::JSONWriter* writer) : mWriter(writer) {}

    ResultOrError<JSONComputePassVisitor*> BeginComputePass(
        const schema::CommandBufferCommandBeginComputePassCmdData& data) {
        mWriter->BeginCommand("BeginComputePass");
        mWriter->Visit(data);
        mWriter->BeginArray("commands");
        mComputePassVisitor = std::make_unique<JSONComputePassVisitor>(mWriter);
        return mComputePassVisitor.get();
    }

    ResultOrError<JSONRenderPassVisitor*> BeginRenderPass(
        const schema::CommandBufferCommandBeginRenderPassCmdData& data) {
        mWriter->BeginCommand("BeginRenderPass");
        mWriter->Visit(data);
        mWriter->BeginArray("commands");
        mRenderPassVisitor = std::make_unique<JSONRenderPassVisitor>(mWriter);
        return mRenderPassVisitor.get();
    }

    void EndPass() {
        mWriter->EndArray();
        mWriter->EndCommand();
    }

#define VISITOR_METHOD(NAME)                                                         \
    MaybeError operator()(const schema::CommandBufferCommand##NAME##CmdData& data) { \
        mWriter->BeginCommand(#NAME);                                                \
        mWriter->Visit(data);                                                        \
        mWriter->EndCommand();                                                       \
        return {};                                                                   \
    }
    DAWN_REPLAY_ENCODER_ONLY_COMMANDS(VISITOR_METHOD)
#undef VISITOR_METHOD

  private:
    JSONWalker::JSONWriter* mWriter;
    std::unique_ptr<JSONComputePassVisitor> mComputePassVisitor;
    std::unique_ptr<JSONRenderPassVisitor> mRenderPassVisitor;
};

template <typename Visitor>
MaybeError JSONProcessEncoderCommands(ReadHead* readHead, Visitor* visitor) {
    while (!readHead->IsDone()) {
        schema::CommandBufferCommand cmd;
        DAWN_TRY(Deserialize(*readHead, &cmd));

        switch (cmd) {
            case schema::CommandBufferCommand::BeginComputePass: {
                schema::CommandBufferCommandBeginComputePassCmdData data;
                DAWN_TRY(Deserialize(*readHead, &data));
                JSONComputePassVisitor* subVisitor;
                DAWN_TRY_ASSIGN(subVisitor, visitor->BeginComputePass(data));
                DAWN_TRY(JSONProcessComputePassCommands(readHead, subVisitor));
                visitor->EndPass();
                break;
            }
            case schema::CommandBufferCommand::BeginRenderPass: {
                schema::CommandBufferCommandBeginRenderPassCmdData data;
                DAWN_TRY(Deserialize(*readHead, &data));
                JSONRenderPassVisitor* subVisitor;
                DAWN_TRY_ASSIGN(subVisitor, visitor->BeginRenderPass(data));
                DAWN_TRY(JSONProcessRenderPassCommands(readHead, subVisitor));
                visitor->EndPass();
                break;
            }

                DAWN_REPLAY_DEBUG_COMMANDS(PASS_COMMAND_CASE_LOCAL)
                PASS_COMMAND_CASE_LOCAL(ClearBuffer)
                PASS_COMMAND_CASE_LOCAL(CopyBufferToBuffer)
                PASS_COMMAND_CASE_LOCAL(CopyBufferToTexture)
                PASS_COMMAND_CASE_LOCAL(CopyTextureToBuffer)
                PASS_COMMAND_CASE_LOCAL(CopyTextureToTexture)
                PASS_COMMAND_CASE_LOCAL(ResolveQuerySet)
                PASS_COMMAND_CASE_LOCAL(WriteBuffer)
                PASS_COMMAND_CASE_LOCAL(WriteTimestamp)
                PASS_COMMAND_CASE_LOCAL(End)

            default:
                return DAWN_INTERNAL_ERROR("unhandled encoder command");
        }
    }
    return DAWN_INTERNAL_ERROR("Missing encoder End command");
}

class JSONResourceVisitor {
  public:
    explicit JSONResourceVisitor(JSONWalker::JSONWriter* writer) : mWriter(writer) {}

#define VISIT_RESOURCE(NAME, TYPE) \
    MaybeError operator()(const TYPE& data) { return VisitResourceData(data); }
    DAWN_REPLAY_RESOURCE_DATA_MAP(VISIT_RESOURCE)
#undef VISIT_RESOURCE

    MaybeError operator()(const std::monostate&) {
        return DAWN_INTERNAL_ERROR("Invalid resource data (monostate)");
    }

  private:
    template <typename T>
    MaybeError VisitResourceData(const T& data) {
        mWriter->Visit(data);
        return {};
    }

    MaybeError VisitResourceData(const BindGroupData& data) {
        mWriter->Visit(data.bg);
        mWriter->BeginArray("entries");
        for (const auto& entry : data.entries) {
            std::visit(
                [&](auto&& e) {
                    if constexpr (!std::is_same_v<decltype(e), const std::monostate&>) {
                        mWriter->KeyValue(nullptr, e);
                    }
                },
                entry);
        }
        mWriter->EndArray();
        return {};
    }

    MaybeError VisitResourceData(const BindGroupLayoutData& data) {
        mWriter->Visit(data.bgl);
        mWriter->BeginArray("entries");
        for (const auto& entry : data.entries) {
            std::visit(
                [&](auto&& e) {
                    if constexpr (!std::is_same_v<decltype(e), const std::monostate&>) {
                        mWriter->KeyValue(nullptr, e);
                    }
                },
                entry);
        }
        mWriter->EndArray();
        return {};
    }

    MaybeError VisitResourceData(const CommandBufferData& data) {
        mWriter->BeginArray("commands");
        JSONEncoderVisitor encoderVisitor(mWriter);
        DAWN_TRY(JSONProcessEncoderCommands(data.readHead, &encoderVisitor));
        mWriter->EndArray();
        return {};
    }

    MaybeError VisitResourceData(const RenderBundleData& data) {
        mWriter->Visit(data.bundle);
        mWriter->BeginArray("commands");
        JSONRenderBundleVisitor renderBundleVisitor(mWriter);
        DAWN_TRY(JSONProcessRenderBundleCommands(data.readHead, &renderBundleVisitor));
        mWriter->EndArray();
        return {};
    }

    MaybeError VisitResourceData(const InvalidData& data) {
        return DAWN_INTERNAL_ERROR("Invalid resource data");
    }

    MaybeError VisitResourceData(const DeviceData& data) {
        return DAWN_INTERNAL_ERROR("Device data not expected here");
    }

    JSONWalker::JSONWriter* mWriter;
};

JSONWalker::JSONWalker(bool pretty) : mWriter(std::make_unique<JSONWriter>(pretty)) {}

JSONWalker::~JSONWalker() = default;

std::string JSONWalker::GetJSON() const {
    return mWriter->GetJSON();
}

void JSONWalker::SetContentReadHead(ReadHead* readHead) {}

ResourceVisitor& JSONWalker::GetResourceVisitor() {
    DAWN_UNREACHABLE();
}

// RootCommandVisitor implementations
namespace {
template <typename T>
MaybeError VisitRootCommandData(JSONWalker::JSONWriter* writer, const T& data) {
    writer->Visit(data);
    return {};
}

MaybeError VisitRootCommandData(JSONWalker::JSONWriter* writer, const CreateResourceData& data) {
    writer->Visit(data.resource);
    writer->BeginObject("args");
    JSONResourceVisitor visitor(writer);
    DAWN_TRY(std::visit(visitor, data.data));
    writer->EndObject();
    return {};
}

MaybeError VisitRootCommandData(JSONWalker::JSONWriter* writer,
                                const schema::RootCommandEndCmdData& data) {
    return {};
}
}  // namespace

#define DAWN_REPLAY_GET_X_MACRO(_1, _2, NAME, ...) NAME

#define VISIT_ROOT_COMMAND_INVALID(NAME, ...)                               \
    MaybeError JSONWalker::operator()(                                      \
        const RootCommandDataType<RootCommandInvalidCmdData>::Type& data) { \
        DAWN_UNREACHABLE();                                                 \
    }

#define VISIT_ROOT_COMMAND_VALID(NAME, ...)                                          \
    MaybeError JSONWalker::operator()(                                               \
        const RootCommandDataType<schema::RootCommand##NAME##CmdData>::Type& data) { \
        if constexpr (std::is_same_v<schema::RootCommand##NAME##CmdData,             \
                                     schema::RootCommandEndCmdData>) {               \
            return {};                                                               \
        }                                                                            \
        mWriter->BeginCommand(#NAME);                                                \
        DAWN_TRY(VisitRootCommandData(mWriter.get(), data));                         \
        mWriter->EndCommand();                                                       \
        return {};                                                                   \
    }

#define VISIT_ROOT_COMMAND(...)                                                                \
    DAWN_REPLAY_GET_X_MACRO(__VA_ARGS__, VISIT_ROOT_COMMAND_INVALID, VISIT_ROOT_COMMAND_VALID) \
    (__VA_ARGS__)

DAWN_REPLAY_ROOT_COMMANDS(VISIT_ROOT_COMMAND)
#undef VISIT_ROOT_COMMAND
#undef VISIT_ROOT_COMMAND_VALID
#undef VISIT_ROOT_COMMAND_INVALID
#undef DAWN_REPLAY_GET_X_MACRO

}  // namespace dawn::replay
