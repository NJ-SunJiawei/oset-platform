cmake_minimum_required(VERSION 3.00)
project(FSPL)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CFLAG "$ENV{CFLAGS} -Wextra -fPIC")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -g3 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall -ffunction-sections -fdata-sections")

set(CMAKE_CXXFLAG "$ENV{CXXFLAGS} -Wextra -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g3 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(SRC_FILES
    FSPLModel.cpp
)

add_library(libFSPLModel SHARED ${SRC_FILES})
target_include_directories(libFSPLModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libFSPLModel m)

set_target_properties(libFSPLModel PROPERTIES
     OUTPUT_NAME "FSPLModel"
)



