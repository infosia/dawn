# Copyright 2026 The Dawn & Tint Authors
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

set(MESA_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/install")
set(MESON_SCRIPT "${DAWN_THIRD_PARTY_DIR}/meson/src/meson.py")
set(MESA_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/build")

string(TOLOWER "${CMAKE_BUILD_TYPE}" build_type_lower)
if(build_type_lower STREQUAL "debug")
    set(MESON_BUILD_TYPE "debug")
else()
    set(MESON_BUILD_TYPE "release")
endif()

# Configure Mesa
add_custom_command(
    OUTPUT "${MESA_BUILD_DIR}/build.ninja"
    COMMAND ${Python3_EXECUTABLE} ${MESON_SCRIPT} setup
        "${MESA_BUILD_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "--prefix=${MESA_INSTALL_DIR}"
        "-Dgallium-drivers=llvmpipe"
        "-Dvulkan-drivers=swrast"
        "-Dbuildtype=${MESON_BUILD_TYPE}"
        "-Dlibdir=lib"
        "--reconfigure"
    COMMENT "Configuring Mesa"
)

add_custom_target(mesa_configured DEPENDS "${MESA_BUILD_DIR}/build.ninja")

# Build and Install Mesa
add_custom_target(mesa_base
    COMMAND ${Python3_EXECUTABLE} ${MESON_SCRIPT} install
        "-C" "${MESA_BUILD_DIR}"
    COMMENT "Building and installing Mesa"
)
add_dependencies(mesa_base mesa_configured)

# Interface library for GL
add_library(mesa_lib INTERFACE)
add_dependencies(mesa_lib mesa_base)
target_include_directories(mesa_lib INTERFACE "${MESA_INSTALL_DIR}/include")
target_link_directories(mesa_lib INTERFACE "${MESA_INSTALL_DIR}/lib")
target_link_libraries(mesa_lib INTERFACE EGL GL)
