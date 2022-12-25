cmake_minimum_required(VERSION 3.00)

set(JUCI_VERSION "1.0.0")

set(CMAKE_CXX_STANDARD 14)

add_compile_options(-pthread -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations)
add_definitions(-DJUCI_VERSION="${JUCI_VERSION}")
if(CMAKE_BUILD_TYPE STREQUAL "")
  add_compile_options(-O3)
endif()

# temporarily disable these warnings: Only <gtksourceview/gtksource.h> can be included directly.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  add_compile_options(-Wno-cpp)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options("-Wno-#warnings")
  add_compile_options(-Wthread-safety -Wno-deprecated -Wstring-conversion -Wliteral-conversion)
endif()

if(APPLE)
  # Added if expressions to avoid linking warnings:
  if(EXISTS /usr/local/lib)
    link_directories(/usr/local/lib)
  endif()
  if(EXISTS /usr/local/opt/gettext/lib)
    link_directories(/usr/local/opt/gettext/lib)
  endif()
  if(EXISTS /usr/local/opt/libsigc++@2/lib)
    link_directories(/usr/local/opt/libsigc++@2/lib)
  endif()
  if(EXISTS /opt/homebrew/lib)
    link_directories(/opt/homebrew/lib)
  endif()

  include_directories(/usr/local/opt/gettext/include)
  set(CMAKE_MACOSX_RPATH 1)
  set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/libsigc++@2/lib/pkgconfig:/usr/local/opt/zlib/lib/pkgconfig:/usr/local/opt/libxml2/lib/pkgconfig")
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES FreeBSD)
  link_directories(/usr/local/lib)
  link_directories(${CMAKE_INSTALL_PREFIX}/lib)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES NetBSD)
  link_directories(/usr/pkg/lib)
  link_directories(${CMAKE_INSTALL_PREFIX}/lib)
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()

option(BUILD_TESTING "Build tests")
option(BUILD_FUZZING "Build tests")
option(LIBCLANG_PATH "Use custom path for libclang")
option(LIBLLDB_PATH "Use custom path for liblldb")

find_package(Boost 1.54 COMPONENTS REQUIRED filesystem serialization)
find_package(ASPELL REQUIRED)
include(FindPkgConfig)
pkg_check_modules(GTKMM gtkmm-3.0 REQUIRED)
pkg_check_modules(GTKSVMM gtksourceviewmm-3.0 REQUIRED)
pkg_check_modules(LIBGIT2 libgit2 REQUIRED)

include(FindPackageHandleStandardArgs)

if(LIBCLANG_PATH)
  find_library(LIBCLANG_LIBRARIES clang "${LIBCLANG_PATH}/lib" NO_DEFAULT_PATH)
  set(LIBCLANG_INCLUDE_DIRS "${LIBCLANG_PATH}/include")
  if(LIBCLANG_LIBRARIES)
    find_package_handle_standard_args(libclang DEFAULT_MSG LIBCLANG_LIBRARIES LIBCLANG_INCLUDE_DIRS)
  else()
    message(FATAL_ERROR "libclang not found")
  endif()
else()
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/lib/libclangmm/cmake_modules/")
  find_package(LibClang REQUIRED)
endif()

set(BUILD_TESTING_SAVED ${BUILD_TESTING})
set(BUILD_TESTING OFF CACHE BOOL "Disable sub-project tests" FORCE)
add_subdirectory(lib/tiny-process-library)
add_subdirectory(lib/libclangmm)
set(BUILD_TESTING ${BUILD_TESTING_SAVED} CACHE BOOL "Set to previous value" FORCE)

if(LIBLLDB_PATH)
  find_library(LIBLLDB_LIBRARIES lldb "${LIBLLDB_PATH}/lib" NO_DEFAULT_PATH)
  set(LIBLLDB_INCLUDE_DIRS "${LIBLLDB_PATH}/include")
else()
  # Find liblldb with the same version as the version of libclang found
  string(REPLACE libclang liblldb LIBLLDB_LIBRARIES "${LIBCLANG_LIBRARIES}")
  set(LIBLLDB_INCLUDE_DIRS ${LIBCLANG_INCLUDE_DIRS})
endif()
if(EXISTS "${LIBLLDB_LIBRARIES}")
  set(LIBLLDB_FOUND TRUE)
elseif(EXISTS "${LIBLLDB_LIBRARIES}.1")
  set(LIBLLDB_LIBRARIES "${LIBLLDB_LIBRARIES}.1")
  set(LIBLLDB_FOUND TRUE)
endif()
if(LIBLLDB_FOUND)
  add_definitions(-DJUCI_ENABLE_DEBUG)
  include_directories(${LIBLLDB_INCLUDE_DIRS})
  find_package_handle_standard_args(liblldb DEFAULT_MSG LIBLLDB_LIBRARIES LIBLLDB_INCLUDE_DIRS)
else()
  set(LIBLLDB_LIBRARIES "")
  message("liblldb not found. Building juCi++ without debugging support")
endif()

if(CMAKE_SYSTEM_NAME MATCHES .*BSD|DragonFly)
  add_definitions(-DJUCI_USE_UCTAGS) # See https://svnweb.freebsd.org/ports?view=revision&revision=452957
  add_definitions(-DJUCI_USE_GREP_EXCLUDE) # --exclude-dir is not an argument in bsd grep
endif()

# For both src and tests targets
include_directories(
  ${Boost_INCLUDE_DIRS}
  ${GTKMM_INCLUDE_DIRS}
  ${GTKSVMM_INCLUDE_DIRS}
  ${LIBCLANG_INCLUDE_DIRS}
  ${ASPELL_INCLUDE_DIR}
  ${LIBGIT2_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/json/include
)

if(${CMAKE_SYSTEM_NAME} MATCHES Linux|.*BSD|DragonFly)
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/share/openSET-Dev.desktop"
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/applications")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/share/openSET-Dev.png"
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps")
  install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/share/openSET-Dev.svg"
          DESTINATION "${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps")
elseif(APPLE)
  install(CODE "execute_process(COMMAND /usr/bin/python ${CMAKE_CURRENT_SOURCE_DIR}/share/set_icon_macos.py ${CMAKE_CURRENT_SOURCE_DIR}/share/juci.png ${CMAKE_INSTALL_PREFIX}/bin/juci)")
endif()

# add a target to generate API documentation with Doxygen
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/share/cmake_modules/")
find_package(Plantuml)
find_package(Doxygen)
if(DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen to ${CMAKE_CURRENT_BINARY_DIR}" VERBATIM
  )
endif(DOXYGEN_FOUND)

add_subdirectory(src)

if(BUILD_TESTING OR BUILD_FUZZING)
  if(BUILD_TESTING)
    enable_testing()
  endif()
  add_subdirectory(tests)
endif()
