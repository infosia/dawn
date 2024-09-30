// Copyright 2021 The Dawn & Tint Authors
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

#include "src/tint/lang/glsl/writer/writer.h"

#include <memory>
#include <utility>

#include "src/tint/lang/glsl/writer/ast_printer/ast_printer.h"
#include "src/tint/lang/glsl/writer/raise/raise.h"

#include "src/tint/lang/core/type/reference.h"

namespace tint::glsl::writer {

namespace {

void GetInputDeclarations(const Program& program, std::vector<InputDeclaration>* inputs) {
    for (const tint::ast::Variable* var : program.AST().GlobalVariables()) {
        const tint::sem::GlobalVariable* sem = program.Sem().Get<tint::sem::GlobalVariable>(var);
        if (!sem || sem->AddressSpace() != tint::core::AddressSpace::kIn) {
            continue;
        }
        const std::optional<uint32_t>& location = sem->Attributes().location;
        if (!location) {
            continue;
        }

        const core::type::Type* type = sem->Type()->As<core::type::Reference>()->StoreType();
        inputs->push_back({
            static_cast<int>(*location),
            type->FriendlyName(),
            var->name->symbol.Name(),
        });
    }
}

void AddNonStructFields(const core::type::Type* type,
                        int group,
                        int binding,
                        Vector<std::string, 4>& prefix,
                        std::vector<UniformDeclaration>* uniforms) {
    if (const core::type::Struct* str = type->As<core::type::Struct>()) {
        for (const core::type::StructMember* member : str->Members()) {
            prefix.Push(member->Name().Name());
            AddNonStructFields(member->Type(), group, binding, prefix, uniforms);
            prefix.Pop();
        }

    } else {
        uniforms->push_back({ group, binding, type->FriendlyName(), Join(prefix, ".") });
    }
}

void GetUniformDeclarations(const Program& program, std::vector<UniformDeclaration>* uniforms) {
    for (const tint::ast::Variable* var : program.AST().GlobalVariables()) {
        const tint::sem::GlobalVariable* sem = program.Sem().Get<tint::sem::GlobalVariable>(var);
        if (!sem || sem->AddressSpace() != tint::core::AddressSpace::kUniform) {
            continue;
        }
        const std::optional<tint::BindingPoint>& bindingPoint = sem->Attributes().binding_point;
        if (!bindingPoint) {
            continue;
        }

        // Recursively add non-struct fields to uniforms.
        const core::type::Type* type = sem->Type()->As<core::type::Reference>()->StoreType();
        Vector<std::string, 4> prefix{ var->name->symbol.Name() };
        AddNonStructFields(type, bindingPoint->group, bindingPoint->binding, prefix, uniforms);
    }
}

}  // anonymous namespace

Result<Output> Generate(core::ir::Module& ir, const Options& options, const std::string&,
                        core::ir::Function::PipelineStage stage,
                        std::vector<InputDeclaration>* inputs,
                        std::vector<UniformDeclaration>* uniforms) {
    Output output;

    // Raise from core-dialect to GLSL-dialect.
    if (auto res = Raise(ir, options); res != Success) {
        return res.Failure();
    }

    // Generate the GLSL code.
    auto result = Print(ir, options.version, stage, inputs, uniforms);
    if (result != Success) {
        return result.Failure();
    }
    output.glsl = result.Get();

    return output;
}

Result<Output> Generate(const Program& program,
                        const Options& options,
                        const std::string& entry_point,
                        ast::PipelineStage stage,
                        std::vector<InputDeclaration> *inputs,
                        std::vector<UniformDeclaration> *uniforms) {
    if (!program.IsValid()) {
        return Failure{program.Diagnostics()};
    }

    Output output;

    // Sanitize the program.
    auto sanitized_result = Sanitize(program, options, entry_point);
    if (!sanitized_result.program.IsValid()) {
        return Failure{sanitized_result.program.Diagnostics()};
    }

    // Generate the GLSL code.
    auto impl = std::make_unique<ASTPrinter>(sanitized_result.program, options.version, stage);
    if (!impl->Generate()) {
        return Failure{impl->Diagnostics()};
    }

    if (inputs) {
        GetInputDeclarations(sanitized_result.program, inputs);
    }
    if (uniforms) {
        GetUniformDeclarations(sanitized_result.program, uniforms);
    }

    output.glsl = impl->Result();

    return output;
}

}  // namespace tint::glsl::writer
