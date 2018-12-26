add_executable(chester main.c)

if (NOT MSVC)
    set_target_properties(chester PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR}/cmake-modules)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake-modules/FindSDL2.cmake")
    file(DOWNLOAD "https://github.com/tcbrindle/sdl2-cmake-scripts/raw/master/FindSDL2.cmake" "${CMAKE_BINARY_DIR}/cmake-modules/FindSDL2.cmake")
endif()

find_package(SDL2 REQUIRED)

target_include_directories(chester PRIVATE ${SDL2_INCLUDE_DIR})
target_link_libraries(chester libchester ${SDL2_LIBRARY})
