include(CheckIPOSupported)

add_library(pck
  pck/PckFile.h pck/PckFile.cpp
  PckTool.h PckTool.cpp
  FileFilter.h FileFilter.cpp
  "${PROJECT_BINARY_DIR}/Include.h" Define.h
  )

target_link_libraries(pck PUBLIC)
target_link_libraries(pck PRIVATE md5)

set_target_properties(pck PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  )


add_executable(godotpcktool main.cpp)

target_link_libraries(godotpcktool PRIVATE pck)

# Static standard lib
target_link_libraries(godotpcktool PRIVATE -static-libgcc -static-libstdc++)

# Fully static executable
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_link_libraries(godotpcktool PRIVATE -static)
endif()

if(NOT WIN32)
  set_target_properties(godotpcktool PROPERTIES LINK_FLAGS_RELEASE "-s -fuse-ld=gold")
else()
  set_target_properties(godotpcktool PROPERTIES LINK_FLAGS_RELEASE "-s")
endif()

set_target_properties(godotpcktool PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  )

check_ipo_supported(RESULT result)
if(result)
  set_target_properties(godotpcktool PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS godotpcktool)
