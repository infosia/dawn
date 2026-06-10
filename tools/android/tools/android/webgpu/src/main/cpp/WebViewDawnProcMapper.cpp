#include <jni.h>
#include <dawn/dawn_proc.h>
#include <dawn/dawn_proc_table.h>

#include <android/log.h>
#define LOG_TAG "WebViewDawnProcMapper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

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
    LOGI("Java_androidx_webgpu_helper_WebGpuUtils_nativeSetDawnProcTable called with address %lld", (long long)procTableAddress);
    if (procTableAddress == 0) return;

    // Cast the raw memory address back to a DawnProcTable struct
    const DawnProcTable* procs = reinterpret_cast<const DawnProcTable*>(procTableAddress);
    LOGI("Received DawnProcTable from WebView. Version: %02x%02x%02x%02x", procs->version[0], procs->version[1], procs->version[2], procs->version[3]);

    // Call the generated function using getProcAddress to avoid ABI mismatch issues
    // between Chromium's bundled Dawn and the Android Jetpack version.
    ::InitializeDawnProcsFromWebView(procs->getProcAddress);
}
