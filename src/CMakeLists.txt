add_library(pck
  pck/GodotCompatibility.h
  pck/file_access_pack.h pck/file_access_pack.cpp
  PackTool.h PackTool.cpp
  Include.h Define.h
  )

target_link_libraries(pck PUBLIC)

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

set_target_properties(godotpcktool PROPERTIES LINK_FLAGS_RELEASE -s)

set_target_properties(godotpcktool PROPERTIES
  CXX_STANDARD 17
  CXX_EXTENSIONS OFF
  )

install(TARGETS godotpcktool)
