cmake_minimum_required(VERSION 2.8.12)
project(chester)

option (EXPERIMENTAL_CGB "Experimental CGB support." OFF)

if (EXPERIMENTAL_CGB)
    message(STATUS "")
    message(STATUS " *************************************")
    message(STATUS " * Enabled experimental CGB support! *")
    message(STATUS " ************************************* ")
    message(STATUS "")

    add_definitions(-DEXPERIMENTAL_CGB)
endif ()

if (MSVC)
    add_compile_options(/WX /wd4996)
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
endif()

add_subdirectory(src/lib)
add_subdirectory(src/app)
