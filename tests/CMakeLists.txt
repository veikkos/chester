cmake_minimum_required(VERSION 3.0.0)

project (rom-tests)

# See https://github.com/google/googletest/tree/master/googletest#incorporating-into-an-existing-cmake-project
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
    EXCLUDE_FROM_ALL)

# Get Blargg's test ROMs
include(ExternalProject)

ExternalProject_Add(blarggs-roms
    PREFIX ${CMAKE_BINARY_DIR}/blarggs-roms
    GIT_REPOSITORY https://github.com/retrio/gb-test-roms.git
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(blarggs-roms SOURCE_DIR)
set(BLARGGS_ROMS_DIR ${SOURCE_DIR})
unset(SOURCE_DIR)

# Get Gekkio's test ROMs
ExternalProject_Add(gekkios-roms
    PREFIX ${CMAKE_BINARY_DIR}/gekkios-roms
    URL https://gekkio.fi/files/mooneye-gb/latest/mooneye-gb_hwtests.zip
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(gekkios-roms SOURCE_DIR)
set(GEKKIOS_ROMS_DIR ${SOURCE_DIR})
unset(SOURCE_DIR)

# Create test executable
add_executable(${PROJECT_NAME}
    blarggs-tests.cpp
    gekkios-tests.cpp
    test-runner.cpp
    test-runner.hpp)

target_compile_features(${PROJECT_NAME}
    PUBLIC cxx_std_11)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    BLARGGS_ROMS_DIR=\"${BLARGGS_ROMS_DIR}\"
    GEKKIOS_ROMS_DIR=\"${GEKKIOS_ROMS_DIR}\")

target_link_libraries(${PROJECT_NAME}
    gtest_main
    libchester)

add_dependencies(${PROJECT_NAME}
    gekkios-roms
    blarggs-roms)
