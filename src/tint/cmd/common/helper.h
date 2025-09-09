// Copyright 2023 The Dawn & Tint Authors
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

#ifndef SRC_TINT_CMD_COMMON_HELPER_H_
#define SRC_TINT_CMD_COMMON_HELPER_H_

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "src/tint/lang/wgsl/inspector/inspector.h"
#include "src/tint/utils/diagnostic/source.h"

#if TINT_BUILD_SPV_READER
#include "src/tint/lang/spirv/reader/common/options.h"
#endif

// Forward declarations
namespace tint {
class Program;
class InternalCompilerError;
}  // namespace tint

namespace tint::cmd {

enum class InputFormat {
    kUnknown,
    kWgsl,
    kSpirvBin,
    kSpirvAsm,
};

/// Information on a loaded program
struct ProgramInfo {
    /// The loaded program
    tint::Program program;
    /// The source file information
    std::unique_ptr<tint::Source::File> source_file;
};

/// PrintWGSL writes the WGSL of the program to the provided ostream, if the
/// WGSL writer is enabled, otherwise it does nothing.
/// @param out the output stream to write the WGSL to
/// @param program the program
void PrintWGSL(std::ostream& out, const tint::Program& program);

/// Prints inspector data information to stderr
/// @param inspector the inspector to print.
void PrintInspectorData(tint::inspector::Inspector& inspector);

/// Prints inspector binding information to stderr
/// @param inspector the inspector to print.
void PrintInspectorBindings(tint::inspector::Inspector& inspector);

/// Options for the LoadProgramInfo call
struct LoadProgramOptions {
    /// The file to be loaded, might be "-" for stdin
    std::string filename;
    /// The input format. Optional for files, mandatory for stdin
    InputFormat input_format;
#if TINT_BUILD_SPV_READER
    /// Spirv-reader options
    tint::spirv::reader::Options spirv_reader_options;
#endif
    /// The text printer to use for output
    StyledTextPrinter* printer = nullptr;
};

/// Loads the source and program information for the given file.
/// If the file cannot be loaded then an error is printed and the program is aborted before
/// returning.
/// @param opts the loading options
ProgramInfo LoadProgramInfo(const LoadProgramOptions& opts);

/// @param stage the pipeline stage
/// @returns the string representation
std::string EntryPointStageToString(tint::inspector::PipelineStage stage);

/// @param dim the dimension
/// @returns the text name
std::string TextureDimensionToString(tint::inspector::ResourceBinding::TextureDimension dim);

/// @param kind the sample kind
/// @returns the text name
std::string SampledKindToString(tint::inspector::ResourceBinding::SampledKind kind);

/// @param format the texel format
/// @returns the text name
std::string TexelFormatToString(tint::inspector::ResourceBinding::TexelFormat format);

/// @param type the resource type
/// @returns the text name
std::string ResourceTypeToString(tint::inspector::ResourceBinding::ResourceType type);

/// @param type the composition type
/// @return the text name
std::string CompositionTypeToString(tint::inspector::CompositionType type);

/// @param type the component type
/// @return the text name
std::string ComponentTypeToString(tint::inspector::ComponentType type);

/// @param type the interpolation sampling type
/// @return the text name
std::string InterpolationSamplingToString(tint::inspector::InterpolationSampling type);

/// @param type the interpolation type
/// @return the text name
std::string InterpolationTypeToString(tint::inspector::InterpolationType type);

/// @param type the override type
/// @return the text name
std::string OverrideTypeToString(tint::inspector::Override::Type type);

/// Returns true if the given `name` is either empty or `-` which signifies `stdout` is selected.
bool IsStdout(const std::string& name);

/// On Windows, sets the standard input stream stdin into binary mode. Non-Windows platforms are
/// not affected. If stdin is in text mode reading the byte 0x1A might be interpreted as EOF signal.
void SetStdinModeBinary();

/// Writes the given `buffer` to standard output. If any error occurs, returns false and outputs
/// error message to standard error. The ContainerT type must have data() and size() methods, like
/// `std::string` and `std::vector` do.
/// @returns true on success
/// @private
template <typename ContainerT>
bool WriteStdoutImpl(const ContainerT& buffer) {
    size_t written =
        fwrite(buffer.data(), sizeof(typename ContainerT::value_type), buffer.size(), stdout);
    if (buffer.size() != written) {
        std::cerr << "Could not write all output to standard output\n";
        return false;
    }
    fflush(stdout);
    return true;
}

/// Writes the given `buffer` into the file named as `output_file` using the given `mode`. If any
/// error occurs, returns false and outputs error message to standard error. The ContainerT type
/// must have data() and size() methods, like `std::string` and `std::vector` do.
/// @returns true on success
/// @private
template <typename ContainerT>
bool WriteFileImpl(const std::string& output_file,
                   const std::string mode,
                   const ContainerT& buffer) {
    FILE* file = nullptr;

#if defined(_MSC_VER)
    fopen_s(&file, output_file.c_str(), mode.c_str());
#else
    file = fopen(output_file.c_str(), mode.c_str());
#endif
    if (!file) {
        std::cerr << "Could not open file " << output_file << " for writing\n";
        return false;
    }

    size_t written =
        fwrite(buffer.data(), sizeof(typename ContainerT::value_type), buffer.size(), file);
    if (buffer.size() != written) {
        std::cerr << "Could not write to file " << output_file << "\n";
        fclose(file);
        return false;
    }
    fclose(file);

    return true;
}

/// Writes the given `buffer` into the file named as `output_file` using the
/// given `mode`.  If `output_file` is empty or "-", writes to standard
/// output. If any error occurs, returns false and outputs error message to
/// standard error. The ContainerT type must have data() and size() methods,
/// like `std::string` and `std::vector` do.
/// @returns true on success
template <typename ContainerT>
bool WriteFile(const std::string& output_file, const std::string mode, const ContainerT& buffer) {
    if (IsStdout(output_file)) {
        return WriteStdoutImpl(buffer);
    }
    return WriteFileImpl(output_file, mode, buffer);
}

/// Copies the content from the file named `input_file` to `buffer`,
/// assuming each element in the file is of type `T`.  If any error occurs,
/// writes error messages to the standard error stream and returns false.
/// Assumes the size of a `T` object is divisible by its required alignment.
/// @returns true if we successfully read the file.
template <typename T>
bool ReadFileImpl(const std::string& input_file, std::vector<T>* buffer) {
    FILE* file = nullptr;
#if defined(_MSC_VER)
    fopen_s(&file, input_file.c_str(), "rb");
#else
    file = fopen(input_file.c_str(), "rb");
#endif
    if (!file) {
        std::cerr << "Failed to open " << input_file << "\n";
        return false;
    }

    fseek(file, 0, SEEK_END);
    const auto file_size = static_cast<size_t>(ftell(file));
    if (0 != (file_size % sizeof(T))) {
        std::cerr << "File " << input_file
                  << " does not contain an integral number of objects: " << file_size
                  << " bytes in the file, require " << sizeof(T) << " bytes per object\n";
        fclose(file);
        return false;
    }
    fseek(file, 0, SEEK_SET);

    buffer->clear();
    buffer->resize(file_size / sizeof(T));

    size_t bytes_read = fread(buffer->data(), 1, file_size, file);
    fclose(file);
    if (bytes_read != file_size) {
        std::cerr << "Failed to read " << input_file << "\n";
        return false;
    }

    return true;
}

/// Copies the content from stdin to `buffer`,
/// assuming each element in the file is of type `T`.
/// Maximally allowed input size is 2 GiB. If any error occurs,
/// writes error messages to the standard error stream and returns false.
/// Assumes the size of a `T` object is divisible by its required alignment.
/// @returns true if we successfully read the file.
template <typename T>
bool ReadStdinImpl(std::vector<T>* buffer) {
    // Following values are guessed, please change them if they are unsatisfying.
    // Largest temporary buffer has size 1024 << 20 = 1.074 GB = 1 GiB
    // Maximum size of all temporary buffers combined is therefore 1024 << 21 = 2.714 GB = 2 GiB
    // When this data is copied to the result buffer a total of 4GiB =  might
    // be allocated at the same time
    const size_t INITIAL_BUFFER_SIZE = 1024; // == 1 << 10;
    const size_t MAX_TMP_BUFFERS = 21;

    // Create temporary buffers holding data as we read from the stdin stream
    // The data is copied only twice:
    // 1. from stream to tmp_buffers
    // 2. from tmp_buffers to the `buffer`argument
    tint::Vector<std::vector<uint8_t>, MAX_TMP_BUFFERS> tmp_buffers;

    SetStdinModeBinary();

    size_t total_bytes_read = 0;
    bool is_stdin_fully_read = false;
    for (size_t current_buffer = 0; current_buffer < MAX_TMP_BUFFERS; current_buffer += 1) {
        // Exponential growth with exponent 2
        size_t current_buffer_size = INITIAL_BUFFER_SIZE << current_buffer;
        // Allocate buffer. std::vector unnecessarily initializes with default value 0 here
        tmp_buffers.Push(std::vector<uint8_t>(current_buffer_size));
        size_t bytes_read = fread(tmp_buffers.Back().data(), 1, current_buffer_size, stdin);
        total_bytes_read += bytes_read;
        // If the temporary buffer did not fill up completely we can end the loop
        if (bytes_read < current_buffer_size) {
            is_stdin_fully_read = true;
            break;
        }
    }
    // stdin is too large, we cannot fully read the file
    if (!is_stdin_fully_read)
    {
        std::cerr << "Standard input stream size exceeds maximum of 2.714 GB";
        return false;
    }

    if (0 != total_bytes_read % sizeof(T))
    {
        std::cerr << "Standard input stream"
            << " does not contain an integral number of objects: " << total_bytes_read
            << " bytes in the stream, require " << sizeof(T) << " bytes per object\n";
        return false;
    }

    size_t total_bytes_copied = 0;
    buffer->clear();
    buffer->resize(total_bytes_read / sizeof(T));
    for (const std::vector<uint8_t>& tmp_buffer : tmp_buffers) {
        size_t bytes_to_read = std::min(tmp_buffer.size(), total_bytes_read - total_bytes_copied);
        std::memcpy(reinterpret_cast<uint8_t*>(buffer->data()) + total_bytes_copied, tmp_buffer.data(), bytes_to_read);
        total_bytes_copied += bytes_to_read;
    }

    return true;
}

/// Copies the content from the file named `input_file` to `buffer`.
/// If `input_file` is empty or "-", reads from standard input.
/// assuming each element in the file is of type `T`.  If any error occurs,
/// writes error messages to the standard error stream and returns false.
/// @returns true on success
template <typename T>
bool ReadFile(const std::string& input_file, std::vector<T>* buffer) {
    if (!buffer) {
        std::cerr << "The buffer pointer was null\n";
        return false;
    }

    if (input_file.empty() || input_file == "-") {
        return ReadStdinImpl(buffer);
    }
    return ReadFileImpl(input_file, buffer);
}

}  // namespace tint::cmd

#endif  // SRC_TINT_CMD_COMMON_HELPER_H_
