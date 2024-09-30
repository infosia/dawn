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
#include "src/tint/lang/glsl/writer/printer/printer.h"
#include "src/tint/lang/glsl/writer/raise/raise.h"

#include "src/tint/lang/wgsl/ast/transform/canonicalize_entry_point_io.h"
#include "src/tint/lang/wgsl/ast/transform/manager.h"
#include "src/tint/lang/wgsl/sem/module.h"

namespace tint::glsl::writer {

namespace {

void GetInputDeclarations(const Program& program, std::vector<InputDeclaration>* inputs) {
    for (const tint::ast::Variable* var : program.AST().GlobalVariables()) {
        const auto* sem = program.Sem().Get<tint::sem::GlobalVariable>(var);
        if (!sem || sem->AddressSpace() != tint::core::AddressSpace::kIn) {
            continue;
        }
        int location = -1;
        for (const tint::ast::Attribute* attr : var->attributes) {
            if (const auto* locAttr = attr->As<tint::ast::LocationAttribute>()) {
                if (const auto* expr = locAttr->expr->As<tint::ast::IntLiteralExpression>()) {
                    location = expr->value;
                }
            }
        }
        inputs->push_back({
            location,
            var->type->identifier->symbol.Name(),
            var->name->symbol.Name(),
        });
    }
}

// TODO need to do something similar for attributes?
void AddNonStructFields(const Program& program, const tint::ast::Struct* structDecl, int group, int binding, Vector<std::string, 4>& prefix, std::vector<UniformDeclaration>* uniforms) {
    for (const tint::ast::StructMember* member : structDecl->members) {
        const tint::Symbol& typeSymbol = member->type->identifier->symbol;
        const tint::Symbol& nameSymbol = member->name->symbol;
        prefix.Push(nameSymbol.Name());

        if (const tint::ast::TypeDecl* typeDecl = program.AST().LookupType(typeSymbol)) {
            if (const tint::ast::Struct* nestedStructDecl = typeDecl->As<tint::ast::Struct>()) {
                AddNonStructFields(program, nestedStructDecl, group, binding, prefix, uniforms);
                prefix.Pop();
                continue;
            }
        }

        uniforms->push_back({ group, binding, typeSymbol.Name(), Join(prefix, ".") });
        prefix.Pop();
    }
}

void GetUniformDeclarations(const Program& program, std::vector<UniformDeclaration>* uniforms) {
    for (const tint::ast::Variable* var : program.AST().GlobalVariables()) {
        if (!var->HasBindingPoint()) {
            continue;
        }
        const tint::ast::TypeDecl* typeDecl = program.AST().LookupType(var->type->identifier->symbol);
        if (!typeDecl) {
            continue;
        }
        const tint::ast::Struct* structDecl = typeDecl->As<tint::ast::Struct>();
        if (!structDecl) {
            continue;
        }

        int group = -1;
        int binding = -1;
        for (const tint::ast::Attribute* attr : var->attributes) {
            if (const auto* groupAttr = attr->As<tint::ast::GroupAttribute>()) {
                if (const auto* expr = groupAttr->expr->As<tint::ast::IntLiteralExpression>()) {
                    group = expr->value;
                }
            } else if (const auto* bindingAttr = attr->As<tint::ast::BindingAttribute>()) {
                if (const auto* expr = bindingAttr->expr->As<tint::ast::IntLiteralExpression>()) {
                    binding = expr->value;
                }
            }
        }
        TINT_ASSERT(group >= 0 && binding >= 0);

        // Recursively add non-struct fields to uniforms.
        Vector<std::string, 4> prefix{ var->name->symbol.Name() };
        AddNonStructFields(program, structDecl, group, binding, prefix, uniforms);
    }
}

}  // anonymous namespace

Result<Output> Generate(core::ir::Module& ir, const Options& options, const std::string&) {
    Output output;

    // Raise from core-dialect to GLSL-dialect.
    if (auto res = Raise(ir, options); res != Success) {
        return res.Failure();
    }

    // Generate the GLSL code.
    auto result = Print(ir, options.version);
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
