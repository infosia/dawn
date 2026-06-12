# Copyright 2024 The Dawn & Tint Authors
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Common compiler warning config. This can be depended on directly, or via
# dawn_internal_config.
add_library(dawn_warnings_config INTERFACE)

# Config that enables -Weverything and then suppresses a lot of warnings.
# Tint always enables this on Clang, while Dawn only enables it if DAWN_WEVERYTHING is ON.
add_library(dawn_everything_config INTERFACE)

if (COMPILER_IS_CLANG)
  target_compile_options(dawn_everything_config
    INTERFACE
      "-Weverything"
      "-Wno-c++98-compat"
      "-Wno-c++98-compat-pedantic"
      "-Wno-covered-switch-default"
      "-Wno-cast-function-type-strict"
      "-Wno-c++20-extensions"
      "-Wno-c++20-compat"
      "-Wno-deprecated-redundant-constexpr-static-def"
      "-Wno-documentation"
      "-Wno-documentation-unknown-command"
      "-Wno-double-promotion"
      "-Wno-exit-time-destructors"
      "-Wno-extra-semi-stmt"
      "-Wno-float-conversion"
      "-Wno-float-equal"
      "-Wno-format-nonliteral"
      "-Wno-format-pedantic"
      "-Wno-global-constructors"
      "-Wno-implicit-float-conversion"
      "-Wno-implicit-int-conversion"
      "-Wno-lifetime-safety-intra-tu-suggestions"
      "-Wno-lifetime-safety-cross-tu-suggestions"
      "-Wno-lifetime-safety-invalidation"
      "-Wno-missing-noreturn"
      "-Wno-missing-prototypes"
      "-Wno-missing-variable-declarations"
      "-Wno-newline-eof"
      "-Wno-non-virtual-dtor"
      "-Wno-nullability-extension"
      "-Wno-objc-messaging-id"
      "-Wno-old-style-cast"
      "-Wno-padded"
      "-Wno-poison-system-directories"
      "-Wno-redundant-parens"
      "-Wno-reserved-identifier"
      "-Wno-return-std-move-in-c++11"
      "-Wno-shadow"
      "-Wno-shadow-field-in-constructor"
      "-Wno-shorten-64-to-32"
      "-Wno-sign-conversion"
      "-Wno-switch-default"
      "-Wno-switch-enum"
      "-Wno-tautological-type-limit-compare"
      "-Wno-thread-safety-negative"
      "-Wno-undefined-func-template"
      "-Wno-undefined-reinterpret-cast"
      "-Wno-undefined-var-template"
      "-Wno-unsafe-buffer-usage"
      "-Wno-unsafe-buffer-usage-in-libc-call"
      "-Wno-unused-macros"
      "-Wno-unused-member-function"
      "-Wno-unused-parameter"
      "-Wno-unused-template"
      "-Wno-used-but-marked-unused"
      "-Wno-weak-vtables"
      "-Wno-zero-as-null-pointer-constant"
      "-Wno-nrvo"
  )
endif()

if (DAWN_WERROR)
  if (COMPILER_IS_LIKE_GNU)
    target_compile_options(dawn_warnings_config
      INTERFACE
        "-Werror"
    )
  endif()
endif()

if (COMPILER_IS_LIKE_GNU)
  target_compile_options(dawn_warnings_config
    INTERFACE
      "-pedantic-errors"
      "-Wall"
      "-Wextra"
      "-Wno-c++20-compat"
  )
endif()

if (DAWN_WEVERYTHING)
  target_link_libraries(dawn_warnings_config INTERFACE dawn_everything_config)
endif()

# Flags that might not be supported in every version of a compiler.

# This requires CMake 3.19 features (for the `CheckCompilerFlag` module).
# Just skip it for older CMake versions and let the warnings appear.
if (CMAKE_VERSION VERSION_LESS "3.19")
  return ()
endif ()

include(CheckCompilerFlag)

