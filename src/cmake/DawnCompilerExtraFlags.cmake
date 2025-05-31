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

if (${DAWN_ENABLE_TSAN})
  add_compile_options(-stdlib=libc++)
  add_link_options(-stdlib=libc++)
endif ()

# Don't warn about C++20 only constructs, since Dawn requires C++20
if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR
    ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang") OR
    ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU"))
    add_compile_options(-Wno-c++20-compat)
endif()

################################################################################
# common_compile_options - sets compiler and linker options common for dawn
################################################################################
function(common_compile_options target)
  target_link_libraries(${target}
    INTERFACE
      dawn_compiler_flag_config
  )

  # Abseil headers can cause warnings when included that will cause failures
  # with -Werror.
  # Needs to be PUBLIC to propagate to targets that depend indirectly on abseil
  # via this target.
  if (NOT MSVC)
    get_target_property(deps ${target} LINK_LIBRARIES)
    if (deps MATCHES "absl")
      target_compile_options(${target} PUBLIC
              "-Wno-gcc-compat"
              "-Wno-unreachable-code-break"
              "-Wno-nullability-extension"
              "-Wno-shadow"
      )
    endif ()
  endif ()
endfunction()

