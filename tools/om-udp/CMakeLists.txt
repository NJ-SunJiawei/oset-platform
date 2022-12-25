cmake_minimum_required(VERSION 3.00)

include_directories(${SSET_MODS_PATH}/application/app_om)

set(SRC1_FILES
    udp-pod.c
)
add_executable(udp-pod ${SRC1_FILES})
target_include_directories(udp-pod PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set(SRC2_FILES
    udp-omc.c
)
add_executable(udp-omc ${SRC2_FILES})
target_include_directories(udp-omc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(udp-omc pthread)
