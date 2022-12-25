cmake_minimum_required(VERSION 3.00)

set(SRC_FILES
    mod_console.c
)

add_library(libconsole SHARED ${SRC_FILES})
target_include_directories(libconsole PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(libconsole PUBLIC -D)
#target_link_libraries(libconsole )

set_target_properties(libconsole PROPERTIES
     OUTPUT_NAME "mod_console"
)

install(TARGETS libconsole
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



