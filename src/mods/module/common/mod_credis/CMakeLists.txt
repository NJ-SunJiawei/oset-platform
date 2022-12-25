cmake_minimum_required(VERSION 3.00)

FIND_PACKAGE(OSET-CREDIS REQUIRED)

set(SRC_FILES
    mod_credis.c
)

add_library(libcredis SHARED ${SRC_FILES})
target_include_directories(libcredis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OSET_CREDIS_INCLUDE_DIRS})
#target_compile_options(libcredis PUBLIC -D)
target_link_libraries(libcredis  ${OSET_CREDIS_LIBRARIES})

set_target_properties(libcredis PROPERTIES
     OUTPUT_NAME "mod_credis"
)

install(TARGETS libcredis
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



