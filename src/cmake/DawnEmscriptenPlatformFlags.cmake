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

if (${DAWN_ENABLE_EMSCRIPTEN})
    set(EM_SRC_DIR "${DAWN_SRC_DIR}/emdawnwebgpu")
    set(EM_BUILD_GEN_DIR "${DAWN_BUILD_GEN_DIR}/src/emdawnwebgpu")

    ################################################################################
    # emdawnwebgpu_compile_options - sets compiler and linker options for Emscripten
    ################################################################################
    function(emdawnwebgpu_compile_options target)
        add_dependencies(${target}
            emdawnwebgpu
            emdawnwebgpu_js_gen
            webgpu_generated_struct_info_js
        )
        include_directories(BEFORE
            "${EM_BUILD_GEN_DIR}/include"
        )
        set_target_properties(${target} PROPERTIES
            SUFFIX ".html"
        )
        target_link_options(${target}
            PRIVATE
            # We are using Dawn-generated bindings, not built-in ones
            "-sUSE_WEBGPU=0"
            "-sUSE_GLFW=3"
            # The JS libraries needed for bindings
            "--js-library=${EM_BUILD_GEN_DIR}/library_webgpu_enum_tables.js"
            "--js-library=${EM_BUILD_GEN_DIR}/library_webgpu_generated_struct_info.js"
            "--js-library=${EM_BUILD_GEN_DIR}/library_webgpu_generated_sig_info.js"
            "--js-library=${DAWN_EMDAWNWEBGPU_DIR}/library_webgpu.js"
            "--js-library=${DAWN_EMDAWNWEBGPU_DIR}/library_html5_webgpu.js"
            # Emscripten only enables webgpu-externs.js with -sUSE_WEBGPU=1, so add them explicitly.
            "--closure-args=--externs=${DAWN_EMSCRIPTEN_TOOLCHAIN}/src/closure-externs/webgpu-externs.js"
        )
    endfunction()

endif()