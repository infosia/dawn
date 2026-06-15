#include <jni.h>
#include <dawn/dawn_proc.h>
#include <dawn/dawn_proc_table.h>

#include <android/log.h>
#define LOG_TAG "WebViewDawnProcMapper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// Generated function that takes getProcAddress and sets up the local DawnProcTable.
extern void InitializeDawnProcsFromWebView(WGPUProcGetProcAddress getProcAddress, JNIEnv* env, jintArray requiredFeatures);

/**
 * JNI Implementation for WebGpuUtils.nativeSetDawnGetProcAddress.
 * 
 * Takes the memory address of the WGPUProcGetProcAddress function from Chromium WebView,
 * and invokes the auto-generated method
 * to correctly wire up all Dawn function pointers for the current Android app.
 */
#include <dawn/native/DawnNative.h>
#include <dlfcn.h>
#include <android/log.h>

extern "C" JNIEXPORT jboolean JNICALL
Java_androidx_webgpu_helper_WebGpuUtils_nativeInitializeBundledDawn(JNIEnv* env, jclass clazz) {
    void* handle = dlopen("libwebgpu_dawn_shared.so", RTLD_NOW);
    if (!handle) {
        __android_log_print(ANDROID_LOG_ERROR, "WebGpu", "Failed to load libwebgpu_dawn_shared.so: %s", dlerror());
        return JNI_FALSE;
    }
    
    typedef const DawnProcTable* (*GetProcsFunc)();
    GetProcsFunc getProcs = (GetProcsFunc)dlsym(handle, "getBundledDawnProcs");
    if (!getProcs) {
        __android_log_print(ANDROID_LOG_ERROR, "WebGpu", "Failed to find getBundledDawnProcs: %s", dlerror());
        dlclose(handle);
        return JNI_FALSE;
    }
    
    const DawnProcTable* procs = getProcs();
    dawnProcSetProcs(procs);
    __android_log_print(ANDROID_LOG_INFO, "WebGpu", "Successfully initialized bundled Dawn dynamically.");
    return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_WebGpuUtils_nativeSetDawnGetProcAddress(JNIEnv* env, jclass clazz, jlong getProcAddressPtr, jintArray requiredFeatures) {
    LOGI("Java_androidx_webgpu_helper_WebGpuUtils_nativeSetDawnGetProcAddress called with address %lld", (long long)getProcAddressPtr);
    if (getProcAddressPtr == 0) {
        ::InitializeDawnProcsFromWebView(nullptr, env, requiredFeatures);
        return;
    }

    // Cast the raw memory address to WGPUProcGetProcAddress
    WGPUProcGetProcAddress getProcAddress = reinterpret_cast<WGPUProcGetProcAddress>(getProcAddressPtr);
    LOGI("Received Dawn GetProcAddress pointer from WebView.");

    // Call the generated function using getProcAddress to avoid ABI mismatch issues
    // between Chromium's bundled Dawn and the Android Jetpack version.
    ::InitializeDawnProcsFromWebView(getProcAddress, env, requiredFeatures);
}
