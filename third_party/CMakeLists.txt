# Third party libs


# md5 project doesn't use cmake, so here's a basic definition
if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
  set(MD5_BIG_ENDIAN 1)
else()
  set(MD5_BIG_ENDIAN 0)
endif()

configure_file("MD5-conf/conf.h.in" "${PROJECT_BINARY_DIR}/MD5-conf/conf.h")

add_library(md5 STATIC MD5/src/md5.h MD5/src/md5.cpp MD5/src/md5_loc.h)
target_include_directories(md5 PRIVATE "${PROJECT_BINARY_DIR}/MD5-conf")
target_include_directories(md5 INTERFACE MD5/src)
