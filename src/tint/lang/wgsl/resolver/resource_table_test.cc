// Copyright 2025 The Dawn & Tint Authors
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

#include "src/tint/lang/core/type/resource_table.h"
#include "src/tint/lang/wgsl/resolver/resolver.h"
#include "src/tint/lang/wgsl/resolver/resolver_helper_test.h"

#include "gmock/gmock.h"

using namespace tint::core::fluent_types;     // NOLINT
using namespace tint::core::number_suffixes;  // NOLINT

namespace tint::resolver {
namespace {

using ResolverResourceTableTest = ResolverTest;

TEST_F(ResolverResourceTableTest, ValidGlobalDecl) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    auto* var = GlobalVar("a", ty("resource_table"));

    EXPECT_TRUE(r()->Resolve()) << r()->error();

    auto* m = TypeOf(var)->UnwrapRef()->As<core::type::ResourceTable>();
    ASSERT_NE(m, nullptr);
}

TEST_F(ResolverResourceTableTest, MultipleSingleEntryPoint) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    auto* a = GlobalVar(Source{{12, 34}}, "a", ty("resource_table"));
    auto* b = GlobalVar(Source{{13, 5}}, "b", ty("resource_table"));

    Func("ep", tint::Empty, ty.void_(),
         Vector{
             Assign(Phony(), a),
             Assign(Phony(), b),
         },
         Vector{
             Stage(ast::PipelineStage::kCompute),
             WorkgroupSize(1_i),
         });

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(
        r()->error(),
        R"(13:5 error: entry point 'ep' references multiple variables that use the 'resource_binding' type
12:34 note: first 'resource_table' usage declared here)");
}

TEST_F(ResolverResourceTableTest, MultipleDifferentEntryPoints) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    auto* a = GlobalVar("a", ty("resource_table"));
    auto* b = GlobalVar("b", ty("resource_table"));

    Func("ep_1", tint::Empty, ty.void_(),
         Vector{
             Assign(Phony(), a),
         },
         Vector{
             Stage(ast::PipelineStage::kCompute),
             WorkgroupSize(1_i),
         });

    Func("ep_2", tint::Empty, ty.void_(),
         Vector{
             Assign(Phony(), b),
         },
         Vector{
             Stage(ast::PipelineStage::kCompute),
             WorkgroupSize(1_i),
         });

    EXPECT_TRUE(r()->Resolve()) << r()->error();

    ASSERT_NE(TypeOf(a)->UnwrapRef()->As<core::type::ResourceTable>(), nullptr);
    ASSERT_NE(TypeOf(b)->UnwrapRef()->As<core::type::ResourceTable>(), nullptr);
}

TEST_F(ResolverResourceTableTest, InvalidNoFeature) {
    GlobalVar("a", Binding(0_a), ty("resource_table"));

    Resolver resolver{this, wgsl::AllowedFeatures{}};
    EXPECT_FALSE(resolver.Resolve());
    EXPECT_EQ(
        resolver.error(),
        R"(error: use of a 'resource_table' requires enabling extension 'chromium_experimental_dynamic_binding')");
}

TEST_F(ResolverResourceTableTest, InvalidGlobalDeclNoGroup) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    GlobalVar("a", Binding(0_a), Group(0_a), ty("resource_table"));

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(error: 'resource_table' does not have '@group' and '@binding' attributes)");
}

TEST_F(ResolverResourceTableTest, InvalidGlobalDeclNoBinding) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    GlobalVar("a", Group(0_a), ty("resource_table"));

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(error: 'resource_table' does not have '@group' and '@binding' attributes)");
}

TEST_F(ResolverResourceTableTest, InvalidGlobalDeclPrivate) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    GlobalVar("a", private_, ty("resource_table"));

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(error: variables of type 'resource_table' must not specify an address space)");
}

TEST_F(ResolverResourceTableTest, InvalidGlobalDeclWorkgroup) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    GlobalVar("a", workgroup, ty("resource_table"));

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(error: variables of type 'resource_table' must not specify an address space)");
}

TEST_F(ResolverResourceTableTest, InvalidGlobalDeclStorageWithHandleType) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    GlobalVar("a", storage, ty("resource_table"));

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(
        r()->error(),
        R"(error: type 'resource_table' cannot be used in address space 'storage' as it is non-host-shareable
note: while instantiating 'var' a)");
}

TEST_F(ResolverResourceTableTest, InvalidGlobalDeclUniformWithHandleType) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    GlobalVar("a", uniform, ty("resource_table"));

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(
        r()->error(),
        R"(error: type 'resource_table' cannot be used in address space 'uniform' as it is non-host-shareable
note: while instantiating 'var' a)");
}

TEST_F(ResolverResourceTableTest, InvalidGlobalDeclOverride) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    Override("a", ty("resource_table"));

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), R"(error: resource_table cannot be used as the type of a 'override')");
}

TEST_F(ResolverResourceTableTest, InvalidFuncDecl) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    Func("foo", Empty, ty.void_(),
         Vector{
             Decl(Var("a", ty("resource_table"))),
         });

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), R"(error: function-scope 'var' must have a constructible type)");
}

TEST_F(ResolverResourceTableTest, InvalidStructMember) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    Structure("S", Vector{Member("a", ty("resource_table"))});

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(),
              R"(error: resource_table cannot be used as the type of a structure member)");
}

TEST_F(ResolverResourceTableTest, InvalidAsFunctionParameter) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    Func("foo", Vector{Param("a", ty("resource_table"))}, ty.void_(), Empty);

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), R"(error: type of function parameter cannot be resource_table)");
}

TEST_F(ResolverResourceTableTest, InvalidFunctionPointerParameterWithHandleType) {
    Enable(wgsl::Extension::kChromiumExperimentalDynamicBinding);
    Func("foo", Vector{Param("a", ty.ptr<function>(ty("resource_table")))}, ty.void_(), Empty);

    EXPECT_FALSE(r()->Resolve());
    EXPECT_EQ(r()->error(), R"(error: pointer can not be formed to handle type resource_table)");
}

}  // namespace
}  // namespace tint::resolver
