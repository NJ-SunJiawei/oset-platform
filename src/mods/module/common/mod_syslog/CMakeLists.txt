cmake_minimum_required(VERSION 3.00)

set(SRC_FILES
    mod_syslog.c
)

add_library(libsyslog SHARED ${SRC_FILES})
target_include_directories(libsyslog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(libsyslog PUBLIC -D)
#target_link_libraries(libsyslog )

set_target_properties(libsyslog PROPERTIES
     OUTPUT_NAME "mod_syslog"
)

install(TARGETS libsyslog
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



