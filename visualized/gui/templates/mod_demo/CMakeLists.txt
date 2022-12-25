cmake_minimum_required(VERSION 3.00)

project(MODULE_@TEMPLATE@)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CFLAG "$ENV{CFLAGS} -Wextra -fPIC")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -g3 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall -ffunction-sections -fdata-sections")

set(CMAKE_CXXFLAG "$ENV{CXXFLAGS} -Wextra -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g3 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(INSTALL_ROOT ${CMAKE_INSTALL_PREFIX}/install/sset)
message(STATUS "sset INSTALL PATH = ${INSTALL_ROOT}")
set(SSET_INSTALL_PATH        ${INSTALL_ROOT})
set(SSET_MOD_INSTALL_PATH    ${SSET_INSTALL_PATH}/mod)
set(SSET_CNF_INSTALL_PATH    ${SSET_INSTALL_PATH}/conf/autoload)
set(SSET_LIB_INSTALL_PATH    ${SSET_INSTALL_PATH}/lib)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${SSET_LIB_INSTALL_PATH}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
FIND_PACKAGE(OSET REQUIRED)
include_directories(${OSET_INCLUDE_DIRS})
link_libraries(${OSET_LIBRARIES})

FIND_PACKAGE(APR REQUIRED)
FIND_PACKAGE(APRUtil REQUIRED)
include_directories(${APR_INCLUDE_DIRS}
                    ${APRUtil_INCLUDE_DIRS})
link_libraries(${APR_LIBRARIES}
               ${APRUtil_LIBRARIES})
			   
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

ADD_CUSTOM_TARGET(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
###########################################################################################

set(SRC_FILES
    mod_@template@.c
)

add_library(lib@template@ SHARED ${SRC_FILES})
target_include_directories(lib@template@ PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lib@template@  ${OSET_CREDIS_LIBRARIES})

set_target_properties(lib@template@ PROPERTIES
     OUTPUT_NAME "app_@template@"
)

install(TARGETS lib@template@
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)
		
install(FILES @template@.conf.xml
        DESTINATION ${SSET_CNF_INSTALL_PATH}
		COMPONENT sset)


