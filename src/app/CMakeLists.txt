add_executable(chester main.c)

find_library(sdl2_location SDL2)
target_link_libraries(chester LINK_PUBLIC ${sdl2_location})

target_link_libraries(chester LINK_PUBLIC libchester)
