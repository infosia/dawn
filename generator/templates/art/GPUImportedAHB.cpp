#include <jni.h>
#include <android/hardware_buffer_jni.h>
#include <webgpu/webgpu.h>

#include "JNIClasses.h"
#include "JNIContext.h"
#include "structures.h"
#ifndef VK_IMAGE_LAYOUT_UNDEFINED
#define VK_IMAGE_LAYOUT_UNDEFINED 0
#endif
namespace dawn::kotlin_api {
// Struct to hold the zero-copy texture lifecycle for Begin/End access
    struct AhbRgbTextureWrapper {
        WGPUSharedTextureMemory stm;
        WGPUTexture texture;
    };

    extern "C" {

    JNIEXPORT jobject JNICALL
    Java_androidx_webgpu_helper_DawnAhbUtil_importRgbAhbNative(
            JNIEnv *env, jobject thiz, jobject deviceObj, jobject hardwareBuffer) {

        JNIClasses *classes = JNIClasses::getInstance(env);
        JNIContext c(env);

        jclass memberClass = classes->device;
        jmethodID getHandle = env->GetMethodID(memberClass, "getHandle", "()J");
        auto device = reinterpret_cast<WGPUDevice>(env->CallLongMethod(deviceObj, getHandle));
        if (!device) {
            env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "WGPUDevice handle is null.");
            return nullptr;
        }
        if (env->ExceptionCheck()) {
            return nullptr;
        }
        if (!wgpuDeviceHasFeature(device, WGPUFeatureName_SharedTextureMemoryAHardwareBuffer)) {
            env->ThrowNew(env->FindClass("java/lang/UnsupportedOperationException"),
                          "WebGPU Device lacks WGPUFeatureName_SharedTextureMemoryAHardwareBuffer.");
            return nullptr;
        }
        // --- Extract Native AHB ---
        // Note: AHardwareBuffer_fromHardwareBuffer requires API level 26.
        AHardwareBuffer *ahb = AHardwareBuffer_fromHardwareBuffer(env, hardwareBuffer);
        if (!ahb) {
            env->ThrowNew(env->FindClass("java/lang/IllegalStateException"), "Failed to extract native AHardwareBuffer.");
            return nullptr;
        }
        // --- Create SharedTextureMemory ---
        WGPUSharedTextureMemoryAHardwareBufferDescriptor ahbDesc = {};
        ahbDesc.chain.next = nullptr;
        ahbDesc.chain.sType = WGPUSType_SharedTextureMemoryAHardwareBufferDescriptor;
        ahbDesc.handle = ahb;

        WGPUSharedTextureMemoryDescriptor stmDesc = {};
        stmDesc.nextInChain = reinterpret_cast<WGPUChainedStruct *>(&ahbDesc);
        stmDesc.label = {"ZeroCopyCameraAHB_RGB", WGPU_STRLEN};

        WGPUSharedTextureMemory stm = wgpuDeviceImportSharedTextureMemory(device, &stmDesc);
        if (!stm) {
            jclass exceptionClass = env->FindClass("java/lang/IllegalStateException");
            env->ThrowNew(exceptionClass, "Dawn failed to import the AHardwareBuffer.");
            return nullptr;
        }
        WGPUSharedTextureMemoryProperties props = {};
        wgpuSharedTextureMemoryGetProperties(stm, &props);

        if (props.format == WGPUTextureFormat_Undefined) {
            wgpuSharedTextureMemoryRelease(stm);
            env->ThrowNew(env->FindClass("java/lang/UnsupportedOperationException"),
                          "Dawn imported the AHB, but its format maps to WGPUTextureFormat_Undefined. Dawn cannot sample this.");
            return nullptr;
        }

        // --- Create WGPUTexture ---
        WGPUTextureDescriptor texDesc = {};
        texDesc.nextInChain = nullptr;
        texDesc.label = {"AHB_RGB_Texture", WGPU_STRLEN};
        texDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
        texDesc.dimension = WGPUTextureDimension_2D;
        texDesc.size = {props.size.width, props.size.height, props.size.depthOrArrayLayers};
        texDesc.format = props.format;
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount = 1;
        texDesc.viewFormatCount = 0;
        texDesc.viewFormats = nullptr;

        WGPUTexture texture = wgpuSharedTextureMemoryCreateTexture(stm, &texDesc);
        if (env->ExceptionCheck()) {
            if (texture) wgpuTextureRelease(texture);
            wgpuSharedTextureMemoryRelease(stm);
            return nullptr;
        }
        if (!texture) {
            wgpuSharedTextureMemoryRelease(stm);
            jclass exceptionClass = env->FindClass("java/lang/IllegalStateException");
            env->ThrowNew(exceptionClass, "Dawn failed to create a WGPUTexture from the imported AHardwareBuffer.");
            return nullptr;
        }
        // --- Store in our wrapper for begin/end access ---
        auto *wrapper = new AhbRgbTextureWrapper{stm, texture};

        // --- Wrap WGPUTexture into androidx.webgpu.GPUTexture using cached JNIClasses ---
        jobject texture_kt;
        {
            jclass clz = classes->texture; // Fetches the cached GPUTexture class
            jmethodID init = env->GetMethodID(clz, "<init>", "(J)V");
            texture_kt = env->NewObject(clz, init, reinterpret_cast<jlong>(texture));
        }

        // --- Return our custom Kotlin class holding both the wrapper ptr and the GPUTexture ---
        jclass wrapperClz = env->FindClass("androidx/webgpu/helper/GPUImportedRGBHardwareBuffer");
        jmethodID wrapperInit = env->GetMethodID(wrapperClz, "<init>",
                                                 "(JLandroidx/webgpu/GPUTexture;)V");
        jobject result_kt = env->NewObject(wrapperClz, wrapperInit,
                                           reinterpret_cast<jlong>(wrapper), texture_kt);

        return result_kt;
    }

