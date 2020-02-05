# vim: syntax=cmake
# hal jpeg reg

set(HAL_JPEGE_HDR
    )

set(HAL_JPEGE_SRC
    hal_jpege_hdr.c
    hal_jpege_api.c
    hal_jpege_vepu1.c
    hal_jpege_vepu2.c
    )

add_library(${HAL_JPEGE} STATIC
    ${HAL_JPEGE_SRC}
    ${HAL_JPEGE_HDR}
    )

# remote vpu or software codec
if (USE_REMOTE_VPU AND ANDROID)
include_directories(../../rvpu/jpege/)
add_definitions(-DUSE_REMOTE_VPU)
target_link_libraries(${HAL_JPEGE} hal_jpege_stub mpp_base)
else()
target_link_libraries(${HAL_JPEGE} mpp_base)
endif()

set_target_properties(${HAL_JPEGE} PROPERTIES FOLDER "mpp/hal")
