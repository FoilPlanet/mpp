# vim: syntax=cmake
include_directories(.)
include_directories(..)
include_directories(../../common/h264/)
include_directories(../../rkdec/h264d/)

# hal h264 header
set(HAL_H264_STUB_HDR
    hal_h264d_stub.h
    hal_h264e_stub.h
    )

# hal h264 encoder sourse
set(HAL_H264_STUB_SRC
    hal_h264d_stub.c
    hal_h264e_stub.c
    )

add_library(hal_h264_stub STATIC
    ${HAL_H264_STUB_HDR}
    ${HAL_H264_STUB_SRC}
    )

if (USE_VPU_NVIDIA)
  include_directories(../nvidia)
  add_definitions(-DUSE_VPU_NVIDIA)
  set(H264_STUB_DEPS hal_h264_nv)
endif ()

if (USE_SOFT_X264)
  include_directories(../x264)
  add_definitions(-DUSE_SOFT_VPU_X264)
  set(H264_STUB_DEPS hal_h264e_x264)
endif ()

if (USE_REMOTE_VPU)
  add_definitions(-D_VPU_STUB_)
endif ()

target_link_libraries(hal_h264_stub ${H264_STUB_DEPS} hal_h264e hal_h264d)
set_target_properties(hal_h264_stub PROPERTIES FOLDER "mpp/hal")
