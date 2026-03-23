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
// Universal wrapper for both RGB and YUV zero-copy textures
struct AhbTextureWrapper {
  WGPUSharedTextureMemory stm = nullptr;
  WGPUTexture texture = nullptr;               // Used by both
  WGPUTextureView view = nullptr;              // Used only by YUV (Plane 0)
  WGPUExternalTexture externalTexture = nullptr; // Used only by YUV
  AHardwareBuffer* ahb = nullptr;              // Track AHB to prevent native memory leak
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

  // AHardwareBuffer_fromHardwareBuffer acquires a reference, so it must be tracked and released later.
  *outAhb = AHardwareBuffer_fromHardwareBuffer(env, hardwareBuffer);
  if (!*outAhb) {
    env->ThrowNew(classes->javaIllegalStateException, "Failed to extract native AHardwareBuffer.");
    return nullptr;
  }

  WGPUSharedTextureMemoryAHardwareBufferDescriptor ahbDesc = {};
  ahbDesc.chain.sType = WGPUSType_SharedTextureMemoryAHardwareBufferDescriptor;
  ahbDesc.chain.next = nullptr;
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
Java_androidx_webgpu_helper_DawnAhbUtil_importRgbAhbNative(
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
  if (!texture || env->ExceptionCheck()) {
    if (texture) wgpuTextureRelease(texture);
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    return nullptr;
  }

  auto* wrapper = new AhbTextureWrapper{stm, texture, nullptr, nullptr, ahb};

  jobject texture_kt = env->NewObject(classes->texture, env->GetMethodID(classes->texture, "<init>", "(J)V"), reinterpret_cast<jlong>(texture));

  jclass wrapperClz = classes->gpuImportedRgbAhbTexture;
  jmethodID wrapperInit = env->GetMethodID(wrapperClz, "<init>", "(JLandroidx/webgpu/GPUTexture;)V");
  return env->NewObject(wrapperClz, wrapperInit, reinterpret_cast<jlong>(wrapper), texture_kt);
}

JNIEXPORT jobject JNICALL
Java_androidx_webgpu_helper_DawnAhbUtil_importYuvAhbNative(
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
    if (texture) wgpuTextureRelease(texture);
    wgpuSharedTextureMemoryRelease(stm);
    AHardwareBuffer_release(ahb);
    return nullptr;
  }

  WGPUExternalTextureDescriptor extDesc = {};
  extDesc.plane0 = view;

  jclass descClass = env->GetObjectClass(descriptorObj);

  // Helper to call Kotlin getters and prevent local ref table overflow
  auto callObjectMethod = [&](const char* methodName, const char* sig) {
    jmethodID mid = env->GetMethodID(descClass, methodName, sig);
    return mid ? env->CallObjectMethod(descriptorObj, mid) : nullptr;
  };

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

  jobject apparentSizeObj = callObjectMethod("getApparentSize", "()Landroidx/webgpu/GPUExtent2D;");
  if (apparentSizeObj) {
    jclass extentClass = env->GetObjectClass(apparentSizeObj);
    extDesc.apparentSize.width = env->GetIntField(apparentSizeObj, env->GetFieldID(extentClass, "width", "I"));
    extDesc.apparentSize.height = env->GetIntField(apparentSizeObj, env->GetFieldID(extentClass, "height", "I"));
    env->DeleteLocalRef(extentClass);
    env->DeleteLocalRef(apparentSizeObj);
  }
  if (extDesc.apparentSize.width == 0) extDesc.apparentSize = extDesc.cropSize;

  extDesc.mirrored = env->CallBooleanMethod(descriptorObj, env->GetMethodID(descClass, "isMirrored", "()Z"));
  extDesc.doYuvToRgbConversionOnly = env->CallBooleanMethod(descriptorObj, env->GetMethodID(descClass, "isDoYuvToRgbConversionOnly", "()Z"));

  jint rotation = env->CallIntMethod(descriptorObj, env->GetMethodID(descClass, "getRotation", "()I"));
  switch (rotation) {
    case 2: extDesc.rotation = WGPUExternalTextureRotation_Rotate90Degrees; break;
    case 3: extDesc.rotation = WGPUExternalTextureRotation_Rotate180Degrees; break;
    case 4: extDesc.rotation = WGPUExternalTextureRotation_Rotate270Degrees; break;
    default: extDesc.rotation = WGPUExternalTextureRotation_Rotate0Degrees; break;
  }

