cmake_minimum_required(VERSION 3.00)

set(SRC_FILES
    mod_app.c
)

add_library(libapp SHARED ${SRC_FILES})
target_include_directories(libapp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(libapp PUBLIC -D)
#target_link_libraries(libapp )

set_target_properties(libapp PROPERTIES
     OUTPUT_NAME "mod_app"
)

install(TARGETS libapp
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



