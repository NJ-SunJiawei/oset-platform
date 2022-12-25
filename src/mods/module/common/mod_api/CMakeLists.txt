cmake_minimum_required(VERSION 3.00)

set(SRC_FILES
    mod_api.c
)

add_library(libapi SHARED ${SRC_FILES})
target_include_directories(libapi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(libapi PUBLIC -D)
#target_link_libraries(libapi )

set_target_properties(libapi PROPERTIES
     OUTPUT_NAME "mod_api"
)

install(TARGETS libapi
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



