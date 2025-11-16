// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <jni.h>
#include <vector>
#include <string>
#include <android/log.h>
#include <iostream>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/task/single_thread_task_executor.h"
#include "base/test/launcher/unit_test_launcher.h"
#include "base/test/test_suite.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
// Custom stream buffer to redirect C++ output streams (cout, cerr) to Android Logcat.
class LogcatStreamBuffer : public std::streambuf {
 public:
  // Constructor takes the Log Priority (e.g., ANDROID_LOG_INFO or ANDROID_LOG_ERROR)
  LogcatStreamBuffer(android_LogPriority priority) : mPriority(priority) {}

 protected:
  // Override overflow to handle characters being written to the stream
  virtual int overflow(int c) override {
    if (c != EOF) {
      mBuffer += (char)c;
    }
    // If we hit a newline or stream ends, flush the accumulated buffer to Logcat
    if (c == '\n' || c == EOF) {
      flushBuffer();
    }
    return c;
  }

  // Override sync to manually flush the buffer if the stream is explicitly flushed
  virtual int sync() override {
    return flushBuffer();
  }

 private:
  android_LogPriority mPriority;
  std::string mBuffer;
  const char* kLogTag = "DawnStd"; // Custom tag for GTest output in Logcat

  int flushBuffer() {
    if (!mBuffer.empty()) {
      // CRITICAL STEP: Call the native Android logging function for the buffered line.
      __android_log_print(mPriority, kLogTag, "%s", mBuffer.c_str());
      mBuffer.clear();
    }
    return 0; // Success
  }
};
}

// Definition located in third_party/dawn/src/tests/DawnTest.h
// Forward declared here to avoid pulling in the Dawn headers.
void InitDawnEnd2EndTestEnvironment(int argc, char** argv);


// --- Internal C++ Implementation Function ---
// This function contains the original JNI logic. It is no longer marked JNIEXPORT
// because it is registered dynamically via JNI_OnLoad.
jint RunEnd2EndTestsImpl(JNIEnv* env, jclass clazz, jobjectArray jArgs) {
    LogcatStreamBuffer g_cout_buffer(ANDROID_LOG_INFO);
    LogcatStreamBuffer g_cerr_buffer(ANDROID_LOG_ERROR);
    [[maybe_unused]]
    std::streambuf* g_old_cout_buffer = std::cout.rdbuf(&g_cout_buffer);
    [[maybe_unused]]
    std::streambuf* g_old_cerr_buffer = std::cerr.rdbuf(&g_cerr_buffer);

    // Get the number of arguments (this will be argc for the C++ main functions)
    int argc = 0;
    if (jArgs != nullptr) {
        argc = env->GetArrayLength(jArgs);
    }

    // Since the C++ functions (like InitGoogleTest) expect an argc >= 1
    // (with argv[0] being the program name), we add a dummy program name
    // if no arguments are passed.
    if (argc == 0) {
        // Only the program name will be in the vector
        argc = 1;
    }

    std::vector<std::string> args_vec;
    args_vec.reserve(argc);

    // If no arguments were passed, use a dummy program name.
    if (jArgs == nullptr || env->GetArrayLength(jArgs) == 0) {
        args_vec.push_back("DawnJniRunner");
    }

    // Iterate through the Java String array and convert to std::string
    for (int i = 0; i < env->GetArrayLength(jArgs); ++i) {
        jstring jStr = (jstring)env->GetObjectArrayElement(jArgs, i);

        // Convert jstring to const char*
        const char* cStr = env->GetStringUTFChars(jStr, nullptr);
        if (cStr == nullptr) {
            // Handle OutOfMemoryError or other critical error during conversion
            return -1;
        }

        // Store as std::string
        args_vec.push_back(cStr);

        // Release the native string memory immediately
        env->ReleaseStringUTFChars(jStr, cStr);
        env->DeleteLocalRef(jStr);
    }
    
    // Check if the first element is the program name (required by Google Test)
    if (args_vec.empty()) {
        args_vec.push_back("DawnJniRunner");
        argc = 1;
    } else {
        argc = args_vec.size();
    }


    // --- 2. Create the char** (argv) array from the vector of strings ---

    // The C++ main functions expect a char** argv. We use a vector of char*
    // for safe memory management, with the elements pointing to the internal
    // data of the std::string objects in args_vec.

    std::vector<char*> argv_vec;
    argv_vec.reserve(argc + 1); // +1 for the mandatory nullptr terminator

    for (std::string& arg : args_vec) {
        // const_cast is necessary because InitGoogleTest and other main-like
        // functions often take non-const char** and modify the contents/pointers.
        argv_vec.push_back(const_cast<char*>(arg.c_str()));
    }

    // Add the required nullptr terminator
    argv_vec.push_back(nullptr);

    // Get the raw pointer to the start of the argv array
    char** argv = argv_vec.data();

    base::CommandLine::Init(argc, argv);

    // Environment initialization is done before test initialization
    // so tests can be instantiated for each available GPU adapter.
    InitDawnEnd2EndTestEnvironment(argc, argv);

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

// --- Dynamic Registration Setup ---

// 1. Define the JNI mapping structure
static const JNINativeMethod kNativeMethods[] = {
    // {Java Method Name, Signature, Native Function Pointer}
    // Java Method Name is "end2endTests"
    // Signature is "([Ljava/lang/String;)I" (String array input, Integer return)
    {"end2endTests", "([Ljava/lang/String;)I", (void*)RunEnd2EndTestsImpl},
};

// 2. Define the Java class path where the native method is declared
// Inferred from the original function name: Java_com_example_dawnandroid_MainActivity_end2endTests
static const char* const kClassPath = "com/example/dawnandroid/MainActivity";

// --- The Exported JNI Entry Point ---
// JNI_OnLoad is the function that the linker must expose. It performs the dynamic registration.
extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = nullptr;

    // 1. Get JNIEnv for the current thread/process
    if (vm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // JNI version not supported
    }

    // 2. Find the Java class
    jclass clazz = env->FindClass(kClassPath);
    if (clazz == nullptr) {
        // Failed to find the target class. An exception is pending.
        return JNI_ERR;
    }

    // 3. Register the native methods
    int result = env->RegisterNatives(clazz, kNativeMethods,
                                     sizeof(kNativeMethods) / sizeof(kNativeMethods[0]));

    if (result < 0) {
        // Registration failed. An exception is pending.
        return JNI_ERR;
    }

    // Return the required JNI version
    return JNI_VERSION_1_6;
}