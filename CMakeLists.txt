##############################################################
#create by sunjiawei 2022/4/19 
#V1.0.0-0 ----create         2022/4/19
#V1.1.0-1 ----support module 2022/6/2
#V1.1.0-2 ----support IDE    2022/8/25
##############################################################
cmake_minimum_required(VERSION 3.00)
project(sset_platform)

#set(OP_VERSION "1.1.0")
#set(CPACK_PACKAGE_NAME "sset_platform")
#set(CPACK_PACKAGE_CONTACT "<sunjiawei@.com>")
#set(CPACK_PACKAGE_VENDOR ${CPACK_PACKAGE_CONTACT})
#set(CPACK_PACKAGE_VERSION ${OP_VERSION})
#set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
#set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simulation  platform independent IDE.")
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
# 设置支持指定安装目录的控制为 ON                                   
#set(CPACK_SET_DESTDIR ON)
# 设置安装到的目录路径
#set(CPACK_INSTALL_PREFIX "/usr/local/install")  
##set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
#set(CPACK_DEBIAN_PACKAGE_DEPENDS "cmake, make, g++")
#set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://gitlab.com/")
#set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

#set(CPACK_GENERATOR "DEB")
##set(CPACK_GENERATOR "RPM")

# 安装前和安装后执行的shell脚本, 会打包到RPM/DEB中，安装时执行。这里可扩展性很强, 放在源码目录下即可
# set(CPACK_RPM_PRE_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pre_script.sh)
# set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/post_script.sh)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  message(STATUS "using ccache.")
else()
  message(STATUS "ccache was not found.")
endif(CCACHE_FOUND)

#include(CPack)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

###################
#版本控制
###################
# Version of the source code
set(FD_PROJECT_VERSION_MAJOR 1)
set(FD_PROJECT_VERSION_MINOR 1)
set(FD_PROJECT_VERSION_REV 0)
# Version of the API with the library
set(FD_PROJECT_VERSION_API 2)

###################
#获取操作系统
###################
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
   set(CURRENT_SYSTEM "Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
   set(CURRENT_SYSTEM "Windows")
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
   set(CURRENT_SYSTEM "FreeBSD")
endif()
cmake_host_system_information(RESULT UNIX_NAME QUERY OS_VERSION)
message(STATUS "${UNIX_NAME}")

#if(CMAKE_CL_64)
#   set(CURRENT_PLATFORM "x64")
#else(CMAKE_CL_64)
#   set(CURRENT_PLATFORM "x86")
#endif(CMAKE_CL_64)
message(STATUS "current platporm is ${CURRENT_SYSTEM}")
###################
#可视化开关
###################
set(GUI_SWITCH ON)
set(IDE_SWITCH ON)

###################
#调试模式开关
###################
set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CFLAG "$ENV{CFLAGS} -Wextra -fPIC")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -g3 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall -ffunction-sections -fdata-sections")

set(CMAKE_CXXFLAG "$ENV{CXXFLAGS} -Wextra -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g3 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

###################
#system path
###################
set(LINUX_HEADER_PATH        /usr/include)
set(LINUX_HEADER_PATH_LOCAL  /usr/local/include)
set(LINUX_LIB_PATH           /usr/lib
                             /usr/lib64)
set(LINUX_LIB_PATH_LOCAL     /usr/local/lib
                             /usr/local/lib64)
set(LINUX_LOG_PATH           /etc/logrotate.d) #日志轮转
set(LINUX_SERVICE_PATH       /usr/lib/systemd/system) #服务自启动
set(LINUX_NETWORK_PATH       /etc/systemd/network) #网络自启动
include_directories(${LINUX_HEADER_PATH_LOCAL})
#link_directories(${LINUX_LIB_PATH_LOCAL})
###################
#root path
###################
set(SSET_ROOT_PATH  ${CMAKE_CURRENT_SOURCE_DIR})
set(SSET_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR})

set(SSET_CLI_PATH    ${SSET_ROOT_PATH}/cli)
set(SSET_SRC_PATH    ${SSET_ROOT_PATH}/src)
set(SSET_MODS_PATH   ${SSET_SRC_PATH}/mods)
set(SSET_TOOLS_PATH  ${SSET_ROOT_PATH}/tools)
set(SSET_CNF_PATH    ${SSET_ROOT_PATH}/configs)
set(SSET_GUI_PATH    ${SSET_ROOT_PATH}/visualized/gui)
set(SSET_IDE_PATH    ${SSET_ROOT_PATH}/visualized/IDE)
set(SSET_THIRD_PATH  ${SSET_ROOT_PATH}/thirdparty)
###################
#install path
###################
set(INSTALL_ROOT ${CMAKE_INSTALL_PREFIX}/install/sset)
message(STATUS "sset INSTALL PATH = ${INSTALL_ROOT}")
set(SSET_INSTALL_PATH        ${INSTALL_ROOT})
set(SSET_CLI_INSTALL_PATH    ${SSET_INSTALL_PATH}/cli)
set(SSET_LIB_INSTALL_PATH    ${SSET_INSTALL_PATH}/lib)
set(SSET_RUN_INSTALL_PATH    ${SSET_INSTALL_PATH}/run)
set(SSET_MOD_INSTALL_PATH    ${SSET_INSTALL_PATH}/mod)
set(SSET_CNF_INSTALL_PATH    ${SSET_INSTALL_PATH}/conf)
set(SSET_LOG_INSTALL_PATH    ${SSET_INSTALL_PATH}/log)
set(SSET_DB_INSTALL_PATH     ${SSET_INSTALL_PATH}/db)
set(SSET_GUI_INSTALL_PATH    ${SSET_INSTALL_PATH}/gui)
set(SSET_THIRD_INSTALL_PATH  ${SSET_INSTALL_PATH}/thirdparty)
#file(MAKE_DIRECTORY ${SSET_THIRD_INSTALL_PATH})

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${SSET_LIB_INSTALL_PATH}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
###################
#uninstall path
###################
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

ADD_CUSTOM_TARGET(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
###################
#platform cmakeFind
###################
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
###################
#install configs
###################
install(DIRECTORY  ${SSET_CNF_PATH}/    DESTINATION  ${SSET_CNF_INSTALL_PATH})
if(GUI_SWITCH)
install(DIRECTORY  ${SSET_GUI_PATH}/    DESTINATION  ${SSET_GUI_INSTALL_PATH})
if(${CMAKE_SYSTEM_NAME} MATCHES Linux|.*BSD|DragonFly)
  install(FILES "${SSET_GUI_PATH}/share/openSET-ToolKit.desktop"
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications")
endif()
endif()
install(DIRECTORY  ${SSET_THIRD_PATH}/  DESTINATION  ${SSET_THIRD_INSTALL_PATH})
##############################################################
#************************************************************#
#platform编译
#************************************************************#
#src common include
include_directories(${SSET_SRC_PATH})
include_directories(${SSET_MODS_PATH})
###################
#sub path
###################
add_subdirectory(${SSET_CLI_PATH})
add_subdirectory(${SSET_SRC_PATH})
add_subdirectory(${SSET_MODS_PATH})
add_subdirectory(${SSET_TOOLS_PATH})
if(IDE_SWITCH)
add_subdirectory(${SSET_IDE_PATH})
endif()




