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

set(EM_BUILD_GEN_DIR "${DAWN_BUILD_GEN_DIR}/src/emdawnwebgpu")

DawnJSONGenerator(
    TARGET "emdawnwebgpu_headers"
    PRINT_NAME "Dawn WebGPU Emscripten headers"
    OUTPUT_HEADERS EMDAWNWEBGPU_HEADERS_GEN_HEADERS
)
add_custom_target(emdawnwebgpu_headers_gen
    DEPENDS
        ${EMDAWNWEBGPU_HEADERS_GEN_HEADERS}
        # Also include the copied headers in this target.
        ${EMDAWNWEBGPU_HEADERS_COPIED_HEADERS}
)

DawnJSONGenerator(
    TARGET "emdawnwebgpu_js"
    PRINT_NAME "Dawn WebGPU Emscripten JS files"
    OUTPUT_JSONS EMDAWNWEBGPU_STRUCT_INFO_JSON_GEN_SOURCES
    OUTPUT_JS EMDAWNWEBGPU_JS_GEN_SOURCES
)
add_custom_target(emdawnwebgpu_js_gen
    DEPENDS ${EMDAWNWEBGPU_STRUCT_INFO_JSON_GEN_SOURCES} ${EMDAWNWEBGPU_JS_GEN_SOURCES}
)

