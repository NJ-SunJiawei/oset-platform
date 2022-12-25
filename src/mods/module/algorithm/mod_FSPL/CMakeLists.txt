cmake_minimum_required(VERSION 3.00)

#file(GLOB LIB_FSPL   ${SSET_THIRD_INSTALL_PATH}/*.so)
#link_directories(${LIB_FSPL})

set(LIB_FSPL   ${SSET_THIRD_PATH}/libFSPLModel.so)

set(SRC_FILES
    mod_FSPL.c
)

add_library(libFSPL SHARED ${SRC_FILES})
target_include_directories(libFSPL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(libFSPL PUBLIC -D)
target_link_libraries(libFSPL ${LIB_FSPL})

set_target_properties(libFSPL PROPERTIES
     OUTPUT_NAME "mod_FSPL"
)

install(TARGETS libFSPL
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



