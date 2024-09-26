cmake_minimum_required(VERSION 3.16.3)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

cmake_policy(SET CMP0091 NEW)

project(ENETCPP)

file(GLOB_RECURSE enet_sources enet/*.c enet/include/enet/*.h)
file(GLOB_RECURSE enetcpp_sources include/enetcpp/*.hpp)

add_library(enet-cpp ${enetcpp_sources} ${enet_sources})
target_include_directories(enet-cpp PUBLIC src include enet/include)

file( GLOB TEST_SOURCES test/*.cpp )
foreach( sourcefile ${TEST_SOURCES} )
    get_filename_component( name ${sourcefile} NAME_WE )
    add_executable( ${name} ${sourcefile} )
    target_link_libraries( ${name} enet-cpp)
endforeach( sourcefile ${TEST_SOURCES} )