# When Emscripten is available, we can use one of its helper scripts to generate
# the struct info needed for our bindings fork (third_party/emdawnwebgpu).
# Those helpers, and their tree of generated dependencies, are:
#
# - library_webgpu_generated_struct_info.js
#   is constructed by concatenating:
#     - Some glue "snippets" from txt files
#     - webgpu_generated_struct_info{32,64}.json
#       which are generated using an Emscripten tool "gen_struct_info.py", from:
#         - webgpu.h (generated from dawn.json)
#         - struct_info_webgpu.json (generated from dawn.json)
#
# The bindings also require the following helpers (generated above):
#
# - library_webgpu_enum_tables.js
# - library_webgpu_generated_sig_info.js
#   which we generate directly instead of using "gen_sig_info.py"
if (EMSCRIPTEN)
    set(EM_SRC_DIR "${DAWN_SRC_DIR}/emdawnwebgpu")

    function(webgpu_gen_struct_info)
        cmake_parse_arguments(PARSE_ARGV 0 arg
            ""
            "OUTPUT_JSON;WASM64"
            ""
        )
        if (arg_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR
                "Unparsed arguments for webgpu_gen_struct_info: "
                "${arg_UNPARSED_ARGUMENTS}")
        endif()

        # Note this requires Emscripten 4.0.3+ (prior to that it was located
        # at tools/maint/gen_struct_info.py).
        set(ARGS
            ${Python3_EXECUTABLE}
            "${EMSCRIPTEN_ROOT_PATH}/tools/gen_struct_info.py"
            -q
            "${EM_BUILD_GEN_DIR}/struct_info_webgpu.json"
            "-I=${EM_BUILD_GEN_DIR}/include"
        )

        set(PRINT_NAME)
        if (${arg_WASM64})
            set(PRINT_NAME "webgpu_generated_struct_info64.json")
            list(APPEND ARGS "--wasm64")
        else()
            set(PRINT_NAME "webgpu_generated_struct_info32.json")
        endif()
        set(OUTPUT "${EM_BUILD_GEN_DIR}/${PRINT_NAME}")
        set(${arg_OUTPUT_JSON} "${OUTPUT}" PARENT_SCOPE)
        list(APPEND ARGS "-o=${OUTPUT}")

        add_custom_command(
            COMMAND ${ARGS}
            OUTPUT ${OUTPUT}
            COMMENT "Dawn Emscripten: Generating ${PRINT_NAME}."
            DEPENDS
                "${EMSCRIPTEN_ROOT_PATH}/tools/gen_struct_info.py"
                ${EMDAWNWEBGPU_HEADERS_GEN_HEADERS}  # for webgpu.h
                ${EMDAWNWEBGPU_STRUCT_INFO_JSON_GEN_SOURCES}  # for struct_info_webgpu.json
        )
    endfunction()

    message(STATUS "Dawn: Configuring Emscripten gen_struct_info for WebGPU.")

    webgpu_gen_struct_info(
        OUTPUT_JSON WEBGPU_STRUCT_INFO_GEN32
        WASM64 OFF
    )
    webgpu_gen_struct_info(
        OUTPUT_JSON WEBGPU_STRUCT_INFO_GEN64
        WASM64 ON
    )

    # TODO(crbug.com/346806934): Consider using CMake builtin `cat` as per https://stackoverflow.com/a/62362885,
    # especially if we are to remove the GN build where concat.py is needed.
    function(webgpu_gen_struct_info_js)
        cmake_parse_arguments(PARSE_ARGV 0 arg
        ""
        "OUTPUT_JS"
        ""
        )
        if (arg_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR
                "Unparsed arguments for webgpu_gen_struct_info_js: "
                "${arg_UNPARSED_ARGUMENTS}")
        endif()

        set(PRINT_NAME library_webgpu_generated_struct_info.js)
        set(OUTPUT "${EM_BUILD_GEN_DIR}/${PRINT_NAME}")
        set(SRCS
            "${EM_SRC_DIR}/snippets/library_webgpu_struct_info_part1.txt"
            "${WEBGPU_STRUCT_INFO_GEN32}"
            "${EM_SRC_DIR}/snippets/library_webgpu_struct_info_part2.txt"
            "${WEBGPU_STRUCT_INFO_GEN64}"
            "${EM_SRC_DIR}/snippets/library_webgpu_struct_info_part3.txt"
        )
        set(ARGS
            ${Python3_EXECUTABLE}
            "${EM_SRC_DIR}/concat.py"
            "${OUTPUT}"
            ${SRCS}
        )
        set(${arg_OUTPUT_JS} "${OUTPUT}" PARENT_SCOPE)

        add_custom_command(
            COMMAND ${ARGS}
            OUTPUT ${OUTPUT}
            COMMENT "Dawn Emscripten: Generating ${PRINT_NAME}."
            DEPENDS
                "${EM_SRC_DIR}/concat.py"
                ${SRCS}
        )
    endfunction()

    webgpu_gen_struct_info_js(
        OUTPUT_JS EMDAWNWEBGPU_STRUCT_INFO_JS
    )

    #
    # emdawnwebgpu_pkg
    # This constructs the self-contained package. Other targets will use the packaged files.
    #

    set(EMDAWNWEBGPU_PKG "${CMAKE_BINARY_DIR}/emdawnwebgpu_pkg")

    # Individually list every file in the package so that dependencies can be set correctly.
    set(EMDAWNWEBGPU_PKG_DEPENDS "")
    set(EMDAWNWEBGPU_PKG_BYPRODUCTS "")
    set(EMDAWNWEBGPU_PKG_COMMANDS "")
    macro(pkg_add_dir dstsubdir)
        list(PREPEND EMDAWNWEBGPU_PKG_BYPRODUCTS
            "${EMDAWNWEBGPU_PKG}/${dstsubdir}"
        )
        list(APPEND EMDAWNWEBGPU_PKG_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "${EMDAWNWEBGPU_PKG}/${dstsubdir}"
        )
    endmacro()
    macro(pkg_add srcdir dstsubdir filename)
        list(APPEND EMDAWNWEBGPU_PKG_DEPENDS
            "${srcdir}/${filename}"
        )
        list(PREPEND EMDAWNWEBGPU_PKG_BYPRODUCTS
            "${EMDAWNWEBGPU_PKG}/${dstsubdir}/${filename}"
        )
        list(APPEND EMDAWNWEBGPU_PKG_COMMANDS
            COMMAND ${CMAKE_COMMAND} -E copy
                "${srcdir}/${filename}"
                "${EMDAWNWEBGPU_PKG}/${dstsubdir}/${filename}"
        )
        set_source_files_properties(
            "${EMDAWNWEBGPU_PKG}/${dstsubdir}/${filename}"
            PROPERTIES GENERATED TRUE
        )
    endmacro()
    pkg_add_dir("webgpu")
    pkg_add_dir("webgpu/src")
    pkg_add_dir("webgpu/include")
    pkg_add_dir("webgpu/include/webgpu")
    pkg_add_dir("webgpu_cpp")
    pkg_add_dir("webgpu_cpp/include")
    pkg_add_dir("webgpu_cpp/include/dawn")
    pkg_add_dir("webgpu_cpp/include/webgpu")
    pkg_add("${EM_SRC_DIR}/pkg" "" "README.md")
    pkg_add("${EM_SRC_DIR}/pkg" "" "emdawnwebgpu.port.py")
    pkg_add("${EM_SRC_DIR}/pkg" "" "webgpu_cpp/LICENSE")
    pkg_add("${DAWN_EMDAWNWEBGPU_DIR}/pkg/webgpu/src" "webgpu/src" "LICENSE")
    pkg_add("${DAWN_EMDAWNWEBGPU_DIR}/pkg/webgpu/src" "webgpu/src" "library_webgpu.js")
    pkg_add("${DAWN_EMDAWNWEBGPU_DIR}/pkg/webgpu/src" "webgpu/src" "webgpu-externs.js")
    pkg_add("${DAWN_EMDAWNWEBGPU_DIR}/pkg/webgpu/src" "webgpu/src" "webgpu.cpp")
    pkg_add("${DAWN_INCLUDE_DIR}/webgpu" "webgpu/include/webgpu" "webgpu_glfw.h")
    pkg_add("${DAWN_INCLUDE_DIR}/webgpu" "webgpu_cpp/include/webgpu" "webgpu_enum_class_bitmasks.h")
    pkg_add("${EM_BUILD_GEN_DIR}/include/webgpu" "webgpu/include/webgpu" "webgpu.h")
    pkg_add("${EM_BUILD_GEN_DIR}" "webgpu/src" "library_webgpu_enum_tables.js")
    pkg_add("${EM_BUILD_GEN_DIR}" "webgpu/src" "library_webgpu_generated_struct_info.js")
    pkg_add("${EM_BUILD_GEN_DIR}" "webgpu/src" "library_webgpu_generated_sig_info.js")
    pkg_add("${EM_BUILD_GEN_DIR}/include/webgpu" "webgpu_cpp/include/webgpu" "webgpu_cpp.h")
    pkg_add("${EM_BUILD_GEN_DIR}/include/webgpu" "webgpu_cpp/include/webgpu" "webgpu_cpp_chained_struct.h")
    pkg_add("${EM_BUILD_GEN_DIR}/include/dawn" "webgpu_cpp/include/dawn" "webgpu_cpp_print.h")

    # Define a custom command that runs when ANY of the files in the package are used as a
    # dependency. There isn't an actual "target" that needs to be included to use them.
    #
    # The primary output is a stamp indicating when the emdawnwebgpu_pkg directory was regenerated.
    # (This is a separate file, rather than the emdawnwebgpu_pkg directory itself, so that we can
    # control the cleaning order, and also to avoid accidentally including it as a dependency of
    # multiple targets, which according to CMake docs can cause race conditions. We're assuming
    # byproducts don't have that issue, but it's not entirely clear.)
    add_custom_command(
        OUTPUT "${EMDAWNWEBGPU_PKG}.stamp"
        # List the outputs. If ANY of these files are used as a dependency, it will trigger this
        # command. (List files before directories so that `ninja clean` deletes in the right order.)
        BYPRODUCTS
            "${EMDAWNWEBGPU_PKG_BYPRODUCTS}"
            "${EMDAWNWEBGPU_PKG}"
        # - First build all the generated files. Also list each file, to trigger rebuilds correctly.
        DEPENDS
            emdawnwebgpu_headers_gen
            "${EMDAWNWEBGPU_PKG_DEPENDS}"
        # - Delete the output directory.
        COMMAND ${CMAKE_COMMAND} -E rm -rf -- "${EMDAWNWEBGPU_PKG}"
        # - Then run the rest of the COMMANDs, to re-create the output directory.
        ${EMDAWNWEBGPU_PKG_COMMANDS}
        # - Finally, create or update the stamp file.
        COMMAND ${CMAKE_COMMAND} -E touch "${EMDAWNWEBGPU_PKG}.stamp"
    )

    # User-exposed target for building just emdawnwebgpu_pkg. This does NOT need to be included
    # as a dependency by any other CMake targets - use emdawnwebgpu_c or emdawnwebgpu_cpp instead.
    add_custom_target(emdawnwebgpu_pkg
        ALL
        DEPENDS "${EMDAWNWEBGPU_PKG}.stamp"
    )

    # Everything after this point should refer to files inside emdawnwebgpu_pkg instead of
    # the generator output.
    unset(EM_BUILD_GEN_DIR)

    #
    # emdawnwebgpu_c
    # Use this to get Emdawnwebgpu with only the C include directory.
    #

    dawn_add_library(
        emdawnwebgpu_c
        ENABLE_EMSCRIPTEN
        HEADERS
            "${EMDAWNWEBGPU_PKG}/webgpu/include/webgpu/webgpu.h"
        SOURCES
            "${EMDAWNWEBGPU_PKG}/webgpu/src/webgpu.cpp"
    )
    target_include_directories(emdawnwebgpu_c BEFORE PUBLIC
        "${EMDAWNWEBGPU_PKG}/webgpu/include"
    )
    macro(emdawnwebgpu_c_link_option prefix filename)
        # Add the link option, and add the file as a dependency of linking this target.
        target_link_options(emdawnwebgpu_c INTERFACE "${prefix}${filename}")
        set_property(TARGET emdawnwebgpu_c APPEND PROPERTY INTERFACE_LINK_DEPENDS "${filename}")
    endmacro()
    emdawnwebgpu_c_link_option("--js-library=" "${EMDAWNWEBGPU_PKG}/webgpu/src/library_webgpu_enum_tables.js")
    emdawnwebgpu_c_link_option("--js-library=" "${EMDAWNWEBGPU_PKG}/webgpu/src/library_webgpu_generated_sig_info.js")
    emdawnwebgpu_c_link_option("--js-library=" "${EMDAWNWEBGPU_PKG}/webgpu/src/library_webgpu_generated_struct_info.js")
    emdawnwebgpu_c_link_option("--js-library=" "${EMDAWNWEBGPU_PKG}/webgpu/src/library_webgpu.js")
    emdawnwebgpu_c_link_option("--closure-args=--externs=" "${EMDAWNWEBGPU_PKG}/webgpu/src/webgpu-externs.js")

    #
    # emdawnwebgpu_cpp
    # Use this to get Emdawnwebgpu with both C and C++ include directories.
    #

    dawn_add_library(
        emdawnwebgpu_cpp
        ENABLE_EMSCRIPTEN
        HEADER_ONLY
        HEADERS
            "${EMDAWNWEBGPU_PKG}/webgpu_cpp/include/dawn/webgpu_cpp_print.h"
            "${EMDAWNWEBGPU_PKG}/webgpu_cpp/include/webgpu/webgpu_cpp.h"
            "${EMDAWNWEBGPU_PKG}/webgpu_cpp/include/webgpu/webgpu_cpp_chained_struct.h"
            "${EMDAWNWEBGPU_PKG}/webgpu_cpp/include/webgpu/webgpu_enum_class_bitmasks.h"
        DEPENDS
            emdawnwebgpu_c
    )
    target_include_directories(emdawnwebgpu_cpp BEFORE INTERFACE
        "${EMDAWNWEBGPU_PKG}/webgpu_cpp/include"
    )

    #
    # Tests
    #

    if (${DAWN_BUILD_TESTS})
        set(emdawnwebgpu_test_sources
            "tests/FuturesTests.cpp"
            "tests/SpotTests.cpp"
        )
        set(emdawnwebgpu_test_deps
            dawn::dawn_wgpu_utils
            emdawnwebgpu_cpp
            gmock_main_wasmsafe
        )

        add_library(emdawnwebgpu_test_linkopts INTERFACE)
        target_link_options(emdawnwebgpu_test_linkopts
            INTERFACE
                "-sALLOW_MEMORY_GROWTH"
                # Always build with Closure, even in debug, for its static analysis checks
                # and to test the externs. Also needed for testing code size in release.
                "--closure=1"
        )
        if (NOT DAWN_ENABLE_ASAN)
            target_link_options(emdawnwebgpu_test_linkopts
                INTERFACE
                    # If ASAN is not enabled, enable SAFE_HEAP to catch some bugs.
                    # (ASAN+UBSAN is preferable to this.)
                    "$<$<CONFIG:Debug>:-sSAFE_HEAP>"
            )
        endif()

        add_library(emdawnwebgpu_tests)
        target_link_libraries(
            emdawnwebgpu_tests
            PUBLIC
                emdawnwebgpu_test_linkopts
                ${emdawnwebgpu_test_deps}
        )

        add_executable(emdawnwebgpu_tests_asyncify ${emdawnwebgpu_test_sources})
        set_target_properties(emdawnwebgpu_tests_asyncify PROPERTIES
            SUFFIX ".html")
        target_link_libraries(emdawnwebgpu_tests_asyncify PUBLIC emdawnwebgpu_tests)
        target_link_options(emdawnwebgpu_tests_asyncify
            PUBLIC
                # We need Asyncify or JSPI for Future tests.
                "-sASYNCIFY=1"
                "-sASYNCIFY_STACK_SIZE=10000"  # Needed when building with ASAN.
        )

        add_executable(emdawnwebgpu_tests_jspi ${emdawnwebgpu_test_sources})
        set_target_properties(emdawnwebgpu_tests_jspi PROPERTIES
            SUFFIX ".html")
        target_link_libraries(emdawnwebgpu_tests_jspi PUBLIC emdawnwebgpu_tests)
        target_link_options(emdawnwebgpu_tests_jspi
            PUBLIC
                # We need Asyncify or JSPI for Future tests.
                "-sJSPI=1"
        )

        # A "sample" that makes real (bogus) API calls to serve as a basic code size test.
        DawnJSONGenerator(
            TARGET "emdawnwebgpu_link_test_cpp"
            PRINT_NAME "emdawnwebgpu LinkTest.cpp"
            OUTPUT_SOURCES EMDAWNWEBGPU_LINK_TEST_CPP_SOURCES
        )
        add_executable(emdawnwebgpu_link_test
            ${EMDAWNWEBGPU_LINK_TEST_CPP_SOURCES}
        )
        # The test is just that this links, not that it runs (it will just
        # crash), so just build to .js. Since it has a main() function, this is
        # the same as building to .html, but skipping the .html file.
        set_target_properties(emdawnwebgpu_link_test PROPERTIES
            SUFFIX ".js")
        target_link_libraries(
            emdawnwebgpu_link_test
            PUBLIC
                emdawnwebgpu_test_linkopts
                emdawnwebgpu_c
        )

        # A "sample" that just creates a device. This is used to check the code size for the
        # elimination of unused library code.
        add_executable(emdawnwebgpu_init_only_sample
            "InitOnlySample.cpp"
        )
        set_target_properties(emdawnwebgpu_init_only_sample PROPERTIES
            SUFFIX ".js")
        target_link_libraries(
            emdawnwebgpu_init_only_sample
            PUBLIC
                emdawnwebgpu_test_linkopts
                emdawnwebgpu_cpp
        )
    endif()
endif()
