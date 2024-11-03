include(../cmake/CPM.cmake)

CPMAddPackage(
  NAME SDL2
  VERSION 2.26.5
  URL "https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-2.26.5.zip"
  URL_HASH MD5=0664f3980570c4641128866e6c9f2e29
)

add_library(window window.cpp)
target_link_libraries(window PUBLIC SDL2::SDL2)
target_compile_definitions(window PUBLIC SDL_MAIN_HANDLED) # don't override main()

add_executable(hello_triangle hello_triangle.cpp)
target_link_libraries(hello_triangle PRIVATE mugfx window)