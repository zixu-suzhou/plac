cmake_minimum_required(VERSION 3.10.1)

project(plac)
add_definitions(-std=c++11)

find_package(yaml-cpp REQUIRED)
include_directories(./include)
include_directories(${YAML_CPP_INCLUDE_DIR})

add_executable ( plac src/main.cpp)
target_link_libraries ( plac ${YAML_CPP_LIBRARIES} )
