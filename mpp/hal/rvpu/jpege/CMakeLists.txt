# vim: syntax=cmake
# hal jpeg reg

if (ANDROID)
  set(TARGET_ABI ${ANDROID_ABI})
endif ()

set(HAL_JPEGE_STUB_HDR
    hal_jpege_stub.h
    )

set(HAL_JPEGE_STUB_SRC
    hal_jpege_stub.c
    )

set(HAL_JPEGE_STUB_LIBS
    ${CMAKE_CURRENT_SOURCE_DIR}/libjpeg-turbo/lib/${TARGET_ABI}/libjpeg-turbo.a
    )

add_library(hal_jpege_stub STATIC
    ${HAL_JPEGE_STUB_SRC}
    ${HAL_JPEGE_STUB_HDR}
    )

# add path for libjpeg-turbo ${TARGET_ABI}
include_directories(./libjpeg-turbo/inc)

# cmake-3.16
# target_link_directories(./libjpeg-turbo/lib/${TARGET_ABI})
# target_link_libraries(hal_jpege_stub jpeg-turbo mpp_base)

target_link_libraries(hal_jpege_stub ${HAL_JPEGE_STUB_LIBS} mpp_base)
set_target_properties(hal_jpege_stub PROPERTIES FOLDER "mpp/hal")
