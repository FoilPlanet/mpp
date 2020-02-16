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

if (USE_VPU_NVIDIA)
  include_directories(../nvidia)
  add_definitions(-DUSE_VPU_NVIDIA)
  set(H264E_STUB_DEPS hal_h264e_nv)
endif ()

if (USE_SOFT_X264)
  include_directories(../x264e)
  add_definitions(-DUSE_SOFT_VPU_X264)
  set(H264E_STUB_DEPS hal_h264e_x264)
endif ()

if (USE_REMOTE_VPU)
  add_definitions(-D_VPU_STUB_)
endif ()

target_link_libraries(hal_h264e_stub ${H264E_STUB_DEPS} hal_h264e)
set_target_properties(hal_h264e_stub PROPERTIES FOLDER "mpp/hal")
