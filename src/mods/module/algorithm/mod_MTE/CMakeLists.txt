cmake_minimum_required(VERSION 3.00)

set(LIB_MTE   ${SSET_THIRD_PATH}/libMTEnvironmentModel.so)

set(SRC_FILES
    mod_MTE.cpp
)

add_library(libMTE SHARED ${SRC_FILES})
target_include_directories(libMTE PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libMTE ${LIB_MTE})

set_target_properties(libMTE PROPERTIES
     OUTPUT_NAME "mod_MTE"
)

install(TARGETS libMTE
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



