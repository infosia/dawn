// Copyright 2024 The Dawn & Tint Authors
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

#include <iostream>
#include <string>
#include <vector>

#include "src/tint/cmd/fuzz/ir/fuzz.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/transform/single_entry_point.h"
#include "src/tint/lang/glsl/writer/helpers/generate_bindings.h"
#include "src/tint/lang/glsl/writer/printer/printer.h"
#include "src/tint/lang/glsl/writer/writer.h"

#if TINT_BUILD_FUZZER_OPENGL_SUPPORT
#include <dlfcn.h>
#endif

namespace tint::glsl::writer {
namespace {

// Fuzzed options used to init tint::glsl::writer::Options
struct FuzzedOptions {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // NOTE: These options should not be reordered or removed as it will change the operation of //
    // pre-existing fuzzer cases. Always append new options to the end of the list.              //
    ///////////////////////////////////////////////////////////////////////////////////////////////
    bool strip_all_names;
    bool disable_robustness;
    bool enable_integer_range_analysis;
    bool disable_workgroup_init;
    bool disable_polyfill_integer_div_mod;
    bool use_array_length_from_uniform;
    std::unordered_set<uint32_t> bgra_swizzle_locations;
    SubstituteOverridesConfig substitute_overrides_config;

    /// Reflect the fields of this class so that it can be used by tint::ForeachField()
    TINT_REFLECT(FuzzedOptions,
                 strip_all_names,
                 disable_robustness,
                 enable_integer_range_analysis,
                 disable_workgroup_init,
                 disable_polyfill_integer_div_mod,
                 use_array_length_from_uniform,
                 bgra_swizzle_locations,
                 substitute_overrides_config);
    TINT_REFLECT_HASH_CODE(FuzzedOptions);
};

#if TINT_BUILD_FUZZER_OPENGL_SUPPORT
Result<SuccessType> ValidateUsingOpenGL(const std::string& gl_driver, const std::string& glsl) {
    if (gl_driver.empty()) {
        return Success;
    }

    // EGL types
    using EGLDisplay = void*;
    using EGLContext = void*;
    using EGLConfig = void*;
    using EGLSurface = void*;
    using EGLint = int;
    using EGLBoolean = unsigned int;
    using EGLenum = unsigned int;

    // GLES types
    using GLuint = unsigned int;
    using GLenum = unsigned int;
    using GLint = int;
    using GLsizei = int;
    using GLchar = char;

    // EGL constants
    const EGLint EGL_DEFAULT_DISPLAY = 0;
    const EGLContext EGL_NO_CONTEXT = nullptr;
    const EGLDisplay EGL_NO_DISPLAY = nullptr;
    const EGLSurface EGL_NO_SURFACE = nullptr;
    const EGLint EGL_NONE = 0x3038;
    const EGLint EGL_SURFACE_TYPE = 0x3033;
    const EGLint EGL_PBUFFER_BIT = 0x0001;
    const EGLint EGL_RENDERABLE_TYPE = 0x3040;
    const EGLint EGL_OPENGL_ES3_BIT = 0x00000040;
    const EGLint EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    const EGLenum EGL_OPENGL_ES_API = 0x30A0;

    // GLES constants
    const GLenum GL_COMPUTE_SHADER = 0x91B9;
    const GLenum GL_COMPILE_STATUS = 0x8B81;
    const GLenum GL_INFO_LOG_LENGTH = 0x8B84;

    // EGL function pointers
    using PFN_eglGetDisplay = EGLDisplay (*)(EGLint);
    using PFN_eglInitialize = EGLBoolean (*)(EGLDisplay, EGLint*, EGLint*);
    using PFN_eglBindAPI = EGLBoolean (*)(EGLenum);
    using PFN_eglChooseConfig =
        EGLBoolean (*)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*);
    using PFN_eglCreateContext = EGLContext (*)(EGLDisplay, EGLConfig, EGLContext, const EGLint*);
    using PFN_eglMakeCurrent = EGLBoolean (*)(EGLDisplay, EGLSurface, EGLSurface, EGLContext);

    // GLES function pointers
    using PFN_glCreateShader = GLuint (*)(GLenum);
    using PFN_glShaderSource = void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
    using PFN_glCompileShader = void (*)(GLuint);
    using PFN_glGetShaderiv = void (*)(GLuint, GLenum, GLint*);
    using PFN_glGetShaderInfoLog = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
    using PFN_glDeleteShader = void (*)(GLuint);

