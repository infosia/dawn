// AUTO-GENERATED FILE. DO NOT EDIT.
#include <dawn/dawn_proc.h>
#include <dawn/dawn_proc_table.h>
#include <string.h>
#include <android/log.h>
#define LOG_TAG "WebViewDawnProcInitializer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include <mutex>
#include <unordered_set>
#include <jni.h>
std::unordered_set<int> g_SupportedConditionalFeatures;
std::mutex g_featuresMutex;

bool isConditionalFeatureSupported(int feature_id) {
    std::lock_guard<std::mutex> lock(g_featuresMutex);
    return g_SupportedConditionalFeatures.count(feature_id) > 0;
}

void InitializeDawnProcsFromWebView(WGPUProcGetProcAddress getProcAddress, JNIEnv* env, jintArray requiredFeatures);

void InitializeDawnProcsFromWebView(WGPUProcGetProcAddress getProcAddress, JNIEnv* env, jintArray requiredFeatures) {
    LOGI("InitializeDawnProcsFromWebView called. getProcAddress = %p", getProcAddress);
    std::lock_guard<std::mutex> lock(g_featuresMutex);
    g_SupportedConditionalFeatures.clear();
    if (!getProcAddress) {
        LOGE("getProcAddress is NULL. Assuming bundled Dawn mode for tests.");
        if (requiredFeatures != nullptr) {
            jsize len = env->GetArrayLength(requiredFeatures);
            jint* features = env->GetIntArrayElements(requiredFeatures, 0);
            for (jsize i = 0; i < len; ++i) {
                g_SupportedConditionalFeatures.insert(features[i]);
            }
            env->ReleaseIntArrayElements(requiredFeatures, features, JNI_ABORT);
        }
        return;
    }

    std::unordered_set<int> requiredFeatureSet;
    if (requiredFeatures != nullptr) {
        jsize len = env->GetArrayLength(requiredFeatures);
        jint* features = env->GetIntArrayElements(requiredFeatures, 0);
        for (jsize i = 0; i < len; ++i) {
            requiredFeatureSet.insert(features[i]);
        }
        env->ReleaseIntArrayElements(requiredFeatures, features, JNI_ABORT);
    }

    DawnProcTable newProcs = {};
    const uint8_t* libVersion = dawnProcGetVersion();
    memcpy(newProcs.version, libVersion, 20);
    LOGI("Copied libDawn version: %02x%02x%02x%02x", libVersion[0], libVersion[1], libVersion[2], libVersion[3]);
    newProcs.getProcAddress = getProcAddress;

    jclass exceptionClass = env->FindClass("androidx/webgpu/WebGpuRuntimeException");

    {% for function in by_category["function"] %}
        {% if function.name.get() != "get proc address" %}
            newProcs.{{as_varName(function.name)}} = reinterpret_cast<{{as_cProc(None, function.name)}}>(getProcAddress({"{{as_cMethod(None, function.name)}}", sizeof("{{as_cMethod(None, function.name)}}") - 1}));
            {% if function.is_core %}
            if (!newProcs.{{as_varName(function.name)}}) {
                LOGE("Missing core API: {{as_cMethod(None, function.name)}}");
                if (exceptionClass) env->ThrowNew(exceptionClass, "Missing core WebGPU API: {{as_cMethod(None, function.name)}}");
                return;
            }
            {% else %}
            if (!newProcs.{{as_varName(function.name)}} && requiredFeatureSet.count({{function.conditional_feature_id}})) {
                LOGE("Missing required conditional API: {{as_cMethod(None, function.name)}}");
                if (exceptionClass) env->ThrowNew(exceptionClass, "Missing required conditional WebGPU API: {{as_cMethod(None, function.name)}}");
                return;
            }
            if (newProcs.{{as_varName(function.name)}}) {
                g_SupportedConditionalFeatures.insert({{function.conditional_feature_id}});
            }
            {% endif %}
        {% endif %}
    {% endfor %}

    {% for (type, methods) in c_methods_sorted_by_parent %}
        {% for method in methods %}
            newProcs.{{as_varName(type.name, method.name)}} = reinterpret_cast<{{as_cProc(type.name, method.name)}}>(getProcAddress({"{{as_cMethod(type.name, method.name)}}", sizeof("{{as_cMethod(type.name, method.name)}}") - 1}));
            {% if method.is_core %}
            if (!newProcs.{{as_varName(type.name, method.name)}}) {
                LOGE("Missing core API: {{as_cMethod(type.name, method.name)}}");
                if (exceptionClass) env->ThrowNew(exceptionClass, "Missing core WebGPU API: {{as_cMethod(type.name, method.name)}}");
                return;
            }
            {% else %}
            if (!newProcs.{{as_varName(type.name, method.name)}} && requiredFeatureSet.count({{method.conditional_feature_id}})) {
                LOGE("Missing required conditional API: {{as_cMethod(type.name, method.name)}}");
                if (exceptionClass) env->ThrowNew(exceptionClass, "Missing required conditional WebGPU API: {{as_cMethod(type.name, method.name)}}");
                return;
            }
            if (newProcs.{{as_varName(type.name, method.name)}}) {
                g_SupportedConditionalFeatures.insert({{method.conditional_feature_id}});
            }
            {% endif %}
        {% endfor %}
    {% endfor %}

    dawnProcSetProcs(&newProcs);
}
