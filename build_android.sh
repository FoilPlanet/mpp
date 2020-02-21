#!/bin/bash
#
# usage: build.sh {armeabi-v7a|arm64-v8a|x86|x86_64}
#
# requires: NDK-r17+
#

DEFAULT_NDK=/build/android-ndk-r17b
ANDROID_NDK=${ANDROID_NDK_HOME:-$DEFAULT_NDK}

ANDROID_ABI=${1:-"arm64-v8a"}
API_LEVEL=android-24
BUILD_PATH=build-${ANDROID_ABI}

INSTALL_PREFIX=../minicap/jni/vendor/mpp-wrapper

MAKE_PROGRAM=`which make`

case $ANDROID_ABI in
  armeabi-v7a)
    echo "--- Build $ANDROID_ABI in '$BUILD_PATH' ---"
    mkdir -p ${BUILD_PATH} && cd ${BUILD_PATH}
    PLATFORM=$ANDROID_NDK/platforms/${API_LEVEL}/arch-arm
    cmake -DCMAKE_TOOLCHAIN_FILE=../build/android/android.toolchain.cmake     \
        -DCMAKE_BUILD_TYPE=Release                                            \
        -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}                                  \
        -DANDROID_FORCE_ARM_BUILD=ON                                          \
        -DANDROID_NDK=${ANDROID_NDK}                                          \
        -DANDROID_SYSROOT=${PLATFORM}                                         \
        -DANDROID_ABI="${ANDROID_ABI} with NEON"                              \
        -DANDROID_TOOLCHAIN_NAME="arm-linux-androideabi-4.9"                  \
        -DANDROID_NATIVE_API_LEVEL=${API_LEVEL}                               \
        -DANDROID_STL=system                                                  \
        -DRKPLATFORM=ON                                                       \
        -DHAVE_DRM=ON                                                         \
        ..
    ;;
  arm64-v8a)
    echo "--- Build $ANDROID_ABI in '$BUILD_PATH' ---"
    mkdir -p ${BUILD_PATH} && cd ${BUILD_PATH}
    PLATFORM=$ANDROID_NDK/platforms/${API_LEVEL}/arch-arm64
    VPU_FLAGS=""
    cmake -DCMAKE_TOOLCHAIN_FILE=../build/android/android.toolchain.cmake     \
        -DCMAKE_BUILD_TYPE=Release                                            \
        -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}                                  \
        -DANDROID_FORCE_ARM_BUILD=ON                                          \
        -DANDROID_NDK=${ANDROID_NDK}                                          \
        -DANDROID_SYSROOT=${PLATFORM}                                         \
        -DANDROID_ABI="${ANDROID_ABI}"                                        \
        -DANDROID_TOOLCHAIN_NAME="aarch64-linux-android-4.9"                  \
        -DANDROID_NATIVE_API_LEVEL=${API_LEVEL}                               \
        -DANDROID_STL=system                                                  \
        -DRKPLATFORM=ON                                                       \
        -DHAVE_DRM=ON                                                         \
        ${VPU_FLAGS}                                                          \
        ..
    ;;
  x86)
    echo "--- Build $ANDROID_ABI in '$BUILD_PATH' ---"
    mkdir -p ${BUILD_PATH} && cd ${BUILD_PATH}
    PLATFORM=$ANDROID_NDK/platforms/${API_LEVEL}/arch-x86
    cmake -DCMAKE_TOOLCHAIN_FILE=../build/android/android.toolchain.cmake     \
        -DCMAKE_BUILD_TYPE=Release                                            \
        -DANDROID_FORCE_ARM_BUILD=ON                                          \
        -DANDROID_NDK=${ANDROID_NDK}                                          \
        -DANDROID_SYSROOT=${PLATFORM}                                         \
        -DANDROID_ABI="${ANDROID_ABI}"                                        \
        -DANDROID_TOOLCHAIN_NAME="x86-4.9"                                    \
        -DANDROID_NATIVE_API_LEVEL=${API_LEVEL}                               \
        -DANDROID_STL=system                                                  \
        -DUSE_REMOTE_VPU=ON                                                   \
        ..
    ;;
  x86_64)
    echo "--- Build $ANDROID_ABI in '$BUILD_PATH' ---"
    mkdir -p ${BUILD_PATH} && cd ${BUILD_PATH}
    PLATFORM=$ANDROID_NDK/platforms/${API_LEVEL}/arch-x86_64
    # VPU_FLAGS="-DUSE_REMOTE_VPU=ON -DUSE_SOFT_X264=ON"
    VPU_FLAGS="-DUSE_REMOTE_VPU=ON"
    cmake -DCMAKE_TOOLCHAIN_FILE=../build/android/android.toolchain.cmake     \
        -DCMAKE_BUILD_TYPE=Release                                            \
        -DANDROID_NDK=${ANDROID_NDK}                                          \
        -DANDROID_SYSROOT=${PLATFORM}                                         \
        -DANDROID_ABI="${ANDROID_ABI}"                                        \
        -DANDROID_TOOLCHAIN_NAME="x86_64-4.9"                                 \
        -DANDROID_NATIVE_API_LEVEL=${API_LEVEL}                               \
        -DANDROID_STL=system                                                  \
        ${VPU_FLAGS}                                                          \
        ..
    ;;
  install)
    INSTALL_PATH=$INSTALL_PREFIX
    cp -av ./build-x86_64/mpp/libmpp*    ${INSTALL_PATH}/libs/x86_64
    cp -av ./build-arm64-v8a/mpp/libmpp* ${INSTALL_PATH}/libs/arm64-v8a
    exit 0
    ;; 
  *)
    echo "Unknown ABI type $ANDROID_ABI"
    exit 1
    ;;
esac

# ----------------------------------------------------------------------------
# usefull cmake debug flag
# ----------------------------------------------------------------------------
      #-DCMAKE_BUILD_TYPE=Debug                                              \
      #-DCMAKE_VERBOSE_MAKEFILE=true                                         \
      #--trace                                                               \
      #--debug-output                                                        \

echo ""
echo "Building in '$BUILD_PATH'..."

#cmake --build . --clean-first -- V=1
cmake --build .

# ----------------------------------------------------------------------------
# test script
# ----------------------------------------------------------------------------
#adb push osal/test/rk_log_test /system/bin/
#adb push osal/test/rk_thread_test /system/bin/
#adb shell sync
#adb shell logcat -c
#adb shell rk_log_test
#adb shell rk_thread_test
#adb logcat -d|tail -30



