# vim: syntax=cmake
include_directories(.)
include_directories(../../common/h264/)
include_directories(../../rkdec/h264d/)

if (ANDROID)
  set(TARGET_ABI ${ANDROID_ABI})
endif ()

# hal h264 header
set(HAL_X264_HDR
    hal_h264e_x264.h
    )

# hal h264 encoder sourse
set(HAL_X264_SRC
    hal_h264e_x264.c
    )

add_library(hal_h264e_x264 STATIC
            ${HAL_X264_HDR}
            ${HAL_X264_SRC}
            )

# add path for libx264 ${TARGET_ABI}
include_directories(./libx264/inc)

# cmake-3.16
# target_link_directories(./libx264/lib/${TARGET_ABI})
# target_link_libraries(hal_h264e_x264 x264 hal_h264e)

set(X264E_DEPS dl
    ${CMAKE_CURRENT_SOURCE_DIR}/libx264/lib/${TARGET_ABI}/libx264.a
    )

target_link_libraries(hal_h264e_x264 ${X264E_DEPS} hal_h264e)
set_target_properties(hal_h264e_x264 PROPERTIES FOLDER "mpp/hal")
