# Running Dawn tests on Android with CMake

This document describes how to build and run `dawn_end2end_tests` on Android from a standalone Dawn checkout using CMake.

## Get the Android NDK
Download a recent version of the Android NDK and unpack it somewhere on your system.

Subsequent steps will assume it is in `${ANDROID_NDK_DIR}`.

## Set up a new CMake build using the Android NDK
Create a new directory for your CMake+Android build:

```sh
mkdir build-cmake-android
```

Configure CMake using the Android NDK Toolchain:

```sh
cmake . -B build-cmake-android -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK_DIR}/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=29 \
  -DDAWN_BUILD_TESTS=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DDAWN_BUILD_MONOLITHIC_LIBRARY=SHARED
```

Build the tests:

```sh
ninja -C build-cmake-android dawn_end2end_tests
```

## Push the tests to the device and run them
If you have more than one device, you can select one for all future ADB commands by setting the `ANDROID_SERIAL` environment variable:

```sh
adb devices -l   # List all devices to find the serial number
export ANDROID_SERIAL=########    # Insert the serial number from the previous step
```

Push the tests to a writable directory on the device:

```sh
adb push build-cmake-android/dawn_end2end_tests /data/local/tmp/
```

Run the tests:

```sh
adb shell /data/local/tmp/dawn_end2end_tests
adb shell /data/local/tmp/dawn_end2end_tests --backend=vulkan --gtest_filter=ShaderTests*    # Arguments work too
```

**NOTE:** Some output (e.g. the header showing the device information) does not go to stdout, so you have to get it from logcat:

```sh
adb logcat Dawn:I *:S
```

## Other tips
You can set up some aliases to combine these steps, and also avoid pushing the test binary if it hasn't changed.

Create an `env.sh` file:

```sh
function adb_push() {
    loc=$(md5sum "build-cmake-android/$1" | awk '{print $1}')
    dev=$(adb shell md5sum "/data/local/tmp/$1" | awk '{print $1}')
    if [ "$loc" != "$dev" ]; then
        adb push "build-cmake-android/$1" "/data/local/tmp/$1"
    fi
}

alias pixel9='export ANDROID_SERIAL=########'
alias dawn-e2e-rel-android='ninja -C build-cmake-android dawn_end2end_tests \
    && adb_push dawn_end2end_tests \
    && adb shell /data/local/tmp/dawn_end2end_tests'
```

Then you can just run:

```sh
source env.sh
pixel9
dawn-e2e-rel-android --gtest_filter=ShaderTests*
```
