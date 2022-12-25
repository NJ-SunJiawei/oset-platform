cmake_minimum_required(VERSION 3.00)

set(SRC_FILES
    mod_event_socket.c
)

add_library(libevent_sc SHARED ${SRC_FILES})
target_include_directories(libevent_sc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#target_compile_options(libevent_sc PUBLIC -D)
#target_link_libraries(libevent_sc )

set_target_properties(libevent_sc PROPERTIES
     OUTPUT_NAME "mod_event_socket"
)

install(TARGETS libevent_sc
        LIBRARY DESTINATION ${SSET_MOD_INSTALL_PATH}
		COMPONENT sset)



