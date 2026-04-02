#include <vector>
#include <unistd.h>
#include <jni.h>
#include <android/hardware_buffer_jni.h>
#include <webgpu/webgpu.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

#include "src/dawn/common/ColorSpace.h"
#include "src/dawn/common/ExternalTextureParams.h"
#include "JNIClasses.h"
#include "JNIContext.h"
#include "structures.h"

#ifndef VK_IMAGE_LAYOUT_UNDEFINED
#define VK_IMAGE_LAYOUT_UNDEFINED 0
#endif

namespace dawn::kotlin_api {

struct AhbTextureWrapper {
  WGPUSharedTextureMemory stm = nullptr;
  WGPUTexture texture = nullptr;
  WGPUTextureView view = nullptr;
  WGPUExternalTexture externalTexture = nullptr;
  AHardwareBuffer* ahb = nullptr;
  WGPUDevice device = nullptr;
};

extern "C" {

static WGPUSharedTextureMemory createSharedTextureMemoryFromAhb(
    JNIEnv* env, jobject deviceObj, jobject hardwareBuffer, AHardwareBuffer** outAhb, WGPUDevice* outDevice) {

  JNIClasses* classes = JNIClasses::getInstance(env);
  jmethodID getHandle = env->GetMethodID(classes->device, "getHandle", "()J");
  *outDevice = reinterpret_cast<WGPUDevice>(env->CallLongMethod(deviceObj, getHandle));

  if (!*outDevice) {
    env->ThrowNew(classes->javaIllegalStateException, "WGPUDevice handle is null.");
    return nullptr;
  }

  *outAhb = AHardwareBuffer_fromHardwareBuffer(env, hardwareBuffer);
  if (!*outAhb) {
    env->ThrowNew(classes->javaIllegalStateException, "Failed to extract native AHardwareBuffer.");
    return nullptr;
  }

  WGPUSharedTextureMemoryAHardwareBufferDescriptor ahbDesc = {};
  ahbDesc.chain.sType = WGPUSType_SharedTextureMemoryAHardwareBufferDescriptor;
  ahbDesc.handle = *outAhb;

  WGPUSharedTextureMemoryDescriptor stmDesc = {};
  stmDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&ahbDesc);
  stmDesc.label = {"ZeroCopyCameraAHB", WGPU_STRLEN};

  WGPUSharedTextureMemory stm = wgpuDeviceImportSharedTextureMemory(*outDevice, &stmDesc);
  if (!stm && !env->ExceptionCheck()) {
    env->ThrowNew(classes->javaIllegalStateException, "Dawn failed to import the AHardwareBuffer.");
  }
  return stm;
}

JNIEXPORT jobject JNICALL
Java_androidx_webgpu_helper_GPUAndroidHardwareBufferUtil_createTextureNative(
    JNIEnv* env, jclass clazz, jobject deviceObj, jobject hardwareBuffer) {

  JNIContext c(env);
  JNIClasses* classes = JNIClasses::getInstance(env);
  WGPUDevice device = nullptr;
  AHardwareBuffer* ahb = nullptr;

  WGPUSharedTextureMemory stm = createSharedTextureMemoryFromAhb(env, deviceObj, hardwareBuffer, &ahb, &device);
  if (!stm) {
    if (ahb) AHardwareBuffer_release(ahb);
    return nullptr;
  }

  if (!wgpuDeviceHasFeature(device, WGPUFeatureName_SharedTextureMemoryAHardwareBuffer)) {
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    env->ThrowNew(classes->javaUnsupportedOperationException, "Missing WGPUFeatureName_SharedTextureMemoryAHardwareBuffer.");
    return nullptr;
  }

  WGPUSharedTextureMemoryProperties props = {};
  wgpuSharedTextureMemoryGetProperties(stm, &props);

  if (props.format == WGPUTextureFormat_Undefined) {
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    env->ThrowNew(classes->javaUnsupportedOperationException, "AHB format maps to WGPUTextureFormat_Undefined. Dawn cannot sample this.");
    return nullptr;
  }

  WGPUTextureDescriptor texDesc = {};
  texDesc.label = {"AHB_RGB_Texture", WGPU_STRLEN};
  texDesc.usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopySrc;
  texDesc.dimension = WGPUTextureDimension_2D;
  texDesc.size = {props.size.width, props.size.height, props.size.depthOrArrayLayers};
  texDesc.format = props.format;
  texDesc.mipLevelCount = 1;
  texDesc.sampleCount = 1;

  WGPUTexture texture = wgpuSharedTextureMemoryCreateTexture(stm, &texDesc);
  if (!texture || env->ExceptionCheck()) {
    if (texture) wgpuTextureRelease(texture);
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    return nullptr;
  }

  wgpuDeviceAddRef(device);
  auto* wrapper = new AhbTextureWrapper{stm, texture, nullptr, nullptr, ahb, device};

  jobject texture_kt = env->NewObject(classes->texture, env->GetMethodID(classes->texture, "<init>", "(J)V"), reinterpret_cast<jlong>(texture));

  jclass wrapperClz = classes->gpuHardwareBufferTexture;
  jmethodID wrapperInit = env->GetMethodID(wrapperClz, "<init>", "(JLandroidx/webgpu/GPUTexture;)V");
  return env->NewObject(wrapperClz, wrapperInit, reinterpret_cast<jlong>(wrapper), texture_kt);
}

JNIEXPORT jobject JNICALL
Java_androidx_webgpu_helper_GPUAndroidHardwareBufferUtil_createExternalTextureNative(
    JNIEnv* env, jclass clazz, jobject deviceObj, jobject hardwareBuffer, jobject descriptorObj) {

  JNIContext c(env);
  JNIClasses* classes = JNIClasses::getInstance(env);
  WGPUDevice device = nullptr;
  AHardwareBuffer* ahb = nullptr;

  WGPUSharedTextureMemory stm = createSharedTextureMemoryFromAhb(env, deviceObj, hardwareBuffer, &ahb, &device);
  if (!stm) {
    if (ahb) AHardwareBuffer_release(ahb);
    return nullptr;
  }

  if (!wgpuDeviceHasFeature(device, WGPUFeatureName_OpaqueYCbCrAndroidForExternalTexture)) {
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    env->ThrowNew(classes->javaUnsupportedOperationException, "Missing WGPUFeatureName_OpaqueYCbCrAndroidForExternalTexture.");
    return nullptr;
  }

  WGPUSharedTextureMemoryProperties props = {};
  wgpuSharedTextureMemoryGetProperties(stm, &props);
  if (env->ExceptionCheck()) {
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    return nullptr;
  }

  if (props.format == WGPUTextureFormat_Undefined) {
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    env->ThrowNew(classes->javaUnsupportedOperationException, "YUV AHB format maps to WGPUTextureFormat_Undefined.");
    return nullptr;
  }

  WGPUTextureDescriptor texDesc = {};
  texDesc.usage = WGPUTextureUsage_TextureBinding;
  texDesc.dimension = WGPUTextureDimension_2D;
  texDesc.size = {props.size.width, props.size.height, 1};
  texDesc.format = props.format;
  texDesc.mipLevelCount = 1;
  texDesc.sampleCount = 1;

  WGPUTexture texture = wgpuSharedTextureMemoryCreateTexture(stm, &texDesc);
  WGPUTextureView view = wgpuTextureCreateView(texture, nullptr);
  if (env->ExceptionCheck()) {
    if (view) wgpuTextureViewRelease(view);
    if (texture) wgpuTextureRelease(texture);
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    return nullptr;
  }

  WGPUExternalTextureDescriptor extDesc = {};
  extDesc.plane0 = view;

  jclass descClass = env->GetObjectClass(descriptorObj);

  auto callObjectMethod = [&](const char* methodName, const char* sig) {
    jmethodID mid = env->GetMethodID(descClass, methodName, sig);
    return mid ? env->CallObjectMethod(descriptorObj, mid) : nullptr;
  };

  jobject colorSpaceObj = callObjectMethod("getSrcColorSpace", "()Landroidx/webgpu/GPUColorSpaceDawn;");
  jint dstColorSpaceInt = env->CallIntMethod(descriptorObj, env->GetMethodID(descClass, "getDstColorSpace", "()I"));

  wgpu::ColorSpaceDawn srcColorSpace = {};
  jclass csClass = env->GetObjectClass(colorSpaceObj);
  srcColorSpace.primaries = static_cast<wgpu::ColorSpacePrimariesDawn>(env->GetIntField(colorSpaceObj, env->GetFieldID(csClass, "primaries", "I")));
  srcColorSpace.transfer = static_cast<wgpu::ColorSpaceTransferDawn>(env->GetIntField(colorSpaceObj, env->GetFieldID(csClass, "transfer", "I")));
  srcColorSpace.yCbCrRange = static_cast<wgpu::ColorSpaceYCbCrRangeDawn>(env->GetIntField(colorSpaceObj, env->GetFieldID(csClass, "yCbCrRange", "I")));
  srcColorSpace.yCbCrMatrix = static_cast<wgpu::ColorSpaceYCbCrMatrixDawn>(env->GetIntField(colorSpaceObj, env->GetFieldID(csClass, "yCbCrMatrix", "I")));
  env->DeleteLocalRef(csClass);
  env->DeleteLocalRef(colorSpaceObj);

  wgpu::PredefinedColorSpace dstColorSpace = static_cast<wgpu::PredefinedColorSpace>(dstColorSpaceInt);

  dawn::ExternalTextureColorSpaceParams params;
  wgpu::Status status = dawn::ComputeExternalTextureParams(srcColorSpace, dstColorSpace, &params);

  if (status != wgpu::Status::Success) {
    // FIX: Plugged memory leak
    wgpuTextureViewRelease(view);
    wgpuTextureRelease(texture);
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    env->ThrowNew(classes->javaIllegalStateException, "Failed to compute external texture params.");
    return nullptr;
  }

  extDesc.yuvToRgbConversionMatrix = params.yuvToRgbConversionMatrix.data();
  extDesc.srcTransferFunctionParameters = params.srcTransferFunction.data();
  extDesc.dstTransferFunctionParameters = params.dstTransferFunction.data();
  extDesc.gamutConversionMatrix = params.gamutConversionMatrix.data();

  jobject originObj = callObjectMethod("getCropOrigin", "()Landroidx/webgpu/GPUOrigin2D;");
  if (originObj) {
    jclass originClass = env->GetObjectClass(originObj);
    extDesc.cropOrigin.x = env->GetIntField(originObj, env->GetFieldID(originClass, "x", "I"));
    extDesc.cropOrigin.y = env->GetIntField(originObj, env->GetFieldID(originClass, "y", "I"));
    env->DeleteLocalRef(originClass);
    env->DeleteLocalRef(originObj);
  }

  jobject cropSizeObj = callObjectMethod("getCropSize", "()Landroidx/webgpu/GPUExtent2D;");
  if (cropSizeObj) {
    jclass extentClass = env->GetObjectClass(cropSizeObj);
    extDesc.cropSize.width = env->GetIntField(cropSizeObj, env->GetFieldID(extentClass, "width", "I"));
    extDesc.cropSize.height = env->GetIntField(cropSizeObj, env->GetFieldID(extentClass, "height", "I"));
    env->DeleteLocalRef(extentClass);
    env->DeleteLocalRef(cropSizeObj);
  }
  if (extDesc.cropSize.width == 0) extDesc.cropSize.width = props.size.width;
  if (extDesc.cropSize.height == 0) extDesc.cropSize.height = props.size.height;

  extDesc.apparentSize = extDesc.cropSize;
  extDesc.mirrored = env->CallBooleanMethod(descriptorObj, env->GetMethodID(descClass, "isMirrored", "()Z"));

  jint rotation = env->CallIntMethod(descriptorObj, env->GetMethodID(descClass, "getRotation", "()I"));
  switch (rotation) {
    case 2: extDesc.rotation = WGPUExternalTextureRotation_Rotate90Degrees; break;
    case 3: extDesc.rotation = WGPUExternalTextureRotation_Rotate180Degrees; break;
    case 4: extDesc.rotation = WGPUExternalTextureRotation_Rotate270Degrees; break;
    default: extDesc.rotation = WGPUExternalTextureRotation_Rotate0Degrees; break;
  }

  WGPUExternalTexture externalTexture = wgpuDeviceCreateExternalTexture(device, &extDesc);
  env->DeleteLocalRef(descClass);

  wgpuDeviceAddRef(device);
  auto* wrapper = new AhbTextureWrapper{stm, texture, view, externalTexture, ahb, device};

  jobject extTex_kt = env->NewObject(classes->externalTexture, env->GetMethodID(classes->externalTexture, "<init>", "(J)V"), reinterpret_cast<jlong>(externalTexture));

  jclass wrapperClz = classes->gpuHardwareBufferExternalTexture;
  jmethodID wrapperInit = env->GetMethodID(wrapperClz, "<init>", "(JLandroidx/webgpu/GPUExternalTexture;)V");

  return env->NewObject(wrapperClz, wrapperInit, reinterpret_cast<jlong>(wrapper), extTex_kt);
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUHardwareBufferWrapper_nativeBeginAccess(JNIEnv* env, jobject thiz, jlong handle, jobjectArray fences) {
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  std::vector<WGPUSharedFence> importedFences;
  if (fences != nullptr) {
    jsize len = env->GetArrayLength(fences);
    if (len > 0) {
      jclass pfdClass = env->FindClass("android/os/ParcelFileDescriptor");
      jmethodID detachFdMethod = env->GetMethodID(pfdClass, "detachFd", "()I");
      for (int i = 0; i < len; ++i) {
        jobject pfdObj = env->GetObjectArrayElement(fences, i);
        if (pfdObj) {
          int fd = env->CallIntMethod(pfdObj, detachFdMethod);
          if (fd >= 0) {
            WGPUSharedFenceSyncFDDescriptor syncFdDesc = {};
            syncFdDesc.chain.sType = WGPUSType_SharedFenceSyncFDDescriptor;
            syncFdDesc.handle = fd;

            WGPUSharedFenceDescriptor fenceDesc = {};
            fenceDesc.nextInChain = &syncFdDesc.chain;

            WGPUSharedFence fence = wgpuDeviceImportSharedFence(wrapper->device, &fenceDesc);
            close(fd);
            if (fence) {
              importedFences.push_back(fence);
            }
          }
          env->DeleteLocalRef(pfdObj);
        }
      }
    }
  }

  // FIX: Re-added VkImageLayoutBeginState for strict Dawn Validation
  WGPUSharedTextureMemoryVkImageLayoutBeginState beginLayout = {};
  beginLayout.chain.sType = WGPUSType_SharedTextureMemoryVkImageLayoutBeginState;
  beginLayout.chain.next = nullptr;
  beginLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  beginLayout.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  WGPUSharedTextureMemoryBeginAccessDescriptor beginDesc = {};
  beginDesc.fenceCount = importedFences.size();
  beginDesc.fences = importedFences.data();
  beginDesc.concurrentRead = false;
  beginDesc.initialized = true;
  beginDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&beginLayout.chain);

  wgpuSharedTextureMemoryBeginAccess(wrapper->stm, wrapper->texture, &beginDesc);

  for (WGPUSharedFence f : importedFences) {
    wgpuSharedFenceRelease(f);
  }
}

JNIEXPORT jobjectArray JNICALL
Java_androidx_webgpu_helper_GPUHardwareBufferWrapper_nativeEndAccess(JNIEnv* env, jobject thiz, jlong handle) {
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  WGPUSharedTextureMemoryVkImageLayoutEndState endLayout = {};
  endLayout.chain.sType = WGPUSType_SharedTextureMemoryVkImageLayoutEndState;
  endLayout.chain.next = nullptr;
  endLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  endLayout.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  WGPUSharedTextureMemoryEndAccessState endState = {};
  endState.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&endLayout.chain);

