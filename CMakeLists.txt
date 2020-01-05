cmake_minimum_required(VERSION 3.0.0)
project(chester)

option (CGB "CGB support." ON)

if (CGB)
    message(STATUS "")
    message(STATUS " ************************")
    message(STATUS " * Enabled CGB support! *")
    message(STATUS " ************************")
    message(STATUS "")

    add_definitions(-DCGB)

    option (COLOR_CORRECTION "Emulate original display colors." ON)

    if (COLOR_CORRECTION)
        add_definitions(-DCOLOR_CORRECTION)
    endif()
endif ()

if (MSVC)
    add_compile_options(/WX /wd4996)
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")
endif()

add_subdirectory(src/lib)
add_subdirectory(src/app)

option (ROM_TESTS "Automated ROM tests with gtest." OFF)

if (ROM_TESTS)
    add_subdirectory(tests)
endif()
