add_executable(hello_triangle hello_triangle.cpp window.cpp)
target_link_libraries(hello_triangle PRIVATE mugfx)
target_compile_definitions(hello_triangle PUBLIC SDL_MAIN_HANDLED) # don't override main()

if(NOT EMSCRIPTEN)
  include(../cmake/CPM.cmake)

  CPMAddPackage(
    NAME SDL2
    VERSION 2.26.5
    URL "https://github.com/libsdl-org/SDL/releases/download/release-2.26.5/SDL2-2.26.5.zip"
    URL_HASH MD5=0664f3980570c4641128866e6c9f2e29
  )

  target_link_libraries(hello_triangle PUBLIC SDL2::SDL2)
else()
  # USE_SDL is both a compile and link flag!
  target_compile_options(hello_triangle PRIVATE
    -sUSE_SDL=2
  )

  # Instead of INITIAL_HEAP, use ALLOW_MEMORY_GROWTH if appropriate, though it may cause stuttering
  target_link_options(hello_triangle PRIVATE
    -sUSE_SDL=2
    -sINITIAL_HEAP=8mb
    # --shell-file path/to/shell.html # https://emscripten.org/docs/compiling/Deploying-Pages.html
  )

  set_target_properties(hello_triangle PROPERTIES SUFFIX ".html")
endif()
