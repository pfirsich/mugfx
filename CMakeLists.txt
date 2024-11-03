cmake_minimum_required(VERSION 3.3.0)

project(mugfx)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(MUGFX_BACKEND_OPTIONS OpenGL)
set(MUGFX_BACKEND OpenGL CACHE STRING "Backend")
set_property(CACHE MUGFX_BACKEND PROPERTY STRINGS ${MUGFX_BACKEND_OPTIONS})

include(wall)

option(MUGFX_ENABLE_ASAN "Enable ASan" OFF)
if (MUGFX_ENABLE_ASAN)
  include(asan)
endif()

set(MUGFX_SRC "src/shared.cpp")

if(MUGFX_BACKEND STREQUAL OpenGL)
  message("Building with OpenGL backend")
  list(APPEND MUGFX_SRC "src/opengl/opengl.cpp")

  add_library(glad "src/opengl/glad/src/glad.c")
  target_include_directories(glad SYSTEM PRIVATE "src/opengl/glad/include")
  target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
endif()

add_library(mugfx STATIC ${MUGFX_SRC})
target_include_directories(mugfx PUBLIC "include/")
target_compile_options(mugfx PUBLIC -Wno-unused-parameter)
set_wall(mugfx)

if(MUGFX_BACKEND STREQUAL OpenGL)
  target_compile_definitions(mugfx PRIVATE MUGFX_OPENGL)
  target_link_libraries(mugfx PRIVATE glad)
endif()

# This will only be true if this project is not used as a subdirectory (e.g. FetchContent)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  option(MUGFX_BUILD_EXAMPLES "Build Examples" ON)

  if(MUGFX_BUILD_EXAMPLES)
    add_subdirectory(examples)
  endif()
endif()