#!/bin/bash
#
# Build mpp with remote vpu enabled (USE_REMOTE_VPU=on)
# 
# usage: build.sh {armeabi-v7a|arm64-v8a|x86|x86_64}
#

TARGET_ABI=${1:-"x86_64"}

BUILD_PATH=build-linux

MAKE_PROGRAM=`which make`

echo "--- Build linux in '$BUILD_PATH' ---"
mkdir -p ${BUILD_PATH} && cd ${BUILD_PATH}

cmake -DCMAKE_BUILD_TYPE=Release                      \
      -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}            \
      -DTARGET_ABI=${TARGET_ABI}                      \
      -DHAVE_DRM=OFF                                  \
      -DUSE_REMOTE_VPU=ON                             \
      -DUSE_VPU_NVIDIA=ON                             \
      ..

# ----------------------------------------------------------------------------
# usefull cmake debug flag
# ----------------------------------------------------------------------------      #-DCMAKE_BUILD_TYPE=Debug                                              \
      #-DCMAKE_VERBOSE_MAKEFILE=true                                         \
      #--trace                                                               \
      #--debug-output                                                        \

echo ""
echo "Building in '$BUILD_PATH'..."

#cmake --build . --clean-first -- V=1
cmake --build .




