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
if (USE_SOFT_JPEG)
  include_directories(../../rvpu/jpege/)
  add_definitions(-DUSE_SOFT_JPEG)
  set(HAL_JPEGE_LIBS hal_jpege_stub)
endif()

target_link_libraries(${HAL_JPEGE} ${HAL_JPEGE_LIBS} mpp_base)
set_target_properties(${HAL_JPEGE} PROPERTIES FOLDER "mpp/hal")