  wgpuSharedTextureMemoryEndAccess(wrapper->stm, wrapper->texture, &endState);

  jclass pfdClass = env->FindClass("android/os/ParcelFileDescriptor");
  jobjectArray resultFenceArray = env->NewObjectArray(endState.fenceCount, pfdClass, nullptr);

  if (endState.fenceCount > 0) {
    jmethodID adoptFdMethod = env->GetStaticMethodID(pfdClass, "adoptFd", "(I)Landroid/os/ParcelFileDescriptor;");
    for (size_t i = 0; i < endState.fenceCount; ++i) {
      WGPUSharedFence cFence = endState.fences[i];

      WGPUSharedFenceExportInfo exportInfo = {};
      WGPUSharedFenceSyncFDExportInfo syncFdExportInfo = {};
      syncFdExportInfo.chain.sType = WGPUSType_SharedFenceSyncFDExportInfo;
      exportInfo.nextInChain = &syncFdExportInfo.chain;

      wgpuSharedFenceExportInfo(cFence, &exportInfo);
      int fd = syncFdExportInfo.handle;
      jobject pfdObj = nullptr;
      if (fd >= 0) {
        int dupFd = dup(fd);
        pfdObj = env->CallStaticObjectMethod(pfdClass, adoptFdMethod, dupFd);
      }
      env->SetObjectArrayElement(resultFenceArray, i, pfdObj);
      if (pfdObj) env->DeleteLocalRef(pfdObj);
    }
  }

