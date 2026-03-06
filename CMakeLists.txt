cmake_minimum_required(VERSION 3.14)
project(ShaderAnimation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

add_executable(shader_app
  src/app_options.cpp
  src/frame_profiler.cpp
  src/main.cpp
  src/performance_profiler.cpp
  src/shader.cpp
)

target_link_libraries(shader_app PRIVATE glfw)

if(NOT APPLE)
  message(WARNING "Non-Apple platforms currently require GLAD; please add a loader or enable FetchContent for GLAD.")
endif()

if(APPLE)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(COREVIDEO_LIBRARY CoreVideo)
  find_library(OPENGL_LIBRARY OpenGL)
  target_link_libraries(shader_app PRIVATE ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY} ${OPENGL_LIBRARY})
endif()
