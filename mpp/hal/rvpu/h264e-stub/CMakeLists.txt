# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)

# hal h264 header
set(HAL_H264E_HDR
    hal_h264e_stub.h
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_stub.c
    )

add_library(hal_h264e_stub STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
            )

# remote encoder - nvenc
if (USE_REMOTE_VPU_NVIDIA)
include_directories(../nvidia)
add_definitions(-DUSE_REMOTE_VPU_NVIDIA)
endif ()

# software encoder - x264
if (ANDROID)
include_directories(../x264e)
add_definitions(-DUSE_SOFT_VPU_X264)
target_link_libraries(hal_h264e_stub hal_h264e_nv hal_h264e_x264 hal_h264e)
else ()
target_link_libraries(hal_h264e_stub hal_h264e_nv hal_h264e)
endif ()

set_target_properties(hal_h264e_stub PROPERTIES FOLDER "mpp/hal")
