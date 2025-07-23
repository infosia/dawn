# Copyright 2025 The Dawn & Tint Authors
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

if(DAWN_ENABLE_DEVTOOL)
    add_library(dawn_devtool_proto_objects OBJECT)

    generate_protos(
        TARGET dawn_devtool_proto_objects
        PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/protos/cmd.proto"
        PROTOC_OUT_DIR "${DAWN_BUILD_GEN_DIR}/dawn/devtool/proto"
    )

    if (DAWN_ENABLE_DEVTOOL_RECORD)
        set(record_headers
            "RecordManager.h"
        )
        set(record_sources
            $<TARGET_OBJECTS:dawn_devtool_proto_objects>
            "RecordManager.cpp"
        )
        set(record_depends
            libprotobuf
        )
        dawn_add_library(
            dawn_devtool_record
            PRIVATE_HEADERS ${record_headers}
            SOURCES ${record_sources}
            PUBLIC_DEPENDS record_depends
        )
        target_include_directories(dawn_devtool_record PUBLIC "${DAWN_BUILD_GEN_DIR}/dawn/devtool/proto")
    endif()

    if (DAWN_ENABLE_DEVTOOL_PLAYBACK)
        set(playback_headers
            "PlaybackManager.h"
        )
        set(playback_sources
            $<TARGET_OBJECTS:dawn_devtool_proto_objects>
            "PlaybackManager.cpp"
        )
        set(playback_depends
            libprotobuf
        )
        dawn_add_library(
            dawn_devtool_playback
            PRIVATE_HEADERS ${playback_headers}
            SOURCES ${playback_sources}
            PUBLIC_DEPENDS playback_depends
        )
        target_include_directories(dawn_devtool_playback PUBLIC "${DAWN_BUILD_GEN_DIR}/dawn/devtool/proto")
    endif()
endif()
