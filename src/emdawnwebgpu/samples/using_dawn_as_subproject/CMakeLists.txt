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

# Sample for how to include Dawn as a subproject in order to use emdawnwebgpu
# (webgpu.h bindings implementation for Emscripten-based projects).
#
# Example build steps:
# ```
# source path/to/emsdk/emsdk_env.sh
# mkdir build && cd build
# emcmake cmake ..  # optionally add -DSAMPLE_USE_JSPI=1
# make -j8
# ```
#
# This is a *top-level* CMake project file which includes Dawn as a subproject.
# It should not be included as a subproject of Dawn.

cmake_minimum_required(VERSION 3.13)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(emdawnwebgpu_cmake_sample)

if (NOT EMSCRIPTEN)
    message(FATAL_ERROR "This project must be build using `emcmake cmake`.")
endif()

#
# Set all global stuff
#

set(CMAKE_CXX_STANDARD 20)

option(SAMPLE_USE_JSPI "Use JSPI (ASYNCIFY=2) instead of ASYNCIFY=1.")

# Building with LTO is recommended for bindings performance and code size.
add_compile_options("$<$<CONFIG:Release>:-flto>")
add_link_options("$<$<CONFIG:Release>:-flto>")

add_compile_options(-Wall -Wextra)

if (SAMPLE_USE_JSPI)
    add_compile_definitions(SAMPLE_USE_JSPI=1)
endif()

#
# Include Dawn after setting any global settings
#

# In your project, point this toward your checkout of Dawn. If that is a
# subdirectory of your project, you won't need the second argument.
add_subdirectory("../../../../" "dawn_binary_dir/" EXCLUDE_FROM_ALL)

#
# Set up target
#

add_executable(main "../main.cpp")

# Depending on this will build all the necessary files in Dawn and
# set all the appropriate link flags to point at them.
target_link_libraries(main emdawnwebgpu_cpp)

set_target_properties(main PROPERTIES
    SUFFIX ".html")
target_link_options(main
    PRIVATE
        "-sEXIT_RUNTIME=0"
        "$<$<CONFIG:Debug>:-sASSERTIONS=1>"
        "$<$<CONFIG:Debug>:-Wno-limited-postlink-optimizations>"
        "$<$<CONFIG:Release>:--closure=1>"
        )
if (SAMPLE_USE_JSPI)
    target_link_options(main
        PRIVATE
            "-sJSPI=1"
            "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$getWasmTableEntry"
            )
else()
    target_link_options(main
        PRIVATE
            "-sASYNCIFY=1"
            "-sASYNCIFY_STACK_SIZE=65536"
            "-sEXPORTED_RUNTIME_METHODS=ccall"
            )
endif()
