# vim: syntax=cmake
include_directories(.)
include_directories(./include)
include_directories(../../common/h264/)
include_directories(../../rkdec/h264d/)

if (ANDROID)
  set(TARGET_ABI ${ANDROID_ABI})
endif ()

if (NOT CUDA_PATH)
  set(CUDA_PATH /usr/lib/${TARGET_ABI}-linux-gnu)
endif ()

# hal nvidia header
set(HAL_NVIDIA_HDR
    hal_h264d_nv.h
    hal_h264e_nv.h
    )

# hal nvidia encoder sourse
set(HAL_NVIDIA_SRC
    hal_h264d_nv.cc
    hal_h264e_nv.cc
    NvEncoder.cpp
    NvEncoderCuda.cpp
    NvEncoderOutputInVidMemCuda.cpp
    )

set(NVIDIA_LIBS
    ${CUDA_PATH}/libcuda.so
#   ${CMAKE_CURRENT_SOURCE_DIR}/libs/${TARGET_ABI}-linux/libnvcuvid.so
    ${CMAKE_CURRENT_SOURCE_DIR}/libs/${TARGET_ABI}-linux/libnvidia-encode.so
    )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fpermissive -Wno-missing-field-initializers -Wno-unused-parameter")

add_library(hal_h264_nv STATIC ${HAL_NVIDIA_HDR} ${HAL_NVIDIA_SRC})

# NOTE: nvidia hal requires remote stub for using in android
if (ANDROID)
  add_definitions(-D_VPU_STUB_)
endif ()

target_link_libraries(hal_h264_nv ${NVIDIA_LIBS} hal_h264d hal_h264e)
set_target_properties(hal_h264_nv PROPERTIES FOLDER "mpp/hal")
