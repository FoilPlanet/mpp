# vim: syntax=cmake

# hal h264 header
set(HAL_H264D_HDR
        hal_h264d_global.h
    )

# hal h264 decoder sourse
set(HAL_H264D_SRC
        hal_h264d_api.c
        hal_h264d_rkv_reg.c
        hal_h264d_vdpu2.c
        hal_h264d_vdpu1.c
        hal_h264d_common.c
    )

add_library(hal_h264d STATIC
            ${HAL_H264D_HDR}
            ${HAL_H264D_SRC}
            )

# remote vpu
if (USE_VPU_NVIDIA)
  add_definitions(-DUSE_VPU_NVIDIA)
endif ()

if (USE_SOFT_X264)
  add_definitions(-DUSE_SOFT_X264)
endif ()

if (USE_REMOTE_VPU OR USE_VPU_NVIDIA OR USE_SOFT_X264)
  include_directories(../../rvpu/h264-stub/)
  add_definitions(-DUSE_REMOTE_VPU)
  set(HAL_H264D_LIBS hal_h264_stub)
else()
  set(HAL_H264D_LIBS )
endif()

target_link_libraries(hal_h264d ${HAL_H264D_LIBS} mpp_base mpp_hal)
set_target_properties(hal_h264d PROPERTIES FOLDER "mpp/hal")

