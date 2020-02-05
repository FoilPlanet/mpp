# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)

# hal h264 header
set(HAL_H264E_HDR
    hal_h264e_x264.h
    )

# hal h264 encoder sourse
set(HAL_H264E_SRC
    hal_h264e_x264.c
    )

add_library(hal_h264e_x264 STATIC
            ${HAL_H264E_HDR}
            ${HAL_H264E_SRC}
            )

# add path for libx264 ${ANDROID_ABI}
include_directories(./libx264/inc)

# cmake-3.16
# target_link_directories(./libx264/lib/${ANDROID_ABI})
# target_link_libraries(hal_h264e_x264 x264 hal_h264e)

target_link_libraries(hal_h264e_x264 ${CMAKE_CURRENT_SOURCE_DIR}/libx264/lib/${ANDROID_ABI}/libx264.a hal_h264e)
set_target_properties(hal_h264e_x264 PROPERTIES FOLDER "mpp/hal")