  auto extractFloatArray = [&](const char* methodName, const float** targetPtr) {
    jfloatArray arr = (jfloatArray) callObjectMethod(methodName, "()[F");
    if (arr && env->GetArrayLength(arr) > 0) {
      *targetPtr = env->GetFloatArrayElements(arr, nullptr);
      return arr;
    }
    if (arr) env->DeleteLocalRef(arr);
    return (jfloatArray) nullptr;
  };

  const float *yuvMatrix = nullptr, *srcTransfer = nullptr, *dstTransfer = nullptr, *gamutMatrix = nullptr;
  jfloatArray yuvArr = extractFloatArray("getYuvToRgbConversionMatrix", &yuvMatrix);
  jfloatArray srcArr = extractFloatArray("getSrcTransferFunctionParameters", &srcTransfer);
  jfloatArray dstArr = extractFloatArray("getDstTransferFunctionParameters", &dstTransfer);
  jfloatArray gamutArr = extractFloatArray("getGamutConversionMatrix", &gamutMatrix);

  extDesc.yuvToRgbConversionMatrix = yuvMatrix;
  extDesc.srcTransferFunctionParameters = srcTransfer;
  extDesc.dstTransferFunctionParameters = dstTransfer;
  extDesc.gamutConversionMatrix = gamutMatrix;

  WGPUExternalTexture externalTexture = wgpuDeviceCreateExternalTexture(device, &extDesc);

  if (yuvArr) { env->ReleaseFloatArrayElements(yuvArr, (jfloat*) yuvMatrix, JNI_ABORT); env->DeleteLocalRef(yuvArr); }
  if (srcArr) { env->ReleaseFloatArrayElements(srcArr, (jfloat*) srcTransfer, JNI_ABORT); env->DeleteLocalRef(srcArr); }
  if (dstArr) { env->ReleaseFloatArrayElements(dstArr, (jfloat*) dstTransfer, JNI_ABORT); env->DeleteLocalRef(dstArr); }
  if (gamutArr) { env->ReleaseFloatArrayElements(gamutArr, (jfloat*) gamutMatrix, JNI_ABORT); env->DeleteLocalRef(gamutArr); }
  env->DeleteLocalRef(descClass);

  auto* wrapper = new AhbTextureWrapper{stm, texture, view, externalTexture, ahb};

  jobject extTex_kt = env->NewObject(classes->externalTexture, env->GetMethodID(classes->externalTexture, "<init>", "(J)V"), reinterpret_cast<jlong>(externalTexture));

  jclass wrapperClz = classes->gpuImportedYuvAhbTexture;
  jmethodID wrapperInit = env->GetMethodID(wrapperClz, "<init>", "(JLandroidx/webgpu/GPUExternalTexture;)V");

  return env->NewObject(wrapperClz, wrapperInit, reinterpret_cast<jlong>(wrapper), extTex_kt);
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUImportedHardwareBuffer_nativeBeginAccess(JNIEnv* env, jobject thiz, jlong handle) {
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  WGPUSharedTextureMemoryBeginAccessDescriptor beginDesc = {};
  beginDesc.concurrentRead = false;
  beginDesc.initialized = true;

  WGPUSharedTextureMemoryVkImageLayoutBeginState beginLayout = {};

  beginLayout.chain.sType = WGPUSType_SharedTextureMemoryVkImageLayoutBeginState;
  beginLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  beginLayout.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  beginDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&beginLayout.chain);

  wgpuSharedTextureMemoryBeginAccess(wrapper->stm, wrapper->texture, &beginDesc);
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUImportedHardwareBuffer_nativeEndAccess(JNIEnv* env, jobject thiz, jlong handle) {
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  WGPUSharedTextureMemoryEndAccessState endState = {};
  WGPUSharedTextureMemoryVkImageLayoutEndState endLayout = {};

  endLayout.chain.sType = WGPUSType_SharedTextureMemoryVkImageLayoutEndState;
  endLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  endLayout.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  endState.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&endLayout.chain);

  wgpuSharedTextureMemoryEndAccess(wrapper->stm, wrapper->texture, &endState);
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUImportedHardwareBuffer_nativeDestroy(JNIEnv* env, jobject thiz, jlong handle) {
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  if (wrapper->externalTexture) wgpuExternalTextureRelease(wrapper->externalTexture);
  if (wrapper->view) wgpuTextureViewRelease(wrapper->view);
  if (wrapper->texture) wgpuTextureRelease(wrapper->texture);
  if (wrapper->stm) wgpuSharedTextureMemoryRelease(wrapper->stm);

  // Release Native NDK HardwareBuffer to prevent leak
  if (wrapper->ahb) AHardwareBuffer_release(wrapper->ahb);

  delete wrapper;
}

} // extern "C"
} // namespace dawn::kotlin_api