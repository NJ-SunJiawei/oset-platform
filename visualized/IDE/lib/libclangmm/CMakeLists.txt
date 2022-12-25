cmake_minimum_required(VERSION 3.1)

project(clangmm)

if(APPLE)
  set(CMAKE_MACOSX_RPATH 1)
endif()

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  set(NOT_SUB_PROJECT 1)
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules")
find_package(LibClang REQUIRED)

option(BUILD_TESTING OFF)

if(${NOT_SUB_PROJECT})
  add_compile_options(-std=c++11 -Wall -Wextra)
  set(BUILD_TESTING ON)
endif()

add_subdirectory(src)

if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()
