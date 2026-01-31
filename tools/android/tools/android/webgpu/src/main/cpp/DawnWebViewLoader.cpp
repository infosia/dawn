//
// Created by sainitarun on 1/28/26.
//
#include <jni.h>
#include <dawn/dawn_proc.h>
#include <dawn/dawn_proc_table.h>

// JNI Function to initialize Dawn using the address from WebView
extern "C" JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_WebGpuUtils_mapDawnAddress(JNIEnv* env, jclass clazz, jlong procTableAddress) {
    if (procTableAddress == 0) return;

    // Cast to the correct pointer type
    const DawnProcTable* procs = reinterpret_cast<const DawnProcTable*>(procTableAddress);

    // Call the function
    ::dawnProcSetProcs(procs);
}
