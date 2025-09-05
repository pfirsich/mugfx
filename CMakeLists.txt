cmake_minimum_required(VERSION 3.3.0)

project(mugfx)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(MUGFX_BACKEND_OPTIONS OpenGL WebGL)
if(EMSCRIPTEN)
  set(MUGFX_BACKEND WebGL CACHE STRING "Backend")
else()
  set(MUGFX_BACKEND OpenGL CACHE STRING "Backend")
endif()
set_property(CACHE MUGFX_BACKEND PROPERTY STRINGS ${MUGFX_BACKEND_OPTIONS})
message(STATUS "Building with backend: ${MUGFX_BACKEND}")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/wall.cmake)

option(MUGFX_ENABLE_ASAN "Enable ASan" OFF)
if (MUGFX_ENABLE_ASAN AND NOT EMSCRIPTEN)
  include(${CMAKE_CURRENT_LIST_DIR}/cmake/asan.cmake)
endif()

set(MUGFX_SRC "src/shared.cpp")

if(MUGFX_BACKEND STREQUAL OpenGL)
  list(APPEND MUGFX_SRC "src/opengl/opengl.cpp")

  add_library(glad "src/opengl/glad/src/gl.c")
  target_include_directories(glad SYSTEM PRIVATE "src/opengl/glad/include")
  target_link_libraries(glad PUBLIC ${CMAKE_DL_LIBS})
endif()

if(MUGFX_BACKEND STREQUAL WebGL)
  list(APPEND MUGFX_SRC "src/opengl/opengl.cpp")
endif()

add_library(mugfx STATIC ${MUGFX_SRC})
target_include_directories(mugfx PUBLIC "include/")
target_compile_options(mugfx PUBLIC -Wno-unused-parameter)
set_wall(mugfx)

if(MUGFX_BACKEND STREQUAL OpenGL)
  target_compile_definitions(mugfx PRIVATE MUGFX_OPENGL)
  target_link_libraries(mugfx PUBLIC glad)
endif()

if(MUGFX_BACKEND STREQUAL WebGL)
  find_package(OpenGL REQUIRED COMPONENTS GL)

  target_include_directories(mugfx PUBLIC ${OPENGL_INCLUDE_DIRS})
  target_link_libraries(mugfx PUBLIC OpenGL::GL)
  target_compile_definitions(mugfx PUBLIC MUGFX_WEBGL)

  if (EMSCRIPTEN)
    target_link_options(mugfx PUBLIC
      -sMAX_WEBGL_VERSION=2
      -sMIN_WEBGL_VERSION=2
      -sGL_EXPLICIT_UNIFORM_BINDING=1 # required for binding layout specifiers
    )
  endif()
endif()

# This will only be true if this project is not used as a subdirectory (e.g. FetchContent)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  option(MUGFX_BUILD_EXAMPLES "Build Examples" ON)

  if(MUGFX_BUILD_EXAMPLES)
    add_subdirectory(examples)
  endif()
endif()
