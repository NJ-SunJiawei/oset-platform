cmake_minimum_required(VERSION 3.00)

set(SSET_VERSION  ${FD_PROJECT_VERSION_MAJOR}.${FD_PROJECT_VERSION_MINOR}.${FD_PROJECT_VERSION_REV})
configure_file(version.h.in  version.h)

set(SRC_FILES
    sset-core.c
)

add_executable(openSET ${SRC_FILES} sset.c)
target_include_directories(openSET PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${OSET_RTMQ_INCLUDE_DIRS})
#target_compile_options(openSET PUBLIC -D )
target_link_libraries(openSET ${OSET_RTMQ_LIBRARIES})
target_link_libraries(openSET pthread)

install(TARGETS openSET
        RUNTIME DESTINATION ${SSET_RUN_INSTALL_PATH}
		COMPONENT sset)

install(TARGETS openSET
        RUNTIME DESTINATION bin
		COMPONENT sset)

