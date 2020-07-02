#!/bin/bash
#
# Build mpp with remote vpu enabled (USE_REMOTE_VPU=on)
# 
# usage: build.sh {armeabi-v7a|arm64-v8a|x86|x86_64}
#

HOST_ABI=$(uname -m)
TARGET_ABI=${1:-"x86_64"}
BUILD_PATH=build-linux-${TARGET_ABI}
VERSION=20190904_1.00

INSTALL_PREFIX=../../..

MAKE_PROGRAM=`which make`

case $HOST_ABI in
  x86_64)
    CROSS_COMPILER_FLAGS="-DCMAKE_SYSTEM_NAME=Linux       \
        -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc          \
        -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++"
    ;;
  *)
    CROSS_COMPILER_FLAGS=""
    ;;
esac

case $TARGET_ABI in
  arm64-v8a)
    echo "--- Build linux in '$BUILD_PATH' ---"
    echo "NOT IMPLEMENTED"
    exit -1
    ;;
  x86 | x86_64)
    echo "--- Build linux in '$BUILD_PATH' ---"
    mkdir -p ${BUILD_PATH} && cd ${BUILD_PATH}
    VPU_FLAGS="-DUSE_SOFT_X264=OFF -DUSE_VPU_NVIDIA=ON"
    cmake -DCMAKE_BUILD_TYPE=Release                      \
          -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}            \
          -DTARGET_ABI=${TARGET_ABI}                      \
          -DHAVE_DRM=OFF                                  \
          ${VPU_FLAGS}                                    \
          ..
    ;;
  install)
    INSTALL_PATH=${INSTALL_PREFIX}/libs/mpp-${VERSION}
    mkdir -p ${INSTALL_PATH}/linux/x86_64/lib.nvidia
    mkdir -p ${INSTALL_PATH}/include
    cp -a  ./inc/* ${INSTALL_PATH}/include
    cp -av ./build-linux-x86_64/mpp/lib*    ${INSTALL_PATH}/linux/x86_64/lib.nvidia
    exit 0
    ;; 
  *)
    echo "Unknown TARGET type $TARGET_ABI"
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




