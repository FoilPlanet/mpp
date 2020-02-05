# vim: syntax=cmake

# software hal or remote stubs
if (ANDROID)
add_subdirectory(x264e)
add_subdirectory(jpege)
endif()

if (USE_REMOTE_VPU)
add_subdirectory(h264e-stub)
add_subdirectory(nvidia)
endif()
