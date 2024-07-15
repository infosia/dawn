# Copyright 2020 The Dawn & Tint Authors
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

include(DawnEmscriptenPlatformFlags)

add_library(dawn_sample_utils STATIC)
if (${DAWN_ENABLE_EMSCRIPTEN})
    target_link_libraries(dawn_sample_utils PUBLIC
        webgpu_cpp
        dawn_internal_config
        dawn_common
        dawn_wgpu_utils
    )
else()
    common_compile_options(dawn_sample_utils)
    target_link_libraries(dawn_sample_utils PUBLIC
        webgpu_cpp
        dawn_internal_config
        dawn_proc
        dawn_common
        dawn_glfw
        dawn_native
        dawn_utils
        glfw
    )
endif()
target_sources(dawn_sample_utils PRIVATE
    "SampleUtils.cpp"
    "SampleUtils.h"
)

add_executable(HelloTriangle "HelloTriangle.cpp")
if (${DAWN_ENABLE_EMSCRIPTEN})
    set_target_properties(HelloTriangle PROPERTIES
        SUFFIX ".html")
else()
    common_compile_options(HelloTriangle)
endif()
target_link_libraries(HelloTriangle dawn_sample_utils)

add_executable(ComputeBoids "ComputeBoids.cpp")
if (${DAWN_ENABLE_EMSCRIPTEN})
    set_target_properties(ComputeBoids PROPERTIES
        SUFFIX ".html")
else()
    common_compile_options(ComputeBoids)
endif()
target_link_libraries(ComputeBoids dawn_sample_utils)

add_executable(Animometer "Animometer.cpp")
if (${DAWN_ENABLE_EMSCRIPTEN})
    set_target_properties(Animometer PROPERTIES
        SUFFIX ".html")
else()
    common_compile_options(Animometer)
endif()
target_link_libraries(Animometer dawn_sample_utils)

add_executable(DawnInfo "DawnInfo.cpp")
common_compile_options(DawnInfo)
target_link_libraries(DawnInfo dawn_sample_utils)

add_executable(ManualSurfaceTest "ManualSurfaceTest.cpp")
common_compile_options(ManualSurfaceTest)
target_link_libraries(ManualSurfaceTest dawn_sample_utils)