  wgpuSharedTextureMemoryEndAccessStateFreeMembers(endState);
  return resultFenceArray;
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUHardwareBufferWrapper_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  if (wrapper->externalTexture) wgpuExternalTextureRelease(wrapper->externalTexture);
  if (wrapper->view) wgpuTextureViewRelease(wrapper->view);
  if (wrapper->texture) wgpuTextureRelease(wrapper->texture);
  if (wrapper->stm) wgpuSharedTextureMemoryRelease(wrapper->stm);
  if (wrapper->ahb) AHardwareBuffer_release(wrapper->ahb);
  if (wrapper->device) wgpuDeviceRelease(wrapper->device);

  delete wrapper;
}

JNIEXPORT jboolean JNICALL
Java_androidx_webgpu_helper_GPUSyncFence_awaitParcelFileDescriptorNative(JNIEnv *env, jobject thiz, jobject pfd, jlong timeoutMillis) {
  if (!pfd) return JNI_TRUE;

  jclass pfdClass = env->GetObjectClass(pfd);
  jmethodID getFdMethod = env->GetMethodID(pfdClass, "getFd", "()I");
  int fd = env->CallIntMethod(pfd, getFdMethod);

  if (fd < 0) return JNI_TRUE;

  struct pollfd pfd_struct = {
      .fd = fd,
      .events = POLLIN,
  };

  int result = poll(&pfd_struct, 1, timeoutMillis);
  return result > 0 ? JNI_TRUE : JNI_FALSE;
}

} // extern "C"
} // namespace dawn::kotlin_api