function (dawn_add_flag flag)
  foreach (lang IN LISTS ARGN)
    check_compiler_flag("${lang}" "${flag}" "dawn_have_compiler_flag-${lang}-${flag}")
    if (dawn_have_compiler_flag-${lang}-${flag})
      target_compile_options(dawn_warnings_config
        INTERFACE
          "$<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:${lang}>:${flag}>>"
      )
    endif ()
  endforeach ()
endfunction ()

set(langs C CXX)

dawn_add_flag("-Wconditional-uninitialized" ${langs})
dawn_add_flag("-Wcstring-format-directive" ${langs})
dawn_add_flag("-Wctad-maybe-unsupported" ${langs})
dawn_add_flag("-Wc++11-narrowing" ${langs})
dawn_add_flag("-Wdeprecated-copy" ${langs})
dawn_add_flag("-Wdeprecated-copy-dtor" ${langs})
dawn_add_flag("-Wduplicate-enum" ${langs})
dawn_add_flag("-Wextra-semi" ${langs})
dawn_add_flag("-Wextra-semi-stmt" ${langs})
dawn_add_flag("-Wimplicit-fallthrough" ${langs})
dawn_add_flag("-Winconsistent-missing-destructor-override" ${langs})
dawn_add_flag("-Winvalid-offsetof" ${langs})
dawn_add_flag("-Wmissing-field-initializers" ${langs})
dawn_add_flag("-Wnon-c-typedef-for-linkage" ${langs})
dawn_add_flag("-Wpessimizing-move" ${langs})
dawn_add_flag("-Wrange-loop-analysis" ${langs})
dawn_add_flag("-Wredundant-move" ${langs})
dawn_add_flag("-Wshadow-field" ${langs})
dawn_add_flag("-Wstrict-prototypes" ${langs})
dawn_add_flag("-Wsuggest-destructor-override" ${langs})
dawn_add_flag("-Wsuggest-override" ${langs})
dawn_add_flag("-Wtautological-unsigned-zero-compare" ${langs})
dawn_add_flag("-Wunreachable-code-aggressive" ${langs})
dawn_add_flag("-Wunused-but-set-variable" ${langs})
dawn_add_flag("-Wunused-macros" ${langs})

if (COMPILER_IS_CLANG)
  # Flags use of __COUNTER__, which is used in histogram macros that were
  # largely copied from Chromium. Chromium continues to use __COUNTER__ in
  # their copy.
  dawn_add_flag("-Wno-c2y-extensions")
  if (CMAKE_CXX_COMPILER_VERSION_MAJOR VERSION_LESS_EQUAL 18)
    dawn_add_flag("-Wno-gcc-compat" ${langs})
  endif ()

  # Suppressions for Abseil headers
  dawn_add_flag("-Wno-gcc-compat" ${langs})
  dawn_add_flag("-Wno-unreachable-code-break" ${langs})
  dawn_add_flag("-Wno-nullability-extension" ${langs})
  dawn_add_flag("-Wno-shadow" ${langs})
  dawn_add_flag("-Wno-unknown-warning-option" ${langs})

  if (WIN32)
    dawn_add_flag("/clang:-pedantic" ${langs})
    # Allow the use of __uuidof()
    dawn_add_flag("-Wno-language-extension-token" ${langs})
  else ()
    dawn_add_flag("-pedantic" ${langs})
  endif ()
endif ()

if (MSVC)
  # Dawn extends wgpu enums with internal enums.
  # MSVC considers these invalid switch values. crbug.com/dawn/397.
  dawn_add_flag("/wd4063" CXX)
  # MSVC things that a switch over all the enum values of an enum class is
  # not sufficient to cover all control paths. Turn off this warning so that
  # the respective clang warning tells us where to add switch cases
  # (otherwise we have to add default: DAWN_UNREACHABLE() that silences clang too)
  dawn_add_flag("/wd4715" CXX)
  # /ZW makes sure we don't add calls that are forbidden in UWP.
  # and /EHsc is required to be used in combination with it,
  # even if it is already added by the windows GN defaults,
  # we still add it to make every /ZW paired with a /EHsc
  dawn_add_flag("/ZW:nostdlib" CXX)
  dawn_add_flag("/EHsc" CXX)
endif ()
