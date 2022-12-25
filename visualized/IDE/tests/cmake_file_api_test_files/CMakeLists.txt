cmake_minimum_required(VERSION 3.1)

project(test)
add_compile_options(-O0 -g)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/custom_build_folder)
add_executable(cmake_file_api_test_executable main.cpp)
set_target_properties(cmake_file_api_test_executable PROPERTIES OUTPUT_NAME "custom_executable")
