# vim: syntax=cmake
 
if (USE_SOFT_JPEG)
  add_subdirectory(jpege)
endif ()

if (USE_VPU_NVIDIA AND NOT ANDROID)
  add_subdirectory(nvidia)
endif ()

if (USE_SOFT_X264)
  add_subdirectory(x264e)
endif ()

# soft or remote vpu needs stubs
if (USE_REMOTE_VPU OR USE_VPU_NVIDIA OR USE_SOFT_X264)
  add_subdirectory(h264e-stub)
endif ()