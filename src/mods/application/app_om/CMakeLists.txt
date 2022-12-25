cmake_minimum_required(VERSION 3.00)

FIND_PACKAGE(OSET-RTMQ REQUIRED)

set(SRC_FILES
    om.c
    om-session.c
    om-routine.c
    om-massive.c
)

add_library(libom SHARED ${SRC_FILES})
target_include_directories(libom PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OSET_RTMQ_INCLUDE_DIRS})
#target_compile_options(libom PUBLIC -D )
target_link_libraries(libom ${OSET_RTMQ_LIBRARIES})

set_target_properties(libom PROPERTIES
     OUTPUT_NAME "app_om"
)

install(TARGETS libom
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)


