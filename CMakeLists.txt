project(tbak)
cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)

include_directories(.)
aux_source_directory(. SRC_LIST)
aux_source_directory(net SRC_LIST)
aux_source_directory(util SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} sodium z)
