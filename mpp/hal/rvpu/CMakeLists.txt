# vim: syntax=cmake
 
if (USE_SOFT_JPEG)
  add_subdirectory(jpege)
endif ()

if (USE_VPU_NVIDIA AND NOT ANDROID)
  add_subdirectory(nvidia)
endif ()

if (USE_SOFT_X264)
  add_subdirectory(x264)
endif ()

# soft or remote vpu needs stubs
if (USE_REMOTE_VPU OR USE_VPU_NVIDIA OR USE_SOFT_X264)
  add_subdirectory(h264-stub)
endif ()

#
# hal_rvpu.a
#
include_directories(.)
include_directories(../common/h264/)

if (USE_REMOTE_VPU)
  add_definitions(-D_VPU_STUB_)
endif ()

# rvpu-stub header
set(HAL_RVPU_SRC rvpu_primitive.c)
set(HAL_RVPU_HDR rvpu_primitive.h)

add_library(hal_rvpu STATIC
    ${HAL_RVPU_HDR}
    ${HAL_RVPU_SRC}
    )

target_link_libraries(hal_rvpu ${RVPU_STUB_DEPS})
set_target_properties(hal_rvpu PROPERTIES FOLDER "mpp/hal")