    JNIEXPORT void JNICALL
    Java_androidx_webgpu_helper_GPUImportedRGBHardwareBuffer_beginAccessNative(JNIEnv *env, jobject thiz) {
      jclass cls = env->GetObjectClass(thiz);
      jfieldID handleField = env->GetFieldID(cls, "handle", "J");
      jlong handle = env->GetLongField(thiz, handleField);
      auto* wrapper = reinterpret_cast<AhbRgbTextureWrapper*>(handle);

      // 1. Define the Vulkan Layout Begin State
      WGPUSharedTextureMemoryVkImageLayoutBeginState beginLayout = {};
      beginLayout.chain.sType = WGPUSType_SharedTextureMemoryVkImageLayoutBeginState;
      beginLayout.chain.next = nullptr;
      beginLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      beginLayout.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;

      // 2. Chain it to the BeginAccess descriptor
      WGPUSharedTextureMemoryBeginAccessDescriptor beginDesc = {};
      beginDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&beginLayout.chain);
      beginDesc.concurrentRead = false;
      beginDesc.initialized = true;

      wgpuSharedTextureMemoryBeginAccess(wrapper->stm, wrapper->texture, &beginDesc);
    }

    JNIEXPORT void JNICALL
    Java_androidx_webgpu_helper_GPUImportedRGBHardwareBuffer_endAccessNative(JNIEnv *env, jobject thiz) {
        jclass cls = env->GetObjectClass(thiz);
        jfieldID handleField = env->GetFieldID(cls, "handle", "J");
        jlong handle = env->GetLongField(thiz, handleField);
        auto* wrapper = reinterpret_cast<AhbRgbTextureWrapper*>(handle);

        // 1. Define the Vulkan Layout End State
        WGPUSharedTextureMemoryVkImageLayoutEndState endLayout = {};
        endLayout.chain.sType = WGPUSType_SharedTextureMemoryVkImageLayoutEndState;
        endLayout.chain.next = nullptr;
        endLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        endLayout.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        // 2. Chain it to the EndAccess state
        WGPUSharedTextureMemoryEndAccessState endState = {};
        endState.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&endLayout.chain);

        wgpuSharedTextureMemoryEndAccess(wrapper->stm, wrapper->texture, &endState);

        // Note: The Media3 code uses wgpuSharedTextureMemoryEndAccessStateFreeMembers here
        // to free exported Fences. Since we aren't exporting cross-process sync fences right now,
    }

    JNIEXPORT void JNICALL
    Java_androidx_webgpu_helper_GPUImportedRGBHardwareBuffer_destroyNative(JNIEnv *env, jobject thiz) {
        jclass cls = env->GetObjectClass(thiz);
        jfieldID handleField = env->GetFieldID(cls, "handle", "J");
        jlong handle = env->GetLongField(thiz, handleField);
        auto *wrapper = reinterpret_cast<AhbRgbTextureWrapper *>(handle);
        if (wrapper->texture) {
            wgpuTextureRelease(wrapper->texture);
        }
        if (wrapper->stm) {
            wgpuSharedTextureMemoryRelease(wrapper->stm);
        }
        delete wrapper;
    }

    } // extern "C"
}
