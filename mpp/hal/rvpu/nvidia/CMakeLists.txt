# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)
include_directories(./include)

# hal h264 header
set(HAL_H264E_HDR
    hal_h264e_nv.h
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_nv.cc
    NvEncoder.cpp
    NvEncoderOutputInVidMemCuda.cpp
    )

add_library(hal_h264e_nv STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
            )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fpermissive -Wno-missing-field-initializers -Wno-unused-parameter")

# NOTE: nvidia hal requires remote stub for using in android
if (ANDROID)
add_definitions(-D_VPU_STUB_)
endif ()

target_link_libraries(hal_h264e_nv hal_h264e)
set_target_properties(hal_h264e_nv PROPERTIES FOLDER "mpp/hal")