    static void* lib = nullptr;
    static PFN_eglGetDisplay eglGetDisplay = nullptr;
    static PFN_eglInitialize eglInitialize = nullptr;
    static PFN_eglBindAPI eglBindAPI = nullptr;
    static PFN_eglChooseConfig eglChooseConfig = nullptr;
    static PFN_eglCreateContext eglCreateContext = nullptr;
    static PFN_eglMakeCurrent eglMakeCurrent = nullptr;
    static PFN_glCreateShader glCreateShader = nullptr;
    static PFN_glShaderSource glShaderSource = nullptr;
    static PFN_glCompileShader glCompileShader = nullptr;
    static PFN_glGetShaderiv glGetShaderiv = nullptr;
    static PFN_glGetShaderInfoLog glGetShaderInfoLog = nullptr;
    static PFN_glDeleteShader glDeleteShader = nullptr;

    if (!lib) {
        lib = dlopen(gl_driver.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (!lib) {
            return Failure{"Failed to load OpenGL driver: " + std::string(dlerror())};
        }

        auto load = [&](const char* name) {
            auto sym = dlsym(lib, name);
            if (!sym) {
                std::cerr << "Failed to load symbol: " << name << "\n";
            }
            return sym;
        };

        eglGetDisplay = reinterpret_cast<PFN_eglGetDisplay>(load("eglGetDisplay"));
        eglInitialize = reinterpret_cast<PFN_eglInitialize>(load("eglInitialize"));
        eglBindAPI = reinterpret_cast<PFN_eglBindAPI>(load("eglBindAPI"));
        eglChooseConfig = reinterpret_cast<PFN_eglChooseConfig>(load("eglChooseConfig"));
        eglCreateContext = reinterpret_cast<PFN_eglCreateContext>(load("eglCreateContext"));
        eglMakeCurrent = reinterpret_cast<PFN_eglMakeCurrent>(load("eglMakeCurrent"));
        glCreateShader = reinterpret_cast<PFN_glCreateShader>(load("glCreateShader"));
        glShaderSource = reinterpret_cast<PFN_glShaderSource>(load("glShaderSource"));
        glCompileShader = reinterpret_cast<PFN_glCompileShader>(load("glCompileShader"));
        glGetShaderiv = reinterpret_cast<PFN_glGetShaderiv>(load("glGetShaderiv"));
        glGetShaderInfoLog = reinterpret_cast<PFN_glGetShaderInfoLog>(load("glGetShaderInfoLog"));
        glDeleteShader = reinterpret_cast<PFN_glDeleteShader>(load("glDeleteShader"));

        if (!eglGetDisplay || !eglInitialize || !eglBindAPI || !eglChooseConfig ||
            !eglCreateContext || !eglMakeCurrent || !glCreateShader || !glShaderSource ||
            !glCompileShader || !glGetShaderiv || !glGetShaderInfoLog || !glDeleteShader) {
            return Failure{"Failed to load required OpenGL symbols"};
        }
    }

    thread_local EGLDisplay display = EGL_NO_DISPLAY;
    thread_local EGLContext context = EGL_NO_CONTEXT;

    if (display == EGL_NO_DISPLAY) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            return Failure{"eglGetDisplay failed"};
        }
        EGLint major, minor;
        if (!eglInitialize(display, &major, &minor)) {
            return Failure{"eglInitialize failed"};
        }
        eglBindAPI(EGL_OPENGL_ES_API);
        EGLint config_attribs[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE,
                                   EGL_OPENGL_ES3_BIT, EGL_NONE};
        EGLConfig config;
        EGLint num_configs;
        if (!eglChooseConfig(display, config_attribs, &config, 1, &num_configs)) {
            return Failure{"eglChooseConfig failed"};
        }
        EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attribs);
        if (context == EGL_NO_CONTEXT) {
            return Failure{"eglCreateContext failed"};
        }
        if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) {
            return Failure{"eglMakeCurrent failed"};
        }
    }

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    if (shader == 0) {
        return Failure{"glCreateShader failed"};
    }

    const char* source = glsl.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        std::vector<char> log(log_length);
        glGetShaderInfoLog(shader, log_length, nullptr, log.data());
        glDeleteShader(shader);
        return Failure{std::string(log.data())};
    }

    glDeleteShader(shader);
    return Success;
}
#endif

