# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)

# hal h264 header
set(HAL_H264E_HDR
    hal_h264e_nv.h
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_nv.c
    )

add_library(hal_h264e_nv STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
            )

# NOTE: nvidia hal requires remote stub for using in android
if (ANDROID)
add_definitions(-D_VPU_STUB_)
endif ()

target_link_libraries(hal_h264e_nv hal_h264e)
set_target_properties(hal_h264e_nv PROPERTIES FOLDER "mpp/hal")
