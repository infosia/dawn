// Copyright 2024 The Dawn & Tint Authors
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

#include <jni.h>
#include <dawn/dawn_proc.h>
#include <dawn/dawn_proc_table.h>

#include <android/log.h>

// Generated function that takes getProcAddress and sets up the local DawnProcTable.
extern void InitializeDawnProcsFromWebView(WGPUProcGetProcAddress getProcAddress);

/**
 * JNI Implementation for WebGpuUtils.nativeSetDawnProcTable.
 * 
 * Takes the memory address of the DawnProcTable populated by Chromium WebView,
 * extracts its `getProcAddress` function, and invokes the auto-generated method
 * to correctly wire up all Dawn function pointers for the current Android app.
 */
extern "C" JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_WebGpuUtils_nativeSetDawnProcTable(JNIEnv* env, jclass clazz, jlong procTableAddress) {
    if (procTableAddress == 0) return;

    // Cast the raw memory address back to a DawnProcTable struct
    const DawnProcTable* procs = reinterpret_cast<const DawnProcTable*>(procTableAddress);

    // Call the generated function using getProcAddress to avoid ABI mismatch issues
    // between Chromium's bundled Dawn and the Android Jetpack version.
    ::InitializeDawnProcsFromWebView(procs->getProcAddress);
}
