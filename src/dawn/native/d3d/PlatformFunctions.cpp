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

#include "dawn/native/d3d/PlatformFunctions.h"

#include <comdef.h>
#include <versionhelpers.h>

#include <vector>

namespace dawn::native::d3d {

namespace {
// Referenced from base/win/registry.cc in Chromium
uint64_t ReadFromSZRegistryKey(HKEY registerKey, const char* registerKeyName) {
    DWORD valueType;
    DWORD returnSize;
    if (RegQueryValueExA(registerKey, registerKeyName, nullptr, &valueType, nullptr, &returnSize) !=
        ERROR_SUCCESS) {
        return 0;
    }
    std::vector<char> returnStringValue(returnSize);
    auto hr = RegQueryValueExA(registerKey, registerKeyName, nullptr, &valueType,
                               reinterpret_cast<LPBYTE>(returnStringValue.data()), &returnSize);
    if (hr != ERROR_SUCCESS || valueType != REG_SZ) {
        return 0;
    }
    constexpr int32_t kRadix = 10;
    return strtol(returnStringValue.data(), nullptr, kRadix);
}

}  // anonymous namespace

PlatformFunctions::PlatformFunctions() : mCurrentBuildNumber(0) {}

PlatformFunctions::~PlatformFunctions() = default;

MaybeError PlatformFunctions::LoadFunctions() {
    DAWN_TRY(LoadDXGI());
    DAWN_TRY(LoadFXCompiler());
    InitWindowsVersion();
    return {};
}

MaybeError PlatformFunctions::LoadDXGI() {
#if DAWN_PLATFORM_IS(WINUWP)
#if defined(_DEBUG)
    // DXGIGetDebugInterface1 is tagged as a development-only capability
    // which implies that linking to this function will cause
    // the application to fail Windows store certification
    // But we need it when debuging using VS Graphics Diagnostics or PIX
    // So we only link to it in debug build
    dxgiGetDebugInterface1 = &DXGIGetDebugInterface1;
#endif
    createDxgiFactory2 = &CreateDXGIFactory2;
#else
    std::string error;
    if (!mDXGILib.Open("dxgi.dll", &error) ||
        !mDXGILib.GetProc(&dxgiGetDebugInterface1, "DXGIGetDebugInterface1", &error) ||
        !mDXGILib.GetProc(&createDxgiFactory2, "CreateDXGIFactory2", &error)) {
        return DAWN_INTERNAL_ERROR(error.c_str());
    }
#endif

    return {};
}

MaybeError PlatformFunctions::LoadFXCompiler() {
#if DAWN_PLATFORM_IS(WINUWP)
    d3dCompile = &D3DCompile;
    d3dDisassemble = &D3DDisassemble;
#else
    std::string error;
    std::string ret_error = "Error log start:";

    if (!mFXCompilerLib.Open("api-ms-win-crt-string-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-crt-string-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "strncmp", &error)) { ret_error += "\nError loading strncmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "wcsncmp", &error)) { ret_error += "\nError loading wcsncmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "strcmp", &error)) { ret_error += "\nError loading strcmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "strlen", &error)) { ret_error += "\nError loading strlen " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__isascii", &error)) { ret_error += "\nError loading __isascii " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "strnlen", &error)) { ret_error += "\nError loading strnlen " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "memset", &error)) { ret_error += "\nError loading memset " + error; }
if (!mFXCompilerLib.Open("api-ms-win-crt-math-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-crt-math-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_isnan", &error)) { ret_error += "\nError loading _isnan " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_finite", &error)) { ret_error += "\nError loading _finite " + error; }
if (!mFXCompilerLib.Open("api-ms-win-crt-runtime-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-crt-runtime-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_initterm", &error)) { ret_error += "\nError loading _initterm " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__doserrno", &error)) { ret_error += "\nError loading __doserrno " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_initterm_e", &error)) { ret_error += "\nError loading _initterm_e " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_clearfp", &error)) { ret_error += "\nError loading _clearfp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_controlfp", &error)) { ret_error += "\nError loading _controlfp " + error; }
if (!mFXCompilerLib.Open("api-ms-win-crt-private-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-crt-private-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__filelengthi64", &error)) { ret_error += "\nError loading _o__filelengthi64 " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__fpclass", &error)) { ret_error += "\nError loading _o__fpclass " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__free_base", &error)) { ret_error += "\nError loading _o__free_base " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__fullpath", &error)) { ret_error += "\nError loading _o__fullpath " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__initialize_narrow_environment", &error)) { ret_error += "\nError loading _o__initialize_narrow_environment " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__initialize_onexit_table", &error)) { ret_error += "\nError loading _o__initialize_onexit_table " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__invalid_parameter_noinfo", &error)) { ret_error += "\nError loading _o__invalid_parameter_noinfo " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__invalid_parameter_noinfo_noreturn", &error)) { ret_error += "\nError loading _o__invalid_parameter_noinfo_noreturn " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__lseeki64", &error)) { ret_error += "\nError loading _o__lseeki64 " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__malloc_base", &error)) { ret_error += "\nError loading _o__malloc_base " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__mbscmp", &error)) { ret_error += "\nError loading _o__mbscmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__mbstrlen", &error)) { ret_error += "\nError loading _o__mbstrlen " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__memicmp", &error)) { ret_error += "\nError loading _o__memicmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__open_osfhandle", &error)) { ret_error += "\nError loading _o__open_osfhandle " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__purecall", &error)) { ret_error += "\nError loading _o__purecall " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__read", &error)) { ret_error += "\nError loading _o__read " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__register_onexit_function", &error)) { ret_error += "\nError loading _o__register_onexit_function " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__seh_filter_dll", &error)) { ret_error += "\nError loading _o__seh_filter_dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__strdup", &error)) { ret_error += "\nError loading _o__strdup " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__stricmp", &error)) { ret_error += "\nError loading _o__stricmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__strnicmp", &error)) { ret_error += "\nError loading _o__strnicmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__strtoui64", &error)) { ret_error += "\nError loading _o__strtoui64 " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wcsdup", &error)) { ret_error += "\nError loading _o__wcsdup " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wcsicmp", &error)) { ret_error += "\nError loading _o__wcsicmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wcsnicmp", &error)) { ret_error += "\nError loading _o__wcsnicmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wdupenv_s", &error)) { ret_error += "\nError loading _o__wdupenv_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wfsopen", &error)) { ret_error += "\nError loading _o__wfsopen " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wfullpath", &error)) { ret_error += "\nError loading _o__wfullpath " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wgetenv", &error)) { ret_error += "\nError loading _o__wgetenv " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wmakepath_s", &error)) { ret_error += "\nError loading _o__wmakepath_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wsplitpath_s", &error)) { ret_error += "\nError loading _o__wsplitpath_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__wtoi", &error)) { ret_error += "\nError loading _o__wtoi " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_abort", &error)) { ret_error += "\nError loading _o_abort " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_acos", &error)) { ret_error += "\nError loading _o_acos " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_asin", &error)) { ret_error += "\nError loading _o_asin " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_atan", &error)) { ret_error += "\nError loading _o_atan " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_atan2", &error)) { ret_error += "\nError loading _o_atan2 " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_atof", &error)) { ret_error += "\nError loading _o_atof " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_atoi", &error)) { ret_error += "\nError loading _o_atoi " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_bsearch", &error)) { ret_error += "\nError loading _o_bsearch " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_calloc", &error)) { ret_error += "\nError loading _o_calloc " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_ceil", &error)) { ret_error += "\nError loading _o_ceil " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_ceilf", &error)) { ret_error += "\nError loading _o_ceilf " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_cos", &error)) { ret_error += "\nError loading _o_cos " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_cosh", &error)) { ret_error += "\nError loading _o_cosh " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_exp", &error)) { ret_error += "\nError loading _o_exp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_fclose", &error)) { ret_error += "\nError loading _o_fclose " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_floor", &error)) { ret_error += "\nError loading _o_floor " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_floorf", &error)) { ret_error += "\nError loading _o_floorf " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_fmod", &error)) { ret_error += "\nError loading _o_fmod " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_fread", &error)) { ret_error += "\nError loading _o_fread " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_free", &error)) { ret_error += "\nError loading _o_free " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_fseek", &error)) { ret_error += "\nError loading _o_fseek " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_ftell", &error)) { ret_error += "\nError loading _o_ftell " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_getenv", &error)) { ret_error += "\nError loading _o_getenv " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_isalnum", &error)) { ret_error += "\nError loading _o_isalnum " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_isalpha", &error)) { ret_error += "\nError loading _o_isalpha " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_isdigit", &error)) { ret_error += "\nError loading _o_isdigit " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_isspace", &error)) { ret_error += "\nError loading _o_isspace " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_isxdigit", &error)) { ret_error += "\nError loading _o_isxdigit " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_log", &error)) { ret_error += "\nError loading _o_log " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_malloc", &error)) { ret_error += "\nError loading _o_malloc " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_modf", &error)) { ret_error += "\nError loading _o_modf " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_pow", &error)) { ret_error += "\nError loading _o_pow " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_qsort", &error)) { ret_error += "\nError loading _o_qsort " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_setlocale", &error)) { ret_error += "\nError loading _o_setlocale " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_sin", &error)) { ret_error += "\nError loading _o_sin " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_sinh", &error)) { ret_error += "\nError loading _o_sinh " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_sqrt", &error)) { ret_error += "\nError loading _o_sqrt " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_strcat_s", &error)) { ret_error += "\nError loading _o_strcat_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_strcpy_s", &error)) { ret_error += "\nError loading _o_strcpy_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_strncpy_s", &error)) { ret_error += "\nError loading _o_strncpy_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_strtod", &error)) { ret_error += "\nError loading _o_strtod " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_strtoul", &error)) { ret_error += "\nError loading _o_strtoul " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_tan", &error)) { ret_error += "\nError loading _o_tan " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_tanh", &error)) { ret_error += "\nError loading _o_tanh " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_terminate", &error)) { ret_error += "\nError loading _o_terminate " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_tolower", &error)) { ret_error += "\nError loading _o_tolower " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_toupper", &error)) { ret_error += "\nError loading _o_toupper " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_towlower", &error)) { ret_error += "\nError loading _o_towlower " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_wcscat_s", &error)) { ret_error += "\nError loading _o_wcscat_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_wcscpy_s", &error)) { ret_error += "\nError loading _o_wcscpy_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_wcsncat_s", &error)) { ret_error += "\nError loading _o_wcsncat_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_wcsncpy_s", &error)) { ret_error += "\nError loading _o_wcsncpy_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o_wcstoul", &error)) { ret_error += "\nError loading _o_wcstoul " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "strrchr", &error)) { ret_error += "\nError loading strrchr " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "strchr", &error)) { ret_error += "\nError loading strchr " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__C_specific_handler", &error)) { ret_error += "\nError loading __C_specific_handler " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__current_exception", &error)) { ret_error += "\nError loading __current_exception " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__current_exception_context", &error)) { ret_error += "\nError loading __current_exception_context " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_CxxThrowException", &error)) { ret_error += "\nError loading _CxxThrowException " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__crt_atexit", &error)) { ret_error += "\nError loading _o__crt_atexit " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__configure_narrow_argv", &error)) { ret_error += "\nError loading _o__configure_narrow_argv " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__close", &error)) { ret_error += "\nError loading _o__close " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__cexit", &error)) { ret_error += "\nError loading _o__cexit " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__callnewh", &error)) { ret_error += "\nError loading _o__callnewh " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__atoi64", &error)) { ret_error += "\nError loading _o__atoi64 " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__aligned_malloc", &error)) { ret_error += "\nError loading _o__aligned_malloc " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__aligned_free", &error)) { ret_error += "\nError loading _o__aligned_free " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___stdio_common_vswprintf_s", &error)) { ret_error += "\nError loading _o___stdio_common_vswprintf_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___stdio_common_vswprintf", &error)) { ret_error += "\nError loading _o___stdio_common_vswprintf " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___stdio_common_vsscanf", &error)) { ret_error += "\nError loading _o___stdio_common_vsscanf " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___stdio_common_vsprintf_s", &error)) { ret_error += "\nError loading _o___stdio_common_vsprintf_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___stdio_common_vsprintf", &error)) { ret_error += "\nError loading _o___stdio_common_vsprintf " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___stdio_common_vsnwprintf_s", &error)) { ret_error += "\nError loading _o___stdio_common_vsnwprintf_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___stdio_common_vsnprintf_s", &error)) { ret_error += "\nError loading _o___stdio_common_vsnprintf_s " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___std_type_info_destroy_list", &error)) { ret_error += "\nError loading _o___std_type_info_destroy_list " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___std_exception_destroy", &error)) { ret_error += "\nError loading _o___std_exception_destroy " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o___std_exception_copy", &error)) { ret_error += "\nError loading _o___std_exception_copy " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__execute_onexit_table", &error)) { ret_error += "\nError loading _o__execute_onexit_table " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "memcpy", &error)) { ret_error += "\nError loading memcpy " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__CxxFrameHandler4", &error)) { ret_error += "\nError loading __CxxFrameHandler4 " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "memmove", &error)) { ret_error += "\nError loading memmove " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "strstr", &error)) { ret_error += "\nError loading strstr " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__unDNameEx", &error)) { ret_error += "\nError loading __unDNameEx " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "memcmp", &error)) { ret_error += "\nError loading memcmp " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "wcschr", &error)) { ret_error += "\nError loading wcschr " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__unDName", &error)) { ret_error += "\nError loading __unDName " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "wcsrchr", &error)) { ret_error += "\nError loading wcsrchr " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "__CxxFrameHandler3", &error)) { ret_error += "\nError loading __CxxFrameHandler3 " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_o__errno", &error)) { ret_error += "\nError loading _o__errno " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-libraryloader-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-libraryloader-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetProcAddress", &error)) { ret_error += "\nError loading GetProcAddress " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "DisableThreadLibraryCalls", &error)) { ret_error += "\nError loading DisableThreadLibraryCalls " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "LoadLibraryExW", &error)) { ret_error += "\nError loading LoadLibraryExW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "FreeLibrary", &error)) { ret_error += "\nError loading FreeLibrary " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetModuleFileNameA", &error)) { ret_error += "\nError loading GetModuleFileNameA " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-string-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-string-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "MultiByteToWideChar", &error)) { ret_error += "\nError loading MultiByteToWideChar " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "WideCharToMultiByte", &error)) { ret_error += "\nError loading WideCharToMultiByte " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-synch-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-synch-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "AcquireSRWLockShared", &error)) { ret_error += "\nError loading AcquireSRWLockShared " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "ReleaseSRWLockShared", &error)) { ret_error += "\nError loading ReleaseSRWLockShared " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "InitializeCriticalSectionAndSpinCount", &error)) { ret_error += "\nError loading InitializeCriticalSectionAndSpinCount " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "AcquireSRWLockExclusive", &error)) { ret_error += "\nError loading AcquireSRWLockExclusive " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "InitializeSRWLock", &error)) { ret_error += "\nError loading InitializeSRWLock " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "ReleaseSRWLockExclusive", &error)) { ret_error += "\nError loading ReleaseSRWLockExclusive " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "InitializeCriticalSection", &error)) { ret_error += "\nError loading InitializeCriticalSection " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "DeleteCriticalSection", &error)) { ret_error += "\nError loading DeleteCriticalSection " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "EnterCriticalSection", &error)) { ret_error += "\nError loading EnterCriticalSection " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "LeaveCriticalSection", &error)) { ret_error += "\nError loading LeaveCriticalSection " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "InitializeCriticalSectionEx", &error)) { ret_error += "\nError loading InitializeCriticalSectionEx " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-debug-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-debug-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "OutputDebugStringA", &error)) { ret_error += "\nError loading OutputDebugStringA " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "IsDebuggerPresent", &error)) { ret_error += "\nError loading IsDebuggerPresent " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-heap-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-heap-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "HeapDestroy", &error)) { ret_error += "\nError loading HeapDestroy " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "HeapFree", &error)) { ret_error += "\nError loading HeapFree " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetProcessHeap", &error)) { ret_error += "\nError loading GetProcessHeap " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "HeapAlloc", &error)) { ret_error += "\nError loading HeapAlloc " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "HeapCreate", &error)) { ret_error += "\nError loading HeapCreate " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-file-l1-2-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-file-l1-2-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CreateFile2", &error)) { ret_error += "\nError loading CreateFile2 " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-errorhandling-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-errorhandling-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "SetLastError", &error)) { ret_error += "\nError loading SetLastError " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "UnhandledExceptionFilter", &error)) { ret_error += "\nError loading UnhandledExceptionFilter " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "SetUnhandledExceptionFilter", &error)) { ret_error += "\nError loading SetUnhandledExceptionFilter " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetLastError", &error)) { ret_error += "\nError loading GetLastError " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-file-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-file-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetFullPathNameA", &error)) { ret_error += "\nError loading GetFullPathNameA " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetFullPathNameW", &error)) { ret_error += "\nError loading GetFullPathNameW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CreateFileW", &error)) { ret_error += "\nError loading CreateFileW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetFileSize", &error)) { ret_error += "\nError loading GetFileSize " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetFileAttributesW", &error)) { ret_error += "\nError loading GetFileAttributesW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "SetFileAttributesW", &error)) { ret_error += "\nError loading SetFileAttributesW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "DeleteFileW", &error)) { ret_error += "\nError loading DeleteFileW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetFileType", &error)) { ret_error += "\nError loading GetFileType " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "SetEndOfFile", &error)) { ret_error += "\nError loading SetEndOfFile " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "SetFilePointerEx", &error)) { ret_error += "\nError loading SetFilePointerEx " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "WriteFile", &error)) { ret_error += "\nError loading WriteFile " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "ReadFile", &error)) { ret_error += "\nError loading ReadFile " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CreateFileA", &error)) { ret_error += "\nError loading CreateFileA " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetFileSizeEx", &error)) { ret_error += "\nError loading GetFileSizeEx " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-handle-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-handle-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CloseHandle", &error)) { ret_error += "\nError loading CloseHandle " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-synch-l1-2-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-synch-l1-2-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "Sleep", &error)) { ret_error += "\nError loading Sleep " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-processthreads-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-processthreads-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetCurrentProcess", &error)) { ret_error += "\nError loading GetCurrentProcess " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "TlsGetValue", &error)) { ret_error += "\nError loading TlsGetValue " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "TlsSetValue", &error)) { ret_error += "\nError loading TlsSetValue " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "TlsAlloc", &error)) { ret_error += "\nError loading TlsAlloc " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "TerminateProcess", &error)) { ret_error += "\nError loading TerminateProcess " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "TlsFree", &error)) { ret_error += "\nError loading TlsFree " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetCurrentProcessId", &error)) { ret_error += "\nError loading GetCurrentProcessId " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetCurrentThreadId", &error)) { ret_error += "\nError loading GetCurrentThreadId " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-processenvironment-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-processenvironment-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetEnvironmentVariableA", &error)) { ret_error += "\nError loading GetEnvironmentVariableA " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "ExpandEnvironmentStringsW", &error)) { ret_error += "\nError loading ExpandEnvironmentStringsW " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-memory-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-memory-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CreateFileMappingW", &error)) { ret_error += "\nError loading CreateFileMappingW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "VirtualFree", &error)) { ret_error += "\nError loading VirtualFree " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "UnmapViewOfFile", &error)) { ret_error += "\nError loading UnmapViewOfFile " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "MapViewOfFile", &error)) { ret_error += "\nError loading MapViewOfFile " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "MapViewOfFileEx", &error)) { ret_error += "\nError loading MapViewOfFileEx " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "VirtualAlloc", &error)) { ret_error += "\nError loading VirtualAlloc " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-sysinfo-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-sysinfo-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetSystemTimeAsFileTime", &error)) { ret_error += "\nError loading GetSystemTimeAsFileTime " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetSystemInfo", &error)) { ret_error += "\nError loading GetSystemInfo " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-rtlsupport-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-rtlsupport-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RtlVirtualUnwind", &error)) { ret_error += "\nError loading RtlVirtualUnwind " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RtlLookupFunctionEntry", &error)) { ret_error += "\nError loading RtlLookupFunctionEntry " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RtlCaptureContext", &error)) { ret_error += "\nError loading RtlCaptureContext " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-processthreads-l1-1-1.dll", &error)) { ret_error += "\nError loading api-ms-win-core-processthreads-l1-1-1.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "IsProcessorFeaturePresent", &error)) { ret_error += "\nError loading IsProcessorFeaturePresent " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-profile-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-profile-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "QueryPerformanceCounter", &error)) { ret_error += "\nError loading QueryPerformanceCounter " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-interlocked-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-interlocked-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "InitializeSListHead", &error)) { ret_error += "\nError loading InitializeSListHead " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-heap-obsolete-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-heap-obsolete-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "LocalAlloc", &error)) { ret_error += "\nError loading LocalAlloc " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "LocalFree", &error)) { ret_error += "\nError loading LocalFree " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-registry-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-registry-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RegQueryValueExA", &error)) { ret_error += "\nError loading RegQueryValueExA " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RegCloseKey", &error)) { ret_error += "\nError loading RegCloseKey " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RegOpenKeyExW", &error)) { ret_error += "\nError loading RegOpenKeyExW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RegEnumKeyExA", &error)) { ret_error += "\nError loading RegEnumKeyExA " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RegQueryValueExW", &error)) { ret_error += "\nError loading RegQueryValueExW " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "RegOpenKeyExA", &error)) { ret_error += "\nError loading RegOpenKeyExA " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-localization-l1-2-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-localization-l1-2-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "LCMapStringW", &error)) { ret_error += "\nError loading LCMapStringW " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-io-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-io-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "DeviceIoControl", &error)) { ret_error += "\nError loading DeviceIoControl " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-kernel32-legacy-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-kernel32-legacy-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CreateFileMappingA", &error)) { ret_error += "\nError loading CreateFileMappingA " + error; }
if (!mFXCompilerLib.Open("RPCRT4.dll", &error)) { ret_error += "\nError loading RPCRT4.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "UuidCreate", &error)) { ret_error += "\nError loading UuidCreate " + error; }
if (!mFXCompilerLib.Open("api-ms-win-crt-time-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-crt-time-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "_time64", &error)) { ret_error += "\nError loading _time64 " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-string-obsolete-l1-1-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-string-obsolete-l1-1-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "lstrcmpiA", &error)) { ret_error += "\nError loading lstrcmpiA " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-threadpool-l1-2-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-threadpool-l1-2-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "SubmitThreadpoolWork", &error)) { ret_error += "\nError loading SubmitThreadpoolWork " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "WaitForThreadpoolWorkCallbacks", &error)) { ret_error += "\nError loading WaitForThreadpoolWorkCallbacks " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CreateThreadpoolWork", &error)) { ret_error += "\nError loading CreateThreadpoolWork " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CloseThreadpoolWork", &error)) { ret_error += "\nError loading CloseThreadpoolWork " + error; }
if (!mFXCompilerLib.Open("api-ms-win-core-sysinfo-l1-2-0.dll", &error)) { ret_error += "\nError loading api-ms-win-core-sysinfo-l1-2-0.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "GetNativeSystemInfo", &error)) { ret_error += "\nError loading GetNativeSystemInfo " + error; }
if (!mFXCompilerLib.Open("CRYPTSP.dll", &error)) { ret_error += "\nError loading CRYPTSP.dll " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CryptReleaseContext", &error)) { ret_error += "\nError loading CryptReleaseContext " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CryptDestroyHash", &error)) { ret_error += "\nError loading CryptDestroyHash " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CryptHashData", &error)) { ret_error += "\nError loading CryptHashData " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CryptCreateHash", &error)) { ret_error += "\nError loading CryptCreateHash " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CryptGetHashParam", &error)) { ret_error += "\nError loading CryptGetHashParam " + error; }
if (!mFXCompilerLib.GetProc(&d3dCompile, "CryptAcquireContextW", &error)) { ret_error += "\nError loading CryptAcquireContextW " + error; }

    if (!mFXCompilerLib.Open("d3dcompiler_47.dll", &error)) {
        ret_error += ("\nError loading the d3dcompiler_47.dll!!! " + error);
        return DAWN_INTERNAL_ERROR(ret_error.c_str());
    }
    if (!mFXCompilerLib.GetProc(&d3dCompile, "D3DCompile", &error) ||
        !mFXCompilerLib.GetProc(&d3dDisassemble, "D3DDisassemble", &error)) {
        ret_error += ("\nError loading One of of PRocs " + error);
        return DAWN_INTERNAL_ERROR(ret_error.c_str());
    }
    return DAWN_INTERNAL_ERROR(ret_error.c_str());
#endif
    //return {};
}

void PlatformFunctions::InitWindowsVersion() {
    // Currently we only care about the build number of Windows 10 and Windows 11.
    if (!IsWindows10OrGreater()) {
        return;
    }

    // Referenced from base/win/windows_version.cc in Chromium
    constexpr wchar_t kRegKeyWindowsNTCurrentVersion[] =
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, kRegKeyWindowsNTCurrentVersion, 0, KEY_QUERY_VALUE,
                      &hKey) != ERROR_SUCCESS) {
        return;
    }

    mCurrentBuildNumber = ReadFromSZRegistryKey(hKey, "CurrentBuildNumber");

    RegCloseKey(hKey);
}

uint64_t PlatformFunctions::GetWindowsBuildNumber() const {
    return mCurrentBuildNumber;
}

}  // namespace dawn::native::d3d
