cmake_minimum_required(VERSION 3.00)

set(SRC_FILES
    mod_logfile.c
)

add_library(liblogfile SHARED ${SRC_FILES})
target_include_directories(liblogfile PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(liblogfile PUBLIC -D)
#target_link_libraries(liblogfile )

set_target_properties(liblogfile PROPERTIES
     OUTPUT_NAME "mod_logfile"
)

install(TARGETS liblogfile
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



