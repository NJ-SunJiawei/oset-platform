cmake_minimum_required(VERSION 3.00)

include_directories(${LINUX_HEADER_PATH}/libmongoc-1.0/mongoc
                    ${LINUX_HEADER_PATH}/libmongoc-1.0
					${LINUX_HEADER_PATH}/libbson-1.0)
set(SRC_FILES
    mod_mongo.c
)

add_library(libmongo SHARED ${SRC_FILES})
target_include_directories(libmongo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(libmongo PUBLIC -D)
target_link_libraries(libmongo mongoc-1.0)

set_target_properties(libmongo PROPERTIES
     OUTPUT_NAME "mod_mongo"
)

install(TARGETS libmongo
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



