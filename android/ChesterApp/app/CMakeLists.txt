cmake_minimum_required(VERSION 3.4.1)

file(GLOB ChesterSrcs ../../../src/lib/*.c)

add_definitions(-DCGB -DCOLOR_CORRECTION -DRGBA8888)

add_library(native-lib
            SHARED
            ${ChesterSrcs}
            src/main/cpp/native-lib.cpp)

include_directories(../../../src/lib)
