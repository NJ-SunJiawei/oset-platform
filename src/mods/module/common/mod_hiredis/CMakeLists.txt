cmake_minimum_required(VERSION 3.00)

FIND_PACKAGE(OSET-HIREDIS REQUIRED)

set(SRC_FILES
    mod_hiredis.c
)

add_library(libhiredis SHARED ${SRC_FILES})
target_include_directories(libhiredis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OSET_HIREDIS_INCLUDE_DIRS})
#target_compile_options(libhiredis PUBLIC -D)
target_link_libraries(libhiredis  ${OSET_HIREDIS_LIBRARIES})

set_target_properties(libhiredis PROPERTIES
     OUTPUT_NAME "mod_hiredis"
)

install(TARGETS libhiredis
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