Result<SuccessType> IRFuzzer(core::ir::Module& module,
                             const fuzz::ir::Context& context,
                             FuzzedOptions fuzzed_options) {
    // TODO(375388101): We cannot run the backend for every entry point in the module unless we
    // clone the whole module each time, so for now we just generate the first entry point.

    // Strip the module down to a single entry point.
    core::ir::Function* entry_point = nullptr;
    for (auto& func : module.functions) {
        if (func->IsEntryPoint()) {
            entry_point = func;
            break;
        }
    }
    std::string ep_name;
    if (entry_point) {
        ep_name = module.NameOf(entry_point).NameView();
    }
    if (ep_name.empty()) {
        // No entry point, just return success
        return Success;
    }

    // We fuzz options that Dawn will vary depending on the platform and provided toggles.
    // Options that are entirely controlled by Dawn (e.g. binding points) are not fuzzed.
    Options options;
    options.strip_all_names = fuzzed_options.strip_all_names;
    options.disable_robustness = fuzzed_options.disable_robustness;
    options.disable_integer_range_analysis = !fuzzed_options.enable_integer_range_analysis;
    options.disable_workgroup_init = fuzzed_options.disable_workgroup_init;
    options.disable_polyfill_integer_div_mod = fuzzed_options.disable_polyfill_integer_div_mod;
    options.use_array_length_from_uniform = fuzzed_options.use_array_length_from_uniform;
    options.entry_point_name = ep_name;
    options.bgra_swizzle_locations = fuzzed_options.bgra_swizzle_locations;
    options.substitute_overrides_config = fuzzed_options.substitute_overrides_config;

    options.version = Version(Version::Standard::kES, 3, 1);

    auto data = GenerateBindings(module, ep_name);
    options.bindings = std::move(data.bindings);
    options.texture_builtins_from_uniform = std::move(data.texture_builtins_from_uniform);

    // Leave some room for user-declared immediate data.
    uint32_t next_immediate_offset = 0x800;
    auto builtin_immediate = [&next_immediate_offset] {
        auto offset = next_immediate_offset;
        next_immediate_offset += 4;
        return offset;
    };

    // Set offsets for immediate data used for certain builtins.
    for (auto& func : module.functions) {
        if (!func->IsEntryPoint()) {
            continue;
        }

        // vertex_index and instance_index use immediate data for offsets if used.
        for (auto* param : func->Params()) {
            if (auto* str = param->Type()->As<core::type::Struct>()) {
                for (auto* member : str->Members()) {
                    if (member->Attributes().builtin == core::BuiltinValue::kVertexIndex) {
                        options.first_vertex_offset = builtin_immediate();
                    } else if (member->Attributes().builtin == core::BuiltinValue::kInstanceIndex) {
                        options.first_vertex_offset = builtin_immediate();
                    }
                }
            } else {
                if (param->Builtin() == core::BuiltinValue::kVertexIndex) {
                    options.first_vertex_offset = builtin_immediate();
                } else if (param->Builtin() == core::BuiltinValue::kInstanceIndex) {
                    options.first_vertex_offset = builtin_immediate();
                }
            }
        }

        // frag_depth uses immediate data for min and max clamp values if used.
        if (auto* str = func->ReturnType()->As<core::type::Struct>()) {
            for (auto* member : str->Members()) {
                if (member->Attributes().builtin == core::BuiltinValue::kFragDepth) {
                    options.depth_range_offsets = {
                        builtin_immediate(),
                        builtin_immediate(),
                    };
                }
            }
        } else {
            if (func->ReturnBuiltin() == core::BuiltinValue::kFragDepth) {
                options.depth_range_offsets = {
                    builtin_immediate(),
                    builtin_immediate(),
                };
            }
        }
    }

    TINT_CHECK_RESULT(CanGenerate(module, options));

    TINT_CHECK_RESULT_UNWRAP(output, Generate(module, options));
    if (context.options.dump) {
        std::cout << "Dumping generated GLSL:\n" << output.glsl << "\n";
    }

#if TINT_BUILD_FUZZER_OPENGL_SUPPORT
    TINT_CHECK_RESULT(ValidateUsingOpenGL(context.options.gl_driver, output.glsl));
#endif

    return Success;
}

}  // namespace
}  // namespace tint::glsl::writer

TINT_IR_MODULE_FUZZER(tint::glsl::writer::IRFuzzer,
                      tint::core::ir::Capabilities{},
                      tint::glsl::writer::kPrinterCapabilities);
