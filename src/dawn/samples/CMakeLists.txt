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

###############################################################################
# Dawn sample utilities
###############################################################################
set(sample_util_depends
    webgpu_cpp
    dawn_internal_config
    dawn::dawn_common
    dawn::dawn_system_utils
    dawn::dawn_wgpu_utils
)
if (NOT ${DAWN_ENABLE_EMSCRIPTEN})
    list(APPEND sample_util_depends
        dawn::dawn_test_utils
        dawn::dawn_proc
        dawn::dawn_native
    )
endif ()

if (NOT ANDROID AND NOT ${DAWN_ENABLE_EMSCRIPTEN})
    list(APPEND sample_util_depends
        dawn::dawn_glfw
        glfw
    )
endif ()

dawn_add_library(
    dawn_sample_utils
    FORCE_STATIC
    ENABLE_EMSCRIPTEN
    HEADERS
        "SampleUtils.h"
    SOURCES
        "SampleUtils.cpp"
    DEPENDS
        ${sample_util_depends}
)

if (ANDROID)
function(DawnAPK TARGET_NAME SOURCES)
    set(MANIFEST_DIR "${DAWN_BUILD_GEN_DIR}/src/dawn/samples/${TARGET_NAME}")
    set(STAGING_DIR "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}")
    set(STAGING_LIB_DIR "${STAGING_DIR}/lib/${CMAKE_ANDROID_ARCH_ABI}")
    set(MANIFEST_FILE "${MANIFEST_DIR}/AndroidManifest.xml")
    set(APK_FILE "${TARGET_NAME}.apk")
    set(MAIN_LIBRARY ${TARGET_NAME}Main)
    set(KEYSTORE "${Dawn_SOURCE_DIR}/scripts/dawn.keystore")

    add_library(${MAIN_LIBRARY} SHARED ${SOURCES} "AndroidMain.cpp")
    target_link_libraries(${MAIN_LIBRARY} PUBLIC dawn::dawn_sample_utils PRIVATE android_native_app_glue)
    common_compile_options(${MAIN_LIBRARY})

    add_custom_target(LinkTo${MAIN_LIBRARY}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${STAGING_LIB_DIR}
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_BINARY_DIR}/lib${MAIN_LIBRARY}.so" "${STAGING_LIB_DIR}/lib${MAIN_LIBRARY}.so"
        DEPENDS ${MAIN_LIBRARY}
    )

    add_custom_command(
        OUTPUT "${MANIFEST_FILE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory ${MANIFEST_DIR}
        COMMAND ${Python3_EXECUTABLE} "${Dawn_SOURCE_DIR}/generator/android_manifest_generator.py" --target-name ${TARGET_NAME} --out ${MANIFEST_FILE}
    )

    add_custom_command(
        OUTPUT "${APK_FILE}"
        COMMAND "${ANDROID_SDK}/build-tools/latest/aapt" package -f -M "${MANIFEST_FILE}" -I "${ANDROID_SDK}/platforms/android-26/android.jar" -F ${APK_FILE} "${STAGING_DIR}"
        COMMAND "${ANDROID_SDK}/build-tools/34.0.0/apksigner" sign --ks ${KEYSTORE} --ks-pass pass:webgpu "${APK_FILE}"
        DEPENDS ${MANIFEST_FILE} ${KEYSTORE} "LinkTo${MAIN_LIBRARY}"
    )

    add_custom_target(${TARGET_NAME} ALL
        DEPENDS "${APK_FILE}"
        VERBATIM
    )
endfunction()
endif()

###############################################################################
# Dawn samples
#   - Note that not all samples are currently buildable for Emscripten.
###############################################################################

function(Sample)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        ""
        "NAME;ENABLE_EMSCRIPTEN;SOURCES"
        ""
    )

    # Skip targets that shouldn't be built with Emscripten.
    if (NOT arg_ENABLE_EMSCRIPTEN AND ${DAWN_ENABLE_EMSCRIPTEN})
        return()
    endif ()

    if (ANDROID)
        DawnAPK(${arg_NAME} ${arg_SOURCES})
    else()
        add_executable(${arg_NAME} ${arg_SOURCES})
        if (${DAWN_ENABLE_EMSCRIPTEN})
            set_target_properties(${arg_NAME} PROPERTIES
                SUFFIX ".html")
        endif()
        target_link_libraries(${arg_NAME} PUBLIC dawn::dawn_sample_utils)
        common_compile_options(${arg_NAME})
    endif()
endfunction()

Sample(
    NAME HelloTriangle
    ENABLE_EMSCRIPTEN
    SOURCES "HelloTriangle.cpp"
)

Sample(
    NAME ComputeBoids
    ENABLE_EMSCRIPTEN
    SOURCES "ComputeBoids.cpp"
)

Sample(
    NAME Animometer
    ENABLE_EMSCRIPTEN
    SOURCES "Animometer.cpp"
)

Sample(
    NAME DawnInfo
    SOURCES "DawnInfo.cpp"
)

if (NOT ANDROID)
    Sample(
        NAME ManualSurfaceTest
        SOURCES "ManualSurfaceTest.cpp"
    )
endif()
