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
};

extern "C" {

JNIEXPORT jobject JNICALL
Java_androidx_webgpu_helper_DawnAhbUtil_importRgbAhbNative(
    JNIEnv* env, jobject thiz, jobject deviceObj, jobject hardwareBuffer) {

  JNIClasses* classes = JNIClasses::getInstance(env);
  JNIContext c(env);

  jclass memberClass = classes->device;
  jmethodID getHandle = env->GetMethodID(memberClass, "getHandle", "()J");
  auto device =
      reinterpret_cast<WGPUDevice>(env->CallLongMethod(deviceObj, getHandle));
  if (!device) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                  "WGPUDevice handle is null.");
    return nullptr;
  }
  if (env->ExceptionCheck()) {
    return nullptr;
  }
  if (!wgpuDeviceHasFeature(device,
                            WGPUFeatureName_SharedTextureMemoryAHardwareBuffer)) {
    env->ThrowNew(env->FindClass("java/lang/UnsupportedOperationException"),
                  "WebGPU Device lacks WGPUFeatureName_SharedTextureMemoryAHardwareBuffer.");
    return nullptr;
  }
  // --- Extract Native AHB ---
  // Note: AHardwareBuffer_fromHardwareBuffer requires API level 26.
  AHardwareBuffer
      * ahb = AHardwareBuffer_fromHardwareBuffer(env, hardwareBuffer);
  if (!ahb) {
    env->ThrowNew(env->FindClass("java/lang/IllegalStateException"),
                  "Failed to extract native AHardwareBuffer.");
    return nullptr;
  }
  // --- Create SharedTextureMemory ---
  WGPUSharedTextureMemoryAHardwareBufferDescriptor ahbDesc = {};
  ahbDesc.chain.next = nullptr;
  ahbDesc.chain.sType = WGPUSType_SharedTextureMemoryAHardwareBufferDescriptor;
  ahbDesc.handle = ahb;

  WGPUSharedTextureMemoryDescriptor stmDesc = {};
  stmDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&ahbDesc);
  stmDesc.label = {"ZeroCopyCameraAHB_RGB", WGPU_STRLEN};

  WGPUSharedTextureMemory
      stm = wgpuDeviceImportSharedTextureMemory(device, &stmDesc);
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
  texDesc.size =
      {props.size.width, props.size.height, props.size.depthOrArrayLayers};
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
    env->ThrowNew(exceptionClass,
                  "Dawn failed to create a WGPUTexture from the imported AHardwareBuffer.");
    return nullptr;
  }
  // --- Store in our wrapper for begin/end access ---
  auto* wrapper = new AhbTextureWrapper{stm, texture, nullptr, nullptr};

  // --- Wrap WGPUTexture into androidx.webgpu.GPUTexture using cached JNIClasses ---
  jobject texture_kt;
  {
    jclass clz = classes->texture; // Fetches the cached GPUTexture class
    jmethodID init = env->GetMethodID(clz, "<init>", "(J)V");
    texture_kt = env->NewObject(clz, init, reinterpret_cast<jlong>(texture));
  }

  // --- Return our custom Kotlin class holding both the wrapper ptr and the GPUTexture ---
  jclass wrapperClz =
      env->FindClass("androidx/webgpu/helper/GPUImportedRGBHardwareBuffer");
  jmethodID wrapperInit = env->GetMethodID(wrapperClz, "<init>",
                                           "(JLandroidx/webgpu/GPUTexture;)V");
  jobject result_kt = env->NewObject(wrapperClz,
                                     wrapperInit,
                                     reinterpret_cast<jlong>(wrapper),
                                     texture_kt);

  return result_kt;
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUImportedRGBHardwareBuffer_beginAccessNative(
    JNIEnv* env,
    jobject thiz) {
  jclass cls = env->GetObjectClass(thiz);
  jfieldID handleField = env->GetFieldID(cls, "handle", "J");
  jlong handle = env->GetLongField(thiz, handleField);
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  // 1. Define the Vulkan Layout Begin State
  WGPUSharedTextureMemoryVkImageLayoutBeginState beginLayout = {};
  beginLayout.chain.sType =
      WGPUSType_SharedTextureMemoryVkImageLayoutBeginState;
  beginLayout.chain.next = nullptr;
  beginLayout.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  beginLayout.newLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  // 2. Chain it to the BeginAccess descriptor
  WGPUSharedTextureMemoryBeginAccessDescriptor beginDesc = {};
  beginDesc.nextInChain =
      reinterpret_cast<WGPUChainedStruct*>(&beginLayout.chain);
  beginDesc.concurrentRead = false;
  beginDesc.initialized = true;

  wgpuSharedTextureMemoryBeginAccess(wrapper->stm,
                                     wrapper->texture,
                                     &beginDesc);
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUImportedRGBHardwareBuffer_endAccessNative(JNIEnv* env,
                                                                         jobject thiz) {
  jclass cls = env->GetObjectClass(thiz);
  jfieldID handleField = env->GetFieldID(cls, "handle", "J");
  jlong handle = env->GetLongField(thiz, handleField);
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

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
Java_androidx_webgpu_helper_GPUImportedRGBHardwareBuffer_destroyNative(JNIEnv* env,
                                                                       jobject thiz) {
  jclass cls = env->GetObjectClass(thiz);
  jfieldID handleField = env->GetFieldID(cls, "handle", "J");
  jlong handle = env->GetLongField(thiz, handleField);
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);
  // Safely release only the resources that were actually created
  if (wrapper->externalTexture) wgpuExternalTextureRelease(wrapper->externalTexture);
  if (wrapper->view) wgpuTextureViewRelease(wrapper->view);
  if (wrapper->texture) wgpuTextureRelease(wrapper->texture);
  if (wrapper->stm) wgpuSharedTextureMemoryRelease(wrapper->stm);
  delete wrapper;
}

JNIEXPORT jobject JNICALL
Java_androidx_webgpu_helper_DawnAhbUtil_importYuvAhbNative(
    JNIEnv* env,
    jobject thiz,
    jobject deviceObj,
    jobject hardwareBuffer,
    jobject descriptorObj) {

  // 1. Extract Device & AHB
  jclass deviceClass = env->GetObjectClass(deviceObj);
  jmethodID getHandle = env->GetMethodID(deviceClass, "getHandle", "()J");
  WGPUDevice device =
      reinterpret_cast<WGPUDevice>(env->CallLongMethod(deviceObj, getHandle));

  AHardwareBuffer
      * ahb = AHardwareBuffer_fromHardwareBuffer(env, hardwareBuffer);
  if (!ahb) return nullptr;

  // 2. Create SharedTextureMemory
  WGPUSharedTextureMemoryAHardwareBufferDescriptor ahbDesc = {};
  ahbDesc.chain.sType = WGPUSType_SharedTextureMemoryAHardwareBufferDescriptor;
  ahbDesc.handle = ahb;

  WGPUSharedTextureMemoryDescriptor stmDesc = {};
  stmDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&ahbDesc);
  WGPUSharedTextureMemory
      stm = wgpuDeviceImportSharedTextureMemory(device, &stmDesc);

  if (env->ExceptionCheck()) {
    return nullptr; // Instantly return to Kotlin so it can cleanly print the WebGPU error!
  }
  if (!stm) {
    jclass exceptionClass = env->FindClass("java/lang/IllegalStateException");
    env->ThrowNew(exceptionClass, "Dawn failed to import the AHardwareBuffer.");
    return nullptr;
  }
  // -----------------------------

  WGPUSharedTextureMemoryProperties props = {};
  wgpuSharedTextureMemoryGetProperties(stm, &props);
  if (env->ExceptionCheck()) return nullptr;

  if (props.format == WGPUTextureFormat_Undefined) {
    wgpuSharedTextureMemoryRelease(stm);
    env->ThrowNew(env->FindClass("java/lang/UnsupportedOperationException"),
                  "Dawn imported the YUV AHB, but its format maps to WGPUTextureFormat_Undefined. Your Vulkan driver cannot sample this specific video format.");
    return nullptr;
  }
  // 3. Create the underlying Texture & View for Plane 0 inside C++
  WGPUTextureDescriptor texDesc = {};
  texDesc.usage = WGPUTextureUsage_TextureBinding;
  texDesc.dimension = WGPUTextureDimension_2D;
  texDesc.size = {props.size.width, props.size.height, 1};
  texDesc.format = props.format;
  texDesc.mipLevelCount = 1;
  texDesc.sampleCount = 1;

  WGPUTexture texture = wgpuSharedTextureMemoryCreateTexture(stm, &texDesc);
  WGPUTextureView view = wgpuTextureCreateView(texture, nullptr);
  if (env->ExceptionCheck()) return nullptr;
  // 4. Map the Kotlin Descriptor to WGPUExternalTextureDescriptor
  WGPUExternalTextureDescriptor extDesc = {};
  extDesc.plane0 = view; // OVERRIDE the Kotlin dummy with the real view!

  jclass descClass = env->GetObjectClass(descriptorObj);

  // Helper to cleanly call Kotlin getters
  auto callObjectMethod = [&](const char* methodName, const char* sig) {
    jmethodID mid = env->GetMethodID(descClass, methodName, sig);
    return mid ? env->CallObjectMethod(descriptorObj, mid) : nullptr;
  };

  // Extract GPUOrigin2D
  jobject originObj =
      callObjectMethod("getCropOrigin", "()Landroidx/webgpu/GPUOrigin2D;");
  if (originObj) {
    jclass originClass = env->GetObjectClass(originObj);
    extDesc.cropOrigin.x =
        env->GetIntField(originObj, env->GetFieldID(originClass, "x", "I"));
    extDesc.cropOrigin.y =
        env->GetIntField(originObj, env->GetFieldID(originClass, "y", "I"));
  }

  // Extract GPUExtent2D (cropSize)
  jobject cropSizeObj =
      callObjectMethod("getCropSize", "()Landroidx/webgpu/GPUExtent2D;");
  if (cropSizeObj) {
    jclass extentClass = env->GetObjectClass(cropSizeObj);
    extDesc.cropSize.width = env->GetIntField(cropSizeObj,
                                              env->GetFieldID(extentClass,
                                                              "width",
                                                              "I"));
    extDesc.cropSize.height = env->GetIntField(cropSizeObj,
                                               env->GetFieldID(extentClass,
                                                               "height",
                                                               "I"));
  }
  if (extDesc.cropSize.width == 0) extDesc.cropSize.width = props.size.width;
  if (extDesc.cropSize.height == 0) extDesc.cropSize.height = props.size.height;

  // Extract GPUExtent2D (apparentSize)
  jobject apparentSizeObj =
      callObjectMethod("getApparentSize", "()Landroidx/webgpu/GPUExtent2D;");
  if (apparentSizeObj) {
    jclass extentClass = env->GetObjectClass(apparentSizeObj);
    extDesc.apparentSize.width = env->GetIntField(apparentSizeObj,
                                                  env->GetFieldID(extentClass,
                                                                  "width",
                                                                  "I"));
    extDesc.apparentSize.height = env->GetIntField(apparentSizeObj,
                                                   env->GetFieldID(extentClass,
                                                                   "height",
                                                                   "I"));
  }
  if (extDesc.apparentSize.width == 0) extDesc.apparentSize = extDesc.cropSize;

  // Extract Booleans
  extDesc.mirrored = env->CallBooleanMethod(descriptorObj,
                                            env->GetMethodID(descClass,
                                                             "isMirrored",
                                                             "()Z"));
  extDesc.doYuvToRgbConversionOnly = env->CallBooleanMethod(descriptorObj,
                                                            env->GetMethodID(
                                                                descClass,
                                                                "isDoYuvToRgbConversionOnly",
                                                                "()Z"));

  // Extract Rotation Enum Mapping
  jint rotation = env->CallIntMethod(descriptorObj,
                                     env->GetMethodID(descClass,
                                                      "getRotation",
                                                      "()I"));
  switch (rotation) {
    case 2: extDesc.rotation = WGPUExternalTextureRotation_Rotate90Degrees;
      break;
    case 3: extDesc.rotation = WGPUExternalTextureRotation_Rotate180Degrees;
      break;
    case 4: extDesc.rotation = WGPUExternalTextureRotation_Rotate270Degrees;
      break;
    default: extDesc.rotation = WGPUExternalTextureRotation_Rotate0Degrees;
      break;
  }

  // Safely extract FloatArrays (ignoring empty dummy arrays)
  auto
      extractFloatArray = [&](const char* methodName, const float** targetPtr) {
    jfloatArray arr = (jfloatArray) callObjectMethod(methodName, "()[F");
    if (arr != nullptr) {
      if (env->GetArrayLength(arr) > 0) {
        *targetPtr = env->GetFloatArrayElements(arr, nullptr);
        return arr;
      }
    }
    return (jfloatArray) nullptr;
  };

  const float* yuvMatrix = nullptr;
  const float* srcTransfer = nullptr;
  const float* dstTransfer = nullptr;
  const float* gamutMatrix = nullptr;

  jfloatArray
      yuvArr = extractFloatArray("getYuvToRgbConversionMatrix", &yuvMatrix);
  jfloatArray srcArr =
      extractFloatArray("getSrcTransferFunctionParameters", &srcTransfer);
  jfloatArray dstArr =
      extractFloatArray("getDstTransferFunctionParameters", &dstTransfer);
  jfloatArray
      gamutArr = extractFloatArray("getGamutConversionMatrix", &gamutMatrix);

  extDesc.yuvToRgbConversionMatrix = yuvMatrix;
  extDesc.srcTransferFunctionParameters = srcTransfer;
  extDesc.dstTransferFunctionParameters = dstTransfer;
  extDesc.gamutConversionMatrix = gamutMatrix;

  // 5. Create the External Texture
  WGPUExternalTexture
      externalTexture = wgpuDeviceCreateExternalTexture(device, &extDesc);

  // 6. Release JNI Float Arrays
  if (yuvArr)
    env->ReleaseFloatArrayElements(yuvArr,
                                   (jfloat*) yuvMatrix,
                                   JNI_ABORT);
  if (srcArr)
    env->ReleaseFloatArrayElements(srcArr,
                                   (jfloat*) srcTransfer,
                                   JNI_ABORT);
  if (dstArr)
    env->ReleaseFloatArrayElements(dstArr,
                                   (jfloat*) dstTransfer,
                                   JNI_ABORT);
  if (gamutArr)
    env->ReleaseFloatArrayElements(gamutArr,
                                   (jfloat*) gamutMatrix,
                                   JNI_ABORT);

  // 7. Store in Wrapper and Return
  auto* wrapper = new AhbTextureWrapper{stm, texture, view, externalTexture};

  JNIClasses* classes = JNIClasses::getInstance(env);

  jobject extTex_kt;
  {
    jclass clz = classes->externalTexture;
    jmethodID init = env->GetMethodID(clz, "<init>", "(J)V");
    extTex_kt =
        env->NewObject(clz, init, reinterpret_cast<jlong>(externalTexture));
  }

  jclass wrapperClz =
      env->FindClass("androidx/webgpu/helper/GPUImportedYuvHardwareBuffer");
  jmethodID wrapperInit = env->GetMethodID(wrapperClz,
                                           "<init>",
                                           "(JLandroidx/webgpu/GPUExternalTexture;)V");

  jobject result_kt = env->NewObject(wrapperClz,
                                     wrapperInit,
                                     reinterpret_cast<jlong>(wrapper),
                                     extTex_kt);

  return result_kt;
}
// --- Access Management Methods for YUV ---
JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUImportedYuvHardwareBuffer_beginAccessNative(
    JNIEnv* env,
    jobject thiz) {
  jlong handle = env->GetLongField(thiz,
                                   env->GetFieldID(env->GetObjectClass(thiz),
                                                   "handle",
                                                   "J"));
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  WGPUSharedTextureMemoryBeginAccessDescriptor beginDesc = {};
  beginDesc.concurrentRead = false;
  beginDesc.initialized = true;
  wgpuSharedTextureMemoryBeginAccess(wrapper->stm,
                                     wrapper->texture,
                                     &beginDesc);
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUImportedYuvHardwareBuffer_endAccessNative(JNIEnv* env,
                                                                         jobject thiz) {
  jlong handle = env->GetLongField(thiz,
                                   env->GetFieldID(env->GetObjectClass(thiz),
                                                   "handle",
                                                   "J"));
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  WGPUSharedTextureMemoryEndAccessState endState = {};
  wgpuSharedTextureMemoryEndAccess(wrapper->stm, wrapper->texture, &endState);
}

JNIEXPORT void JNICALL
Java_androidx_webgpu_helper_GPUImportedYuvHardwareBuffer_destroyNative(JNIEnv* env,
                                                                       jobject thiz) {
  jlong handle = env->GetLongField(thiz,
                                   env->GetFieldID(env->GetObjectClass(thiz),
                                                   "handle",
                                                   "J"));
  auto* wrapper = reinterpret_cast<AhbTextureWrapper*>(handle);

  if (wrapper->externalTexture) wgpuExternalTextureRelease(wrapper->externalTexture);
  if (wrapper->view) wgpuTextureViewRelease(wrapper->view);
  if (wrapper->texture) wgpuTextureRelease(wrapper->texture);
  if (wrapper->stm) wgpuSharedTextureMemoryRelease(wrapper->stm);

  delete wrapper;
}

} // extern "C"
}
